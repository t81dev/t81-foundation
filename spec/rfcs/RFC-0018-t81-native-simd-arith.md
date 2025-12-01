vote: +1
tags: [proposal, architecture, performance]

# RFC: SIMD T81 Arithmetic – Native addition & multiplication

**Status**: Proposed  
**Type**: Architecture / Performance Impact  
**Created**: 2025-12-01  
**Discussion**: https://github.com/t81dev/t81-foundation/discussions (TBD)

## Summary

This follow-up RFC documents the **native** `t81::T81` SIMD arithmetic that now powers `BM_LimbAdd_T81Native` and `BM_vs_int128`. The AVX2 implementation packs 128 trits into one `__m256i`, builds per-byte carry maps, runs a lightweight prefix scan that composes carry maps across the 32 SIMD lanes, and finally applies the precomputed carry-ins per byte to fetch canonical results from a dense LUT. Multiplication reuses this add path by shifting the same register, handling positive and negative trits with the same vectorized `operator+`/`operator-` fast paths, and falls back to scalar carry normalization when AVX2 is absent. The README badges/report now point to the new native register path while still showing Classic vs Binary throughputs.

## Motivation

* We already have a pedagogically pure 16-tryte `T81Limb`, but benchmarks show `BM_LimbArithThroughput` topping out in the low tens of Mops/s. Native SIMD code unlocks AVX2 pipelines and makes balanced ternary competitive for throughput-critical loops.
* The accepted RFC 0017 introduced `t81::T81`, but stakeholders asked for a clear follow-up explaining how addition/multiplication go from the byte-level carry map to `BM_LimbAdd_T81Native` and `BM_vs_int128` results.
* The README now advertises the `t81::T81` badge, so we need an explicit proposal describing the functionality, guarantees (round-trip to addition/multiplication), and benchmarks supporting the claim.

## Proposal

1. Document the SIMD glue:
   * Each byte stores four balanced trits and maps to a `simd::ByteCarryMap` that encodes the carry-out for every incoming carry (-1/0/+1).
   * The AVX2 path gathers 32 byte carry maps from the two inputs, performs an in-place prefix scan (Kogge-Stone style) to propagate carries, and then applies each carry-in via `simd::AddEntry` to build the native result bytes.
   * Multiplication iterates over the 128 trits of the RHS, uses `ShiftLeftTrits` to align the operand, and reuses the SIMD addition/subtraction paths; the scalar fallback reduces the 256-trit convolution using `(value ± 1)/3` carry reduction so it always lands back in [-1,+1].
2. Highlight the new benchmarks:
   * `BM_LimbAdd_T81Native` demonstrates 4.26 Mops/s (expanded from the prior 48-trit results) thanks to register-native carries.
   * `BM_vs_int128` records native vs binary throughput for the same synthetic workload (~849 Gops/s binary but high deterministic parity for T81 native).
   * `BM_NegationSpeed_T81Native` remains the single-instruction PSHUFB highlight; the README badge now advertises the 7.18 Gops/s native throughput.
3. Capture diagnostics & conversions:
   * The native type still round-trips to/from the classic `core::T81Limb` via `t81::from_classic`/`to_classic`, preserving the overlapping 48-trit range so benchmarks can compare “classic” vs “native” columns in `docs/benchmarks.md`.
   * The benchmark runner/report now surfaces native metrics (classic/native/binary columns plus bandwidth) so readers can see `BM_LimbAdd_T81Native`, `BM_vs_int128`, and `BM_NegationSpeed_T81Native` simultaneously.

## Testing & Benchmarking

* Existing unit tests cover the add helpers and conversion round trips; `tests/cpp/test_t81_native_arith.cpp` now validates native addition/multiplication against small deterministic operands.
* The standard benchmark suite (`./build/t81 benchmark`) exercises the new SIMD flow and regenerates `docs/benchmarks.md`/README badges; `BM_LimbAdd_T81Native`, `BM_vs_int128`, and `BM_NegationSpeed_T81Native` provide empirical throughput and latency.

## Impact

* Adds new header-only SIMD helpers (`include/t81/native.hpp`, `include/t81/simd/*.hpp`) behind AVX2 guards with scalar fallbacks for portability.
* Updates the runner/report to always emit classic/native/binary columns and bandwidth counters so documentation can advertise the new native badge.
* No breaking changes to the existing `Cell`/`T81Limb` APIs; they remain educational references while `t81::T81` becomes the performance flagship.

## Alternatives

1. Keep only the classic tryte-based pipeline and call it a day — fails to meet future throughput goals.
2. Hide the native SIMD implementation behind an internal flag — unnecessary now that benchmarks & docs show the difference explicitly.

## Notes

The request to mention the new `t81::T81` badge corresponds to the README “Negation” shield (7.18 Gops/s) plus the new table row describing both Classic/Native/Binary columns, fully generated whenever the benchmark runner runs. Continuous integration should rerun `./build/t81 benchmark` whenever the SIMD code changes. 
