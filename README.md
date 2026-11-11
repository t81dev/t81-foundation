# T81 Foundation

[English](README.md) · [简体中文](README.zh-CN.md) · [Español](README.es.md) · [Português (Brasil)](README.pt-BR.md) · [Русский](README.ru.md)

### Researching alternative representations, adaptive execution, and verifiable computing.

**Balanced ternary · Representation-aware AI/ML · Deterministic execution · Experimental architecture**

[Research Charter](docs/research/RESEARCH_CHARTER.md) · [Research Roadmap](docs/ROADMAP.md) · [Architecture](docs/explanation/ARCHITECTURE.md) · [AI Research Context](docs/ai-context/) · [Determinism Surface Registry](docs/governance/DETERMINISM_SURFACE_REGISTRY.md)

---

## What is T81?

**T81 Foundation is an implemented experimental computing architecture and open-source scientific research program investigating whether alternative computational representations—particularly balanced ternary—can provide measurable advantages over conventional binary systems.**

T81 is an **implemented, functioning architecture**: it defines the Ternary Instruction Set Computer (TISC) as its computational instruction contract, with bounded cross-profile execution demonstrated across both a hosted virtual machine (T81VM) and a bare-metal freestanding microkernel environment (Axion OS / TernaryOS on AArch64 QEMU).

The project studies computation across multiple domains: mathematical representation, algorithms, software execution, storage, hardware architecture, and verifiable system behavior. Its research-first architecture originated in discrete mathematics, CWEB/Hanoi, and investigations of alternative numerical systems before expanding into language, VM, instruction, policy, and research-assurance subsystems.

A secondary, explicitly experimental direction is **representation-aware adaptive execution**: selecting data representations, algorithms, and execution paths for particular workloads, especially AI/ML, while measuring the full costs of selection, conversion, computation, and accuracy. This is an architectural hypothesis, **not** an established T81 performance advantage.

Its central question is:

> Under what conditions can ternary computation outperform, simplify, or otherwise improve upon an equivalent binary approach?

The answer is not assumed.

Some experiments have demonstrated useful mathematical and representational properties. Others have shown that conventional binary implementations remain faster or more efficient. Several fundamental questions remain unresolved.

T81 exists to investigate those boundaries systematically.

**This is an experimental research platform, not a claim that a superior ternary computer has already been built.**

---

## Why ternary?

Modern computing overwhelmingly uses binary states: 0 and 1.

Ternary computing uses three states. In balanced ternary, these are represented as:

\[
T = \{-1,0,+1\}
\]

This creates distinct possibilities for signed arithmetic, numerical representation, state encoding, decision systems, and computational architecture.

For example:

| Property | Mathematical relationship |
|---|---|
| One trit | Three possible states |
| Four trits | \(3^4 = 81\) states |
| Six trits | \(3^6 = 729\) states |
| Information per trit | \(\log_2 3 \approx 1.585\) bits |
| Base-81 digit | Four grouped trits |

These relationships are mathematical facts.

They do **not** establish that ternary processors are faster, that ternary memory is physically denser, or that ternary algorithms consume less energy.

Those are separate engineering questions requiring rigorous, matched comparisons against optimized binary alternatives.

T81 investigates the distinction.

---

## What have we learned?

Research is valuable when it identifies both opportunities and limitations.

**Lead result — an unfavorable optimized-binary comparison:** In the historical AVX2 matrix-vector experiment below, native INT8 reached **17.15 GFLOP/s**, versus **12.91 GFLOP/s** for packed 2-bit ternary (approximately **32.8% higher throughput** for the native INT8 implementation). This is a workload-specific result, not a universal radix ranking; T81 publishes it prominently because an unfavorable result is evidence, not a defect to conceal.

T81's experimental history has established several important findings.

### 1. AVX2 negative result: representation density is not execution speed

Packed ternary data can reduce storage requirements.

However, software must still decode, manipulate, and execute those representations on conventional binary hardware.

In one historical AVX2 matrix-vector experiment:

| Implementation | Reported throughput |
|---|---:|
| Native INT8 AVX2 | 17.15 GFLOP/s |
| Packed 2-bit ternary AVX2 | 12.91 GFLOP/s |

The compact ternary representation did not outperform the conventional INT8 implementation in that workload.

This is a bounded experimental result—not a proof that ternary computation is universally slower.

[Explore the packed ternary research](docs/research/PACKED_TERNARY_MATVEC_PERFORMANCE_REPORT.md)

### 2. Adaptive representations have conditional advantages

Research into adaptive representation tiering has identified configurations in which a ternary representation has a lower modeled cost than a matched binary alternative.

Those advantages depend on workload assumptions, metadata overhead, conversion costs, and representation boundaries.

The corrected H04-R02 research preserves these conditional findings while explicitly leaving physical performance unresolved.

[Read the corrected H04-R02 findings](experiments/h04_r02_adaptive_representation/RESULTS.md)

### 3. Correctness does not establish optimization

Experimental investigation T81-AIOPT-001 examined multiple representations of four-trit pattern matching.

Its M1 implementation passed extensive local correctness checks, but the experimental workload could be reduced to a simple ordinal comparison.

The result demonstrates why an exhaustive correctness test can still be insufficient for evaluating computational advantage.

The relevant question is not simply whether another representation works, but whether the chosen workload can distinguish meaningful differences between competing algorithms.

[Explore T81-AIOPT-001](docs/research/aiopt/)

### 4. Bounded cross-profile execution demonstrated (CPA-004/CPA-005)

**Evidence classification: bounded determinism and portability, not comparative performance or whole-system maturity.** Determinism guarantees are strictly bounded by the [Determinism Surface Registry](docs/governance/DETERMINISM_SURFACE_REGISTRY.md).

Recent research in the Cross-Profile Architecture (CPA) program demonstrated that the computational core of T81 executes identically across divergent execution environments:

- **Canonical Program Artifact:** A frozen 351-byte canonical program comprising 27 canonical 13-byte records (SHA-256 `b0649777cfa1c9e0fa1febb8bdfb1191c5cd3deea0d313791a4a32d5f207c80b`) exercised an admitted subset of 17 TISC instructions.
- **Cross-Profile Equivalence:** Fresh executions across three hosted T81VM processes and three fresh AArch64 QEMU bare-metal boots (after UEFI `ExitBootServices`) matched an independently derived mathematical oracle at **all 25 observation boundaries** (initial state + 24 instruction steps).
- **Comprehensive Boundary Matching:** Equivalence was verified across all 81 register values and tags, 1,563 logical memory words and tags, flags, and system registers (R75–R80), halting deterministically at PC=27, SP=283.
- **Distinction:** Demonstrating cross-profile execution correctness proves the computational architecture functions as specified; it is **not** evidence of ternary performance, energy, or hardware superiority over binary baselines.

### 5. SCG-08 active research program: carry propagation and arithmetic work

Program **SCG-08** investigates balanced-ternary carry propagation and multi-precision arithmetic work under a frozen weighted cost metric:

- **Phase 0 Research Charter:** Accepted and integrated (`SCG08_PHASE0_RESEARCH_CHARTER.md`).
- **Phase 1 Gate 1 Arithmetic Harness:** Implemented and verified under isolated test harness `experiments/scg08_arithmetic_work/`. It establishes mathematical correctness across 5 standardized operand bit-widths (64, 128, 256, 512, 1024 bits) against a decoupled arbitrary-precision base-$10^9$ exact integer oracle, deterministic replay, exact primitive operation accounting, and strict trial budget enforcement ($\le 1,000$ trials; 179 executed in the smoke suite).
- **Gate 2 Benchmarking Status:** **STRICTLY WITHHELD.** Full-scale empirical campaigns and statistical hypothesis evaluations remain unauthorized until explicit, authoritative review and gate elevation occur. Merged harness code does not constitute experimental authorization.

### 6. Physical ternary computing remains an open question

Software-emulated ternary computation on binary CPUs is not equivalent to computation on native ternary hardware.

Evaluating physical devices requires matched evidence for area, energy, latency, reliability, sensing, encoding, and peripheral circuitry.

Physical evaluation remains blocked where adequate device-characterization evidence is unavailable (Investigation 10A). T81 does not interpret missing evidence as proof of impossibility, nor does it treat theoretical radix efficiency as proof of physical advantage.

---

## The T81 research architecture

The repository contains several subsystems developed to investigate computational representations and controlled execution.

```text
                     T81 FOUNDATION
                Implemented Architecture & Research
                             |
          +------------------+------------------+
          |                  |                  |
    REPRESENTATION       EXECUTION          ASSURANCE
          |                  |                  |
    Balanced ternary     TISC ISA (Contract)  Axion Policy
    Base-81 / Base-243   +-- T81VM (Hosted)   CanonFS
    Packed data          +-- Axion (Baremetal) Evidence
    Adaptive tiering     T81Lang Frontend     Provenance
          |                  |                  |
          +------------------+------------------+
                             |
                   CONTROLLED EXPERIMENTS
                             |
               Conventional binary baselines
                             |
                 Verification and comparison
                             |
                    Research conclusions
```

### Computational Contract and Execution Profiles

In the T81 architecture:
- **TISC (Ternary Instruction Set Computer):** Serves as the central computational instruction contract, defining opcode semantics, 64-bit state vectors, and deterministic transition rules.
- **Hosted T81VM:** Serves as the hosted execution profile for user-space development, tooling, testing, and deterministic replay on conventional host OSes (Linux, macOS, Windows).
- **Freestanding Axion OS / TernaryOS:** Serves as the bare-metal execution profile running directly on hardware or hypervisors (e.g., AArch64 QEMU), executing TISC without user-space OS dependencies.
- **CanonFS:** Provides content-addressed object identity, immutable artifact storage, cryptographic provenance, and RFC-00D1 interchange contracts.
- **DAIOS (Deterministic AI OS):** Remains an independent, forward-looking architectural concept and target architecture proposal (RFC-00D2) for high-level autonomous agent and AI runtime governance, distinct from the implemented low-level TISC execution engine.

### Subsystem Maturity Classifications

The project distinguishes among different maturity classifications across its subsystems. A working implementation or specification does not imply equal maturity or validated advantage:

| Component | Maturity | Role |
|---|---|---|
| **TISC** | Frozen | Computational instruction contract governing execution semantics (v1.9.0) |
| **T81VM** | Stable | Hosted execution profile for virtual-machine execution and determinism (bounded by the [Determinism Surface Registry](docs/governance/DETERMINISM_SURFACE_REGISTRY.md)) |
| **Axion Kernel** | Stable | Freestanding execution profile (bare-metal AArch64 microkernel/HAL) and pre-side-effect policy authorization |
| **T81Lang** | Stable | Language frontend and compilation infrastructure targeting TISC |
| **T81 Core** | Stable | Numerical types, containers, and representation primitives |
| **CanonFS** | Stable | Content-addressed artifacts and integrity mechanisms |
| **DAIOS** | Concept | Target architecture proposal for deterministic AI operating systems (RFC-00D2) |
| **Research experiments** | Experimental | Hypothesis testing, correctness validation, and comparative measurements |

---

## Getting Started

### Prerequisites

Building T81 requires:
- **CMake** 3.24 or higher
- **C++20 compliant compiler** (GCC 12+, Clang 15+, or MSVC 2022)
- **Python** 3.10+ (for governance, validation, and testing scripts)
- *Optional:* Ninja build system, QEMU AArch64 (for bare-metal targets)

### Building the Project

Configure and build the core project:

```bash
# Configure out-of-source build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Build core targets
cmake --build build --target t81 -j$(nproc)

# Run standard test suite
ctest --test-dir build --output-on-failure
```

### Running Experiments (Opt-In)

Research experiments and harnesses (such as SCG-08) are guarded behind opt-in CMake flags:

```bash
# Configure with experimental harnesses enabled
cmake -B build -S . -DT81_BUILD_EXPERIMENTS=ON -DCMAKE_BUILD_TYPE=Release

# Build and execute SCG-08 Gate 1 arithmetic validation
cmake --build build --target t81_exp_scg08_01_test
ctest --test-dir build -R scg08 --output-on-failure

# Validate evidence schema
python3 scripts/ci/validate_scg08_gate1_evidence.py
```

---

## Research methodology

T81 follows an evidence-first approach.

Every comparative experiment must answer five questions:

1. **What is the hypothesis?** Define a falsifiable proposition.
2. **What is the baseline?** Compare against a functionally equivalent, appropriately optimized conventional alternative.
3. **What is measured?** Separate correctness, storage, latency, throughput, energy, and memory traffic.
4. **What are the limitations?** Identify assumptions, host hardware, workload scope, and unresolved dependencies.
5. **What conclusion is justified?** Distinguish supported, unsupported, inconclusive, blocked, and untested outcomes.

The project strictly enforces evidence classifications:

**Implemented → Tested → Measured → Comparatively established**

These are different evidentiary levels, not interchangeable labels. An experimental result must not claim more than its evidence supports.

---

## Documentation Navigation

| Directory / File | Description |
|---|---|
| [`docs/research/`](docs/research/) | Primary scientific investigations and experimental findings |
| [`experiments/`](experiments/) | Bounded experimental implementations, harnesses, and evidence records |
| [`spec/`](spec/) | Instruction-set specifications and RFC directory |
| [`docs/governance/`](docs/governance/) | Governance surface registers, authority model, and [Determinism Surface Registry](docs/governance/DETERMINISM_SURFACE_REGISTRY.md) |
| [`docs/ROADMAP.md`](docs/ROADMAP.md) | Contributor roadmap and active research sequencing |
| [`docs/ai-context/`](docs/ai-context/) | Authoritative continuity documentation and state reviews for AI research collaborators |
| [`include/t81/`](include/t81/) | Core public C++ headers and contracts |
| [`vm/`](vm/) | Hosted virtual-machine implementation (T81VM) |
| [`kernel/axion/`](kernel/axion/) | Axion policy engine and governance kernel |
| [`fs/`](fs/) | CanonFS content-addressed storage and interchange driver |
| [`PROJECT_STRUCTURE.md`](PROJECT_STRUCTURE.md) | Comprehensive map of repository directories and layout |
| [`SUPPORT_TAXONOMY.md`](SUPPORT_TAXONOMY.md) | Taxonomy of support tiers, platform boundaries, and profiles |

---

## What T81 does not claim

T81 does not claim:

- Universal superiority of ternary computation over binary computation.
- Demonstrated superiority or commercial feasibility of native ternary silicon.
- A general performance advantage for the integrated T81 architecture over optimized conventional runtimes.
- A validated adaptive AI optimizer that outperforms strong fixed-policy or algorithm-selection baselines.
- That historical PDF case-study figures constitute independently reproduced measurements.
- That ternary model weights inherently accelerate AI inference.
- That deterministic execution or policy enforcement requires a ternary virtual machine.
- That an AI-generated experiment is independently validated merely because its local tests pass.
- That merged harness code or Gate 1 qualification authorizes Phase 2 benchmarking without explicit governance approval.

These boundaries are essential to the scientific integrity of the research.

---

## The question remains open

Binary computing dominates today's physical and software infrastructure for compelling engineering reasons.

That dominance does not eliminate the value of investigating alternative representations, computational models, or future hardware substrates.

T81's purpose is to determine where those alternatives are useful—and where they are not.

**We are not building a conclusion and searching for evidence to support it.**

**We are building the experimental foundation needed to discover the conclusion.**

---

**T81 Foundation** — Experimental Computing Architecture & Systems Research

[License: Apache 2.0](LICENSE) · [GitHub](https://github.com/t81dev/t81-foundation)
