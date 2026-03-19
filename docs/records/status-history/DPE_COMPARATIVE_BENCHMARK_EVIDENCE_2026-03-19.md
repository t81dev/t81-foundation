# DPE Comparative Benchmark Evidence

Date: 2026-03-19  
Status: Active  
Owner: @t81dev

## Scope

This record captures direct sequential-vs-pooled benchmark evidence for the DPE
epoch execution path across multiple epoch shapes and sizes.

It does **not** claim that DPE is universally faster. It records measured
comparisons on the current host using the in-repo `benchmark_runner`.

## Build Conditions

- `T81_ENABLE_DPE=ON`
- `T81_BUILD_BENCHMARKS=ON`
- benchmark target: `build/benchmarks/benchmark_runner`

## Command

```bash
T81_BENCHMARK_VERBOSE_CONSOLE=1 \
T81_BENCHMARK_WRITE_REPORTS=1 \
./build/benchmarks/benchmark_runner \
  --benchmark_filter='^BM_DPE_(SequentialEpoch_(Independent|Chain|Fanout)|PooledEpoch_(Independent|Chain|Fanout))' \
  --benchmark_min_time=0.01s \
  --benchmark_format=json
```

Captured console log:

- [console.txt](/Users/t81dev/Code/t81-foundation/build/dpe-bench/console.txt)

## Benchmarks Exercised

- `BM_DPE_SequentialEpoch_Independent`
- `BM_DPE_PooledEpoch_Independent`
- `BM_DPE_SequentialEpoch_Chain`
- `BM_DPE_PooledEpoch_Chain`
- `BM_DPE_SequentialEpoch_Fanout`
- `BM_DPE_PooledEpoch_Fanout`

These benchmarks measure matched DPE task bodies and canonical commit behavior
while varying:

- epoch shape:
  - independent
  - chain
  - fan-out
- task count:
  - `4`
  - `16`
  - `64`
- execution mode:
  - sequential execution
  - pooled execution

## Observed Result

Current local runs:

| Shape | Size | Sequential | Pooled | Observed Ratio |
|---|---:|---:|---:|---:|
| Independent | 4 tasks | `228.68 Kops/s` | `129.15 Kops/s` | `0.56x` |
| Independent | 16 tasks | `296.01 Kops/s` | `186.28 Kops/s` | `0.63x` |
| Independent | 64 tasks | `307.40 Kops/s` | `391.84 Kops/s` | `1.27x` |
| Chain | 4 tasks | `74.31 Kops/s` | `75.19 Kops/s` | `1.01x` |
| Chain | 16 tasks | `88.59 Kops/s` | `90.37 Kops/s` | `1.02x` |
| Chain | 64 tasks | `96.62 Kops/s` | `94.26 Kops/s` | `0.98x` |
| Fan-out | 4 tasks | `32.24 Kops/s` | `62.57 Kops/s` | `1.94x` |
| Fan-out | 16 tasks | `71.81 Kops/s` | `75.10 Kops/s` | `1.05x` |
| Fan-out | 64 tasks | `73.49 Kops/s` | `95.64 Kops/s` | `1.30x` |

## Aggregate Category Summary

These local results support a category-based reading rather than a single
"DPE on/off" claim.

| Category | Included Cases | Aggregate Read |
|---|---|---|
| Parallel-friendly workloads | `Independent/64`, `Fan-out/4`, `Fan-out/16`, `Fan-out/64` | pooled DPE averaged about `1.39x` throughput |
| Serialization-dominated workloads | `Chain/4`, `Chain/16`, `Chain/64` | pooled DPE averaged about `1.00x` throughput |
| Mixed profile | all measured rows in this record | pooled DPE averaged about `1.08x` throughput |

Interpretation:

- The mixed-profile average is not a good product claim by itself.
- The useful statement is narrower:
  - pooling helps on parallel-friendly epoch shapes
  - pooling does not materially help serialization-dominated shapes
- For release communication and benchmark dashboards, DPE results should be
  split by workload class rather than rolled into one "overall" speed claim.

Observed interpretation:

- Pooled DPE is not a blanket win.
- Independent epochs only showed a throughput win once the workload was large
  enough to amortize orchestration overhead on this host.
- Chain epochs behaved as expected: little to no benefit from pooling because
  the dependency shape is effectively serialized.
- Fan-out epochs showed the clearest benefit from pooling, especially at small
  and large widths where the parallel level has meaningful width.
- A naive "overall average" would hide the important fact that DPE wins and
  losses cluster by epoch shape.
- Iteration latency as reported by the benchmark runner should still be treated
  cautiously because the runner is optimized for family summaries rather than a
  dedicated DPE speedup report.

## What This Proves

- DPE does not only benchmark as overhead.
- There are in-repo workloads where pooled DPE outperforms the sequential epoch
  path on the current host.
- Dependency shape matters materially to DPE performance.
- Fan-out and sufficiently wide independent epochs can benefit from pooling.
- Chain-shaped epochs should not be expected to show meaningful pooling wins.
- The benchmark harness can express comparative DPE execution modes directly.

## What This Does Not Prove

- It does not establish a universal DPE speedup.
- It does not establish a general break-even threshold beyond these local runs.
- It does not prove wins for diamond-shaped, conflict-heavy, or
  changed-page-heavy epochs.
- It does not extend DCP / Verified determinism claims.

## Known Caveats

- The benchmark runner emitted host metadata warnings on this macOS system:
  - CPU frequency could not be determined via `sysctl`
  - thread affinity could not be enforced
- Those warnings affect metadata quality, not benchmark execution.
- The current runner’s summary table is optimized for family reporting and is
  not yet a dedicated DPE speedup report.

## Recommended Next Measurements

1. Add `diamond` epoch comparisons to complement `chain` and `fan-out`.
2. Record worker-count sweeps for the pooled path.
3. Add break-even analysis for task granularity and changed-page count.
4. Split orchestration cost from canonical commit cost in the comparative view.
