# RFC-00C4 — Per-Device Wake Filtering

| Field      | Value                                      |
|------------|--------------------------------------------|
| RFC        | 00C4                                       |
| Title      | Per-Device Wake Filtering                  |
| Status     | **Accepted**                               |
| Depends-on | RFC-00C2 (IRQ-driven WaitForDevice), RFC-00C3 (Async Audit) |
| Supersedes | —                                          |

---

## 1. Problem Statement

RFC-00C2 introduced `WaitForDevice` as a blocking syscall that parks an EL0
thread until the next timer IRQ fires.  However, every blocked thread woke on
every IRQ regardless of which interrupt fired.  This coarse-grained policy
cannot model real device multiplexing — a thread waiting on, say, a UART RX
interrupt must not be unblocked by a timer tick.

RFC-00C4 adds a `device_id` discriminator so that a thread is only transitioned
from `BlockedDeviceWait` to `Runnable` when the INTID of the firing interrupt
matches the thread's registered device_id (or when device_id is 0, which means
"wake on any device" and preserves RFC-00C2 semantics).

---

## 2. Wire-Protocol Extension

The `WaitForDevice` KernelCall request gains an optional 4-byte `device_id`
field at bytes `[12:16]`.

| Offset | Size | Field      | Value                      |
|--------|------|------------|----------------------------|
| 0      | 4    | magic      | `KAQR` (0x52514B41)        |
| 4      | 2    | version    | 1                          |
| 6      | 2    | bytes      | 12 (legacy) or 16 (v2)     |
| 8      | 4    | kind       | 43 (`WaitForDevice`)       |
| 12     | 4    | device_id  | INTID to match; 0 = any    |

Requests with `bytes < 16` are treated as `device_id = 0` (backward-compatible
with all RFC-00C2 threads).  The SVC handler reads `device_id` only when
`req_size >= 16`.

---

## 3. Scheduler Extension

### 3.1 `FsSchedThread` — new field

```cpp
struct FsSchedThread {
    uint32_t     tid;
    FsSchedState state;
    uint64_t     resume_elr;
    uint64_t     resume_sp_el0;
    uint64_t     resume_spsr;
    uint64_t     ipc_rsp_tva;
    uint64_t     ipc_rsp_size;
    uint32_t     device_id;   // RFC-00C4: INTID to match on wake; 0 = any
};
```

### 3.2 `fs_sched_timer_device_wake(uint32_t intid)` — signature change

RFC-00C2 defined `fs_sched_timer_device_wake()` (no parameters).  RFC-00C4
changes the signature to accept the firing INTID and applies the filter:

```cpp
extern "C" void fs_sched_timer_device_wake(uint32_t intid) noexcept {
    for (uint32_t i = 0u; i < kMaxSchedThreads; ++i) {
        if (s_sched[i].state == FsSchedState::BlockedDeviceWait) {
            const uint32_t did = s_sched[i].device_id;
            if (did == 0u || did == intid) {
                s_sched[i].state = FsSchedState::Runnable;
                fs_gov_record(s_sched[i].tid, kGovTimerDeviceWake, intid);
            }
        }
    }
}
```

The timer ISR passes `kTimerIntid = 30` (GICv3 PPI30).

### 3.3 WaitForDevice SVC handler

After validating the request header the handler reads `device_id`:

```cpp
uint32_t req_device_id = 0u;
if (req_size >= 16u)
    __builtin_memcpy(&req_device_id, req + 12, 4);
cur->device_id = req_device_id;
```

Threads that pass `device_id = 0` retain RFC-00C2 wake-on-any behaviour.

---

## 4. Governance Ring Extension

`FsGovRecord` is extended to 24 bytes to record the `device_id` that triggered
the wake event:

```cpp
struct FsGovRecord {
    uint32_t seq_id;       // monotonic sequence within this ring
    uint32_t tid;          // thread that was transitioned
    uint32_t event;        // kGovTimerDeviceWake=1, kGovAsyncContextSwitch=2
    uint32_t obs_seq_at;   // obs ring seq number at time of event (causal anchor)
    uint32_t device_id;    // INTID that fired (RFC-00C4); 0 for non-device events
    uint32_t _reserved;    // pad to 24 bytes
};
```

A new query function enables precise per-device lookup:

```cpp
extern "C" bool fs_gov_find_device(uint32_t tid,
                                   uint32_t event,
                                   uint32_t device_id) noexcept;
```

Returns `true` when the governance ring contains a record matching all three
fields.  Used by `canon_device_filter_load_and_run()` to verify Phase 15.

---

## 5. CanonFS Layout

| LBA | Content                                | Details                                    |
|-----|----------------------------------------|--------------------------------------------|
|  8  | `el0_wait_test.bin` (T81X v2, tid=4)   | Phase 11 — identity + direct wake          |
|  9  | T81M manifest (code_hash LBA 8)        | Phase 12 — call-sequence manifest          |
| 10  | `el0_wait_test.bin` (T81X v2, tid=5)   | Phase 13 — IRQ-driven wake (device_id=0)  |
| 11  | `el0_device_filter_test.bin` (T81X v2, tid=6) | Phase 15 — per-device filter (device_id=30) |

---

## 6. Process E — `el0_device_filter_test.S`

Process E (tid=6) is the Phase 15 subject.  It sends a 16-byte
`WaitForDevice` request with `device_id = 30` (GICv3 PPI30 / physical
timer), then calls `ExitThread`.

The 16-byte wire request:

```
[0:4]   KAQR magic
[4:6]   version = 1
[6:8]   bytes = 16
[8:12]  kind = 43 (WaitForDevice)
[12:16] device_id = 30
```

Because `device_id == kTimerIntid (30)`, the filter in
`fs_sched_timer_device_wake(30)` passes: `did == intid` → E transitions to
`Runnable`.

---

## 7. Execution Flow

```
canon_device_filter_load_and_run()
  ├─ Load T81X v2 from LBA 11 (el0_device_filter_test.bin, tid=6)
  ├─ Verify FNV-1a-64 code_hash
  ├─ g_axion_el1_device_wait_pc = &fs_sched_device_wait_loop
  ├─ fs_sched_reset()
  ├─ fs_sched_register(6, code_pa, stack_top, 0x3C0)
  ├─ run_proc_entry(code_pa, stack_top)
  │     EL0: E calls WaitForDevice(device_id=30)
  │       └─ EL1 SVC handler: device_id=30 stored, BlockedDeviceWait
  │             ERET → fs_sched_device_wait_loop
  │               wfi ──(timer IRQ INTID=30)──►
  │                 fs_sched_timer_device_wake(30)
  │                   did==30 match → E = Runnable
  │                   fs_gov_record(6, kGovTimerDeviceWake, 30)
  │               ERET → E resumes
  │     EL0: E calls ExitThread
  │       └─ EL1 SVC #2: ERET → g_axion_el1_return_pc → EL1
  ├─ g_axion_el1_device_wait_pc = 0
  └─ fs_gov_find_device(6, kGovTimerDeviceWake=1, 30)
       → true → print CI gate string
```

---

## 8. CI Gate

Phase 15 is verified by the following serial output gate in
`.github/workflows/qemu-boot.yml`:

```
[axion] el0: device filter OK (device_id=30, tid=6)
```

This confirms:
1. `el0_device_filter_test.bin` was loaded from LBA 11 with a valid T81X v2
   header and matching FNV-1a-64 code_hash.
2. Process E successfully called `WaitForDevice(device_id=30)` and parked in
   the IRQ-driven device-wait loop.
3. The timer IRQ (INTID 30) fired and the per-device filter correctly matched
   `device_id == intid == 30`, transitioning E to `Runnable`.
4. E resumed, called `ExitThread`, and returned to EL1.
5. The governance ring contains a `kGovTimerDeviceWake` record for tid=6 with
   `device_id=30`, queryable via `fs_gov_find_device(6, 1, 30)`.

---

## 9. Backward Compatibility

| Thread | Request bytes | device_id stored | Wake condition         |
|--------|---------------|------------------|------------------------|
| tid=4  | 12            | 0 (any)          | wake on any INTID      |
| tid=5  | 12            | 0 (any)          | wake on any INTID      |
| tid=6  | 16            | 30               | wake only on INTID 30  |

Threads using the 12-byte request format (RFC-00C2) continue to receive
`device_id = 0` and are woken by any IRQ — no behaviour change.

---

## 10. Implementation Files

| File                                         | Change                                         |
|----------------------------------------------|------------------------------------------------|
| `userland/experimental/hal/qemu_slice6_el0_svc_bridge.cpp` | `FsSchedThread.device_id`, `FsGovRecord` 24-byte layout, `fs_sched_timer_device_wake(intid)`, `fs_gov_find_device()`, WaitForDevice SVC reads device_id |
| `userland/experimental/hal/qemu_slice6_bridge_irq.cpp`     | Timer ISR passes `kTimerIntid` to `fs_sched_timer_device_wake` |
| `userland/experimental/hal/canon_exec_loader.cpp`          | `canon_device_filter_load_and_run()` + `fs_gov_find_device` extern |
| `userland/experimental/hal/qemu_slice6_cpp_bridge.cpp`     | Declaration + call for `canon_device_filter_load_and_run()` |
| `userland/experimental/hal/el0_device_filter_test.S`       | New — Process E, 16-byte WaitForDevice(device_id=30) |
| `.github/workflows/qemu-boot.yml`                          | Phase 15 build step (LBA 11) + CI gate check  |
