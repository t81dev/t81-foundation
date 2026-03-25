# TernaryOS Scheduler — Graduation Document

**Date:** 2026-03-23
**Milestone:** Scheduler lane graduated from `userland/experimental/` to `ternaryos/`
**RFC range covered:** RFC-00B0 through RFC-00C6

---

## 1. Overview

The TernaryOS cooperative EL0 scheduler began as a freestanding experiment in
`userland/experimental/`.  After completing seven design phases — from bare
KernelCall ABI through per-thread address-space isolation — the lane has been
promoted to a first-class subsystem at `ternaryos/`.

All paths, CMake targets, CI triggers, and spec references have been updated.
No source files were altered during graduation; only directory paths changed.

---

## 2. Stable API Surface

### 2.1 KernelCall Wire Protocol

| Call | kind | Request bytes | Description |
|------|------|---------------|-------------|
| `WaitForDevice` | 43 | 12 (v1) or 16 (v2) | Park calling thread until device IRQ |
| `ExitThread`   |  2 | SVC #2 immediate   | Terminate calling thread             |

**`WaitForDevice` v2 request layout (16 bytes)**

| Offset | Size | Field     | Value                         |
|--------|------|-----------|-------------------------------|
| 0      | 4    | magic     | `KAQR` (0x52514B41)           |
| 4      | 2    | version   | 1                             |
| 6      | 2    | bytes     | 12 (legacy) or 16 (v2)        |
| 8      | 4    | kind      | 43                            |
| 12     | 4    | device_id | INTID to match; 0 = any IRQ   |

Requests with `bytes < 16` are backward-compatible: `device_id` defaults to 0.

### 2.2 Scheduler Functions (`qemu_slice6_el0_svc_bridge.cpp`)

```cpp
// Register a thread before run_proc_entry.
extern "C" void fs_sched_register(uint32_t tid,
                                   uint64_t entry_pa,
                                   uint64_t stack_top,
                                   uint64_t spsr) noexcept;

// Reset all scheduler state (call before each test phase).
extern "C" void fs_sched_reset() noexcept;

// Assign a per-thread L3 page-table slot (RFC-00C6).
// Pass kNoThreadL3 (~0u) to use the shared L3 (default).
extern "C" void fs_sched_set_thread_l3(uint32_t tid, uint32_t l3_slot) noexcept;

// Wake all BlockedDeviceWait threads matching intid (or did==0 wildcard).
// Called from the timer ISR with kTimerIntid = 30.
extern "C" void fs_sched_timer_device_wake(uint32_t intid) noexcept;
```

### 2.3 Observability Ring (`qemu_slice6_el0_svc_bridge.cpp`)

```cpp
// Record an observation event into the obs ring (RFC-00BF).
extern "C" void fs_obs_record(uint32_t tid,
                               uint32_t event,
                               uint64_t pc) noexcept;

// Record a governance event into the gov ring (RFC-00C3).
extern "C" void fs_gov_record(uint32_t tid,
                               uint32_t event,
                               uint32_t device_id) noexcept;

// Query the gov ring for a matching record (RFC-00C4).
// Returns true when (tid, event, device_id) all match.
extern "C" bool fs_gov_find_device(uint32_t tid,
                                    uint32_t event,
                                    uint32_t device_id) noexcept;
```

**Gov ring event constants**

| Constant | Value | Meaning |
|----------|-------|---------|
| `kGovTimerDeviceWake` | 1 | Thread woken by timer/device IRQ |
| `kGovAsyncContextSwitch` | 2 | Async context switch after wfi |

### 2.4 MMU API (`qemu_slice6_el0_mmu.cpp`)

```cpp
// Initialize EL0 page tables (called once at boot).
extern "C" void el0_mmu_init() noexcept;

// Validate that a TVA falls within any mapped EL0 page.
extern "C" bool el0_tva_valid(uint64_t tva) noexcept;

// Page accessors.
extern "C" uint8_t* el0_mmu_proc_code_page()   noexcept; // tid=6 / slot 0
extern "C" uint64_t el0_mmu_proc_stack_top()   noexcept;
extern "C" uint8_t* el0_mmu_proc_code_page2()  noexcept; // tid=7 / slot 1
extern "C" uint64_t el0_mmu_proc_stack_top2()  noexcept;

// Per-thread L3 management (RFC-00C6).
// Build isolated L3 for slot — own pages EL0-accessible, others EL1-only.
extern "C" void el0_mmu_build_thread_l3(uint32_t slot,
                                          uint64_t own_code_pa,
                                          uint64_t own_stack_pa) noexcept;
// Install thread's private L3 before ERET (L2 swap + TLBI).
extern "C" void el0_mmu_install_thread_l3(uint32_t slot) noexcept;
// Restore shared L3 after last thread exits.
extern "C" void el0_mmu_install_shared_l3() noexcept;
```

---

## 3. Data Structures

### 3.1 `FsSchedThread`

```cpp
struct FsSchedThread {
    uint32_t     tid;
    FsSchedState state;          // Init/Runnable/Running/BlockedDeviceWait/Exited
    uint64_t     resume_elr;
    uint64_t     resume_sp_el0;
    uint64_t     resume_spsr;
    uint64_t     ipc_rsp_tva;
    uint64_t     ipc_rsp_size;
    uint32_t     device_id;      // RFC-00C4: INTID filter; 0 = wake on any
    uint32_t     l3_slot;        // RFC-00C6: per-thread L3 index; kNoThreadL3 = shared
};
static constexpr uint32_t kNoThreadL3 = ~0u;
```

### 3.2 `FsGovRecord` (24 bytes)

```cpp
struct FsGovRecord {
    uint32_t seq_id;       // Monotonic sequence within gov ring
    uint32_t tid;          // Thread that was transitioned
    uint32_t event;        // kGovTimerDeviceWake=1, kGovAsyncContextSwitch=2
    uint32_t obs_seq_at;   // Obs ring sequence number at time of event
    uint32_t device_id;    // INTID that fired (0 for non-device events)
    uint32_t _reserved;    // Pad to 24 bytes
};
```

---

## 4. CanonFS LBA Layout (stable)

| LBA | Content | Phase |
|-----|---------|-------|
| 0–2 | Reserved / boot | — |
| 3 | `el0_process_stub.bin` (T81X v1) | Phase 1–5 |
| 4 | IPC test A | Phase 6–7 |
| 5 | IPC test B | Phase 8 |
| 6 | Scheduler test A | Phase 9 |
| 7 | Scheduler test B | Phase 10 |
| 8 | `el0_wait_test.bin` (T81X v2, tid=4) | Phase 11 |
| 9 | T81M manifest | Phase 12 |
| 10 | `el0_wait_test.bin` (T81X v2, tid=5) | Phase 13–14, reused as tid=7 in Phase 16–17 |
| 11 | `el0_device_filter_test.bin` (T81X v2, tid=6) | Phase 15, reused as tid=6 in Phase 16–17 and Phase 19 |
| 12 | `el0_fault_test.bin` (T81X v2, tid=8) | Phase 18, reused as tid=7 in Phase 19 |
| 13 | `el0_fault_summary_test.bin` (T81X v2, tid=9) | Phase 20 |

---

## 5. CI Gates (Phases 11–23)

| Phase | CI gate string |
|-------|----------------|
| 11 | `[axion] el0: identity OK (tid=4)` |
| 12 | `[axion] el0: manifest OK (tid=4)` |
| 13 | `[axion] el0: irq wake OK (tid=5)` |
| 14 | `[axion] el0: obs ring OK (tid=5)` |
| 15 | `[axion] el0: device filter OK (device_id=30, tid=6)` |
| 16 | `[axion] el0: concurrent wake OK (device_id=30, tid=6+7)` |
| 17 | `[axion] el0: per-thread pt OK (tid=6+7, isolated)` |
| 18 | `[axion] el0: fault contained (tid=8, ec=0x24)` |
| 19 | `[axion] el0: concurrent fault OK (tid=7 faulted, tid=6 exited)` |
| 20 | `[axion] el0: fault summary OK (tid=9 sees tid=8 fault)` |
| 21 | `[axion] el0: fault detail OK (tid=10 sees tid=8 ec=0x24 far=0x0)` |
| 22 | `[axion] el0: fault ack OK (tid=11 drained tid=8 fault)` |
| 23 | `[axion] el0: supervisor recovery OK (tid=12 pending=1 drained=1)` |

---

## 6. RFC Dependency Chain

```
RFC-00B0 (KernelCall ABI)
  └─ RFC-00BE (EL0 cooperative scheduler)
       └─ RFC-00BF (freestanding KernelCall observability)
            └─ RFC-00C0 (CanonFS executable identity)
                 └─ RFC-00C1 (T81X v2 / T81M manifest)
                      └─ RFC-00C2 (IRQ-driven WaitForDevice)
                           └─ RFC-00C3 (async audit / gov ring)
                                └─ RFC-00C4 (per-device wake filtering)
                                     └─ RFC-00C5 (concurrent device wait)
                                          └─ RFC-00C6 (per-thread TTBR0 isolation)
```

---

## 7. Files Promoted to Mainline

| Path (under `ternaryos/`) | Purpose |
|---------------------------|---------|
| `hal/qemu_slice6_el0_svc_bridge.cpp` | Scheduler, obs ring, gov ring, SVC handlers |
| `hal/qemu_slice6_el0_mmu.cpp` | EL0 MMU, per-thread L3 tables |
| `hal/qemu_slice6_bridge_irq.cpp` | Timer ISR → `fs_sched_timer_device_wake` |
| `hal/canon_exec_loader.cpp` | Phase 11–17 load-and-run entry points |
| `hal/qemu_slice6_cpp_bridge.cpp` | EFI stub C++ entry → phase dispatch |
| `hal/el0_device_filter_test.S` | Process E (tid=6, device_id=30) |
| `hal/el0_wait_test.S` | Process C/D (tid=4/5, device_id=0) |
| `hal/axion_el0_init.S` | EL0 init trampoline |
| `dev/virtio_blk_mmio.hpp/.cpp` | VirtIO block MMIO driver |
