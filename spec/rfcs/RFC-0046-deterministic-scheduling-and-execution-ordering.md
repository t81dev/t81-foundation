# RFC-0046: Deterministic Scheduling and Execution Ordering

- **RFC-ID:** RFC-0046
- **Title:** Deterministic Scheduling and Execution Ordering
- **Status:** accepted
- **Type:** standards-track
- **Applies-To:** T81VM execution ordering, DPE task scheduling, Axion-visible execution order, future multi-core and distributed scheduling
- **Created:** 2026-03-19
- **Updated:** 2026-03-21
- **Supersedes:** None
- **Discussion:** Builds on RFC-0002 and RFC-DPE-0001 through RFC-DPE-0009

## Summary

This RFC defines the deterministic scheduling contract for T81.

It specifies:

- what execution ordering means on the deterministic surface
- which schedule properties are observable
- how task and epoch execution order is governed
- how conflicts between parallel work are resolved constitutionally

The goal is to ensure that concurrency and parallelism remain controlled realizations of one deterministic computation rather than sources of competing histories.

## Motivation

T81 already has:

- a deterministic execution contract
- DPE task graph primitives
- epoch execution and canonical commit
- bounded thread-pool execution

What is still missing is a single RFC stating the constitutional rule for scheduling itself.

Without it:

- concurrency semantics remain partially inferred from implementation
- DPE rules appear separate from core execution rules
- future multi-core, JIT, or distributed work can reopen execution-order ambiguity

## Proposal

### 1. Scheduling Principle

On the deterministic surface, scheduling is not “whatever order happened at runtime.”

Scheduling is the governed mapping from:

- executable work items
- dependency constraints
- policy constraints

to a single deterministic observable history.

Any implementation may use internal parallelism, but the observable history must be equivalent to the governed schedule.

### 2. Observable Scheduling Boundary

Scheduling is observable only through:

- final committed state
- deterministic fault result
- Axion-visible scheduling and epoch audit meaning
- trace-visible ordering where ordering is part of the governed surface

The system does not promise wall-clock timing or specific host-thread interleavings.

It promises deterministic meaning.

### 3. Single-Threaded Core Rule

For non-DPE execution, the default ordering rule is instruction order as defined by the TISC program counter and the deterministic execution contract.

No host scheduling effect may alter:

- which instruction is observed next
- which trap occurs first
- which canonical state transition happens first

### 4. DPE Scheduling Rule

For DPE-governed execution, scheduling consists of two layers:

1. execution eligibility determined by the task graph and epoch rules
2. canonical commit ordering determined by governed ordering rules

Parallel execution may occur internally, but canonical outcome is defined by:

- dependency legality
- epoch snapshot semantics
- canonical commit ordering

The number of host threads used is not part of deterministic meaning.

### 5. Eligibility and Readiness

A task becomes eligible only when:

1. its declared dependencies are satisfied
2. its required input snapshot is fixed
3. its policy boundary permits execution

Readiness must be derived from governed metadata only.

Runtime timing, queue jitter, or host scheduler behavior may not change readiness semantics.

### 6. Ordering Classes

This RFC recognizes three ordering classes:

#### Program Order

Used by ordinary VM instruction execution.

#### Dependency Order

Used by DPE task eligibility and DAG legality.

#### Canonical Commit Order

Used when independently executed work becomes part of canonical state.

If these classes disagree internally, canonical commit order determines the final observable write history, while preserving dependency legality.

### 7. Conflict Resolution

When independently executed work produces overlapping effects, the outcome MUST be governed by an explicit ordering rule.

For DPE-governed epochs, that rule is supplied by the DPE canonical commit model.

Future scheduling domains MUST define an equivalent explicit conflict rule before entering a deterministic surface.

Implicit “winner depends on timing” behavior is forbidden.

### 8. Policy Interaction

Scheduling decisions that cross policy boundaries MUST remain deterministic.

This includes:

- whether work is allowed to begin
- whether an epoch is aborted
- what audit meaning is emitted

Policy evaluation may constrain the schedule, but may not introduce schedule ambiguity.

### 9. Timeout and Abort Semantics

Timeouts and aborts are part of the scheduling contract when they are on the deterministic surface.

If a timeout policy is governed, then:

- the timeout class
- abort effect
- recovery behavior

must be deterministic with respect to the governed execution model.

No partial canonical commit may leak through an aborted governed scheduling unit.

### 10. Thread Pool Independence

Bounded thread pools and worker assignment are implementation details unless explicitly promoted.

Worker identity, host thread identity, and host CPU placement MUST NOT appear in deterministic meaning.

Different worker assignment strategies are allowed if they preserve:

- identical final state
- identical faults
- identical governed audit meaning

### 11. Future Multi-Core and Distributed Extension

This RFC defines the local constitutional rule that future extensions must inherit.

Future multi-core or distributed execution may add:

- more schedulable units
- more sophisticated readiness rules
- cross-node ordering

But they may not relax the core requirement:

> one governed computation must have one deterministic observable history

### 12. Relationship to Memory Model

Scheduling and memory cannot be separated completely.

This RFC defines ordering.
The memory model RFC defines visibility and state semantics.

Together they define deterministic concurrency.

## Acceptance Criteria

This RFC is ready for `accepted` when all of the following are true:

1. the distinction between program order, dependency order, and canonical commit order is reflected consistently in DPE and governance language
2. scheduling observability is defined consistently with Axion audit and trace surfaces
3. conflict resolution rules for parallel work are explicitly referenced rather than inferred from worker timing
4. timeout/abort semantics are documented as governed schedule effects where applicable
5. future multi-core or distributed work is explicitly constrained to inherit this ordering contract

## Impact

### Backward Compatibility

This RFC should not change current user-visible semantics.

It constrains future concurrency implementations and clarifies what existing DPE behavior means constitutionally.

### Performance

It does not prohibit parallelism.

It prohibits relying on nondeterministic interleaving as a semantic mechanism.

### Security

The RFC reduces risk of hidden race semantics, policy-order ambiguity, and backend-dependent schedule drift.

## Alternatives Considered

### Let DPE RFCs stand alone as the scheduling model

Rejected because the project needs a cross-cutting scheduling constitution, not just an epoch-series implementation story.

### Treat scheduling as non-observable as long as outputs match

Rejected because T81 also governs traps, audit meaning, and trace-visible ordering.

### Defer scheduling governance until distributed execution exists

Rejected because the ambiguity already exists with current DPE and future JIT interaction.

## References

- `spec/rfcs/RFC-0002-deterministic-execution-contract.md`
- `spec/rfcs/RFC-0045-deterministic-memory-model.md`
- `spec/rfcs/RFC-0053-distributed-deterministic-execution-protocol.md`
- `spec/rfcs/RFC-DPE-0001-deterministic-parallel-execution-vision.md`
- `spec/rfcs/RFC-DPE-0002-tisc-task-graph-primitives.md`
- `spec/rfcs/RFC-DPE-0003-epoch-execution-and-canonical-commit.md`
- `spec/rfcs/RFC-DPE-0004-topological-execution-order.md`
- `spec/rfcs/RFC-DPE-0005-parallel-epoch-execution.md`
- `spec/rfcs/RFC-DPE-0006-bounded-thread-pool.md`
- `spec/rfcs/RFC-DPE-0007-epoch-execution-timeout.md`
- `spec/rfcs/RFC-DPE-0008-epoch-audit-events.md`

## Implementation Record (2026-03-21)

All acceptance criteria are satisfied as of this date.

**AC1 — Program order / dependency order / canonical commit order consistently reflected in DPE language:**
RFC-0046 §6 defines all three ordering classes canonically.  The terminology is uniformly
adopted across the DPE RFC suite: RFC-DPE-0003 §2 ("Canonical Commit Ordering", TaskId-ascending
commit rule), RFC-DPE-0004 §2 ("Topological Execution Order"), and RFC-DPE-0005 §2.2
("within-level ordering is ascending canonical TaskId") all use the same three-class
vocabulary without introducing local alternatives.  No DPE RFC contradicts or re-derives the ordering hierarchy.

**AC2 — Scheduling observability defined consistently with Axion audit and trace surfaces:**
RFC-0046 §2 enumerates exactly four observable scheduling surfaces: final committed state,
deterministic fault result, Axion-visible scheduling and epoch audit meaning, and
trace-visible ordering.  RFC-DPE-0008 (Epoch Audit Events) instantiates the Axion-visible
surface with `EpochSubmitted`, `EpochCommitted`, and `EpochAborted` events tied to
scheduling transitions, with audit entries following the same sequenced CanonHash81-signed
path as existing policy events.  RFC-DPE-0003 §7 shows that epoch state-machine transitions
are recorded in the Axion audit log with sequence numbers, satisfying the trace-visible ordering requirement.

**AC3 — Conflict resolution explicitly referenced, not inferred from worker timing:**
RFC-0046 §7 explicitly forbids implicit "winner depends on timing" conflict resolution and
requires every scheduling domain to declare an explicit ordering rule before entering a
deterministic surface.  For DPE-governed execution that rule is supplied by RFC-DPE-0003 §3:
non-exclusive regions use last-writer-in-canonical-TaskId-order; exclusive regions abort at
commit if violated.  RFC-DPE-0005 §8 proves that level-parallel execution does not change
the EpochHash — canonical commit ordering is independent of thread scheduling and execution
order, with no timing-based tiebreaks possible.

**AC4 — Timeout/abort semantics documented as governed schedule effects:**
RFC-0046 §9 establishes timeouts and aborts as first-class schedule effects when on the
deterministic surface, and forbids partial canonical commit through an aborted scheduling unit.
RFC-DPE-0007 (Epoch Execution Timeout) implements this: timeout checking occurs after level
completion (not mid-task), producing `KernelEpochStatus::Aborted_Timeout` with no partial commit;
the `EpochHash` is never computed for a timed-out epoch.  RFC-DPE-0003 §6 defines abort atomicity
for all abort triggers (fault, mapping fault, exclusive conflict, policy fault).  RFC-DPE-0008
emits `EpochAborted` audit events for timeout aborts, keeping the governed audit surface complete.

**AC5 — Future multi-core/distributed work explicitly constrained to inherit this ordering contract:**
RFC-0046 §11 establishes the constitutional inheritance rule: future extensions may add
schedulable units and readiness rules but may not relax the requirement of one deterministic
observable history per governed computation.  RFC-0053 (Distributed Deterministic Execution
Protocol) explicitly cites RFC-0046 and directly inherits the ordering contract: §1 requires
a distributed execution to be a single canonical computation; §4 requires a canonical commit
order that all nodes can derive or verify; §5 forbids "first packet wins", wall-clock
precedence, and timing-based tiebreaks.  RFC-0053 §10 prohibits inheriting DCP status
merely from having a locally DCP-verified executor.
