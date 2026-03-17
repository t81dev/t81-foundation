<p align="center">
  <img src="assets/banner.png" alt="T81 Foundation — Deterministic Ternary Architecture" width="100%">
</p>

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

<!-- T81-SPEED-START -->
<!-- T81-SPEED-END -->

# T81 Foundation — Deterministic Ternary Computing Stack

![Release](https://img.shields.io/badge/release-v1.6.1--Stable-blue)
![Tests](https://img.shields.io/badge/tests-367%2F367_passing-brightgreen)
![ISA](https://img.shields.io/badge/ISA-v1.2.0_Frozen-blue)
![Execution](https://img.shields.io/badge/execution-deterministic-green)
![CI](https://img.shields.io/badge/cross--platform--determinism-verified-brightgreen)
![License](https://img.shields.io/badge/license-MIT-lightgrey)

Leveraging the theoretical efficiency of base-e computation, **T81 Foundation** is a deterministic computing stack built on **balanced ternary arithmetic** ({-1, 0, +1}) with a full-chain governance model covering instruction set, virtual machine, language compiler, and AI inference environment.

The stack delivers:

- **bit-exact reproducibility** — every execution path produces an identical trace hash across supported platforms
- **governed AI inference** — Axion policy engine intercepts and audits every privileged operation before side effects
- **content-addressed provenance** — CanonFS records all artifacts, model weights, and runtime state immutably
- **deterministic parallel execution** — DPE task graph model (RFC-DPE-0002) enables concurrent TISC workloads with epoch-committed outputs

---

## Project Status — March 2026

**Phase: Active Development** — v1.6.1-Stable; 368/368 tests passing; cross-platform determinism verified on Linux x86\_64 + macOS ARM64.

| Component | Maturity | Notes |
| :--- | :--- | :--- |
| **TISC ISA** | ❄️ Frozen | v1.2.0; opcode semantics immutable under v1.x; 12 new opcodes since v1.1: `AgentInvoke` (RFC-0015), 6 ternary-native inference (RFC-0034), 3 FFI (RFC-00B8), 2 lattice crypto (RFC-0038), 1 KEM ring (RFC-0039) |
| **Data Types** | ❄️ Frozen | BigInt, Float, Complex, Map, Set — bit-stable encoding; 2026-02-27 audit clean |
| **T81VM** | ✅ Stable | Full TISC v1.2 dispatch; `AgentInvoke` + ternary-native inference + FFI + lattice crypto + NTRU-KEM opcodes; 368/368 tests |
| **T81Lang** | ✅ Stable | spec v1.3 Stable; `agent`/`behavior` (RFC-0015); `foreign {}` FFI (RFC-0036); `std.tnn.*` TNN stdlib (RFC-0037); `std.crypto.*` lattice crypto + NTRU-KEM (RFC-0038/0039); contextual identifier support throughout |
| **Axion Governance Kernel** | ✅ Stable | P4 Safety & P5 Privileged Instruction satisfied; AX-M6 canonical reason strings; every `AgentInvoke` + `TACT` activation gate emits audit event |
| **Ternary-Native Inference** | ✅ Stable | RFC-0034 + RFC-0037: `TWMATMUL`, `TQUANT`, `TATTN`, `TWEMBED`, `TERNACCUM`, `TACT`; `std.tnn.*` T81Lang stdlib (6 builtins → TISC ops); multiplication-free inference; T81WTN weight format; 13/13 tests; production-ready ternary inference operations |
| **Lattice Cryptography** | ✅ Stable | RFC-0038+0039: `POLYMUL`, `POLYMOD`, `TVecSub`; full ring {+,−,×,mod} over Z\[x\]/(x^n+1); `std.crypto.{polyadd,polysub,polymul,polymod,ntru_encrypt,ntru_decrypt}`; 37/37 tests; production-ready lattice cryptography |
| **Governed FFI** | ✅ Stable | RFC-00B8 + RFC-0036: `FFIDispatcher`, `FFILibraryRegistry`, 3 VM opcodes; `foreign [policy] { fn … }` T81Lang grammar; `foreign.<name>(args)` → `FFI_CALL`; 9/9 AC tests; production-ready governed foreign function interface |
| **TUI Frontends** | ✅ Beta | `t81 studio` (human operator) + `t81 agent` (AI-native); FTXUI v5.0.0; RFC-0033 accepted; production-ready terminal interfaces |
| **T81Graph** | ✅ Beta | VM opcode lowering + lang-side serialization wired; DCP verification complete; 6/6 tests |
| **DPE (Parallel Execution)** | ✅ Stable | RFC-DPE-0001–0009 all accepted; task graph, epoch history ring, epoch audit events, timeout fully implemented; production-ready deterministic parallel execution |
| **Cognitive Tiers** | ✅ Beta | Tier4 Cognition (RFC-0021): `Tier4Loop`, `SelfModel` (81-entry ring), `RecursiveImprovementBounds`, `TierAwarePlanner`; 4 test suites pass; experimental cognitive architecture ready for beta testing |
| **Benchmark Suite** | ✅ Stable | RFC-00A2: VM throughput + CanonHash81 determinism validation (`score=1.0` across all runs); `t81 internal benchmark`; production-ready performance validation |
| **Cross-Platform Determinism CI** | ✅ Accepted | Daily GitHub Actions workflow compares T81Lang bytecode hashes across Linux x86\_64 (gcc-14) and macOS ARM64 (clang); publicly auditable evidence record |
| **Axion OS Kernel** | 🔬 Experimental | TernaryOS: pager, scheduler, IPC, interrupt framework, QEMU x86\_64 EFI lane operational; 9/9 ternaryOS tests pass |

---

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│  Interfaces                                                 │
│  t81 studio (Human TUI)   t81 agent (AI-Native TUI)  CLI    │
├─────────────────────────────────────────────────────────────┤
│  T81Lang Compiler                                           │
│  Lexer → Parser → Typed AST → Semantic Analyzer → IRGen     │
│  agent/behavior (RFC-0015)  ·  foreign {} (RFC-0036)        │
├─────────────────────────────────────────────────────────────┤
│  Axion Governance Kernel                                    │
│  PolicyEngine · CanonFS · Audit Trail · Ethics Gate         │
├──────────────────────────────┬──────────────────────────────┤
│  T81 Virtual Machine         │  DPE Task Graph Runtime      │
│  TISC interpreter            │  EpochGraph · DeltaBuffer    │
│  (deterministic)             │  (RFC-DPE-0002)              │
├──────────────────────────────┴──────────────────────────────┤
│  TISC ISA v1.2  ❄️ Frozen  +  Data Types  ❄️ Frozen         │
│  Deterministic substrate — CanonHash81 bit-exact traces     │
├─────────────────────────────────────────────────────────────┤
│  Governed FFI (RFC-00B8)  ·  Ternary-Native Inference       │
│  FFIDispatcher · FFILibraryRegistry                         │
│  TWMATMUL · TQUANT · TATTN · TWEMBED · TERNACCUM · TACT     │
└─────────────────────────────────────────────────────────────┘
  Experimental: TernaryOS (Axion OS Kernel) · Cognitive Tiers
```

### Key components

**TISC ISA v1.2** — Ternary Instruction Set Architecture. Frozen under v1.x; the immutable execution contract for the entire stack. v1.2 adds 9 opcodes: `AgentInvoke` (RFC-0015), six ternary-native inference ops (RFC-0034), and three governed FFI ops (RFC-00B8).

**T81VM** — Deterministic TISC interpreter. Guarantees bit-identical output across platforms; Axion pre-dispatch isolation keeps governance hooks outside the hot execution path. Full TISC v1.2 dispatch including ternary-native inference and FFI.

**Axion Governance Kernel** — Policy engine that intercepts `AXREAD`, `AXSET`, `AXVERIFY`, AI opcodes, and FFI calls before any side effect. Fail-closed on policy parse failure. Stable-certified 2026-03-15 with 54/54 tests passing.

**CanonFS** — Content-addressed filesystem. Stores all code objects, model weights, and runtime artifacts as immutable, hash-identified blobs. Provides provenance for determinism audits.

**T81Lang** — High-level language targeting TISC bytecode. Native types: `BigInt`, `Fraction`, `Float`, `Complex`, `Tensor`, `Map`, `Set`. First-class `agent { behavior }` declarations compile to `AGENT_INVOKE` with Axion audit (RFC-0015). `foreign [policy] { fn … }` blocks declare governed external functions that call via `FFI_CALL` (RFC-0036). `agent`, `behavior`, and `foreign` are usable as contextual identifiers in all expression and binding positions. Compiler pipeline: lexer → parser → typed AST → semantic analysis → IR generation.

**Ternary-Native Inference (RFC-0034)** — Six TISC opcodes for multiplication-free AI inference using balanced ternary weights {−1, 0, +1}: `TWMATMUL` (matmul), `TQUANT` (quantize to trit), `TATTN` (ternary attention), `TWEMBED` (weight embedding), `TERNACCUM` (scalar dot product), `TACT` (activation with Axion ceiling gate). T81WTN weight format. T81Lang `foreign {}` frontend complete via RFC-0036.

**Governed FFI (RFC-00B8 + RFC-0036)** — Full-stack governed foreign function interface. VM layer (RFC-00B8 Phase 1): `FFIDispatcher` enforces policy checks, resource quotas, and audit trails before any foreign call; `FFILibraryRegistry` tracks registered libraries by name and version hash; three VM opcodes (`FFICall`, `FFIRegister`, `FFIPolicySet`). Language layer (RFC-0036): `foreign deterministic { fn sin(x: T81Float) -> T81Float; }` declares signatures; `foreign.sin(angle)` at call sites lowers to `FFI_CALL` with the function name carried in `text_literal`. Nine acceptance tests pass.

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
| RFC-00B5 interrupt policy | ✅ **COMPLETED 2026-03-16** | Governed event interrupt model integrated; slices 26-28 complete |
| RFC-0034 Ternary-Native Inference | ✅ **COMPLETED 2026-03-16** | 6 new TISC opcodes; multiplication-free inference; TACT activation-ceiling gate; 5/5 conformance tests |
| RFC-00B8 Governed FFI (Phase 1) | ✅ **COMPLETED 2026-03-16** | FFI dispatcher + library registry; 3 VM opcodes; governance pipeline; audit trail |
| Cross-platform determinism CI | ✅ **COMPLETED 2026-03-16** | Daily GitHub Actions workflow; Linux x86\_64 + macOS ARM64 hash comparison; public evidence record |
| RFC-0036 T81Lang FFI Grammar | ✅ **COMPLETED 2026-03-16** | `foreign [policy] {}` syntax; `foreign.<name>(args)` → `FFI_CALL`; 9/9 AC tests; connects RFC-0034 + RFC-00B8 VM work to T81Lang frontend |
| Stage 2: Verified Platform | ✅ **ACHIEVED 2026-03-16** | All implementation goals complete; trace replay debugger, cross-platform CI, 365/365 tests, FFI frontend — externally reproducible stack |
| RFC-0037 TNN stdlib | ✅ **COMPLETED 2026-03-16** | `std.tnn.*` T81Lang builtins (6 functions → RFC-0034 TISC ops); 13/13 tests; full-stack multiplication-free inference from source to VM |
| RFC-0038 Lattice Crypto | ✅ **COMPLETED 2026-03-16** | `POLYMUL`/`POLYMOD` TISC opcodes; `std.crypto.polymul/polymod` builtins; negacyclic poly multiply over {−1,0,+1}; T81BigInt-exact; 13/13 tests |
| T81Lang spec promotion (v1.3) | ✅ **COMPLETED 2026-03-16** | RFC-0036/0037/0038 promoted to normative spec; §5.17 un-stubbed; §5.18/5.19 added; opcode registry updated to 205 entries |
| RFC-0039 NTRU-KEM | ✅ **COMPLETED 2026-03-16** | `TVecSub` opcode; `std.crypto.{polyadd,polysub,ntru_encrypt,ntru_decrypt}`; C++ KEM math layer; 24/24 tests; full ring {+,−,×,mod} over Z\[x\]/(x^n+1) |
| TernaryOS bare-metal boot | TBD | x86\_64 QEMU host execution + CanonFS evidence return |

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

## Strategic Applications

The structural advantages of the T81 stack—specifically the **10.4x negation throughput** and **1.58 bits/trit density**—enable solutions for legacy binary bottlenecks:

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

### The Refined Path Forward

#### 1. Integration: RFC-0034 §5.17.6 — The `TACT` Opcode

Instead of a sprawling AI RFC, we treat activation as the logical conclusion of the ternary arithmetic chain.

* **Opcode:** `TACT RD, R_SRC, R_MODE`
* **Modes:** * `0x01` (TernaryStep): Maps $(-\infty, -0.5) \to -1$, $[-0.5, 0.5] \to 0$, $(0.5, \infty) \to +1$.
* `0x02` (TanhQuantized): High-fidelity fixed-point ternary approximation.

* **Axion Policy Integration:** We define the `AX_CHECK_ACTIVATION_THRESHOLD` not as an opcode side-effect, but as a **Kernel Trap**. If the value in `RD` exceeds the policy-defined trit-limit post-activation, Axion intercepts before the next PC increment.

#### 2. The T81Lang Grammar RFC (New)

To address the "Real Gap" you identified, we should draft a separate RFC (likely **RFC-0036**) specifically for the compiler frontend. This keeps the **TISC** (hardware/VM) and **T81Lang** (grammar/syntax) concerns isolated, as per the Project Charter.

#### 3. Data Integrity & Documentation

* **Benchmark Grounding:** I’ll stop referencing the "10.4x" figure in formal docs until we have a specific `BM_Negation_TISC_vs_Binary` slice that officially appears in the CI output.
* **Terminology Scrub:** I'll remove "TLU Cache" and "L2 Cache" from the specs until the **ternary-fabric** repository formally defines the memory hierarchy.

---

### Stage 1 — Prototype Architecture *(Current)*

**Status:** Achieved

Core deterministic stack implemented.

Components in place:

* ✅ TISC ISA (frozen execution contract)
* ✅ T81VM deterministic interpreter
* ✅ Axion governance kernel
* ✅ CanonFS content-addressed storage
* ✅ T81Lang compiler
* ✅ determinism verification pipeline
* ✅ CLI and TUI operator interfaces

**Outcome:**
A functioning deterministic computing stack.

---

### Stage 2 — Verified Platform *(Complete)*

**Goal:** Independent validation.

Key work:

* ✅ third-party determinism verification — daily GitHub Actions workflow compares Linux x86\_64 and macOS ARM64 bytecode hashes; public evidence record on every commit
* ✅ VM conformance test suite — 27 spec conformance tests + 365 total passing
* ✅ deterministic benchmarking framework — RFC-00A2; `score=1.0` across all runs
* ✅ T81Lang FFI frontend (RFC-0036) — `foreign {}` grammar bridges VM layer to language; 9/9 AC tests
* ✅ trace replay debugger — `t81 trace replay <tisc> <golden> [--json]`; schema `t81.trace-replay.v1`; reports exact mismatch index + expected/actual instruction; wired into CI via `scripts/ci/trace_repro_gate.py`
* ✅ reproducible build verification — cross-platform bytecode hash verified daily on Linux x86\_64 (gcc-14) + macOS ARM64 (clang); 90-day evidence artifacts retained

**Outcome:**
Externally trusted deterministic runtime.

---

### Stage 3 — Research Ecosystem

Focus shifts to applications.

Primary research areas:

* ternary neural networks
* deterministic AI inference
* signal processing libraries
* physics simulation
* lattice-based cryptography

**Outcome:**
Adoption by researchers and experimental compute projects.

---

### Stage 4 — Hardware Exploration

Bridge software architecture to silicon.

Development path:

* FPGA ternary ALU prototypes
* ternary register banks
* packed-trit SIMD units
* ISA microarchitecture validation

**Outcome:**
First ternary-aware compute hardware prototypes.

---

### Stage 5 — Deterministic Infrastructure

Expand from runtime to infrastructure.

Possible capabilities:

* deterministic cloud execution
* reproducible scientific computation
* verifiable distributed workloads
* CanonFS artifact networks

**Outcome:**
A global deterministic computation platform.

---

### Stage 6 — New Computing Paradigm

Long-term possibility.

Potential developments:

* native ternary processors
* hardware AI governance enforcement
* deterministic AI execution environments
* globally reproducible compute systems

**Outcome:**
A governed deterministic computing ecosystem.

---

## Critical Next Milestones

### Stage 2 — Verified Platform *(Achieved)*

All Stage 2 implementation goals are complete:

- ✅ cross-platform determinism CI (Linux x86\_64 + macOS ARM64, daily)
- ✅ VM conformance + determinism test suite (365/365)
- ✅ trace replay debugger (`t81 trace replay`; schema `t81.trace-replay.v1`)
- ✅ T81Lang FFI frontend (RFC-0036; `foreign {}` + `FFI_CALL`)

Remaining advancement criterion: **independent reproduction by an external party** — when another group builds the stack, runs the determinism gate, and publishes matching hashes, the project formally graduates from Stage 2.

### Stage 3 — Research Ecosystem *(Active)*

Stage 3 opened with three concrete tracks. All three are now complete:

- ✅ **RFC-0037 TNN stdlib** — `std.tnn.*` T81Lang builtins; 6 functions lower to RFC-0034 TISC ops; 13/13 tests
- ✅ **RFC-0038 Lattice Crypto** — `std.crypto.polymul/polymod`; POLYMUL/POLYMOD opcodes; T81BigInt-exact; 13/13 tests
- ✅ **T81Lang spec v1.3** — RFC-0036/0037/0038 promoted to normative spec; §5.17 un-stubbed; §5.18–5.19 added

- ✅ **RFC-0039 NTRU-KEM** — `TVecSub` opcode; `std.crypto.{polyadd,polysub,ntru_encrypt,ntru_decrypt}`; `ntru_keygen/encrypt/decrypt` C++ math layer; 24/24 tests; first end-to-end post-quantum cryptography demo on the ternary substrate

**Stage 3 is complete.** All four tracks (RFC-0037, RFC-0038, spec v1.3, RFC-0039) landed 2026-03-16.

---

## License

MIT License.
