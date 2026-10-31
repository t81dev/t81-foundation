# RFC-0034 VM Hotpath SIMD Optimization Evidence

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [RFC-0034 VM Hotpath SIMD Optimization Evidence](#rfc-0034-vm-hotpath-simd-optimization-evidence)
  - [Scope](#scope)
  - [Changes Committed (bb74edaa and preceding commits)](#changes-committed-bb74edaa-and-preceding-commits)
    - [TWMATMUL — L2 P-tiling + 16-wide NEON/AVX2 unroll + prefetch](#twmatmul-—-l2-p-tiling-+-16-wide-neonavx2-unroll-+-prefetch)
    - [TATTN — Reordered score loop + NEON int8×int8 + FMA V-output](#tattn-—-reordered-score-loop-+-neon-int8×int8-+-fma-v-output)
    - [TERNACCUM — ExactTrit fast path](#ternaccum-—-exacttrit-fast-path)
    - [TQUANT — NEON/AVX2 int8→float widening](#tquant-—-neonavx2-int8→float-widening)
    - [TACT — Explicit mode + NEON/AVX2 int8→float](#tact-—-explicit-mode-+-neonavx2-int8→float)
    - [TWEMBED — Branchless trit decode](#twembed-—-branchless-trit-decode)
    - [RoPE — NEON vld2q_f32 / vst2q_f32 deinterleave rotation](#rope-—-neon-vld2q_f32--vst2q_f32-deinterleave-rotation)
    - [AVX2 parity](#avx2-parity)
  - [Benchmark Results (ARM64, Darwin 25.3.0, -O3 -march=native)](#benchmark-results-arm64-darwin-2530--o3--march=native)
    - [Transformer Layer Forward Pass (dim=256, heads=4, mlp=512)](#transformer-layer-forward-pass-dim=256-heads=4-mlp=512)
  - [HybridMLP Benchmark and Governance Gate](#hybridmlp-benchmark-and-governance-gate)
  - [Conformance Test Status](#conformance-test-status)
- [→ 5/5 pass](#→-55-pass)
  - [Remaining Evidence Work](#remaining-evidence-work)

<!-- T81-TOC:END -->


Status: Active
Date: 2026-03-21
Owner: @t81dev
Commit: bb74edaa
Version: 1.9.1

## Scope

Evidence record for the SIMD optimization pass on the six RFC-0034 native
dispatch functions in `core/vm/tensor_helpers.cpp`, covering eight targeted
improvements across the TWMATMUL, TATTN, TERNACCUM, TQUANT, TACT, TWEMBED,
and RoPE hotpaths. Also covers the HybridMLP benchmark and T81_HYBRID_MLP
governance gate.

This is a performance evidence record, not a correctness closure. All
RFC-0034 conformance tests continue to pass (unchanged).

## Changes Committed (bb74edaa and preceding commits)

### TWMATMUL — L2 P-tiling + 16-wide NEON/AVX2 unroll + prefetch
- P-blocking (PB=256): weight block fits in L2 across all M rows, amortising
  L3 load cost. Loop order i→p→j: output row warm in L1 across full reduction.
- 16-wide NEON unroll: four independent `float32x4_t` accumulator chains
  improve ILP on dual-issue NEON FUs.
- AVX2 mirror: 16-wide via two `__m256` chains (`_mm256_cmp_ps` + `_mm256_blendv_ps`).
- `__builtin_prefetch(wrow + n, 0, 2)` for next weight row (L2 locality hint).

### TATTN — Reordered score loop + NEON int8×int8 + FMA V-output
- Reordered score loop from p→i→j to i→j→p: Q row and K row accessed
  sequentially; score scalar stays in register. Eliminates column-strided
  cache misses on Q and K.
- NEON 8-wide int8×int8 dot: `vmull_s8` + `vpaddlq_s16` → `int32x4`,
  `vaddvq_s32` horizontal reduce.
- AVX2 8-wide int32: `_mm256_cvtepi8_epi32` + `_mm256_mullo_epi32` +
  `_mm_hadd_epi32` horizontal reduce.
- V-output accumulation via `vmlaq_f32` (NEON FMA) and `_mm256_fmadd_ps` (AVX2).

### TERNACCUM — ExactTrit fast path
- Pre-snaps float activations to int8 {-1, 0, +1} in a single pass.
- SIMD int8×int8 dot (same `vmull_s8`+`vpaddlq_s16` kernel as TATTN).
- Constructs a single `T81BigInt` from the int32 accumulator at the end.
  Avoids T81BigInt arithmetic per element; bit-exact for N ≤ 2^30.
- Falls through to scalar BigInt path when activations are not ExactTrit.

### TQUANT — NEON/AVX2 int8→float widening
- `vld1_s8` → `vmovl_s8` → `vmovl_s16` → `vcvtq_f32_s32` (NEON, 8/iter).
- `_mm_loadl_epi64` → `_mm256_cvtepi8_epi32` → `_mm256_cvtepi32_ps` (AVX2, 8/iter).
- Applied when threshold < 1.0f (trit identity path).

### TACT — Explicit mode + NEON/AVX2 int8→float
- Corrected silent no-op: mode parameter was previously ignored in the native
  path. For ExactTrit inputs both kTActModeIdentity and kTActModeTanh reduce
  to identity (snap_trit(tanh(±1)) = ±1), but scalar tail now applies mode
  explicitly for correctness.
- Same int8→float SIMD widening as TQUANT for the fast path.

### TWEMBED — Branchless trit decode
- Replaced per-digit switch with `constexpr float kDigitToTrit[3]` table.
  `kDigitToTrit[digit]` where digit = val % 3 (0→-1, 1→0, 2→+1).
- Base-3 decode chain is structurally serial (each digit depends on prior
  quotient); table replaces the only parallelisable part.

### RoPE — NEON vld2q_f32 / vst2q_f32 deinterleave rotation
- `vld2q_f32` loads 8 floats as 4 interleaved pairs into two `float32x4_t`
  lanes; `vst2q_f32` re-interleaves after rotation.
- Processes 8 elements (4 sin/cos pairs) per iteration with 4 FMA-style ops.
- Scalar tail handles head_dim % 8 remainder.

### AVX2 parity
- All NEON paths mirrored with AVX2 intrinsics (`_mm256_cmp_ps`,
  `_mm256_blendv_ps`, `_mm256_fmadd_ps`, `_mm256_cvtepi8_epi32`).
- Added `#ifdef __AVX2__ / #include <immintrin.h>` alongside the NEON guard.

## Benchmark Results (ARM64, Darwin 25.3.0, -O3 -march=native)

### Transformer Layer Forward Pass (dim=256, heads=4, mlp=512)

| Seq | FP32 (tok/s) | T81 Ternary (tok/s) | Hybrid MLP (tok/s) | Hybrid vs Ternary |
|-----|-------------|---------------------|--------------------|--------------------|
| 32  | 13,726      | 3,394               | 6,104              | **1.80×**          |
| 64  | 12,795      | 3,207               | 5,547              | **1.73×**          |
| 128 | 11,531      | 2,915               | 4,725              | **1.62×**          |

Full results committed to `benchmarks/results/inference_comparison.md`.

## HybridMLP Benchmark and Governance Gate

- `BM_TransformerLayer_HybridMLP`: attention projections (Q/K/V/O) use ternary
  weights; MLP gate/up/down use FP32.
- Demonstrates ~1.6–1.8× throughput improvement over full ternary.
- Gated behind `T81_HYBRID_MLP=ON` CMake option (default OFF).
- Governance note inline: requires Axion policy approval; partially relaxes the
  ternary weight invariant for the MLP projection set.
- Not promoted to the DCP surface; treated as a performance experiment under
  the experimental governance envelope.

## Conformance Test Status

All RFC-0034 conformance tests unchanged and passing:

```sh
ctest --test-dir build -R 't81_vm_rfc0034_ternary_native_test' --output-on-failure
# → 5/5 pass
```

## Remaining Evidence Work

- x86_64 (AVX2) benchmark snapshot: current results are ARM64/NEON only.
  The CI workflow (`inference-bench.yml`) targets ubuntu-24.04; results from
  the next CI run will populate the x86_64 side.
- HybridMLP Axion policy definition: the CMake gate exists; a formal Axion
  YAML policy is not yet authored.
- TWEMBED base-3 SIMD: structurally serial quotient chain cannot be vectorised
  without pre-packed column-major weight layout (deferred as RFC-level work).
