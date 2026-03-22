# RFC-0043: Deterministic Conformance and Validation Framework

- **RFC-ID:** RFC-0043
- **Title:** Deterministic Conformance and Validation Framework
- **Status:** accepted
- **Type:** standards-track
- **Applies-To:** TISC, T81VM, compiler determinism, SWAR, SIMD, DPE, CI governance
- **Created:** 2026-03-19
- **Updated:** 2026-03-21
- **Supersedes:** None
- **Discussion:** Builds on RFC-0002, RFC-0027, RFC-0042, RFC-0049; governs proof model for all accepted determinism surfaces

## Summary

This RFC defines how determinism claims in T81 are proven, classified, and enforced.

It introduces a common validation framework for:

- backend equivalence
- cross-platform replay
- trace-hash validation
- CI gating
- determinism breach classification

The goal is to replace implied validation with a single governed conformance model.

## Motivation

The repository already contains substantial determinism testing and cross-platform verification, but the proof model is distributed across ad hoc suites, scripts, and workflow conventions.

That is no longer sufficient.

T81 now needs one RFC that answers:

- what corpus must pass before a surface is called deterministic
- what exact artifacts must match
- how backend equivalence is proven
- how divergence is classified
- what CI is required before merge or promotion

Without this RFC, determinism remains partly cultural instead of fully constitutional.

## Proposal

### 1. Conformance Layers

Deterministic validation in T81 is divided into five layers:

1. semantic conformance
2. backend equivalence
3. serialization conformance
4. cross-platform replay
5. governance gate enforcement

Each deterministic surface MUST declare which layers apply.

### 2. Validation Artifacts

The canonical validation artifacts are:

- input corpus
- expected result corpus
- trace hash or equivalent trace-visible evidence
- fault classification
- backend matrix result
- platform matrix result

Wall-clock timing is evidence for performance only. It is not determinism evidence.

### 3. Canonical Corpus Classes

Every governed deterministic surface MUST be tested against at least these corpus classes:

1. nominal valid cases
2. edge-width boundary cases
3. malformed or invalid encoding cases
4. fault-producing cases
5. randomized differential cases with deterministic seed

For packed-trit and backend surfaces, the corpus MUST explicitly include:

- scalar vs SWAR
- scalar vs SIMD
- size boundaries around backend thresholds
- invalid packed-trit encodings
- tail and alignment edge cases

For arithmetic surfaces (ADD, SUB, MUL, NEG, and type-specific variants), the corpus
MUST additionally include cases that validate conformance to the RFC-0049 arithmetic
oracle: carry propagation across trit boundaries, negation involution, the subtraction
identity `a − b = a + (−b)`, multiplication commutativity and distributivity, and the
explicit overflow/fault policy declared per surface.  The conformance test
`t81_arithmetic_backend_equivalence_test` satisfies this requirement for the scalar
and BigInt multi-limb surfaces.

### 4. Cross-Platform Replay Protocol

For every verified deterministic surface, the replay protocol MUST record:

1. commit or artifact identity
2. platform and compiler tuple
3. test corpus identity
4. resulting deterministic artifacts

The replay succeeds only if all governed artifacts match exactly across supported architectures.

Supported architectures are defined by the current governance surface, not by every possible host.

### 5. Backend Equivalence Matrix

For backend-governed execution surfaces, the minimum matrix is:

- scalar vs SWAR
- scalar vs SIMD
- SWAR vs SIMD

When JIT is in scope, the matrix expands to:

- scalar vs JIT
- SWAR vs JIT
- SIMD vs JIT

When heterogeneous acceleration is in scope, the matrix expands again.

No backend may be promoted into the deterministic surface without an explicit matrix entry.

### 6. Determinism Breach Classification

Determinism failures are classified as:

#### Hard Divergence

- final bytes differ
- trap class differs
- trace hash differs
- canonical serialized output differs

This is a merge-blocking critical defect on a verified surface.

#### Soft Divergence

- non-DCP diagnostic strings differ
- non-governed benchmark metadata differs
- non-governed logging differs

This does not count as DCP failure unless it crosses into a governed boundary.

#### Undefined Behavior Exposure

- backend depends on UB
- memory layout assumptions break across compilers
- function ABI mismatch changes results

This is treated as a hard determinism breach if it affects a verified surface.

### 7. CI Enforcement Rules

Each verified deterministic surface MUST map to:

1. a local executable or scriptable conformance harness
2. at least one CI workflow gate
3. a documented owner RFC or determinism-surface entry

Required CI categories:

- unit-level determinism checks
- cross-platform replay checks
- backend equivalence checks where applicable
- sanitizer or UB detection on backend-sensitive code paths

### 8. Promotion Rules

A surface may be marked `Verified` in governance only when:

1. the applicable conformance layers are declared
2. the required corpus exists in-repo
3. CI gates are wired and passing
4. any architecture-specific exclusions are documented explicitly

Experimental surfaces may use the framework, but they MUST NOT inherit DCP claims until the full promotion requirements are met.

### 9. Evidence Records

Human-readable evidence records may summarize results, but they do not replace executable conformance.

Evidence records SHOULD include:

- date
- architecture
- compiler tuple
- corpus or benchmark identifiers
- conclusion

Executable conformance remains authoritative.

### 10. Scope Boundaries

This RFC governs proof and enforcement.

It does not define:

- the backend semantics themselves
- the packed trit vector ABI
- the full deterministic memory model

Those are governed by companion RFCs.

## Acceptance Criteria

This RFC is ready for `accepted` when all of the following are true:

1. the Determinism Surface Registry references this framework as the proof model for verified surfaces
2. backend equivalence suites are mapped into the framework explicitly
3. cross-platform replay artifacts are defined for the currently supported architecture set
4. determinism breach classes are used consistently in CI and governance language
5. at least one promotion path cites this RFC for proof of determinism rather than bespoke wording

## Impact

### Backward Compatibility

This RFC does not change user-visible semantics.

It changes the burden of proof for determinism claims.

### Performance

The framework may increase CI cost and test runtime, but it reduces governance ambiguity.

### Security

This RFC strengthens detection of UB, silent drift, and unsupported determinism claims.

## Alternatives Considered

### Keep validation distributed across individual RFCs

Rejected because drift in proof standards becomes inevitable.

### Use benchmark parity as determinism proof

Rejected because timing parity is not semantic parity.

### Validate only final values, not trace artifacts

Rejected because T81 determinism is trace-relevant and audit-relevant.

## References

- `spec/rfcs/RFC-0002-deterministic-execution-contract.md`
- `spec/rfcs/RFC-0027-spec-as-executable.md`
- `spec/rfcs/RFC-0042-deterministic-backend-equivalence-contract.md`
- `spec/rfcs/RFC-0049-canonical-ternary-arithmetic-semantics.md` — arithmetic oracle for backend proof on arithmetic surfaces
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/governance/FREEZE_ENFORCEMENT.md`
- `tests/cpp/test_tritwise_backend_equivalence.cpp`
- `tests/cpp/test_arithmetic_backend_equivalence.cpp`
- `tests/cpp/vm_determinism_property_test.cpp`

## Implementation Record (2026-03-21)

All acceptance criteria are satisfied as of this date.

**AC1 — Determinism Surface Registry references this framework as the proof model:**
`docs/governance/DETERMINISM_SURFACE_REGISTRY.md §5` proof model note was updated to
declare RFC-0043 `accepted` and authoritative.  The conditional "once accepted" qualifier
was removed.  §5 now cites RFC-0043 §8 promotion rules as the governing promotion path.

**AC2 — Backend equivalence suites explicitly mapped to the framework:**
`docs/governance/DETERMINISM_SURFACE_REGISTRY.md §5.1` (Conformance Layer Mapping)
was added.  It maps every verified surface to the applicable RFC-0043 §1 conformance
layers (semantic, backend equivalence, serialization, cross-platform, governance gate).
The Tritwise and Arithmetic Backend Equivalence rows are mapped to layers 1+2+4+5.

**AC3 — Cross-platform replay artifacts defined for the supported architecture set:**
`docs/governance/DETERMINISM_SURFACE_REGISTRY.md §5.2` (Cross-Platform Replay Artifacts)
was added.  It defines the two-row replay matrix: x86_64+AVX2 (Clang 18+/GCC 14+,
scalar/SWAR/AVX2 paths) and AArch64+NEON (AppleClang 17+/Clang 18+, scalar/SWAR/NEON-OR).
Architecture-specific exclusions (NEON TNot/TAnd disabled) are explicitly documented
as non-divergence entries per `spec/tisc-spec.md §5.2.3`.

**AC4 — Determinism breach classes used consistently in CI and governance language:**
`docs/governance/FREEZE_ENFORCEMENT.md §4` (Determinism Breach Protocol) was updated
to adopt the RFC-0043 §6 breach taxonomy table (Hard Divergence, Soft Divergence,
UB Exposure) with explicit merge-impact column.  The prior "Critical Defect" wording
now maps to Hard Divergence.  `docs/governance/DETERMINISM_SURFACE_REGISTRY.md §6`
was updated to list all three breach classes and reference RFC-0043 §6 by section.

**AC5 — At least one promotion path cites RFC-0043 for proof of determinism:**
The RFC-0042 implementation record (2026-03-21) was updated with a "Promotion framework"
paragraph that explicitly cites RFC-0043 as the governing proof model, names the
applicable conformance layers (1 and 2), references the cross-platform replay artifact
table, and references the breach classification codification.  This is the first
promotion path in the project governed by RFC-0043 rather than bespoke wording.
