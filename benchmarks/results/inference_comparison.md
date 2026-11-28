# Inference Comparison Benchmark Results

**Generated:** 2026-11-28  
**Commit:** `be816e21`  
**Runner:** runnervm0j63q · 4 CPUs @ 3244 MHz  
**Build:** Release · `-O3 -march=native`

> Throughput in weight-elements/second (higher is better).  
> T81 ternary: weights in {-1, 0, +1} — multiply replaced by conditional ADD/SUB or skip.  
> NEON: same compute pattern, 4-wide SIMD (AArch64). Int8: 8-bit weights, scalar MUL.

## Dot Product

| N | FP32 | FP16-sim | Int8 | T81 Scalar | T81 NEON | NEON vs FP32 | NEON vs Int8 |
|---|------|----------|------|------------|----------|--------------|--------------|
| 64 | 1.90 Gops/s | 748.38 Mops/s | 7.96 Gops/s | 1.21 Gops/s | N/A | N/A | N/A |
| 256 | 1.37 Gops/s | 814.36 Mops/s | 8.46 Gops/s | 1.33 Gops/s | N/A | N/A | N/A |
| 1,024 | 1.13 Gops/s | 837.56 Mops/s | 8.61 Gops/s | 1.34 Gops/s | N/A | N/A | N/A |
| 4,096 | 1.08 Gops/s | 839.88 Mops/s | 8.65 Gops/s | 1.37 Gops/s | N/A | N/A | N/A |
| 16,384 | 1.07 Gops/s | 838.97 Mops/s | 8.66 Gops/s | 1.37 Gops/s | N/A | N/A | N/A |

## Matrix Multiply (1×N · N×N → 1×N)

| N | FP32 | FP16-sim | Int8 | T81 Scalar | T81 NEON | NEON vs FP32 | NEON vs Int8 |
|---|------|----------|------|------------|----------|--------------|--------------|
| 64 | 9.89 Gops/s | 2.35 Gops/s | 9.89 Gops/s | 1.20 Gops/s | N/A | N/A | N/A |
| 128 | 11.33 Gops/s | 2.38 Gops/s | 10.21 Gops/s | 1.28 Gops/s | N/A | N/A | N/A |
| 256 | 11.35 Gops/s | 2.36 Gops/s | 10.48 Gops/s | 1.32 Gops/s | N/A | N/A | N/A |
| 512 | 11.71 Gops/s | 2.40 Gops/s | 10.58 Gops/s | 1.34 Gops/s | N/A | N/A | N/A |

## Scaled Dot-Product Attention (head_dim=64)

| Seq len | FP32 | T81 Ternary | T81 vs FP32 |
|---------|------|-------------|-------------|
| 32 | 1.42 Gops/s | 829.05 Mops/s | **0.58×** |
| 64 | 1.50 Gops/s | 845.45 Mops/s | **0.57×** |
| 128 | 1.47 Gops/s | 851.70 Mops/s | **0.58×** |

## Transformer Layer Forward Pass (dim=256, heads=4, mlp=512)

| Seq len | FP32 (tok/s) | T81 Ternary (tok/s) | T81 vs FP32 |
|---------|-------------|---------------------|-------------|
| 32 | 16626 ops/s | 2076 ops/s | **0.12×** |
| 64 | 15082 ops/s | 2044 ops/s | **0.14×** |
| 128 | 12639 ops/s | 1958 ops/s | **0.15×** |

## Weight Loading — Memory Efficiency

| Weights | FP32 bytes | FP16 bytes | Int8 bytes | Ternary bytes | Ternary/FP32 |
|---------|------------|------------|------------|---------------|--------------|
| 1,024 | 4096 | 2048 | 1024 | 256 | 0.06× |
| 4,096 | 16384 | 8192 | 4096 | 1024 | 0.06× |
| 16,384 | 65536 | 32768 | 16384 | 4096 | 0.06× |
| 65,536 | 262144 | 131072 | 65536 | 16384 | 0.06× |

## Key Takeaways

- **NEON ternary dot product:** no-multiply 4-wide SIMD (vcgtq_f32 + vbslq_f32).
  Beats scalar FP32 at large N once working set exceeds L1 cache.
- **Int8** fills the quantization ladder between FP16 and ternary.
  SDOT/SMULL autovectorization gives very high throughput at small N.
- **Matmul:** FP32/int8 autovectorize to fused MACs; ternary NEON cmp+sel
  still trails. Next step: tiled inner loop to keep output row in registers.
- **Transformer layer:** end-to-end tokens/second through attention + SwiGLU MLP.
  Ternary weight projections (Q,K,V,O,gate,up,down) — no float multiply.
- **Memory:** ternary = 16× fewer bytes than FP32, 8× vs FP16, 4× vs Int8.
  A 7B model: FP32=28 GB, FP16=14 GB, Int8=7 GB, Ternary=1.75 GB.

## Methodology

- **FP32:** `float` multiply-accumulate via inner loop.
- **FP16-sim:** `uint16_t` weights promoted to `float` per element (no AVX-512 FP16 MACs).
- **Int8:** `int8_t` weights, scalar MUL to `int32` accumulator. Compiler autovectorizes.
- **T81 Ternary (scalar):** weights in {-1,0,+1}, branch to ADD/SUB/skip. No float multiply.
- **T81 Ternary (NEON):** same compute pattern, 4 elements/cycle via `vcgtq_f32` + `vbslq_f32`.
  No `vmulq_f32` — the ternary constraint eliminates all multiplies. AArch64 only.
- All benchmarks compiled with `-O3 -march=native`, mean of 2 repetitions.
- Results committed automatically by the `inference-bench` CI workflow.

For the VM packed-trit dispatch path see `BM_NativeWeightsExecution.cpp`.
