# Empirical Note: Dual-Rail `(P, N)` Heterogeneous Execution & Narrowed TISC Subgraphs

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Empirical Note: Dual-Rail `(P, N)` Heterogeneous Execution & Narrowed TISC Subgraphs](#empirical-note-dual-rail-`p-n`-heterogeneous-execution-&-narrowed-tisc-subgraphs)
  - [1. Summary of Findings](#1-summary-of-findings)
    - [Positive Results (Verified & Reproducible)](#positive-results-verified-&-reproducible)
  - [2. Preserved Negative Results (Falsifiers Triggered & Reverted)](#2-preserved-negative-results-falsifiers-triggered-&-reverted)
  - [3. Reproducing Locally](#3-reproducing-locally)
- [1. Dual-Rail CPU SIMD benchmark](#1-dual-rail-cpu-simd-benchmark)
- [2. Heterogeneous CPU/Metal Dual-Rail fused decode + capability report](#2-heterogeneous-cpumetal-dual-rail-fused-decode-+-capability-report)
- [3. Narrowed TISC static policy verifier + cross-device receipt equivalence](#3-narrowed-tisc-static-policy-verifier-+-cross-device-receipt-equivalence)

<!-- T81-TOC:END -->


- **Status:** Evidence-bounded engineering note (reproducible on pinned commits `cfe7a269a`–`123745f2b`)
- **Platform:** Apple M2 (`arm64-apple-darwin`, `UnifiedMemory = YES`, `100 GB/s` peak bandwidth), Apple Clang `-std=c++23 -O3 -DNDEBUG`
- **Scope:** Evaluates balanced-ternary `{-1, 0, +1}` execution on conventional binary CPU/GPU/AMX silicon with full pack/unpack accounting, competent binary controls, and preserved negative results. Does **not** claim general ternary superiority or native three-state hardware advantages (`H01` remains open).

---

## 1. Summary of Findings

### Positive Results (Verified & Reproducible)
1. **SIMD Dual-Rail Codec & CPU Kernels** ([benchmarks/BM_DualRailTrit.cpp](../../benchmarks/BM_DualRailTrit.cpp), `cfe7a269a`):
   - NEON `vpaddq_u8` bitmask packing cut `int8 -> DualRail64` conversion by **14.4×** (`54.2 µs` → `3.78 µs` at `N = 65536`; **21.3×** faster than `ComputeTritVector::from_trits`).
   - Break-even reuse factor `K*` vs. competent NEON `vdotq_s32` `int8` dropped from `~70` → **`5.4`** (1 dynamic pack, pre-packed weights) or **`10.7`** (2 dynamic packs).
   - Kernel throughput (`N = 65536`): `DualRail64` dot = `76.16 Gops/s` (**`1.82×`** vs. NEON `vdotq_s32` `41.96 Gops/s`); Kleene AND = `157.98 Gops/s` (**`3.80×`** vs. `int8`, **`6.68×`** vs. 2-bit interleaved `ComputeTritVector`).
2. **Zero-Copy CPU ↔ Metal GPU Fused 32-Layer Decode** ([examples/research/heterogeneous-dualrail/](../../examples/research/heterogeneous-dualrail/), `45b5b6b61`, `123745f2b`):
   - Streaming 32 distinct layers of `(1×4096×4096 + 1×4096×11008)` (`1.98B` weights: `473 MB` DualRail vs. `1.89 GB` `int8`) in **one** fused Metal command buffer over `MTLResourceStorageModeShared` reached **`5.55 ms`** (`180.02 tok/s`, **`89.10 GB/s`** ≈ 89% of Apple M2's `100 GB/s` DRAM peak): **`3.45×`** over unfused 64-CB GPU, **`3.79×`** over CPU DualRail, and **`25.13×`** over 1-thread CPU `int8`.
   - **4-Way Real Silicon Crossover vs. Apple AMX (`Accelerate cblas_sgemm` FP32) & Apple Production GPU (`MPSMatrixMultiplication` FP16):**
     - `1×4096×4096` decode: T81 DualRail GPU (`34.6 µs` kern, `969 GOPS`) = **`72.0×`** vs. MPS FP16 / **`141.7×`** vs. AMX.
     - `1×4096×11008` decode: T81 DualRail GPU (`139.9 µs` kern, `645 GOPS`) = **`42.4×`** vs. MPS FP16 / **`29.0×`** vs. AMX.
     - `256×2048×2048` prefill: T81 DualRail GPU (`646.4 µs` kern, **`3.32 TOPS`**) = **`2.90×`** vs. MPS FP16 / **`4.78×`** vs. AMX.
     - `512×2048×2048` prefill: T81 DualRail GPU (`1,364.2 µs` kern, **`3.15 TOPS`**) = **`1.55×`** vs. MPS FP16 / **`4.13×`** vs. AMX.
     - *Crossover boundary:* As `M` grows past `512` (`M >= 1024`), compute-bound FP16/NPU tensor engines overtake bitwise `DualRail`.
   - **Cross-Device Determinism:** `SHA3-512(CPU int8) == SHA3-512(CPU DualRail) == SHA3-512(Fused Metal GPU DualRail)` bit-for-bit on every tested shape.
3. **Narrowed TISC Tensor + Policy Subgraph** ([examples/research/tisc-subgraph-dispatch/](../../examples/research/tisc-subgraph-dispatch/), `c322ab8e2`):
   - A 55-line static verifier (`9.92 µs` over 65 encoded `t81::tisc::Program` instructions) enforces `WeightsLoad` → `ChkShape` → `AxVerify` → `TMatMul` before execution, fails closed on missing policy or shape checks, dispatches zero-copy to CPU NEON or Fused Metal GPU, and produces the **exact same `SHA3-512` execution receipt** across both backends.
4. **T81Lang Frontend Lexer** ([lang/frontend/lexer.cpp](../../lang/frontend/lexer.cpp), `dbba8f11a`):
   - Replacing `std::unordered_map` keyword hashing with a 256-byte ASCII LUT + length-switched matcher accelerated `BM_Lexer_AllTokens` from `197.88` → **`290.23 Mops/s`** (**`1.47×`**) with 6/6 frontend tests passing.

---

## 2. Preserved Negative Results (Falsifiers Triggered & Reverted)

| Experiment | Measured Delta vs. Control | Root Cause | Action |
| :--- | :--- | :--- | :--- |
| `t81 mcp serve` SHA3-512 AST policy cache | `+8.7%` on 100-hash `.apl` (`-7.6%` on 1-hash) | Parsing is `~21 µs`; file I/O + `SHA3-512` dominate (`~223 µs`). | Reverted (`< 15%` falsifier). |
| Manual 25-lane Keccak-f[1600] unrolling (`crypto/sha3.cpp`) | `0.99×` (`197.95` vs `199.50 MB/s`) | Apple Clang `-O3` already unrolls and register-allocates all 25 lanes on ARM64. | Reverted (`< 1.25×` falsifier). |
| Harley-Seal CSA-3 popcount GEMM on ARM64 | `0.57×` vs. `vdotq_s32` (`0.46×` vs. `popcnt`) | GPR↔SIMD transfer + register pressure lose to native NEON `vcntq_u8`. | Kept out of repo; used NEON `vcntq_u8` instead. |
| `T81Prob<27>` vs. `int64_t` saturating log-odds | `37.8×` slower (`115.7` vs `3.06 ns/op`) | Software trit-by-trit `T81Int<27>` + `try/catch` overhead; `int64_t` matches stability/permutation properties (`0/200` drift vs `191/200` for `double`). | Documented scope: ABI fidelity, not throughput. |

---

## 3. Reproducing Locally

```bash
# 1. Dual-Rail CPU SIMD benchmark
cmake --build build --target benchmark_runner -j8
T81_BENCHMARK_VERBOSE_CONSOLE=1 ./build/benchmarks/benchmark_runner \
  --benchmark_filter="BM_(DualRail|Int8|ComputeTritVector_TAnd)" --benchmark_min_time=0.05s

# 2. Heterogeneous CPU/Metal Dual-Rail fused decode + capability report
./examples/research/heterogeneous-dualrail/run_demo.sh --layers=32

# 3. Narrowed TISC static policy verifier + cross-device receipt equivalence
./examples/research/tisc-subgraph-dispatch/run_subgraph_demo.sh
```
