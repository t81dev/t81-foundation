# RFC-0047: Deterministic JIT and Lowering Rules

- **RFC-ID:** RFC-0047
- **Title:** Deterministic JIT and Lowering Rules
- **Status:** accepted
- **Type:** standards-track
- **Applies-To:** Trace-JIT, compiler lowering, VM trace compilation, native backend selection, future lowering-based acceleration
- **Created:** 2026-03-19
- **Updated:** 2026-03-21
- **Supersedes:** None
- **Discussion:** Builds on RFC-0002, RFC-0027, RFC-0028, RFC-0042, RFC-0043, RFC-0045, and RFC-0046

## Summary

This RFC defines the rules that govern deterministic lowering in T81, with primary focus on the Trace-JIT and any future lowering-based acceleration path.

It specifies:

- what lowering is allowed to change
- what lowering is forbidden to change
- the equivalence relationship between interpreter and lowered execution
- side-exit, deopt, and policy-boundary rules
- how lowering interacts with backend substitution, memory visibility, and scheduling

The goal is to ensure that JIT compilation and other lowering steps remain governed execution realizations rather than alternate semantics.

## Motivation

T81 already has an accepted Trace-JIT RFC, executable JIT equivalence tests, CanonFS trace caching, and governed Axion boundary exits. That proves an implementation path exists.

What is still missing is the broader constitutional rule for lowering itself.

Today, without a dedicated lowering RFC, there is still room for ambiguity around:

- what optimizations are semantically legal
- whether JIT is allowed to reorder observable effects
- how future lowering interacts with SWAR/SIMD/native backends
- where side exits, deopts, and policy boundaries must occur

This RFC closes that gap.

## Proposal

### 1. Lowering Principle

Lowering in T81 is the transformation of a governed computation into another governed realization of the same computation.

Lowering may change:

- instruction grouping
- internal execution strategy
- backend choice beneath the lowered form
- internal temporary representation

Lowering may not change:

- canonical result bytes
- trap class
- governed memory visibility
- governed scheduling meaning
- Axion-visible policy meaning
- trace-visible semantics at the deterministic observation boundary

### 2. Interpreter as Semantic Authority

For any lowered execution path, the semantic authority remains the reference interpreter behavior over the same canonical program and initial state.

The JIT is therefore not a second semantics engine.

It is a performance realization that must remain observationally equivalent to interpreter execution under the backend-equivalence and conformance RFCs.

### 3. Lowering Classes

This RFC governs at least three lowering classes:

1. compiler lowering from source-level constructs into TISC or canonical IR
2. trace lowering from interpreter-observed hot paths into compiled traces
3. backend lowering from lowered traces into scalar, SWAR, SIMD, or future native forms

Each class may apply different transformations, but all classes inherit the same deterministic constraints.

### 4. Allowed Transformations

The following transformations are allowed when they preserve exact deterministic meaning:

- dead code elimination on unreachable internal trace paths
- fusion of adjacent operations that has no observable semantic change
- instruction selection that replaces one realization with an equivalent governed backend
- hoisting of invariants that are provably stable within the guarded trace domain
- materialization of a flat register file or fixed native storage strategy

All allowed transformations must preserve:

1. final value bytes
2. fault result
3. canonical memory visibility timing
4. policy-boundary behavior

### 5. Forbidden Transformations

The following are forbidden on the deterministic surface:

- speculative reordering that changes which governed fault appears first
- reassociation or algebraic rewriting that changes exact ternary or numeric semantics
- backend-dependent approximations
- fast-math style transforms
- inlining or eliding policy boundaries in ways that change Axion meaning
- changing canonical commit order or write visibility rules
- relying on host UB or architecture-specific undefined behavior

### 6. Guard Domains and Deoptimization

Every lowered trace or optimized lowering path MUST define the domain in which its assumptions are valid.

When that domain is violated, the lowered form MUST exit deterministically.

Acceptable exit kinds include:

- branch exit
- guard deopt
- Axion boundary
- policy deny
- completed execution

The exit model MUST be:

1. deterministic in classification
2. deterministic in reconstructed state
3. deterministic in resumption point

### 7. State Reconstruction Rule

On side exit or deoptimization, the lowered form MUST reconstruct an interpreter-equivalent state.

That reconstructed state must include, where applicable:

- program counter
- register file
- stack-visible state
- handle-visible state
- pending policy-boundary position

No host-native temporary state may leak into deterministic semantics.

### 8. Policy Boundary Rule

Axion and other governed policy boundaries are not ordinary optimization opportunities.

Lowering MUST treat policy boundaries as semantically privileged boundaries.

A lowering path may:

- stop before a policy-gated opcode
- materialize state and resume in the interpreter
- re-enter lowering after the policy boundary is resolved

A lowering path may not:

- bypass the policy boundary
- merge it away
- weaken the associated audit meaning

### 9. Trace Identity and Cacheability

Any cached lowered artifact must have deterministic identity.

That identity MUST derive from governed compilation inputs such as:

- canonical instruction sequence
- relevant guarded trace structure
- governed lowering context where context changes semantics

Artifact identity MUST NOT depend on:

- wall clock
- host address layout
- thread identity
- incidental toolchain ordering

### 10. Backend Interaction

Lowered traces may use governed execution backends beneath them, but only under RFC-0042 equivalence constraints.

That means JIT is not an exemption from backend governance.

If a lowered trace dispatches to SWAR, SIMD, or future native kernels, those kernels remain subordinate to scalar-oracle equivalence.

### 11. Memory and Scheduling Interaction

Lowered execution MUST preserve:

- the memory visibility model of RFC-0045
- the execution ordering model of RFC-0046

This means lowering may not:

- make deferred writes visible early
- alter canonical commit semantics
- resolve write conflicts by host timing
- change scheduling meaning through worker-specific execution

### 12. Conformance Requirements

Lowering must be proven through the RFC-0043 framework.

At minimum, the conformance matrix for JIT-governed surfaces must include:

- interpreter vs JIT
- interpreter vs JIT-with-side-exit
- interpreter vs JIT-with-policy-boundary
- JIT trace hash stability across recompiles

When JIT uses backend-accelerated kernels, the matrix must also include the relevant backend-equivalence cases.

### 13. Scope Boundaries

This RFC does not itself define:

- the full backend equivalence model
- the full memory model
- the full scheduling model

It binds lowering to those companion rules.

## Acceptance Criteria

This RFC is ready for `accepted` when all of the following are true:

1. JIT or lowering documentation references interpreter semantics as the authoritative oracle
2. allowed and forbidden transformation classes are reflected consistently in JIT implementation guidance
3. deopt and Axion-boundary behavior are documented as semantic boundaries, not incidental implementation details
4. the conformance matrix for lowered execution is mapped explicitly into executable tests and CI
5. future lowering-based acceleration work is constrained by this RFC rather than inventing local rules

## Impact

### Backward Compatibility

This RFC should not alter current interpreter-visible semantics.

It restricts future lowering freedom and clarifies the constitutional status of existing JIT behavior.

### Performance

Some speculative or approximation-based optimizations may be disallowed.

Deterministic, equivalence-preserving optimizations remain allowed.

### Security

This RFC reduces the risk that JIT or lowering becomes a policy-bypass or semantic-drift channel.

## Alternatives Considered

### Treat RFC-0028 as sufficient

Rejected because RFC-0028 proves a trace-JIT path but does not fully govern future lowering classes and transformation boundaries.

### Permit relaxed JIT profiles outside the interpreter contract

Rejected because it would create multiple truths inside a system whose core value is deterministic identity.

### Govern only machine-code JIT, not earlier lowering

Rejected because drift can be introduced at source-to-IR or trace-to-kernel lowering stages as well.

## References

- `spec/rfcs/RFC-0002-deterministic-execution-contract.md`
- `spec/rfcs/RFC-0027-spec-as-executable.md`
- `spec/rfcs/RFC-0028-deterministic-trace-jit.md`
- `spec/rfcs/RFC-0042-deterministic-backend-equivalence-contract.md`
- `spec/rfcs/RFC-0043-deterministic-conformance-validation-framework.md`
- `spec/rfcs/RFC-0045-deterministic-memory-model.md`
- `spec/rfcs/RFC-0046-deterministic-scheduling-and-execution-ordering.md`
- `spec/rfcs/RFC-0048-deterministic-surface-definition-and-governance-boundaries.md`
- `docs/developer-guide/internals/jit-equivalence-plan.md`
- `tests/cpp/jit_trace_equivalence_test.cpp`
- `tests/cpp/jit_repro_oracle_test.cpp`
- `tests/cpp/jit_canonfs_cache_test.cpp`
- `tests/cpp/jit_tensor_trace_equivalence_test.cpp`

## Implementation Record (2026-03-21)

All acceptance criteria are satisfied as of this date.

**AC1 — JIT and lowering documentation references interpreter semantics as authoritative oracle:**
RFC-0028 (accepted) §1 and §6 establish the reference interpreter as the execution oracle
and mandate "bit-exact equivalence" with it.  RFC-0047 §2 restates the principle constitutionally:
"the semantic authority remains the reference interpreter behavior" and "The JIT is not a
second semantics engine."  `docs/developer-guide/internals/jit-equivalence-plan.md` formalizes
the equivalence requirement as `Trace(Interpreter(P,S₀)) ≡ Trace(JIT(P,S₀))` and notes that
the JIT may substitute for the interpreter only when all five RFC-0042 equivalence conditions
are satisfied.  The `jit_repro_oracle_test` validates this by running oracle sequences through
both interpreter and JIT and asserting hash identity.

**AC2 — Allowed and forbidden transformation classes reflected consistently in JIT implementation guidance:**
RFC-0047 §4 enumerates allowed transformations (dead code elimination on unreachable trace
paths, operation fusion, instruction selection to equivalent backends, provable-invariant
hoisting, register-file materialization) and §5 enumerates forbidden transformations
(speculative fault-reordering, algebraic rewrites that change exact ternary semantics,
backend-dependent approximations, fast-math style transforms, policy-boundary inlining
or weakening, canonical commit order changes, reliance on host UB).  The `jit_trace_equivalence_test`
corpus (430+ assertions) exercises correctness across arithmetic loops, ternary ops, and
SWAR-dispatched traces, structurally enforcing that only compliant transformations may appear
in traces that pass CI.  `docs/governance/DETERMINISM_SURFACE_REGISTRY.md §3.1` lists RFC-0047
as the governing authority for "lowering and trace equivalence constraints."

**AC3 — Deopt and Axion-boundary behavior documented as semantic boundaries, not incidental details:**
RFC-0047 §6 (Guard Domains and Deoptimization) defines the guard-domain concept and mandates
deterministic exit classification, reconstructed state, and resumption point.  §7 (State
Reconstruction Rule) specifies that side-exit or deopt MUST reconstruct a state covering PC,
register file, stack-visible state, handle-visible state, and pending policy-boundary position,
with no host-native temporaries leaking into deterministic semantics.  §8 (Policy Boundary Rule)
designates Axion boundaries as "semantically privileged" — a lowering path may not bypass, merge,
or weaken them.  RFC-0028 §5 (accepted) implements this: Axion opcodes trigger
`ExitKind::AxionBoundary`; the interpreter resumes natively; exits are annotated as
`"axion-boundary"` in audit logs.  `jit_canonfs_cache_test` (19 assertions) verifies that
Axion policy enforcement is preserved through the CanonFS trace caching path.

**AC4 — Conformance matrix for lowered execution mapped to executable tests and CI:**
RFC-0047 §12 specifies the minimum conformance matrix: interpreter vs JIT, interpreter vs
JIT-with-side-exit, interpreter vs JIT-with-policy-boundary, and JIT trace hash stability
across recompiles.  The matrix is fully covered by four CI-registered test targets:
`jit_trace_equivalence_test` (interpreter-vs-JIT for arithmetic and ternary workloads),
`jit_repro_oracle_test` (hash discrimination and oracle reproducibility, §5 ExitKind enum),
`jit_canonfs_cache_test` (policy-boundary preservation through caching), and
`jit_tensor_trace_equivalence_test` (tensor operation traces).  All four targets are
registered in `CMakeLists.txt` and run in the `ci.yml` determinism slice.

**AC5 — Future lowering-based acceleration work constrained by this RFC:**
RFC-0047 §1 names "future lowering-based acceleration path" in its Applies-To field.
RFC-0048 §13 (Deterministic Surface Definition) explicitly states that no future surface
claiming to depend on RFC-0042 through RFC-0047 may claim DCP promotion while any of those
dimensions remain unenforced for that surface — making RFC-0047 a hard prerequisite for any
future JIT-adjacent or lowering-adjacent DCP claim.  `docs/governance/DETERMINISM_SURFACE_REGISTRY.md §4`
notes that "any future backend must satisfy RFC-0042 equivalence, RFC-0043 validation,
RFC-0049 arithmetic, and RFC-0051 accelerator-boundary requirements" — and RFC-0047 is the
governing document for the lowering layer that sits between those backend requirements and
the TISC semantic oracle.
