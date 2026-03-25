# RFC-00CA — EL0 Fault Summary Query

| Field      | Value                                           |
|------------|-------------------------------------------------|
| RFC        | 00CA                                            |
| Title      | EL0 Fault Summary Query                         |
| Status     | **Accepted**                                    |
| Depends-on | RFC-00C9 (EL0 Fault Evidence Query)             |
| Supersedes | —                                               |

---

## 1. Problem Statement

RFC-00C9 added an EL1-internal helper, `fs_sched_get_fault(tid, &ec, &far)`,
which makes retained fault evidence inspectable after the scheduler returns to
EL1.

That closes the EL1 post-mortem gap, but it still leaves no governed way for an
EL0 thread to ask the freestanding scheduler for fault-state information through
the existing KernelCall channel.

The global ABI already reserves `KernelCallKind::QueryFaultSummary` (ordinal 21)
for that purpose.  Prior to RFC-00CA, the freestanding EL0 bridge did not
implement it.

RFC-00CA wires the reserved ordinal into the freestanding scheduler bridge and
proves that an EL0 thread can query fault summary state immediately after a
sibling fault handoff.

---

## 2. ABI Decision

RFC-00CA uses the existing frozen KernelCall ordinal:

```asm
KCALL_QueryFaultSummary = 21
```

No new syscall number and no new KernelCall ordinal are introduced.

Instead, the freestanding bridge adopts the existing hosted wire-response field
offsets for the five fault-summary counters:

- `fault_summary_recorded_faults`
- `fault_summary_pending_faults`
- `fault_summary_delivered_faults`
- `fault_summary_routed_thread_faults`
- `fault_summary_quarantined_threads`

This requires a response buffer large enough to cover offsets `200..232`,
therefore RFC-00CA defines:

```cpp
static constexpr uint64_t kMinFaultSummaryRspBytes = 240u;
```

Requests with a smaller response buffer are rejected as `InvalidRequest`.

---

## 3. Freestanding Semantics

The freestanding scheduler has a narrower fault model than the full kernel.
There is no fault inbox, no supervisor mediation, and no separate delivery
queue.  RFC-00CA therefore defines the following mapping:

Let `faulted = count(FsSchedThread.state == Faulted && fault_ec != 0)`.

Then `QueryFaultSummary` returns:

- `recorded_faults = faulted`
- `pending_faults = 0`
- `delivered_faults = faulted`
- `routed_thread_faults = faulted`
- `quarantined_threads = faulted`

Rationale:

- every retained freestanding fault has already been routed through
  `fs_sched_fault_handler()`,
- there is no separate undelivered/pending queue,
- a `Faulted` thread is terminal and retained in the scheduler table, which is
  the closest freestanding analogue to quarantine history.

---

## 4. EL0 Validation Scenario

Phase 20 proves the EL0-visible path end-to-end with two threads:

- **Faulting thread** (`tid=8`, LBA 12, `el0_fault_test.bin`)
  faults immediately on VA `0x0`.
- **Query thread** (`tid=9`, LBA 13, `el0_fault_summary_test.bin`)
  is already `Runnable`.

Execution:

1. Start `tid=8` as `Running`, `tid=9` as `Runnable`.
2. `tid=8` faults.
3. `fs_sched_fault_handler()` marks `tid=8` `Faulted` and switches directly to
   `tid=9`.
4. `tid=9` issues `KernelCall(QueryFaultSummary)` from EL0 and exits.
5. EL1 inspects the returned response block in `tid=9`'s stack.

Expected summary seen by EL0:

- `recorded_faults = 1`
- `pending_faults = 0`
- `delivered_faults = 1`
- `routed_thread_faults = 1`
- `quarantined_threads = 1`

---

## 5. CI Gate

Phase 20 is verified by:

```
[axion] el0: fault summary OK (tid=9 sees tid=8 fault)
```

This confirms:

1. a fault was retained in the freestanding scheduler,
2. the fault-handler handoff to a Runnable sibling succeeded,
3. the sibling issued `QueryFaultSummary` from EL0 through SVC #1,
4. the freestanding bridge wrote the expected summary fields into the wire
   response block at the hosted-compatible offsets.

---

## 6. Backward Compatibility

RFC-00CA is additive:

- it activates an already-reserved KernelCall ordinal,
- it does not modify existing ordinals,
- it does not change Phase 18/19 gate strings,
- it preserves the compact 48-byte response minimum for all existing bridge
  calls other than `QueryFaultSummary`.

---

## 7. Implementation Files

| File                                           | Change                                                           |
|------------------------------------------------|------------------------------------------------------------------|
| `ternaryos/hal/qemu_slice6_el0_svc_bridge.cpp` | `QueryFaultSummary` dispatch + freestanding summary encoding     |
| `ternaryos/hal/el0_fault_summary_test.S`       | New EL0 client for `KernelCall(QueryFaultSummary)`               |
| `ternaryos/hal/canon_exec_loader.cpp`          | Phase 20 harness validating EL0-visible summary response         |
| `ternaryos/hal/qemu_slice6_cpp_bridge.cpp`     | Phase 20 declaration + call                                      |
| `.github/workflows/qemu-boot.yml`              | LBA 13 build/embed step + Phase 20 gate                          |
| `spec/rfcs/RFC-00CA-el0-fault-summary-query.md` | This RFC                                                       |
