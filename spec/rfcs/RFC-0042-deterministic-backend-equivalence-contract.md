# RFC-0042: Deterministic Backend Equivalence Contract

- **RFC-ID:** RFC-0042
- **Title:** Deterministic Backend Equivalence Contract
- **Status:** accepted
- **Type:** standards-track
- **Applies-To:** TISC, T81VM, Trace-JIT, SWAR, SIMD, future heterogeneous execution backends
- **Created:** 2026-03-19
- **Updated:** 2026-03-21
- **Supersedes:** None
- **Discussion:** Builds on RFC-0002, RFC-0040, RFC-0041, RFC-0049, and the Determinism Surface Registry

## Summary

This RFC defines the binding equivalence rules across execution backends in T81.

It establishes:

- the scalar path as the canonical semantic oracle
- the conditions under which SWAR, SIMD, and future backends may substitute for scalar execution
- the exact meaning of backend equivalence
- the failure policy when equivalence cannot be established

The purpose of this RFC is to prevent T81 from having multiple backend-specific truths.

## Motivation

T81 now exposes multiple execution realizations for the same ternary operations:

- scalar reference logic
- SWAR kernels
- SIMD kernels
- future JIT-lowered forms
- future heterogeneous accelerators

RFC-0040 and RFC-0041 formalize optimized execution paths, but they do not yet define the constitutional rule that all optimized paths are merely alternate realizations of a single deterministic computation.

Without this RFC:

- optimized backends remain performance features rather than governed execution surfaces
- backend-specific output drift can hide behind “implementation detail” language
- future JIT or GPU integration would have no normative equivalence boundary

## Proposal

### 1. Canonical Oracle

For every governed operation family, T81 MUST define a canonical oracle implementation.

For the current tritwise surface, the canonical oracle is:

- scalar trit semantics over canonical packed-trit values

For arithmetic surfaces (ADD, SUB, MUL, NEG, DIV, MOD and their type-specific
variants), the canonical oracle is defined by RFC-0049 (Canonical Ternary
Arithmetic Semantics).  The arithmetic oracle specifies:

- the canonical trit domain `{-1, 0, +1}`
- the carry propagation law (digit-wise balanced-ternary summation)
- the negation law (trit flip: Pos↔Neg, Zero unchanged)
- the subtraction identity: `a − b ≡ a + (−b)` (no independent borrow)
- the overflow/fault policy per surface (unbounded for BigInt, exception for `T81Int<N>`,
  special-value saturation for T81Float, explicit fault for division by zero)
- the comparison rule (value-based, not representation-based)

Backends claiming equivalence on arithmetic surfaces MUST satisfy the RFC-0049
arithmetic oracle, not merely an approximate or locally-defined scalar reference.

The canonical oracle is not required to be the fastest implementation. It is required to be the semantically authoritative implementation.

### 2. Definition of Backend Equivalence

Two backend implementations are backend-equivalent if, for identical canonical inputs, they produce all of the following identically:

1. final value bytes
2. fault class
3. fault timing at the observable operation boundary
4. trace-visible operation result
5. canonical serialization of any produced value

For VM-visible operations, equivalence additionally requires:

1. identical register-visible result
2. identical memory-visible result
3. identical Axion-visible audit meaning
4. identical CanonHash-relevant trace contribution

“Close enough” is forbidden. Equivalence is exact.

### 3. Backend Hierarchy

The backend hierarchy is:

1. scalar
2. SWAR
3. SIMD
4. JIT-lowered native form
5. future heterogeneous form

Higher tiers may replace lower tiers only if this RFC and the conformance RFC guarantee exact equivalence.

No backend may widen semantics. Backends may only change realization strategy.

### 4. Allowed Backend Substitutions

A backend substitution is allowed only when:

1. input encoding is canonical
2. output encoding is canonical
3. the backend is within the declared determinism surface
4. the backend passed the required equivalence corpus for the active architecture
5. the backend’s dispatch preconditions are deterministic

Examples of allowed substitutions:

- scalar `TNot` to SWAR `TNot`
- SWAR-dispatched tail handling inside SIMD kernels
- JIT lowering to the same tritwise backend semantics as interpreter dispatch

Examples of forbidden substitutions:

- backend-specific approximations
- host-intrinsic behavior that changes fault outcomes
- architecture-specific “fast math” style rewrites
- substitutions whose result depends on runtime timing, CPU model quirks, or undefined behavior

### 5. Observable Boundaries

Backend equivalence is evaluated at the deterministic observation boundary, not at internal micro-steps.

For library-level compute surfaces, the observation boundary is:

- returned `Result<T>`
- returned canonical bytes
- emitted deterministic error

For VM-visible execution, the observation boundary is:

- the instruction result
- trap outcome
- Axion-visible event meaning
- trace hash contribution

This allows implementation freedom internally while preserving exact external behavior.

### 6. Trace Identity Rules

Backend substitution MUST NOT alter the canonical meaning of the trace.

The following are allowed:

- backend-internal implementation details omitted from the trace
- backend-specific counters kept outside the DCP surface

The following are forbidden:

- backend-dependent user-visible reason strings
- backend-dependent opcode result meaning
- backend-dependent canon-hash input bytes

If the system records backend selection for diagnostics, that metadata MUST be outside the DCP trace boundary unless separately governed.

### 7. Dispatch Rules

Dispatch MUST be deterministic.

Dispatch decisions may depend on:

- canonical input size
- compile-time architecture support
- explicitly governed runtime capability flags
- policy-approved backend availability

Dispatch decisions may not depend on:

- elapsed time
- benchmark history
- speculative quality heuristics
- non-governed host environment features

### 8. Failure Policy

If a backend cannot satisfy the equivalence contract, the system MUST:

1. fall back to a lower verified backend, or
2. fail closed with a deterministic error if no verified backend is available

Silent semantic drift is forbidden.

The fallback order SHOULD prefer the nearest lower verified backend.

### 9. Cross-Architecture Invariant

Supported architectures MUST agree on:

- input canonicalization
- output canonicalization
- trap classes
- visible trace meaning

Architecture-specific code generation is allowed only if these invariants remain exact.

### 10. Scope of This RFC

This RFC governs backend equivalence for execution realization.

It does not, by itself, define:

- the full test matrix
- the CI gate protocol
- the packed trit vector ABI
- the full memory model

Those are delegated to companion RFCs.

## Acceptance Criteria

This RFC is ready for `accepted` when all of the following are true:

1. every governed tritwise backend family declares a canonical oracle
2. backend dispatch rules are documented for scalar, SWAR, and SIMD paths
3. trace-visible equivalence rules are explicitly wired into the determinism surface inventory
4. at least one executable conformance suite enforces scalar vs SWAR vs SIMD equivalence across supported architectures
5. JIT and future heterogeneous work are explicitly constrained to this contract rather than inventing parallel semantics

## Impact

### Backward Compatibility

This RFC does not require changing user-facing ternary semantics.

It constrains future implementation freedom by forbidding backend-specific observable behavior.

### Performance

No direct performance regression is required.

However, some optimizations may become disallowed if they cannot preserve exact equivalence.

### Security

This RFC reduces deterministic-surface drift by making backend substitution governable and testable.

It also narrows the risk of architecture-specific latent divergence.

## Alternatives Considered

### Treat optimized backends as implementation details

Rejected because the project already exposes backend-governed determinism claims.

### Allow value equivalence but not trace equivalence

Rejected because T81 determinism is trace-relevant, not merely result-relevant.

### Define equivalence only for current CPU paths

Rejected because JIT and heterogeneous acceleration would reopen the same governance hole later.

## References

- `spec/rfcs/RFC-0002-deterministic-execution-contract.md`
- `spec/rfcs/RFC-0040-swar-formalization.md`
- `spec/rfcs/RFC-0041-simd-formalization.md`
- `spec/rfcs/RFC-0049-canonical-ternary-arithmetic-semantics.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `tests/cpp/test_tritwise_backend_equivalence.cpp`
- `tests/cpp/test_arithmetic_backend_equivalence.cpp`
- `docs/developer-guide/internals/jit-equivalence-plan.md`

## Implementation Record (2026-03-21)

All acceptance criteria are satisfied as of this date.

**AC1 — Canonical oracle declared for every governed backend family:**
`spec/tisc-spec.md §5.2.3` ("Backend Equivalence Contract (RFC-0042)") defines the
canonical oracle table.  For tritwise operations the oracle is the scalar trit-by-trit
reference implementation.  For arithmetic operations (ADD, SUB, MUL, NEG, DIV, MOD)
the oracle is the RFC-0049 arithmetic specification (`t81::ternary::arith.hpp`).
No governed backend family is without an explicitly declared oracle.

**AC2 — Backend dispatch rules documented for scalar, SWAR, and SIMD:**
`spec/tisc-spec.md §5.2.3` contains the normative dispatch rules table with explicit
size thresholds (`AVX2_THRESHOLD_BYTES = 64`, `NEON_TOR_THRESHOLD_BYTES = 64`),
tail handling strategy (SWAR tail for SIMD kernels), and disabled-path notation
(NEON TNot/TAnd set to `SIZE_MAX` → always falls to SWAR).

**AC3 — Trace-visible equivalence wired into the determinism surface inventory:**
`docs/governance/DETERMINISM_SURFACE_REGISTRY.md §5` was updated with two explicit
backend equivalence rows:

- "Tritwise Backend Equivalence (RFC-0042)": `test_tritwise_backend_equivalence.cpp`
  covering scalar vs SWAR vs AVX2/NEON for sizes 1–4097.
- "Arithmetic Backend Equivalence (RFC-0042 + RFC-0049)":
  `test_arithmetic_backend_equivalence.cpp` covering scalar trit oracle vs T81BigInt
  for ADD, SUB, MUL, NEG, comparison, carry propagation, and overflow policy.

`docs/governance/DETERMINISM_SURFACE_REGISTRY.md §3.1` marks RFC-0042 as `accepted`.

**AC4 — Executable conformance suite enforcing scalar vs SWAR vs SIMD equivalence:**
Two CI-enforced test targets enforce equivalence across architectures:

- `t81_tritwise_backend_equivalence_test` (`tests/cpp/test_tritwise_backend_equivalence.cpp`)
  — verifies scalar ↔ SWAR ↔ AVX2 ↔ NEON for NOT, AND, OR across 14 sizes on each
  supported architecture.
- `t81_arithmetic_backend_equivalence_test` (`tests/cpp/test_arithmetic_backend_equivalence.cpp`)
  — verifies scalar trit oracle ↔ T81BigInt for all arithmetic ops including algebraic
  laws, overflow policy, and 3^k carry boundaries.

Both targets are registered in `T81_TEST_TARGETS` in `CMakeLists.txt` and run in `ci.yml`.

**AC5 — JIT and future heterogeneous work explicitly constrained to this contract:**
`docs/developer-guide/internals/jit-equivalence-plan.md` was updated with a governance
note establishing the JIT as tier 4 in the RFC-0042 backend hierarchy.  The note
enumerates all five equivalence conditions the JIT must satisfy (register-visible result,
memory-visible result, fault class and timing, Axion-visible audit meaning, and
CanonHash-relevant trace contribution) and specifies that failure to satisfy any
condition MUST trigger interpreter fallback or a deterministic fault — never silent drift.

**Promotion framework:**
This implementation record constitutes the first promotion path that cites RFC-0043
(Deterministic Conformance Validation Framework) as the governing proof model, satisfying
RFC-0043 AC5.  Backend equivalence for this RFC is proven via RFC-0043 conformance layers
1 (semantic) and 2 (backend equivalence).  The cross-platform replay artifacts for
x86_64 + AArch64 are defined in `docs/governance/DETERMINISM_SURFACE_REGISTRY.md §5.2`.
Breach classification follows RFC-0043 §6 as codified in `docs/governance/FREEZE_ENFORCEMENT.md §4`.
