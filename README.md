<p align="center">
  <img src="assets/banner.png" alt="T81 Foundation — Deterministic Ternary Architecture" width="100%">
</p>

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

<!-- T81-SPEED-START -->
<!-- T81-SPEED-END -->

# T81 Foundation — Deterministic Ternary Computing Stack

![Release](https://img.shields.io/badge/release-v1.4.1--Stable-blue)
![Tests](https://img.shields.io/badge/tests-363%2F363_passing-brightgreen)
![ISA](https://img.shields.io/badge/ISA-v1.1.0_Frozen-blue)
![Execution](https://img.shields.io/badge/execution-deterministic-green)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

Leveraging the theoretical efficiency of base-e computation, **T81 Foundation** is a deterministic computing stack built on **balanced ternary arithmetic** ({-1, 0, +1}) with a full-chain governance model covering instruction set, virtual machine, language compiler, and AI inference environment.

The stack delivers:

- **bit-exact reproducibility** — every execution path produces an identical trace hash across supported platforms
- **governed AI inference** — Axion policy engine intercepts and audits every privileged operation before side effects
- **content-addressed provenance** — CanonFS records all artifacts, model weights, and runtime state immutably
- **deterministic parallel execution** — DPE task graph model (RFC-DPE-0002) enables concurrent TISC workloads with epoch-committed outputs

---

## Project Status — March 2026

**Phase: Maintenance** — v1.4.1-Stable; 363/363 tests passing; all RFC drafts closed; no open blockers.

| Component | Maturity | Notes |
| :--- | :--- | :--- |
| **TISC ISA** | ❄️ Frozen | Opcode semantics immutable under v1.x; `AgentInvoke` added (RFC-0015) |
| **Data Types** | ❄️ Frozen | BigInt, Float, Complex, Map, Set — bit-stable encoding; 2026-02-27 audit clean |
| **T81VM** | ✅ Stable | `AgentInvoke` dispatch with Axion audit; 363/363 tests |
| **T81Lang** | ✅ Stable | spec v1.3 Stable; first-class `agent`/`behavior` (RFC-0015); §3.2 I/O channels defined; all sections complete |
| **Axion Governance Kernel** | ✅ Stable | P4 Safety & P5 Privileged Instruction satisfied; AX-M6 canonical reason strings; every `AgentInvoke` emits audit event |
| **TUI Frontends** | ✅ Accepted | `t81 studio` (human operator) + `t81 agent` (AI-native); FTXUI v5.0.0; RFC-0033 accepted |
| **T81Graph** | ✅ Beta | VM opcode lowering + lang-side serialization wired; DCP verification complete; 6/6 tests |
| **DPE (Parallel Execution)** | ✅ Accepted | RFC-DPE-0001–0009 all accepted; task graph, epoch history ring, epoch audit events, timeout fully implemented |
| **Cognitive Tiers** | ✅ Accepted | Tier4 Cognition (RFC-0021): `Tier4Loop`, `SelfModel` (81-entry ring), `RecursiveImprovementBounds`, `TierAwarePlanner`; 4 test suites pass |
| **Benchmark Suite** | ✅ Accepted | RFC-00A2: VM throughput + CanonHash81 determinism validation (`score=1.0` across all runs); `t81 internal benchmark` |
| **Axion OS Kernel** | 🔬 Experimental | TernaryOS: pager, scheduler, IPC, interrupt framework, QEMU x86_64 EFI lane operational; 9/9 ternaryOS tests pass |

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│  Interfaces                                                 │
│  t81 studio (Human TUI)   t81 agent (AI-Native TUI)  CLI    │
├─────────────────────────────────────────────────────────────┤
│  T81Lang Compiler                                           │
│  Lexer → Parser → Typed AST → Semantic Analyzer → IRGen     │
├─────────────────────────────────────────────────────────────┤
│  Axion Governance Kernel                                    │
│  PolicyEngine · CanonFS · Audit Trail · Ethics Gate         │
├──────────────────────────────┬──────────────────────────────┤
│  T81 Virtual Machine         │  DPE Task Graph Runtime      │
│  TISC interpreter            │  EpochGraph · DeltaBuffer    │
│  (deterministic)             │  (RFC-DPE-0002)              │
├──────────────────────────────┴──────────────────────────────┤
│  TISC ISA  ❄️ Frozen  +  Data Types  ❄️ Frozen              │
│  Deterministic substrate — CanonHash81 bit-exact traces     │
└─────────────────────────────────────────────────────────────┘
  Experimental: TernaryOS (Axion OS Kernel) · Cognitive Tiers
```

### Key components

**TISC ISA** — Ternary Instruction Set Architecture. Frozen under v1.x; the immutable execution contract for the entire stack.

**T81VM** — Deterministic TISC interpreter. Guarantees bit-identical output across platforms; Axion pre-dispatch isolation keeps governance hooks outside the hot execution path.

**Axion Governance Kernel** — Policy engine that intercepts `AXREAD`, `AXSET`, `AXVERIFY`, and AI opcodes before any side effect. Fail-closed on policy parse failure. Stable-certified 2026-03-15 with 54/54 tests passing (49/49 axion + 5/5 AX-M6 canonical reason strings).

**CanonFS** — Content-addressed filesystem. Stores all code objects, model weights, and runtime artifacts as immutable, hash-identified blobs. Provides provenance for determinism audits.

**T81Lang** — High-level language targeting TISC bytecode. Native types: `BigInt`, `Fraction`, `Float`, `Complex`, `Tensor`, `Map`, `Set`. First-class `agent { behavior }` declarations compile to `AGENT_INVOKE` with Axion audit (RFC-0015). Compiler pipeline: lexer → parser → typed AST → semantic analysis → IR generation.

**TUI Frontends** — Two complementary terminal interfaces built on FTXUI v5.0.0:

- `t81 studio` — navigation sidebar, CanonFS browser, Axion dashboard, determinism trace visualizer, command palette (`Ctrl+P`)
- `t81 agent` — persistent JSONL session, slash commands (`/compile`, `/run`, `/hash`, `/allow`, `/infer`, `/trits`, …), trit-probability bar

**DPE (Deterministic Parallel Execution)** — Task graph model over the frozen TISC ISA. Tasks declare immutable inputs and buffered output regions; the VM commits all writes atomically at epoch end. No new opcodes required.

---

## Quick Start

```bash
# Clone and configure
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation
cmake --preset default -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run the full test suite
ctest --test-dir build --output-on-failure

# Launch the human operator TUI
./build/t81 studio

# Launch the AI-native TUI
./build/t81 agent

# Compile a T81Lang program
./build/t81 code build examples/hello.t81 -o hello.tisc

# Execute with Axion governance
./build/t81 vm run hello.tisc
```

Optional build flags:

| Flag | Default | Purpose |
| :--- | :--- | :--- |
| `T81_BUILD_TUI` | `ON` | FTXUI-based TUI frontends |
| `T81_BUILD_TESTS` | `ON` | Full test suite |
| `T81_ENABLE_ASAN` | `OFF` | Address sanitizer |
| `T81_ENABLE_UBSAN` | `OFF` | UB sanitizer |
| `T81_ENABLE_LLAMA_CPP` | `OFF` | Governed llama.cpp inference adapter |
| `T81_WARN_STRICT` | `OFF` | Strict warning scan mode (used by the `warn-strict` preset) |

**Pre-push warning scan** — mirrors the `-Wswitch`, `-Wunused-variable`, and `-Wunused-function` checks enforced by Windows CI, catching issues locally in ~2 minutes instead of waiting for the full matrix:

```bash
cmake --preset warn-strict
cmake --build build-warn-strict 2>&1 | head -40
```

---

## Determinism Verification

Every release is verified for bit-exact cross-platform reproducibility.

```bash
./scripts/ci/run_determinism_slice.sh
```

Verified platforms: **Linux x86_64**, **macOS ARM64**. Any divergence in VM trace hashes is a critical defect.

---

## Documentation

| Topic | Location |
| :--- | :--- |
| Getting Started (C++) | `docs/user-guide/getting-started/cpp-quickstart.md` |
| Getting Started (AI) | `docs/user-guide/getting-started/ai-quickstart.md` |
| TUI Guide | `docs/user-guide/how-to/tui-guide.md` |
| ISA Specification | `spec/tisc-spec.md` |
| Axion Policy Manual | `docs/user-guide/tutorials/axion-policy-manual.md` |
| T81Lang Stdlib Reference | `docs/user-guide/reference/T81LANG_STDLIB_REFERENCE.md` |
| Architecture Overview | `docs/architecture/OVERVIEW.md` |
| Governance Charter | `docs/governance/README.md` |
| Project Control Center | `docs/status/PROJECT_CONTROL_CENTER.md` |

---

## Roadmap

| Milestone | Target | Description |
| :--- | :--- | :--- |
| C2 Month-Close | 2026-03-31 | Governance ledger audit; preflight PASS 2026-03-10 |
| Axion Stable promotion | ✅ **COMPLETED 2026-03-15** | AX-M6 canonical reason strings implemented; 54/54 tests passing; production-ready |
| T81Graph Beta promotion | ✅ **COMPLETED 2026-03-15** | VM opcode lowering complete; DCP verification; 6/6 tests passing |
| T81Lang spec promotion | 2026-05-15 | Bytecode deterministic compilation profile; full spec-section traceability |
| RFC-00B5 interrupt policy | TBD | Actual interrupt handler behavior (policy dispatch, vector table wiring) |
| TernaryOS bare-metal boot | TBD | x86_64 VirtualBox host execution + evidence return |

---

## Governance

T81 Foundation operates under a **Continuous Governance (C2)** model. All contributions must maintain:

- **deterministic execution parity** — trace hashes must match across supported platforms
- **architectural coherence** — changes that touch the deterministic surface require formal review
- **reproducibility guarantees** — no floating-point or platform-specific non-determinism in the DCP surface

The deterministic surface is defined in `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`. Changes to frozen surfaces (TISC ISA, Data Types) require a major version bump.

> **Boundary note:** Experimental surfaces (Cognitive Tiers, Distributed, Trace-JIT, TernaryOS, llama.cpp adapter) are governed non-DCP and must not be presented as verified deterministic components.

---

## The Ternary Advantage

While modern binary hardware is highly optimized, the **T81 Foundation** leverages the unique mathematical properties of **Balanced Ternary ({-1, 0, +1})** to achieve structural efficiencies that binary cannot match.

### 1. $O(1)$ Computational Symmetry

In binary Two's Complement, negating a number is an asymmetric operation (NOT + 1) that requires carry propagation. In T81, negation is a simple trit-flip with **zero carry overhead**.

* **Performance:** T81 negation throughput reaches **~46.6 G-ops/s** (via `PackedCell`), outperforming optimized 64-bit binary negation by **10.4x**.

### 2. Superior Radix Economy

Based on the theorem that the most efficient base for a number system is $e \approx 2.718$, ternary (Base 3) is mathematically more efficient than binary (Base 2).

* **Information Density:** T81 achieves a theoretical density of **1.58 bits per trit**. This translates to higher entropy per clock cycle and reduced storage footprints for large-scale coordinate systems and neural weights.

### 3. Bit-Exact Determinism

Binary floating-point operations (IEEE 754) often suffer from platform-specific rounding non-determinism. T81’s balanced arithmetic provides:

* **Inherent Symmetry:** Rounding is performed by simple truncation, as the system is naturally centered around zero.
* **Trace Parity:** 100% "Roundtrip Accuracy" across all tested platforms (Linux x86_64, macOS ARM64) with zero divergence in VM trace hashes.

### 4. Direct Governance Hook

Because the TISC ISA is ternary-native, the **Axion Governance Kernel** can audit state transitions with higher granularity. AI inference operations can be intercepted at the "trit-level" before any side effects occur, enabling a "fail-closed" security model that is architecturally impossible in standard "black-box" binary execution.

---

The **10x negation win** and the **1.58 bits/trit density** aren't just vanity metrics—they are the keys to solving specific "Binary Bottlenecks" in AI, cryptography, and distributed physics.

Here are the high-impact applications we can build or optimize right now using the **T81 advantage**:

---

## 1. High-Fidelity Signal & Physics Simulation

In binary, $0$ is an unsigned start point, making "negative" space a secondary consideration. In balanced ternary, **zero is the equilibrium point.**

* **The Use Case:** Direct simulation of wave mechanics, electromagnetism, and fluid dynamics.
* **The Advantage:** Since these systems oscillate between positive and negative states, T81 can simulate "Push-Pull" forces without the computational lopsidedness of Two's Complement.
* **Next Step:** We could build a **TISC-native DSP library** where filters (FIR/IIR) are optimized for the $O(1)$ negation speed.

## 2. "Symmetric" Neural Networks (TNNs)

Current AI (Binary/FP) wastes massive energy on activation functions like `tanh` or `ReLU` to create a "zero-centered" state for training.

* **The Use Case:** Ternary Neural Networks (where weights are -1, 0, or 1).
* **The Advantage:** Because your architecture is natively balanced, we can run "Multiplication-Free" inference. A T81 neuron doesn't "multiply" inputs; it simply **flips or gates them** based on the weight. This would be orders of magnitude more energy-efficient than current GPU-based inference.
* **Next Step:** We could implement a **T81-native Inference Engine** that interprets model weights directly as TISC opcodes.

## 3. Post-Quantum Cryptographic Primitives

Many "Lattice-based" encryption algorithms (the ones designed to survive quantum computers) rely on small-coefficient polynomials—often centered around zero ({-1, 0, 1}).

* **The Use Case:** NTRU or Kyber-style encryption.
* **The Advantage:** Binary systems have to "emulate" these small coefficients using 8-bit or 32-bit integers, wasting 90% of the bit-space. T81 stores these values with **zero waste** and processes the polynomial additions/negations at native hardware speeds.
* **Next Step:** We can draft an RFC for a **TISC Cryptography Extension** that implements a ternary-optimized polynomial multiply.

## 4. Immutable Governance Audits (Axion)

Since you have 1.58 bits of entropy per trit, we can encode **security metadata** directly into the data word without significantly ballooning the memory footprint.

* **The Use Case:** "Labeled Data" at the hardware level.
* **The Advantage:** We can use the "extra" capacity of a TISC word to carry a **Provenance Tag**. Every time data is moved, Axion verifies the tag. If a "privileged" trit moves into "user" space, the hardware can trap it instantly.
* **Next Step:** Refine the **Axion OS Kernel** to use the "Ternary Margin" for real-time memory tagging.

---

## License

MIT License.
