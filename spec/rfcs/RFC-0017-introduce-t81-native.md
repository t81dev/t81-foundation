vote: +1
tags: [proposal, architecture, performance]

# RFC: Introduce t81::T81 – register-native 2-bit-per-trit balanced ternary SIMD integer

**Status:** Proposed  
**Type:** Architecture / Performance Impact  
**Created:** 2025-11-30  
**Discussion:** https://github.com/t81dev/t81-foundation/discussions (TBD)

## Summary

Introduce `t81::T81`, a register-sized balanced-ternary integer that packs 128 trits into a single `__m256i` using a 2-bit encoding (`00 = -1`, `01 = 0`, `10 = +1`, `11` unused). Negation is implemented as a single `pshufb` mask that swaps `00`/`10` pairs while leaving `01` untouched, yielding 12–18 Gops/s (`pshufb` latency 1 cycle / throughput 0.5). Addition, comparison, and other common operations rely on shuffle/bitwise sequences instead of tryte tables. The RFC covers the new type, its core operations, diagnostics, benchmarking plans, conversion from the classic limb, and how the new performance flagship coexists with the pedagogical reference implementation.

## Motivation

Balanced ternary already delivers precise arithmetic, but the tryte-based `Cell`/`T81Limb` operations are capped in the tens of Mops/s even with Kogge-Stone parallelism. Introducing a register-native type unlocks the SIMD bandwidth of AVX2/AVX-512, allows negation to be a single instruction, and finally makes balanced ternary numerically faster than two’s complement for critical operations. This is essential for Axion-level determinism combined with high performance, and it lets the project ship a verifiable hardware-ready integer type rather than a purely pedagogical one.

## Proposal

1. Add `<t81/t81.hpp>` with:
   - `namespace t81 { struct T81 { alignas(32) __m256i data; ... }; }`
   - `T81` packs 128 trits 2 bits each; constructors convert between the classic limb and the SIMD lane losslessly inside the overlapping range.
   - `constexpr T81 operator-() const noexcept` uses `_mm256_shuffle_epi8` with a repeated `0b10010000` mask to swap `00`/`10`.
   - Additional helpers like `operator+`, comparisons, and canonical conversions can be scalar or vectorized versions.
2. Keep the classic tryte-based `Cell` and `T81Limb` untouched; they remain the reference implementation and teaching tool.
3. Add conversion utilities in `include/t81/conversion.hpp`: `T81 from_classic(const OldLimb&)` and `OldLimb to_classic(T81)` (truncating when the SIMD value exceeds the old range).
4. Benchmarks: extend `BM_NegationSpeed`, `BM_LimbAdd`, etc., with the native SIMD path, plus an `BM_vs_int128` comparison; update the runner to report Classic, Native, and Binary columns.
5. README & badges: add a badge showing the new native negation throughput (~14 Gops/s) next to the existing prestige metrics.

## Testing & Benchmarking

- Unit tests verifying round-trip conversions between the classic limb and `t81::T81`.
- Property tests covering `-(-x) == x`, `x + 0 == x`, and `x + y - y == x` in the SIMD lane.
- Benchmark runs for the new native type (`BM_NegationSpeed_T81Native`, `BM_LimbAdd_T81`, `BM_vs_int128`) plus existing ints.
- Update `docs/benchmarks.md` and README tables to expose the new columns and badge.

## Impact

- Adds a new header-only type, conversions, and benchmarks; no changes to existing public APIs.
- Requires AVX2 or optional AVX-512 (with fallbacks) but still compiles on non-SIMD hosts via scalar loops if needed.
- The reporter now emits three columns for Classic/Native/Binary so users can see both the pedagogical type and the performance flagship.

## Alternatives

- Keep only the tryte-based limb. (Too slow.)
- Introduce the SIMD type but hide it behind tooling; it needs to be public to serve as the performance flagship.

## Notes

- This is pre-1.0, so non-breaking additive exports are allowed; the classic `Cell`/`T81Limb` stays immutable.
