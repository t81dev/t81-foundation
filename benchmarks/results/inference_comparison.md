# Inference Comparison Benchmark Results

**Generated:** 2026-03-21  
**Commit:** `00ed7b0c`  
**Runner:** t81devs-MacBook-Air.local · 8 CPUs @ 24 MHz  
**Build:** Release · `-O3 -march=native`

> Throughput in weight-elements/second (higher is better).  
> T81 ternary: weights in {-1, 0, +1} — multiply replaced by conditional ADD/SUB or skip.  
> NEON: same compute pattern, 4-wide SIMD (AArch64). Int8: 8-bit weights, scalar MUL.

## Dot Product

| N | FP32 | FP16-sim | Int8 | T81 Scalar | T81 NEON | NEON vs FP32 | NEON vs Int8 |
|---|------|----------|------|------------|----------|--------------|--------------|}
| 64 | 3.53 Gops/s | 793.79 Mops/s | 49.64 Gops/s | 1.95 Gops/s | 7.46 Gops/s | **2.12×** | **0.15×** |
| 256 | 2.02 Gops/s | 824.31 Mops/s | 72.07 Gops/s | 1.81 Gops/s | 6.23 Gops/s | **3.08×** | **0.09×** |
| 1,024 | 1.29 Gops/s | 819.94 Mops/s | 72.10 Gops/s | 1.70 Gops/s | 5.04 Gops/s | **3.90×** | **0.07×** |
| 4,096 | 1.17 Gops/s | 824.28 Mops/s | 52.06 Gops/s | 1.69 Gops/s | 4.61 Gops/s | **3.94×** | **0.09×** |
| 16,384 | 1.14 Gops/s | 833.30 Mops/s | 38.99 Gops/s | 1.68 Gops/s | 4.36 Gops/s | **3.82×** | **0.11×** |

## Matrix Multiply (1×N · N×N → 1×N)

| N | FP32 | FP16-sim | Int8 | T81 Scalar | T81 NEON | NEON Tiled | NEON vs FP32 | NEON vs Int8 |
|---|------|----------|------|------------|----------|------------|--------------|--------------|
| 64 | 12.28 Gops/s | 804.22 Mops/s | 12.42 Gops/s | 1.92 Gops/s | 5.50 Gops/s | 3.66 Gops/s | **0.45×** | **0.44×** |
| 128 | 17.39 Gops/s | 824.85 Mops/s | 18.74 Gops/s | 1.84 Gops/s | 5.73 Gops/s | 3.60 Gops/s | **0.33×** | **0.31×** |
| 256 | 13.84 Gops/s | 832.14 Mops/s | 19.51 Gops/s | 1.91 Gops/s | 5.82 Gops/s | 3.63 Gops/s | **0.42×** | **0.30×** |
| 512 | 13.94 Gops/s | 832.00 Mops/s | 18.77 Gops/s | 1.96 Gops/s | 5.57 Gops/s | 3.47 Gops/s | **0.40×** | **0.30×** |

> **NEON Tiled** keeps the output tile in registers across K but accesses weights
> column-strided, causing L2/L3 misses that outweigh the register savings at N≥128.
> Row-streaming NEON (sequential weight access) wins at all inference-typical sizes.

## Scaled Dot-Product Attention (head_dim=64)

| Seq len | FP32 | T81 Ternary | T81 vs FP32 |
|---------|------|-------------|-------------|
| 32 | 2.85 Gops/s | 730.28 Mops/s | **0.26×** |
| 64 | 2.91 Gops/s | 727.76 Mops/s | **0.25×** |
| 128 | 2.93 Gops/s | 707.61 Mops/s | **0.24×** |

## Transformer Layer Forward Pass (dim=256, heads=4, mlp=512)

| Seq len | FP32 (tok/s) | T81 Ternary (tok/s) | T81 vs FP32 |
|---------|-------------|---------------------|-------------|
| 32 | 13764 ops/s | 3368 ops/s | **0.24×** |
| 64 | 12944 ops/s | 3127 ops/s | **0.24×** |
| 128 | 11178 ops/s | 2771 ops/s | **0.25×** |

## Weight Loading — Memory Efficiency

| Weights | FP32 bytes | FP16 bytes | Int8 bytes | Ternary bytes | Ternary/FP32 |
|---------|------------|------------|------------|---------------|--------------|}
| 1,024 | 4096 | 2048 | 1024 | 256 | 0.06× |
| 4,096 | 16384 | 8192 | 4096 | 1024 | 0.06× |
| 16,384 | 65536 | 32768 | 16384 | 4096 | 0.06× |
| 65,536 | 262144 | 131072 | 65536 | 16384 | 0.06× |

## VM Hotpath Optimizations (applied since first results)

All six RFC-0034 native dispatch functions in `core/vm/tensor_helpers.cpp` now have
full SIMD coverage. Changes committed in this session:

| Op | Change | Impact |
|---|---|---|
| **TWMATMUL** | Loop order p→i→j → **i→p→j**; NEON 4-wide + AVX2 8-wide int8 select | Output row stays warm in L1 |
| **TATTN** score | Loop order p→i→j → **i→j→p**; NEON/AVX2 8-wide `int8×int8` inner product | Sequential Q/K access, score in register |
| **TATTN** softmax | NEON 4-wide / AVX2 8-wide normalize multiply | Scalar `exp()` retained for determinism |
| **TATTN** V output | NEON `vmlaq_f32` FMA 4-wide / AVX2 `_mm256_fmadd_ps` 8-wide | Zero-skip preserved |
| **TERNACCUM** | ExactTrit fast path: pre-snap → SIMD int8 dot → single `T81BigInt` | Replaces per-element BigInt ops |
| **TQUANT** | NEON/AVX2 8-wide int8→float widening | Eliminates scalar cast loop |
| **TACT** | NEON/AVX2 8-wide int8→float (both modes identity on pre-ternary) | Scalar tail retains tanh+snap |
| **RoPE** | NEON `vld2q_f32` + `vmlsq/vmlaq` + `vst2q_f32` for 4 pairs/iter | Deinterleave + rotate + reinterleave in 3 instructions |

## Key Takeaways

- **NEON ternary dot product:** 3.8–4.0× FP32 at large N (no multiply — cmp+sel only).
- **Int8** is the dominant quantization ladder entry: Apple `SDOT` autovectorization
  at 38–72 Gops/s, 7–15× faster than ternary dot product.
  Ternary advantage is memory (16× fewer bytes), not raw throughput.
- **MatMul:** FP32/Int8 autovectorize to fused MACs; ternary NEON row-streaming
  reaches 5.5 Gops/s. Tiled (output-register) is slower due to column-strided weight access.
- **Attention:** T81 ~0.25× FP32 — score loop and V output now fully SIMD;
  `std::exp()` in softmax remains the bottleneck.
- **Transformer layer:** ~3,000–3,400 tok/s ternary vs ~11,000–14,000 FP32 (~0.25×).
  Memory pressure at 1.75 GB for a 7B model is the primary deployment advantage.
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
