# RFC-00C6 — Per-Thread TTBR0 Address-Space Isolation

| Field      | Value                                           |
|------------|-------------------------------------------------|
| RFC        | 00C6                                            |
| Title      | Per-Thread TTBR0 Address-Space Isolation        |
| Status     | **Accepted**                                    |
| Depends-on | RFC-00C5 (Concurrent Device Wait)               |
| Supersedes | —                                               |

---

## 1. Problem Statement

RFC-00C5 validated that two threads can park on `WaitForDevice` concurrently
and be woken by the same timer IRQ.  However, both threads share a single TTBR0
mapping: tid=6's code/stack pages and tid=7's code/stack pages are all mapped
with EL0 access simultaneously.  A misbehaving thread could read or write the
other's stack, violating process isolation.

RFC-00C6 introduces per-thread L3 page tables so that each thread's TTBR0 view
exposes **only its own** proc code and stack pages at EL0.  The other thread's
proc pages are remapped to EL1-only access in that thread's view.

---

## 2. Architecture

### 2.1 Scope of isolation

The EFI binary, EDK2 identity map, and init code/stack pages remain accessible
to both threads — these are shared EL1 infrastructure and must be reachable for
SVC exception delivery.  Isolation is applied only to the four *proc* pages
(`proc_code_page`, `proc_stack_page`, `proc_code_page2`, `proc_stack_page2`).

### 2.2 Mechanism — L2 entry swap + TLBI

All six EL0 pages reside in a single 2 MB block.  That block's L2 entry
(`s_l2[s_l2_el0_block_idx]`) normally points to the shared L3 table
(`s_shared_l3`, captured during `el0_mmu_init()`).

RFC-00C6 builds a *private* L3 table per thread slot.  Before each ERET to
a thread, the scheduler:
1. Writes `s_l2[s_l2_el0_block_idx]` to point to the thread's private L3.
2. Issues `DSB SY` + `TLBI VMALLE1` + `DSB SY` + `ISB`.

After all threads exit (via `fs_sched_exit_thread` / `el0_mmu_install_shared_l3`),
the L2 entry is restored to `s_shared_l3`, and another TLBI is issued.

No `TTBR0_EL1` write is required; only the L2 → L3 pointer changes.

### 2.3 Private L3 content

`el0_mmu_build_thread_l3(slot, own_code_pa, own_stack_pa)`:

1. Clone `s_shared_l3` → `s_thread_l3[slot]` (preserves all EL1 mappings).
2. For every proc page PA in `s_all_proc_page_pas[0..3]`:
   set that L3 entry to `kLeafProcEl1Only`
   (`AP[2:1]=0b00` = EL1 R/W, EL0 no access; `UXN=1`).
3. Restore the entry for `own_code_pa` → `kLeafProcCode` (EL0 R/W/X).
4. Restore the entry for `own_stack_pa` → `kLeafStack` (EL0 R/W, NX).

Result: EL0 executing as thread E can access page1 + stack1; accessing page2
or stack2 triggers a Data/Instruction Abort (EC=0x25/0x20) at EL1.

---

## 3. New MMU API

```cpp
// Build the per-thread isolated L3 for slot.
// own_code_pa  = base PA of the thread's 4 KB code page.
// own_stack_pa = base PA of the thread's 4 KB stack page (= stack_top - 4096).
extern "C" void el0_mmu_build_thread_l3(uint32_t slot,
                                          uint64_t own_code_pa,
                                          uint64_t own_stack_pa) noexcept;

// Swap s_l2[block_idx] → thread's L3 + TLBI.  Called before ERET to thread.
extern "C" void el0_mmu_install_thread_l3(uint32_t slot) noexcept;

// Restore s_l2[block_idx] → shared L3 + TLBI.  Called when returning to EL1.
extern "C" void el0_mmu_install_shared_l3() noexcept;
```

---

## 4. Scheduler Changes

### 4.1 `FsSchedThread` — new field

```cpp
uint32_t l3_slot;  // RFC-00C6: per-thread L3 slot; kNoThreadL3 (~0u) = shared
```

`fs_sched_reset()` initializes all `l3_slot` fields to `kNoThreadL3`, preserving
backward compatibility with all prior phases.

### 4.2 New API

```cpp
// Assign an L3 slot to a registered thread (call after fs_sched_register).
extern "C" void fs_sched_set_thread_l3(uint32_t tid, uint32_t l3_slot) noexcept;
```

### 4.3 Context switch points

| Function                          | Change                                                        |
|-----------------------------------|---------------------------------------------------------------|
| `WaitForDevice` SVC handler       | Install next thread's L3 before setting trap frame for ERET   |
| `fs_sched_device_wait_loop()`     | Install next thread's L3 before inline-asm ERET               |
| `fs_sched_exit_thread()` — next   | Install next thread's L3 before setting trap frame for ERET   |
| `fs_sched_exit_thread()` — last   | `el0_mmu_install_shared_l3()` before returning to EL1        |

When `l3_slot == kNoThreadL3`, the install call is skipped (shared L3 remains active).

---

## 5. Execution Flow (Phase 17)

```
canon_per_thread_pt_load_and_run()
  ├─ load_t81x_v2_into(LBA 10, page2)  → code_pa_f (tid=7)
  ├─ load_t81x_v2_into(LBA 11, page1)  → code_pa_e (tid=6)
  ├─ el0_mmu_build_thread_l3(slot=0, page1_pa, stack1_base)
  │     L3[page1_idx] = kLeafProcCode (EL0 R/W/X)
  │     L3[stack1_idx] = kLeafStack  (EL0 R/W, NX)
  │     L3[page2_idx] = kLeafProcEl1Only  (EL0 no access)
  │     L3[stack2_idx] = kLeafProcEl1Only (EL0 no access)
  ├─ el0_mmu_build_thread_l3(slot=1, page2_pa, stack2_base)
  │     (symmetric, page1/stack1 are EL1-only)
  ├─ fs_sched_register(6, ...) + fs_sched_set_thread_l3(6, 0)
  ├─ fs_sched_register(7, ...) + fs_sched_set_thread_l3(7, 1)
  └─ run_proc_entry(code_pa_e, stack_top_e)
        SVC WaitForDevice (tid=6)
          → next=tid=7 Runnable
          → el0_mmu_install_thread_l3(slot=1)  [s_l2→L3#1, TLBI]
          → ERET tid=7
        SVC WaitForDevice (tid=7)
          → no Runnable → device_wait_loop
        wfi → timer INTID=30 → both Runnable
        device_wait_loop → tid=6 next
          → el0_mmu_install_thread_l3(slot=0)  [s_l2→L3#0, TLBI]
          → ERET tid=6
        ExitThread (tid=6) → fs_sched_exit_thread
          → tid=7 next
          → el0_mmu_install_thread_l3(slot=1)  [s_l2→L3#1, TLBI]
          → ERET tid=7
        ExitThread (tid=7) → fs_sched_exit_thread
          → no next → el0_mmu_install_shared_l3()
          → ERET g_axion_el1_return_pc
  ├─ fs_gov_find_device(6, kGovTimerDeviceWake, 30) → true
  ├─ fs_gov_find_device(7, kGovTimerDeviceWake, 30) → true
  └─ print CI gate string
```

---

## 6. CI Gate

```
[axion] el0: per-thread pt OK (tid=6+7, isolated)
```

This confirms:
1. Both private L3 tables were built correctly (own pages EL0-accessible,
   other thread's pages EL1-only).
2. The scheduler installed the correct L3 before each ERET without a
   permission fault.
3. Both threads ran to completion — meaning they could access their own
   code and stack pages without an access abort.
4. The shared L3 was restored after the last thread exited.

---

## 7. Memory Budget

| Addition                              | Size   |
|---------------------------------------|--------|
| `s_thread_l3[2][512]` (2 L3 tables)  | 8 KB   |

All new BSS. No additional L3 pool slots consumed (same 2 MB block as existing pages).

---

## 8. Implementation Files

| File                                              | Change                                                              |
|---------------------------------------------------|---------------------------------------------------------------------|
| `ternaryos/hal/qemu_slice6_el0_mmu.cpp`      | `s_thread_l3[2][512]`, `s_shared_l3`, `s_l2_el0_block_idx`, `s_all_proc_page_pas`; `el0_mmu_build_thread_l3()`, `el0_mmu_install_thread_l3()`, `el0_mmu_install_shared_l3()` |
| `ternaryos/hal/qemu_slice6_el0_svc_bridge.cpp` | `kNoThreadL3`, `FsSchedThread.l3_slot`, `fs_sched_set_thread_l3()`; L3 install at all three ERET sites; shared-L3 restore on last exit |
| `ternaryos/hal/canon_exec_loader.cpp`          | `canon_per_thread_pt_load_and_run()` + new externs               |
| `ternaryos/hal/qemu_slice6_cpp_bridge.cpp`     | Declaration + Phase 17 call                                      |
| `.github/workflows/qemu-boot.yml`                          | Phase 17 CI gate (reuses LBA 10+11)                              |
