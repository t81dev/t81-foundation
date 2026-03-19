# benchmarks

Benchmark definitions and runner wiring for performance tracking.

## Layout
- `BM_*.cpp`: benchmark suites (SIMD/base81, tensors, CanonFS, kernels)
- `BM_DPE.cpp`: DPE acceptance / topo-order / commit microbenchmarks when `T81_ENABLE_DPE=ON`
- `runner/`: benchmark runner executable entry points and microbench groups
- `benchmark.md`: benchmark notes/reference

## Build and run
```bash
cmake --build build --target benchmark_runner
./build/benchmarks/benchmark_runner
```

By default `benchmark_runner` applies a local smoke profile.

- `T81_BENCHMARK_PROFILE=full` enables the bounded human-usable full profile.
- `T81_BENCHMARK_PROFILE=deep` enables the exhaustive research/nightly profile.
- `T81_BENCHMARK_VERBOSE_CONSOLE=1` enables the console benchmark table.
- `T81_BENCHMARK_WRITE_REPORTS=1` writes the report artifacts.

Or via CLI wrapper:
```bash
t81 benchmark
```

## DPE benchmarks
When `T81_ENABLE_DPE=ON`, the benchmark runner includes the `BM_DPE_*` family.
That family now covers both:

- overhead-oriented DPE microbenchmarks
- comparative sequential-vs-pooled epoch benchmarks for `independent`,
  `chain`, and `fan-out` shapes

Focused local command:
```bash
./build/benchmarks/benchmark_runner \
  --benchmark_filter='^BM_DPE_' \
  --benchmark_min_time=0.001s \
  --benchmark_format=json
```

Verbose console output:
```bash
T81_BENCHMARK_VERBOSE_CONSOLE=1 \
./build/benchmarks/benchmark_runner \
  --benchmark_filter='^BM_DPE_' \
  --benchmark_min_time=0.001s
```

Comparative shape-focused run:
```bash
T81_BENCHMARK_VERBOSE_CONSOLE=1 \
./build/benchmarks/benchmark_runner \
  --benchmark_filter='^BM_DPE_(SequentialEpoch_(Independent|Chain|Fanout)|PooledEpoch_(Independent|Chain|Fanout))' \
  --benchmark_min_time=0.005s
```

## Reporting
- Benchmark outputs feed `docs/reference/benchmarks.md` in the current workflow.
- Keep benchmark names stable when possible to preserve historical comparability.
