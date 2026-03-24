# RFC-00C7 — EL0 Fault Containment

| Field      | Value                                           |
|------------|-------------------------------------------------|
| RFC        | 00C7                                            |
| Title      | EL0 Fault Containment                          |
| Status     | **Accepted**                                    |
| Depends-on | RFC-00C6 (Per-Thread TTBR0 Address-Space Isolation) |
| Supersedes | —                                               |

---

## 1. Problem Statement

RFC-00C6 introduced per-thread L3 page tables so that each thread's TTBR0
view exposes only its own proc pages at EL0.  Accessing the other thread's
pages generates a real hardware fault: the AArch64 Lower EL (AArch64)
Synchronous exception fires at offset 0x400 with ESR_EL1 EC=0x24 (Data Abort
from Lower EL).

Prior to RFC-00C7, `axion_kernel_handle_svc_trap_aarch64()` treated every
synchronous exception from Lower EL as an SVC.  A Data Abort with EC=0x24
would extract a garbage SVC immediate from ESR_EL1[15:0] (bits 15:0 of the
fault status, not an SVC number) and fall into the `default:` case — which
halts by redirecting ERET to `g_axion_el1_return_pc` but does not record any
governance information.

More importantly, any future faulting thread that is not the last active thread
would permanently stall the system: the other thread is never scheduled and no
governance record is emitted.

RFC-00C7 adds first-class fault containment:

1. **EC check at the SVC entry point** — distinguish SVC (EC=0x15) from aborts.
2. **`fs_sched_fault_handler()`** — marks the faulting thread `Faulted`,
   records `kGovThreadFault` in the governance ring, context-switches to the
   next Runnable thread (if any), or returns to EL1 cleanly.
3. **Process G** (`el0_fault_test.bin`, tid=8) — a minimal EL0 binary that
   dereferences virtual address `0x0` (never mapped at EL0) to exercise the
   full fault path end-to-end.

---

## 2. EC Discrimination at the SVC Entry Point

`axion_kernel_handle_svc_trap_aarch64()` (in
`qemu_slice6_bridge_irq.cpp`) now checks ESR_EL1 bits [31:26] before
reading the SVC immediate:

```cpp
const uint32_t ec_raw = static_cast<uint32_t>((f->esr_el1 >> 26) & 0x3fu);
if (ec_raw != 0x15u) {   // 0x15 = SVC64
    fs_sched_fault_handler(f);
    return;
}
// existing SVC dispatch...
```

EC values caught by this guard:

| EC   | Exception class                              |
|------|----------------------------------------------|
| 0x20 | Instruction Abort from Lower EL (AArch64)   |
| 0x24 | **Data Abort from Lower EL** (primary case) |
| others | Any future non-SVC synchronous exception  |

---

## 3. Scheduler Extension

### 3.1 `FsSchedState` — new state

```cpp
enum class FsSchedState : uint8_t {
    ...
    Exited            = 5,
    Faulted           = 6,  // RFC-00C7: terminated by hardware fault
};
```

### 3.2 `FsSchedThread` — new fields

```cpp
struct FsSchedThread {
    ...
    uint32_t     fault_ec;   // RFC-00C7: ESR_EL1 EC when faulted; 0 = no fault
    uint64_t     fault_far;  // RFC-00C7: FAR_EL1 when faulted; 0 = no fault
};
```

`fs_sched_reset()` zero-initialises all fields; `fault_ec=0` means "no fault."

### 3.3 `fs_sched_fault_handler(void* frame_ptr)`

```cpp
extern "C" void fs_sched_fault_handler(void* frame_ptr) noexcept {
    auto* f = static_cast<FsBridgeTrapFrame*>(frame_ptr);
    const uint32_t ec = static_cast<uint32_t>((f->esr_el1 >> 26) & 0x3fu);

    uint64_t far = 0u;
    __asm__ volatile("mrs %[r], far_el1" : [r] "=r"(far) :: "memory");

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
        // context-switch to next Runnable thread
        ...
    } else {
        el0_mmu_install_shared_l3();
        f->elr_el1  = g_axion_el1_return_pc;
        f->spsr_el1 = 0x5u;  // EL1h
    }
}
```

`kGovThreadFault = 3` — the `device_id` field in `FsGovRecord` is repurposed
as `ec` for fault records (0 is never a valid EC, so there is no ambiguity with
device-wake records).

---

## 4. Governance Ring Extension

A new event constant and query function:

```cpp
static constexpr uint32_t kGovThreadFault = 3u;  // RFC-00C7

// Returns true when a kGovThreadFault record exists for (tid, ec).
extern "C" bool fs_gov_find_fault(uint32_t tid, uint32_t ec) noexcept;
```

The `FsGovRecord.device_id` field holds `ec` for fault records, matching the
existing 24-byte layout without any structural change.

---

## 5. Process G — `el0_fault_test.S`

Process G (tid=8) is the Phase 18 subject.  Its entire body:

```asm
fault_g_entry:
    ldr  x0, [xzr]    // load from VA 0x0 — unmapped at EL0 → Data Abort
    mov  x0, #0
    svc  #2            // fallback ExitThread if fault containment broken
```

When `ldr x0, [xzr]` executes at EL0:
- The MMU raises a translation fault (DFSC=0b000100, L0 table walk) for VA 0x0.
- The CPU takes the Lower EL (AArch64) Synchronous exception (vector offset 0x400).
- `axion_svc_entry` saves the full `AArch64TrapFrame` on the EL1 stack and calls
  `axion_kernel_handle_svc_trap_aarch64()`.
- EC check: ESR_EL1[31:26] = 0x24 ≠ 0x15 → `fs_sched_fault_handler()`.
- FAR_EL1 = `0x0000000000000000`, EC = 0x24.
- Gov ring: `kGovThreadFault`, tid=8, device_id=0x24.
- No next Runnable thread → shared L3 restored → ERET to `g_axion_el1_return_pc`.

---

## 6. CanonFS Layout (updated)

| LBA | Content                                          | Details                                     |
|-----|--------------------------------------------------|---------------------------------------------|
|  8  | `el0_wait_test.bin` (T81X v2, tid=4)             | Phase 11 — identity + direct wake           |
|  9  | T81M manifest                                    | Phase 12 — call-sequence manifest           |
| 10  | `el0_wait_test.bin` (T81X v2, tid=5/7)           | Phase 13/16/17 — IRQ wake + concurrent      |
| 11  | `el0_device_filter_test.bin` (T81X v2, tid=6)   | Phase 15/16/17 — filter + concurrent + pt  |
| 12  | `el0_fault_test.bin` (T81X v2, tid=8)            | Phase 18 — fault containment               |

---

## 7. Execution Flow

```
canon_fault_contain_load_and_run()
  ├─ load_t81x_v2_into(LBA 12, code_page1) → code_pa (tid=8)
  ├─ fs_sched_reset()
  ├─ fs_sched_register(8, code_pa, stack_top, 0x3C0)
  ├─ fs_sched_mark_running(8)
  └─ run_proc_entry(code_pa, stack_top)
        EL0: G executes ldr x0, [xzr]
          └─ Data Abort: EC=0x24, FAR_EL1=0x0
             axion_kernel_handle_svc_trap_aarch64()
               ec_raw=0x24 ≠ 0x15 → fs_sched_fault_handler()
                 tid=8 → Faulted; fault_ec=0x24; fault_far=0
                 fs_gov_record(8, kGovThreadFault=3, 0x24)
                 no Runnable → el0_mmu_install_shared_l3()
                 f→elr_el1 = g_axion_el1_return_pc
             ERET → EL1 (run_proc_entry label)
  ├─ fs_gov_find_fault(8, 0x24) → true
  └─ print CI gate string
```

---

## 8. CI Gate

Phase 18 is verified by:

```
[axion] el0: fault contained (tid=8, ec=0x24)
```

This confirms:
1. `el0_fault_test.bin` loaded from LBA 12 with valid T81X v2 header and hash.
2. Process G executed `ldr x0, [xzr]` and triggered a Data Abort (EC=0x24).
3. The EC discriminator correctly identified it as a non-SVC synchronous fault.
4. `fs_sched_fault_handler()` marked tid=8 `Faulted` and restored the shared L3.
5. ERET to EL1 completed — the system did not hang.
6. The governance ring contains a `kGovThreadFault` record for tid=8, ec=0x24.

---

## 9. Backward Compatibility

The EC check is transparent to all prior phases:
- All prior EL0 threads use `svc #1` (KernelCall) or `svc #2` (ExitThread).
  Both generate EC=0x15. The existing SVC dispatch path is unchanged.
- No fault-generating code existed before RFC-00C7 (all prior tests exit cleanly
  via ExitThread without accessing unmapped pages).

---

## 10. Implementation Files

| File                                              | Change                                                       |
|---------------------------------------------------|--------------------------------------------------------------|
| `ternaryos/hal/qemu_slice6_bridge_irq.cpp`        | EC check in `axion_kernel_handle_svc_trap_aarch64()`; extern `fs_sched_fault_handler` |
| `ternaryos/hal/qemu_slice6_el0_svc_bridge.cpp`    | `Faulted` state; `fault_ec`/`fault_far` fields; `kGovThreadFault=3`; `fs_sched_fault_handler()`; `fs_gov_find_fault()` |
| `ternaryos/hal/el0_fault_test.S`                  | New — Process G (tid=8), `ldr x0, [xzr]`                    |
| `ternaryos/hal/canon_exec_loader.cpp`             | `canon_fault_contain_load_and_run()` + `fs_gov_find_fault` extern |
| `ternaryos/hal/qemu_slice6_cpp_bridge.cpp`        | Phase 18 declaration + call                                  |
| `.github/workflows/qemu-boot.yml`                 | Phase 18 build step (LBA 12) + CI gate check                 |
