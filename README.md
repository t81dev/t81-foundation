<p align="center">
  <img src="assets/banner.png" alt="T81 — A Ternary Computing Architecture" width="100%">
</p>

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

<!-- T81-SPEED-START -->
<!-- T81-SPEED-END -->

# T81 — A Ternary Computing Architecture

![Release](https://img.shields.io/badge/release-v1.9.1--Stable-blue)
![Tests](https://img.shields.io/badge/tests-369%2F369_passing-brightgreen)
![ISA](https://img.shields.io/badge/ISA-v1.9.0_Frozen-blue)
![Execution](https://img.shields.io/badge/execution-deterministic-green)
![CI](https://img.shields.io/badge/cross--platform--determinism-verified-brightgreen)
![License](https://img.shields.io/badge/license-Apache_2.0-blue)

Leveraging the theoretical efficiency of base-e computation, **T81** is a deterministic computing architecture built on **balanced ternary arithmetic** ({-1, 0, +1}) with a full-chain governance model covering instruction set, virtual machine, language compiler, and AI inference environment.

The architecture delivers:

- **bit-exact reproducibility on verified surfaces** — governed deterministic surfaces produce identical trace hashes across supported platforms
- **governed AI inference** — Axion policy engine intercepts and audits every privileged operation before side effects
- **content-addressed provenance** — CanonFS records all artifacts, model weights, and runtime state immutably
- **deterministic parallel execution for governed epoch semantics** — DPE task graph model (RFC-DPE-0002) enables concurrent TISC workloads with epoch-committed outputs under the current determinism surface

---

## Table of Contents
- [Project Status](#project-status)
- [Architecture & Ecosystem Overview](#architecture--ecosystem-overview)
- [Key Components](#key-components)
- [Quick Start](#quick-start)
- [Determinism Verification](#determinism-verification)
- [Documentation](#documentation)
- [Governance](#governance)
- [The Ternary Advantage](#the-ternary-advantage)
- [License](#license)

---

## Project Status

**Phase: Active Development** — v1.9.0-Stable; 369/369 tests passing; cross-platform determinism verified on Linux x86\_64 + macOS ARM64.

Deterministic-surface classification follows the governance model introduced by the Determinism Surface Registry and RFC-0048:

- **DCP / verified deterministic surface**: semantics, boundary, and replay proof are governed and CI-enforced
- **Governed non-DCP**: policy-bounded and architecturally important, but not yet entitled to full deterministic claims
- **Experimental**: active design or validation surface; no deterministic guarantee claim

| Component | Maturity | Notes |
| :--- | :--- | :--- |
| **TISC ISA** | ❄️ Frozen | v1.9.0; opcode semantics immutable under v1.x; `AgentInvoke` (RFC-0015), 6 ternary-native inference (RFC-0034), 3 FFI (RFC-00B8), 2 lattice crypto (RFC-0038), 1 KEM ring (RFC-0039) |
| **Data Types** | ❄️ Frozen | BigInt, Float, Complex, Map, Set — bit-stable encoding; clean audit |
| **T81VM** | ✅ Stable | DCP / verified deterministic surface for the interpreter and current supported-platform trace parity; full TISC v1.9.0 dispatch with `AgentInvoke`, ternary-native inference, FFI, lattice crypto, and NTRU-KEM opcodes; 369/369 tests |
| **T81Lang** | ✅ Stable | Governed non-DCP overall: spec v1.9.0 Stable with active compiler determinism controls, but compiler emission remains partially verified rather than fully promoted as a verified deterministic surface |
| **Axion Governance Kernel** | ✅ Stable | Governed non-DCP overall: canonical reason strings and audit hooks are live, but the full kernel/governance surface is broader than the currently verified deterministic registry |
| **Ternary-Native Inference** | ✅ Stable | Governed non-DCP: RFC-0034 + RFC-0037 opcode/runtime/stdlib surface is implemented and evidenced, but not all inference-adjacent execution paths are yet promoted as verified deterministic surfaces |
| **Lattice Cryptography** | ✅ Stable | Governed non-DCP: RFC-0038+0039 surface is implemented and policy-bounded; deterministic promotion remains surface-specific rather than implied for the entire crypto vertical |
| **Governed FFI** | ✅ Stable | Governed non-DCP: RFC-00B8 + RFC-0036 VM/language bridge is implemented end to end, but sandbox and broader schema promotion remain open before stronger deterministic claims |
| **TUI Frontends** | ✅ Beta | Governed non-DCP: operator and agent TUIs are production-usable interfaces, but UI/runtime integration is not itself a verified deterministic surface |
| **DPE (Parallel Execution)** | ✅ Stable | Governed deterministic execution model with accepted RFC-DPE-0001–0009; deterministic epoch semantics are in place, while broader surface promotion remains governed by the registry and companion RFC chain |
| **Cognitive Tiers** | ✅ Beta | Experimental / non-DCP: Tier4 Cognition remains governance-bounded but not a verified deterministic surface |
| **TernaryOS User Environment** | ✅ Beta | Governed non-DCP / beta: implemented and policy-bounded, but not currently presented as a verified deterministic surface |
| **Axion OS** | ✅ Alpha | Governed non-DCP / alpha: active governance architecture, but not yet a promoted verified deterministic surface as a whole |

---

## Architecture & Ecosystem Overview

```text
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
│  TISC ISA v1.9.0  ❄️ Frozen  +  Data Types  ❄️ Frozen       │
│  Deterministic substrate — CanonHash81 bit-exact traces     │
├─────────────────────────────────────────────────────────────┤
│  Governed FFI (RFC-00B8)  ·  Ternary-Native Inference       │
│  FFIDispatcher · FFILibraryRegistry                         │
│  TWMATMUL · TQUANT · TATTN · TWEMBED · TERNACCUM · TACT     │
└─────────────────────────────────────────────────────────────┘
  Experimental: TernaryOS · Cognitive Tiers
```

### Key Components

**TISC ISA v1.9.0** — Ternary Instruction Set Architecture. Frozen under v1.x; the immutable execution contract for the entire stack.

**T81VM** — Deterministic TISC interpreter. Guarantees bit-identical output across platforms; Axion pre-dispatch isolation keeps governance hooks outside the hot execution path.

**Axion Governance Kernel** — Policy engine that intercepts `AXREAD`, `AXSET`, `AXVERIFY`, AI opcodes, and FFI calls before any side effect. Fail-closed on policy parse failure.

**CanonFS** — Content-addressed filesystem. Stores all code objects, model weights, and runtime artifacts as immutable, hash-identified blobs. Provides provenance for determinism audits.

**T81Lang** — High-level language targeting TISC bytecode. Native types: `BigInt`, `Fraction`, `Float`, `Complex`, `Tensor`, `Map`, `Set`. Compiler pipeline: lexer → parser → typed AST → semantic analysis → IR generation.

**Ternary-Native Inference (RFC-0034)** — Six TISC opcodes for multiplication-free AI inference using balanced ternary weights {−1, 0, +1}: `TWMATMUL` (matmul), `TQUANT` (quantize to trit), `TATTN` (ternary attention), `TWEMBED` (weight embedding), `TERNACCUM` (scalar dot product), `TACT` (activation with Axion ceiling gate). T81WTN weight format. T81Lang `foreign {}` frontend complete via RFC-0036.

**Governed FFI (RFC-00B8 + RFC-0036)** — Full-stack governed foreign function interface. VM layer (RFC-00B8 Phase 1): `FFIDispatcher` enforces policy checks, resource quotas, and audit trails before any foreign call; `FFILibraryRegistry` tracks registered libraries by name and version hash; three VM opcodes (`FFICall`, `FFIRegister`, `FFIPolicySet`). Language layer (RFC-0036): `foreign deterministic { fn sin(x: T81Float) -> T81Float; }` declares signatures; `foreign.sin(angle)` at call sites lowers to `FFI_CALL` with the function name carried in `text_literal`.

**TUI Frontends** — Two complementary terminal interfaces built on FTXUI v5.0.0:
- `t81 studio` — navigation sidebar, CanonFS browser, Axion dashboard, determinism trace visualizer, command palette (`Ctrl+P`)
- `t81 agent` — persistent JSONL session, slash commands (`/compile`, `/run`, `/hash`, `/allow`, `/infer`, `/trits`, …), trit-probability bar

**DPE (Deterministic Parallel Execution)** — Task graph model over the frozen TISC ISA. Tasks declare immutable inputs and buffered output regions; the VM commits all writes atomically at epoch end. No new opcodes required.

---

## Quick Start

### Try it in Docker (60 seconds, nothing to install)

```sh
docker run --rm -it ghcr.io/t81dev/t81-foundation demo
```

This pulls a ~100 MB image, runs three short programs showing ternary types and
determinism guarantees, then drops you into an interactive T81Lang REPL — all
without touching your local toolchain. Exit with `:quit` or `Ctrl-D`.

```sh
# Interactive REPL only
docker run --rm -it ghcr.io/t81dev/t81-foundation

# Run any t81 subcommand
docker run --rm -it ghcr.io/t81dev/t81-foundation vm run /t81/examples/tisc/hello_world.tisc
```

---

### Download a prebuilt binary

Prebuilt binaries are published for every release — no compiler required.

**macOS / Linux** (one-liner):

```sh
curl -fsSL https://github.com/t81dev/t81-foundation/releases/latest/download/install.sh | sh
```

**Windows** (PowerShell):

```powershell
irm https://github.com/t81dev/t81-foundation/releases/latest/download/install.ps1 | iex
```

Or download an archive directly from the [latest release](https://github.com/t81dev/t81-foundation/releases/latest):

| Platform | Archive |
| :--- | :--- |
| Linux x86\_64 | `t81-<version>-linux-x86_64.tar.gz` |
| Linux ARM64 | `t81-<version>-linux-arm64.tar.gz` |
| macOS Apple Silicon | `t81-<version>-macos-arm64.tar.gz` |
| macOS Intel | `t81-<version>-macos-x86_64.tar.gz` |
| Windows x86\_64 | `t81-<version>-windows-x86_64.zip` |

Each archive follows the standard install layout (`bin/`, `lib/`, `include/`). Extract and place `bin/t81` (or `bin\t81.exe`) anywhere on your `PATH`.

### Build from source

```bash
# Clone the repository
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation

# Configure and build (Release mode)
cmake --preset default -DCMAKE_BUILD_TYPE=Release
cmake --build build

# Run the test suite (369 tests)
ctest --test-dir build --output-on-failure
```

### CLI & Interfaces

```bash
# Compile a T81Lang program
t81 code build examples/hello.t81 -o hello.tisc

# Execute with Axion governance
t81 vm run hello.tisc

# Launch the human operator TUI
t81 studio

# Launch the AI-native TUI
t81 agent
```

---

## Determinism Verification

Verified deterministic surfaces are checked for bit-exact cross-platform reproducibility.

```bash
./scripts/ci/run_determinism_slice.sh
```

Verified platforms for the current core surface: **Linux x86_64**, **macOS ARM64**. Any divergence on a verified deterministic surface is a critical defect.

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

## Governance

T81 Foundation operates under a **Continuous Governance (C2)** model. All contributions must maintain:

- **deterministic execution parity** — trace hashes must match across supported platforms
- **architectural coherence** — changes that touch the deterministic surface require formal review
- **reproducibility guarantees** — no floating-point or platform-specific non-determinism in the DCP surface

The deterministic surface is defined in `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`. Changes to frozen surfaces (TISC ISA, Data Types) require a major version bump.

> **Boundary note:** DCP, governed non-DCP, experimental, and out-of-scope classifications are defined constitutionally in RFC-0048. Public docs must not present governed non-DCP or experimental surfaces as verified deterministic components.

---

## The Ternary Advantage

While modern binary hardware is highly optimized for general-purpose computing, **T81 Foundation** exploits the unique mathematical and structural properties of **balanced ternary** ({−1, 0, +1}) to deliver advantages that are difficult or impossible to achieve in conventional binary systems — especially in deterministic execution, governed AI inference, and low-complexity neural workloads.

### 1. O(1) Computational Symmetry — Negation Without Carry

In two's complement binary, negation requires a bitwise NOT followed by +1, which can trigger long carry chains. In balanced ternary, negation is simply flipping the sign of every non-zero trit (+1 ↔ −1, 0 stays 0) — **zero carry propagation**, constant time.

- **Measured performance**: PackedCell negation reaches **~49.9 G-ops/s** on recent x86_64 hardware, outperforming optimized 64-bit integer negation by **~10.9×** (benchmarks verified on Linux x86_64 and macOS ARM64).
- This symmetry extends naturally to many arithmetic patterns, reducing latency and power in sign-heavy operations.

### 2. Superior Radix Economy & Theoretical Density

The information-theoretic optimum radix for positional number systems is close to *e ≈ 2.718*. Ternary (base 3) is mathematically closer than binary (base 2), delivering **~1.585 bits of information per trit** (log₂(3)).

- In practice this enables higher entropy per digit and more compact representation of symmetric ranges — especially valuable for neural network weights, embeddings, coordinate systems, and large-scale sparse tensors.

### 3. Inherent Bit-Exact Determinism & Platform-Independent Rounding

IEEE 754 floating-point suffers from platform-specific rounding modes, associativity differences, and denormal handling that break reproducibility. Balanced ternary arithmetic is naturally symmetric around zero:

- Rounding uses simple truncation — no direction-dependent bias.
- Every execution path produces **identical trace hashes** across supported platforms (currently Linux x86_64 + macOS ARM64 verified at 100% parity).
- This provides rock-solid reproducibility for scientific computing, AI inference auditing, and governed agent runs.

### 4. Multiplication-Free Neural Inference

Balanced ternary weights {−1, 0, +1} allow **multiplication-free dot products** — replace MUL with conditional ADD/SUB (or pure accumulation when skipping zeros). Combined with custom TISC opcodes (`TWMATMUL`, `TQUANT`, `TATTN`, `TWEMBED`, `TERNACCUM`, `TACT`):

- Enables dramatically lower power and higher throughput for AI inference.
- Aligns with 2024–2026 trends in extreme low-bit models (BitNet b1.58, xTern, ternary transformers), delivering 15–60× energy reduction and 4–90× throughput gains vs FP16/FP32 baselines with minimal accuracy loss.
- T81WTN weight format + ternary-native operations make this advantage production-ready in the stack.

### 5. Architectural Governance & Security Hooks

Because the entire TISC ISA is ternary-native, the **Axion Governance Kernel** can intercept and audit state transitions at **trit-level granularity** before any side effect occurs. This enables:

- Fail-closed policy enforcement on privileged operations (AI invokes, FFI calls, agent behaviors).
- Fine-grained ethics gates, provenance tracking via CanonFS, and deterministic audit trails.
- A security model that is fundamentally more inspectable than black-box binary execution.

These advantages compound in domains where **reproducibility**, **low-complexity inference**, **governed execution**, and **mathematical symmetry** matter most — exactly the target use cases of the T81 architecture.

---

## License

Apache License 2.0.
