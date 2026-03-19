# RFC-0050: Vectorized Ternary Operations for TISC

- **RFC-ID:** RFC-0050
- **Title:** Vectorized Ternary Operations for TISC
- **Status:** draft
- **Type:** standards-track
- **Applies-To:** `spec/tisc-spec.md`, `spec/t81vm-spec.md`, vector/tritwise opcode surfaces, SWAR/SIMD backend integration
- **Created:** 2026-03-19
- **Updated:** 2026-03-19
- **Supersedes:** None
- **Superseded-By:** None
- **Discussion:** Builds on RFC-0002, RFC-0005, RFC-0040, RFC-0041, RFC-0042, RFC-0044, RFC-0047, and RFC-0049

---

## Summary

This RFC defines how vectorized ternary operations become first-class TISC and VM concepts rather than remaining implicit implementation details. It specifies the semantic opcode surface, execution model, trace behavior, backend mapping rules, and compatibility constraints for vector-width ternary operations so that SWAR, SIMD, and future vector backends can be exposed without fragmenting the ISA contract.

## Motivation

T81 already contains vector-like execution in practice:

- SWAR tritwise paths
- SIMD tritwise paths
- native packed-trit arithmetic helpers

But most of that power is still expressed as backend machinery or library surfaces rather than as a clear ISA/VM contract. That creates three risks:

- the VM and ISA cannot distinguish semantic vector intent from backend optimization
- trace and policy surfaces do not have a stable rule for vector operations
- future JIT or heterogeneous lowering may invent vector behavior ad hoc

This RFC makes vectorized ternary execution explicit at the semantic layer without coupling the ISA to one physical backend.

## Proposal

### 1. Semantic, Not Hardware, Vectorization

Vector operations in TISC are semantic operations over fixed-width or declared-width ternary lanes.

They are not:

- AVX-specific instructions
- NEON-specific instructions
- backend-specific intrinsics

The ISA contract names vector intent and vector semantics. Backend selection remains an implementation detail governed by RFC-0042.

### 2. Canonical Lane Model

A vectorized ternary operation is defined over:

- a canonical lane count or declared width
- a canonical lane ordering
- a canonical element interpretation
- a canonical result shape

Lane ordering must be architecture-independent. No backend may reinterpret lane order based on endianness, register layout, or packed physical arrangement.

### 3. Opcode Surface

This RFC introduces the rule that vector operations may exist as first-class TISC opcodes where their semantics are materially different from scalar instruction repetition or where explicit vector intent is required for governance, traceability, or optimization boundaries.

Vector opcode naming must be semantic:

- `TVNOT`
- `TVAND`
- `TVOR`
- `TVXOR`
- `TVADD`
- `TVSUB`
- `TVMUL`

Names are illustrative. Final mnemonic selection must remain consistent with existing opcode naming conventions.

### 4. Explicit vs Implicit Vectorization

Two forms are permitted:

- **Explicit vector opcodes**: the program requests vector semantics directly
- **Implicit backend vectorization**: the implementation lowers scalar-equivalent operations into SWAR/SIMD internally under RFC-0042 and RFC-0047

Rule:

- if vector width, trace meaning, policy behavior, or fault behavior is externally relevant, the vector operation must be explicit
- if vectorization is only an internal optimization with no externally governed semantic effect, it may remain implicit

### 5. Width and Shape Semantics

Each vector opcode family must define:

- supported widths
- width declaration encoding
- width mismatch behavior
- tail-lane or partial-width behavior
- interaction with packed-trit representations

Forbidden:

- backend-specific inferred width with no canonical program-visible meaning
- silent widening or narrowing that changes semantics

### 6. Fault and Validation Behavior

Vector operations must define fail-closed behavior for:

- invalid width declaration
- incompatible operand shapes
- out-of-range lane metadata
- unsupported explicit vector width on a given runtime configuration

Allowed runtime behavior:

- deterministic fallback to scalar or narrower governed backend if the RFC and opcode contract permit it
- deterministic fault if no permitted fallback exists

Fallback behavior must be explicit and trace-stable.

### 7. Trace and Policy Semantics

Vectorized ternary execution must be visible to trace and policy surfaces in a stable way.

Requirements:

- trace hashing must not depend on the physical backend used beneath the vector semantic opcode
- vector operations must produce stable semantic trace categories
- Axion/policy hooks must reason about semantic vector operations, not AVX/NEON details

If a vector opcode falls back to scalar execution, the semantic trace remains the vector opcode event unless explicitly specified otherwise.

### 8. Relation to SWAR and SIMD

RFC-0040 and RFC-0041 define stable implementation-facing SWAR and SIMD surfaces.

RFC-0050 defines how those surfaces relate to TISC:

- SWAR/SIMD may implement explicit vector opcodes
- SWAR/SIMD may also remain internal execution backends for scalar or vector semantics
- explicit vector opcodes must not expose SWAR/SIMD-specific artifacts

### 9. Relation to Arithmetic Semantics

Arithmetic vector opcodes inherit canonical arithmetic semantics from RFC-0049.

This means:

- lane-local arithmetic must match scalar arithmetic per lane
- reductions or cross-lane operations require separate explicit semantics
- vector execution cannot redefine carry, overflow, or comparison behavior

### 10. Cross-Lane Operations

Cross-lane operations are a separate semantic class and must not be smuggled into lane-local vector opcodes.

Examples:

- reductions
- prefix scans
- shuffles
- permutes
- gathers/scatters

If introduced, they must define:

- lane topology
- ordering guarantees
- fault behavior
- backend-equivalence proof requirements

### 11. VM and JIT Integration

The VM must:

- decode vector opcodes as semantic operations
- validate widths and operand compatibility deterministically
- dispatch through governed backend selection
- preserve trace/policy semantics independent of backend choice

The JIT may lower vector opcodes to backend-specific instruction sequences only under RFC-0047 and RFC-0042 constraints.

### 12. Promotion Rule

No explicit vector opcode family may be promoted into Verified / DCP scope unless:

- its arithmetic or tritwise semantics are defined independently of backend
- lane ordering is canonical
- trace behavior is stable
- fallback/fault behavior is explicit
- backend equivalence is validated through RFC-0042 and RFC-0043

## Determinism / Safety Considerations

Determinism considerations:

- explicit vector semantics reduce ambiguity between "same computation" and "different optimized computation"
- lane ordering must be invariant across architectures
- vector fallback behavior must not alter trace identity or externally visible faults

Safety considerations:

- invalid widths and shapes must fail closed
- policy hooks must observe semantic vector behavior rather than backend internals
- cross-lane operations are especially sensitive and should remain narrow until separately specified

## Compatibility

This RFC is additive if explicit vector opcodes are introduced as new semantic operations.

Compatibility rules:

- existing scalar opcodes remain valid and unchanged
- internal SWAR/SIMD optimization may continue where it does not alter semantics
- any explicit vector opcode introduction must not retroactively reinterpret existing scalar bytecode

## Implementation Plan

1. Decide which current tritwise and arithmetic operations require explicit vector opcodes versus internal backend acceleration only.
2. Add normative lane-order, width, and fault semantics to the ISA/VM specs.
3. Add VM decode/validation rules for explicit vector operations.
4. Add trace/policy semantics for vector operations.
5. Add backend-equivalence and replay tests for explicit vector opcode families across scalar, SWAR, and SIMD execution.

## Open Questions

- Which vector operations should be explicit in TISC v1.x versus deferred to a future ISA revision?
- Should widths be fixed per opcode family or parameterized by operand metadata?
- Are gather/scatter and reduction semantics ready for this RFC or better handled by follow-on vector-memory/vector-reduction RFCs?

## Acceptance Criteria

- The ISA/VM specs explicitly define the semantic role of vectorized ternary operations.
- Explicit vector opcode families, if adopted, have canonical lane-order, width, trace, and fault semantics.
- Scalar, SWAR, and SIMD execution of explicit vector operations are covered by RFC-0042 equivalence tests and RFC-0043 conformance rules.
- RFC-0040 and RFC-0041 are cross-referenced as implementation-layer companions rather than de facto ISA definitions.
- JIT/lowering rules for vector operations are explicitly tied to RFC-0047.
