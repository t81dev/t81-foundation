# RFC-00BE: Freestanding Cooperative Scheduler

**Status:** accepted
**Type:** standards-track
**Applies-To:** TernaryOS freestanding EL0 bridge, KernelCall SVC ABI (EL0 subset)
**Created:** 2026-03-23
**Author:** @t81dev
**Depends on:** RFC-00BD (KernelCall ABI Ordinal Freeze), RFC-00BC (TernaryOS EL0 Userland Bring-Up)
**Blocks:** RFC-00BF (Freestanding KernelCall Observability), RFC-00C0 (CanonFS Executable Identity)

---

## Summary

This RFC defines a deterministic freestanding cooperative scheduler for the
TernaryOS EL0 bridge.  It introduces a fixed-capacity thread table, a
six-state thread lifecycle, two blocking KernelCall yield points
(`BlockOnIpcReceive` kind=42 and `WaitForDevice` kind=43), and a
trap-frame–based context switch mechanism that operates entirely within the
existing SVC exception path — no new exception vectors, no timer preemption.

The Phase 9 CI gate (`[axion] el0: sched roundtrip OK (B blocked, A->B, tid=3←2)`)
proves the round-trip: a blocking thread parks itself, a sending thread delivers
a message and exits, and the scheduler resumes the parked thread from its exact
register state.

---

## Motivation

RFC-00BC Phase 8 established the IPC mailbox, but the two EL0 processes ran
sequentially: A sent, exited, then B received.  No thread ever had to wait for
another thread to act.

For any real scheduling contract — capability-based wake-up, device I/O
completion, multi-agent synchronisation — a thread must be able to:

1. Declare that it is waiting for an event (park itself, yield the CPU).
2. Be woken by a peer that produces the event.
3. Resume from the exact point where it parked, seeing the event payload in its
   response buffer.

RFC-00BD froze `BlockOnIpcReceive` (kind=42) and `WaitForDevice` (kind=43) as
the canonical yield-point ordinals before this scheduler existed.  This RFC
gives those ordinals their semantics.

---

## Scope

This RFC governs:

1. **Thread lifecycle states** — the six-state `FsSchedState` machine and legal
   transitions.
2. **Thread table** — fixed-capacity table, registration API, and ownership rules.
3. **`BlockOnIpcReceive` (kind=42)** — save context, mark blocked, switch to next
   runnable thread.
4. **`WaitForDevice` (kind=43)** — same park mechanism; no waker is defined in this
   RFC (waker is RFC-00C0 territory).
5. **`SendMessage` (kind=13) enhancement** — wakes a `BlockedIpcReceive` thread
   matching the `ipc_dst` field by delivering directly to its saved response buffer.
6. **`ExitThread` (SVC #2) enhancement** — switches to the next runnable thread
   instead of returning to EL1 when the scheduler is active.
7. **Trap-frame context switch** — mechanism for redirecting ERET by modifying
   `elr_el1` / `sp_el0` / `spsr_el1` in the saved trap frame before returning from
   the C exception handler.
8. **Phase 9 EL0 binaries** — `el0_sched_test_b.S` (Process B: parks first) and
   `el0_sched_test_a.S` (Process A: sends and exits).

This RFC does **not** govern:

- Hardware timer preemption of EL0 threads (the GICv3 timer tick drives only the
  EL1 event-loop counters; it does not interrupt EL0 processes in this RFC).
- `WaitForDevice` wake-up (deferred to RFC-00C0).
- The observability ring (RFC-00BF).
- CanonFS executable identity or content-hash validation (RFC-00C0/C1).

---

## Thread Lifecycle

### States

| State | Value | Meaning |
| :--- | ---: | :--- |
| `Unused` | 0 | Slot is free; no thread assigned |
| `Runnable` | 1 | Registered and eligible; not yet running |
| `Running` | 2 | Currently executing at EL0 (at most one) |
| `BlockedIpcReceive` | 3 | Parked via `BlockOnIpcReceive` (kind=42) |
| `BlockedDeviceWait` | 4 | Parked via `WaitForDevice` (kind=43) |
| `Exited` | 5 | Called `ExitThread` (SVC #2); slot is done |

### Legal Transitions

```
Unused → Runnable             registration (fs_sched_register)
Runnable → Running            scheduler picks this thread (initial ERET or context switch)
Running → BlockedIpcReceive   thread calls BlockOnIpcReceive (kind=42)
Running → BlockedDeviceWait   thread calls WaitForDevice (kind=43)
Running → Exited              thread calls ExitThread (SVC #2)
BlockedIpcReceive → Runnable  SendMessage delivers to this tid
BlockedDeviceWait → Runnable  device interrupt fires (RFC-00C0; not this RFC)
Runnable → Running            scheduler picks this thread (context switch)
```

Transitions not listed are **forbidden**.  The scheduler does not re-use Exited
slots within a single scheduler session; `canon_sched_reset()` clears all slots
between sessions.

---

## Thread Table

```cpp
static constexpr int kMaxSchedThreads = 8;

struct FsSchedThread {
    uint32_t     tid;            // 0 = slot unused
    FsSchedState state;          // six-state enum
    uint64_t     resume_elr;     // ELR_EL1 to ERET to (entry PA or resume PC)
    uint64_t     resume_sp_el0;  // SP_EL0 at registration or at block time
    uint64_t     resume_spsr;    // SPSR_EL1 (0x3C0 for EL0t + DAIF masked)
    uint64_t     ipc_rsp_tva;    // EL0 VA of response buffer (saved at block time)
    uint64_t     ipc_rsp_size;   // response buffer size in bytes
};
```

The table is a flat array of 8 slots.  No dynamic allocation.  The scheduler
scans linearly; with 8 or fewer threads this is O(1) in practice.

### Registration

`fs_sched_register(tid, entry_elr, sp_el0, spsr)` finds the first `Unused` slot
and sets it to `Runnable` with the supplied initial context.  The `ipc_rsp_tva`
and `ipc_rsp_size` fields are zeroed at registration; they are populated only
when the thread calls `BlockOnIpcReceive`.

### Code Page Layout (Phase 9)

Both Phase 9 stubs share the single 4 KB EL0 proc code page.  They are placed
at non-overlapping offsets:

| Process | tid | Page offset | Stack top |
| :--- | ---: | ---: | :--- |
| Process B (blocker) | 3 | 0 | `stack_top` |
| Process A (sender) | 2 | 256 | `stack_top − 256` |

Both stubs are position-independent (PC-relative addressing only).  At 256-byte
offset the `.text` sections do not overlap (each stub is ≤ 128 bytes).

Stack layout (single 4 KB stack page, grows down from `stack_top`):

```
stack_top          ← B initial SP_EL0
stack_top − 64     ← B frame top (after `sub sp, #64`)
…                  ← B response buffer  [stack_top−48 … stack_top−1]
stack_top − 256    ← A initial SP_EL0
stack_top − 320    ← A frame top (after `sub sp, #64`)
…                  ← A response buffer  [stack_top−304 … stack_top−257]
```

No overlap between A's and B's frames. ✓

---

## Context Switch Mechanism

The `axion_svc_entry` stub in `aarch64_exception_vectors.S` saves the full
280-byte trap frame to the EL1 stack before calling
`axion_kernel_handle_svc_trap_aarch64(frame)`.  The C handler receives a
pointer to this frame.  The restore sequence reads back `elr_el1`, `sp_el0`,
and `spsr_el1` from the frame before `eret`.

**Cooperative context switch:** the C handler modifies the frame's
`elr_el1 / sp_el0 / spsr_el1` fields to point at the next thread's saved
context.  The restore + ERET naturally delivers execution to that thread.
No separate context-switch assembly stub is needed.

This is safe because:
- The EL1 SVC stack is not shared across SVCs (each SVC gets its own frame).
- The modified fields redirect the ERET; the caller-saved GPRs in the frame are
  irrelevant to the next thread (it has its own stack and register state).

---

## KernelCall Behaviour

### BlockOnIpcReceive (kind=42)

1. Validate `req_tva` / `rsp_tva` against EL0-mapped VA range (existing Phase 5
   check; already enforced at dispatch entry).
2. Write common Ok response header to `rsp_tva` (magic, version, bytes,
   `status=Ok`, `rejection=0`).  The response body fields (`spawned_tid` etc.)
   are zeroed; the waker fills `spawned_tid` (offset 36) when it delivers.
3. Save current thread's context:
   - `resume_elr    ← frame.elr_el1`  (hardware advanced past `svc` instruction)
   - `resume_sp_el0 ← frame.sp_el0`
   - `resume_spsr   ← frame.spsr_el1`
   - `ipc_rsp_tva   ← frame.x[2]`     (rsp TVA, already validated)
   - `ipc_rsp_size  ← frame.x[3]`
4. Transition current thread: `Running → BlockedIpcReceive`.
5. Find next `Runnable` thread (linear scan, first match).
6. If found: transition it to `Running`, update `s_sched_running_tid` and
   `s_current_el0_tid`, redirect frame to its context.
7. If none found: redirect frame to `g_axion_el1_return_pc` / SPSR=EL1h, clear
   `s_sched_running_tid`.

### WaitForDevice (kind=43)

Identical to `BlockOnIpcReceive` steps 1–7, except the state is `BlockedDeviceWait`
and there is no IPC response payload.  The waker (RFC-00C0) will transition
`BlockedDeviceWait → Runnable` on device interrupt.  In Phase 9 no waker exists;
a `WaitForDevice` call from EL0 parks the thread indefinitely for the duration
of the scheduler session.

### SendMessage (kind=13) — enhanced

Before updating the legacy Phase 8 mailbox:

1. Read `ipc_dst` (uint32) from `req[12:16]`.
2. Search the scheduler table for a thread with `tid == ipc_dst` and
   `state == BlockedIpcReceive`.
3. If found:
   - Write `sender_tid` (= `s_current_el0_tid`) to `target->ipc_rsp_tva + 36`
     (the `spawned_tid` field of the target's saved response buffer).
   - Transition target: `BlockedIpcReceive → Runnable`.
   - Set `s_sched_ipc_delivered = true`.
4. Then update the legacy mailbox (`s_ipc_mailbox`) for Phase 8 backward
   compatibility.

The sender does **not** immediately yield to the woken thread.  The woken thread
becomes `Runnable` and will be selected by the next `ExitThread` or
`BlockOnIpcReceive` call from the sender.

### ExitThread (SVC #2) — enhanced

`fs_sched_exit_thread(frame)` replaces the hard-coded EL1 return:

1. Locate the current `Running` thread in the table (by `s_sched_running_tid`).
2. Transition it to `Exited`.
3. Find next `Runnable` thread.
4. If found: transition to `Running`, update `s_sched_running_tid` and
   `s_current_el0_tid`, redirect frame.
5. If none found: redirect to `g_axion_el1_return_pc` / SPSR=EL1h, clear
   `s_sched_running_tid`.  Control returns to EL1 in `canon_sched_load_and_run()`.

`fs_sched_exit_thread` is also invoked when the scheduler is **not** active
(`s_sched_running_tid == 0`), in which case step 1 finds nothing and step 5 runs
immediately — preserving the existing Phase 6–8 ExitThread behavior.

---

## Phase 9 CI Sequence

```
EL1: canon_sched_load_and_run()
  ├─ load el0_sched_test_b.bin → code_page[0..N-1]  (Process B, tid=3)
  ├─ load el0_sched_test_a.bin → code_page[256..256+M-1]  (Process A, tid=2)
  ├─ register B: {Runnable, elr=code_page_pa+0,   sp=stack_top,     spsr=0x3C0}
  ├─ register A: {Runnable, elr=code_page_pa+256, sp=stack_top−256, spsr=0x3C0}
  ├─ mark B Running, s_sched_running_tid=3
  └─ ERET → B

EL0 B (tid=3):
  sub sp, #64
  [build BlockOnIpcReceive request at sp+0]
  svc #1  ── kind=42 ──►  EL1: BlockOnIpcReceive handler
                              saves B context: elr=resume_pc, sp_el0=stack_top−64
                              B → BlockedIpcReceive
                              next Runnable = A (tid=2)
                              redirect frame → A entry (code_page_pa+256, stack_top−256)
                              A → Running
                          ◄──  ERET (to A)

EL0 A (tid=2):
  sub sp, #64
  [build SendMessage request: kind=13, ipc_dst=3]
  svc #1  ── kind=13 ──►  EL1: SendMessage handler
                              finds B (BlockedIpcReceive, tid=3)
                              writes sender_tid=2 → B.ipc_rsp_tva+36
                              B → Runnable
                              s_sched_ipc_delivered = true
                          ◄──  (A continues)
  svc #2  ── ExitThread ──►  EL1: fs_sched_exit_thread
                              A → Exited
                              next Runnable = B (tid=3)
                              redirect frame → B resume_elr, B resume_sp_el0
                              B → Running
                          ◄──  ERET (to B, instruction after svc #1)

EL0 B resumes:
  (response[36:40] = 2 — sender_tid written by A's SendMessage)
  svc #2  ── ExitThread ──►  EL1: fs_sched_exit_thread
                              B → Exited
                              no Runnable threads
                              redirect frame → g_axion_el1_return_pc / EL1h
                          ◄──  ERET (to EL1)

EL1: canon_sched_load_and_run() resumes
  fs_sched_ipc_delivered() == true
  → emit "[axion] el0: sched roundtrip OK (B blocked, A->B, tid=3<-2)"
```

---

## Acceptance Criteria

- [x] `FsSchedThread` table (8 slots) + `FsSchedState` (6 states) in
  `qemu_slice6_el0_svc_bridge.cpp`
- [x] `BlockOnIpcReceive` (kind=42) saves context, switches frame to next Runnable
- [x] `WaitForDevice` (kind=43) parks thread; no waker in Phase 9
- [x] `SendMessage` (kind=13) wakes matching `BlockedIpcReceive` thread; backward
  compatible with Phase 8 mailbox
- [x] `ExitThread` (SVC #2) switches to next Runnable or returns to EL1 via
  `fs_sched_exit_thread`
- [x] `el0_sched_test_a.S` and `el0_sched_test_b.S` implemented; embedded at
  LBA 6 and LBA 7 of `canon_store.img`
- [x] QEMU boot CI gate: `[axion] el0: sched roundtrip OK (B blocked, A->B, tid=3<-2)`
- [ ] RFC-00BF (observability) adds per-call records referencing scheduler state

---

## Relationship to Other RFCs

- **RFC-00BD** — froze `BlockOnIpcReceive` (42) and `WaitForDevice` (43) before
  this scheduler was designed.  This RFC gives those ordinals their semantics.
- **RFC-00BC** — established the Phase 6–8 EL0 path.  Phase 9 extends Phase 8
  from sequential IPC to blocking IPC.
- **RFC-00BF** — the observability ring will record `{tid, kind, state_before,
  state_after}` tuples driven by the transitions defined here.
- **RFC-00C0** — defines the device-interrupt waker that transitions
  `BlockedDeviceWait → Runnable`.

---

## References

- [RFC-00BD: KernelCall ABI Ordinal Freeze](RFC-00BD-kernelcall-abi-ordinal-freeze.md)
- [RFC-00BC: TernaryOS EL0 Userland Bring-Up](RFC-00BC-ternaryos-el0-userland-bringup.md)
- `userland/experimental/hal/qemu_slice6_el0_svc_bridge.cpp` — scheduler impl
- `userland/experimental/hal/qemu_slice6_bridge_irq.cpp` — ExitThread hook
- `userland/experimental/hal/canon_exec_loader.cpp` — Phase 9 loader
- `userland/experimental/hal/el0_sched_test_a.S` — Phase 9 Process A
- `userland/experimental/hal/el0_sched_test_b.S` — Phase 9 Process B
- `userland/experimental/hal/kernelcall_abi.inc` — symbolic constants (RFC-00BD)
