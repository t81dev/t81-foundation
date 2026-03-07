# PackedTritVector Phase 2E Proposal

## 1. Objective

Transition the `ComputeTritVector` accelerator from an AVX2-only prototype to an architecture-complete deterministic backend by implementing ARM NEON support, refining dispatch thresholds, and hardening the benchmark infrastructure.

## 2. Scope

*   **NEON Backend:** Implement `kernel_*_neon` for `TAnd`, `TOr`, `TNot` using 128-bit ARM NEON intrinsics.
*   **Threshold Dispatch:** Implement a size-based dispatch mechanism to select between Scalar SWAR and SIMD kernels to avoid regression on small vectors (< 256 trits).
*   **Allocator Awareness:** Introduce an optional `Allocator` interface or integration with a memory pool to reduce overhead for by-value APIs.
*   **Benchmark CI:** Formalize the benchmark suite into a CI job that enforces non-regression thresholds.

## 3. NEON Implementation Plan

The NEON implementation will mirror the logic of Phase 2C SWAR and Phase 2D AVX2, adapted for 128-bit registers (`uint8x16_t`).

*   **Mapping:**
    *   `_mm256_and_si256` -> `vandq_u8`
    *   `_mm256_or_si256` -> `vorrq_u8`
    *   `_mm256_xor_si256` -> `veorq_u8`
    *   `_mm256_slli_epi64` -> `vshlq_n_u64` (or byte-wise shifts if applicable)
*   **Tail Handling:** Similar to AVX2, NEON kernels will process 16-byte chunks (64 trits). Remaining bytes will be delegated to the canonical SWAR fallback.
*   **Parity Testing:** Differential testing against Scalar and AVX2 backends will ensure bit-exact output.
*   **Byte Identity:** The implementation must guarantee identical byte-level output (including padding bits) to preserve the canonical format.

## 4. Threshold Strategy

Benchmarks from Phase 2D indicate a crossover point around 256 trits where SIMD kernel overhead is amortized.

*   **Mechanism:** `kernel_dispatch` will check `len < THRESHOLD`.
*   **Tuning:** Micro-benchmarks will be run on target architectures (x86_64 and ARM64) to determine the optimal threshold `N` (likely ~64 bytes).
*   **Guarantee:** This ensures strictly monotonic performance improvement (or at least non-regression) across the entire size spectrum.

## 5. RFC Alignment Statement

*   **Library-Level Acceleration:** Phase 2D and 2E demonstrate that significant speedups (2x-5x) are achievable via library-level optimization without modifying the TISC ISA.
*   **No ISA Extension:** No new TISC opcodes are required at this stage. The existing `Bit*` opcodes in TISC v1.1.0 are sufficient.
*   **Re-evaluation Criteria:** ISA expansion for packed tritwise operations will only be considered if:
    1.  Profiling confirms a >5% hotspot in real-world workloads that cannot be addressed by the library accelerator.
    2.  The library approach proves insufficient for specific critical paths.
    This aligns with the conservative "Freeze" posture of the TISC v1.1.0 specification.

## 6. Deliverables

1.  **NEON Backend:** [DONE] Complete implementation of `kernel_not_neon`, `kernel_and_neon`, `kernel_or_neon` in `include/t81/experimental/packed_trit_vector.hpp`.
2.  **Tuned Dispatch:** [DONE] `ComputeTritVector` uses size thresholds (`AVX2_THRESHOLD_BYTES = 64`) to select the fastest kernel.
3.  **CI Benchmarks:** [DONE] Automated job `benchmark_packed_trit_vector.yml` enforces <15% regression.
4.  **Extended Determinism Gate:** [DONE] `tests/cpp/test_packed_trit_vector.cpp` updated to validate NEON paths explicitly (guarded by `__ARM_NEON`).
