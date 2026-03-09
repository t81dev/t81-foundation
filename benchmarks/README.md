# benchmarks

Benchmark definitions and runner wiring for performance tracking.

## Layout
- `BM_*.cpp`: benchmark suites (SIMD/base81, tensors, CanonFS, kernels)
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

Or via CLI wrapper:
```bash
t81 benchmark
```

## Reporting
- Benchmark outputs feed `docs/reference/benchmarks.md` in the current workflow.
- Keep benchmark names stable when possible to preserve historical comparability.
