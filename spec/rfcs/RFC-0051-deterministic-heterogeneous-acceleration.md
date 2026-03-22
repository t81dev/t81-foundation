# RFC-0051: Deterministic Heterogeneous Acceleration

- **RFC-ID:** RFC-0051
- **Title:** Deterministic Heterogeneous Acceleration
- **Status:** accepted
- **Type:** standards-track
- **Applies-To:** accelerator backends, GPU/Metal/CUDA/SYCL lowering, backend equivalence, memory and scheduling governance, policy/trace boundaries
- **Created:** 2026-03-19
- **Updated:** 2026-03-22
- **Supersedes:** None
- **Superseded-By:** None
- **Discussion:** Builds on RFC-0002, RFC-0027, RFC-0042, RFC-0043, RFC-0045, RFC-0046, RFC-0047, RFC-0048, RFC-0049, and RFC-0050

---

## Summary

This RFC defines how T81 may use heterogeneous acceleration, including GPU and other non-CPU execution targets, without violating deterministic-surface guarantees. It constrains the classes of kernels, memory transfer rules, scheduling behavior, reduction semantics, fallback rules, and promotion requirements for accelerator-backed execution.

## Motivation

Acceleration pressure will eventually push T81 beyond scalar, SWAR, and SIMD CPU execution. Without a governance contract, GPU or heterogeneous support would create a large nondeterminism aperture:

- vendor-dependent math behavior
- unordered reduction behavior
- nondeterministic scheduling and kernel launch ordering
- hidden memory movement and stale-state divergence
- backend-specific "close enough" results passing as equivalent

T81 can support heterogeneous execution only if the architecture treats it as a governed backend class rather than a performance-only feature.

## Proposal

### 1. Accelerator Backends Are Governed Execution Backends

Any heterogeneous accelerator path is a governed execution backend under RFC-0042.

It is not automatically:

- DCP eligible
- semantically trusted
- interchangeable with CPU backends

It must earn equivalence and promotion explicitly.

### 2. Allowed Accelerator Classes

This RFC allows future accelerator backends only for workloads that can define:

- canonical input serialization
- canonical kernel boundaries
- canonical output interpretation
- explicit fault/fallback rules
- proof obligations under RFC-0043

Example classes that may become eligible:

- lane-local tritwise transforms
- canonical packed-trit arithmetic kernels
- width-bounded tensor primitives
- backend-stable reductions with explicit order model

Example classes that are not eligible by default:

- vendor-tuned floating-point heuristics
- unordered atomic accumulation
- nondeterministic work-stealing kernels
- sampling or hardware-rng-driven execution

### 3. Canonical Memory Transfer Rules

All host↔accelerator movement must be deterministic and explicitly modeled.

Requirements:

- canonical byte/trit serialization before transfer
- explicit transfer boundaries visible to the governed execution model
- no hidden mutation outside RFC-0045 visibility rules
- deterministic initialization of device-visible memory
- deterministic handling of padding, alignment, and unused bits

### 4. Kernel Launch and Scheduling Semantics

Accelerator launches must obey RFC-0046 ordering rules.

This means:

- kernel launch order is semantically defined where externally relevant
- completion order must not alter canonical results
- queueing, batching, or stream selection may not change governed semantics
- fallback from asynchronous to synchronous execution must preserve trace and fault identity

### 5. Reduction and Cross-Workgroup Constraints

Reductions are the highest-risk accelerator surface.

Allowed only if:

- the reduction order is explicitly defined, or
- regrouping is proven equivalent under RFC-0049 arithmetic semantics and RFC-0046 ordering semantics

Forbidden:

- backend-local unordered reduction accepted as "close enough"
- race-resolved accumulation whose result depends on scheduler timing

### 6. Trace and Policy Semantics

Accelerator execution must remain semantically observable at the T81 level.

Requirements:

- trace hashing must remain backend-invariant for verified surfaces
- policy hooks must reason about semantic operations, not CUDA/Metal/SYCL implementation details
- fallback from accelerator to CPU must not silently change the semantic trace class

### 7. Fault, Fallback, and Availability Rules

Accelerator surfaces must define:

- unsupported-device behavior
- unavailable-driver behavior
- kernel-compilation failure behavior
- timeout or synchronization failure behavior

Allowed responses:

- deterministic fallback to a permitted CPU backend
- deterministic hard fault

Forbidden:

- silent backend substitution that changes governed semantics
- best-effort execution with downgraded correctness

### 8. DCP Boundary Rule

Accelerator backends are governed non-DCP by default.

They may only become Verified / DCP eligible if:

- backend equivalence is proven against the scalar oracle
- memory movement is audited against RFC-0045
- scheduling is audited against RFC-0046
- conformance evidence exists across supported vendors/architectures per RFC-0043
- public boundary docs are updated under RFC-0048

### 9. Vendor-Neutral Semantic Layer

The T81 architecture must define accelerator semantics above any vendor API.

Vendor APIs such as:

- CUDA
- Metal
- HIP
- SYCL
- Vulkan compute

are implementation mechanisms only. No public deterministic claim may depend on vendor-specific semantic wording.

### 10. Relation to Vector and JIT RFCs

- RFC-0050 defines vector semantics at the ISA/VM level.
- RFC-0047 defines how lowering may target alternate backends.
- RFC-0051 defines the governance boundary when that alternate backend is heterogeneous hardware rather than CPU-local execution.

## Determinism / Safety Considerations

Determinism considerations:

- heterogeneous acceleration is a major divergence amplifier if unconstrained
- reduction order and memory transfer boundaries are the main breach points
- vendor-specific math and launch semantics must never leak into DCP claims

Safety considerations:

- accelerator failure must be fail-closed or deterministically fallback-safe
- policy surfaces must not be bypassed by offloaded execution
- unsupported devices must not produce partial or best-effort results under governed claims

## Compatibility

This RFC is additive. It does not require introducing accelerator backends now.

Compatibility rules:

- current CPU-only execution remains valid
- any future accelerator path begins outside DCP
- no existing CPU semantic surface may be weakened to accommodate accelerator quirks

## Implementation Plan

1. Define accelerator backend registration and classification as governed non-DCP by default.
2. Add explicit transfer, fallback, and trace semantics for any first accelerator prototype.
3. Build RFC-0043 conformance matrices that compare scalar CPU against accelerator results.
4. Add vendor/driver support policy and exclusion rules.
5. Promote only narrowly scoped kernels after proof and governance review.

## Open Questions

- Which accelerator class should be the first prototype target: tritwise ops, packed arithmetic, or tensor kernels?
- Should vendor qualification be per kernel family or per backend runtime?
- What minimum reproducibility matrix is required before any accelerator path can leave governed non-DCP?

## Acceptance Criteria

- Accelerator execution is explicitly classified as governed non-DCP by default.
- Kernel classes, transfer semantics, fallback behavior, and reduction constraints are normatively defined.
- RFC-0042, RFC-0043, RFC-0045, RFC-0046, and RFC-0048 are cross-referenced as binding constraints.
- No accelerator path may be promoted without explicit conformance and boundary updates.

## Implementation Record (2026-03-22)

All acceptance criteria are satisfied as of this date.

**AC1 — Accelerator execution explicitly classified as governed non-DCP by default:**
`spec/tisc-spec.md §5.2.5` ("Heterogeneous Acceleration Governance (RFC-0051)") is a
normative section stating: "Any heterogeneous accelerator execution path … is classified
as governed non-DCP by default" and "No accelerator backend is automatically DCP-eligible,
semantically trusted, or interchangeable with verified CPU backends."
`docs/governance/DETERMINISM_SURFACE_REGISTRY.md §4` ("Experimental / Planned") was
updated to add an explicit "External Hardware Accelerators (governed non-DCP)" entry
citing RFC-0051 §1 and `spec/tisc-spec.md §5.2.5`.

**AC2 — Kernel classes, transfer semantics, fallback behavior, and reduction constraints normatively defined:**
`spec/tisc-spec.md §5.2.5` contains four normative subsections covering each topic:
"Allowed Accelerator Kernel Classes" enumerates eligible and ineligible classes with
rationale; "Memory Transfer Rules" specifies six required canonical-serialization and
visibility conditions; "Kernel Launch and Scheduling Semantics" states the RFC-0046
ordering requirement for launch order, completion order, and async/sync fallback parity;
"Reduction and Cross-Workgroup Constraints" defines the two allowed paths (explicit order
or proven-equivalent regrouping) and forbids unordered accumulation.  The "Fault, Fallback,
and Availability" subsection defines required behavior for all four failure modes.

**AC3 — RFC-0042, RFC-0043, RFC-0045, RFC-0046, and RFC-0048 cross-referenced as binding constraints:**
`spec/tisc-spec.md §5.2.5` "Binding Cross-References" subsection explicitly lists
RFC-0042, RFC-0043, RFC-0045, RFC-0046, and RFC-0048 as binding constraints and states:
"An accelerator backend may not relax any rule established by those RFCs."  Each subsection
also cross-references its governing companion RFC inline (RFC-0045 for memory transfer,
RFC-0046 for scheduling, RFC-0049 for arithmetic oracle in reductions).

**AC4 — No accelerator path may be promoted without explicit conformance and boundary updates:**
`spec/tisc-spec.md §5.2.5` "DCP Promotion Gate" enumerates five mandatory conditions
(scalar-oracle equivalence via RFC-0042, memory audit via RFC-0045, scheduling audit
via RFC-0046, cross-vendor conformance evidence via RFC-0043, and public boundary docs
via RFC-0048) and states: "Until all five conditions are met, the surface MUST remain
governed non-DCP."  The registry entry reinforces this: the governed non-DCP classification
is permanent until an explicit promotion path satisfies all five gates.
