# T81 Foundation Architecture — Research Gaps & Open Scientific Questions

## Executive Overview

This document provides a systematic analysis of unresolved research hypotheses, unverified claims, and architectural gaps in the T81 Foundation computing stack.

To maintain scientific integrity, four distinct classes of gaps are recognized and evaluated:
1. **Implementation Gap**: The specification is adequate, but code realization is missing, stubbed, or incomplete.
2. **Verification Gap**: Code realization exists, but test coverage, platform verification, or correctness oracles are missing or failing.
3. **Measurement Gap**: Correct behavior is established, but comparative advantage against optimized binary controls has not been measured under fair baseline conditions.
4. **Architectural Gap**: Existing language, ISA, representation, or execution semantics are fundamentally inadequate for the computational model.

---

## 1. Summary of Prior Scientific Investigations (01–08)

The T81 research roadmap is anchored by eight formal scientific investigations. Their verified dispositions set the baseline for this gap analysis:

| Investigation | Domain / Subject | Final Scientific Disposition | Supporting Artifacts |
| :--- | :--- | :--- | :--- |
| **Investigation 01** | Numerical Correctness Assessment | **BOUNDED** | `/docs/research/NUMERICAL_SCIENTIFIC_ASSESSMENT.md` |
| **Investigation 02** | Exact Rational Conversion Feasibility | **REVISE** | `/docs/research/EXACT_RATIONAL_CONVERSION_FEASIBILITY.md` |
| **Investigation 03** | Accountable Computation Experiment Framework | **VALIDATED** | `/docs/research/ACCOUNTABLE_COMPUTATION_EXPERIMENT_FRAMEWORK.md` |
| **Investigation 04** | Independent Validation of Rational Conversion | **VALIDATED** | `/INDEPENDENT_RATIONAL_CONVERSION_VALIDATION.md` |
| **Investigation 05** | Balanced Ternary Radix & Arithmetic Comparison | **COMPLETED** | `/BALANCED_TERNARY_COMPARATIVE_RESEARCH.md` |
| **Investigation 06** | Audit of Ternary Comparative Performance Claims | **AUDITED** | `/TERNARY_COMPARATIVE_CLAIMS_AUDIT.md` |
| **Investigation 07** | Ternary Dot-Product Hardware Feasibility | **NOT SUPPORTED** | `/experiments/ternary_hardware_feasibility/FINDINGS_REPORT.md` |
| **Investigation 08** | Integrated System Advantage Assessment | **MIXED** (Assurance: **SUPPORTED**; Computational: **NOT SUPPORTED**; Compositional: **INCONCLUSIVE**) | `/docs/research/INTEGRATED_SYSTEM_ADVANTAGE_REPORT.md`, `/docs/research/INVESTIGATIONS_07_08_CORRECTIVE_ASSESSMENT.md` |

---

## 2. Four Classes of Gaps in T81 Architecture

### 2.1 Class 1: Implementation Gaps

*Definition: Specifications exist in normative RFCs, but source code is missing, stubbed, or incomplete.*

1. **Google Axion Cloud Lowering Target (RFC-0056)**
   - **Specification**: RFC-0056 specifies ARM64 TISC lowering, Titanium offload governance, and multi-region epoch synchronization.
   - **Gap**: Phase 1–3 code realization on native ARM64 Google Axion cloud instances is pending. Current execution uses host x86_64 AVX2 / ARM64 NEON emulated instructions.
   - **Source Path**: `/spec/rfcs/RFC-0056-google-axion-t81-integration.md`.

2. **Network & Async VM Opcodes (`NSend`, `NRecv`, `VWait`, `VYield`)**
   - **Specification**: Opcodes `0x7B`–`0x7E` are registered in TISC ISA v1.1.0 (`/spec/tisc/opcode-registry.md`).
   - **Gap**: VM interpreter handles trap execution by failing closed with `Trap::Unimplemented`.
   - **Source Path**: `/vm/vm.cpp`.

3. **Neural Forward/Backward Builtin Opcodes (`TNeuralFwd`, `TNeuralBwd`)**
   - **Specification**: Opcodes `0xA5`–`0xA6` are registered in TISC ISA.
   - **Gap**: Stubbed in `/vm/vm.cpp` as `Trap::Unimplemented`. Neural execution uses higher-level `TWMATMUL`, `TATTN`, and `QMATMUL` opcodes instead.

4. **Operator-Shell Slice6 Command Surface (RFC-00CF)**
   - **Specification**: RFC-00CF draft defines operator-shell surface over CanonFS.
   - **Gap**: Command handlers are draft/unmerged.

---

### 2.2 Class 2: Verification Gaps

*Definition: Code realization exists, but test coverage, platform verification, or correctness oracles are incomplete.*

1. **Bare-Metal & QEMU/EFI Guest Target Verification in CI**
   - **Implementation**: TernaryOS HAL and QEMU/EFI startup snapshots exist in `/ternaryos/`.
   - **Gap**: QEMU/EFI guest target tests (`t81_ternaryos_qemu_*`) require Homebrew LLVM clang and are disabled in Linux CI workflows (`CMakeLists.txt` condition).
   - **Source Path**: `/ternaryos/CMakeLists.txt`.

2. **Cross-Architecture Vector Parity Verification (x86_64 AVX2 vs ARM64 NEON)**
   - **Implementation**: Vector kernels exist in `/include/t81/simd/simd.hpp` for both AVX2 and NEON.
   - **Gap**: Multi-architecture bit-exact parity is tested on local host platforms, but automated x86_64 ↔ ARM64 cross-runner verification in CI is pending runner availability.
   - **Source Path**: `/tests/cpp/t81_simd_add_helpers_test.cpp`.

3. **Extended Non-Llama Model Family GGUF Execution Evidence**
   - **Implementation**: `/tools/model/gguf_import_bridge.cpp` implements compatibility profiles for `gemma`, `mistral`, `phi3`, `qwen2`.
   - **Gap**: Real model execution evidence currently uses `tiny-random-llama`. Non-llama family proofs use synthetic tensor fixtures.
   - **Source Path**: `/tests/cpp/gguf_import_bridge_test.cpp`.

---

### 2.3 Class 3: Measurement Gaps

*Definition: Behavior is verified correct, but comparative performance advantage against optimized binary controls is unmeasured or unsubstantiated.*

1. **End-to-End Task Throughput vs Binary LLM Runtime Controls**
   - **Established Behavior**: `t81 ai task answer-fixed` and `t81 ai inference run` execute natively on VM with ternary weights.
   - **Gap**: Comparative execution throughput (tokens/sec) against optimized binary INT4/INT8 llama.cpp / vLLM baselines on identical CPU hardware has not been measured under controlled, equivalent memory bandwidth conditions.
   - **Reference**: `/docs/research/TERNARY_INFERENCE_CROSS_ARCH_BREAK_EVEN_REPORT.md`.

2. **Memory Bandwidth & Cache Efficiency of Packed Trit Representation**
   - **Established Behavior**: `t81::T81` packs 128 trits into 256 bits (2 bits/trit), achieving a 1.585x theoretical memory compression over 8-bit binary weights.
   - **Gap**: Actual hardware cache-miss reduction and DRAM bandwidth savings during full-scale model weight streaming have not been measured on physical memory buses.

---

### 2.4 Class 4: Architectural Gaps

*Definition: Fundamental limitations arising from representation, ISA, or binary-host emulation model.*

1. **Physical Silicon Gate Penalty for 3-State Logic on Binary CMOS (Investigation 07)**
   - **Limitation**: Standard CMOS physical processes are binary (`0V` vs `VDD`). Synthesizing 3-state ternary logic (`-1`, `0`, `+1`) on binary CMOS requires dual-rail differential signaling or 2-bit encoding, resulting in a **1.4x–1.8x area penalty** and **1.3x–1.5x power penalty** at the standard-cell level compared to binary AND/OR gates.
   - **Evidence**: ASIC synthesis benchmark report in `/experiments/ternary_hardware_feasibility/FINDINGS_REPORT.md`.
   - **Conclusion**: Computational performance advantage cannot be claimed from radix efficiency alone on conventional binary Silicon.

2. **Computational Speedup vs Assurance Advantage (Investigation 08)**
   - **Limitation**: Executing ternary virtual machines on binary host CPUs incurs an interpreter/decoding translation tax.
   - **Evidence**: Investigation 08 demonstrated that T81's primary system advantage lies in **Assurance** (bit-exact determinism, policy-gated state transitions, accountable audit trails) rather than pure **Computational** raw FLOPS speedups.
   - **Evidence Path**: `/docs/research/INVESTIGATIONS_07_08_CORRECTIVE_ASSESSMENT.md`.

---

## 3. Scientific Coordination & Investigation 09 Integration

Investigation 09 is currently exploring three-state computational expressiveness and executable footprint.

### Guidance for Interpreting Investigation 09 Evidence:
1. **provisional Status**: Unmerged or ongoing Investigation 09 data must be treated as provisional hypotheses.
2. **Distinguishing Root Causes**:
   - Compiler lowering limitations (e.g. unreachable opcodes in T81Lang) must not be confused with ISA flaws.
   - Binary host emulation overhead must not be interpreted as proof against native ternary ISA design.
   - Memory representation density (1.585 bits/trit) must not be confused with host compute throughput.

---

## 4. Scientifically Justified Future Experiments

Based on established evidence, the following future experiments are scientifically justified:

1. **Hardware Memory Bandwidth Streaming Experiment**: Measure DRAM byte throughput during large-scale ternary weight loading vs INT8/FP16 models on identical memory channels.
2. **Deterministic Governance Overhead Quantification**: Measure the precise microsecond latency tax of Axion policy evaluation and CanonFS hashing per inference token.
3. **ARM64 Native Lowering & Axion Cloud Verification**: Benchmark native ARM64 TISC lowered binaries on physical Neoverse V2 / Axion cloud cores.
