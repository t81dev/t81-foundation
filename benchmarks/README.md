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

## Governed workflow benchmarks
The benchmark runner also includes the `BM_Governed_*` family for operational
comparisons across governed VM and tensor-loading workflows.

Current coverage:

- `BM_GovernedVMRun_Arith_NoPolicy`
- `BM_GovernedVMRun_Arith_AllowPolicy`
- `BM_GovernedObservability_Arith_NoPolicy`
- `BM_GovernedObservability_Arith_AllowPolicy`
- `BM_GovernedRender_Arith_NoPolicy`
- `BM_GovernedRender_Arith_AllowPolicy`
- `BM_GovernedEmit_Arith_NoPolicy`
- `BM_GovernedEmit_Arith_AllowPolicy`
- `BM_GovernedCLI_VMTrace_Export`
- `BM_GovernedCLI_VMTrace_Export_Accumulator`
- `BM_GovernedCLI_VMTrace_Export_SystemIntegration`
- `BM_GovernedCLI_VMTrace_Export_WithPolicy`
- `BM_GovernedCLI_VMTrace_Export_NeuralNet`
- `BM_GovernedCLI_AxionLog_JSON`
- `BM_GovernedCLI_CodeRun_WeightsModelHash`
- `BM_GovernedTensorLoad_LocalWeights_NoPolicy`
- `BM_GovernedTensorLoad_LocalWeights_AllowPolicy`
- `BM_GovernedTensorLoad_CanonFSHash_NoPolicy`
- `BM_GovernedTensorLoad_CanonFSHash_AllowPolicy`
- `BM_GovernedTensorLoad_HashFixture_NoPolicy`
- `BM_GovernedTensorLoad_HashFixture_AllowPolicy`

Focused local command:
```bash
./build/benchmarks/benchmark_runner \
  --benchmark_filter='^BM_Governed' \
  --benchmark_min_time=0.001s \
  --benchmark_format=json
```

Verbose console output:
```bash
T81_BENCHMARK_VERBOSE_CONSOLE=1 \
./build/benchmarks/benchmark_runner \
  --benchmark_filter='^BM_Governed' \
  --benchmark_min_time=0.001s
```

Interpretation note:

- `BM_GovernedVMRun_*` is a matched-workload policy-on vs policy-off comparison.
- `BM_GovernedTensorLoad_*_NoPolicy` vs `*_AllowPolicy` measures policy cost
  within a fixed tensor-load path.
- `BM_GovernedTensorLoad_HashFixture_*` exercises the same `TLoadHash` opcode
  against a preloaded in-memory CanonFS fixture, which helps separate
  hash-resolution path cost from persistent storage cost.
- `BM_GovernedObservability_*` materializes a stable signature over `trace` and
  `axion_log` from a completed VM run. This measures observability processing
  cost, not program execution cost.
- `BM_GovernedRender_*` formats replay-safe trace text and compact Axion audit
  JSON from a completed VM run. This is closer to CLI/operator export work than
  the signature-only lane, but still excludes file I/O.
- `BM_GovernedEmit_*` writes those rendered payloads to temp files and flushes
  them, so formatting cost and file-emission cost can be compared separately.
- `BM_GovernedCLI_*` shells out to the built `t81` binary for end-to-end export
  paths, so process startup and CLI orchestration cost are measured separately
  from the in-process emit lane.
  The `vm-trace` lane now includes both a tiny hello-world artifact and a
  looping accumulator artifact plus a larger system-integration artifact to
  avoid overfitting conclusions to one trivial trace, and a policy-file case so
  the subprocess layer includes an explicitly governed export path. It now also
  includes `neural_net.t81` as the first tensor-heavy CLI export profile. The
  family also now includes a CanonFS-backed `code run --weights-model
  sha3-256:...` lane, which measures model-hash resolution and runtime
  execution through the real CLI subprocess path rather than only the in-memory
  tensor benchmarks.
- local-weights vs CanonFS-hash remains a workflow-level path comparison and
  should not be presented as an isolated storage-layer overhead claim because
  the opcode path still differs.

## Reporting
- Benchmark outputs feed `docs/reference/benchmarks.md` in the current workflow.
- Keep benchmark names stable when possible to preserve historical comparability.
