# T81 Foundation

[English](README.md) · [简体中文](README.zh-CN.md) · [Español](README.es.md) · [Português (Brasil)](README.pt-BR.md) · [Русский](README.ru.md)

### Researching alternative representations, adaptive execution, and verifiable computing.

**Balanced ternary · Representation-aware AI/ML · Deterministic execution · Experimental architecture**

[Research Charter](docs/research/RESEARCH_CHARTER.md) · [Research Roadmap](docs/ROADMAP.md) · [Architecture Overview](docs/architecture/OVERVIEW.md) · [Stratified AI OS Blueprint (Horizon D)](docs/architecture/AI_OS_STRATIFIED_BLUEPRINT.md) · [AI Research Context](docs/ai-context/) · [Determinism Surface Registry](docs/governance/DETERMINISM_SURFACE_REGISTRY.md)

---

## 60-Second Proof (`v2.3.0`)

Install the Python SDK wheels (`CPython 3.9–3.14` on Linux, macOS, Windows) and run the self-contained **BitNet-2B streaming ingest + governed MCP session + offline Ed25519 receipt verification** proof from any empty directory (no repo clone or compiler required):

```bash
# Quickstart via PyPI (no C++ compiler or repo clone required)
pip install t81
python -m t81.examples.stranger_proof --keep-dir ./t81_proof_out

# Re-verify the exported forensic bundle 100% offline (zero server running)
python -m t81.examples.stranger_proof --verify-bundle ./t81_proof_out/forensic_bundle
```

See the full walkthrough and expected pinned SHA3-512 / Ed25519 output in **[examples/stranger-proof/README.md](examples/stranger-proof/README.md)**.

Or build from source (`cmake -S . -B build -G Ninja && cmake --build build --target t81 -j4`) and run the verified end-to-end stack:

```bash
# 1. Native C++ MCP server (15 in-process tools: Axion policy, CanonFS, DPE+MCP tx, KEV gate, Dual-Rail diff/RAG/adapter)
python3 examples/research/agent-receipt-sidecar/test_native_mcp.py

# 2. Full-stack CanonFS -> Axion -> TISC -> T81VM -> Dual-Rail TMatMul (bit-exact SHA3-512 receipt)
./examples/research/full-stack-t81-pipeline/run_pipeline.sh

# 3. Zero-copy CPU NEON <-> Apple Silicon Metal GPU 32-layer Dual-Rail decode (89.1 GB/s on M2; CPU fallback on Linux)
./examples/research/heterogeneous-dualrail/run_demo.sh --layers=8
```

See [docs/research/DUAL_RAIL_HETEROGENEOUS_TISC_NOTE.md](docs/research/DUAL_RAIL_HETEROGENEOUS_TISC_NOTE.md) for pinned measurements, controls, caveats, and preserved negative results.

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

**Evidence classification: bounded determinism and portability, not comparative performance.** Across three hosted `T81VM` processes and three fresh bare-metal AArch64 QEMU boots (after UEFI `ExitBootServices`), a frozen 351-byte canonical program (17 TISC instructions) matched an independent mathematical oracle at all **25 observation boundaries** across all 81 registers, 1,563 logical memory words, flags, and system registers (`R75–R80`). This demonstrates execution correctness within the [Determinism Surface Registry](docs/governance/DETERMINISM_SURFACE_REGISTRY.md), not ternary speed or hardware superiority.

### 5. SCG-08 active research program: carry propagation and arithmetic work

Program **SCG-08** investigates balanced-ternary carry propagation under a frozen weighted cost metric (`SCG08_PHASE0_RESEARCH_CHARTER.md`). Its Phase 1 Gate 1 harness (`experiments/scg08_arithmetic_work/`) is verified across 64–1024 bit widths against an exact arbitrary-precision oracle; **Gate 2 empirical benchmarking remains strictly withheld (`NO-GO`)** pending explicit maintainer authorization.

### 6. Representation-independent governed execution is regime-bounded (`H07` closed)

In the reproducible head-to-head comparison against `Wasmtime 51.0.0` (precompiled `.cwasm`) + `OPA 1.23.0` (`opa eval` / persistent HTTP keep-alive) + `OpenSSL 3.6.2` on Apple Silicon M2 (`#906`), hypothesis **`H07`** closed as **`PARTIALLY SUPPORTED (REGIME-BOUNDED)`**:

- **Where T81 won:** Binary footprint (`5.48 MB` vs `83.36 MB`, `15.2×` smaller), steady-state daemon RSS (`3.38 MiB` vs `40.14 MiB` for OPA alone, `11.9×` lower), `p99` policy tail latency (`298 µs` vs `446 µs`, `N=1,000`), offline receipt verification (`9.12 ms` vs `10.12 ms`), and built-in policy-pinning + RFC 8032 Ed25519 receipt integration without external glue.
- **Where `Wasmtime + OPA` won:** Pure AOT compute (`8.52 ms` vs `157.11 ms` `.tisc` interpreter, `18.4×` faster), end-to-end cold-start (`54.23 ms` vs `168.22 ms`, `3.1×` faster), and median warm policy-only `p50` (`152.1 µs` vs `166.1 µs`).
- **Attribution & "For AI, By AI" Handoff:** Observed T81 wins come from single-binary C++23 systems integration rather than ternary arithmetic superiority. Together with the `pip install t81` outsider-reproducible proof ([examples/stranger-proof/README.md](examples/stranger-proof/README.md), `#905`), these primitives anchor the [RFC-00D4](spec/rfcs/RFC-00D4-accountable-decision-bundle.md) offline two-agent receipt handoff roadmap (draft companion profile in PR `#908`).

[Read the H07 Wasmtime + OPA comparison](docs/research/H07_WASMTIME_OPA_COMPARISON.md)

### 7. Physical ternary computing remains an open question

Software-emulated ternary computation on binary CPUs is not equivalent to computation on native ternary hardware.

Evaluating physical devices requires matched evidence for area, energy, latency, reliability, sensing, encoding, and peripheral circuitry.

Physical evaluation remains blocked where adequate device-characterization evidence is unavailable (Investigation 10A). T81 does not interpret missing evidence as proof of impossibility, nor does it treat theoretical radix efficiency as proof of physical advantage.

---

## The T81 research architecture

The repository contains several subsystems developed to investigate computational representations and controlled execution.

```text
+-------------------------------------------------------------------+
| Plane 1: Deterministic Policy & Context (Implemented)             |
|   Axion .apl (axion_root.apl) · TISC v1.9.0 · {-1, 0, +1} Lattice |
+-------------------------------------------------------------------+
                                 |
+-------------------------------------------------------------------+
| Plane 2: Stochastic / Neural Helpers (Planned — Horizon D)        |
|   Advisory Only · Zero direct write authority · < 100 MB RSS cap  |
+-------------------------------------------------------------------+
                                 |
+-------------------------------------------------------------------+
| Plane 3: Hybrid Reconciliation & Control Plane (Implemented)      |
|   15-Tool C++ MCP · DPE+MCP tx · KEV Gate · RegimeSelector R0–R4  |
+-------------------------------------------------------------------+
                                 |
+-------------------------------------------------------------------+
| Plane 0: Deterministic Foundation & Enclave Base (Implemented)    |
|   T81VM · CanonFS SHA3-512 · C++23 Ed25519 RFC-00D4 · SIMD/Metal  |
+-------------------------------------------------------------------+
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
| **TISC** | Frozen | Computational instruction contract governing execution semantics (`v1.9.0`, opcodes `0–173` frozen) |
| **T81VM** | Stable | Hosted execution profile for virtual-machine execution, Dual-Rail `SBDR1T2D`, and determinism (bounded by the [Determinism Surface Registry](docs/governance/DETERMINISM_SURFACE_REGISTRY.md)) |
| **Axion Kernel** | Stable | Fail-closed pre-side-effect policy authorization (`axion_root.apl`) and freestanding AArch64 enclave (`t81 enclave run`, experimental) |
| **Native C++ MCP & DPE** | Stable (bounded) | 15-tool `RFC-00D8` server (`t81 mcp serve`), `DPE`+`MCP` transactional isolation, KEV/advisory fail-closed gating, `t81==2.3.0` Python M3 SDK |
| **T81Lang** | Stable | Language frontend and compilation infrastructure targeting TISC |
| **T81 Core** | Stable | Numerical types, containers, and representation primitives (`PUBLIC_API_LOCK.json` `v2.3.0`) |
| **CanonFS & Ed25519 Receipts** | Stable (bounded) | Content-addressed artifacts (`SHA3-512`), `RFC-00D1` interchange, native C++23 Ed25519 `RFC-00D4`/`00D6` receipt DAGs, and offline `t81 receipt export` bundles |
| **Stratified AI OS (Horizon D)** | Planned / Design Target | Four-plane neural-symbolic architecture ([`AI_OS_STRATIFIED_BLUEPRINT.md`](docs/architecture/AI_OS_STRATIFIED_BLUEPRINT.md), `DEC-004`); advisory-only Plane 2 helpers are not currently implemented |
| **DAIOS** | Concept | Target architecture proposal for deterministic AI operating systems (RFC-00D2) |
| **Research experiments** | Experimental | Hypothesis testing, correctness validation, and comparative measurements |

---

## Getting Started

### Prerequisites

Building T81 requires:
- **CMake** 3.24 or higher
- **C++23 compliant compiler** (GCC 14+, Clang 18+, or MSVC 2022; C++20 compatibility lane supported)
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

## Extended Verification & Research Labs

To run the full compromised-agent gauntlet, exact-rational lab, replay consensus, and deterministic risk triage alongside the 60-Second Proof above:

```bash
# Self-hosted MCP receipt dogfooding + 11-scenario compromised-agent gauntlet
bash scripts/ci/run_agent_receipt_dogfood_smoke.sh

# Ternary AI primitives (Dual-Rail RAG, flip-budget diffs, block-zero skip, SBDR1 vs Base-243, Kleene policy)
./examples/research/ternary-ai-primitives/run_primitives.sh

# Exact-rational lab, two-node replay consensus, and deterministic risk triage
bash examples/research/exact-rational-lab/run_lab.sh
bash examples/research/replay-consensus/run_consensus.sh
bash examples/research/deterministic-risk-triage/run_triage_demo.sh
```

See [`examples/research/agent-receipt-sidecar/QUICKSTART.md`](examples/research/agent-receipt-sidecar/QUICKSTART.md) and [`THREAT_MODEL.md`](examples/research/agent-receipt-sidecar/THREAT_MODEL.md) for MCP client setup and explicit security boundaries.

---

## Documentation Navigation

| Directory / File | Description |
|---|---|
| [`docs/architecture/OVERVIEW.md`](docs/architecture/OVERVIEW.md) | Canonical architecture snapshot, layer boundaries, and four-plane alignment |
| [`docs/architecture/AI_OS_STRATIFIED_BLUEPRINT.md`](docs/architecture/AI_OS_STRATIFIED_BLUEPRINT.md) | Stratified Neural-Symbolic AI OS Blueprint (*Planned / Design Target — Horizon D*, `DEC-004`) |
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
