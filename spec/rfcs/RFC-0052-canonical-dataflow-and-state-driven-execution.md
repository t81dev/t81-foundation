# RFC-0052: Canonical Dataflow and State-Driven Execution

- **RFC-ID:** RFC-0052
- **Title:** Canonical Dataflow and State-Driven Execution
- **Status:** draft
- **Type:** standards-track
- **Applies-To:** DPE, CanonFS-linked execution, task/state propagation, dependency graphs, VM/runtime orchestration
- **Created:** 2026-03-19
- **Updated:** 2026-03-19
- **Supersedes:** None
- **Superseded-By:** None
- **Discussion:** Builds on RFC-0002, RFC-0025, RFC-DPE-0001 through RFC-DPE-0009, RFC-0045, RFC-0046, and RFC-0048

---

## Summary

This RFC defines T81’s canonical dataflow execution model: how state transitions trigger computation, how dependency graphs are represented and ordered, how CanonFS-backed state participates in execution, and how propagation remains deterministic across local and future distributed runtimes.

## Motivation

T81 already has the ingredients of a dataflow architecture:

- deterministic VM execution
- DPE task/epoch execution
- CanonFS-backed artifact and state surfaces
- policy-gated loading and execution boundaries

What is missing is the explicit contract that says how state and computation compose. Without that contract:

- task graphs remain an execution mechanism rather than an architectural model
- CanonFS-linked execution remains ad hoc
- future reactive or service-style behavior can drift into implicit event systems
- local and distributed execution will lack a shared propagation rule

This RFC turns existing execution machinery into an explicit canonical state-driven model.

## Proposal

### 1. State-Driven Computation

In T81, computation may be triggered by:

- explicit program invocation
- canonical task-graph submission
- governed state transition on a registered dataflow surface
- CanonFS-backed state/materialization events where explicitly allowed

No implicit or hidden reactive behavior is permitted outside registered dataflow surfaces.

### 2. Canonical Dependency Graph

Dataflow execution is defined over a canonical dependency graph with:

- deterministic node identity
- deterministic edge identity
- explicit dependency types
- canonical ordering for ready-node selection

Graph semantics must not depend on container iteration order, host hash order, or scheduler timing.

### 3. Node Semantics

Each node in the dataflow graph must define:

- its input state dependencies
- its output state transitions
- its fault behavior
- whether it is pure, effect-bounded, or policy-mediated

Node execution must be reproducible from canonical input state plus canonical graph metadata.

### 4. CanonFS Participation

CanonFS may participate in dataflow only through explicit governed surfaces.

Allowed examples:

- loading canonical artifacts as inputs
- materializing outputs to canonical storage
- policy-gated dependency resolution via canonical identifiers

Forbidden:

- implicit filesystem watch semantics
- host-local path ordering affecting graph semantics
- opportunistic external mutation bypassing canonical state rules

### 5. Ready-State and Activation Semantics

A node becomes ready only when:

- all required dependencies are satisfied, and
- all required policy gates are satisfied, and
- its activation condition evaluates true under canonical state

Ready-node selection must obey RFC-0046 ordering constraints.

### 6. Propagation Semantics

When a node commits output state:

- downstream readiness is recomputed deterministically
- propagation order is canonical
- faulted outputs do not silently activate downstream nodes
- retry or requeue behavior must be explicit and deterministic

### 7. Interaction with DPE

RFC-DPE defines concrete deterministic parallel execution mechanics.

RFC-0052 defines the architectural model above them:

- DPE epochs may serve as one realization of dataflow scheduling
- dataflow semantics do not replace DPE; they organize when and why DPE runs work
- any DPE optimization remains subordinate to canonical dependency and commit semantics

### 8. State Identity and Versioning

Dataflow execution must define:

- canonical state identity
- canonical version or epoch relation
- explicit stale-state detection rules
- explicit re-materialization/recomputation rules

State identity may not depend on host pointer identity or incidental runtime addresses.

### 9. Fault Propagation

Faults in dataflow execution must define:

- whether they block downstream activation
- whether fallback state exists
- whether retry is permitted
- whether the graph enters a terminal or recoverable condition

Fault propagation must be deterministic and auditable.

### 10. Observability

Dataflow execution must be observable through:

- stable node/edge identity
- activation and commit records
- fault and retry records
- canonical summary state

This observability must remain semantic rather than scheduler-specific.

### 11. DCP Boundary Rule

Dataflow execution as an architectural model may exist before it is DCP-verified.

Rules:

- explicit dataflow surfaces are governed non-DCP by default unless registry-promoted
- any DCP claim for a dataflow surface requires conformance, ordering, memory, and propagation proof
- local service orchestration and future distributed propagation must not inherit DCP claims automatically

## Determinism / Safety Considerations

Determinism considerations:

- dependency graph identity and ready-node ordering are the key invariants
- state versioning and propagation order must not depend on incidental runtime behavior
- CanonFS participation must remain canonical and explicit

Safety considerations:

- policy-mediated nodes must not activate outside explicit governance checks
- fault propagation must be deterministic and fail-closed
- implicit reactivity is forbidden because it obscures the execution contract

## Compatibility

This RFC is additive and architectural.

Compatibility rules:

- existing explicit invocation and DPE execution remain valid
- current implementations may remain partial while adopting the canonical model incrementally
- no existing deterministic execution guarantee is weakened by defining dataflow semantics

## Implementation Plan

1. Define canonical node/edge/state identity structures for dataflow-enabled surfaces.
2. Map existing DPE epoch/task machinery to the dataflow model.
3. Add explicit readiness, propagation, and fault semantics for governed service/runtime flows.
4. Add trace and audit surfaces for node activation/commit/fault transitions.
5. Bind any CanonFS-triggered execution to explicit policy-gated registration.

## Open Questions

- Which current runtime surfaces should be the first officially registered dataflow surfaces?
- Should CanonFS-triggered execution remain narrow and explicit, or can broader artifact-triggered workflows be allowed later?
- How much of service orchestration in TernaryOS should be folded into this model versus kept in OS-specific RFCs?

## Acceptance Criteria

- The architecture defines canonical dependency graph, state identity, readiness, and propagation semantics.
- DPE is explicitly positioned as an execution realization beneath the dataflow model.
- CanonFS-linked execution is constrained to explicit governed surfaces.
- Fault and retry propagation are deterministic and auditable.
- DCP claims for dataflow surfaces are explicitly gated through RFC-0043, RFC-0045, RFC-0046, and RFC-0048.
