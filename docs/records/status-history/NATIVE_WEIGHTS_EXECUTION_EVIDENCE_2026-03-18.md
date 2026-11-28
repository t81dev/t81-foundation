# Native Weights Execution Evidence Snapshot

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Native Weights Execution Evidence Snapshot](#native-weights-execution-evidence-snapshot)
  - [Scope](#scope)
  - [Verification](#verification)
  - [Interpretation](#interpretation)

<!-- T81-TOC:END -->


Status: Active
Date: 2026-03-18
Owner: @t81dev

## Scope

Focused evidence for the native `.t81w` execution path after adding:

- deterministic native weights serialization checks
- native-only benchmark registration in `benchmark_runner`
- reporter support for native-only rows

This snapshot is intentionally native-only. It does not claim direct parity
with `llama.cpp` prompt-runtime metrics.

## Verification

Commands run locally on Darwin ARM64:

```sh
cmake --build build --target t81_native_weights_metrics_test benchmark_runner -j4
./build/t81_native_weights_metrics_test
env T81_BENCHMARK_PROFILE=deep T81_BENCHMARK_VERBOSE_CONSOLE=1 \
  ./build/benchmarks/benchmark_runner \
  --benchmark_filter='BM_NativeWeightsLoad_T81Native/64$|BM_NativeWeightsLoadAndExpand_T81Native/64$' \
  --benchmark_min_time=0.00001s
cmake --build build-llama --target benchmark_runner -j4
env T81_BENCHMARK_PROFILE=deep T81_BENCHMARK_VERBOSE_CONSOLE=1 \
  ./build-llama/benchmarks/benchmark_runner \
  --benchmark_filter='BM_(NativeWeightsDecode(AndExp)?_T81Native|LlamaGguf(Dequantize|DequantizeAndExp)_Binary)/8192$' \
  --benchmark_min_time=0.00001s
env T81_BENCHMARK_PROFILE=deep T81_BENCHMARK_VERBOSE_CONSOLE=1 \
  ./build-llama/benchmarks/benchmark_runner \
  --benchmark_filter='BM_(NativeWeightsDecodeAndExp_T81Native|NativeWeightsPackedExp_T81Native|NativeWeightsPackedExpRawFloat_T81Native|LlamaGgufDequantizeAndExp_Binary)/(8192|65536)$' \
  --benchmark_min_time=0.00001s
./build/t81_vm_tensor_test
env T81_BENCHMARK_PROFILE=deep T81_BENCHMARK_VERBOSE_CONSOLE=1 \
  ./build-llama/benchmarks/benchmark_runner \
  --benchmark_filter='BM_NativeWeightsLoadAndExp_T81Native/64$' \
  --benchmark_min_time=0.00001s
env T81_BENCHMARK_PROFILE=deep T81_BENCHMARK_VERBOSE_CONSOLE=1 \
  ./build-llama/benchmarks/benchmark_runner \
  --benchmark_filter='BM_NativeWeightsLoadAndSiLU_T81Native/64$' \
  --benchmark_min_time=0.00001s
env T81_BENCHMARK_PROFILE=deep T81_BENCHMARK_VERBOSE_CONSOLE=1 \
  ./build-llama/benchmarks/benchmark_runner \
  --benchmark_filter='BM_NativeWeightsLoadAndSoftmax_T81Native/64$' \
  --benchmark_min_time=0.00001s
env T81_BENCHMARK_PROFILE=deep T81_BENCHMARK_VERBOSE_CONSOLE=1 \
  ./build-llama/benchmarks/benchmark_runner \
  --benchmark_filter='BM_NativeWeightsLoadAnd(Exp|SiLU|Softmax)_(T81Native|Binary)/64$' \
  --benchmark_min_time=0.00001s
env T81_BENCHMARK_PROFILE=deep T81_BENCHMARK_VERBOSE_CONSOLE=1 \
  ./build-llama/benchmarks/benchmark_runner \
  --benchmark_filter='BM_NativeWeightsLoadAnd(RMSNorm|Exp|SiLU|Softmax)_(T81Native|Binary)/64$' \
  --benchmark_min_time=0.00001s
env T81_BENCHMARK_PROFILE=deep T81_BENCHMARK_VERBOSE_CONSOLE=1 \
  ./build-llama/benchmarks/benchmark_runner \
  --benchmark_filter='BM_NativeWeightsLoadAnd(RMSNorm|RoPE|Exp|SiLU|Softmax)_(T81Native|Binary)/64$' \
  --benchmark_min_time=0.00001s
env T81_BENCHMARK_PROFILE=deep T81_BENCHMARK_VERBOSE_CONSOLE=1 \
  ./build-llama/benchmarks/benchmark_runner \
  --benchmark_filter='BM_NativeWeightsLoadAndTWEMBED_(T81Native|Binary)/64$' \
  --benchmark_min_time=0.00001s
env T81_BENCHMARK_PROFILE=deep T81_BENCHMARK_VERBOSE_CONSOLE=1 \
  ./build-llama/benchmarks/benchmark_runner \
  --benchmark_filter='BM_NativeWeightsLoadAndTWMATMUL_(T81Native|Binary)/(64|256|4096)$' \
  --benchmark_min_time=0.00001s
env T81_BENCHMARK_PROFILE=deep T81_BENCHMARK_VERBOSE_CONSOLE=1 \
  ./build-llama/benchmarks/benchmark_runner \
  --benchmark_filter='BM_NativeWeightsLoadAndTATTN_(T81Native|Binary)/256$' \
  --benchmark_min_time=0.00001s
env T81_BENCHMARK_PROFILE=deep T81_BENCHMARK_VERBOSE_CONSOLE=1 \
  ./build-llama/benchmarks/benchmark_runner \
  --benchmark_filter='BM_NativeWeightsLoadAndTQUANT_(T81Native|Binary)/64$' \
  --benchmark_min_time=0.00001s
env T81_BENCHMARK_PROFILE=deep T81_BENCHMARK_VERBOSE_CONSOLE=1 \
  ./build-llama/benchmarks/benchmark_runner \
  --benchmark_filter='BM_NativeWeightsLoadAndTACT_(T81Native|Binary)/64$' \
  --benchmark_min_time=0.00001s
env T81_BENCHMARK_PROFILE=deep T81_BENCHMARK_VERBOSE_CONSOLE=1 \
  ./build-llama/benchmarks/benchmark_runner \
  --benchmark_filter='BM_NativeWeightsLoadAndTERNACCUM_(T81Native|Binary)/64$' \
  --benchmark_min_time=0.00001s
```

Observed results:

- `t81_native_weights_metrics_test`: pass
- `BM_NativeWeightsLoad_T81Native/64`: `372.09 Kops/s`, `172.67 µs`
- `BM_NativeWeightsPromote_T81Native/64`: `37.42 ops/s`, `1.81 s`
- `BM_NativeWeightsLoadAndExp_T81Native/64`: `15.59 ops/s`, `4.24 s`
- `BM_LlamaGgufDequantize_Binary/8192`: `118.72 Mops/s`
- `BM_LlamaGgufDequantizeAndExp_Binary/8192`: `409.60 Mops/s`
- `BM_NativeWeightsDecode_T81Native/8192`: `41.58 Mops/s`, `196.79 µs`
- `BM_NativeWeightsDecodeAndExp_T81Native/8192`: `90.22 Kops/s`, `228.99 ms`
- `BM_NativeWeightsPackedExp_T81Native/8192`: about `85-97 Kops/s`, `100-195 ms`
- `BM_NativeWeightsPackedExpRawFloat_T81Native/8192`: `372.36 Mops/s`, `23.13 µs`
- `BM_LlamaGgufDequantizeAndExp_Binary/65536`: `348.60 Mops/s`, `188.33 µs`
- `BM_NativeWeightsDecodeAndExp_T81Native/65536`: `643.54 Kops/s`, `209.14 ms`
- `BM_NativeWeightsPackedExp_T81Native/65536`: `671.21 Kops/s`, `243.58 ms`
- `BM_NativeWeightsPackedExpRawFloat_T81Native/65536`: `407.06 Mops/s`, `160.37 µs`
- `t81_vm_tensor_test`: pass
- `BM_NativeWeightsLoadAndExp_T81Native/64`: `696.30 ops/s`, `115.54 ms`
- `BM_NativeWeightsLoadAndSiLU_T81Native/64`: `784.42 ops/s`, `93.37 ms`
- `BM_NativeWeightsLoadAndSoftmax_T81Native/64`: `711.95 ops/s`, `92.08 ms`
- Paired native-vs-binary VM rows at `64`:
  - `BM_NativeWeightsLoadAndExp`: native `820.52 ops/s`, binary `13.14 ops/s`
    (`62.43x` throughput, `62.60x` latency)
  - `BM_NativeWeightsLoadAndSiLU`: native `867.27 ops/s`, binary `12.92 ops/s`
    (`67.12x` throughput, `68.36x` latency)
  - `BM_NativeWeightsLoadAndSoftmax`: native `866.34 ops/s`, binary `13.68 ops/s`
    (`63.34x` throughput, `64.07x` latency)
- Extended higher-level VM pair at `64`:
  - `BM_NativeWeightsLoadAndRMSNorm`: native `226.04 ops/s`, binary `18.85 ops/s`
    (`11.99x` throughput, `11.65x` latency)
  - `BM_NativeWeightsLoadAndRoPE`: after RoPE coefficient caching in the
    generic and native paths, native `81.34 ops/s`, binary `19.96 ops/s`
    (`4.07x` throughput, `4.20x` latency)
  - `BM_NativeWeightsLoadAndTWEMBED`: native `265.56 Kops/s`, binary `345.05 ops/s`
    (`769.63x` throughput, `923.86x` latency)
  - `BM_NativeWeightsLoadAndTWMATMUL`:
    - at `64`: native `376.47 Kops/s`, binary `450.70 Kops/s`
      (`0.84x` throughput, `0.84x` latency)
    - at `256`: native `5.22 Mops/s`, binary `441.38 Kops/s`
      (`11.84x` throughput, `12.00x` latency)
    - at `4096`: native `61.13 Mops/s`, binary `107.17 Kops/s`
      (`570.42x` throughput, `642.25x` latency)
  - `BM_NativeWeightsLoadAndTATTN`:
    - at `256`: native `1.27 Mops/s`, binary `11.48 ops/s`
      (`110970.48x` throughput, `124317.13x` latency)
  - `BM_NativeWeightsLoadAndTQUANT`:
    - at `64`: native `309.18 Kops/s`, binary `49.39 ops/s`
      (`6259.43x` throughput, `7239.39x` latency)
  - `BM_NativeWeightsLoadAndTACT`:
    - at `64`: native `378.70 Kops/s`, binary `55.41 ops/s`
      (`6834.34x` throughput, `7603.96x` latency)
  - `BM_NativeWeightsLoadAndTERNACCUM`:
    - at `64`: native `183.91 Kops/s`, binary `2.28 Kops/s`
      (`80.57x` throughput, `88.49x` latency)

## Interpretation

- The native weights handle-load path is measurable and now visible through the
  benchmark runner as a first-class `T81Native` family.
- The dominant cost is not handle load. It is tensor materialization from the
  native packed representation, and then deterministic exponentiation on the
  materialized tensor.
- The split benchmark shows `TExp` is expensive, but it is expensive on top of
  an already-costly promote/materialize step. The optimization target is
  therefore the promoted-tensor path as a whole, not just the raw `exp`
  implementation in isolation.
- The stronger current claims remain substrate claims:
  deterministic native artifact emission, ternary packing density, governed
  profile admission, and measurable native execution paths.
- After removing eager float-cache construction from canonical-fixed tensor
  creation, native packed-weight decode is now measurable in the same
  release-mode harness as the llama.cpp baseline rather than timing out.
- That decode path is still slower than llama.cpp GGUF dequantization at the
  current 8192-element substrate comparison, but it is now within the same
  order of magnitude.
- A benchmark-only direct packed-native `exp` path that still returns a normal
  canonical-fixed tensor does not materially improve throughput over the current
  `decode()` + `ops::exp()` route. Bypassing decode alone is therefore not the
  correct optimization target.
- A benchmark-only direct packed-native `exp` path that writes only a raw float
  buffer is dramatically faster and, at 65536 elements, slightly faster than
  the current llama.cpp dequantize+`exp` baseline in the same `Release` build.
  That isolates the dominant remaining cost to result representation and tensor
  construction, not to trit classification or exponent lookup.
- Narrow VM fast paths for native unary ops on `BalancedTernary`
  `WeightsTensorHandle` values, combined with a host-float tensor factory that
  avoids eager canonical cache construction, materially improved the real
  interpreter path:
  `BM_NativeWeightsLoadAndExp_T81Native/64` moved from `15.59 ops/s`
  (`4.24 s`) to `696.30 ops/s` (`115.54 ms`), and
  `BM_NativeWeightsLoadAndSiLU_T81Native/64` now measures `784.42 ops/s`
  (`93.37 ms`), while
  `BM_NativeWeightsLoadAndSoftmax_T81Native/64` now measures `711.95 ops/s`
  (`92.08 ms`).
- The first matched VM-path native-vs-binary comparison is now explicit too.
  On the same `64`-element interpreter loops, the native unary fast-path set is
  currently about `62x-67x` faster than the corresponding binary host-float
  tensor path. That is the strongest current evidence that T81's ternary-native
  execution path can beat the ordinary generic VM route in a way that is not
  just artifact-level or substrate-level.
- That direct-path result also extends beyond pure unary kernels now.
  `TRMSNorm` on native balanced-trit weights is materially faster than the
  equivalent binary host-float VM loop (`11.99x` throughput at `64` elements),
  which is the first higher-level kernel evidence that the same lightweight
  result-handling strategy generalizes beyond `TExp`/`TSiLU`/`TSoftmax`.
- `TRoPE` now has the same direct native path and still comes out ahead of the
  binary host-float VM route. After caching RoPE coefficients once per call in
  both the generic tensor kernel and the native fast path, the same `64`
  element VM comparison now shows a `4.07x` throughput and `4.20x` latency
  advantage. That is still well below the unary set and `TRMSNorm`, but it is
  no longer merely marginal. It also confirms that some higher-level kernels
  can still move materially with bounded algorithm-level cleanup before deeper
  representation changes are required.
- `TWEMBED` is now the clearest example of a kernel that should never have been
  forced through full native tensor promotion. Direct row extraction from the
  packed balanced-trit table yields a very large VM-path win at `64` elements:
  `769.63x` throughput and `923.86x` latency over the binary host-tensor path.
  That makes table-gather style native kernels a strong target class for
  continued direct packed-native handling.
- `TWMATMUL` now shows a more nuanced crossover story rather than a flat loss.
  The tiny `64`-trit smoke case is still fixed-overhead dominated and remains
  slower than the binary host-tensor path, though the latest activation-side
  scratch-buffer cleanup narrowed that gap to about `0.84x`. By `256` trits the native path
  is already materially positive (`11.84x`), and by `4096` trits it is
  decisively ahead (`570.42x`). That means the current native matmul kernel is
  usable evidence for medium and larger shapes, with remaining work focused on
  reducing small-shape setup overhead rather than replacing the steady-state
  kernel.
- `TATTN` turned out to be the next clear promotion-bound kernel. Once the VM
  stopped promoting native balanced-trit `K` weights and instead executed the
  attention score path directly from the cached packed-native view, the `256`
  trit VM comparison flipped from slightly negative to decisively positive:
  native `1.27 Mops/s` versus binary `11.48 ops/s`. That result is strong
  enough to treat direct native attention as established, not speculative.
- `TQUANT` is now another clear example of a direct-path win. The first
  benchmark run looked negative only because dispatch still promoted the native
  weights handle before checking the direct path. After fixing the dispatch
  order, the balanced-trit quantization fast path at `64` measures
  `309.18 Kops/s` versus `49.39 ops/s` for the binary host-tensor route,
  which is another strong signal that already-ternary native kernels should
  bypass generic promotion entirely.
- `TACT` now lands in the same category. For balanced native trits both
  supported activation modes map the native domain back onto itself, so the VM
  can skip promotion and return the direct ternary result before applying the
  existing activation-ceiling gate. At `64`, that yields `378.70 Kops/s`
  versus `55.41 ops/s` for the binary host-tensor path.
- `TERNACCUM` is now covered by the same native-vs-binary evidence style. It
  does not need tensor result allocation at all, just a direct ternary dot
  product into a `BigInt` handle, and at `64` that yields `183.91 Kops/s`
  versus `2.28 Kops/s` for the binary host-tensor route.
- This snapshot does not establish superiority over `llama.cpp` for end-user
  prompt inference. It establishes that T81's native ternary path is real,
  measurable, and now instrumented well enough to target the next bottleneck.
