# Inference Comparison Benchmark Results

**Generated:** 2026-03-21  
**Commit:** `14d71b3f`  
**Runner:** t81devs-MacBook-Air.local · 8 CPUs  
**Build:** Release · `-O3 -march=native`

> Throughput in weight-elements/second (higher is better).  
> T81 ternary: weights in {-1, 0, +1} — multiply replaced by conditional ADD/SUB or skip.
> NEON: same compute pattern, 4-wide SIMD (AArch64). Int8: 8-bit weights, scalar MUL.

## Dot Product

| N | FP32 | FP16-sim | Int8 | T81 Scalar | T81 NEON | NEON vs FP32 | NEON vs Int8 |
|---|------|----------|------|------------|----------|--------------|--------------|
| 64 | 3.45 Gops/s | 808.52 Mops/s | 46.93 Gops/s | 1.92 Gops/s | 7.55 Gops/s | **2.19×** | **0.16×** |
| 256 | 2.01 Gops/s | 807.55 Mops/s | 72.34 Gops/s | 1.80 Gops/s | 6.40 Gops/s | **3.18×** | **0.09×** |
| 1,024 | 1.20 Gops/s | 837.95 Mops/s | 73.20 Gops/s | 1.71 Gops/s | 5.09 Gops/s | **4.23×** | **0.07×** |
| 4,096 | 1.15 Gops/s | 843.02 Mops/s | 52.39 Gops/s | 1.71 Gops/s | 4.63 Gops/s | **4.01×** | **0.09×** |
| 16,384 | 1.15 Gops/s | 834.77 Mops/s | 39.20 Gops/s | 1.70 Gops/s | 4.56 Gops/s | **3.96×** | **0.12×** |

## Matrix Multiply (1×N · N×N → 1×N)

| N | FP32 | FP16-sim | Int8 | T81 Scalar | T81 NEON | NEON vs FP32 | NEON vs Int8 |
|---|------|----------|------|------------|----------|--------------|--------------|
| 64 | 12.31 Gops/s | 813.27 Mops/s | 12.42 Gops/s | 1.93 Gops/s | 5.62 Gops/s | **0.46×** | **0.45×** |
| 128 | 17.44 Gops/s | 834.00 Mops/s | 18.58 Gops/s | 1.86 Gops/s | 5.81 Gops/s | **0.33×** | **0.31×** |
| 256 | 13.64 Gops/s | 835.85 Mops/s | 19.57 Gops/s | 1.93 Gops/s | 5.82 Gops/s | **0.43×** | **0.30×** |
| 512 | 13.96 Gops/s | 801.14 Mops/s | 18.76 Gops/s | 1.93 Gops/s | 5.56 Gops/s | **0.40×** | **0.30×** |

## Scaled Dot-Product Attention (head_dim=64)

| Seq len | FP32 | T81 Ternary | T81 vs FP32 |
|---------|------|-------------|-------------|
| 32 | 2.94 Gops/s | 731.81 Mops/s | **0.25×** |
| 64 | 2.94 Gops/s | 730.58 Mops/s | **0.25×** |
| 128 | 2.89 Gops/s | 743.71 Mops/s | **0.26×** |

## Transformer Layer Forward Pass (dim=256, heads=4, mlp=512)

| Seq len | FP32 (tok/s) | T81 Ternary (tok/s) | T81 vs FP32 |
|---------|-------------|---------------------|-------------|
| 32 | 12871.32 ops/s | 3166.43 ops/s | **0.25×** |
| 64 | 11423.91 ops/s | 2902.43 ops/s | **0.25×** |
| 128 | 10276.55 ops/s | 2870.89 ops/s | **0.28×** |

## Weight Loading — Memory Efficiency

| Weights | FP32 bytes | FP16 bytes | Int8 bytes | Ternary bytes | Ternary/FP32 |
|---------|------------|------------|------------|---------------|--------------|
| 1,024 | 4096 | 2048 | 1024 | 256 | 0.06× |
| 4,096 | 16384 | 8192 | 4096 | 1024 | 0.06× |
| 16,384 | 65536 | 32768 | 16384 | 4096 | 0.06× |
| 65,536 | 262144 | 131072 | 65536 | 16384 | 0.06× |

## Key Takeaways

- **NEON ternary dot product:** no-multiply 4-wide SIMD using compare-select.
  Expected to flip the matmul script vs scalar ternary.
- **Int8 baseline** fills the quantization ladder between FP16 and ternary.
- **Transformer layer:** end-to-end tokens/second through a full attention + SwiGLU MLP.
  Weight matrices: Q, K, V, O, gate, up, down (all ternary for T81 path).
  This is where the 1.75 GB story translates to real inference throughput.
- **Memory:** ternary = 16× fewer bytes than FP32, 8× vs FP16, 4× vs Int8.
  A 7B model at 2 bits/param = 1.75 GB — fits in 4 GB RAM with KV cache headroom.

## Methodology

- **FP32:** `float` multiply-accumulate.
- **FP16-sim:** `uint16_t` weights promoted to `float` per element (no AVX-512 FP16 MACs).
- **Int8:** `int8_t` weights, scalar MUL to `int32` accumulator. Compiler autovectorizes.
- **T81 Ternary (scalar):** weights in {-1,0,+1}, branch to ADD/SUB/skip. No float multiply.
- **T81 Ternary (NEON):** same compute pattern, 4 elements/cycle via `vcgtq_f32` + `vbslq_f32`.
  No `vmulq_f32` — the ternary constraint eliminates all multiplies.
- All benchmarks compiled with `-O3 -march=native`, mean of 2 repetitions.
- Results committed automatically by the `inference-bench` CI workflow.

For the VM packed-trit dispatch path see `BM_NativeWeightsExecution.cpp`.
