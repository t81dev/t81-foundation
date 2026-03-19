# RFC 0041 SIMD Evidence

Date: 2026-03-18
Host: Apple M2
Platform: macOS ARM64 (`Darwin 25.3.0`, `arm64`)

## Scope

This record captures the current RFC-0041 evidence on the available ARM64/NEON
host after promotion of the stable SIMD facade in `include/t81/simd/simd.hpp`.

It is an evidence record for:

- stable API promotion
- backend-equivalence safety
- current ARM64/NEON benchmark behavior
- ARM64 threshold tuning outcome

It is **not** a cross-architecture closeout by itself. A matching refreshed
x86_64 record is still required to fully close the RFC evidence line.

## Verified Test Surface

The following tests passed on this host:

- `t81_packed_trit_vector_test`
- `t81_packed_trit_vector_stress_test`
- `t81_tritwise_backend_equivalence_test`
- `t81_simd_api_test`

Command:

```sh
ctest --test-dir build -R 't81_(packed_trit_vector_test|packed_trit_vector_stress_test|tritwise_backend_equivalence_test|simd_api_test)' --output-on-failure
```

These cover:

- scalar vs SWAR vs SIMD byte identity
- explicit boundary cases and padding integrity
- chained-operation stress behavior
- the promoted `t81::simd` facade

## Benchmark Slice

Focused benchmark command:

```sh
./build/benchmarks/benchmark_runner \
  --benchmark_filter='BM_(ComputeTAnd_Phase2C_SWAR|ComputeTAnd_Phase2D_AVX2|ComputeTOr_Phase2C_SWAR|ComputeTOr_Phase2D_AVX2|ComputeTNot_Phase2C_SWAR|ComputeTNot_Phase2D_AVX2|Kernel_TAnd_SWAR|Kernel_TAnd_NEON)/(256|1024|4096)$' \
  --benchmark_min_time=0.01s \
  --benchmark_format=json \
  --benchmark_out=build/rfc0041_simd_neon_bench_stable.json
```

Selected CPU-time results:

| Benchmark | 256 | 1024 | 4096 |
| :--- | :--- | :--- | :--- |
| `BM_ComputeTAnd_Phase2C_SWAR` | `64.27 ns` | `78.32 ns` | `168.05 ns` |
| `BM_ComputeTAnd_Phase2D_AVX2` (default dispatch on ARM64) | `77.24 ns` | `86.03 ns` | `152.06 ns` |
| `BM_ComputeTOr_Phase2C_SWAR` | `65.09 ns` | `86.25 ns` | `148.57 ns` |
| `BM_ComputeTOr_Phase2D_AVX2` (default dispatch on ARM64) | `65.49 ns` | `75.21 ns` | `134.90 ns` |
| `BM_ComputeTNot_Phase2C_SWAR` | `56.47 ns` | `76.51 ns` | `116.77 ns` |
| `BM_ComputeTNot_Phase2D_AVX2` (default dispatch on ARM64) | `59.17 ns` | `78.25 ns` | `124.61 ns` |
| `BM_Kernel_TAnd_SWAR` | `3.01 ns` | `9.81 ns` | `38.30 ns` |
| `BM_Kernel_TAnd_NEON` | `5.11 ns` | `19.00 ns` | `74.70 ns` |

## Current Interpretation

What this ARM64 record proves:

- The promoted SIMD surface is stable and test-covered.
- NEON and SWAR remain byte-identical on the tested boundary and randomized cases.
- Threshold-based dispatch remains deterministic on ARM64.

What this ARM64 record does **not** prove:

- It does not show a universal NEON speedup over SWAR on this host.
- It does not close the x86_64 side of RFC-0041.

The current ARM64 behavior is mixed:

- `TOr` via default dispatch improves over SWAR at `1024` and `4096`.
- `TAnd` via default dispatch only overtakes SWAR at `4096`.
- `TNot` via default dispatch is still slightly slower than SWAR in this slice.
- The raw `TAnd` NEON kernel remains slower than raw SWAR in this specific benchmark.

## ARM64 Threshold Tuning Outcome

Based on the measured ARM64 behavior, the implementation now uses per-operation
NEON thresholds:

- `TOr`: keep NEON enabled at `64` bytes
- `TAnd`: disable NEON on the current stable ARM64 path
- `TNot`: disable NEON on the current stable ARM64 path

Focused post-tuning probe:

```sh
./build/benchmarks/benchmark_runner \
  --benchmark_filter='BM_(ComputeTAnd_Phase2C_SWAR|ComputeTAnd_Phase2D_AVX2|ComputeTOr_Phase2C_SWAR|ComputeTOr_Phase2D_AVX2|ComputeTNot_Phase2C_SWAR|ComputeTNot_Phase2D_AVX2)/(256|1024|4096|16384|65536)$' \
  --benchmark_min_time=0.01s \
  --benchmark_format=json \
  --benchmark_out=build/rfc0041_threshold_probe_tuned.json
```

Selected post-tuning CPU-time results:

| Benchmark | 256 | 1024 | 4096 | 16384 | 65536 |
| :--- | :--- | :--- | :--- | :--- | :--- |
| `BM_ComputeTAnd_Phase2C_SWAR` | `68.83 ns` | `80.95 ns` | `139.91 ns` | `328.81 ns` | `1184.03 ns` |
| `BM_ComputeTAnd_Phase2D_AVX2` | `71.02 ns` | `83.23 ns` | `201.18 ns` | `518.45 ns` | `1929.37 ns` |
| `BM_ComputeTOr_Phase2C_SWAR` | `67.55 ns` | `80.45 ns` | `145.43 ns` | `344.32 ns` | `1107.79 ns` |
| `BM_ComputeTOr_Phase2D_AVX2` | `61.90 ns` | `72.63 ns` | `128.62 ns` | `319.97 ns` | `1148.24 ns` |
| `BM_ComputeTNot_Phase2C_SWAR` | `66.75 ns` | `78.51 ns` | `117.47 ns` | `229.11 ns` | `791.22 ns` |
| `BM_ComputeTNot_Phase2D_AVX2` | `68.62 ns` | `83.60 ns` | `161.06 ns` | `396.90 ns` | `1681.15 ns` |

Interpretation:

- `TOr` remains the only clear NEON candidate on this ARM64 host.
- `TAnd` and `TNot` should not currently advertise NEON as a throughput win at
  the stable API level on this host.
- The remaining default-path overhead above explicit SWAR is now wrapper and
  dispatch-path cost, not a NEON-kernel claim for `TAnd` or `TNot`.

## Conclusion

RFC-0041 now has:

- a stable public SIMD facade
- migration guidance
- explicit ARM64/NEON evidence for correctness and current performance

The remaining evidence work is:

1. refresh and record the matching x86_64 benchmark/test snapshot
2. decide whether the remaining ARM64 default-path overhead justifies a small
   wrapper-path optimization pass
3. finish the direct-`experimental` compatibility/deprecation wording
