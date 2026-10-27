# T81 Foundation — Systems Research in Accountable & Deterministic Computation

**Deterministic execution · Content-addressed artifacts · Pre-dispatch governance · Empirical representation limits**

[![License](https://img.shields.io/badge/license-Apache_2.0-blue.svg)](LICENSE)
[![ISA](https://img.shields.io/badge/ISA-v1.9.0_Frozen-blue.svg)](spec/tisc/opcode-registry.md)
[![Status](https://img.shields.io/badge/Research-Active-green.svg)](docs/research/RESEARCH_CHARTER.md)

T81 Foundation is an experimental scientific research project and systems engineering substrate investigating the boundaries of **deterministic execution, policy-governed computation, content-addressed artifact provenance, and alternative computational representations** (including balanced ternary, Base-81 trytes, and packed low-bit tensors).

Rather than claiming a settled architecture or production operating system, T81 operates as an **empirical research program** that formulates falsifiable hypotheses, implements executable reference systems, runs comparative benchmarks, and preserves both positive and negative results.

---

## 1. Research Motivation & Scientific Questions

Contemporary computing relies almost exclusively on binary CMOS representations, IEEE 754 floating-point arithmetic, and implicit post-hoc authorization. While efficient, these foundations create challenges for cross-platform state reproduction, verifiable execution traces, and pre-side-effect policy enforcement in autonomous or probabilistic AI runtimes.

T81 investigates one central scientific question:

> **Can alternative representations, deterministic runtime execution, or explicit policy governance produce measurable or theoretically defensible advantages over standard binary pipelines?**

### Master Canonical Hypothesis Register (H01–H08)

The project evaluates eight canonical hypotheses under its Master Continuity Charter (`docs/research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md`):

- **Track R — Scientific Computing & Alternative Representations:**
  - *H01 (Native Ternary Cost & Density):* Can native ternary storage or logic reduce normalized cost at equal information capacity vs binary? *(Density established; software CPU arithmetic falsified/frozen)*
  - *H02 (Balanced Ternary Arithmetic):* Does balanced ternary improve signed arithmetic or rounding properties? *(Algebraic symmetry established; software SWAR SIMD falsified/frozen for speed)*
  - *H03 (Base-81 Digit Grouping):* Does Base-81 4-trit digit grouping yield system-level interface or storage advantages? *(Validated for 2-bit weight storage; CPU MatVec speed limited by decode tax in cache)*
  - *H04 (Adaptive Representation Tiering):* Does adaptive T81/T243/T729 capacity tiering achieve lower cost than fixed representation? *(Refined by H04-R01 exact trace model; conditional support with explicit counterexamples)*
  - *H05 (Three-State Algebra Primitives):* Do three-state algebra primitives reduce circuit or system cost vs Boolean logic? *(Exploratory; in-VM policy coupling established for T81VM)*
  - *H06 (Ternary AI Inference):* Do ternary weights/activations improve accuracy-constrained inference vs low-bit binary? *(Plausible for storage/bandwidth; CPU speedup disproven on binary SIMD)*
- **Track F — Foundation Systems & Governance Engineering:**
  - *H07 (Governed Execution & Auditability):* Does representation-independent governed execution improve auditability, policy enforcement, and trace replay? *(Established for DCP determinism, CanonFS byte identity, and detached evidence protocols)*
- **Track M — AI-Assisted Research Methodology:**
  - *H08 (AI-Assisted Research Iteration):* Does AI-assisted research iteration improve trustworthiness and discovery under auditable provenance? *(Established for prompt-to-code provenance and session continuity)*

---

## 2. Demonstrated System Capabilities

The repository contains concrete, functional C++23 software components validated through CTest targets and regression harnesses:

| Component | Implemented Role | Key Source Paths | Evidence & Status |
| :--- | :--- | :--- | :--- |
| **TISC ISA (v1.9.0)** | 81-opcode instruction set for deterministic state machine execution | `include/t81/isa/opcodes.hpp` | **v1.9.0 Frozen** (CTest #391–396) |
| **T81VM Interpreter** | Bit-exact virtual machine executing TISC bytecode | `src/vm/vm.cpp` | **DCP Compliant** (`vm_determinism_property_test`) |
| **Axion Policy OS** | Fail-closed pre-dispatch policy evaluation engine | `src/axion/` | **Alpha Governance** (CTest #154) |
| **CanonFS Storage** | Content-addressed storage and foreign filesystem interchange (RFC-00D1) | `src/core/fs/` | **Beta Identity FS** (CTest #84–85) |
| **T81Lang Tooling** | Lexer, parser, AST, IR generator, and TISC compiler | `src/lang/` | **Beta Language** (`t81 code build`) |
| **Model Intake Bridge** | GGUF/Safetensors tensor inventory & header inspection | `src/tools/model/` | **Metadata Bridge** (CTest #166) |
| **Accountable Evidence** | Detached evidence bundle generator & verifier (RFC-00D4) | `examples/governance/` | **Research Reference** (CTest #4) |

---

## 3. Verified Experimental Results & Hypothesis Ledger

All experimental conclusions in T81 are grounded in reproducible benchmarks and documented research closure reports:

### 1. Ternary CPU Arithmetic & Radix Comparison (Track R: H01, H03)
- **Finding:** Grouping 4 balanced trits into Base-81 trytes (`std::array` stack allocation) reduced software tryte addition latency from $105.3\text{ ns}$ down to $11.7\text{ ns}$.
- **Baseline Comparison:** Native 64-bit binary ALU integer addition executes in $1.1\text{ ns}$ ($10.6\times$ faster).
- **Status:** Software CPU ternary addition is **FALSIFIED for speed / FROZEN** on binary ALUs (`docs/research/TERNARY_ARITHMETIC_AND_RADIX_COMPARISON.md`).

### 2. Packed 2-Bit Ternary MatVec Inference (Track R: H02, H03, H06)
- **Finding:** VPSHUFB nibble-parallel lookup table decoding (`matvec_ternary_2bit_avx2`) achieves $12.91\text{ GFLOP/s}$ on $4096^2$ matrices, saving $75\%$ memory footprint versus FP32 ($0.25$ bytes/weight).
- **Baseline Comparison:** INT8 AVX2 SIMD dot products achieve $17.15\text{ GFLOP/s}$ ($1.33\times$ faster). The SIMD bit-shuffle decode instruction tax offsets memory bandwidth savings in cache.
- **Status:** **VALIDATED for storage footprint; FALSIFIED for CPU MatVec speed in L1/L2 cache** (`docs/research/PACKED_TERNARY_MATVEC_PERFORMANCE_REPORT.md`).

### 3. Accountable Computation Baseline Experiment (Track F: H07)
- **Finding:** Pre-dispatch policy evaluation in T81VM (`eval_axion_call`) achieves $0.0\%$ unauthorized side-effect breach rates under mid-flight policy revocation under test conditions.
- **Baseline Comparison:** A conventional Wasmtime + Open Policy Agent (OPA) host FFI capability baseline (Project Baseline-X) achieved identical $0.0\%$ breach rates and $100\%$ detached evidence auditability with $10\times$ lower host C++ code complexity.
- **Status:** **ESTABLISHED for protocol evidence** — Accountable computation is a runtime-agnostic protocol (RFC-00D4), not a custom VM requirement (`docs/research/consolidation/README.md`).

---

## 4. System Architecture

```
+-----------------------------------------------------------------------------------+
|                            APPLICATION / AI ARTIFACT                              |
|                   (TISC Bytecode, Model Weights, Policy AST)                      |
+-----------------------------------------------------------------------------------+
                                       |
                                       v
+-----------------------------------------------------------------------------------+
|                             AXION POLICY KERNEL                                   |
|             Pre-Dispatch Authorization (`eval_axion_call`) -> Allow / Deny          |
+-----------------------------------------------------------------------------------+
                                       |
                                       v
+-----------------------------------------------------------------------------------+
|                              T81VM DETERMINISTIC CORE                             |
|          Bit-Exact Interpreter (DCP) + Soft-Float Transcendentals (RFC-0030)      |
+-----------------------------------------------------------------------------------+
                                       |
                                       v
+-----------------------------------------------------------------------------------+
|                              CANONFS IDENTITY FS                                  |
|          Content-Addressed Storage (CanonHash-81) + RFC-00D1 Foreign Interchange    |
+-----------------------------------------------------------------------------------+
```

---

## 5. Research Lineage

The concepts investigated in T81 originate from a historical sequence of discrete mathematics and experimental systems research:

1. **Knuth Seminumerical Algorithms:** Foundational theory of balanced ternary representations, positional number systems, and 3-valued logic.
2. **Hanoi CWEB Exploration:** Experimental CWEB implementation of ternary state machines (`experimental/hanoi/`), analyzing 41 historical shared source blobs and arithmetic carry defects.
3. **TISC ISA & T81VM Engine:** Formalization of the frozen v1.9.0 81-opcode ISA, C++ deterministic interpreter, and Base-81 codecs.
4. **Axion & CanonFS Framework:** Integration of pre-side-effect policy governance, content-addressed storage, and detached accountable evidence bundles (RFC-00D4).

---

## 6. Reproducibility & Verification Guide

Follow these step-by-step instructions to build, test, and verify the repository.

### Requirements
- C++23 compliant compiler (GCC 13+, Apple Clang 16+, MSVC 2022+)
- CMake 3.25+ and Ninja
- Python 3.10+
- Git

### 6.1 Build and Run Core Tests

```bash
# Clone the repository
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation

# Configure and build
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_TESTS=ON
cmake --build build --parallel 4

# Run CTest suite
ctest --test-dir build --output-on-failure
```

### 6.2 Run Research Regression Suite

To execute all eight consolidated research regression tests:

```bash
python3 scripts/ci/run_research_regressions.py --build-dir build
```

### 6.3 Try CanonFS Import / Export & Policy Denial

```bash
tmp_dir="$(mktemp -d)"
canon_root="$tmp_dir/.t81_canonfs"
artifact="examples/storage-and-canonfs/canonfs-interchange/v1/model.t81w"
deny_policy="examples/storage-and-canonfs/canonfs-interchange/v1/policy-deny-all.apl"

# Import artifact into CanonFS
./build/t81 canonfs import "$artifact" --canonfs-root "$canon_root" --json

# Extract content-addressed hash
canon_hash="$(./build/t81 canonfs import "$artifact" --canonfs-root "$canon_root" --json | grep -o '"hash": "[^"]*"' | head -n 1 | cut -d'"' -f4)"

# Export artifact by hash
./build/t81 canonfs export "$canon_hash" --canonfs-root "$canon_root" --out "$tmp_dir/restored.t81w" --json

# Attempt import under denying policy (verifies policy enforcement)
./build/t81 canonfs import "$artifact" --canonfs-root "$canon_root" --policy "$deny_policy" --json
```

---

## 7. What T81 Is Not (Limitations)

- **Not a Commercial Operating System:** T81 is a systems research substrate, not a desktop or server OS replacement.
- **Not Physical Hardware:** T81 runs via software emulation on binary x86_64 and ARM64 CPUs; no physical ternary silicon is claimed or required.
- **Not a Proof of Ternary Speed Superiority:** Software ternary CPU arithmetic is proven slower than native binary ALUs on modern processors.
- **Not an Un-gated LLM Runtime:** Model header inspection (GGUF/Safetensors) does not imply general-purpose, un-governed AI inference.

---

## 8. Research Roadmap & Next Experiment

The immediate research focus is defined by **Candidate 3 (Deterministic AI Execution Parity)**:

- **Scientific Objective:** Validate whether soft-float transcendentals (RFC-0030) and CanonHash-81 trace caching achieve $100\%$ bit-identical trace parity across x86_64, ARM64, and WebAssembly targets with $<5\%$ execution time tax vs native `libm`.
- **Target Surface:** `tests/determinism/` and `vm/determinism/` bounded by the [Determinism Surface Registry](docs/governance/DETERMINISM_SURFACE_REGISTRY.md).
- **Reassessment Report:** See [`docs/research/T81_FULL_REASSESSMENT_2026-10-25.md`](docs/research/T81_FULL_REASSESSMENT_2026-10-25.md) for full experimental details.

---

## 9. Primary Evidence & Documentation References

- [Full Research Reassessment Report (October 2026)](docs/research/T81_FULL_REASSESSMENT_2026-10-25.md)
- [T81 Research Charter](docs/research/RESEARCH_CHARTER.md)
- [Ternary Research Closure & Next Hypothesis Decision](docs/research/TERNARY_RESEARCH_CLOSURE_AND_NEXT_HYPOTHESIS.md)
- [TISC v1.9.0 Opcode Registry Specification](spec/tisc/opcode-registry.md)
- [CanonFS Foreign Filesystem Interchange (RFC-00D1)](spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md)
- [Accountable Decision Bundle Specification (RFC-00D4)](spec/rfcs/RFC-00D4-accountable-decision-bundle.md)
- [Research Consolidation Evidence Ledger](docs/research/consolidation/README.md)

---

## 10. Contributing

We welcome contributions that bring empirical rigor, reproducible experiments, or precise specification improvements. Please read [CONTRIBUTING.md](CONTRIBUTING.md) and [AGENTS.md](AGENTS.md) before submitting pull requests.

## License

T81 Foundation is licensed under the [Apache License, Version 2.0](LICENSE).
