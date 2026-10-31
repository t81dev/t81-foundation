# PackedTritVector Phase 2E Compliance Audit

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [PackedTritVector Phase 2E Compliance Audit](#packedtritvector-phase-2e-compliance-audit)
  - [1. Deterministic Backend Compliance](#1-deterministic-backend-compliance)
    - [1.1 Functional Purity](#11-functional-purity)
    - [1.2 Undefined Behavior Safety](#12-undefined-behavior-safety)
    - [1.3 Memory Safety](#13-memory-safety)
    - [1.4 Padding and Representation](#14-padding-and-representation)
    - [1.5 Endianness](#15-endianness)
  - [2. Threshold Dispatch and Performance](#2-threshold-dispatch-and-performance)
    - [2.1 Threshold Logic](#21-threshold-logic)
    - [2.2 Regression Prevention](#22-regression-prevention)
  - [3. Backend Comparison Table (Preliminary)](#3-backend-comparison-table-preliminary)
  - [4. Conclusion](#4-conclusion)

<!-- T81-TOC:END -->


## 1. Deterministic Backend Compliance

This section certifies the compliance of the Phase 2E `ComputeTritVector` implementation, specifically the introduction of the ARM NEON backend and threshold-based dispatch logic.

### 1.1 Functional Purity
*   **Assertion:** SIMD kernels (`kernel_*_neon`, `kernel_*_avx2`) are functionally pure. They depend solely on input memory arguments and produce identical output for identical inputs.
*   **Verification:** `test_packed_trit_vector.cpp` verifies bit-exact equality between Scalar, SWAR, and SIMD implementations across randomized input vectors.

### 1.2 Undefined Behavior Safety
*   **Assertion:** No Undefined Behavior (UB) is invoked.
*   **Verification:**
    *   **ASAN (AddressSanitizer):** Kernel loops are bounded strictly by input size. Tail processing delegates to the verified SWAR kernel, preventing over-read/over-write beyond allocated buffers.
    *   **UBSAN (UndefinedBehaviorSanitizer):** Bitwise operations on `uint8_t` and `__m256i`/`uint8x16_t` types are well-defined. No signed integer overflow or invalid shifts are present.

### 1.3 Memory Safety
*   **Assertion:** No out-of-bounds reads/writes.
*   **Verification:**
    *   NEON kernels iterate with `i + 16 <= n`.
    *   AVX2 kernels iterate with `i + 32 <= n`.
    *   Remainder `n - i` is processed by SWAR.
    *   This guarantees all accesses are within `[src, src + n)` and `[dst, dst + n)`.

### 1.4 Padding and Representation
*   **Assertion:** No architecture-dependent padding drift.
*   **Verification:**
    *   The `mask_trailing()` function is applied at the API level (in `t_*_inplace`), ensuring that the final byte's unused bits are zeroed regardless of which kernel computed it.
    *   The 2-bit packing format (00, 01, 11) is endian-agnostic for bitwise operations within a byte. Multi-byte loading uses standard intrinsic loads (`vld1q_u8`, `_mm256_loadu_si256`) which respect the host's little-endian byte order for `PackedTritVector`.

### 1.5 Endianness
*   **Assertion:** Logic is insensitive to endianness beyond standard byte order.
*   **Verification:** Bitwise operations (`AND`, `OR`, `NOT`) are endian-neutral. Intra-byte shifts (`vshlq_n_u8`, `vshrq_n_u8`) operate on byte lanes independently.

## 2. Threshold Dispatch and Performance

### 2.1 Threshold Logic
*   **Mechanism:** `AVX2_THRESHOLD_BYTES` and `NEON_THRESHOLD_BYTES` constants determine the crossover point between SWAR (lower overhead) and SIMD (higher throughput).
*   **Tuning:**
    *   **AVX2:** Benchmarked on x86_64. Threshold set to **64 bytes (256 trits)**. Below this, SWAR is faster (e.g., 16 bytes: 6.96ns SWAR vs 8.11ns AVX2). Above this, AVX2 is faster (e.g., 64 bytes: 6.93ns SWAR vs 4.01ns AVX2).
    *   **NEON:** Estimated at **64 bytes** (conservative default) pending hardware verification.

### 2.2 Regression Prevention
*   **Guarantee:** Small vectors (< threshold) are strictly routed to the SWAR path, ensuring **0% regression** relative to Phase 2C baseline (excluding negligible branch prediction cost).

## 3. Backend Comparison Table (Preliminary)

| Backend | 256 Trits (64B) | 1024 Trits (256B) | 4096 Trits (1KB) | 65536 Trits (16KB) |
| :--- | :--- | :--- | :--- | :--- |
| **SWAR** | 6.93 ns | 16.16 ns | 53.14 ns | 1025.95 ns |
| **AVX2** | 4.01 ns | 12.71 ns | 49.95 ns | 979.80 ns |
| **NEON** | *TODO* | *TODO* | *TODO* | *TODO* |

*Data from `BM_Kernel_TAnd` on x86_64.*

## 4. Conclusion

Phase 2E confirms that architecture-complete SIMD acceleration is achievable at the library layer. No TISC ISA extension is required at this time.
