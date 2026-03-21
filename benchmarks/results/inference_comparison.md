# Inference Comparison Benchmark Results

**Generated:** 2026-03-21  
**Commit:** `85a54b98`  
**Runner:** t81devs-MacBook-Air.local · 8 CPUs  
**Build:** Release · `-O3 -march=native`

> Throughput in weight-elements/second (higher is better).  
> T81 ternary: weights in {-1, 0, +1} — multiply replaced by conditional ADD/SUB or skip.

## Dot Product

| N | FP32 | FP16-sim | T81 Ternary | T81 vs FP32 | T81 vs FP16 |
|---|------|----------|-------------|-------------|-------------|
| 64 | 3.33 Gops/s | 808.72 Mops/s | 1.97 Gops/s | **0.59×** | **2.44×** |
| 256 | 1.95 Gops/s | 833.79 Mops/s | 1.84 Gops/s | **0.95×** | **2.21×** |
| 1,024 | 1.28 Gops/s | 840.90 Mops/s | 1.74 Gops/s | **1.36×** | **2.07×** |
| 4,096 | 1.17 Gops/s | 846.01 Mops/s | 1.70 Gops/s | **1.46×** | **2.01×** |
| 16,384 | 1.15 Gops/s | 839.05 Mops/s | 1.69 Gops/s | **1.48×** | **2.02×** |

## Matrix Multiply (1×N · N×N → 1×N)

| N | FP32 | FP16-sim | T81 Ternary | T81 vs FP32 | T81 vs FP16 |
|---|------|----------|-------------|-------------|-------------|
| 64 | 12.40 Gops/s | 805.58 Mops/s | 1.91 Gops/s | **0.15×** | **2.37×** |
| 128 | 17.50 Gops/s | 817.88 Mops/s | 1.87 Gops/s | **0.11×** | **2.28×** |
| 256 | 13.87 Gops/s | 826.07 Mops/s | 1.94 Gops/s | **0.14×** | **2.34×** |
| 512 | 13.84 Gops/s | 835.67 Mops/s | 1.98 Gops/s | **0.14×** | **2.37×** |

## Scaled Dot-Product Attention (head_dim=64)

| Seq len | FP32 | T81 Ternary | T81 vs FP32 |
|---------|------|-------------|-------------|
| 32 | 2.92 Gops/s | 740.22 Mops/s | **0.25×** |
| 64 | 2.93 Gops/s | 741.64 Mops/s | **0.25×** |
| 128 | 2.94 Gops/s | 741.81 Mops/s | **0.25×** |

## Weight Loading — Memory Efficiency

| Weights | FP32 bytes | FP16 bytes | Ternary bytes | FP16/FP32 | Ternary/FP32 |
|---------|------------|------------|---------------|-----------|--------------|
| 1,024 | 4096 | 2048 | 256 | 0.50× | 0.06× |
| 4,096 | 16384 | 8192 | 1024 | 0.50× | 0.06× |
| 16,384 | 65536 | 32768 | 4096 | 0.50× | 0.06× |
| 65,536 | 262144 | 131072 | 16384 | 0.50× | 0.06× |

## Key Takeaways

- **Dot product:** T81 ternary is **1.4–1.5× faster than FP32** at large N and **2× faster
  than FP16-sim** across all sizes. The memory-bandwidth advantage of 2-bit packed weights
  pays off once the working set exceeds L1 cache.
- **Matrix multiply:** FP32 autovectorizes well (SIMD MACs); T81 conditional-branch pattern
  prevents vectorization, so FP32 leads here. T81 still beats FP16-sim (2.3×) because
  FP16's load+promote per element costs more than a branch.
- **Memory:** Ternary packed storage uses **16× fewer bytes than FP32** (2 bits/trit vs 32).
  A 7B-parameter model at 4 bytes/param = 28 GB; at 2 bits/trit = 1.75 GB.

## Methodology

- **FP32:** `float` multiply-accumulate via inner loop.
- **FP16-sim:** Weights stored as `uint16_t` (IEEE 754 half), promoted to `float` before each
  multiply. Represents realistic CPU FP16 inference without AVX-512 FP16 MACs.
- **T81 Ternary:** Weights constrained to {-1, 0, +1}. Multiply replaced by:
  `w==+1 → acc+=a`, `w==-1 → acc-=a`, `w==0 → skip`. This is the compute pattern of the
  T81 VM packed-trit path (TWMATMUL/TERNACCUM opcodes). The ops-layer reference
  (`t81::ops::ternaccum`) uses BigInt for bit-exact audit trails and is not a
  throughput comparator.
- All benchmarks compiled with `-O3 -march=native`, mean of 2 repetitions.
- Results committed automatically by the `inference-bench` CI workflow.

For the VM-dispatch path with packed trit storage see `BM_NativeWeightsExecution.cpp`.
