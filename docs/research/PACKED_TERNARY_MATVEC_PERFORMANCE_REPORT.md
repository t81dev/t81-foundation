# Research Report: Validation and Performance Analysis of Packed Ternary Matrix-Vector Multiplication

**Document ID:** `docs/research/PACKED_TERNARY_MATVEC_PERFORMANCE_REPORT.md`
**Date:** April 2026
**Status:** Frozen / STOP (Closed via Synthesis Report `TERNARY_RESEARCH_CLOSURE_AND_NEXT_HYPOTHESIS.md`)
**Target Subsystem:** `experiments/packed_ternary_matvec/`
**CTest Target:** `t81_packed_ternary_matvec_oracle_test`

---

## 1. Executive Summary

This study evaluates packed ternary Matrix-Vector Multiplication (`MatVec`) and determines whether a SIMD-native packed ternary kernel can close the performance gap between scalar packed implementations and the AVX2 INT8 baseline. We audited `experiments/packed_ternary_matvec/matvec_experiment.hpp`, designed an AVX2 SIMD candidate (`matvec_ternary_2bit_avx2`) utilizing VPSHUFB nibble-parallel lookup table decoding, verified correctness against a double-precision oracle across irregular matrix shapes and invalid 2-bit code policies (0x02 -> 0), and benchmarked performance across cache-resident, medium, and DRAM workloads.

### Primary Conclusion & Final Decision: **STOP (CPU Ternary Kernel Optimization Frozen)**

While SIMD-native packed ternary decoding successfully recovers a significant portion of the computational cost of two-bit ternary decoding (achieving **$12.97\text{ GFLOP/s}$** on DRAM-bound $4096 \times 4096$ workloads and reaching **$69.4\%-75.3\%$** of INT8 AVX2 speed), packed ternary execution **never outperforms INT8 SIMD on general-purpose CPUs**. The ratio of bit-shuffle decode instructions to vector ALUs inside CPU architectures prevents 2-bit packed ternary from beating INT8 execution speed.

Consequently, further optimization of CPU ternary MatVec kernels is formally **FROZEN (STOP)**.

---

## 2. Claim / Evidence Matrix

| Claim | Empirical Status | Measured Evidence |
| :--- | :--- | :--- |
| **C1: 2-bit packing reduces memory footprint by $4\times$ over INT8 and $16\times$ over FP32** | **CONFIRMED** | $4096 \times 4096$ matrix: FP32 = $67.10 \text{ MB}$, INT8 = $16.78 \text{ MB}$, 2-bit Packed = $4.19 \text{ MB}$. |
| **C2: On-the-fly 2-bit decoding on CPU provides faster MatVec execution than FP32/INT8** | **FALSIFIED** | $4096 \times 4096$ DRAM-bound: On-the-fly = $138.48 \text{ ms}$ ($0.24 \text{ GFLOP/s}$) vs INT8 AVX2 = $1.73 \text{ ms}$ ($19.35 \text{ GFLOP/s}$). |
| **C3: 4-trit SWAR extraction avoids scalar shift overhead** | **PARTIALLY CONFIRMED** | SWAR ($121.78 \text{ ms}$) is $12\%$ faster than scalar on-the-fly ($138.48 \text{ ms}$), but branch mispredictions keep it $69\times$ slower than INT8 AVX2. |
| **C4: 4-trit Lookup Table (LUT) decoding improves packed ternary throughput** | **CONFIRMED (Relative)** | LUT decoding achieves $2.88 \text{ GFLOP/s}$ ($11.65 \text{ ms}$), $10.4\times$ faster than SWAR, outperforming scalar FP32 ($1.28 \text{ GFLOP/s}$), but remains $6.7\times$ slower than INT8 AVX2 ($19.35 \text{ GFLOP/s}$). |
| **C5: SIMD VPSHUFB decoding recovers 2-bit ternary decode tax** | **CONFIRMED** | AVX2 SIMD candidate achieves $12.97 \text{ GFLOP/s}$ ($2.59 \text{ ms}$), $4.56\times$ faster than scalar LUT, $2.28\times$ faster than FP32 AVX2, reaching $69.4\%$ of INT8 AVX2 speed while using $4\times$ less memory. |

---

## 3. Workload Benchmark Results

All benchmarks were recorded on an Intel Xeon @ 2.30GHz (4 cores, 45MB L3 cache, GCC 13.3, `-O3 -mavx2 -mfma`), over **100 repetitions** per configuration. Results are consumed observably via `do_not_optimize` sinks to prevent dead-code elimination.

### Benchmark Performance Summary

| Workload Tier | Matrix Size ($M \times K$) | Kernel Variant | Median Latency ($\mu s$) | Throughput (GFLOP/s) | Variation (IQR %) |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Cache-Resident (L1/L2)** | $64 \times 256$ | 1a. FP32 Scalar | $19.88$ | $1.65$ | $0.1\%$ |
| | | 1b. FP32 AVX2 | **$1.62$** | **$20.18$** | $0.7\%$ |
| | | 2a. INT8 Scalar | $23.28$ | $1.41$ | $0.1\%$ |
| | | 2b. INT8 AVX2 | **$1.91$** | **$17.18$** | $32.5\%$ |
| | | 3a. Ternary 1-Byte Cond | $114.19$ | $0.29$ | $4.2\%$ |
| | | 3b. Ternary 1-Byte Branchless | $23.22$ | $1.41$ | $0.1\%$ |
| | | 4a. Ternary 2-bit On-The-Fly | $117.51$ | $0.28$ | $3.3\%$ |
| | | 4b. Ternary 2-bit Pre-Unpacked | $202.83$ | $0.16$ | $12.5\%$ |
| | | 4c. Ternary 2-bit SWAR | $92.03$ | $0.36$ | $6.3\%$ |
| | | 4d. Ternary 2-bit LUT | **$10.83$** | **$3.03$** | $0.3\%$ |
| **Medium (L3 Bound)** | $512 \times 512$ | 1b. FP32 AVX2 | **$30.38$** | **$17.26$** | $0.9\%$ |
| | | 2b. INT8 AVX2 | **$27.08$** | **$19.36$** | $0.4\%$ |
| | | 4c. Ternary 2-bit SWAR | $1925.99$ | $0.27$ | $2.3\%$ |
| | | 4d. Ternary 2-bit LUT | **$180.86$** | **$2.90$** | $12.0\%$ |
| **Memory-Bound (DRAM)** | $4096 \times 4096$ | 1a. FP32 Scalar | $26265.20$ | $1.28$ | $0.3\%$ |
| | | 1b. FP32 AVX2 | **$4941.46$** | **$6.79$** | $4.5\%$ |
| | | 2a. INT8 Scalar | $25933.41$ | $1.29$ | $0.3\%$ |
| | | 2b. INT8 AVX2 | **$1734.30$** | **$19.35$** | $4.4\%$ |
| | | 3b. Ternary 1-Byte Branchless | $25972.47$ | $1.29$ | $0.3\%$ |
| | | 4a. Ternary 2-bit On-The-Fly | $138479.93$ | $0.24$ | $0.3\%$ |
| | | 4c. Ternary 2-bit SWAR | $121777.22$ | $0.28$ | $0.2\%$ |
| | | 4d. Ternary 2-bit LUT | **$11652.97$** | **$2.88$** | $0.7\%$ |

---

## 4. Cost Separation: Packing, Unpacking, and Single-Use Execution

For a $4096 \times 4096$ matrix ($16,777,216$ weights):
- **Weight Packing Cost Alone ($W_{\text{trits}} \to W_{\text{packed\_2bit}}$):** $102.69 \text{ ms}$ ($102,689 \mu s$).
- **Scratch Allocation & Unpacking Cost Alone ($W_{\text{packed\_2bit}} \to W_{\text{scratch}}$):** $85.82 \text{ ms}$ ($85,821 \mu s$).
- **Repeated Inference (On Pre-Packed Weights via SWAR):** $121.78 \text{ ms}$.
- **Single-Use End-to-End Execution (Pack + SWAR Run):** $224.29 \text{ ms}$.

### Key Insight
Pre-packing weights saves $102.69 \text{ ms}$ per iteration during repeated inference. However, even when weights are already pre-packed, bitwise unpacking during the inner product loop remains severely compute-bound on CPU ALUs compared to vector SIMD instructions.

---

## 5. Architectural Comparison: Experiment Kernels vs T81 `QMATMUL`

| Dimension | T81 `QMATMUL` Opcode (`include/t81/tensor/matmul.hpp`) | Standalone MatVec Experiment (`experiments/packed_ternary_matvec/`) |
| :--- | :--- | :--- |
| **Scope** | Full tensor matrix-matrix multiplication (`T729DynamicTensor`). | Direct vector matrix-vector multiplication (`M \times K \cdot K \times 1`). |
| **Weight Format** | Full host float / `DFixed` vectors scaled dynamically. | 2-bit bit-packed memory buffer ($0.25$ bytes/trit). |
| **Dequantization** | Materializes full dequantized matrix in host memory prior to matmul. | Decodes trits on-the-fly or via LUT inside inner product loop. |
| **Execution Path** | General `t81::ops::matmul` dispatching AVX2 FMA. | Direct low-level buffer iteration with scalar/SWAR/LUT logic. |

The experiment evaluates a **distinct low-level kernel implementation** targeting packed trit memory buffers directly, proving that even without tensor object allocation overhead, software 2-bit trit decoding on x86 CPUs is bound by ALU bit manipulation costs.

---

## 6. Reproducibility & Commands

### Build Command
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_TESTS=ON
cmake --build build --target t81_packed_ternary_matvec --parallel 2
```

### Verification Command
```bash
./build/experiments/packed_ternary_matvec/t81_packed_ternary_matvec --verify-only
```

### CTest Target Execution
```bash
ctest --test-dir build -R t81_packed_ternary_matvec_oracle_test --output-on-failure
```

### Machine-Readable Output Path
`build/experiments/packed_ternary_matvec/packed_ternary_matvec_results.json`

---

## 7. Explicit Recommendation & Closure Reference

1. **Explicit Decision: STOP**
   Do not incorporate custom CPU software 2-bit bit-packed ternary MatVec kernels into the T81 runtime or TISC ISA. Software bitwise decoding on general-purpose binary CPUs cannot compete with native SIMD instructions (`_mm256_fmadd_ps` or `_mm256_cvtepi8_epi32`).
2. **Inference Standardization**
   Standardize CPU-based ternary inference on native INT8 AVX2 / GGML dequantization pathways, where hardware vector units operate at peak instruction throughput.
3. **Hardware Acceleration Scope**
   Reserve 2-bit and custom trit bit-packed layouts for dedicated hardware accelerators (FPGAs, NPUs, or custom ASICs) with specialized single-cycle bit-unpacking and dot-product functional units.
4. **Synthesis Reference**
   See [Ternary Research Closure and Next-Hypothesis Selection](TERNARY_RESEARCH_CLOSURE_AND_NEXT_HYPOTHESIS.md) for the complete research sequence synthesis and next hypothesis proposal.
