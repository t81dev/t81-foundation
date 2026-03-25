# RFC-00CE: Supervisor Fault-Group Acknowledgement

**Status:** accepted  
**Type:** standards-track  
**Applies-To:** supervisor recovery lifecycle, fault-group acknowledgement, freestanding recovery bridge  
**Created:** 2026-03-25  
**Updated:** 2026-03-25  

---

## Summary

RFC-00CE completes the recovery arc opened by RFC-00CD. RFC-00CD proved that
thread-level drain does not imply supervisor-level recovery completion: after
`AcknowledgeThreadFault`, `QuerySupervisorRecoveryStatus` still reports
`pending_fault_groups == 1` while `drained_threads == 1`. RFC-00CE defines the
next explicit transition: acknowledging the supervisor-owned fault group
through ordinal `17` so that recovery state can move from "pending" to
"acknowledged/complete" deterministically.

## Motivation

The fault stack is now split cleanly into two levels:

- thread-local retention and drain
- supervisor-level recovery state

That split is necessary, but incomplete without a supervisor-level action.

Today, the system can answer:

- a thread fault happened
- the sibling survived
- the retained thread inbox is now drained
- supervisor recovery is still pending

What it still cannot do is express the next state transition:

- the supervising authority has seen the fault group and accepted responsibility
  for recovery/teardown

Without that step, `pending_fault_groups` has no deterministic path to zero.

## Proposal

### 1. Activate ordinal 17 narrowly

RFC-00CE activates the already-frozen ABI surface:

- `AcknowledgeSupervisorFaultGroup` (ordinal `17`)

The first freestanding implementation should stay intentionally small:

- no general supervisor object model
- no dynamic fault-group allocation
- no multi-group routing logic

Instead, the freestanding lane may define a single synthetic recovery group:

- `group_id = 1` iff any faulted threads currently exist in the scheduler lane

That is sufficient to prove the state transition contract before the richer
hosted model is brought in.

### 2. Supervisor-group acknowledgement semantics

`AcknowledgeSupervisorFaultGroup(group_id)` succeeds iff:

- the named freestanding recovery group currently exists
- the group still has pending supervisor recovery work

On success:

- `pending_fault_groups` decreases deterministically
- `acknowledged_fault_groups` increases deterministically
- `blocked_fault_groups` remains unchanged unless a separate prerequisite model
  is introduced later
- thread execution state is not revived or rerun
- immutable governance history is not erased

### 3. Relationship to thread drain

RFC-00CE preserves the distinction from RFC-00CC:

- `AcknowledgeThreadFault`
  drains retained per-thread fault inbox detail
- `AcknowledgeSupervisorFaultGroup`
  advances supervisor-level recovery state

One does not imply the other.

The freestanding Phase 24 proof should perform them in sequence:

1. drain thread fault state
2. query supervisor recovery status -> pending
3. acknowledge supervisor fault group
4. query supervisor recovery status again -> acknowledged/complete

### 4. Recovery status effect

After successful supervisor-group acknowledgement, one valid compact mapping is:

- `recorded_fault_groups == 1`
- `pending_fault_groups == 0`
- `acknowledged_fault_groups == 1`
- `blocked_fault_groups == 0`
- `recoverable_threads == 1`
- `drained_threads == 1`
- `quarantined_threads == 1`

That mapping preserves the distinction between:

- immutable historical existence of the fault group (`recorded == 1`)
- mutable recovery obligation (`pending -> 0`, `acknowledged -> 1`)

RFC-00CE does not require this exact tuple in all future implementations, but
the first freestanding proof should use a similarly explicit, deterministic
transition.

## Determinism / Safety Considerations

- Group acknowledgement must be explicit; no query path may implicitly clear
  recovery state.
- Acknowledging a supervisor group must not restart, unquarantine, or otherwise
  mutate the faulted thread beyond recovery bookkeeping.
- Governance and audit history must remain append-only.
- Freestanding and hosted implementations must agree on the meaning of
  "acknowledged" even if group identity becomes richer later.

## Compatibility

RFC-00CE is additive.

- RFC-00C7 through RFC-00CD remain valid.
- Existing thread-level fault query/drain behavior is unchanged.
- The RFC activates ordinal `17`, which was already frozen in the ABI.

## Implementation Plan

1. Define minimal freestanding fault-group bookkeeping, initially with one
   synthetic group when any faulted thread exists.
2. Activate `AcknowledgeSupervisorFaultGroup` in the freestanding bridge.
3. Extend `QuerySupervisorRecoveryStatus` to reflect the acknowledged state.
4. Add a new EL0 client and Phase 24 harness proving:
   - thread fault drain
   - supervisor recovery pending
   - group acknowledgement succeeds
   - recovery status changes deterministically afterward
5. Add a Phase 24 CI gate:
   `[axion] el0: supervisor ack OK (tid=13 pending->0 acked=1)`.

## Open Questions

1. Should the freestanding synthetic group key be hard-coded to `1`, or should
   it mirror a future hosted supervisor identifier even before that model is
   implemented?
2. Should supervisor-group acknowledgement require prior thread-level drain, or
   should Phase 24 intentionally prove they are independent?
3. Once group acknowledgement succeeds, should `QuerySupervisorStatus` expose a
   coarse `recovery_acknowledged` bit immediately, or should that remain scoped
   to ordinal `23` until later?

## Acceptance Criteria

- The role of ordinal `17` is activated coherently with RFC-00CD.
- The RFC clearly distinguishes supervisor-group acknowledgement from
  thread-level drain.
- A deterministic post-acknowledgement recovery-state transition is defined.
- Phase 24 proves supervisor recovery moves from `pending_fault_groups == 1`
  to `pending_fault_groups == 0` with `acknowledged_fault_groups == 1`.
- The design is implemented without changing the frozen ABI ordinal table.
