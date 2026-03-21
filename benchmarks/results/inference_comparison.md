# Inference Comparison Benchmark Results

**Generated:** 2026-03-21  
**Commit:** `5a59bf7c`  
**Runner:** t81devs-MacBook-Air.local · 8 CPUs  
**Build:** Release · `-O3 -march=native`

> Throughput in weight-elements/second (higher is better).  
> T81 ternary: weights in {-1, 0, +1} — multiply replaced by conditional ADD/SUB or skip.

## Dot Product

| N | FP32 | FP16-sim | Int8 | T81 Scalar | T81 NEON | NEON vs FP32 | NEON vs Int8 |
|---|------|----------|------|------------|----------|--------------|--------------|
| 64 | 3.52 Gops/s | 809.61 Mops/s | 46.93 Gops/s | 1.95 Gops/s | 7.55 Gops/s | **2.14×** | **0.16×** |
| 256 | 2.05 Gops/s | 831.69 Mops/s | 72.34 Gops/s | 1.82 Gops/s | 6.40 Gops/s | **3.12×** | **0.09×** |
| 1,024 | 1.29 Gops/s | 826.11 Mops/s | 73.20 Gops/s | 1.71 Gops/s | 5.09 Gops/s | **3.94×** | **0.07×** |
| 4,096 | 1.18 Gops/s | 842.08 Mops/s | 52.39 Gops/s | 1.70 Gops/s | 4.63 Gops/s | **3.93×** | **0.09×** |
| 16,384 | 1.15 Gops/s | 840.46 Mops/s | 39.20 Gops/s | 1.68 Gops/s | 4.56 Gops/s | **3.96×** | **0.12×** |

## Matrix Multiply (1×N · N×N → 1×N)

| N | FP32 | Int8 | T81 Scalar | T81 NEON | T81 NEON Tiled | NEON vs FP32 | Tiled vs NEON |
|---|------|------|------------|----------|----------------|--------------|---------------|
| 64 | 12.18 Gops/s | 12.42 Gops/s | 1.94 Gops/s | 5.62 Gops/s | 3.70 Gops/s | **0.46×** | **0.66×** |
| 128 | 17.45 Gops/s | 18.58 Gops/s | 1.85 Gops/s | 5.77 Gops/s | 3.60 Gops/s | **0.33×** | **0.62×** |
| 256 | 13.90 Gops/s | 19.57 Gops/s | 1.90 Gops/s | 5.85 Gops/s | 3.66 Gops/s | **0.42×** | **0.63×** |
| 512 | 13.94 Gops/s | 18.76 Gops/s | 1.97 Gops/s | 5.47 Gops/s | 3.69 Gops/s | **0.39×** | **0.67×** |

> **NEON Tiled** uses output-register tiling (acc in NEON register across all K, single store
> per 4-element tile) but column-strided weight access. **NEON** uses row-streaming (sequential
> weight load, out reloaded per K step). Row-streaming wins because L1 out-reload is cheaper
> than L2/L3 misses from column-stride. The VM hotpath uses the row-streaming `i→p→j` order.

## Scaled Dot-Product Attention (head_dim=64)

| Seq len | FP32 | T81 Ternary | T81 vs FP32 |
|---------|------|-------------|-------------|
| 32 | 2.94 Gops/s | 710.65 Mops/s | **0.24×** |
| 64 | 2.94 Gops/s | 739.89 Mops/s | **0.25×** |
| 128 | 2.83 Gops/s | 727.33 Mops/s | **0.26×** |

## Transformer Layer Forward Pass (dim=256, heads=4, mlp=512)

| Seq len | FP32 (tok/s) | T81 Ternary (tok/s) | T81 vs FP32 |
|---------|-------------|---------------------|-------------|
| 32 | 12871 ops/s | 3166 ops/s | **0.25×** |
| 64 | 11424 ops/s | 2902 ops/s | **0.25×** |
| 128 | 10277 ops/s | 2871 ops/s | **0.28×** |

## Weight Loading — Memory Efficiency

| Weights | FP32 bytes | FP16 bytes | Int8 bytes | Ternary bytes | Ternary/FP32 |
|---------|------------|------------|------------|---------------|--------------|
| 1,024 | 4096 | 2048 | 1024 | 256 | 0.06× |
| 4,096 | 16384 | 8192 | 4096 | 1024 | 0.06× |
| 16,384 | 65536 | 32768 | 16384 | 4096 | 0.06× |
| 65,536 | 262144 | 131072 | 65536 | 16384 | 0.06× |

## VM Hotpath Fix (`native_tensor_twmatmul_direct`)

The production TWMATMUL opcode handler was updated in this commit:

| Before | After |
|--------|-------|
| Loop order: `p → i → j` | Loop order: `i → p → j` |
| Activation access: column-strided (`act[i*k+p]` with fixed p) | Sequential row access (`act[i*k+p]` with p in inner loop) |
| Output `out[i*n+j]` reloaded every p step across all i | Output row stays warm in L1 across entire K reduction |
| Scalar inner loop with branch | 4-wide NEON (`vcvtq+vbslq`, no multiply) + scalar tail |

The row-streaming NEON pattern (`i→p→j` with NEON in j) outperforms output-register
tiling (`i→j_tile→p`) at inference-typical sizes because sequential weight-row access
is cheaper than column-strided access at K≥128.

## Memory Ladder

| Format | Bytes/weight | 7B model size |
|--------|-------------|---------------|
| FP32   | 4           | 28 GB         |
| FP16   | 2           | 14 GB         |
| Int8   | 1           | 7 GB          |
| Ternary (packed) | 0.25 | 1.75 GB  |

## Methodology

- **FP32:** `float` multiply-accumulate.
- **FP16-sim:** `uint16_t` weights promoted to `float` per element (no AVX-512 FP16 MACs).
- **Int8:** `int8_t` weights, scalar `int8×int8→int32` MUL. Compiler autovectorizes to SDOT.
- **T81 Ternary (scalar):** weights in {-1,0,+1}, branch to ADD/SUB/skip. No float multiply.
- **T81 Ternary (NEON):** row-streaming 4-wide via `vcgtq_f32`+`vbslq_f32`. No `vmulq_f32`.
- **T81 Ternary (NEON Tiled):** output-register tiled (acc in register, single store/tile),
  but column-strided weight access hurts at K≥128. Shown for comparison.
- All benchmarks compiled with `-O3 -march=native`, mean of 2 repetitions.
- Results committed automatically by the `inference-bench` CI workflow.

For the VM packed-trit dispatch path see `BM_NativeWeightsExecution.cpp`.
