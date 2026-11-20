# T81 Research & Engineering Status — 2026-11-19 (`v2.0.1` + Dual-Rail Heterogeneous & Full-Stack VM Integration)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Research & Engineering Status — 2026-11-19 (`v2.0.1` + Dual-Rail Heterogeneous & Full-Stack VM Integration)](#t81-research-&-engineering-status-—-2026-11-19-`v201`-+-dual-rail-heterogeneous-&-full-stack-vm-integration)
  - [1. Verified Engineering & Research Wins (`cfe7a269a` → `16d083136`)](#1-verified-engineering-&-research-wins-`cfe7a269a`-→-`16d083136`)
  - [2. Preserved Negative Results (Falsifiers Triggered & Reverted)](#2-preserved-negative-results-falsifiers-triggered-&-reverted)
  - [3. Explicit Limits & Next Concrete Steps](#3-explicit-limits-&-next-concrete-steps)

<!-- T81-TOC:END -->


**Date:** 2026-11-19  
**Frozen Release Tags:** `v2.0.0` (`7d4582ce5`), `v2.0.1` (`f3b41b583`)  
**Current `main` Head:** `16d083136`  
**Status:** Non-normative operational snapshot; preserves all active research holds (`F01 M1–M6 STOP`, `SCG-08 Gate 2 NO-GO`, `H01/10A BLOCKED`, `AXION-AI-001 HD01–HD06 open`) and zero locked `include/t81/**/*.hpp` (`PUBLIC_API_LOCK.json`) mutations.

---

## 1. Verified Engineering & Research Wins (`cfe7a269a` → `16d083136`)

1. **Release & Packaging (`v2.0.1` @ `f3b41b583`, PR `#835` closed as superseded):**
   - `Python Wheels` (`44486838709`) passed **6/6** jobs (`sdist`, `macos-arm64`, `macos-x86_64`, `linux-x86_64`, `linux-aarch64`, `windows-amd64`).
   - Root directory cleaned (`59` entries; `check_root_structure.py` passing); `pyproject.toml`, `CMakeLists.txt`, and `include/t81/config.hpp` bumped to `2.0.1`.
2. **Dual-Rail `(P, N)` SIMD Codec & Kernels (`cfe7a269a`, `benchmarks/BM_DualRailTrit.cpp`):**
   - NEON `vpaddq_u8` bitmask pack (`3.78 µs` at `N = 65536`, **14.4×** faster than scalar, **21.3×** faster than `ComputeTritVector::from_trits`) dropped break-even reuse factor `K*` vs. competent NEON `vdotq_s32` `int8` from `~70` → **`5.4`** (1-pack) / **`10.7`** (2-pack).
   - Kernel throughput (`N = 65536`, Apple M2): `DualRail64` dot = `76.16 Gops/s` (**`1.82×`** vs. NEON `vdotq_s32`); Kleene AND = `157.98 Gops/s` (**`3.80×`** vs. `int8`, **`6.68×`** vs. 2-bit interleaved `ComputeTritVector`). Shared on Issue `#555`.
3. **Zero-Copy CPU ↔ Metal GPU Fused Decode & 4-Way Silicon Crossover (`45b5b6b61`, `123745f2b`, `examples/research/heterogeneous-dualrail/`):**
   - 32 distinct layers (`1.98B` weights: `473 MB` DualRail vs. `1.89 GB` `int8`) fused into **one** Metal command buffer over `MTLResourceStorageModeShared` reached **`5.55 ms`** (`180.02 tok/s`, **`89.10 GB/s`** ≈ 89% of Apple M2 `100 GB/s` DRAM peak; **`3.45×`** vs. unfused 64-CB GPU, **`3.79×`** vs. CPU DualRail, **`25.13×`** vs. 1-thread CPU `int8`).
   - Against Apple production frameworks on Apple M2: T81 DualRail GPU beat **`MPSMatrixMultiplication` (FP16)** by **`72.0×`** (`1×4096×4096` decode), **`42.4×`** (`1×4096×11008` decode), **`2.90×`** (`256×2048×2048` prefill, **`3.32 TOPS`**), and **`1.55×`** (`512×2048×2048`, **`3.15 TOPS`**), and beat **Apple `AMX` (`Accelerate cblas_sgemm` FP32)** by **`4.1×`–`141.7×`** (compute-bound FP16/NPU engines overtake above `M >= 1024`). Added 256-bit AVX2 `vpshufb` popcount path for x86_64.
   - **Cross-Device Determinism:** `SHA3-512(CPU int8) == SHA3-512(CPU DualRail) == SHA3-512(Fused Metal GPU DualRail)` bit-for-bit on every tested shape.
4. **Narrowed TISC Subgraph Verifier, BitNet b1.58 Demo & Live MCP Control Plane (`c322ab8e2`, `70b9592f5`, `0809681b8`):**
   - `examples/research/tisc-subgraph-dispatch/`: 55-line static verifier (`9.92 µs`) enforcing `WeightsLoad → ChkShape → AxVerify → TMatMul → Halt`, fail-closed negative tests, and identical `SHA3-512` execution receipts across CPU and Metal GPU.
   - `examples/research/bitnet-verifiable-token/`: BitNet b1.58 absmean quantization → `SHA3-512` 1-bit tamper detection → TISC verification → bit-exact 8-token autoregressive output & receipt across CPU and Metal GPU.
   - `tools/cli/core/mcp_server.cpp`: Added **`t81.verify_subgraph`** as the 11th native in-process MCP tool (`test_native_mcp.py`: `11` tools, `all_checks_passed: true`, `~0.108–0.123 ms` warm `gate_check`).
   - `docs/research/DUAL_RAIL_HETEROGENEOUS_TISC_NOTE.md`: Pinned empirical summary covering all positive and negative outcomes.
5. **T81Lang Lexer & Full-Stack `CanonFS → Axion → TISC → T81VM → Dual-Rail` Integration (`dbba8f11a`, `f38dbe956`, `01843ed43`, `16d083136`):**
   - `lang/frontend/lexer.cpp`: 256-byte ASCII LUT + length-switched keyword lookup accelerated `BM_Lexer_AllTokens` by **`+47%`** (`197.88` → **`290.23 Mops/s`**), 6/6 frontend tests passing.
   - **Seam 1 (`f38dbe956`, `vm/tensor_helpers.cpp`):** Dual-Rail fast path inside `tensor_matmul_2d` accelerated `T81VM` `Opcode::TMatMul` on ternary `T729DynamicTensor` by **`24,253×`** (`64×128×64`: `3.24 s` → `0.134 ms`) and **`35,483×`** (`128×256×128`: `25.48 s` → `0.718 ms`), with automatic fallback for non-ternary floats.
   - **Seam 2 (`16d083136`, `vm/vm.cpp`, `vm/tensor_helpers.cpp`):** Bounded 64-entry deterministic FIFO `CanonDualRailRhsCache` keyed strictly by `CanonFS` content hash on `Opcode::TLoadHash` → `Opcode::TMatMul`.
   - **Seam 3 (`01843ed43`, `16d083136`, `examples/research/full-stack-t81-pipeline/`):** End-to-end verified pipeline (`t81 code check verified_inference.t81` + live `t81 mcp serve` `t81.verify_subgraph` + `CanonFS → Axion → TISC → T81VM → DualRail_TMatMul`, `oracle_exact_match: true`, `unauthorized_trapped_security_fault: true`).

---

## 2. Preserved Negative Results (Falsifiers Triggered & Reverted)

- **`t81 mcp serve` SHA3-512 AST policy cache:** `+8.7%` on 100-hash `.apl` (`-7.6%` on 1-hash) — file I/O + `SHA3-512` dominate `~21 µs` parse time. Reverted cleanly.
- **Manual 25-lane Keccak-f[1600] unrolling (`crypto/sha3.cpp`):** `0.99×` (`197.95` vs `199.50 MB/s`) — Apple Clang `-O3` already unrolls and register-allocates all 25 lanes on ARM64. Reverted cleanly.
- **Harley-Seal CSA-3 popcount GEMM on ARM64:** `0.57×` vs `vdotq_s32` (`0.46×` vs `popcnt`) — GPR↔SIMD transfer + register pressure lose to native NEON `vcntq_u8`.
- **`T81Prob<27>` vs. `int64_t` saturating log-odds:** `37.8×` slower (`115.7` vs `3.06 ns/op`) with identical stability/permutation guarantees (`0/200` drift vs `191/200` for `double`); value on binary silicon is ABI fidelity, not throughput.

---

## 3. Explicit Limits & Next Concrete Steps

- **Limits:** `NativeFormat::T3_K` decode in `include/t81/tensor/native.hpp` still triggers `T729TensorBase::build_canonical_fixed_cache_` (`div_mod<384>` per element); `CanonicalFixed` avoids this via `from_canonical_fixed()`. `BitNet b1.58` demo uses synthetic quantized weights rather than a downloaded real-world checkpoint.
- **Next high-leverage steps:**
  1. Ingest a real open-weight BitNet b1.58 / ternary checkpoint into `CanonFS` → `T81VM` / Fused Metal GPU to generate real English tokens with bit-exact cross-device `SHA3-512` receipts.
  2. Gather independent x86_64 AVX2/AVX-512 and CUDA/Vulkan cross-checks against `BM_DualRailTrit` and `heterogeneous-dualrail`.
  3. Keep all scientific holds (`F01`, `SCG-08 Gate 2`, `H01/10A`, `AXION-AI-001`) respected until explicitly authorized.
