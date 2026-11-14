# PackedTritVector Phase 2D Implementation Plan

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [PackedTritVector Phase 2D Implementation Plan](#packedtritvector-phase-2d-implementation-plan)
  - [1. Objective](#1-objective)
  - [2. Scope & Constraints](#2-scope-&-constraints)
    - [2.1 In-Scope](#21-in-scope)
    - [2.2 Out-of-Scope / Excluded](#22-out-of-scope--excluded)
  - [3. Implementation Tasks](#3-implementation-tasks)
    - [3.1 Task A: Allocator Reuse & In-Place APIs](#31-task-a-allocator-reuse-&-in-place-apis)
    - [3.2 Task B: Native SIMD Kernels (AVX2)](#32-task-b-native-simd-kernels-avx2)
    - [3.3 Task C: Architecture Dispatch](#33-task-c-architecture-dispatch)
  - [4. Integration Thresholds (The "Gate")](#4-integration-thresholds-the-"gate")
  - [5. Benchmark Matrix](#5-benchmark-matrix)
  - [6. Risk Management](#6-risk-management)

<!-- T81-TOC:END -->


**Status:** Planned / Ready for Implementation
**Pre-requisite:** Phase 2C (Completed)
**Goal:** Native SIMD optimization and zero-alloc compute paths.

## 1. Objective
Achieve decisive performance superiority over scalar auto-vectorization by implementing explicit native SIMD (AVX2/NEON) intrinsics and allocation-free compute APIs for the `ComputeTritVector` prototype.

## 2. Scope & Constraints

### 2.1 In-Scope
*   **Native SIMD:** Implementation of `TNot`, `TAnd`, `TOr` using platform-specific intrinsics (AVX2 for x86_64, NEON for ARM64).
*   **Zero-Alloc API:** New interfaces for in-place mutation and pre-allocated buffer usage to eliminate `std::vector` overhead.
*   **Dispatch:** Runtime or compile-time selection of the fastest available path (SIMD > SWAR > Scalar/LUT).

### 2.2 Out-of-Scope / Excluded
*   **TXor Optimization:** `TXor` (Ternary Difference) remains on the Phase 2B LUT fallback path or Phase 2C SWAR path (if applicable) due to semantic risk. No new SIMD derivation for `TXor` is permitted without a formal proof of equivalence.
*   **Storage Format Changes:** The 2-bit packed representation is frozen.

## 3. Implementation Tasks

### 3.1 Task A: Allocator Reuse & In-Place APIs
The current API returns `Result<ComputeTritVector>`, enforcing a new allocation per op.
*   **Action:** Add in-place mutation methods:
    *   `void t_not_inplace();`
    *   `void t_and_inplace(const ComputeTritVector& other);`
    *   `void t_or_inplace(const ComputeTritVector& other);`
*   **Action:** Add a raw kernel API for advanced usage:
    *   `static void kernel_and(const uint8_t* a, const uint8_t* b, uint8_t* out, size_t byte_len);`

### 3.2 Task B: Native SIMD Kernels (AVX2)
*   **Action:** Port Phase 2C SWAR logic to AVX2 `__m256i` intrinsics.
    *   Utilize `_mm256_and_si256`, `_mm256_or_si256`, `_mm256_xor_si256`, `_mm256_slli_epi64` (or similar shifts).
*   **Action:** Implement masked load/store for non-aligned tails, or fall back to SWAR/Scalar for tails.

### 3.3 Task C: Architecture Dispatch
*   **Action:** Integrate with `T81_IS_X86_64` and `src/simd/` organization.
*   **Action:** Ensure `ComputeTritVector` calls the optimized kernel when `T81_AVX2_ENABLED` is defined.

## 4. Integration Thresholds (The "Gate")

For Phase 2D to be merged, it must satisfy:

| Criteria | Metric | Target |
| :--- | :--- | :--- |
| **Correctness** | `test_phase2c_truth_table` | 100% Pass (Identical to Scalar Ref) |
| **Safety** | ASAN / UBSAN | Clean (0 errors) |
| **Speedup (Large)** | vs Phase 2C SWAR (>1KB) | **> 1.3x** |
| **Overhead (Small)** | vs Phase 2C SWAR (<64B) | **No Regression** (use fallback if needed) |
| **Portability** | Build on non-SIMD platforms | Compiles & Passes tests (via fallback) |

## 5. Benchmark Matrix

The following benchmarks must be reported in the Phase 2D PR:

1.  **Pure Compute:** `BM_Kernel_AVX2` vs `BM_Kernel_SWAR` (measures raw throughput, ignoring allocation).
2.  **API Level:** `BM_InPlace_AVX2` vs `BM_ByValue_SWAR` (measures API improvement).
3.  **Real Workload:** `BM_RealWorkload` (chained ops) with new in-place API.
4.  **Scaling:** Vector sizes: 16, 64, 256, 1024, 4096, 65536 trits.

## 6. Risk Management

*   **Semantic Drift:** SIMD instructions must treat the 2-bit representation exactly as the SWAR logic does. The `(pad)` state `10` must be handled safely (or proven irrelevant).
*   **TXor:** Do not touch `TXor`. The complexity of non-commutative difference in SIMD is high risk. Correctness is prioritized over speed for this operator.
