# DPE Benchmark Commands

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [DPE Benchmark Commands](#dpe-benchmark-commands)
  - [Scope](#scope)
  - [Benchmark Families](#benchmark-families)
  - [Build](#build)
  - [Focused DPE Run](#focused-dpe-run)
  - [Verbose Console Run](#verbose-console-run)
  - [Comparative Shape Run](#comparative-shape-run)
  - [Report-Writing Run](#report-writing-run)
  - [Notes](#notes)

<!-- T81-TOC:END -->


Date: 2026-03-19  
Status: Active  
Owner: @t81dev

## Scope

This record captures the currently supported command surface for the in-repo
DPE benchmark family added to `benchmark_runner`.

The DPE benchmark slice is build-gated:

- `T81_ENABLE_DPE=ON`
- `T81_BUILD_BENCHMARKS=ON`

## Benchmark Families

Current benchmark names:

- `BM_DPE_AcceptEpoch_Independent`
- `BM_DPE_TopologicalLevels_Fanout`
- `BM_DPE_CommitEpoch_Linear`
- `BM_DPE_TaskRunner_RunDirect_NoOutputs`
- `BM_DPE_TaskRunner_RunDirect_WithOutputRegion`
- `BM_DPE_ThreadPool_SubmitWait_Independent`
- `BM_DPE_CommitEpoch_PageExpansion`
- `BM_DPE_TaskRunner_PredecessorSnapshotLoad`
- `BM_DPE_EpochHash_SequentialVsPooled`
- `BM_DPE_SequentialEpoch_Independent`
- `BM_DPE_PooledEpoch_Independent`
- `BM_DPE_SequentialEpoch_Chain`
- `BM_DPE_PooledEpoch_Chain`
- `BM_DPE_SequentialEpoch_Fanout`
- `BM_DPE_PooledEpoch_Fanout`

## Build

```bash
cmake -S . -B build -G Ninja -DT81_ENABLE_DPE=ON
cmake --build build --target benchmark_runner
```

## Focused DPE Run

```bash
./build/benchmarks/benchmark_runner \
  --benchmark_filter='^BM_DPE_' \
  --benchmark_min_time=0.001s \
  --benchmark_format=json
```

## Verbose Console Run

```bash
T81_BENCHMARK_VERBOSE_CONSOLE=1 \
./build/benchmarks/benchmark_runner \
  --benchmark_filter='^BM_DPE_' \
  --benchmark_min_time=0.001s
```

## Comparative Shape Run

```bash
T81_BENCHMARK_VERBOSE_CONSOLE=1 \
./build/benchmarks/benchmark_runner \
  --benchmark_filter='^BM_DPE_(SequentialEpoch_(Independent|Chain|Fanout)|PooledEpoch_(Independent|Chain|Fanout))' \
  --benchmark_min_time=0.005s
```

This is the recommended command when the question is whether pooled DPE helps a
given workload class rather than what the raw DPE overhead looks like.

## Report-Writing Run

```bash
T81_BENCHMARK_WRITE_REPORTS=1 \
./build/benchmarks/benchmark_runner \
  --benchmark_filter='^BM_DPE_' \
  --benchmark_min_time=0.001s \
  --benchmark_format=json
```

## Notes

- The benchmark runner may emit host metadata warnings on some macOS systems
  about CPU frequency discovery and thread affinity. Those warnings affect
  metadata quality, not benchmark execution.
- These benchmarks are performance/support evidence only. They do not extend
  DCP / Verified determinism claims.
