# Inference Comparison Benchmark Results

**Generated:** 2026-03-21  
**Commit:** `e80176ac`  
**Runner:** t81devs-MacBook-Air.local · 8 CPUs @ 24 MHz  
**Build:** Release · `-O3 -march=native`

> Throughput in weight-elements/second (higher is better).  
> T81 ternary: weights in {-1, 0, +1} — multiply replaced by conditional ADD/SUB or skip.  
> NEON: same compute pattern, 4-wide SIMD (AArch64). Int8: 8-bit weights, scalar MUL.

## Dot Product

| N | FP32 | FP16-sim | Int8 | T81 Scalar | T81 NEON | NEON vs FP32 | NEON vs Int8 |
|---|------|----------|------|------------|----------|--------------|--------------| 
| 64 | 3.42 Gops/s | 782.48 Mops/s | 48.57 Gops/s | 1.92 Gops/s | 7.37 Gops/s | **2.16×** | **0.15×** |
| 256 | 2.04 Gops/s | 815.18 Mops/s | 71.53 Gops/s | 1.81 Gops/s | 5.97 Gops/s | **2.93×** | **0.08×** |
| 1,024 | 1.29 Gops/s | 815.18 Mops/s | 72.20 Gops/s | 1.63 Gops/s | 4.96 Gops/s | **3.83×** | **0.07×** |
| 4,096 | 1.17 Gops/s | 816.38 Mops/s | 51.76 Gops/s | 1.65 Gops/s | 4.43 Gops/s | **3.78×** | **0.09×** |
| 16,384 | 1.15 Gops/s | 822.68 Mops/s | 38.79 Gops/s | 1.67 Gops/s | 4.34 Gops/s | **3.79×** | **0.11×** |

## Matrix Multiply (1×N · N×N → 1×N)

| N | FP32 | FP16-sim | Int8 | T81 Scalar | T81 NEON | NEON vs FP32 | NEON vs Int8 |
|---|------|----------|------|------------|----------|--------------|--------------| 
| 64 | 12.33 Gops/s | 804.02 Mops/s | 12.42 Gops/s | 1.93 Gops/s | 5.61 Gops/s | **0.45×** | **0.45×** |
| 128 | 17.27 Gops/s | 821.69 Mops/s | 18.63 Gops/s | 1.84 Gops/s | 5.79 Gops/s | **0.34×** | **0.31×** |
| 256 | 13.86 Gops/s | 828.84 Mops/s | 19.52 Gops/s | 1.91 Gops/s | 5.84 Gops/s | **0.42×** | **0.30×** |
| 512 | 13.92 Gops/s | 830.69 Mops/s | 18.83 Gops/s | 1.96 Gops/s | 5.55 Gops/s | **0.40×** | **0.29×** |

## Scaled Dot-Product Attention (head_dim=64)

| Seq len | FP32 | T81 Ternary | T81 vs FP32 |
|---------|------|-------------|-------------|
| 32 | 2.84 Gops/s | 715.11 Mops/s | **0.25×** |
| 64 | 2.85 Gops/s | 715.41 Mops/s | **0.25×** |
| 128 | 2.84 Gops/s | 709.69 Mops/s | **0.25×** |

## Transformer Layer Forward Pass (dim=256, heads=4, mlp=512)

| Seq len | FP32 (tok/s) | T81 Ternary (tok/s) | Hybrid MLP (tok/s) | Ternary vs FP32 | Hybrid vs Ternary |
|---------|-------------|---------------------|--------------------|-----------------|--------------------|
| 32 | 13726 ops/s | 3394 ops/s | 6104 ops/s | **0.25×** | **1.80×** |
| 64 | 12795 ops/s | 3207 ops/s | 5547 ops/s | **0.25×** | **1.73×** |
| 128 | 11531 ops/s | 2915 ops/s | 4725 ops/s | **0.25×** | **1.62×** |

> **Hybrid MLP:** attention projections (Q/K/V/O) use ternary weights; MLP (gate/up/down) uses FP32.
> Requires Axion policy approval (`T81_HYBRID_MLP=ON`). Ternary weight invariant partially relaxed.

## Weight Loading — Memory Efficiency

| Weights | FP32 bytes | FP16 bytes | Int8 bytes | Ternary bytes | Ternary/FP32 |
|---------|------------|------------|------------|---------------|--------------|
| 1,024 | 4096 | 2048 | 1024 | 256 | 0.06× |
| 4,096 | 16384 | 8192 | 4096 | 1024 | 0.06× |
| 16,384 | 65536 | 32768 | 16384 | 4096 | 0.06× |
| 65,536 | 262144 | 131072 | 65536 | 16384 | 0.06× |

## VM Hotpath Optimizations (this session)

| Function | Optimization | Technique |
|----------|-------------|-----------|
| TWMATMUL | L2 P-blocking (PB=256) + 16-wide NEON/AVX2 unroll + prefetch | Cache tiling, ILP, `__builtin_prefetch` |
| TATTN    | Reordered score loop i→j→p; NEON 8-wide int8×int8; FMA V-output | Cache locality, `vmull_s8`+`vpaddlq_s16`, `vmlaq_f32` |
| TERNACCUM | ExactTrit fast path: int8 snap → SIMD dot → single T81BigInt | Avoids BigInt per element; `vmull_s8`+`vpaddlq_s16` |
| TQUANT   | NEON/AVX2 int8→float via `vmovl`/`_mm256_cvtepi8_epi32` | Vectorised type conversion |
| TACT     | NEON/AVX2 int8→float (both modes reduce to identity for trits) | Explicit mode + SIMD widen |
| TWEMBED  | Branchless trit decode via `kDigitToTrit[3]` constexpr table | Replaces switch per digit |
| RoPE     | NEON `vld2q_f32`/`vst2q_f32` 4-pair deinterleave rotation | Avoids scalar pair loop |
| AVX2     | All NEON paths mirrored: `_mm256_cmp_ps`+`_mm256_blendv_ps`+FMA | x86_64 parity |

## Key Takeaways

- **NEON ternary dot product:** no-multiply 4-wide SIMD (vcgtq_f32 + vbslq_f32).
  Beats scalar FP32 at large N once working set exceeds L1 cache.
- **Int8** fills the quantization ladder between FP16 and ternary.
  SDOT/SMULL autovectorization gives very high throughput at small N.
- **Matmul L2 tiling (PB=256):** keeps weight block in L2 across all M rows,
  amortising L3 load cost. 16-wide unroll provides 4 independent accumulator chains.
- **Transformer layer:** end-to-end tokens/second through attention + SwiGLU MLP.
  Ternary weight projections (Q,K,V,O,gate,up,down) — no float multiply.
- **Hybrid MLP:** FP32 MLP projections give ~1.6× speedup over full ternary.
  Trade-off: partial ternary invariant relaxation; Axion policy gate required.
- **Memory:** ternary = 16× fewer bytes than FP32, 8× vs FP16, 4× vs Int8.
  A 7B model: FP32=28 GB, FP16=14 GB, Int8=7 GB, Ternary=1.75 GB.

## Methodology

- **FP32:** `float` multiply-accumulate via inner loop.
- **FP16-sim:** `uint16_t` weights promoted to `float` per element (no AVX-512 FP16 MACs).
- **Int8:** `int8_t` weights, scalar MUL to `int32` accumulator. Compiler autovectorizes.
- **T81 Ternary (scalar):** weights in {-1,0,+1}, branch to ADD/SUB/skip. No float multiply.
- **T81 Ternary (NEON):** same compute pattern, 4–16 elements/cycle via `vcgtq_f32` + `vbslq_f32`.
  No `vmulq_f32` — the ternary constraint eliminates all multiplies. AArch64 only.
- **Hybrid MLP:** attention=ternary NEON, MLP projections=FP32 BLAS-style loop.
- All benchmarks compiled with `-O3 -march=native`, mean of 2 repetitions.
- Results committed automatically by the scheduled/manual `inference-bench` CI workflow.

For the VM packed-trit dispatch path see `BM_NativeWeightsExecution.cpp`.
