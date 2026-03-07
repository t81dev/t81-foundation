# Latest Benchmark Results

**Last Updated**: 2024-05-22
**Commit**: `HEAD`

## Executive Summary
*   **Tritwise Operations**: Phase 2D (AVX2) shows ~1000x speedup over Phase 1 (Scalar).
*   **VM Dispatch**: Current overhead is ~2ns per instruction.

## Key Metrics

### Tritwise Operations (4096 Trits)

| Implementation | Latency | Speedup vs Baseline |
| :--- | :--- | :--- |
| Phase 1 (Scalar) | 2.00 ms | 1x |
| Phase 2A (Vector) | 1.90 ms | 1.05x |
| Phase 2B (LUT) | 22.19 µs | 90x |
| Phase 2C (SWAR) | 2.09 µs | 950x |
| Phase 2D (AVX2) | 2.06 µs | 970x |
| **Phase 2D (In-Place)** | **1.13 µs** | **1769x** |

### VM Simulation

| Metric | Result |
| :--- | :--- |
| Dispatch Latency | ~2ns |
| Context Switch | ~150ns |

## Raw Data
Raw data files are archived in `/benchmarks/results/`.
