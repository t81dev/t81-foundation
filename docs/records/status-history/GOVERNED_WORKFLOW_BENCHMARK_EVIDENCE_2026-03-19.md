# Governed Workflow Benchmark Evidence

Date: 2026-03-19  
Status: Active  
Owner: @t81dev

## Scope

This record captures the first operational benchmark slice for governed
workflows in the main `benchmark_runner`.

It focuses on two questions:

- what policy enforcement costs on a matched VM workload
- what policy enforcement costs within each tensor-load path
- how a local tensor-load workflow compares to a CanonFS-hash tensor-load
  workflow at fixed governance

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
- `BM_GovernedTensorLoad_LocalWeights_NoPolicy`
- `BM_GovernedTensorLoad_LocalWeights_AllowPolicy`
- `BM_GovernedTensorLoad_HashFixture_NoPolicy`
- `BM_GovernedTensorLoad_HashFixture_AllowPolicy`
- `BM_GovernedTensorLoad_CanonFSHash_NoPolicy`
- `BM_GovernedTensorLoad_CanonFSHash_AllowPolicy`

## Observed Result

Current local run:

| Benchmark | Observed Result | Notes |
|---|---:|---|
| `BM_GovernedVMRun_Arith_NoPolicy` | `321.29 Kops/s`, `255.63 µs` | arithmetic chain, no policy |
| `BM_GovernedVMRun_Arith_AllowPolicy` | `309.55 Kops/s`, `264.98 µs` | same arithmetic chain, simple allow policy |
| `BM_GovernedTensorLoad_LocalWeights_NoPolicy/4096` | `5.83 µs` | local weights-backed tensor materialization |
| `BM_GovernedTensorLoad_LocalWeights_AllowPolicy/4096` | `6.51 µs` | same local weights path with simple allow policy |
| `BM_GovernedTensorLoad_HashFixture_NoPolicy/4096` | `41.01 µs` | in-memory preloaded hash fixture via `TLoadHash` |
| `BM_GovernedTensorLoad_HashFixture_AllowPolicy/4096` | `41.70 µs` | same in-memory hash path with allowlist policy |
| `BM_GovernedTensorLoad_CanonFSHash_NoPolicy/4096` | `98.29 µs` | persistent CanonFS hash-backed tensor load |
| `BM_GovernedTensorLoad_CanonFSHash_AllowPolicy/4096` | `94.29 µs` | same persistent CanonFS hash path with allowlist policy |

## Interpretation

### Policy overhead on a matched workload

The `BM_GovernedVMRun_*` pair is the cleanest comparison in this record because
the workload is held constant and only the simple allow-policy contract is
introduced.

Observed on this host:

- throughput drops from `321.29 Kops/s` to `309.55 Kops/s`
- iteration latency rises from `255.63 µs` to `264.98 µs`

That is roughly:

- `0.96x` throughput
- `1.04x` latency

The main conclusion is narrow:

- simple policy enforcement on this VM arithmetic workload is measurable but
  not catastrophic

### Tensor-path policy cost

For the `4096`-element tensor case:

- `WeightsLoad` rises from `5.83 µs` to `6.51 µs`
- in-memory `TLoadHash` rises from `41.01 µs` to `41.70 µs`
- persistent CanonFS `TLoadHash` is in the same band, `98.29 µs` vs `94.29 µs`,
  with the current small difference well within the noise one should expect
  from a short local benchmark run

That is roughly:

- `1.12x` latency on the local weights path
- `1.02x` latency on the in-memory hash path

So this record now proves policy overhead can be measured inside each tensor
path separately from the local-vs-CanonFS path difference.

### Hash-path and persistence comparison

The new in-memory hash-fixture path makes the comparison more precise:

- `WeightsLoad` remains the fastest path at about `5.83-6.51 µs`
- in-memory `TLoadHash` sits in the middle at about `41.01-41.70 µs`
- persistent CanonFS `TLoadHash` is slower at about `94.29-98.29 µs`

This supports a cleaner layered reading:

- part of the cost gap is the hash-resolution / `TLoadHash` path itself
- an additional large slice comes from using the persistent CanonFS backing

The local weights path and the hash paths are still different opcode paths, so
this remains an operational comparison, not a claim that one specific internal
function accounts for the full delta.

So the current result supports only this statement:

- the current persistent CanonFS hash tensor-load workflow is materially slower
  than both the local weights path and the matched in-memory hash path on this
  host for the exercised `4096`-element case

It does **not** prove that CanonFS persistence alone accounts for the full
difference.

## What This Proves

- the benchmark harness can express governance-aware operational comparisons
- policy-on vs policy-off VM execution can be measured directly in-repo
- policy-on vs policy-off tensor loading can be measured directly within a
  fixed path
- `TLoadHash` can now be benchmarked against a non-persistent in-memory fixture
  through the same VM opcode path
- CanonFS-backed tensor loading now has a repeatable benchmark lane

## What This Does Not Prove

- it does not isolate every internal persistence cost from the broader
  persistent-driver path
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

1. Record the `4` and `256` tensor rows in a dedicated evidence table instead
   of only carrying the `4096` representative row here.
2. Add governed trace-generation and policy-audit benchmarks to complement the
   current execution-path slice.
3. If stricter isolation is needed, benchmark persistent CanonFS with warm-cache
   and cold-cache splits instead of one blended persistent path.
