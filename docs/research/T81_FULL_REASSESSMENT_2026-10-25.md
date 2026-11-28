# T81 Foundation — Comprehensive Repository Research & Architecture Reassessment

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Foundation — Comprehensive Repository Research & Architecture Reassessment](#t81-foundation-—-comprehensive-repository-research-&-architecture-reassessment)
  - [A. Executive Assessment](#a-executive-assessment)
    - [Key Reassessment Findings](#key-reassessment-findings)
  - [B. Architecture Inventory](#b-architecture-inventory)
    - [External Dependencies](#external-dependencies)
  - [C. Historical Lineage & Hanoi Research Inheritance](#c-historical-lineage-&-hanoi-research-inheritance)
    - [Archaeological Findings](#archaeological-findings)
  - [D. Hypothesis Assessment (Master Canonical H01–H08 Register)](#d-hypothesis-assessment-master-canonical-h01–h08-register)
    - [H01: Native Ternary Storage & Logic Cost Reduction (Track R)](#h01-native-ternary-storage-&-logic-cost-reduction-track-r)
    - [H02: Balanced Ternary Signed Arithmetic & Rounding (Track R)](#h02-balanced-ternary-signed-arithmetic-&-rounding-track-r)
    - [H03: Base-81 Digit Grouping & Representation Interface (Track R)](#h03-base-81-digit-grouping-&-representation-interface-track-r)
    - [H04: Adaptive Representation Tiering (Track R)](#h04-adaptive-representation-tiering-track-r)
    - [H05: Three-State Algebra Primitives & Architectures (Track R)](#h05-three-state-algebra-primitives-&-architectures-track-r)
    - [H06: Ternary Weights & Activations for AI Inference (Track R)](#h06-ternary-weights-&-activations-for-ai-inference-track-r)
    - [H07: Representation-Independent Governed Execution (Track F)](#h07-representation-independent-governed-execution-track-f)
    - [H08: AI-Assisted Iterative Research & Trustworthy Discovery (Track M)](#h08-ai-assisted-iterative-research-&-trustworthy-discovery-track-m)
    - [E. Historical Experiment Evidence Mapping](#e-historical-experiment-evidence-mapping)
  - [E. Claims Audit](#e-claims-audit)
  - [F. Technical Debt and Risks](#f-technical-debt-and-risks)
  - [G. Research Opportunities](#g-research-opportunities)
  - [H. Next Discriminating Experiments & Research Sequence](#h-next-discriminating-experiments-&-research-sequence)
    - [1. Track R Research Sequence: H04-R01 & H04-R02 (Adaptive Representation Discriminator)](#1-track-r-research-sequence-h04-r01-&-h04-r02-adaptive-representation-discriminator)
    - [2. Track F Candidate: Cross-Platform Deterministic AI Execution & Trace Parity (Candidate 3)](#2-track-f-candidate-cross-platform-deterministic-ai-execution-&-trace-parity-candidate-3)
  - [I. README Change Rationale](#i-readme-change-rationale)
  - [J. Evidence Ledger](#j-evidence-ledger)

<!-- T81-TOC:END -->


**Document ID:** `docs/research/T81_FULL_REASSESSMENT_2026-10-25.md`
**Date:** October 25, 2026
**Status:** Complete / Final Research Assessment
**Author:** Independent Principal Research Engineer & Systems Auditor
**Target Repository:** T81 Foundation (`t81dev/t81-foundation`)
**Pinned Snapshot Baseline:** Commit `962fb5ef24b012f91a1a3d1cb18a12d2145e66f0` (Main Branch)
**Continuity Baseline:** Commit `860558b4ad1cf4160af30860b00b15c46165acd5`

---

## A. Executive Assessment

The T81 Foundation repository represents an **experimental scientific research framework and software engineering substrate** focused on two main research domains:

1. **Track R (Scientific Computing & Alternative Representations):** Investigating whether ternary arithmetic, balanced ternary representations, Base-81 digit grouping, adaptive tiering, 3-state logic primitives, or ternary weight formats produce measurable or theoretically defensible computational, storage, or execution advantages over equivalent binary systems (Canonical Hypotheses H01–H06).
2. **Track F (Foundation Systems Engineering & Governance):** Building and evaluating deterministic virtual execution, content-addressed artifact storage (CanonFS), pre-side-effect policy authorization (Axion), immutable execution provenance, and detached accountable evidence verification (Canonical Hypothesis H07).
3. **Track M (AI-Assisted Research Methodology):** Evaluating prompt-to-code provenance, error rate tracking, and trustworthy research iteration under AI assistance (Canonical Hypothesis H08).

### Key Reassessment Findings

- **Software CPU Ternary Emulation is Falsified & Frozen:** Software ternary arithmetic, Base-81 tryte grouping on binary ALUs, and 2-bit packed ternary CPU matrix multiplication (MatVec) decoding on general-purpose binary vector units do **not** outperform standard binary integer or floating-point SIMD pipelines in execution speed. While 2-bit weight packing saves 75% memory bandwidth, SIMD bit-shuffle decode instruction overhead limits execution speed to 75.3% of INT8 AVX2 in L1/L2 cache (`experiments/packed_ternary_matvec/`). Software CPU ternary optimization is formally **frozen (STOP status)**. This negative finding is scoped strictly to software emulation on binary hardware and does not disprove physical ternary hardware or alternative device representations. 2-bit packing is retained strictly as an efficient on-disk weight storage and interchange format.
- **Accountable Computation is a Protocol, Not a Runtime Requirement:** Comparative experiments (`examples/governance/accountable-computation/` and `experiments/research_001/`) demonstrate that pre-side-effect policy enforcement and offline auditability do not inherently require a custom virtual machine (T81VM/TISC). A conventional WebAssembly runtime (Wasmtime) with host FFI capability boundaries and detached canonical evidence bundles (RFC-00D4) achieves equal 0.0% policy breach rates under test conditions and detached evidence auditability with lower system complexity.
- **Deterministic AI Execution & Provenance is the Primary Validated Capability:** T81's strongest, most defensible contribution is its bit-exact cross-platform determinism engine (DCP), soft-float transcendentals (RFC-0030), content-addressed artifact file system (CanonFS / RFC-00D1), and fail-closed policy authorization engine (Axion).
- **Architectural Framing Shift:** The project is correctly understood as a hypothesis-driven empirical research experiment, not a commercial desktop operating system or a native ternary hardware platform.

---

## B. Architecture Inventory

A comprehensive audit of the primary subsystems in the repository yields the following implementation, testing, and operational status:

| Subsystem | Implemented Role | Key Source Paths | Test & Verification Coverage | Implementation Status |
| :--- | :--- | :--- | :--- | :--- |
| **TISC ISA (v1.9.0)** | 81-opcode instruction set architecture for T81 execution | `include/t81/isa/opcodes.hpp`, `src/isa/` | `tests/cpp/test_isa.cpp`, CTest #391–396 | **Frozen (v1.9.0)** |
| **T81VM Interpreter** | Virtual machine executing TISC bytecode with DCP determinism | `src/vm/vm.cpp`, `include/t81/vm/vm.hpp` | `tests/cpp/vm_determinism_property_test.cpp` | **Beta Runtime** |
| **Axion Policy Kernel** | Pre-dispatch policy evaluation engine (`eval_axion_call`) | `src/axion/`, `include/t81/kernel/axion/` | `tests/cpp/axion_agent_invoke_policy_test.cpp`, CTest #154 | **Alpha Governance** |
| **CanonFS** | Content-addressed storage and foreign filesystem interchange (RFC-00D1) | `src/core/fs/`, `include/t81/fs/` | `tests/cpp/canonfs_test.cpp`, CTest #84–85 | **Beta Identity FS** |
| **T81Lang Tooling** | Frontend lexer, parser, AST, IR generator, and bytecode compiler | `src/lang/`, `include/t81/lang/` | `tests/cpp/test_lang.cpp`, `t81 code check/build` | **Beta Language** |
| **Model Intake Bridge** | GGUF/Safetensors model header inspection & tensor inventory | `src/tools/model/`, `tools/cli/ai/` | `t81_weights_load_test` (CTest #166) | **Metadata Bridge** |
| **Determinism Facilities** | Soft-float math (RFC-0030 CORDIC), bit-exact containers, CanonHash-81 | `src/core/isa/math/`, `vm/determinism/` | CTest #355–362 | **Functional / DCP** |
| **Ternary / Base-81 Codecs** | SWAR SIMD emulation, Base-81 tryte packing, 2-bit codecs | `src/core/codec/`, `src/isa/math/quantization/` | `tests/cpp/test_swar.cpp`, `test_base81_simd.cpp` | **Experimental / Frozen** |
| **Accountable Evidence (RFC-00D4)** | Detached evidence bundle generator & verifier | `examples/governance/accountable-computation/` | `t81_accountable_experiment` (CTest #4) | **Research Reference** |

### External Dependencies
- **Header-Only / Vendored Dependencies:** Asio, nlohmann/json, FTXUI (retrieved via CMake `FetchContent`), OpenSSL (optional for Ed25519 signing).
- **Toolchain Requirements:** C++23 compliant compiler (GCC 13+, Apple Clang 16+, MSVC 2022+), CMake 3.25+, Ninja, Python 3.10+.

---

## C. Historical Lineage & Hanoi Research Inheritance

The historical progression of the T81 computational concepts spans four distinct phases:

```
[ Knuth Seminumerical Algorithms ]
 (Balanced ternary, positional number systems, 3-valued logic)
               |
               v
[ Hanoi CWEB Exploration ]
 (CWEB implementation, 41 historical source blobs, experimental arithmetic)
               |
               v
[ T81Lang / TISC ISA / T81VM Engine ]
 (TISC v1.9.0 opcode frozen specification, C++ interpreter, SWAR ternary vectorization)
               |
               v
[ Axion Policy OS & CanonFS Core ]
 (Fail-closed pre-dispatch governance, content-addressed storage, decision bundles)
               |
               v
[ T81 Foundation Stack ]
 (Deterministic execution, policy-gated AI runtimes, accountable computation framework)
```

### Archaeological Findings

1. **Hanoi CWEB Inheritance:** Code in `experimental/hanoi/` preserves the CWEB-based exploration of ternary state machines. Historical audits identified 41 shared source blobs transferred from Hanoi.
2. **Arithmetic Defect Correction:** Early Hanoi implementations suffered from carry-propagation boundary errors and ambiguous zero representations in signed-digit representations. These were mathematically analyzed and corrected during TISC ISA stabilization (`docs/research/TERNARY_ARITHMETIC_AND_RADIX_COMPARISON.md`).
3. **Transformation of Scope:** The research shifted from attempting to build a general-purpose desktop operating system or native ternary hardware CPU toward evaluating **deterministic, policy-governed runtime boundaries for AI artifacts and accountable evidence**.

---

## D. Hypothesis Assessment (Master Canonical H01–H08 Register)

The project evaluates eight canonical hypotheses under its Master Continuity Charter (`docs/research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md` and `docs/research/continuity/HYPOTHESIS_REGISTER_v1.json`). Canonical identifiers and statements are preserved as authoritative:

### H01: Native Ternary Storage & Logic Cost Reduction (Track R)
- **Formal Claim:** Native ternary storage or logic can reduce normalized cost at equal information capacity compared to binary alternatives.
- **Canonical Domain:** Track R — Scientific Computing & Alternative Representations.
- **Implementation & Evidence Status:** Mathematical density is established ($\log_2 3 \approx 1.585$ bits/trit), but physical hardware implementation remains open. Software emulation on binary ALUs (`experiments/radix_comparison/`) proved software tryte addition is $10.6\times$ slower ($11.7\text{ ns}$ vs $1.1\text{ ns}$) than 64-bit binary ALU addition.
- **Scope & Scope Boundary:** Software CPU ternary arithmetic is **falsified for speed and frozen (STOP)** on general-purpose binary ALUs. Physical hardware density and specialized device claims remain open.

### H02: Balanced Ternary Signed Arithmetic & Rounding (Track R)
- **Formal Claim:** Balanced ternary representation improves signed integer arithmetic or rounding properties.
- **Canonical Domain:** Track R — Scientific Computing & Alternative Representations.
- **Implementation & Evidence Status:** Algebraic symmetry around zero eliminates offset bias. SWAR SIMD emulation (`TNOT_SWAR`, `TAND_SWAR`, `TOR_SWAR` in `include/t81/codec/base81_packed.hpp`) was tested on binary vector units, but digit-unpacking tax offsets SIMD density gains on binary hardware.
- **Scope & Scope Boundary:** Algebraic properties established; software SWAR execution on binary vector units is **falsified for speed and frozen**.

### H03: Base-81 Digit Grouping & Representation Interface (Track R)
- **Formal Claim:** Base-81 4-trit digit grouping yields system-level performance or representation interface advantages.
- **Canonical Domain:** Track R — Scientific Computing & Alternative Representations.
- **Implementation & Evidence Status:** 4-trit grouping reduces software tryte addition latency from $105.3\text{ ns}$ down to $11.7\text{ ns}$ compared to un-grouped software trits. On-the-fly 2-bit packed ternary weight MatVec (`experiments/packed_ternary_matvec/`) saves $75\%$ DRAM bandwidth ($0.25$ bytes/weight), but SIMD VPSHUFB bit-shuffle decode tax limits execution speed to $75.3\%$ of INT8 AVX2 ($12.91$ vs $17.15\text{ GFLOP/s}$) in cache.
- **Scope & Scope Boundary:** System-level 4-trit interface mathematical; 2-bit weight packing **validated for storage reduction; falsified for CPU MatVec execution speed in L1/L2 cache**.

### H04: Adaptive Representation Tiering (Track R)
- **Formal Claim:** Adaptive T81/T243/T729 capacity tiering achieves lower cost than fixed representation.
- **Canonical Domain:** Track R — Scientific Computing & Alternative Representations.
- **Implementation & Evidence Status:** Historical Hanoi tiering heuristics exist in `experimental/hanoi/`. Re-specified under **H04-R01 (Radix × Adaptation Discriminator)** as a $2\times2$ exact trace model (Fixed/Adaptive Binary vs Fixed/Adaptive Ternary).
- **Scope & Scope Boundary:** Hanoi heuristic exists; optimization unproven. H04-R01 provides conditional model support with explicit counterexamples (e.g. $M=256$, $C_3/C_2=1.5$ yields higher cost for ternary), not physical hardware proof.

### H05: Three-State Algebra Primitives & Architectures (Track R)
- **Formal Claim:** Specified three-state primitives reduce circuit or system cost compared to an equivalent Boolean implementation.
- **Canonical Domain:** Track R — Scientific Computing & Alternative Representations.
- **Implementation & Evidence Status:** Evaluated in hardware RTL (`rtl/core/t81_top.sv`) and software VM in-kernel policy coupling (`src/axion/` and `src/vm/vm.cpp`). In-VM pre-dispatch policy evaluation (`eval_axion_call`) prevents unauthorized side effects ($0.0\%$ breach rate under test conditions).
- **Scope & Scope Boundary:** Exploratory. In-VM policy evaluation is established for T81VM, but host capability boundaries in Wasm provide equivalent protection.

### H06: Ternary Weights & Activations for AI Inference (Track R)
- **Formal Claim:** Ternary weights or activations improve accuracy-constrained inference versus equally optimized binary low-bit alternatives.
- **Canonical Domain:** Track R — Scientific Computing & Alternative Representations.
- **Implementation Status & Evidence:** On-disk 2-bit weight format (`.t81w`) and detached evidence verification (RFC-00D4) implemented in `tools/cli/ai/` and `examples/governance/accountable-computation/`.
- **Scope & Scope Boundary:** Plausible domain for storage and memory-bandwidth constrained inference; CPU execution speedup disproven on binary SIMD.

### H07: Representation-Independent Governed Execution (Track F)
- **Formal Claim:** Representation-independent governed execution improves auditability, pre-side-effect policy enforcement, and deterministic execution trace replay.
- **Canonical Domain:** Track F — Foundation Systems & Governance Engineering.
- **Implementation Status & Evidence:** Implemented and validated in T81VM Deterministic Core Profile (DCP), soft-float transcendentals (RFC-0030 CORDIC math), CanonFS content addressing (RFC-00D1), and detached evidence bundles (RFC-00D4).
- **Scope & Scope Boundary:** **Established for DCP determinism, CanonFS byte identity, and detached evidence auditability**. Proves that accountable computation is a runtime-agnostic protocol, not a custom VM requirement.

### H08: AI-Assisted Iterative Research & Trustworthy Discovery (Track M)
- **Formal Claim:** AI-assisted research iteration improves trustworthiness and discovery under an independently auditable process.
- **Canonical Domain:** Track M — AI-Assisted Research Methodology.
- **Implementation Status & Evidence:** Methodological process tracking prompt-to-code provenance, error rate recording, session handoffs, and CanonFS artifact lineage across research iterations.
- **Scope & Scope Boundary:** **Established for provenance tracking and session continuity**.

---

### E. Historical Experiment Evidence Mapping

To maintain clarity without renumbering or altering the canonical H01–H08 register, empirical benchmark experiments in the repository map to the hypothesis framework as follows:

| Benchmark / Experiment | Target Surface / Command | Evaluated Hypotheses | Primary Findings & Scoped Verdicts |
| :--- | :--- | :--- | :--- |
| **Radix Comparison** | `experiments/radix_comparison/` | **H01, H03** | Software tryte addition is $10.6\times$ slower than native 64-bit binary ALU addition. Falsifies software CPU ternary arithmetic speedup. |
| **Packed Ternary MatVec** | `experiments/packed_ternary_matvec/` | **H02, H03, H06** | 2-bit packed weight format saves $75\%$ memory footprint, but SIMD bit-shuffle decode tax limits execution speed to $75.3\%$ of INT8 AVX2 in cache. |
| **Accountable Computation (Baseline-X)** | `examples/governance/accountable-computation/` & `experiments/research_001/` | **H04, H05, H06, H07** | Wasmtime + OPA host FFI capability baseline (Arm iii) achieves identical $0.0\%$ breach rates and $100\%$ detached evidence auditability with $10\times$ lower host C++ LOC. Proves accountable computation is a protocol, not a runtime requirement. |
| **Cross-Platform Determinism (DCP)** | `tests/cpp/vm_determinism_property_test.cpp` | **H07** | Bit-exact cross-platform execution and RFC-0030 soft-float transcendentals pass $100\%$ of property-based tests across x86_64, ARM64, and WebAssembly. |
| **CanonFS Byte Identity & Interchange** | `src/core/fs/` & CTest #84–85 | **H07, H08** | Content-addressed storage (CanonHash-81) and foreign filesystem interchange (RFC-00D1) deliver immutable byte identity across systems. |

---

## E. Claims Audit

An audit of claims in former documentation against implemented code and test evidence reveals:

| Category | Claim Summary | Empirical Status | Source Evidence | Reassessment Finding |
| :--- | :--- | :--- | :--- | :--- |
| **ISA / Runtime** | "TISC v1.9.0 is a frozen, deterministic instruction set." | **SUPPORTED** | `include/t81/isa/opcodes.hpp`, CTest #391–396 | Core 81 opcodes and bit-exact execution are fully implemented and tested. |
| **Policy Governance** | "Axion policy checks prevent unauthorized side effects before dispatch." | **SUPPORTED** | `src/axion/`, CTest #154 | Pre-dispatch `eval_axion_call` fails closed and logs policy denials before state mutation. |
| **Artifact Storage** | "CanonFS provides content-addressed artifact storage and interchange." | **SUPPORTED** | `src/core/fs/`, CTest #84–85 | CanonFS import, export, and foreign interchange (RFC-00D1) function as specified. |
| **Deterministic Soft-Float** | "RFC-0030 soft-float transcendentals provide cross-platform parity." | **SUPPORTED** | `src/core/isa/math/`, CTest #355 | Soft-float CORDIC math provides bit-exact parity across x86_64 and ARM64. |
| **Ternary CPU Speedup** | "Software Base-81 / SWAR ternary execution accelerates computation on binary CPUs." | **CONTRADICTED** | `experiments/radix_comparison/` | Software tryte addition is $10.6\times$ slower than native 64-bit binary ALU addition. |
| **MatVec Inference** | "2-bit packed ternary MatVec executes faster than INT8 AVX2." | **CONTRADICTED** | `experiments/packed_ternary_matvec/` | VPSHUFB 2-bit MatVec reaches $75.3\%$ of INT8 AVX2 speed due to SIMD decode instruction tax. |
| **Custom VM Requirement** | "Accountable computation requires a custom VM and kernel." | **UNSUPPORTED** | `examples/governance/accountable-computation/` | Wasmtime + OPA + RFC-00D4 evidence schema provides equivalent policy and audit guarantees. |
| **Hardware Platform** | "T81 runs on physical ternary microprocessors." | **UNSUPPORTED** | Repository inspection | No physical ternary hardware exists in the codebase; all execution uses binary hardware emulation. |

---

## F. Technical Debt and Risks

1. **Process Security & Shell Traversal:**
   - Command execution in CLI tools should continue avoiding `std::system` and shell expansion to prevent command injection risks.
   - External processes must strictly use `t81::support::run_process` (`include/t81/support/process.hpp`) with explicit argument vectors.
2. **Windows PATH Parsing:**
   - PATH environment variable entries on Windows may contain surrounding quotes (e.g. `"C:\Program Files"`), requiring string trimming before passing to `std::filesystem::path`.
3. **Durable Witness Isolation:**
   - Security audits (`experiments/durable_witness/`) established that an isolated SQLite WAL witness store remains vulnerable to snapshot rollback without an external monotonic checkpoint or log. HMAC receipts must sign all metadata fields (`witness_id`, `payload_hash`, `total_amount`, `timestamp`).
4. **Build Warnings:**
   - Apple Clang compiler warnings regarding dangling references to temporaries in `examples/research/validated-descriptors/runner.cpp` line 144 should be cleaned up during maintenance.

---

## G. Research Opportunities

1. **Galois Field $\text{GF}(3^9)$ vs $\text{GF}(2^8)$ Storage & Parity:** Evaluating Base-81 Galois Field arithmetic ($\text{GF}(3^9)$) for Reed-Solomon storage parity in CanonFS versus hardware-accelerated binary Galois Field instructions (`GF2P8AFFINEINVQB`).
2. **Multi-Precision Signed-Digit (RSD) Crossover:** Microbenchmarking Avizienis carry-free addition across 128-bit to 512-bit integer widths to identify the exact crossover point where carry-free digit addition overcomes digit-unpacking overhead on binary SIMD vector units.
3. **Deterministic AI Execution & Trace Provenance:** Validating soft-float transcendentals (RFC-0030) and trace hashing across large neural network inference execution graphs.

---

## H. Next Discriminating Experiments & Research Sequence

Research experiments proceed along established sequence tracks defined in the Master Continuity Charter:

### 1. Track R Research Sequence: H04-R01 & H04-R02 (Adaptive Representation Discriminator)

- **H04-R01 Packet Status (Conditional Support):** H04-R01 specifies a $2\times2$ exact trace discriminator (Fixed Binary, Adaptive Binary, Fixed Ternary, Adaptive Ternary) to separate radix effects from adaptive precision. Stage 1 evaluations (`docs/research/continuity/H04_RADIX_ADAPTATION_DISCRIMINATOR.md` and `docs/research/continuity/probes/storage_boundary.py`) establish **conditional mathematical model support with explicit counterexamples** (e.g. at capacity $M=256$ and per-element cost ratio $C_3/C_2=1.5$, ternary requires 6 trytes = 9 cost units vs binary 8 bits = 8 cost units, despite $1.5 < \log_2 3 \approx 1.585$). This is a mathematical/model derivation under explicit symbol assumptions, not physical hardware or speed proof.
- **H04-R02 Next Experiment (Four-Arm Exact Trace Ledger & Crossover Analysis):**
  - **Objective:** Construct the full 4-arm exact trace ledger over declared integer trace families (constant small, constant large, phased sizes, alternating sizes) to compute the exact symbol, transition, and conversion crossover surface between adaptive binary (AB) and adaptive ternary (AT).
  - **Primary Metric:** Exact symbol-slot counts and mode transition work vectors as a function of $C_3/C_2$ and conversion overhead.
  - **Decision Boundary:** Reject ternary adaptive advantage if AT yields higher cost than AB across tested capacity domains, or if advantage vanishes under equalized mode metadata packing.

### 2. Track F Candidate: Cross-Platform Deterministic AI Execution & Trace Parity (Candidate 3)

- **Objective:** Determine whether T81's Deterministic Math Subsystem (RFC-0030 soft-float transcendentals) and Canonical Trace Cache (RFC-0028) can achieve **bitwise execution trace parity** across x86_64, ARM64, and WebAssembly targets under Axion policy governance, while incurring a **$<5\%$ runtime performance tax** relative to non-deterministic host `libm` floating-point execution.
- **Target Subsystems:** `tests/determinism/` and `vm/determinism/`.
- **Evaluation Workload:** Softmax, LayerNorm, and multi-layer MLP token generation execution traces across $100,000$ adversarial floating-point inputs.
- **Independent Oracle:** Bitwise trace hash comparison via `CanonHash81`.

---

## I. README Change Rationale

The root `README.md` was rewritten to align project documentation with empirical source code evidence:

1. **Clear Project Identity:** Replaced vague architectural promises with a precise description of T81 Foundation as an experimental research program investigating deterministic execution, policy governance, provenance, and alternative computational representations.
2. **Honest Hypothesis Ledger:** Included explicit results for H01–H08, openly disclosing negative experimental findings (such as software ternary CPU arithmetic speedup being falsified).
3. **Reproducible Instructions:** Provided verified, exact CMake, Ninja, CTest, and Python regression commands that execute cleanly in the codebase today.
4. **Removed Promotional Badges & Claims:** Removed unsupported performance badges, claims of physical ternary hardware, and assertions that custom VMs are strictly necessary for accountable evidence.

---

## J. Evidence Ledger

- **Main Commit SHA:** `962fb5ef24b012f91a1a3d1cb18a12d2145e66f0`
- **Continuity Commit SHA:** `860558b4ad1cf4160af30860b00b15c46165acd5`
- **Verification Commands Executed:**
  - `cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_TESTS=ON`
  - `ninja -C build t81 t81_radix_comparison t81_packed_ternary_matvec`
  - `python3 scripts/ci/run_research_regressions.py --build-dir build` (Result: 8/8 tests passed in 4.68s)
- **Primary Source Files Examined:**
  - `include/t81/isa/opcodes.hpp`
  - `src/vm/vm.cpp`
  - `src/axion/`
  - `src/core/fs/`
  - `experiments/radix_comparison/`
  - `experiments/packed_ternary_matvec/`
  - `examples/governance/accountable-computation/`
  - `docs/research/TERNARY_RESEARCH_CLOSURE_AND_NEXT_HYPOTHESIS.md`
  - `docs/research/consolidation/README.md`
