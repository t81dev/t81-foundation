# RFC-0044: Stable Packed Trit Vector Interface

- **RFC-ID:** RFC-0044
- **Title:** Stable Packed Trit Vector Interface
- **Status:** accepted
- **Type:** standards-track
- **Applies-To:** packed trit storage, SWAR, SIMD, VM tensor helpers, future JIT lowering
- **Created:** 2026-03-19
- **Updated:** 2026-03-21
- **Supersedes:** None
- **Discussion:** Builds on RFC-0040 and RFC-0041; stabilizes the substrate currently exposed through `experimental/packed_trit_vector.hpp`

## Summary

This RFC stabilizes the packed trit vector substrate used by SWAR and SIMD execution.

It defines:

- canonical packed-trit encoding invariants
- API surface boundaries
- layout and mutability rules
- validation requirements
- serialization expectations

The goal is to stop treating the packed-trit substrate as an implementation detail while higher-level backends depend on it as if it were already stable.

## Motivation

Today, both SWAR and SIMD surfaces rely on types currently rooted in:

- `t81::experimental::PackedTritVector`
- `t81::experimental::ComputeTritVector`

That creates a governance mismatch:

- optimized backends are being formalized
- their shared data substrate still lives under an experimental namespace

Without a stable packed trit vector contract:

- backend equivalence language remains underspecified
- ABI expectations can drift silently
- VM helpers and future JIT lowering depend on a moving target

## Proposal

### 1. Stable Surface Introduction

T81 SHALL introduce a stable packed-trit surface under a governed namespace.

The stable surface MUST provide:

- a canonical owned packed-trit container
- construction from canonical trit sequences
- conversion back to canonical trit sequences
- validated access to raw packed bytes
- deterministic equality over both trit content and canonical packed representation

The experimental namespace may remain temporarily as a compatibility shim, but it must no longer be the constitutional source of truth.

### 2. Canonical Encoding Rules

The packed-trit interface MUST define one canonical encoding for governed trit vectors.

For the current surface, that means:

- 2-bit packed trit encoding
- fixed valid-bit patterns only
- deterministic handling of trailing storage

The interface MUST explicitly state:

1. trits per byte
2. valid and invalid bit patterns
3. tail masking behavior
4. zero-extension behavior for unused trailing slots

### 3. Layout Guarantees

The stable interface MUST guarantee:

- contiguous byte storage
- deterministic byte length as a function of trit count
- deterministic mapping from trit index to byte/bit position

It MUST NOT guarantee:

- host-specific alignment beyond documented minima
- direct compatibility with arbitrary host SIMD register layouts
- binary compatibility with older experimental internal-only representations unless explicitly versioned

### 4. Mutability Model

The interface MUST define which operations are:

- pure and allocate new storage
- in-place and mutation-bearing
- raw-buffer kernel operations

In-place operations MUST preserve canonical encoding invariants.

Raw kernel entry points MUST either:

1. require prevalidated canonical data, or
2. perform deterministic validation before mutation

The distinction must be explicit in the API.

### 5. Validation Contract

The stable interface MUST specify deterministic validation behavior for:

- invalid packed bit patterns
- length mismatches
- malformed tail bytes
- unsupported conversion requests

Validation failures MUST be represented by deterministic error values, not undefined behavior.

### 6. Serialization Rules

If packed-trit vectors are serialized across module boundaries, the stable interface MUST define:

- canonical byte serialization
- versioning policy
- whether the serialized format is DCP-governed

If the packed representation is used only internally for execution, the RFC MUST still define canonical in-memory byte meaning so backend equivalence can be tested against it.

### 7. Backend Interoperability

The stable packed-trit interface is the shared substrate for:

- scalar reference conversion
- SWAR kernels
- SIMD kernels
- VM tensor helpers
- future JIT lowering

Any backend operating on packed-trit storage MUST consume the same canonical packed layout.

Backend-specific reinterpretations are forbidden on the deterministic surface.

### 8. Threshold Independence

Dispatch thresholds belong to backend RFCs, not to the packed-trit interface itself.

This RFC governs representation and manipulation semantics, not optimization policy.

### 9. Migration Rules

Promotion to a stable packed-trit interface SHOULD follow this path:

1. create stable header and namespace
2. keep experimental aliases temporarily
3. migrate SWAR and SIMD facades to the stable type
4. retire direct external dependence on `experimental`

No semantic rewrite is required during namespace promotion.

## Acceptance Criteria

This RFC is ready for `accepted` when all of the following are true:

1. a stable packed-trit type exists outside the experimental namespace
2. canonical encoding and tail-byte rules are documented in code and RFC text
3. SWAR and SIMD public headers use the stable substrate rather than `experimental` aliases
4. invalid-pattern handling is executable and covered by tests
5. VM helper code using packed-trit execution paths depends on the stable surface

## Impact

### Backward Compatibility

This RFC may introduce a compatibility layer and eventual deprecation path for direct external use of `experimental/packed_trit_vector.hpp`.

### Performance

No direct regression is intended.

A stable substrate may improve optimization work by freezing invariants instead of rediscovering them across layers.

### Security

A stable validation contract reduces UB and malformed-input ambiguity in backend-sensitive code.

## Alternatives Considered

### Leave the substrate in `experimental`

Rejected because SWAR and SIMD are already being promoted above it.

### Stabilize SWAR and SIMD independently without a shared substrate RFC

Rejected because they already share representation assumptions.

### Freeze only the raw byte layout and not the API

Rejected because mutation and validation semantics are part of the determinism contract.

## Implementation Record (2026-03-21)

All acceptance criteria are satisfied as of this date.

**AC1 — Stable type outside experimental namespace:**
`t81::ComputeTritVector` and `t81::PackedTritVector` are the governed stable
aliases at `include/t81/packed_trit_vector.hpp`.  The implementation in
`t81::experimental` is the backing detail; the public surface is `t81::`.
`vm/tensor_helpers.cpp`, `vm/jit/jit_compiler.cpp`, and all four benchmark files
were updated to use `t81::ComputeTritVector` / `t81::PackedTritVector` directly.

**AC2 — Canonical encoding and tail-byte rules documented:**
`spec/tisc-spec.md §5.2.2` ("Normative Packed-Trit Encoding (RFC-0044)") now
documents the 2-bit encoding table (00=Z, 01=P, 11=N, 10=forbidden), LSB-first
byte layout, 4 trits/byte, zero-extension of trailing bits, and the
deterministic byte-count formula `⌈trit_count / 4⌉`.

**AC3 — SWAR and SIMD headers use stable substrate:**
`include/t81/swar/swar.hpp` includes `"t81/packed_trit_vector.hpp"` (stable shim).
`include/t81/tritwise/tritwise.hpp` likewise uses the stable surface for dispatch.
No SWAR or SIMD public header imports `experimental` directly.

**AC4 — Invalid-pattern handling executable and covered by tests:**
`test_rfc0044_invalid_pattern_rejection()` was added to `tests/cpp/test_swar.cpp`.
It verifies: `from_packed` rejects the `10` bit pattern in trit 0 and trit 1;
rejects non-zero tail padding for a 3-trit vector; accepts valid bytes and
round-trips correctly; `to_trits()` only produces `{-1, 0, +1}`.

**AC5 — VM helpers depend on stable surface:**
`vm/tensor_helpers.cpp` now uses `using t81::ComputeTritVector;` (stable alias).
`vm/jit/jit_compiler.cpp` now includes `t81/packed_trit_vector.hpp` explicitly
and uses `t81::ComputeTritVector` throughout.

## References

- `spec/rfcs/RFC-0040-swar-formalization.md`
- `spec/rfcs/RFC-0041-simd-formalization.md`
- `spec/rfcs/RFC-0049-canonical-ternary-arithmetic-semantics.md`
- `include/t81/packed_trit_vector.hpp`
- `include/t81/experimental/packed_trit_vector.hpp`
- `include/t81/swar/swar.hpp`
- `include/t81/simd/simd.hpp`
- `vm/tensor_helpers.cpp`
- `vm/jit/jit_compiler.cpp`
- `tests/cpp/test_swar.cpp`
