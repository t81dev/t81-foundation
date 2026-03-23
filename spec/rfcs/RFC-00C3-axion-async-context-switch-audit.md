# RFC-00C3: Axion Async Context Switch Audit Trail

**Status:** accepted
**Type:** standards-track
**Applies-To:** TernaryOS governance layer, freestanding scheduler, KernelCall observability
**Created:** 2026-03-23
**Author:** @t81dev
**Depends on:** RFC-00C2 (Hardware-Interrupt-Driven WaitForDevice Wake), RFC-00BF (Freestanding KernelCall Observability)
**Closes:** OQ-5 (Axion determinism under pre-emption)
**Blocks:** RFC-00C4 (Per-Device Wake Filtering)

---

## Summary

RFC-00C2 introduced async context switches driven by the GICv3 timer IRQ: when
`fs_sched_timer_device_wake()` transitions a thread from `BlockedDeviceWait →
Runnable` and `fs_sched_device_wait_loop()` ERets to it, no governance event is
recorded.  The KernelCall observability ring (RFC-00BF) captures only SVC #1
dispatches — it is blind to IRQ-driven transitions.

This RFC adds a **governance audit ring** (`FsGovRecord`, 16 slots) that records
every async context switch event.  Two event types are defined:

- `kGovTimerDeviceWake` (1) — timer ISR woke a `BlockedDeviceWait` thread.
- `kGovAsyncContextSwitch` (2) — `fs_sched_device_wait_loop` EReted to a thread.

The obs ring (RFC-00BF) and the gov ring together provide a complete, ordered
audit trail of all scheduler transitions — synchronous (SVC-driven) and
asynchronous (IRQ-driven) — closing OQ-5.

The Phase 14 CI gate verifies both gov ring events after the Phase 13 IRQ-wake
roundtrip.

---

## Motivation: OQ-5

OQ-5 states:

> Axion determinism under pre-emption — governance model must be extended for
> async context switches.

The Axion policy engine enforces governance by intercepting every `AgentInvoke`
opcode and every SVC #1 KernelCall.  Both are synchronous: they occur at a
well-defined program point, with known caller context.

RFC-00C2 broke this invariant.  The timer ISR can now:
1. Transition a thread from `BlockedDeviceWait → Runnable` at any point during
   EL1 execution.
2. Cause `fs_sched_device_wait_loop` to ERET to EL0 without going through the
   KernelCall dispatch path.

Neither event is currently visible to Axion.  If the audit trail is missing
async wakes, an adversarial binary could:
- Call `WaitForDevice` to park itself.
- Be woken by the timer without any policy check.
- Resume execution with a clean audit trail despite having bypassed governance.

The gov ring closes this gap.  It does not reproduce exact timer timing (which
would violate determinism) but records the *occurrence* and *ordering* of async
events relative to synchronous KernelCall events via `obs_seq_at` (the obs ring
sequence counter at the time of the async event).

---

## Governance Ring Format

### `FsGovRecord` (16 bytes)

```
[0:4]   seq_id      uint32_t  monotonic across all gov events (global counter)
[4:8]   tid         uint32_t  thread that was woken / context-switched to
[8:12]  event       uint32_t  kGovTimerDeviceWake or kGovAsyncContextSwitch
[12:16] obs_seq_at  uint32_t  value of s_obs_seq at time of this gov event
                              (logical timestamp; orders gov events relative to
                               KernelCall events in the obs ring)
```

### Ring Parameters

```
Capacity : 16 slots  (256 bytes total)
Behaviour: oldest entry overwritten when full (same as obs ring)
Reset    : fs_gov_reset() called from fs_sched_reset()
```

### Event Types

| Constant | Value | Emitted from | Meaning |
|---|---|---|---|
| `kGovTimerDeviceWake` | 1 | `fs_sched_timer_device_wake()` | Timer ISR transitioned tid from BlockedDeviceWait → Runnable |
| `kGovAsyncContextSwitch` | 2 | `fs_sched_device_wait_loop()` | idle loop EReted to tid at EL0 (async switch) |

---

## API

```cpp
// Governance ring query API (qemu_slice6_el0_svc_bridge.cpp).

// Reset the gov ring — called from fs_sched_reset().
extern "C" void fs_gov_reset() noexcept;

// Number of gov events recorded since last reset (monotonic counter).
extern "C" uint64_t fs_gov_count() noexcept;

// Return true iff any gov record matches (tid, event).
extern "C" bool fs_gov_find(uint32_t tid, uint32_t event) noexcept;
```

`fs_gov_reset()` is forward-declared before `fs_sched_reset()` (same pattern
as `fs_obs_reset()`) and called within it.

---

## Recording Discipline

| Caller | Event recorded | Condition |
|---|---|---|
| `fs_sched_timer_device_wake()` | `kGovTimerDeviceWake` for each tid woken | Each `BlockedDeviceWait → Runnable` transition |
| `fs_sched_device_wait_loop()` | `kGovAsyncContextSwitch` for the woken tid | Immediately before the inline ERET to EL0 |

The `obs_seq_at` field is set to the current value of `s_obs_seq` (the obs ring
monotonic counter, accessible within the same TU) at the time of the call.
This timestamps gov events relative to KernelCall events without requiring
wall-clock time.

---

## Phase 14 CI Sequence

```
[Phase 13 completes — Process D (tid=5) has exited; obs ring has WaitForDevice]

EL1: canon_irq_wake_load_and_run() (after Phase 13b obs check)
  ├─ Phase 14: fs_gov_find(5u, kGovTimerDeviceWake)    → true
  ├─ Phase 14: fs_gov_find(5u, kGovAsyncContextSwitch) → true
  └─ emit "[axion] el0: async audit OK (AsyncWake tid=5, irq-driven)"

CI gate (added to Validate boot sequence step):
  check "[axion] el0: async audit OK (AsyncWake tid=5, irq-driven)"
```

The gov ring is populated during Phase 13 (same `run_proc_entry()` call):
- `fs_sched_timer_device_wake()` → records `kGovTimerDeviceWake` for tid=5.
- `fs_sched_device_wait_loop()` → records `kGovAsyncContextSwitch` for tid=5.

Phase 14 verification reads these records without re-running the EL0 binary.

---

## OQ-5 Resolution

OQ-5 is closed by this RFC because:

1. **Complete coverage**: every async wake (`fs_sched_timer_device_wake`) and
   every async ERET (`fs_sched_device_wait_loop`) is recorded.  No async
   scheduler transition is audit-silent.

2. **Ordered relative to synchronous events**: `obs_seq_at` cross-references
   gov events against the obs ring, enabling a full causal ordering of all
   scheduler events without wall-clock timestamps.

3. **CI-gated**: the Phase 14 CI gate fails if any expected async event is
   missing, making audit completeness a hard build requirement.

4. **Governance-extensible**: the gov ring API (`fs_gov_find`, `fs_gov_count`)
   is the hook point for future Axion policy checks.  A policy can query the
   gov ring to validate that async transitions match declared behavioral
   contracts (RFC-00C5 territory).

What OQ-5 does **not** require, and what this RFC explicitly defers:
- Bit-exact reproducibility of *which timer tick* woke the thread.  Axion
  determinism governs the *outcome* of transitions (correct state, correct
  audit record) not the *timing* of hardware interrupts.
- Full Axion policy enforcement on async transitions (deferred to RFC-00C5).

---

## Acceptance Criteria

- [x] `FsGovRecord` (16 bytes) defined; `s_gov_ring[16]`, `s_gov_seq` in
  `qemu_slice6_el0_svc_bridge.cpp`
- [x] `fs_gov_record()` (static), `fs_gov_reset()`, `fs_gov_count()`,
  `fs_gov_find()` implemented
- [x] `fs_gov_reset()` forward-declared before `fs_sched_reset()`; called
  from `fs_sched_reset()`
- [x] `fs_sched_timer_device_wake()` records `kGovTimerDeviceWake` per woken tid
- [x] `fs_sched_device_wait_loop()` records `kGovAsyncContextSwitch` before ERET
- [x] `extern "C"` declarations for gov API in `canon_exec_loader.cpp`
- [x] Phase 14 verification in `canon_irq_wake_load_and_run()`: both gov records
  confirmed; emits Phase 14 gate
- [x] Phase 14 CI gate: `[axion] el0: async audit OK (AsyncWake tid=5, irq-driven)`
- [x] OQ-5 closed in `userland/experimental/docs/PROGRESS.md`
- [ ] RFC-00C4: per-device wake filtering (`device_id` in `FsSchedThread` + wire request)
- [ ] RFC-00C5: Axion policy enforcement on async transitions via gov ring

---

## Relationship to Other RFCs

- **RFC-00BE** — defines `BlockedDeviceWait`; gov ring records transitions *out* of it.
- **RFC-00BF** — obs ring records SVC-driven events; gov ring records IRQ-driven events.
  Together they form the complete scheduler audit trail.
- **RFC-00C2** — introduces the async paths that this RFC audits.
- **RFC-00C4** — per-device filtering; gov ring event will include `device_id` field.
- **RFC-00C5** — Axion policy enforcement hooks into `fs_gov_find()` / gov ring.

---

## References

- [RFC-00BE: Freestanding Cooperative Scheduler](RFC-00BE-freestanding-cooperative-scheduler.md)
- [RFC-00BF: Freestanding KernelCall Observability](RFC-00BF-freestanding-kernelcall-observability.md)
- [RFC-00C2: Hardware-Interrupt-Driven WaitForDevice Wake](RFC-00C2-hardware-interrupt-driven-waitfordevice.md)
- `userland/experimental/hal/qemu_slice6_el0_svc_bridge.cpp` — gov ring impl
- `userland/experimental/hal/canon_exec_loader.cpp` — Phase 14 verification
- `userland/experimental/docs/PROGRESS.md` — OQ-5
