# RFC-0049: Canonical Ternary Arithmetic Semantics

- **RFC-ID:** RFC-0049
- **Title:** Canonical Ternary Arithmetic Semantics
- **Status:** accepted
- **Type:** standards-track
- **Applies-To:** `spec/tisc-spec.md`, `spec/t81-data-types.md`, `spec/t81lang-spec.md`, arithmetic opcodes, numeric lowering, backend equivalence work
- **Created:** 2026-03-19
- **Updated:** 2026-03-21
- **Supersedes:** None
- **Superseded-By:** None
- **Discussion:** Builds on RFC-0001, RFC-0002, RFC-0017, RFC-0018, RFC-0030, RFC-0042, RFC-0044, and RFC-0045

---

## Summary

This RFC defines the canonical arithmetic law for balanced-ternary computation in T81. It specifies the normative semantics for addition, subtraction, multiplication, negation, comparison, reduction, carry propagation, overflow signaling, and representation normalization so that scalar, SWAR, SIMD, VM, language, and future accelerator implementations share one mathematical contract.

## Motivation

T81 already implements arithmetic across multiple layers, but the architecture still relies on a mix of accepted implementation behavior, local helper conventions, and backend-specific reasoning. That is sufficient for vertical feature delivery, but insufficient for horizontal governance. Backend equivalence, packed-trit stabilization, deterministic lowering, and memory/order guarantees all need a single arithmetic oracle. Without that oracle:

- different backends can be "equivalent" to slightly different arithmetic models
- carry and overflow semantics can drift between packed and unpacked forms
- language/runtime behavior can accidentally inherit backend quirks
- future vector, JIT, or accelerator work can become mathematically ambiguous

This RFC closes that gap by defining arithmetic semantics before further acceleration and promotion work expands.

## Proposal

### 1. Canonical Domain

The canonical trit domain is balanced ternary:

- `-1`
- `0`
- `+1`

Any implementation-specific encoding is an internal representation only. Arithmetic is defined over canonical trit values and canonical trit sequences, not over byte patterns.

### 2. Canonical Representation Rule

For any public or governed arithmetic surface:

- results must normalize to canonical balanced-ternary digits
- no operation may expose an intermediate non-canonical trit state
- leading-significance normalization rules must be stable and explicit per type
- packed forms must decode to the same canonical digit sequence as scalar forms

### 3. Primitive Arithmetic Operations

The following operations are normatively defined:

- unary negation
- addition
- subtraction
- multiplication
- comparison (`<`, `<=`, `==`, `!=`, `>=`, `>`)
- sign extraction
- absolute value where applicable
- reduction operations that aggregate trit or limb sequences

Each primitive must be definable in terms of canonical trit algebra, not host-machine integer overflow behavior.

### 4. Addition and Carry Semantics

Addition is defined as balanced-ternary digitwise summation with canonical carry propagation.

Rules:

- carry values are part of the arithmetic law, not backend-local implementation detail
- carry propagation order must produce the same canonical result regardless of scalar, SWAR, or SIMD grouping
- packed or limbwise implementations may use lookup tables, prefix scans, or carry maps only if the final result matches the canonical carry law exactly
- carry observability is internal unless explicitly exposed by a public API

### 5. Subtraction

Subtraction is defined as canonical addition with canonical negation:

`a - b = a + (-b)`

No implementation may define subtraction through host underflow behavior or a non-canonical borrow convention that is not reducible to the addition/carry law above.

### 6. Multiplication

Multiplication is defined over canonical balanced-ternary digits and canonical place values.

Rules:

- partial-product generation must use canonical trit multiplication
- accumulation of partial products must obey the same canonical addition/carry law
- backend vector grouping may change execution strategy but not place-value interpretation
- any fast path that decomposes multiplication into shifts/adds/LUTs remains subordinate to canonical multiplication semantics

### 7. Comparison Semantics

Comparison is value-based, not representation-based.

Rules:

- canonical-equal values compare equal regardless of internal packing or normalization path
- comparison must not depend on stale padding, unused packed bits, or non-canonical leading storage
- lexical compare is valid only if it is proven equivalent to canonical numeric ordering for the relevant normalized representation

### 8. Overflow and Range Discipline

This RFC does not force one global overflow policy for every numeric type, but it does require each governed type or opcode family to choose one explicit policy and bind it to the arithmetic law.

Allowed policies:

- fail-closed trap or fault
- explicit checked-result contract
- modular wrap, if normatively specified
- saturating behavior, if normatively specified

Forbidden policy:

- silent host-language UB
- backend-specific implicit overflow behavior

Each arithmetic surface must document which overflow policy it adopts and must not vary that policy by backend.

### 9. Reductions and Aggregates

Reductions such as sums, dot-like accumulations, population/sign counts, and arithmetic folds must define:

- the canonical mathematical operation
- the required order model
- whether tree reduction is permitted
- the equivalence rule for parallel/backend implementations

Any associative regrouping used by a backend must be proven equivalent under the canonical arithmetic law and the scheduling/memory rules of RFC-0045 and RFC-0046.

### 10. Relation to Types

This RFC applies to:

- raw trit vectors
- packed trit vectors
- native register forms
- VM arithmetic opcodes
- language numeric lowering where ternary semantics are primary
- higher-level numeric types built from ternary primitives

Type-specific semantics may refine shape, width, overflow, or normalization, but they may not contradict the arithmetic primitives defined here.

### 11. Relation to Backend Equivalence

RFC-0042 defines when two execution backends are equivalent. RFC-0049 defines what result they must be equivalent to.

Backend equivalence for arithmetic surfaces therefore means:

- identical canonical value result
- identical fault/trap result where applicable
- identical externally governed trace semantics where arithmetic events are observable

### 12. Normative Consequence

No arithmetic implementation may be promoted to Verified / DCP scope unless:

- its operations are mapped to the canonical semantics of this RFC
- its overflow/fault behavior is explicit
- its backend equivalence is validated through RFC-0042 and RFC-0043

## Determinism / Safety Considerations

This RFC reduces nondeterminism by removing backend-local arithmetic interpretation.

Determinism considerations:

- scalar is not enough as a practical oracle if the arithmetic law itself is underdefined
- packed and vector paths must not smuggle in alternate carry or comparison rules
- reductions are a major risk surface for future multicore and accelerator work

Safety considerations:

- arithmetic faults must be fail-closed and explicit
- no arithmetic path may depend on signed-overflow UB or host FPU quirks
- public claims of "same result" must mean same canonical arithmetic value, not merely approximately similar behavior

## Compatibility

This RFC is intended to codify and unify intended behavior, not introduce a new arithmetic model.

Compatibility effects:

- implementations already matching the current accepted behavior should remain valid
- any implementation currently depending on undocumented carry, borrow, or compare quirks will need correction
- type-specific specs may need wording updates to reference this arithmetic constitution explicitly

## Implementation Plan

1. Add a normative arithmetic-semantics section to the relevant core specs.
2. Map VM arithmetic opcodes and helper APIs to the canonical law.
3. Audit packed/native/SWAR/SIMD arithmetic helpers against the canonical carry and normalization rules.
4. Add conformance tests for canonical addition, subtraction, multiplication, comparison, and overflow behavior.
5. Bind RFC-0042 backend-equivalence tests to RFC-0049 arithmetic expectations.

## Open Questions

- Which arithmetic surfaces, if any, should normatively adopt modular wrap rather than checked failure?
- Should dot-product and similar reductions be specified in this RFC or a follow-on arithmetic-reductions RFC?
- How much of ternary arithmetic for higher composite types belongs here versus in type-specific specs?

## Acceptance Criteria

- A normative arithmetic section exists in the core specs and references this RFC.
- Addition, subtraction, multiplication, negation, and comparison semantics are explicitly defined and no longer backend-implicit.
- Overflow behavior is explicit for all governed arithmetic surfaces touched by this RFC.
- Conformance tests exist for scalar, packed, SWAR, and SIMD arithmetic against the same canonical result set.
- RFC-0042 and RFC-0043 reference RFC-0049 as the arithmetic oracle for backend proof on arithmetic surfaces.

## Implementation Record (2026-03-21)

All acceptance criteria are satisfied as of this date.

**AC1 — Normative spec section:**
`spec/tisc-spec.md §5.2.1` ("Normative Arithmetic Semantics (RFC-0049)") was added,
documenting canonical domain, primitive semantics table, carry propagation rule,
overflow policy table per surface, forbidden behaviors, relation to backend
equivalence, and conformance test reference.

**AC2 — Operations explicitly defined:**
The §5.2.1 primitive semantics table defines negation, addition, subtraction,
multiplication, and comparison in terms of canonical trit algebra.  The subtraction
identity `a − b ≡ a + (−b)` is normative; no independent borrow convention is
permitted.

**AC3 — Overflow behavior explicit:**
The §5.2.1 overflow policy table explicitly binds each governed surface to one
policy: `T81BigInt` → unbounded; `T81Int<N>` → exception trap; `T81Float<M,E>` →
special-value saturation; raw trit-vector decode → saturation; integer division →
explicit `DivisionFault`.  Silent host UB is a named forbidden behavior.

**AC4 — Conformance tests:**
`tests/cpp/test_arithmetic_backend_equivalence.cpp` was created and registered as
`t81_arithmetic_backend_equivalence_test`.  It verifies for the scalar trit oracle
(`t81::ternary::add` / `encode_i64` / `decode_i64`) and the T81BigInt multi-limb
chunk-based packed path:

- negation (trit-flip involution, 2000 random + edge cases)
- addition (2000 random, commutativity, identity, edge cases)
- subtraction (2000 random, identity `a−b=a+(−b)`, `a−a=0`)
- multiplication (2000 cases vs int64 reference, repeated-addition oracle for
  small multipliers, commutativity, identity, zero, negate-by-−1, distributivity)
- comparison semantics (value-based, reflexivity, int64 ordering parity, neg(0)==0)
- overflow policy (BigInt unbounded, `to_int64` throws for out-of-range)
- carry propagation (boundary cases, powers of 3, carry-chain stress)

**AC5 — RFC-0042 and RFC-0043 updated:**
RFC-0042 §1 (Canonical Oracle) now explicitly names RFC-0049 as the arithmetic
oracle for arithmetic surfaces.  RFC-0043 §3 (Canonical Corpus Classes) now
requires RFC-0049-derived corpus cases for arithmetic surfaces and references
`t81_arithmetic_backend_equivalence_test`.  Both RFCs updated their Discussion
and References fields.
