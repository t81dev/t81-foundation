# Governed Workflow Benchmark Evidence

Date: 2026-03-19  
Status: Active  
Owner: @t81dev

## Scope

This record captures the first operational benchmark slice for governed
workflows in the main `benchmark_runner`.

It focuses on two questions:

- what policy enforcement costs on a matched VM workload
- how a local tensor-load workflow compares to a CanonFS-hash-governed tensor
  load workflow

It does **not** claim that CanonFS overhead has been isolated from all other
factors. The CanonFS workflow currently exercises a different opcode path and a
hash-governed load contract.

## Build Conditions

- benchmark target: `build/benchmarks/benchmark_runner`
- benchmark family: `BM_Governed_*`

## Command

```bash
T81_BENCHMARK_VERBOSE_CONSOLE=1 \
./build/benchmarks/benchmark_runner \
  --benchmark_filter='^BM_Governed' \
  --benchmark_min_time=0.001s
```

## Benchmarks Exercised

- `BM_GovernedVMRun_Arith_NoPolicy`
- `BM_GovernedVMRun_Arith_AllowPolicy`
- `BM_GovernedTensorLoad_LocalWeights`
- `BM_GovernedTensorLoad_CanonFSHash`

## Observed Result

Current local run:

| Benchmark | Observed Result | Notes |
|---|---:|---|
| `BM_GovernedVMRun_Arith_NoPolicy` | `298.18 Kops/s`, `276.75 µs` | arithmetic chain, no policy |
| `BM_GovernedVMRun_Arith_AllowPolicy` | `270.85 Kops/s`, `329.04 µs` | same arithmetic chain, simple allow policy |
| `BM_GovernedTensorLoad_LocalWeights` | `6.12 µs` | local weights-backed tensor materialization, `4096` elements |
| `BM_GovernedTensorLoad_CanonFSHash` | `102.59 µs` | CanonFS hash-governed tensor load, `4096` elements |

## Interpretation

### Policy overhead on a matched workload

The `BM_GovernedVMRun_*` pair is the cleanest comparison in this record because
the workload is held constant and only the simple allow-policy contract is
introduced.

Observed on this host:

- throughput drops from `298.18 Kops/s` to `270.85 Kops/s`
- iteration latency rises from `276.75 µs` to `329.04 µs`

That is roughly:

- `0.91x` throughput
- `1.19x` latency

The main conclusion is narrow:

- simple policy enforcement on this VM arithmetic workload is measurable but
  not catastrophic

### CanonFS-backed workflow comparison

The tensor-load pair is intentionally described as a workflow comparison, not a
pure storage benchmark.

The local weights path and the CanonFS hash-governed path differ in:

- load opcode path
- CanonFS object resolution
- governance conditions

So the current result supports only this statement:

- the current CanonFS-hash-governed tensor load workflow is materially slower
  than the local weights workflow on this host for the exercised `4096`-element
  case

It does **not** prove that CanonFS persistence alone accounts for the full
difference.

## What This Proves

- the benchmark harness can express governance-aware operational comparisons
- policy-on vs policy-off VM execution can be measured directly in-repo
- CanonFS-backed governed tensor loading now has a repeatable benchmark lane

## What This Does Not Prove

- it does not isolate storage overhead from governance and opcode-path effects
- it does not establish a policy-overhead bound for all workloads
- it does not extend DCP / Verified determinism claims

## Known Caveats

- the local run emitted benchmark-runner host metadata warnings on macOS:
  - CPU frequency could not be determined from `sysctl`
  - thread affinity could not be set
- those warnings affect metadata quality, not benchmark execution
- the benchmark runner’s family summary is still more readable in verbose
  console mode than in raw report mode for this benchmark slice

## Recommended Next Measurements

1. Add a matched `TLoadHash` local fixture path to isolate CanonFS persistence
   overhead from opcode-path differences.
2. Add a policy-enabled `WeightsLoad` benchmark to separate policy cost from
   CanonFS cost on tensor materialization.
3. Record multiple tensor sizes in a dedicated evidence table instead of relying
   on the current family summary row.
