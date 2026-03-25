# RFC-00CD: Supervisor Fault Recovery Status

**Status:** accepted  
**Type:** standards-track  
**Applies-To:** supervisor fault lifecycle, freestanding/hosted recovery queries, KernelCall recovery status surfaces  
**Created:** 2026-03-25  
**Updated:** 2026-03-25  

---

## Summary

RFC-00CD defines the next layer above per-thread fault handling: a
supervisor-visible recovery-status model that aggregates thread and group fault
state into a deterministic status surface. The current slice6 lane now proves
containment, sibling survival, retained evidence query, EL0-visible summary and
detail, and explicit acknowledgement/drain of retained thread fault state.
What is still missing is a governed way to answer the higher-level question:
"what recovery work remains for the supervising authority?" RFC-00CD defines
that status model and the minimal query contract for it.

## Motivation

RFC-00C7 through RFC-00CC deliberately focused on thread-local correctness:

- RFC-00C7: contain a single fault
- RFC-00C8: keep a healthy sibling alive
- RFC-00C9: inspect retained thread fault evidence from EL1
- RFC-00CA: query retained fault summary from EL0
- RFC-00CB: query retained per-thread fault detail from EL0
- RFC-00CC: acknowledge and drain retained thread fault evidence

That arc is now coherent at the thread level. The remaining gap is supervisory
state.

Once a system has more than one thread, or more than one service under the same
supervisor, thread-local primitives are not enough. The runtime needs a stable
answer to questions like:

- does this supervisor currently own any faulted threads?
- are those faults merely recorded, still pending recovery action, or already
  drained at the thread-inbox level?
- is the supervisor recoverable, quarantined, degraded, or blocked on operator
  action?
- has a whole fault group been acknowledged, or are faults still outstanding?

Those are not shell concerns and not per-thread query concerns. They are the
next real architectural layer in the fault model.

## Proposal

### 1. Supervisor-facing status is distinct from thread inbox state

RFC-00CD separates:

- **thread fault evidence**
  retained `{fault_ec, fault_far}` and per-thread inbox visibility
- **supervisor recovery state**
  aggregate status describing whether a supervisor still has recovery work to do

Acknowledging or draining a thread fault inbox does not, by itself, fully
define supervisor recovery completion. Thread-level evidence and
supervisor-level recovery state are related but not identical surfaces.

### 2. Recovery status model

RFC-00CD proposes a minimal recovery-state classification:

- `Healthy`
  no faulted threads or recovery obligations are present
- `FaultRecorded`
  at least one governed fault has been attributed to the supervisor
- `RecoveryPending`
  recovery action is still required before the supervisor can be considered
  healthy again
- `RecoveryAcknowledged`
  the supervising authority has explicitly acknowledged the affected fault group
- `RecoveryBlocked`
  recovery cannot complete yet because a prerequisite remains unmet
- `RecoveryComplete`
  the supervisor has no remaining pending recovery obligations for the fault
  group in question

The exact numeric encoding is implementation-defined, but the externally
observable state transitions must be deterministic.

### 3. Query surfaces

RFC-00CD does not allocate new ordinals. It activates and constrains the
already-frozen recovery query surfaces from RFC-00B6 / RFC-00BD:

- `QuerySupervisorStatus` (ordinal 22)
- `QuerySupervisorRecoveryStatus` (ordinal 23)
- `AcknowledgeSupervisorFaultGroup` (ordinal 17)

The intended division is:

- `QuerySupervisorStatus`
  broad current-state summary for a supervisor
- `QuerySupervisorRecoveryStatus`
  fault/recovery-specific aggregate state
- `AcknowledgeSupervisorFaultGroup`
  explicit state transition for a supervisor-owned fault group

RFC-00CD only standardizes the recovery-facing subset needed to make those
three surfaces coherent.

### 4. Required recovery counters / flags

At minimum, supervisor recovery status must expose enough information to answer:

- whether any governed fault has been recorded for the supervisor
- whether any recovery work is still pending
- whether any fault group has been acknowledged
- whether any faulted threads remain undrained
- whether recovery is blocked or complete

One valid compact wire shape is:

- `recorded_fault_groups`
- `pending_fault_groups`
- `acknowledged_fault_groups`
- `blocked_fault_groups`
- `recoverable_threads`
- `drained_threads`
- `quarantined_threads`

RFC-00CD does not freeze the exact byte layout yet, but it does require that
the response be mechanically derivable from kernel state and stable under
replay.

### 5. Relationship to RFC-00CC

RFC-00CC drains retained thread inbox state. RFC-00CD defines what still
remains visible after that drain:

- thread inbox detail may be empty
- immutable governance history remains
- supervisor recovery status may still indicate a recorded or acknowledged
  fault group until higher-level recovery criteria are satisfied

This is the key distinction the current system does not yet formalize.

### 6. First freestanding milestone

The first implementation milestone should stay narrow.

Phase 23 narrows the first freestanding proof to status visibility only.

It proves that after a controlled sibling-fault scenario:

1. an EL0 observer can drain the faulted sibling at the thread-inbox level via
   `AcknowledgeThreadFault`,
2. `QuerySupervisorRecoveryStatus` is callable immediately afterward,
3. the returned recovery counters show `pending_fault_groups == 1` while
   `drained_threads == 1`,
4. thread-level drain and supervisor-level recovery completion remain distinct.

This keeps the first implementation small while still proving the core
architectural distinction RFC-00CD exists to capture.

## Determinism / Safety Considerations

- Supervisor recovery status must be derived from canonical runtime state, not
  wall-clock timing or polling races.
- Query surfaces must remain read-only unless the specific acknowledgement call
  is invoked.
- Acknowledging a supervisor fault group must not implicitly rerun, revive, or
  unquarantine threads.
- Supervisor-level acknowledgement must not erase immutable governance history.
- Freestanding and hosted implementations must agree on the meaning of
  "pending", "acknowledged", and "complete" even if their internal
  representations differ.

## Compatibility

RFC-00CD is additive.

- RFC-00C7 through RFC-00CC remain valid and unchanged.
- Thread-level fault query and drain semantics stay intact.
- Existing shells need not expose supervisor recovery state immediately.
- The RFC narrows existing reserved ABI surfaces rather than widening the ABI.

## Implementation Plan

1. Define minimal retained supervisor-recovery bookkeeping in the kernel.
2. Specify the compact response shape for `QuerySupervisorRecoveryStatus`.
3. Wire the freestanding bridge activation for ordinal `23`.
4. Add a new proof phase and CI gate for
   `[axion] el0: supervisor recovery OK (tid=12 pending=1 drained=1)`.
5. Define the minimum viable `AcknowledgeSupervisorFaultGroup` mapping for the
   freestanding lane as a follow-on.
6. Only after that, decide whether the slice6 shell needs a read-only
   supervisor recovery command.

## Open Questions

1. What is the canonical grouping key for "fault group" in the freestanding
   lane before full hosted supervisor state is present?
2. Should thread-level drain be a prerequisite for supervisor-group
   acknowledgement, or may the two surfaces advance independently?
3. Does `QuerySupervisorStatus` need to duplicate recovery counters, or should
   it only carry a coarse summary bit and defer detail to ordinal `23`?
4. When recovery becomes "complete", should that clear "recorded" counts, or
   should recorded history remain visible while pending counts drop to zero?

## Acceptance Criteria

- A deterministic supervisor recovery-status model is specified.
- The role of ordinal `23` is activated coherently with the existing fault arc.
- The RFC clearly distinguishes thread-inbox drain from supervisor recovery
  completion.
- Phase 23 proves `pending_fault_groups == 1` and `drained_threads == 1` after
  a thread-level drain.
- The resulting design is implemented without changing the frozen ABI ordinal
  table.
