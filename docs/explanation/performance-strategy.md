# Performance Architecture Strategy

**Last Updated:** March 18, 2026

This document outlines the performance strategy for the T81 Foundation stack, focusing on balanced ternary arithmetic optimization and tensor backend architecture.

## 1. Deep Optimization of `T81BigInt`

### Status: Completed

We have successfully implemented deep optimization for `T81BigInt` to handle arbitrary-precision balanced ternary arithmetic efficiently.

-   **Algorithmic Improvements**: Karatsuba multiplication is used for large integer products, significantly reducing the complexity compared to standard long multiplication.
-   **SIMD Acceleration**: The implementation utilizes AVX2 intrinsics (where available) to accelerate limb operations, particularly additions and multiplications.
-   **Memory Management**: Allocation is minimized using small-object optimization and custom allocators where beneficial.
-   **Saturation Handling**: Overflow checks are streamlined to avoid expensive `try-catch` blocks in hot paths, using `significant_trits()` checks to predict potential overflows.

These optimizations are benchmarked regularly (see `../reference/benchmarks.md`) and ensure that high-precision arithmetic remains performant even as precision scales.

## 2. Tensor Backend Strategy

### Status: In Progress

Our tensor strategy aims to balance performance with bounded determinism on verified surfaces.

-   **Portable Scalar Parity**: We maintain a portable scalar backend that serves as the "golden reference" for deterministic verification lanes and backend parity checks.
-   **Optimized Backends**: We are developing optimized backends (e.g., AVX-512, NEON, potential GPU/TPU paths) that *must* produce results mathematically identical to the scalar reference.
-   **T729Tensor**: The current implementation (`../../include/t81/tensor`) provides a comprehensive API for dense tensors, supporting broadcasting, slicing, and reduction operations. Future work involves further optimizing these operations for specific hardware targets without compromising determinism.

## 2A. SWAR Dispatch Guidance

### Status: Implemented

RFC 0040 formalizes the middle tier of the ternary compute hierarchy:

-   **Scalar**: correctness baseline and fallback path
-   **SWAR**: preferred packed-trit path for small and medium exact-trit vectors
-   **SIMD**: highest-throughput tier once data size clears the platform threshold

Current guidance:

-   Use `t81::swar::{t_not,t_and,t_or}` for stable API entry points.
-   Use the explicit `*_swar` variants only when tests or benchmarks need to pin the SWAR path.
-   Expect SWAR to be the most efficient deterministic path below the SIMD threshold and for SIMD tail handling.
-   Avoid converting non-`ExactTrit` tensor classes into the SWAR VM opcodes; the VM intentionally traps rather than widening semantics.
-   Treat the experimental `PackedTritVector` methods as compatibility-only entry points during the deprecation window.

## 3. CanonFS Scalability

### Status: Planned

As workload scale increases, CanonFS must handle larger persistent state while maintaining deterministic observability.

-   **Throughput Optimization**: Improving read/write throughput for large datasets without stalling the VM.
-   **Deterministic Observability**: Ensuring that even with high throughput, every state transition is fully auditable and replayable via Axion traces.
-   **Content Addressing**: Leveraging content-addressed storage principles to deduplicate data and ensure integrity.
