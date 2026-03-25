# RFC-00CB — EL0 Fault Detail Query

| Field      | Value                                           |
|------------|-------------------------------------------------|
| RFC        | 00CB                                            |
| Title      | EL0 Fault Detail Query                          |
| Status     | **Accepted**                                    |
| Depends-on | RFC-00CA (EL0 Fault Summary Query)              |
| Supersedes | —                                               |

---

## 1. Problem Statement

RFC-00CA proved that an EL0 sibling can learn that a fault happened by querying
summary counters through `KernelCall(QueryFaultSummary)`.

That still leaves one gap: the observing EL0 thread cannot ask for the retained
per-thread detail that actually identifies the fault:

- which thread faulted,
- which ESR Exception Class was captured,
- which `FAR_EL1` value was retained.

The frozen ABI already reserves `KernelCallKind::ReadFaultInbox` (ordinal 15)
for governed fault-detail reads. Prior to RFC-00CB, the freestanding EL0
bridge did not implement that ordinal.

RFC-00CB activates `ReadFaultInbox` in the freestanding bridge and proves that
an EL0 sibling can query retained `subject_tid`, `fault_ec`, and `fault_far`
immediately after a sibling fault handoff.

---

## 2. ABI Decision

RFC-00CB uses the already-frozen ordinal:

```asm
KCALL_ReadFaultInbox = 15
```

No new KernelCall ordinal is introduced.

The freestanding request shape is minimal:

- request bytes must be at least `16`
- `req[12:16]` carries `target_tid`

The freestanding response uses the common wire header plus a compact retained
detail block:

- `queried_tid @ 40` (`uint32_t`)
- `caller_tid  @ 44` (`uint32_t`)
- `subject_tid @ 200` (`uint32_t`)
- `fault_ec    @ 204` (`uint32_t`)
- `fault_far   @ 208` (`uint64_t`)
- `retained    @ 216` (`uint32_t`, `1 = valid detail`)

Therefore RFC-00CB defines:

```cpp
static constexpr uint64_t kMinFaultInboxRspBytes = 224u;
```

Requests with a smaller response buffer are rejected as `InvalidRequest`.

---

## 3. Freestanding Semantics

The freestanding scheduler does not implement the full hosted fault inbox,
capability gate, or acknowledgement flow. Instead, it maps `ReadFaultInbox`
onto the retained fault evidence frozen by RFC-00C9:

```cpp
fs_sched_get_fault(target_tid, &fault_ec, &fault_far)
```

Semantics:

- if retained fault evidence exists for `target_tid`, `ReadFaultInbox` returns
  `Ok` and writes the compact detail block
- if no retained fault evidence exists, `ReadFaultInbox` returns
  `RetryLater` with rejection `FaultInboxEmpty`

This preserves the hosted intent of “read the currently retained fault detail”
without pretending that freestanding has a multi-entry inbox queue.

---

## 4. EL0 Validation Scenario

Phase 21 proves the path end-to-end with two threads:

- **Faulting thread** (`tid=8`, LBA 12, `el0_fault_test.bin`)
  faults immediately on VA `0x0`
- **Observer thread** (`tid=10`, LBA 14, `el0_fault_detail_test.bin`)
  is already `Runnable`

Execution:

1. Start `tid=8` as `Running`, `tid=10` as `Runnable`.
2. `tid=8` faults.
3. `fs_sched_fault_handler()` marks `tid=8` `Faulted` and switches directly to
   `tid=10`.
4. `tid=10` issues `KernelCall(ReadFaultInbox)` targeting `tid=8`.
5. EL1 inspects the returned response block in `tid=10`'s stack.

Expected detail seen by EL0:

- `queried_tid = 8`
- `caller_tid = 10`
- `subject_tid = 8`
- `fault_ec = 0x24`
- `fault_far = 0x0`
- `retained = 1`

---

## 5. CI Gate

Phase 21 is verified by:

```
[axion] el0: fault detail OK (tid=10 sees tid=8 ec=0x24 far=0x0)
```

This confirms:

1. the retained fault was preserved after fault-handler handoff,
2. `ReadFaultInbox` is live at EL0 through SVC #1,
3. the observing sibling received the exact retained fault detail needed for a
   governed post-mortem.

---

## 6. Backward Compatibility

RFC-00CB is additive:

- it activates an already-reserved KernelCall ordinal,
- it does not modify existing ordinals,
- it does not change Phase 18–20 gates,
- it narrows freestanding `ReadFaultInbox` to retained fault evidence rather
  than a hosted multi-entry inbox surface.

---

## 7. Implementation Files

| File                                            | Change                                                          |
|-------------------------------------------------|-----------------------------------------------------------------|
| `ternaryos/hal/qemu_slice6_el0_svc_bridge.cpp`  | `ReadFaultInbox` dispatch + compact retained fault detail block |
| `ternaryos/hal/el0_fault_detail_test.S`         | New EL0 client for `KernelCall(ReadFaultInbox)`                 |
| `ternaryos/hal/canon_exec_loader.cpp`           | Phase 21 harness validating EL0-visible fault detail response   |
| `ternaryos/hal/qemu_slice6_cpp_bridge.cpp`      | Phase 21 declaration + call                                     |
| `.github/workflows/qemu-boot.yml`               | LBA 14 build/embed step + Phase 21 gate                         |
| `spec/rfcs/RFC-00CB-el0-fault-detail-query.md`  | This RFC                                                        |
