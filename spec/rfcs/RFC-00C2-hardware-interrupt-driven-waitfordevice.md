# RFC-00C2: Hardware-Interrupt-Driven WaitForDevice Wake

**Status:** accepted
**Type:** standards-track
**Applies-To:** TernaryOS freestanding scheduler, GICv3 timer IRQ, KernelCall SVC bridge
**Created:** 2026-03-23
**Author:** @t81dev
**Depends on:** RFC-00C1 (CanonFS Per-Binary Call Sequence Manifest), RFC-00BE (Freestanding Cooperative Scheduler), RFC-00BD (KernelCall ABI Ordinal Freeze)
**Blocks:** RFC-00C3 (Multi-Thread IRQ Wake Arbitration)

---

## Summary

This RFC wires the GICv3 physical timer IRQ (PPI30, ~100 Hz) into the
freestanding cooperative scheduler so that a thread parked on `WaitForDevice`
(kind=43) is woken by real hardware rather than by a direct EL1 API call.

Three new components are added:

1. **`fs_sched_timer_device_wake()`** — called from `axion_irq_handler_aarch64()`
   on every timer tick; transitions all `BlockedDeviceWait` threads to `Runnable`.

2. **`fs_sched_device_wait_loop()`** — an EL1 `wfi` idle function.  When the
   `WaitForDevice` SVC handler parks the last thread and `g_axion_el1_device_wait_pc`
   is set, the handler redirects ERET to this function instead of returning to EL1
   immediately.  The function spins with `wfi` until a timer tick wakes the parked
   thread, then ERets directly to the resumed thread at EL0.

3. **`g_axion_el1_device_wait_pc`** — a global address installed by the caller
   before a scheduler session that wants IRQ-driven waking.  Zero means the
   Phase 11 EL1-direct-wake path (backward compatible).

The Phase 13 CI gate confirms the full hardware-driven roundtrip with a single
`run_proc_entry()` call.

---

## Motivation

RFC-00C0 (Phase 11) introduced `WaitForDevice` waking via EL1 calling
`fs_sched_wake_device()` directly — a two-pass approach.  RFC-00C0 explicitly
deferred the hardware-driven waker:

> Hardware interrupt-driven `WaitForDevice` wake-up (timer IRQ waker deferred
> to RFC-00C1).

RFC-00C1 deferred it again; this RFC delivers it.

Without hardware-driven waking, `WaitForDevice` is semantically identical to a
synchronisation point with known latency — the EL0 thread cannot actually block
on an external event.  With the timer ISR wiring, a `BlockedDeviceWait` thread
is truly suspended until hardware fires, enabling real device driver patterns.

---

## Scope

This RFC governs:

1. **`fs_sched_timer_device_wake()`** — async-signal-safe; called from timer ISR;
   wakes all `BlockedDeviceWait` threads unconditionally (single-device Phase 13).
2. **`fs_sched_device_wait_loop()`** — EL1 `wfi` idle loop; entered via ERET from
   WaitForDevice SVC handler; ERets to the first Runnable thread.
3. **`g_axion_el1_device_wait_pc`** — opt-in global; zero preserves Phase 11
   backward-compatible behavior.
4. **WaitForDevice handler patch** — when no Runnable threads remain, redirects
   ERET to `g_axion_el1_device_wait_pc` if set; else `g_axion_el1_return_pc`.
5. **Phase 13 CI gates** — identity check (LBA 10, tid=5) and IRQ wake check.

This RFC does **not** govern:

- Multi-device IRQ arbitration — which device woke which thread (RFC-00C3).
- Per-device wake filtering — all `BlockedDeviceWait` threads are woken on any
  timer tick regardless of device identity.
- GICv3 SPI / device-specific interrupt lines (timer PPI30 only in Phase 13).

---

## Architecture

### Control Flow

```
EL1: canon_irq_wake_load_and_run()
  ├─ load LBA 10 (T81X v2, tid=5), validate hash
  ├─ emit "[axion] el0: irq identity OK (hash=verified, tid=5)"
  ├─ g_axion_el1_device_wait_pc = &fs_sched_device_wait_loop
  ├─ fs_sched_reset(), register tid=5, mark Running
  └─ run_proc_entry(code_pa, stack_top)
       │
       ▼ ERET → EL0 Process D (tid=5)
       D: sub sp, #64
       D: build WaitForDevice request
       D: svc #1  ──────────────────────────────────────────► EL1 SVC handler
                                                        ├─ save D context
                                                        ├─ D → BlockedDeviceWait
                                                        ├─ no Runnable threads
                                                        │  g_axion_el1_device_wait_pc ≠ 0
                                                        └─ ERET → fs_sched_device_wait_loop
                                                                     │
                                                             EL1 idle (wfi)
                                                                     │
                                                  ┌──────── timer IRQ fires (~10ms)
                                                  │  axion_irq_handler_aarch64:
                                                  │    bridge_timer_irq_tick()
                                                  │    fs_sched_timer_device_wake()
                                                  │      D: BlockedDeviceWait → Runnable
                                                  └─ ERET → fs_sched_device_wait_loop
                                                                     │
                                                             wfi returns; check Runnable
                                                             D is Runnable → ERET → EL0
       │
       ▼ D resumes at instruction after svc #1
       D: svc #2 (ExitThread) ────────────────────────────► EL1: fs_sched_exit_thread
                                                        └─ no Runnable → ERET → return_pc
       │
       ▼ run_proc_entry() returns to canon_irq_wake_load_and_run()
  ├─ g_axion_el1_device_wait_pc = 0  (restore to direct-wake mode)
  ├─ fs_obs_find(5u, 43u, 0u) → true
  └─ emit "[axion] el0: irq wake OK (WaitForDevice tid=5, timer-driven)"
```

### SPSR / DAIF at Each Boundary

| Point | SPSR / PSTATE | IRQs |
|---|---|---|
| WaitForDevice ERET → device_wait_loop | SPSR = 0x5 (EL1h, DAIF=0) | **enabled** |
| `wfi` in device_wait_loop | EL1h | enabled — timer can fire |
| Timer IRQ taken | PSTATE saved; DAIF.I set by hardware | masked during ISR |
| ERET from ISR → device_wait_loop | restored PSTATE | enabled again |
| device_wait_loop ERET → D at EL0 | SPSR = 0x3C0 (EL0t, DAIF masked) | — |

---

## New API

```cpp
// qemu_slice6_el0_svc_bridge.cpp ─────────────────────────────────────────────

// Global: 0 = Phase 11 EL1-direct-wake path (backward compatible).
//         non-zero = address of fs_sched_device_wait_loop(); set by caller.
extern "C" uint64_t g_axion_el1_device_wait_pc;

// Wake all BlockedDeviceWait threads → Runnable.
// Async-signal-safe; called from timer ISR every ~10ms.
extern "C" void fs_sched_timer_device_wake() noexcept;

// EL1 wfi idle loop.  Entered via ERET when g_axion_el1_device_wait_pc is set.
// IRQs are enabled on entry (SPSR = 0x5 from WaitForDevice handler).
// Spins with wfi until a Runnable thread appears, then ERets to it at EL0.
extern "C" void fs_sched_device_wait_loop() noexcept;
```

### Caller Protocol (canon_irq_wake_load_and_run)

```cpp
// Install IRQ-driven wake loop before scheduler session.
g_axion_el1_device_wait_pc = reinterpret_cast<uint64_t>(fs_sched_device_wait_loop);

// Single run_proc_entry() call — returns only after all threads exit.
run_proc_entry(code_pa, stack_top);

// Restore to direct-wake mode for subsequent sessions.
g_axion_el1_device_wait_pc = 0u;
```

---

## Backward Compatibility

- `g_axion_el1_device_wait_pc` is initialized to `0u` — Phase 11
  (`canon_identity_load_and_run`) runs unchanged: `WaitForDevice` handler sees
  `g == 0` and redirects to `g_axion_el1_return_pc` as before.
- `fs_sched_timer_device_wake()` is called on every timer tick.  Before Phase 13
  runs, `s_sched[]` is all-Unused so the function is a no-op.

---

## Acceptance Criteria

- [x] `fs_sched_timer_device_wake()` implemented; wakes all `BlockedDeviceWait`
  entries unconditionally
- [x] `axion_irq_handler_aarch64()` calls `fs_sched_timer_device_wake()` after
  `bridge_timer_irq_tick()` on every timer tick
- [x] `fs_sched_device_wait_loop()` implemented; `wfi` loop; ERets to first
  Runnable thread via inline assembly (ELR_EL1 / SP_EL0 / SPSR_EL1)
- [x] `g_axion_el1_device_wait_pc` global; WaitForDevice handler uses it when
  non-zero; zero preserves Phase 11 behavior
- [x] `canon_irq_wake_load_and_run()`: loads LBA 10 as T81X v2 (tid=5), installs
  loop, single `run_proc_entry()`, verifies obs ring
- [x] LBA 10 embedded in CI as T81X v2 (same binary as LBA 8)
- [x] Phase 13a CI gate: `[axion] el0: irq identity OK (hash=verified, tid=5)`
- [x] Phase 13b CI gate: `[axion] el0: irq wake OK (WaitForDevice tid=5, timer-driven)`
- [ ] RFC-00C3: per-device wake filtering; `fs_sched_timer_device_wake` wakes
  only threads whose `device_id` matches the fired interrupt

---

## Relationship to Other RFCs

- **RFC-00BD** — `WaitForDevice` ordinal 43 is used in the obs ring check.
- **RFC-00BE** — defines `BlockedDeviceWait` state; this RFC adds the transition
  path back to `Runnable` via hardware interrupt.
- **RFC-00BF** — `fs_obs_find(5u, 43u, 0u)` verifies the WaitForDevice record
  in Phase 13b.
- **RFC-00C0** — Phase 11 EL1-direct-wake path; this RFC is backward-compatible
  with it via `g_axion_el1_device_wait_pc == 0`.
- **RFC-00C1** — manifest verification; Phase 13 does not add a manifest (the
  mechanism is proven in Phase 12).
- **RFC-00C3** — will add per-device wake filtering and multi-SPI support.

---

## References

- [RFC-00BD: KernelCall ABI Ordinal Freeze](RFC-00BD-kernelcall-abi-ordinal-freeze.md)
- [RFC-00BE: Freestanding Cooperative Scheduler](RFC-00BE-freestanding-cooperative-scheduler.md)
- [RFC-00BF: Freestanding KernelCall Observability](RFC-00BF-freestanding-kernelcall-observability.md)
- [RFC-00C0: CanonFS Executable Identity](RFC-00C0-canonfs-executable-identity.md)
- [RFC-00C1: CanonFS Per-Binary Call Sequence Manifest](RFC-00C1-canonfs-per-binary-call-sequence-manifest.md)
- `ternaryos/hal/qemu_slice6_el0_svc_bridge.cpp` — timer wake + idle loop
- `ternaryos/hal/qemu_slice6_bridge_irq.cpp` — timer ISR wiring
- `ternaryos/hal/canon_exec_loader.cpp` — Phase 13 loader
