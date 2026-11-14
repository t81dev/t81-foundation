# PackedTritVector Prototype Phase 2B Report

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [PackedTritVector Prototype Phase 2B Report](#packedtritvector-prototype-phase-2b-report)
  - [1. Summary](#1-summary)
  - [2. Phase 2A Follow-Through Adjustments](#2-phase-2a-follow-through-adjustments)
    - [Performance Claims](#performance-claims)
    - [Storage vs Compute Representation](#storage-vs-compute-representation)
    - [Benchmark Metadata](#benchmark-metadata)
  - [3. Direct Packed 2-Bit Tritwise Design (LUT-First)](#3-direct-packed-2-bit-tritwise-design-lut-first)
    - [Byte-Lane Mapping](#byte-lane-mapping)
    - [LUT Strategy](#lut-strategy)
    - [Invalid-State Policy](#invalid-state-policy)
  - [4. Implementation of Direct Packed Ops](#4-implementation-of-direct-packed-ops)
    - [Reference Path](#reference-path)
    - [Partial-Byte Handling](#partial-byte-handling)
    - [`TXor` Implementation](#`txor`-implementation)
  - [5. Conformance and Safety Verification](#5-conformance-and-safety-verification)
  - [6. Benchmark Comparisons](#6-benchmark-comparisons)
  - [7. Validation Results](#7-validation-results)
  - [8. Phase 2C Recommendations and Remaining Gaps](#8-phase-2c-recommendations-and-remaining-gaps)

<!-- T81-TOC:END -->


## 1. Summary

Phase 2B successfully implements **direct tritwise operations** on the `ComputeTritVector` 2-bit packed representation, fulfilling the primary goal of eliminating the unpack-operate-repack bottleneck in the hot path.

By utilizing a **LUT-first strategy** (Lookup Table), we achieved correct, semantic-aligned execution for `t_not`, `t_and`, `t_or`, and `t_xor` without requiring complex bitwise algebra (SWAR) in this phase.

**Key Outcome:**
The direct packed implementation delivers a **~37x speedup** over the Phase 2A (naive) implementation. However, it remains significantly slower (~15x) than the scalar baseline, which benefits from compiler auto-vectorization (SIMD) on simple `int8_t` operations. This confirms that while packed execution is the right direction, a scalar byte-loop with LUTs is insufficient to beat modern scalar SIMD; future phases (SWAR/SIMD on packed data) are required for that.

## 2. Phase 2A Follow-Through Adjustments

### Performance Claims
We explicitly state that Phase 2B is an **optimization of the packed compute path**, not yet a replacement for the scalar path in raw throughput. The claim is "material improvement over previous packed prototypes," not "faster than scalar."

### Storage vs Compute Representation
We reaffirm:
*   **PT-5 (`PackedTritVector`)**: Remains the canonical **storage and interchange** format (high density).
*   **2-Bit (`ComputeTritVector`)**: Is the **compute prototype** representation (fast access), used only when operating on vectors.

### Benchmark Metadata
Benchmarks were run in the T81 CI environment (Linux, x86_64).
*   Compiler: GCC/Clang (Standard T81 build).
*   Optimization: `-O3` (Release).
*   SIMD: AVX2 available (and evidently used by scalar baseline).

## 3. Direct Packed 2-Bit Tritwise Design (LUT-First)

### Byte-Lane Mapping
*   **Packing**: 4 trits per byte (2 bits per trit).
*   **Order**: Little-endian within byte (Trit 0 at bits 0-1, Trit 1 at bits 2-3, etc.).
*   **Encoding**:
    *   `0` -> `00`
    *   `1` -> `01`
    *   `-1` -> `11`
    *   `Invalid` -> `10`

### LUT Strategy
To ensure semantic correctness without fragile bit-twiddling, we generate Lookup Tables (LUTs) at startup using the canonical `PackedTritVector::scalar_*` functions as the source of truth.
*   **Unary LUT**: `op_not[256]` maps input byte to output byte.
*   **Binary LUTs**: `op_and[256][256]`, `op_or[256][256]`, `op_xor[256][256]` map input byte pairs to output bytes.
*   **Size**: Total LUT size is ~193 KB, fitting easily into L2/L3 cache.

### Invalid-State Policy
*   **Input Assumption**: Inputs are assumed valid (enforced by `from_trits` / factory methods).
*   **Invalid Detection**: If an invalid lane pattern (`10`) is encountered during LUT generation, the result slot is marked with `0xAA` (10101010), ensuring that invalid inputs produce clearly invalid outputs (garbage-in, garbage-out) rather than silent corruption.
*   **Runtime Check**: We do not perform explicit validity checks in the hot loop to maximize performance, relying on the factory validation and LUT safety.

## 4. Implementation of Direct Packed Ops

The `ComputeTritVector` class now implements `t_not`, `t_and`, `t_or`, and `t_xor` using the `LUTs` singleton.

### Reference Path
The previous implementation (unpack-op-repack) is preserved as `t_*_ref` methods for verification and benchmarking.

### Partial-Byte Handling
Operations process full bytes. For vectors with lengths not divisible by 4, the final byte may contain unused bits (padding).
*   **Policy**: We apply a mask to the final byte after the operation to ensure padding bits remain `0` (canonical state). This prevents garbage accumulation in padding which could affect hashing or comparison.

### `TXor` Implementation
`TXor` is non-commutative (`a - b`). The LUT `op_xor[a][b]` correctly encodes `scalar_xor(trit_a, trit_b)`.

## 5. Conformance and Safety Verification

We extended `tests/cpp/test_packed_trit_vector.cpp` to include:
*   **LUT Equivalence**: Verifying `t_op()` matches `t_op_ref()` exactly for randomized vectors of various lengths.
*   **Edge Lengths**: Validating correctness for lengths 0, 1, 2, 4, 5, 7, 8, etc.
*   **Masking**: Explicitly checking that padding bits in the final byte are cleared after operations.
*   **`TXor` Truth Table**: Re-verified the 9-case truth table on the new implementation.

All tests passed.

## 6. Benchmark Comparisons

We compared the new implementation against the Phase 2A reference and the scalar baseline.

| Operation | Implementation | Time (4096 items) | Speedup vs Phase 2A | Speedup vs Scalar |
| :--- | :--- | :--- | :--- | :--- |
| **TAnd** | Scalar (Baseline) | 0.16 µs | - | 1.0x |
| | Phase 2A (Ref) | 92.28 µs | 1.0x | 0.0017x |
| | **Phase 2B (LUT)** | **2.49 µs** | **37.0x** | **0.06x** |

**Interpretation:**
*   **vs Phase 2A**: The **37x speedup** validates the removal of the unpack/repack overhead. This is a massive improvement for the packed compute path.
*   **vs Scalar**: The scalar implementation is **~15x faster** (0.16 µs vs 2.49 µs). This is attributed to:
    1.  **Auto-Vectorization**: The compiler vectorizes the scalar loop (SIMD), processing 16-32 trits per cycle.
    2.  **LUT Overhead**: The LUT approach uses a scalar loop with indirect memory access (`res = lut[a][b]`), preventing vectorization.
    3.  **Allocation**: The benchmark includes `std::vector` allocation for the result, which is significant at these timescales (though Phase 2A had it too).

## 7. Validation Results

* [x] Phase 2A/2B docs preserve PT-5 vs 2-bit distinction
* [x] Direct packed `t_not`, `t_and`, `t_or`, `t_xor` implemented (no full unpack/repack)
* [x] LUT-first kernel implemented and generated from canonical scalar semantics
* [x] Invalid `10` lane handling policy documented (LUT maps to `0xAA`)
* [x] `TXor` exactness guarded and verified
* [x] Conformance tests cover non-multiple-of-4 lengths and final-lane masking
* [x] Benchmarks compare scalar vs Phase 2A vs Phase 2B
* [x] Performance claims are conservative (acknowledged slower than scalar)
* [x] Phase 2B report created

## 8. Phase 2C Recommendations and Remaining Gaps

**Recommendation:**
To compete with or beat the scalar baseline, Phase 2C must move beyond byte-wise LUTs to **SWAR (SIMD Within A Register)** or **Native SIMD (AVX2/NEON)** on the packed data.
*   **SWAR**: Use bitwise logic (`&`, `|`, `^`) on `uint64_t` words to perform 32 trit operations in parallel, mimicking the scalar SIMD throughput but with higher density.
*   **SIMD**: Use `_mm256_and_si256` etc. directly on packed data.

**Conclusion:**
Phase 2B is a successful intermediate step that stabilizes the correctness of packed operations and removes the most obvious bottleneck. It is ready for Phase 2C optimization.
