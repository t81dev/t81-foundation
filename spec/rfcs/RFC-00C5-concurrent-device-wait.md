# RFC-00C5 — Concurrent Device Wait

| Field      | Value                                           |
|------------|-------------------------------------------------|
| RFC        | 00C5                                            |
| Title      | Concurrent Device Wait                          |
| Status     | **Accepted**                                    |
| Depends-on | RFC-00C4 (Per-Device Wake Filtering)            |
| Supersedes | —                                               |

---

## 1. Problem Statement

RFC-00C4 validated per-device filtering with a single thread (tid=6,
`device_id=30`).  The filter logic in `fs_sched_timer_device_wake(intid)`
operates over all `BlockedDeviceWait` threads simultaneously, but this was
never exercised with more than one blocked thread at the same time.

RFC-00C5 places two threads in `BlockedDeviceWait` concurrently:
- Thread E (tid=6, `device_id=30`) — exact-match: wakes only on INTID 30
- Thread F (tid=7, `device_id=0`) — wildcard: wakes on any INTID

A single timer IRQ (INTID 30) must wake **both** threads.  This validates
that `fs_sched_timer_device_wake` correctly iterates all blocked threads and
that the scheduler can sequence two thread exits back to EL1 without a
second `run_proc_entry` call from EL1 C++ code.

---

## 2. MMU Extension

The current EL0 address space has four mapped pages (init code, init stack,
proc code, proc stack).  RFC-00C5 adds two more:

| Page              | Array                      | Attributes          | Purpose                          |
|-------------------|----------------------------|---------------------|----------------------------------|
| proc code page 2  | `s_el0_proc_code_page2`    | R/W/X (EL0 + EL1)  | Thread F code (T81X v2 section)  |
| proc stack page 2 | `s_el0_proc_stack_page2`   | R/W, NX             | Thread F EL0 stack               |

Both pages live in BSS alongside the existing four pages, sharing the same
2 MB block and therefore the same owned L3 table.  The `install_el0_page`
call consumes no additional L3 pool slots.

New accessors:

```cpp
extern "C" uint8_t* el0_mmu_proc_code_page2()  noexcept;
extern "C" uint64_t el0_mmu_proc_stack_top2()  noexcept;
```

`el0_tva_valid()` is updated to cover all six pages.

---

## 3. Scheduler Behaviour

No scheduler changes are needed.  The existing machinery handles the
concurrent scenario:

### 3.1 WaitForDevice SVC handler (existing)

When a thread calls WaitForDevice and another thread is already `Runnable`,
the SVC handler ERets directly to the next `Runnable` thread — no wfi
required yet.  The device-wait loop is only entered when *no* Runnable
threads remain.

### 3.2 `fs_sched_exit_thread` (existing)

Called from the ExitThread (SVC #2) handler.  Marks the current thread
`Exited`, then:
- If another `Runnable` thread exists: ERets to it directly.
- If no `Runnable` thread: ERets to `g_axion_el1_return_pc` (back to EL1).

This allows the second thread's exit to return to EL1 without intervention.

---

## 4. Execution Flow

```
canon_concurrent_wait_load_and_run()
  ├─ load_t81x_v2_into(LBA 10, code_page2)   → code_pa_f (tid=7, did=0)
  ├─ load_t81x_v2_into(LBA 11, code_page1)   → code_pa_e (tid=6, did=30)
  ├─ g_axion_el1_device_wait_pc = &fs_sched_device_wait_loop
  ├─ fs_sched_reset()
  ├─ fs_sched_register(6, code_pa_e, stack_top1, 0x3C0)  → Running
  ├─ fs_sched_register(7, code_pa_f, stack_top2, 0x3C0)  → Runnable
  └─ run_proc_entry(code_pa_e, stack_top1)
        EL0: tid=6 calls WaitForDevice(device_id=30)
          └─ SVC handler: tid=6 = BlockedDeviceWait(30)
             find_next_runnable() → tid=7 Runnable
             ERET → tid=7 (no wfi)
        EL0: tid=7 calls WaitForDevice(device_id=0)
          └─ SVC handler: tid=7 = BlockedDeviceWait(0)
             find_next_runnable() → none
             ERET → fs_sched_device_wait_loop
                wfi ──(timer IRQ INTID=30)──►
                  fs_sched_timer_device_wake(30):
                    tid=6: did=30 == intid=30  → Runnable, gov(6,1,30)
                    tid=7: did=0 (wildcard)    → Runnable, gov(7,1,30)
                find_next_runnable() → tid=6
                gov(6, kGovAsyncContextSwitch, 0)
                ERET → tid=6 (resume after svc#1)
        EL0: tid=6 calls ExitThread
          └─ fs_sched_exit_thread: tid=6 = Exited
             find_next_runnable() → tid=7
             ERET → tid=7 (resume after svc#1)
        EL0: tid=7 calls ExitThread
          └─ fs_sched_exit_thread: tid=7 = Exited
             find_next_runnable() → none
             ERET → g_axion_el1_return_pc
  ├─ g_axion_el1_device_wait_pc = 0
  ├─ fs_gov_find_device(6, kGovTimerDeviceWake=1, 30) → true
  ├─ fs_gov_find_device(7, kGovTimerDeviceWake=1, 30) → true
  └─ print CI gate string
```

---

## 5. `canon_concurrent_wait_load_and_run()` — Helper

A private helper `load_t81x_v2_into(lba, dest_page, label)` validates the
T81X v2 header, FNV-1a-64 code_hash, and copies the code section into the
given destination page.  Returns the absolute PA of the entry point, or 0
on any error.  Avoids duplicating the T81X v2 load boilerplate.

---

## 6. CanonFS Layout (updated)

| LBA | Content                                          | Details                                        |
|-----|--------------------------------------------------|------------------------------------------------|
|  8  | `el0_wait_test.bin` (T81X v2, tid=4)             | Phase 11 — identity + direct wake              |
|  9  | T81M manifest                                    | Phase 12 — call-sequence manifest              |
| 10  | `el0_wait_test.bin` (T81X v2, tid=5)             | Phase 13 — IRQ-driven wake (device_id=0)       |
| 11  | `el0_device_filter_test.bin` (T81X v2, tid=6)   | Phase 15 — per-device filter (device_id=30)    |
| 10  | reused for tid=7 in Phase 16                     | Concurrent tid=7 (device_id=0, wildcard)       |
| 11  | reused for tid=6 in Phase 16                     | Concurrent tid=6 (device_id=30, exact)         |

Phase 16 reuses existing LBAs 10 and 11.  No new LBA allocation is needed.

---

## 7. CI Gate

Phase 16 is verified by:

```
[axion] el0: concurrent wake OK (device_id=30, tid=6+7)
```

This confirms:
1. Both T81X v2 binaries loaded with valid headers and matching hashes.
2. tid=6 and tid=7 both reached `BlockedDeviceWait` simultaneously.
3. A single timer IRQ (INTID 30) transitioned both to `Runnable` in one
   `fs_sched_timer_device_wake(30)` call.
4. The scheduler dispatched both threads to ExitThread without additional
   EL1 C++ intervention.
5. Gov ring contains `kGovTimerDeviceWake` records for both tid=6 and tid=7
   with `device_id=30`.

---

## 8. Implementation Files

| File                                              | Change                                                         |
|---------------------------------------------------|----------------------------------------------------------------|
| `ternaryos/hal/qemu_slice6_el0_mmu.cpp`      | Add proc_code_page2 / proc_stack_page2 BSS + PA vars + map + accessors + tva_valid |
| `ternaryos/hal/canon_exec_loader.cpp`         | `load_t81x_v2_into()` helper + `canon_concurrent_wait_load_and_run()` |
| `ternaryos/hal/qemu_slice6_cpp_bridge.cpp`    | Declaration + call for `canon_concurrent_wait_load_and_run()` |
| `.github/workflows/qemu-boot.yml`                         | Phase 16 CI gate (no new LBA — reuses 10+11)                  |
