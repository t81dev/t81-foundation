# RFC-00C8 — Concurrent Fault Isolation

| Field      | Value                                           |
|------------|-------------------------------------------------|
| RFC        | 00C8                                            |
| Title      | Concurrent Fault Isolation                      |
| Status     | **Accepted**                                    |
| Depends-on | RFC-00C7 (EL0 Fault Containment)                |
| Supersedes | —                                               |

---

## 1. Problem Statement

RFC-00C6 proved that two EL0 threads can run with private L3 page tables, and
RFC-00C7 proved that a single faulting thread is contained and returned to EL1
without hanging the system.

However, RFC-00C7 did not exercise the most important concurrent recovery path
inside `fs_sched_fault_handler()`:

1. mark the current thread `Faulted`
2. find another thread already `Runnable`
3. install that thread's private L3
4. ERET directly into the healthy sibling instead of returning to EL1

In Phase 18 there is only one active thread (`tid=8`), so the handler always
takes the "no Runnable thread remains" branch.

RFC-00C8 closes that gap by proving that a fault in one EL0 thread does not
kill a healthy sibling.  The scheduler must contain the fault to the offending
thread and continue execution in the remaining Runnable thread.

---

## 2. Scenario

RFC-00C8 reuses the existing Phase 17/18 binaries and page-table machinery:

- **Thread F** (`tid=7`, LBA 12, `el0_fault_test.bin`)
  faults immediately by executing `ldr x0, [xzr]`, generating a Data Abort
  with ESR_EL1 EC=`0x24`.
- **Thread E** (`tid=6`, LBA 11, `el0_device_filter_test.bin`)
  is already registered as `Runnable` and later performs
  `WaitForDevice(device_id=30)` followed by `ExitThread`.

Execution order:

1. Register `tid=6` as `Runnable`.
2. Register `tid=7` and mark it `Running`.
3. Start `tid=7` under its private L3.
4. `tid=7` faults immediately.
5. `fs_sched_fault_handler()` marks `tid=7` `Faulted` and finds `tid=6`
   already `Runnable`.
6. The handler installs `tid=6`'s private L3 and ERets directly to `tid=6`.
7. `tid=6` parks on `WaitForDevice(30)`, is woken by timer IRQ INTID 30,
   resumes, and exits.
8. EL1 regains control only after the healthy sibling has completed.

This validates the branch in `fs_sched_fault_handler()` that RFC-00C7 added but
did not execute end-to-end.

---

## 3. Scheduler Contract

### 3.1 Required fault-handler behavior

For a fault taken from EL0 while another thread is already `Runnable`,
`fs_sched_fault_handler(void* frame_ptr)` must:

```cpp
FsSchedThread* cur = fs_find_running();
if (cur) {
    cur->state     = FsSchedState::Faulted;
    cur->fault_ec  = ec;
    cur->fault_far = far;
    fs_gov_record(cur->tid, kGovThreadFault /*3*/, ec);
    s_sched_running_tid = 0u;
}

FsSchedThread* next = fs_find_next_runnable();
if (next) {
    next->state         = FsSchedState::Running;
    s_sched_running_tid = next->tid;
    s_current_el0_tid   = next->tid;
    if (next->l3_slot != kNoThreadL3)
        el0_mmu_install_thread_l3(next->l3_slot);
    f->elr_el1  = next->resume_elr;
    f->sp_el0   = next->resume_sp_el0;
    f->spsr_el1 = next->resume_spsr;
} else {
    el0_mmu_install_shared_l3();
    f->elr_el1  = g_axion_el1_return_pc;
    f->spsr_el1 = 0x5u;
}
```

RFC-00C8 specifically validates the `if (next)` path.

### 3.2 Isolation invariant

After the fault:

- `tid=7` must remain terminal (`Faulted`) and must never run again.
- `tid=6` must continue with its own private L3 view.
- Shared L3 restoration must still occur on the final return to EL1.

---

## 4. Governance Requirements

RFC-00C8 does not introduce any new governance record types.  It relies on the
existing ring entries added by RFC-00C3, RFC-00C4, and RFC-00C7.

The run is considered valid only if the governance ring contains:

1. `kGovThreadFault` for `tid=7`, `ec=0x24`
2. `kGovTimerDeviceWake` for `tid=6`, `device_id=30`

This is sufficient evidence that:

- the fault path executed for the faulting sibling, and
- the healthy sibling progressed far enough to block on the timer-backed device
  wait and be resumed normally.

---

## 5. CanonFS Layout (updated)

| LBA | Content                                        | Details                                           |
|-----|------------------------------------------------|---------------------------------------------------|
|  8  | `el0_wait_test.bin` (T81X v2, tid=4)           | Phase 11 — identity + direct wake                 |
|  9  | T81M manifest                                  | Phase 12 — call-sequence manifest                 |
| 10  | `el0_wait_test.bin` (T81X v2, tid=5/7)         | Phase 13/16/17 — IRQ wake + concurrent wait       |
| 11  | `el0_device_filter_test.bin` (T81X v2, tid=6) | Phase 15/16/17/19 — filter + concurrent + pt + sibling survivor |
| 12  | `el0_fault_test.bin` (T81X v2, tid=7/8)       | Phase 18/19 — single fault + concurrent fault     |

No new EL0 test binary is required.

---

## 6. Execution Flow

```
canon_concurrent_fault_load_and_run()
  ├─ load_t81x_v2_into(LBA 12, code_page2) → code_pa_f (tid=7)
  ├─ load_t81x_v2_into(LBA 11, code_page1) → code_pa_e (tid=6)
  ├─ el0_mmu_build_thread_l3(slot=0, page1_pa, stack1_base)
  ├─ el0_mmu_build_thread_l3(slot=1, page2_pa, stack2_base)
  ├─ g_axion_el1_device_wait_pc = &fs_sched_device_wait_loop
  ├─ fs_sched_register(6, code_pa_e, stack_top_e, 0x3C0)
  ├─ fs_sched_set_thread_l3(6, 0)
  ├─ fs_sched_register(7, code_pa_f, stack_top_f, 0x3C0)
  ├─ fs_sched_set_thread_l3(7, 1)
  ├─ fs_sched_mark_running(7)
  ├─ el0_mmu_install_thread_l3(1)
  └─ run_proc_entry(code_pa_f, stack_top_f)
        EL0: tid=7 executes ldr x0, [xzr]
          └─ Data Abort: EC=0x24, FAR_EL1=0x0
             fs_sched_fault_handler()
               tid=7 → Faulted
               fs_gov_record(7, kGovThreadFault=3, 0x24)
               next Runnable = tid=6
               el0_mmu_install_thread_l3(slot=0)
               ERET → tid=6
        EL0: tid=6 executes WaitForDevice(device_id=30)
          └─ no Runnable remains → device_wait_loop
               timer IRQ INTID=30
               fs_sched_timer_device_wake(30) → tid=6 Runnable
               fs_gov_record(6, kGovTimerDeviceWake=1, 30)
               ERET → tid=6
        EL0: tid=6 executes ExitThread
          └─ no Runnable remains → shared L3 restore → EL1
  ├─ fs_gov_find_fault(7, 0x24) → true
  ├─ fs_gov_find_device(6, kGovTimerDeviceWake, 30) → true
  └─ print CI gate string
```

---

## 7. CI Gate

Phase 19 is verified by:

```
[axion] el0: concurrent fault OK (tid=7 faulted, tid=6 exited)
```

This confirms:

1. the faulting thread reached the EL1 fault handler,
2. the handler took the "next Runnable thread exists" branch,
3. the healthy sibling resumed under its private L3,
4. the healthy sibling completed the timer-driven `WaitForDevice` path, and
5. the scheduler returned to EL1 only after the survivor exited cleanly.

---

## 8. Backward Compatibility

RFC-00C8 is a pure validation and orchestration extension:

- no new syscall or KernelCall ordinal is introduced,
- no new scheduler state is introduced,
- no new MMU primitive is introduced,
- no new governance record layout is introduced.

All implementation work is confined to a new loader phase plus CI wiring.

---

## 9. Implementation Files

| File                                           | Change                                                           |
|------------------------------------------------|------------------------------------------------------------------|
| `ternaryos/hal/canon_exec_loader.cpp`          | `canon_concurrent_fault_load_and_run()`; direct L3 install/restore externs |
| `ternaryos/hal/qemu_slice6_cpp_bridge.cpp`     | Phase 19 declaration + call                                      |
| `.github/workflows/qemu-boot.yml`              | Phase 19 CI gate check                                           |
| `spec/rfcs/RFC-00C8-concurrent-fault-isolation.md` | This RFC                                                       |
