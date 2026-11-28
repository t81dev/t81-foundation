# PackedTritVector Prototype Phase 1 Report

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [PackedTritVector Prototype Phase 1 Report](#packedtritvector-prototype-phase-1-report)
  - [1. Summary](#1-summary)
  - [2. Prototype Placement and Reused Components](#2-prototype-placement-and-reused-components)
  - [3. Prototype API and Semantics](#3-prototype-api-and-semantics)
  - [4. Conformance Test Harness](#4-conformance-test-harness)
  - [5. Benchmark Scaffold](#5-benchmark-scaffold)
  - [6. Validation Results](#6-validation-results)
  - [7. Known Limitations and Phase 2 Recommendations](#7-known-limitations-and-phase-2-recommendations)

<!-- T81-TOC:END -->


## 1. Summary

This report documents the Phase 1 prototype of `PackedTritVector`, a library-level implementation of packed tritwise operations. The goal was to establish a correctness baseline and benchmark scaffold using the existing PT-5 codec (5 trits/byte) without modifying the TISC ISA.

The prototype implements `TAnd`, `TOr`, `TXor`, and `TNot` using an "unpack-operate-repack" strategy to ensure strict semantic conformance with scalar ternary logic.

## 2. Prototype Placement and Reused Components

*   **Module Path:** `include/t81/experimental/packed_trit_vector.hpp`
*   **Reused Codec APIs:** `t81::codec::trit_packing` (specifically `pack_pt5` and `unpack_pt5`)
*   **Semantics Source:** `src/vm/vm.cpp` (canonical TISC opcode implementation) and scalar logic defined in `PackedTritVector` for testing.
*   **TXor Semantics:** `TXor` is semantically defined as ternary difference (`lhs - rhs`) with wrapping, exactly matching the TISC VM implementation (`src/vm/vm.cpp`). Note that this is **non-commutative** and distinct from binary XOR. For example, `0 TXor 1` is -1, while `1 TXor 0` is 1.

## 3. Prototype API and Semantics

The `PackedTritVector` class provides the following API:

*   **Construction:**
    *   `static Result<PackedTritVector> from_trits(const std::vector<int8_t>& trits)`: Creates from logical trits. Validates values are {-1, 0, 1}.
    *   `static Result<PackedTritVector> from_packed(const std::vector<uint8_t>& packed, size_t trit_count)`: Creates from raw PT-5 bytes.

*   **Accessors:**
    *   `size_t size()`: Returns number of trits.
    *   `Result<std::vector<int8_t>> to_trits()`: Unpacks to logical trits.
    *   `const std::vector<uint8_t>& packed_data()`: Access raw bytes.

*   **Operations:**
    *   `Result<PackedTritVector> t_not() const`: Ternary NOT.
    *   `Result<PackedTritVector> t_and(const PackedTritVector& other) const`: Ternary AND (Min).
    *   `Result<PackedTritVector> t_or(const PackedTritVector& other) const`: Ternary OR (Max).
    *   `Result<PackedTritVector> t_xor(const PackedTritVector& other) const`: Ternary XOR (Difference modulo 3, non-commutative).

*   **Error Handling:** Uses `t81::Result` to report invalid inputs, length mismatches, or packing errors.

## 4. Conformance Test Harness

A dedicated test suite (`tests/cpp/test_packed_trit_vector.cpp`) verifies correctness:

*   **Fixed Cases:** Validates roundtrip packing/unpacking and basic logic tables.
*   **Scalar Reference:** Compares packed results against a reference scalar implementation derived from TISC semantics.
*   **Randomized Tests:** Uses deterministically seeded random vectors (seed 42) of various lengths (0 to 100) to ensure robustness.
*   **Error Coverage:** Verifies handling of invalid trit values and length mismatches.

## 5. Benchmark Scaffold

A Google Benchmark scaffold (`benchmarks/BM_PackedTritVector.cpp`) compares scalar vs. packed operations across sizes 16, 64, 512, and 4096 trits.

**Methodology:**
*   **Build Mode:** Default CMake configuration (Release/RelWithDebInfo recommended for results).
*   **Framework:** Google Benchmark.
*   **Metrics:** Wall time (ns) and CPU time (ns).
*   **Inclusions:** Phase 1 packed operation timings are inclusive of the full unpack-operate-repack cycle, as per the current prototype strategy.

**Scenarios:**
*   `BM_ScalarTAnd/TOr/TXor/TNot`: Naive loop over `std::vector<int8_t>`.
*   `BM_PackedTAnd/TOr/TXor/TNot`: `PackedTritVector` operations (unpack -> op -> repack).
*   `BM_PackPT5/UnpackPT5`: Isolation of packing/unpacking costs.

**Representative Results (Size 4096):**
*   Scalar TAnd: ~150 ns
*   Packed TAnd: ~50,000 ns (dominated by pack/unpack)
*   Unpack PT-5: ~16,000 ns
*   Pack PT-5: ~12,000 ns

**Observations:** The Phase 1 "unpack-operate-repack" strategy is significantly slower than scalar operations due to the heavy cost of the PT-5 codec. This confirms the need for optimization in Phase 2 (e.g., operating directly on packed bytes or using a more compute-friendly format).

## 6. Validation Results

*   [x] Prototype API implemented (construct/import/export/size/ops)
*   [x] PT-5 backing path used
*   [x] Scalar reference path implemented and identified
*   [x] `TAnd/TOr/TXor/TNot` semantics source identified and honored
*   [x] Conformance tests added (fixed + randomized + error cases)
*   [x] Benchmark scaffold added (with sizes and reporting fields)
*   [x] Prototype report created
*   [x] Tests/benchmarks execution status explicitly reported (all run)

## 7. Known Limitations and Phase 2 Recommendations

**Limitations:**
*   **Performance:** The current implementation relies on full unpacking and repacking for every operation, making it orders of magnitude slower than scalar arithmetic.
*   **Memory:** Creates intermediate vectors during operations.
*   **Semantics:** `TXor` is non-commutative (difference), which is TISC-compliant but potentially counter-intuitive.

**Phase 2 Recommendations:**
*   **Direct Packed Operations:** Investigate implementing `TAnd`, `TOr`, `TXor` directly on PT-5 bytes using lookup tables or bitwise logic, if mathematically possible.
*   **Compute-Friendly Representation:** Evaluate alternative packing formats (e.g., 2-bit unpacked trits or SIMD-aligned formats) for the execution path, keeping PT-5 for storage.
*   **SIMD Acceleration:** Leverage AVX2/NEON for bulk trit operations.
