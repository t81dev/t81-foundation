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
- what observability materialization costs for a completed governed VM run

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
- `BM_GovernedCLI_CodeRun_WeightsModelHash_WithPolicy`
- `BM_GovernedCLI_CodeRun_WeightsModelHash_DenyPolicy`
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
| `BM_GovernedObservability_Arith_NoPolicy` | `668.55 ns` | stable signature over completed run with `82` trace entries and `6` Axion events |
| `BM_GovernedObservability_Arith_AllowPolicy` | `658.55 ns` | same observability materialization path with simple allow policy |
| `BM_GovernedRender_Arith_NoPolicy` | `17.35 µs` | render `975` trace bytes plus `1411` audit JSON bytes from the same `82`-trace / `6`-event snapshot |
| `BM_GovernedRender_Arith_AllowPolicy` | `17.24 µs` | same render path with simple allow policy |
| `BM_GovernedEmit_Arith_NoPolicy` | `506.80 µs` | write the rendered `975` trace bytes and `1411` audit JSON bytes to temp files and flush |
| `BM_GovernedEmit_Arith_AllowPolicy` | `459.18 µs` | same emit path with simple allow policy |
| `BM_GovernedCLI_VMTrace_Export` | `10.45 ms` | end-to-end `t81 vm trace <artifact> -o <trace>` subprocess path on the tiny hello-world artifact, writing a `118`-byte trace file |
| `BM_GovernedCLI_VMTrace_Export_Accumulator` | `8.20 ms` | same subprocess path on the loop-oriented accumulator artifact, writing a `47`-byte trace file |
| `BM_GovernedCLI_VMTrace_Export_SystemIntegration` | `9.19 ms` | same subprocess path on the richer system-integration artifact, writing a `970`-byte trace file |
| `BM_GovernedCLI_VMTrace_Export_WithPolicy` | `8.47 ms` | same subprocess path on hello-world with `--policy examples/system_integration.apl`, writing a `118`-byte trace file |
| `BM_GovernedCLI_VMTrace_Export_NeuralNet` | `9.94 ms` | same subprocess path on the tensor-heavy neural-net artifact, writing a `2523`-byte trace file |
| `BM_GovernedCLI_AxionLog_JSON` | `12.82 ms` | end-to-end `t81 axion log --json` subprocess path, writing an `893`-byte JSON payload |
| `BM_GovernedCLI_CodeRun_WeightsModelHash` | `9.98 ms` | end-to-end `t81 code run tests/fixtures/t81lang_std_tensor/03_matmul_weights.t81 --weights-model sha3-256:...` with `T81_CANONFS_ROOT` set, using a real `267`-byte `.t81w` stored in CanonFS |
| `BM_GovernedCLI_CodeRun_WeightsModelHash_WithPolicy` | `9.58 ms` | same CanonFS-backed `code run --weights-model` subprocess path with a matched `allowed-ternary-model-hashes` policy authorizing that exact model checksum |
| `BM_GovernedCLI_CodeRun_WeightsModelHash_DenyPolicy` | `10.31 ms` | same CanonFS-backed `code run --weights-model` subprocess path with a mismatched `allowed-ternary-model-hashes` policy; command fails closed and emits a `40`-byte stderr diagnostic |
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

### Observability materialization cost

The `BM_GovernedObservability_*` pair measures a post-run stable signature over
the VM `trace` and `axion_log`, not execution.

For the exercised arithmetic-chain snapshot:

- both runs carry `82` trace entries and `6` Axion events
- materialization cost is about `0.66 µs` in either governance state

The useful reading is:

- once a small governed run has completed, deriving a stable in-process
  observability signature over its current trace/audit state is cheap on this
  host
- this does not measure CLI text rendering, file I/O, or report generation

### Render/export formatting cost

The `BM_GovernedRender_*` pair formats two export-style payloads from the same
completed run:

- replay-safe trace text
- compact Axion audit JSON

For the exercised arithmetic-chain snapshot:

- both runs render `975` bytes of trace text and `1411` bytes of audit JSON
- rendering cost is about `17.3 µs` in either governance state

The useful reading is:

- string formatting/export preparation is materially more expensive than the
  signature-only observability pass
- for this small snapshot, render preparation is still much cheaper than the
  VM execution-path benchmarks and much more representative of CLI export work
  than the hash-only lane
- this still excludes filesystem write cost

### File-emission cost

The `BM_GovernedEmit_*` pair writes those already-rendered payloads to temp
files and flushes the output.

For the exercised arithmetic-chain snapshot:

- both runs emit the same `975` trace bytes and `1411` audit JSON bytes
- write+flush cost is about `459-507 µs`

The useful reading is:

- file emission dominates render-only cost for this small snapshot
- write-path overhead is roughly an order of magnitude above string formatting
  and several orders above the in-memory signature-only lane
- this gives the benchmark stack a clean separation between:
  - signature materialization
  - render formatting
  - write emission

### End-to-end CLI export cost

The `BM_GovernedCLI_*` pair shells out to the built `t81` binary and measures
operator-style export paths instead of only in-process helpers.

Current local result:

- `t81 vm trace <artifact> -o <trace>` on hello-world: about `10.45 ms`
- `t81 vm trace <artifact> -o <trace>` on accumulator: about `8.20 ms`
- `t81 vm trace <artifact> -o <trace>` on system-integration: about `9.19 ms`
- `t81 vm trace <artifact> -o <trace> --policy examples/system_integration.apl`: about `8.47 ms`
- `t81 vm trace <artifact> -o <trace>` on neural-net: about `9.94 ms`
- `t81 axion log --json`: about `12.82 ms`
- `t81 code run ... --weights-model sha3-256:...` with CanonFS-backed `.t81w`: about `9.98 ms`
- same CanonFS-backed `code run --weights-model` path with a matched `allowed-ternary-model-hashes` policy: about `9.58 ms`
- same CanonFS-backed `code run --weights-model` path with a mismatched `allowed-ternary-model-hashes` policy: about `10.31 ms`

The useful reading is:

- CLI/process overhead is substantially larger than the in-process emit lane
- the in-process emit lane is still useful because it isolates formatting and
  write cost from process startup, argument parsing, and broader CLI setup
- end-to-end `vm trace` cost is not explained by final trace-file byte count
  alone; the largest trace payload in this slice (`970` bytes) was not the
  slowest case
- the current simple policy-file case does not dominate the subprocess cost on
  this host; it sits in the same general band as the other `vm trace` runs
- the tensor-heavy neural-net case produces the largest trace payload in the
  current CLI trace set (`2523` bytes) and lands near the high end of the
  runtime band, but still does not justify a simple bytes-to-time model
- the CanonFS-backed `code run --weights-model` path lands in the same
  low-double-digit millisecond band as the CLI export commands, but it covers a
  meaningfully different operator workflow: subprocess startup, source compile,
  CanonFS hash resolution, `.t81w` parse, and runtime tensor execution
- the matched allow-policy variant also lands in that same band; in this short
  local run it measured slightly faster than the no-policy case, which should be
  treated as benchmark noise rather than evidence that policy makes the
  subprocess path faster
- the matched deny-policy variant now also lands in the same band, which is the
  useful operational result: fail-closed rejection for this model-backed CLI
  path is explicit and not dramatically cheaper or more expensive than the
  allow path on this host
- the five-layer stack now gives an honest decomposition from in-memory
  signature work up through end-to-end CLI export

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
- post-run observability materialization over `trace` and `axion_log` can be
  measured directly in-repo
- export-style trace/audit rendering can be measured directly in-repo without
  involving CLI file I/O
- temp-file trace/audit emission can be measured directly in-repo as a separate
  write-cost layer
- end-to-end CLI export cost can be measured directly in-repo as a distinct
  process/orchestration layer
- policy-on vs policy-off tensor loading can be measured directly within a
  fixed path
- `TLoadHash` can now be benchmarked against a non-persistent in-memory fixture
  through the same VM opcode path
- CanonFS-backed tensor loading now has a repeatable benchmark lane

## What This Does Not Prove

- it does not isolate every internal persistence cost from the broader
  persistent-driver path
- it does not establish a policy-overhead bound for all workloads
- it does not measure broader CLI orchestration around argument parsing,
  terminal interactivity, or user-facing TTY rendering beyond the specific
  subprocess commands exercised here
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
2. If stricter isolation is needed, benchmark persistent CanonFS with warm-cache
   and cold-cache splits instead of one blended persistent path.
3. Add a CanonFS-backed or weights-backed CLI export case so the subprocess
   lane includes storage-governed AI/tensor paths, not just general VM traces.
