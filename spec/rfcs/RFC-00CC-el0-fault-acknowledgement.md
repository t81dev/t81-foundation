# RFC-00CC: EL0 Fault Acknowledgement and Drain

**Status:** accepted  
**Type:** standards-track  
**Applies-To:** freestanding EL0 bridge, slice6 scheduler fault lifecycle, governed fault observation  
**Created:** 2026-03-25  
**Updated:** 2026-03-25  

---

## Summary

RFC-00CC defines how retained EL0 fault state transitions from "recorded and
queryable" to "acknowledged and drained". The current freestanding slice6 lane
proves fault containment, concurrent sibling survival, EL1-local evidence
inspection, EL0 fault summary query, EL0 fault detail query, and operator-shell
introspection. What it still lacks is a governed way to consume that retained
fault state exactly once and observe that the inbox has been drained
deterministically. RFC-00CC introduces a minimal acknowledgement surface for
retained thread faults.

## Motivation

RFC-00C7 through RFC-00CB deliberately focused on proving that fault metadata is
retained and inspectable:

- RFC-00C7: a faulted EL0 thread is contained
- RFC-00C8: a healthy sibling survives
- RFC-00C9: EL1 can inspect retained `{fault_ec, fault_far}`
- RFC-00CA: EL0 can query fault-summary counters
- RFC-00CB: EL0 can query retained per-thread fault detail

That arc leaves one operational gap: retained faults accumulate until
`fs_sched_reset()`. There is no defined lifecycle for an observer that has
already consumed a fault record and wants the runtime to mark it handled.

Without an acknowledgement step, the current model cannot answer:

- whether a fault has merely been recorded or has been consumed by an observer
- whether "pending fault" means "unseen by EL0" or simply "still retained"
- whether a later EL0 query should observe the same retained record again
- how fault-summary counters should evolve after a record is processed

RFC-00CC closes that gap by defining a single-step transition from retained
fault evidence to drained fault evidence.

## Proposal

### 1. Retained-fault lifecycle

RFC-00CC defines the following lifecycle for a faulted EL0 thread in the
freestanding scheduler:

1. `Faulted` and retained:
   `fault_ec != 0`, `fault_far` stable, thread visible to EL1/EL0 query surfaces.
2. Acknowledged:
   an explicit governed acknowledgement names the faulted `tid`.
3. Drained:
   the retained fault record is no longer visible through EL0 fault-detail
   query, and summary counters reflect that the fault is no longer pending.

The key invariant is that acknowledgement is explicit. Querying fault detail
does not itself implicitly consume or clear a fault record.

### 2. New EL1 helper

RFC-00CC proposes a minimal read-write helper for the freestanding scheduler:

```cpp
extern "C" bool fs_sched_ack_fault(uint32_t tid) noexcept;
```

Behavior:

- returns `true` iff `tid` exists and is currently `Faulted` with retained fault
  evidence
- clears retained fault visibility for that thread
- is idempotent at the state-machine level: once drained, subsequent calls
  return `false`
- does not mutate unrelated scheduler state

The concrete representation is implementation-defined. One valid design is:

- clear `fault_ec` and `fault_far`
- retain `state == Faulted` for audit/history classification

Another valid design is:

- add an explicit `fault_acknowledged` bit while keeping `fault_ec`/`fault_far`
  intact internally
- hide acknowledged records from EL0-visible query surfaces

RFC-00CC does not require one representation over the other, but it does
require deterministic external behavior.

### 3. New EL0 query/ack contract

RFC-00CC proposes a new freestanding KernelCall:

- `AcknowledgeThreadFault`

Inputs:

- `target_tid`

Outputs:

- `StatusOk` if the retained fault existed and is now acknowledged/drained
- `RetryLater/FaultInboxEmpty` if no retained fault exists for `target_tid`

The call is governed and auditable in the same style as the existing fault
query path. It is not a general supervisor operation; it only acknowledges
retained fault evidence.

### 4. Summary semantics after acknowledgement

After successful acknowledgement:

- `ReadFaultInbox(target_tid)` must no longer return retained detail for that
  thread
- `QueryFaultSummary` must report one fewer pending/drainable fault
- previously recorded governance history remains immutable

RFC-00CC preserves the distinction between:

- immutable audit/event history
- mutable retained-inbox state

### 5. Operator-shell implications

The slice6 operator shell may expose acknowledgement indirectly later, but
RFC-00CC does not require a shell builtin. The first implementation should
prove the lifecycle through EL1 harness code and EL0 test binaries before any
interactive shell control surface is added.

## Determinism / Safety Considerations

- Acknowledgement must be deterministic and single-thread safe within the
  cooperative freestanding scheduler.
- Query does not imply mutation; mutation requires an explicit acknowledgement
  operation.
- Audit history must remain append-only even after retained fault state is
  drained.
- Acknowledging a fault must not resurrect or rerun the faulted thread.
- Clearing retained fault visibility must not corrupt sibling thread progress or
  current scheduler state.

## Compatibility

RFC-00CC is additive.

- Existing RFC-00C7 through RFC-00CB behavior remains valid before any
  acknowledgement is performed.
- Existing shells and test harnesses that only query faults remain valid.
- The only semantic extension is that a retained fault can now transition into a
  drained state before `fs_sched_reset()`.

## Implementation Plan

1. Add `fs_sched_ack_fault(tid)` to the freestanding scheduler.
2. Define the EL0 bridge ordinal and wire request/response layout for
   `AcknowledgeThreadFault`.
3. Add a new EL0 test binary and loader phase proving:
   - sibling fault occurs
   - EL0 detail query sees it
   - EL0 acknowledgement succeeds
   - subsequent detail query reports not found / empty
   - summary counters reflect the drained state
4. Add a Phase 22 CI gate:
   `[axion] el0: fault ack OK (tid=11 drained tid=8 fault)`.
5. Optionally add operator-shell read-only reflection if the drained/pending
   distinction becomes useful at the prompt.

## Open Questions

1. Should acknowledgement hide retained detail only from EL0, or from EL1
   helpers as well?
2. Should a drained fault remain in `Faulted` state, or should the scheduler add
   a distinct `FaultAcknowledged` / `FaultDrained` state?
3. Should `QueryFaultSummary` distinguish `recorded` from `pending`, or is the
   current compact counter model sufficient?
4. Should acknowledgement require a dedicated capability if/when the
   freestanding lane grows a richer principal model?

## Acceptance Criteria

- A freestanding helper exists that acknowledges retained fault state for a
  named `tid`.
- An EL0 observer can acknowledge a sibling's retained fault deterministically.
- After acknowledgement, `ReadFaultInbox(target_tid)` reports
  `RetryLater/FaultInboxEmpty`.
- Fault-summary counters change in a deterministic, documented way after
  acknowledgement.
- Governance history for the original fault remains queryable and unchanged.
- Phase 22 proves the full `query -> acknowledge -> drained` lifecycle with CI
  gate `[axion] el0: fault ack OK (tid=11 drained tid=8 fault)`.
