# PackedTritVector Prototype Phase 2A Report

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [PackedTritVector Prototype Phase 2A Report](#packedtritvector-prototype-phase-2a-report)
  - [1. Summary](#1-summary)
  - [2. Phase 1 Report Tightening](#2-phase-1-report-tightening)
  - [3. Phase 2A Compute Representation Prototype](#3-phase-2a-compute-representation-prototype)
  - [4. Conformance and Representation-Invariance Verification](#4-conformance-and-representation-invariance-verification)
  - [5. Benchmark Comparisons](#5-benchmark-comparisons)
  - [6. Validation Results](#6-validation-results)
  - [7. Phase 2B Recommendations and Remaining Gaps](#7-phase-2b-recommendations-and-remaining-gaps)

<!-- T81-TOC:END -->


## 1. Summary

This report documents the Phase 2A investigation into a compute-friendly trit representation for `PackedTritVector`.
While Phase 1 (PT-5 backing) established a semantic baseline, it suffered from high overhead due to the full unpack-operate-repack cycle required for every operation.

Phase 2A prototyped a **2-bit per trit (4 trits/byte)** representation (`ComputeTritVector`) to evaluate if a simpler packing format would improve performance using the same unpack-operate-repack strategy.
The prototype also strengthened the semantic guarantees by adding explicit truth-table verification for the non-commutative `TXor` operation and cross-representation equivalence checks.

**Key Findings:**
*   **Representation:** 2-bit packing (00=0, 01=1, 11=-1) is viable and correct.
*   **Semantics:** Exact semantic equivalence to scalar logic and Phase 1 PT-5 implementation was verified.
*   **Performance:** The naive unpack-operate-repack strategy remains the bottleneck. Phase 2A (2-bit) performance is comparable to or slightly slower than Phase 1 (PT-5), likely due to unoptimized bit-twiddling in the prototype codec versus the mature PT-5 codec.
*   **Conclusion:** Merely changing the packing format is insufficient. The next phase must implement operations **directly on the packed representation** (bypassing unpacking) to achieve significant speedups. 2-bit packing is highly suitable for this (via SWAR/SIMD), whereas PT-5 is not.

## 2. Phase 1 Report Tightening

The Phase 1 report was updated to:
*   Explicitly define `TXor` as **non-commutative ternary difference** (`lhs - rhs` wrapped), aligning with TISC VM semantics.
*   Clarify benchmark methodology (inclusive of full codec roundtrip).
*   Fix validation status reporting.
*   Note the distinction between "storage representation" (PT-5, canonical) and "compute representation" (likely 2-bit or SIMD-friendly).

## 3. Phase 2A Compute Representation Prototype

**Chosen Representation:**
*   **Format:** 2-bit per trit (4 trits per byte).
*   **Mapping:** `0 -> 00`, `1 -> 01`, `-1 -> 11` (binary `10` is invalid).
*   **Rationale:** This mapping aligns with common 2-bit signed integer representations in some SIMD instruction sets and allows for relatively simple bitwise logic for ternary operations compared to PT-5's base-243 encoding.

**API and Semantics:**
*   Class: `t81::experimental::ComputeTritVector`
*   Implements `from_trits`, `to_trits`, `from_phase1`.
*   Implements `t_and`, `t_or`, `t_xor`, `t_not` using the canonical scalar logic on unpacked trits (for semantic verification).

**Error Handling:**
*   Validates input trit values (`-1, 0, 1`).
*   Validates length matching for binary ops.
*   Validates 2-bit patterns during unpacking (detects invalid `10` pattern).

## 4. Conformance and Representation-Invariance Verification

A comprehensive test suite (`tests/cpp/test_packed_trit_vector.cpp`) now verifies:

1.  **Scalar Reference:** All operations checked against a clean scalar implementation.
2.  **TXor Truth Table Guard:** Explicit 9-case truth table test for `TXor` to ensure `(a,b) -> (a-b)` semantics are preserved and distinct from binary XOR.
3.  **Cross-Representation Equivalence:**
    *   `Scalar(op) == Phase1(op)`
    *   `Scalar(op) == Phase2A(op)`
    *   `Phase1(op) == Phase2A(op)`
4.  **Randomized Determinism:** Property-based testing with fixed seeds on various vector lengths (0 to 100).

All tests pass, confirming that `ComputeTritVector` is semantically interchangeable with `PackedTritVector` (PT-5).

## 5. Benchmark Comparisons

Benchmarks comparing Scalar, Phase 1 (PT-5), and Phase 2A (2-bit) were executed on a size of 4096 trits.
Results below (approximate, running on `devbox`):

| Operation (4096 trits) | Time (µs) | vs Scalar | Notes |
| :--- | :--- | :--- | :--- |
| **Scalar TAnd** | **0.16** | 1x | Baseline (naive loop) |
| **Phase 1 (PT-5) TAnd** | **54.0** | ~330x slower | Unpack -> Op -> Repack |
| **Phase 2A (2-bit) TAnd** | **82.0** | ~500x slower | Unpack -> Op -> Repack (naive) |
| | | | |
| **PT-5 Pack** | **13.0** | - | Codec overhead |
| **PT-5 Unpack** | **14.0** | - | Codec overhead |
| **2-bit Pack** | **7.7** | - | Naive implementation |
| **2-bit Unpack** | **14.0** | - | Naive implementation |

**Observations:**
*   The **unpack-operate-repack strategy is the bottleneck**. The overhead of converting to/from logical trits dominates the execution time, making both packed implementations orders of magnitude slower than scalar operations.
*   Phase 2A (2-bit) is slightly slower than Phase 1 (PT-5) for the full cycle in this prototype. This is likely because `pack_pt5` is a mature, optimized codec, while the Phase 2A implementation uses naive bit manipulation.
*   However, **2-bit packing enables SWAR/SIMD optimization** (operating on 4 trits/byte or 32 trits/register simultaneously) without unpacking. PT-5 does not easily support this due to the base-243 arithmetic encoding.

## 6. Validation Results

*   [x] Phase 1 report tightened (`TXor` wording, benchmark methodology, validation wording)
*   [x] Phase 2A compute representation prototype implemented (`ComputeTritVector` / 2-bit)
*   [x] Phase 2A uses canonical `TAnd/TOr/TXor/TNot` semantics matching Phase 1/TISC
*   [x] Conformance tests added (fixed + randomized + error cases + cross-representation checks)
*   [x] Explicit 9-case `TXor` truth-table guard added
*   [x] Benchmark scaffold extended to compare scalar vs Phase 1 vs Phase 2A
*   [x] Conversion-inclusive vs op-only timing distinctions documented
*   [x] Phase 2A report created
*   [x] Any unrun tests/benchmarks clearly marked as unexecuted (All run)

## 7. Phase 2B Recommendations and Remaining Gaps

**Recommendation:**
Proceed to **Phase 2B (Direct Bitwise Operations)**.
The bottleneck is confirmed to be the unpack/repack cycle. The 2-bit representation is verified as semantically correct. The next logical step is to implement `TAnd`, `TOr`, `TXor`, and `TNot` directly on the `std::vector<uint8_t>` data of `ComputeTritVector`, bypassing `to_trits`/`from_trits`.

**Next Steps:**
1.  **Phase 2B:** Implement `t_and`, `t_or`, `t_xor`, `t_not` using bitwise logic on 2-bit packed bytes (SWAR).
    *   Derive boolean logic formulas for the 2-bit mapping (e.g., `res_bit0 = f(a0, a1, b0, b1)`).
    *   Benchmark against Scalar and Phase 1. This is where we expect to see performance exceeding the scalar baseline (due to 4x parallel processing per byte).
2.  **Phase 3:** Investigate SIMD (AVX2) for the 2-bit representation.
3.  **Integration:** If successful, define `ComputeTritVector` as the transient execution format for heavy tritwise workloads, converting from/to PT-5 (canonical storage) at the boundaries.

**Gaps:**
*   The current 2-bit codec is naive and could be optimized, but this is secondary to implementing direct operations.
*   `TXor` (ternary difference) logic in 2-bit arithmetic needs careful derivation to match the wrap-around semantics exactly.
