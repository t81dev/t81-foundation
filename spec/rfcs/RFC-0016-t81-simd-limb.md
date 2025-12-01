vote: +1

# RFC-0016: Register-native SIMD T81 Limb

**Status:** Proposed  
**Type:** Standards Track  
**Created:** 2025-12-xx  
**Updated:** 2025-12-xx  
**Requires:** —  
**Supersedes:** —  
**Discussion:** https://github.com/t81dev/t81-foundation/discussions (TBD thread)

______________________________________________________________________

## Summary

Introduce a new public primitive (`t81::simd::T81` plus `T81x2` for AVX-512) that stores 128 (resp. 256) balanced-ternary trits packed as 2 bits per trit inside SIMD registers using the encoding `00 = −1`, `01 = 0`, `10 = +1`, `11 = reserved`. Negation becomes a single `pshufb` with a constant mask that swaps `00` and `10`, addition and reduction use shuffle/bitwise instructions instead of tryte tables, and comparisons/abs become a handful of mask operations. This lets us ship a register-native ternary limb with one-instruction negation and up to 10–15 Gops/s throughput on modern x86 cores while keeping the existing tryte-based `Cell`/`Limb` for educational and correctness cross-checking.

## Motivation

The current 16-tryte `T81Limb` provides mathematically beautiful carry-map semantics, but even the best parallel-prefix adder is limited to tens of millions of operations per second, making T81 non-competitive for raw integer workloads. A register-native encoding removes the tryte tables, eliminates carry chains for negation, and leverages SIMD throughput to make balanced ternary faster than `int64_t` on the most critical operation (negation) while preserving the enormous range of 128+ trits. That performance and determinism are the explicit positivity Axion watches for, so we need a new type defined in the spec before implementation.

## Guide-Level Explanation

1. **Encoding:** Pack 128 trits into one `__m256i` register using 2 bits/trit. The low-order two-bit pattern maps to balanced digits (−1, 0, +1). `11` remains reserved and treated as a poison value.
2. **Negation:** Use a 32-byte `PSHUFB` mask (`0b10010000` repeated) to swap the `00`/`10` patterns while leaving `01` untouched. The mask is constant, so negation is `1 x PSHUFB`.
3. **Addition:** Replace tryte table lookups with in-register prefix-sum using bitwise majority and carry propagation via fold/shuffle sequences specific to AVX2/AVX-512. No heap tables or loops are required.
4. **Comparison & Sign Tests:** Use mask + blend operations to build `<`, `>`, `==`, `sign`, `abs`, and `sat` operations within a few instructions.
5. **Backwards compatibility:** The existing `t81::core::T81Limb` and `t81::core::Cell` remain unchanged; the SIMD type supplements them under a new namespace (`#include <t81/simd/t81.hpp>`).

## Reference-Level Design

- `namespace t81::simd`: exposes `struct T81 { alignas(32) __m256i data; /* ... */ }` and `struct T81x2 { alignas(64) __m512i data; }`.
- Negation:  
  ```cpp
  inline T81 T81::operator-() const noexcept {
      return T81{_mm256_shuffle_epi8(data, negate_mask)};
  }
  ```
  where `negate_mask` is `alignas(32) static constexpr uint8_t mask[32] = {0b10010000, …}`.
- Addition: implement bitwise carry propagation by folding 2-bit lanes with `_mm256_add_epi8`, `_mm256_andnot_si256`, and `_mm256_shuffle_epi8` to emulate carry maps; fallback to scalar loops when SIMD absent.
- Tests: Provide canonical masks, 2-bit lookups, and coverage for overflow/poison handling.
- Benchmarks: `BM_NegationSpeed` should exercise `T81` negation and show >10 Gops/s; `BM_LimbArithThroughput` will show the SIMD limb versus `__int128`.
- Documentation: Mention the new type in Overviews/Benchmarks docs and highlight that `T81` negation matches the perfect mask operation.

## Impact

- Adds a new public API under `include/t81/simd` (or similar) without breaking existing `core` headers.
- Requires new unit/property tests verifying the 2-bit encoding, mask behaviors, and addition semantics.
- `BM_NegationSpeed` and other benchmarks need to be updated to publish the new SIMD limb and show the improved throughput.
- Build scripts must guard AVX2/AVX-512 support; platforms without those features keep falling back to the tryte-based `T81Limb`.
- Reports (docs/benchmarks.md) and analysis tooling must treat the new type as the canonical T81 baseline for latency/throughput comparisons.

## Alternatives

1. Keep only the tryte-based `T81Limb` and optimize the existing Kogge-Stone adder further. (Insufficient throughput.)
2. Lower to bit-level emulation in software without introducing new SIMD types. (Still slower and loses the register-native argument.)
3. Implement the SIMD limb as a private prototype inside tooling without bringing it into `include/t81`. (Would hide performance gains from consumers.)

## Testing

- Add deterministic unit tests for `negate_mask`, addition-carry propagation, and conversions to/from `int128`/`Cell`.
- Property tests ensuring `-(-x) == x`, `x + 0 == x`, and `carry` semantics across all 128 trits.
- Extend benchmarks to run on both the new SIMD limb and the legacy tryte-based limb.
- Regression: ensure docs/benchmarks.md and other tooling continue to publish T81/Binary flows as before.

## Backwards Compatibility

This RFC introduces a new type and does not alter existing types. Legacy code continues to use `t81::core::T81Limb` and `t81::core::Cell` unchanged. Migration simply means including the new header and opting into the SIMD lane where available.

## Security Considerations

- Avx2/AVX-512 code paths are optional; non-AVX hosts fall back to scalar loops ensuring deterministic behavior.
- The `11` pattern is reserved; all constructors/masks must sanitize inputs to avoid undefined states.
- The SIMD lane inherits the deterministic semantics of existing balanced ternary operations, so Axion’s invariants stay intact.

## Future Work

- Extend the SIMD limb to `T81x2` for AVX-512, enabling 256-trit operations.
- Provide JIT hooks in Axion/governor to detect when `T81` operations are available and prefer them in hot paths.
- Explore formal verification of the new addition/negation micro-ops for Axion audit.
