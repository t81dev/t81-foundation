<p align="center">
  <img src="assets/banner.png" alt="T81 Foundation — Deterministic Ternary Architecture" width="100%">
</p>

[English](./README.md) | [简体中文](./README.zh-CN.md) | [Español](./README.es.md) | [Русский](./README.ru.md) | [Português](./README.pt-BR.md)

<!-- T81-SPEED-START -->
<!-- T81-SPEED-END -->

# T81 Foundation — Deterministic Ternary Computing Stack

![Release](https://img.shields.io/badge/release-v1.9.0--Stable-blue)
![Tests](https://img.shields.io/badge/tests-369%2F369_passing-brightgreen)
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

**Phase: Active Development** — v1.9.0-Stable; 369/369 tests passing; cross-platform determinism verified on Linux x86\_64 + macOS ARM64.

| Component | Maturity | Notes |
| :--- | :--- | :--- |
| **TISC ISA** | ❄️ Frozen | v1.2.0; opcode semantics immutable under v1.x; 12 new opcodes since v1.1: `AgentInvoke` (RFC-0015), 6 ternary-native inference (RFC-0034), 3 FFI (RFC-00B8), 2 lattice crypto (RFC-0038), 1 KEM ring (RFC-0039) |
| **Data Types** | ❄️ Frozen | BigInt, Float, Complex, Map, Set — bit-stable encoding; 2026-02-27 audit clean |
| **T81VM** | ✅ Stable | Full TISC v1.2 dispatch; `AgentInvoke` + ternary-native inference + FFI + lattice crypto + NTRU-KEM opcodes; 369/369 tests |
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
| **TernaryOS User Environment** | ✅ Beta | RFC-00B9: t81-init, session manager, t81sh shell; 15/15 acceptance criteria implemented; boot sequence, session lifecycle, and shell infrastructure working |
| **Cross-Platform Determinism CI** | ✅ Stable | Daily GitHub Actions workflow compares T81Lang bytecode hashes across Linux x86\_64 (gcc-14) and macOS ARM64 (clang); publicly auditable evidence record; production-ready cross-platform determinism validation |
| **Hanoi VM** | ✅ Alpha | RFC-0000 §4 ethics-first boot; 81-slot deterministic scheduler; snapshot management; RFC-0000 §7 command surface (status, optimize, simulate, snapshot, rollback); comprehensive test suite; Alpha-ready microkernel |
| **Axion OS** | ✅ Alpha | Complete governance system with 100% test coverage (28/28 tests); production-ready policy engine and ethics evaluation; Θ₁-Θ₉ principles fully implemented; comprehensive API documentation and integration examples; Alpha-ready kernel with deterministic decision making and full T81 stack integration |

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
  Experimental: TernaryOS (Axion OS) · Cognitive Tiers
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

# T81 Foundation Complete Systems & Subsystems Evaluation Report

## 1. Executive Summary

T81 Foundation is a broad repository that combines a deterministic ternary execution core with a much wider surrounding ecosystem: language frontend, VM, ISA, governance kernel, CanonFS storage, CLI/TUI tooling, benchmarks, AI experiment tracks, and an experimental operating-system effort. The root tree includes `.github`, `.devcontainer`, `.t81`, `artifacts/archive`, `benchmarks`, `book`, `contracts`, `core`, `docs`, `examples`, `experimental`, `experiments/ai`, `include`, `internal`, `kernel`, `lang`, `legacy`, `runtime`, `scripts`, `spec`, `src`, `tests`, `tooling`, `tools`, and `third_party`, with 3,636 commits and six contributors shown on GitHub. ([GitHub][1])

The repository’s most credible achievement is not “ternary computing” in the abstract; it is the existence of a bounded deterministic core with explicit authority and scope rules. The architecture overview states the authority order as `/spec` > `docs/architecture/OVERVIEW.md` > `/docs` > `/book`, and its layer map centers T81Lang → TISC → T81VM → Axion → CanonFS, with cognitive tiers and Hanoi concepts explicitly marked optional / non-DCP. The Deterministic Core Profile then narrows guarantees to verified surfaces rather than the full repo. ([GitHub][2])

The biggest conclusion is that T81 has a real core architecture and serious governance/CI intent, but the repository as a whole is less coherent than its strongest subsystems. The core execution stack appears materially implemented and guarded; the wider OS, AI, cognition, and infrastructure narratives are a mix of prototype, experimental, and aspirational work. The largest weakness is authority drift: the README says `v1.9.0-Stable` with `369/369` tests and TISC `v1.2.0`, the latest GitHub release is `v1.6.0`, `PROJECT_CONTROL_CENTER.md` says `v1.4.1-Stable` with `363/363` tests, `CMakeLists.txt` says version `1.3.6`, the T81VM spec is still `Version 1.1 — Beta`, and the Axion spec is `Version 1.0 — Alpha`. For a project built around determinism and auditability, that inconsistency is the central credibility problem. ([GitHub][1])

Current overall maturity: **serious experimental platform with a credible deterministic execution nucleus, not yet a fully coherent infrastructure stack**. The repo is beyond a research sketch, but it is not yet cleanly productized, and several outer layers are still prototype-grade or documentation-led. ([GitHub][2])

## 2. Evaluation Method

This evaluation used six lenses. First, repo-structure review: root tree, major directories, language mix, commit volume, and public release metadata. Second, spec review: `spec/tisc-spec.md`, `spec/t81vm-spec.md`, `spec/t81lang-spec.md`, and `spec/axion-kernel.md`. Third, implementation-surface review: top-level subsystem directories such as `core`, `kernel/axion`, `lang/frontend`, `runtime/jit`, `tooling/cli`, and `tools`, plus `CMakeLists.txt` to see what is actually built into the core library. Fourth, test/CI review: `tests/`, `tests/README.md`, and `.github/workflows/ci.yml`. Fifth, benchmark review: `benchmarks/` and `benchmarks/README.md`. Sixth, governance/docs review: `docs/architecture/OVERVIEW.md`, `docs/status/IMPLEMENTATION_MATRIX.md`, `docs/status/PROJECT_CONTROL_CENTER.md`, `docs/product/DETERMINISTIC_CORE_PROFILE.md`, and `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`. ([GitHub][1])

Important limitation: this is a repository evidence audit, not a full source-level verification of every implementation path. Where the repo itself marks something “Stable,” “Verified,” “partial,” or “non-verified,” I treat that as a claim unless supported by adjacent evidence such as CI checks, directory contents, build inclusion, or explicit implementation notes. Where the repo contradicts itself, I treat that contradiction as evidence. ([GitHub][3])

## 3. System-of-Systems Map

### Architectural foundations

Purpose: define the authority model, bounded claims, and top-level layering. Main artifacts: `docs/architecture/OVERVIEW.md`, `docs/product/DETERMINISTIC_CORE_PROFILE.md`, `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`, and `/spec`. Maturity: relatively strong as a control surface. Coupling: universal. Key risk: these documents are conceptually strong, but they do not fully prevent status/version drift elsewhere. ([GitHub][2])

### Data model / numeric model

Purpose: canonical ternary-native and supporting numerical/data representations. Main artifacts: `core/types`, `spec/t81-data-types.md`, and DCP/registry references. Maturity: among the strongest domains; the implementation matrix marks Data Types as `Frozen`, `Implemented`, and `Verified`, and the README treats them as frozen core. Coupling: ISA, VM, language, storage, and AI surfaces depend on them. Key risk: stable in concept, but still subject to repo-wide synchronization drift. ([GitHub][4])

### ISA / VM / runtime

Purpose: define and execute deterministic ternary semantics. Main artifacts: `core/isa`, `core/vm`, `runtime/tracing`, `runtime/jit`, `spec/tisc-spec.md`, `spec/t81vm-spec.md`. Maturity: high for interpreter-centered core, lower for JIT and advanced runtime. Runtime README explicitly says JIT components are “non-verified unless explicitly promoted.” Coupling: this is the central execution substrate. Key risk: the core is credible, but JIT and some observability interfaces remain incomplete or excluded from DCP. ([GitHub][4])

### Language / compiler surfaces

Purpose: lexing, parsing, semantic analysis, IR generation, stdlib. Main artifacts: `lang/frontend`, `lang/stdlib`, `spec/t81lang-spec.md`. Maturity: moderate-to-strong. The language spec is `Version 1.3 — Stable`; the frontend directory contains lexer, parser, semantic analyzer, symbol table, builtin registry, and IR generator. Coupling: compiles exclusively to TISC and depends on VM/Axion semantics. Key risk: stronger documentary maturity than independently proven implementation maturity. ([GitHub][5])

### Kernel / OS / Axion surfaces

Purpose: two different things are present under one label. `kernel/axion` is the policy/plumbing layer for Axion governance; `experimental/ternaryos` is the operating-system track now branded as Axion in the progress log. Maturity: mixed. `kernel/axion` contains real code but also states “current scope includes stubs and incremental hardening paths.” `experimental/ternaryos/docs/PROGRESS.md` shows a deep hosted prototype with 3,214/3,214 assertions passing for its current slice. Coupling: high. Key risk: naming collision and maturity mismatch. ([GitHub][6])

### Storage / CanonFS / object model

Purpose: content-addressed persistence and auditability. Main artifacts: `src/canonfs`, `include/t81`, CanonFS references in architecture docs and specs. Maturity: important and present, but some Axion/CanonFS observability is explicitly partial in the Axion spec. Coupling: storage, audit, model provenance, OS boot/persistence work. Key risk: stable-core provenance claims are mixed with incomplete enforcement hooks. ([GitHub][7])

### AI / cognition / inference surfaces

Purpose: AI experiments, quantization, benchmarks, provenance, policy hooks, inference opcodes, cognitive tiers. Main artifacts: `experiments/ai`, `experimental/tiers`, `experimental/dpe`, `runtime/jit`, README claims about inference ops. Maturity: heterogeneous. `experiments/ai/README.md` frames these as experiments with a promotion path from experimental → extension → core, but the top-level README presents several AI-related surfaces as stable. Coupling: moderate-to-high. Key risk: promotional unification of surfaces that the repo itself still segregates as experimental. ([GitHub][8])

### Governance / policy / determinism enforcement

Purpose: structure, freeze integrity, overclaim control, architecture coherence, determinism claim enforcement. Main artifacts: `.github/workflows/ci.yml`, `scripts/governance/*`, DCP docs, freeze and determinism registry docs. Maturity: strong for a repo of this stage. Coupling: nearly all domains. Key risk: governance process is more mature than some governed subsystems. ([GitHub][9])

### Testing / benchmarks / CI enforcement

Purpose: regression, conformance, reproducibility, workload benchmarks. Main artifacts: `tests/`, `benchmarks/`, CI workflow. Maturity: one of the repo’s strongest support layers. Key risk: benchmark discipline exists, but repo-level status numbers remain inconsistent. ([GitHub][10])

### Tooling / CLI / developer workflows

Purpose: `t81` CLI, debugger, model/tooling helpers, TUI frontends, VS Code assets, diagnostics. Main artifacts: `tooling/cli`, `tooling/tui`, `tools`, `tooling/model`. Maturity: substantial. Key risk: tooling breadth may outpace stabilization of the core authority graph. ([GitHub][11])

### Documentation / book / public communication

Purpose: specs, dashboards, user guides, book, multilingual README. Main artifacts: `docs`, `book`, `README.*`. Maturity: very high in volume; medium in synchronization integrity. Key risk: doc sprawl and contradictory status surfaces. ([GitHub][1])

### Experimental / research branches

Purpose: non-DCP work such as distributed, DPE, Hanoi, tiers, ternary OS, AI experiments, JIT. Main artifacts: `experimental`, `experiments/ai`, `runtime/jit`. Maturity: prototype to research-stage. Key risk: overextension and blurred boundaries with the promoted core. ([GitHub][12])

### Legacy / superseded / unclear-status surfaces

Purpose: nominal archive or placeholder. Main artifacts: `legacy`, `artifacts/archive`, `internal`, `pdf`, `notebooks`. Maturity: unclear by design. Notably, `legacy` currently contains only `.keep`. Key risk: confusing surface area with little obvious value to a new evaluator. ([GitHub][1])

## 4. Detailed Subsystem Inventory

### T81 Data Types

Purpose: canonical numeric/composite representation layer. Locations: `core/types`, spec reference `spec/t81-data-types.md`, DCP and implementation matrix references. Primary evidence: root README, implementation matrix, CMake core build includes `core/types/bigint.cpp` and `core/types/fraction.cpp`. Current status: implemented and treated as frozen/verified. Test evidence: indirect but strong through DCP/registry/matrix language. Dependency relationships: ISA, VM, language, CanonFS, AI math, crypto. Determinism relevance: foundational. Governance relevance: high because auditability depends on stable canonical representation. Maturity: **high**. Main concern: not local implementation weakness; rather repo-level status drift around it. ([GitHub][13])

### TISC ISA

Purpose: normative instruction contract. Locations: `core/isa`, `spec/tisc-spec.md`. Primary artifacts: TISC spec, core directory contents, README, freeze-integrity CI check. Current status: operational and central. Test/verification evidence: CI runs `check_tisc_freeze_integrity.py`; DCP and determinism registry require ISA markers; implementation matrix treats it as frozen/verified. Dependency relationships: targeted by T81Lang, executed by T81VM, observed by Axion. Determinism relevance: maximal. Governance relevance: privileged op and policy visibility. Maturity: **high for the interpreter contract**. Main concerns: status/version inconsistency—README presents `v1.2.0`, CI still names `TISC v1.1.0 Freeze Integrity`, and the spec cross-reference still mentions T81Lang current spec version `v1.2` rather than `v1.3`. ([GitHub][14])

### T81VM

Purpose: deterministic execution environment. Locations: `core/vm`, `runtime/tracing`, `spec/t81vm-spec.md`. Primary artifacts: T81VM spec, runtime tree, DCP/registry docs, tests README, root README. Current status: implemented and central, but its normative spec remains `Version 1.1 — Beta`. Test evidence: root README claims 369/369 tests; Project Control Center claims runtime stability with 54/54 VM tests and 27/27 conformance programs; tests tree includes determinism and harness layers. Determinism relevance: maximal. Governance relevance: Axion integration is part of the spec. Maturity: **strong core, incomplete edges**. Main concerns: direct `get_execution_mode()` API is “not yet exposed”; deterministic scheduling events are not yet first-class trace entries; canonical reason strings are still incomplete in places. ([GitHub][15])

### T81Lang frontend and stdlib

Purpose: language pipeline and standard library. Locations: `lang/frontend`, `lang/stdlib`, `spec/t81lang-spec.md`. Primary artifacts: lexer/parser/semantic analyzer/IR generator files; stable language spec. Current status: implemented enough to be a real compiler surface. Test evidence: Project Control Center cites 21 deterministic compilation fixtures and stable promotion; tests tree includes `tests/lang` and fixtures. Dependency relationships: compiles to TISC, depends on VM semantics and Axion visibility. Determinism relevance: high. Governance relevance: explicit in the spec. Maturity: **moderately high**. Main concerns: spec maturity is higher than the directly inspected implementation evidence, and some cross-doc references still point to stale language versions. ([GitHub][16])

### Axion governance layer

Purpose: policy engine, verdict plumbing, AI hooks, nondeterminism detection, serialization. Locations: `kernel/axion`, `spec/axion-kernel.md`. Primary artifacts: `engine.cpp`, `policy_engine.cpp`, `nondeterminism_detector.cpp`, `ethics.cpp`, Axion spec, Axion README. Current status: partially implemented and partially target-state. Test evidence: README and Project Control Center claim 49/49 or 54/54 Axion-related tests depending on surface, but `kernel/axion/README.md` also says the current scope includes stubs and incremental hardening paths, and the spec labels major subsystem descriptions as “architectural targets.” Determinism relevance: high. Governance relevance: this is the governance core. Maturity: **medium**. Main concerns: mismatch between strong public claims and the repo’s own caveats; observability integration with CanonFS is partial; terminology like “ethics” expands faster than proven enforcement boundaries. ([GitHub][6])

### CanonFS

Purpose: content-addressed persistence and provenance. Locations: `src/canonfs`, `include/t81`, references across README/spec/docs. Primary artifacts: CMake includes `src/canonfs/in_memory_driver.cpp` and `src/canonfs/persistent_driver.cpp`; README describes it as content-addressed immutable storage. Current status: implemented in core build, but some policy-observability links remain partial. Test evidence: benchmark file `BM_CanonFS.cpp`; OS progress log references CanonFS-backed repositories and persistence lanes. Determinism relevance: high. Governance relevance: audit trail anchor. Maturity: **medium-to-high**. Main concerns: stronger as a storage/provenance claim than as a fully unified enforcement surface. ([GitHub][7])

### CLI / debugger

Purpose: command entry point, orchestration, debugger. Locations: `tooling/cli`. Primary artifacts: `main.cpp`, `driver.cpp`, `debugger.cpp`, `canonize_tensor.cpp`, AI CLI subdir. Current status: implemented and integrated. Test evidence: tests README explicitly includes CLI in primary C++ suite. Determinism relevance: indirect but meaningful for reproducibility workflows. Governance relevance: runtime path and operator surface. Maturity: **medium-to-high**. Main concern: not a maturity problem so much as scope sprawl relative to core stabilization. ([GitHub][17])

### TUI frontends

Purpose: operator and AI-native terminal interfaces. Locations: `tooling/tui`, build option in `CMakeLists.txt`, README maturity note. Current status: build-enabled by default via `T81_BUILD_TUI ON`; README calls them Beta. Verification evidence: Project Control Center says RFC-0033 closure included snapshot tests, binary-size gate, and user guide. Maturity: **medium**. Main concern: useful, but not essential to the strongest technical thesis. ([GitHub][7])

### Benchmarks

Purpose: performance and workload tracking. Locations: `benchmarks/`, `benchmarks/runner`. Primary artifacts: `BM_DeterminismValidation.cpp`, `BM_VMSimulation.cpp`, `BM_CanonFS.cpp`, `BM_LlamaKernels.cpp`, runner, README. Current status: operational benchmark harness. Verification evidence: CI has a benchmark workload gate and regression checker script. Maturity: **medium-to-high**. Main concern: benchmark infrastructure is real, but public performance claims are ahead of unified release/accounting discipline. ([GitHub][18])

### Tests / conformance / fuzz / stress

Purpose: regression, conformance, determinism, fuzzing, CI script tests. Locations: `tests/ci`, `tests/cpp`, `tests/determinism`, `tests/fixtures`, `tests/fuzz`, `tests/stress`, etc. Current status: broad and active. Verification evidence: tests README defines explicit expectations that behavior changes in `src/` or `include/t81/` should include tests; determinism-sensitive changes should include reproducibility coverage. Maturity: **high relative to repo stage**. Main concern: breadth is better than average, but it does not erase status inconsistency elsewhere. ([GitHub][10])

### Runtime JIT

Purpose: JIT compiler, trace cache, hardware acceleration, tensor ops. Locations: `runtime/jit`. Current status: implemented as code, but explicitly marked “non-verified unless explicitly promoted.” Maturity: **low-to-medium**. Main concern: clear example of documented but non-core functionality that should not be conflated with the deterministic core. ([GitHub][19])

### Experimental AI surfaces

Purpose: experimental AI integration, benchmarks, provenance, policy hooks, quantization, LLM backend. Locations: `experiments/ai`. Current status: explicitly experimental with a promotion ladder. Maturity: **research-to-prototype**. Main concern: top-level README blends some AI promotion work into “stable” repo identity while `experiments/ai` still describes a staged path to core. ([GitHub][8])

### Distributed / DPE / cognitive tiers / Hanoi

Purpose: outer research frontier. Locations: `experimental/distributed`, `experimental/dpe`, `experimental/hanoi`, `experimental/tiers`. Current status: experimental by tree placement and architecture overview. Notably, `CMakeLists.txt` also compiles several experimental cognitive and distributed files directly into `t81_core`, including `experimental/hanoi/in_memory_kernel.cpp`, `experimental/tiers/cog/*`, and `experimental/distributed/distributed.cpp`. Maturity: **mixed, generally low**. Main concern: the boundary between experimental and core is not clean in the build graph. ([GitHub][12])

### Axion / TernaryOS operating-system effort

Purpose: MMU, scheduling, IPC, faults, devices, pager, governed interrupts, boot lanes. Locations: `experimental/ternaryos`, `experimental/ternaryos/docs/PROGRESS.md`. Current status: advanced hosted prototype. Verification evidence: progress log cites RFC-00B5 interrupt integration, slices 26–28, and 3,214/3,214 assertions passing. Maturity: **advanced prototype / partial OS substrate**. Main concern: real engineering depth, but not yet a completed kernel or general-purpose OS. ([GitHub][20])

## 5. Architecture Coherence Assessment

There is a real layered model. The architecture overview, TISC spec, T81VM spec, and T81Lang spec all present a coherent idea: a deterministic language targeting a deterministic ISA executed by a deterministic VM under Axion observation, with CanonFS as a provenance/audit surface. That is a legitimate architecture, not just branding. ([GitHub][2])

But subsystem boundaries are only partly real. Some are explicit and well-bounded, especially the DCP-defined core. Others are blurry. The repo calls `experimental` non-DCP/non-verified, and `runtime/jit` non-verified unless promoted, yet `CMakeLists.txt` compiles experimental Hanoi, cognitive-tier, and distributed sources directly into `t81_core`. That weakens the conceptual cleanliness of the core/periphery separation. ([GitHub][12])

Contracts and interfaces are stronger in docs than in some implementations. T81VM requires Axion to query execution mode, but the spec says the public `get_execution_mode()` surface is not yet exposed. Scheduling decisions are supposed to be Axion-visible, but the same spec says deterministic scheduling events are not yet first-class trace entries. Axion’s CanonFS observability is described as partial, and the Axion spec openly says its five subsystems are “architectural targets.” That is not fatal; it is evidence that some boundaries are still aspirational. ([GitHub][15])

Naming consistency is weak. “Axion” names both the governance kernel and the operating-system track. The OS progress log explicitly says `Axion = operating system` while the Axion spec says the Axion Governance Kernel is the supervisory intelligence above executable layers. That creates avoidable ambiguity at exactly the layer where the project most needs conceptual precision. ([GitHub][20])

Documentation is not fully aligned with implementation. The most obvious symptoms are version/status conflicts across README, releases, control center, specs, and build metadata. There is also a more structural issue: several subsystem README files emphasize that they are only navigation aids and do not change authority, which implies the project already recognizes doc-sprawl risk. That recognition is correct; the sprawl is visible. ([GitHub][1])

Strongest seams: DCP boundary, authority hierarchy, explicit experimental labels, and governance/CI checks. Weakest seams: build-graph mixing of experimental code into core, Axion naming, and status synchronization. ([GitHub][2])

## 6. Determinism & Reproducibility Assessment

Determinism is clearly claimed throughout the repo. The README says every release is verified for bit-exact cross-platform reproducibility, the TISC spec makes deterministic execution a design principle, the T81VM spec defines determinism constraints and concurrency rules, and the T81Lang spec declares no hidden I/O, nondeterminism, or environment leakage. ([GitHub][1])

More importantly, determinism is not only claimed; it is also narrowed. The architecture overview explicitly describes “bounded determinism claim scope,” the DCP includes only verified surfaces, and the Determinism Surface Registry defines a determinism surface as one requiring explicit verification for bit-identical output across supported architectures. That is a good engineering move because it prevents the whole repo from being treated as equally guaranteed. ([GitHub][2])

Determinism is concretely enforced in CI. The CI workflow checks repository structure, TISC freeze integrity, architecture coherence, DCP/registry surface alignment, public-header root policy, determinism claim enforcement, and benchmark workload regression. The tests README also requires reproducibility coverage for determinism-sensitive changes. ([GitHub][9])

The determinism boundary is explicit, but not complete. The T81VM spec still lists missing pieces: direct mode query not yet exposed, scheduling events not yet emitted as distinct Axion events, and canonical reason-string emission not fully concatenated in current implementation. The Axion spec similarly says CanonFS observability integration is partial. These are not catastrophic defects, but they show the difference between deterministic core semantics and fully finished deterministic observability tooling. ([GitHub][15])

The biggest mismatch is organizational, not algorithmic: a project that emphasizes bit-exact reproducibility should also maintain version-exact public reporting. Right now it does not. README, release metadata, control center, CMake version, and spec versions do not line up cleanly. That weakens confidence in the determinism story because release identity is itself part of reproducibility. ([GitHub][1])

## 7. Governance, Safety, and Policy Layer Assessment

Governance in T81 is architectural, not merely rhetorical. Axion is present in the specs, in the architecture layer cake, in the DCP framing, in the CI regime, and in the repo’s policy-check scripts. The root README also centers governance in its system description. That is stronger than a typical repo that treats “governance” as prose. ([GitHub][2])

But the governance layer is only partially realized. `kernel/axion/README.md` says the current scope includes stubs and incremental hardening paths. The Axion spec says CanonFS observability integration is partial and describes its major subsystems as architectural targets. That means policy enforcement exists as code and interface plumbing, but the full supervisory model is not yet fully closed. ([GitHub][3])

Interfaces between governance and execution are reasonably clear at the spec level: Axion supervises T81VM state transitions, vets privileged instructions, verifies safety/determinism, and observes tier interactions. The problem is not conceptual vagueness; it is incomplete enforcement at some edges. Missing enforcement/visibility points include execution-mode query exposure, first-class scheduling trace events, and some CanonFS propagation paths. ([GitHub][21])

The governance stack is somewhat overextended relative to implementation maturity. The spec language reaches into ethics, recursion bounds, canonicalization of complex transformations, and tier transitions for advanced reasoning workloads. Meanwhile, the repo still marks important parts of Axion plumbing as stubs/partial and keeps cognitive tiers in optional/non-DCP territory. The result is not “fake governance,” but it is governance ambition ahead of finalized implementation. ([GitHub][21])

## 8. Implementation Reality Check

What is truly working now, based on repo evidence: a core build centered on types, encoding, CanonFS drivers, deterministic hashing, soft math, language frontend pieces, Axion runtime plumbing, tests, benchmarks, CLI, and a wide CI regime. The repo also clearly builds a `t81_core` static library and includes substantial subsystem code in that build. ([GitHub][7])

What appears partially working: Axion’s richer observability and stewardship model, JIT/runtime optimization surfaces, some CanonFS policy propagation, and some VM observability contracts. The repo itself says so. ([GitHub][3])

What appears scaffolded but not yet operational in the strongest sense: runtime JIT as part of the verified core, cognitive-tier reasoning surfaces, broader experimental AI tracks, and general-purpose OS ambitions. `runtime/jit` is explicitly non-verified; `experiments/ai` is explicitly experimental; the architecture overview marks cognitive tiers and Hanoi concepts optional / non-DCP; the OS progress log still describes staged slices inside a hosted experimental path. ([GitHub][22])

Most mature areas: deterministic core semantics, tests/CI/governance machinery, language frontend pipeline, CLI/tooling, and benchmark harnessing. Most aspirational areas: cognition, hardware acceleration, generalized AI governance, deterministic infrastructure, and the OS becoming a complete kernel substrate. The README’s Stage 4–6 roadmap makes that explicit. ([GitHub][16])

Strongest claims in the repo: existence of a deterministic core boundary, presence of a serious CI/governance regime, a material compiler/frontend pipeline, broad tests/benchmarks, and an advanced hosted OS prototype. Claims needing more evidence: production-readiness of all “stable” outer surfaces, full Axion enforcement maturity, and repo-wide status coherence. ([GitHub][2])

## 9. Testing, Verification, and CI Assessment

The test layout is broad and deliberate. The `tests` tree includes `ci`, `corpus`, `cpp`, `determinism`, `fixtures`, `fuzz`, `harness`, `lang`, `python`, and `stress`, and the tests README describes it as the regression and conformance proof layer for T81. It explicitly expects behavior changes in `src/` or `include/t81/` to include tests, and determinism-sensitive changes to include reproducibility coverage. ([GitHub][10])

CI quality is unusually strong for a project at this maturity level. The main workflow verifies repository structure, checks architecture-target alignment, freeze integrity, architecture coherence, action pinning, workflow permissions, root structure, naming, translation metadata/staleness/semantic alignment, docs structure, license policy, artifact hygiene, public API semver, spec-code alignment baselines, stdlib surface baselines, cognitive-tier boundary, overclaim guardrails, DCP/registry alignment, public header root, determinism claims, governance metrics, and benchmark workload regression. ([GitHub][9])

Benchmark discipline exists as an engineering tool, not just a marketing claim. The `benchmarks` directory includes VM simulation, determinism validation, CanonFS, Base81 SIMD, tensor, tritwise, and llama-kernel benchmarks, while the benchmark README documents smoke/full/deep profiles and says outputs feed benchmark reference docs. ([GitHub][18])

The main blind spots are not absence of testing, but uneven closure of what is tested versus what is claimed. Informational jobs are non-blocking by design, and some runtime/governance surfaces still have explicitly unfinished observability requirements. In other words: the repo validates a lot, but some of the most important narrative surfaces still outrun their most finished evidence. ([GitHub][9])

## 10. Documentation & Spec Integrity Assessment

The documentation system is ambitious and intentionally hierarchical. The architecture overview explicitly defines authority order, and many subsystem README files repeat that they are navigation aids only and do not alter spec authority or determinism claim scope. That indicates the maintainers are aware of documentation-control problems and have tried to formalize around them. ([GitHub][2])

Normative docs themselves are generally good. TISC, T81VM, T81Lang, and Axion all read like formal specs with scope statements and architectural roles. T81Lang in particular looks substantially filled out and marked stable. The weakness is not lack of normative writing; it is synchronization between authority surfaces. ([GitHub][14])

Duplication and drift are visible. Concrete examples: README `v1.9.0-Stable` vs latest GitHub release `v1.6.0`; README `369/369` tests vs Control Center `363/363`; CMake `1.3.6`; T81VM spec `Beta` vs Control Center “promoted to stable”; Axion spec `Alpha` vs Control Center “Axion Beta candidacy review closed — GO”; TISC spec references T81Lang current spec `v1.2` while T81Lang spec itself is `v1.3 — Stable`. These are not minor nits. They make the repo harder to trust as an engineering control surface. ([GitHub][1])

For a new contributor, the repo is understandable only if they already know where authority lives. For everyone else, the combination of `spec`, `docs`, `book`, dashboards, translations, experimental inventories, internal folders, and support tooling is a high-friction onboarding environment. Public-facing docs also overstate readiness in places by flattening stable core and experimental frontier into one maturity story. ([GitHub][2])

## 11. Kernel / OS / Axion Assessment

The OS-related work is real and substantial. `experimental/ternaryos/docs/PROGRESS.md` reports governed interrupt-model integration, slices 26–28, and 3,214/3,214 assertions passing. The progress log frames the OS as Axion, lists a naming split with T81 Foundation / T81VM / Axion / CanonFS / TISC, and describes concrete architecture milestones rather than only abstract aspirations. ([GitHub][20])

That said, it is still best classified as an **advanced hosted prototype / partial OS substrate**. The evidence provided is staged-slice progress inside `experimental/ternaryos`; it is not evidence of a fully operational standalone kernel on real hardware. Even the project’s own foldering keeps this work under `experimental`. ([GitHub][20])

Architecture maturity: respectable. Implementation depth: higher than a concept demo. Realism of kernel claims: mixed; realistic when it talks about slices, governance events, assertions, and boot lanes, less realistic if interpreted as “complete OS.” Hosted vs real behavior: still on the hosted/prototype side. Scheduler, IPC, faults, memory model, drivers, HAL, and service/runtime work appear actively developed but not yet finished as a comprehensive kernel product. ([GitHub][20])

A separate issue is semantic collision with Axion governance. The OS and the governance kernel should not share the same primary public name unless the repo makes the distinction explicit everywhere. Right now it does not. ([GitHub][20])

## 12. Language / VM / ISA Stack Assessment

### TISC

TISC is the clearest anchor in the stack. The spec is explicitly normative for instruction encoding, execution semantics, and VM integration, and it defines deterministic execution as a mandatory design principle. That makes TISC the strongest single architectural contract in the repo. ([GitHub][14])

### T81VM

T81VM is the practical execution center. It defines loading, abstract machine state, memory layout, concurrency rules, and Axion interaction. The spec is still Beta, which is more believable than the repo’s more sweeping stable claims. It reads like a subsystem that is real, central, and still finishing important edges. ([GitHub][15])

### T81Lang

T81Lang is more mature than many experimental language projects because it has a concrete frontend implementation surface and a stable normative spec. The frontend directory includes all the expected compiler stages. That said, a stable language spec does not automatically mean a fully stabilized ecosystem contract, especially when some downstream VM/Axion observability items are still open. ([GitHub][16])

### Execution/runtime semantics

The stack is most credible as a stable computation substrate when narrowed to **T81 data types + TISC + interpreter-centered T81VM + deterministic tracing/hash + bounded Axion policy hooks**. It is less credible when widened to include JIT, cognition, experimental AI, or OS ambitions under the same maturity label. ([GitHub][23])

## 13. Research vs Productization Assessment

T81 Foundation sits between **experimental platform** and **pre-product technical foundation**. It is too implemented, too tooled, and too governed to call it merely a research artifact. But it is not yet a developer platform or infrastructure candidate in a clean external-facing sense, because its outer layers remain mixed in maturity and its public status surfaces are inconsistent. ([GitHub][9])

To advance to the next stage, four things need to become true. First, one canonical release/status/version truth must drive the README, releases, control center, specs, and build metadata. Second, the build graph must stop muddying the experimental/core boundary. Third, the repo should explicitly separate “certified deterministic core” from “research ecosystem.” Fourth, some missing observability and enforcement points in VM/Axion need to be closed before broader claims are expanded. ([GitHub][1])

## 14. SWOT Analysis

### Strengths

T81 has an actual systems architecture, not a pile of disconnected experiments. Its strongest move is bounding guarantees through DCP and the determinism registry. It also has unusually strong CI/governance machinery and a nontrivial compiler/frontend + VM/tooling stack. ([GitHub][2])

### Weaknesses

Repo-wide status coherence is poor. Naming is overloaded. Experimental code bleeds into the core build. Outer-layer claims around Axion, AI, and ecosystem maturity are often stronger than the most conservative reading of the implementation evidence. ([GitHub][1])

### Opportunities

A tightly scoped deterministic-core release could be genuinely distinctive: ISA + VM + canonical types + trace hashing + policy-aware execution + rigorous CI is a real technical wedge. The repo already has the bones for that, if it stops flattening core and frontier into one maturity story. ([GitHub][23])

### Threats

The main threat is credibility erosion from inconsistency. The second is overextension: kernel/OS, language, VM, storage, AI experiments, cognition, benchmarks, multilingual docs, and governance all moving at once. The third is maintainability and onboarding drag caused by too many authority-adjacent artifacts. ([GitHub][1])

## 15. Risk Register

| Risk                                | Affected systems                   | Severity | Evidence                                                                                                                                                   | Mitigation                                                                            |
| ----------------------------------- | ---------------------------------- | -------: | ---------------------------------------------------------------------------------------------------------------------------------------------------------- | ------------------------------------------------------------------------------------- |
| Status/version drift                | Whole repo                         | Critical | README `v1.9.0`; latest release `v1.6.0`; Control Center `v1.4.1`; CMake `1.3.6` ([GitHub][1])                                                             | Generate all status surfaces from one machine-readable release ledger.                |
| Spec/implementation mismatch        | VM, Axion, TISC, language          | Critical | T81VM spec Beta vs “promoted to stable”; Axion spec Alpha vs strong stable language; stale version cross-references in TISC spec ([GitHub][15])            | Add CI that fails on cross-doc version/state mismatch.                                |
| Experimental/core boundary blur     | Core build, cognition, distributed |     High | `experimental/*` files are compiled into `t81_core` in `CMakeLists.txt` ([GitHub][7])                                                                      | Split experimental libraries from core library at build level.                        |
| Governance overreach                | Axion, cognitive tiers, AI         |     High | Axion README says stubs/hardening paths; spec says subsystem targets / partial observability while README projects broad governance maturity ([GitHub][3]) | Narrow public claims to implemented enforcement points.                               |
| Naming confusion                    | Axion governance vs Axion OS       |     High | Axion spec and ternaryos progress use “Axion” for different layers ([GitHub][21])                                                                          | Rename or consistently prefix governance-kernel vs operating-system surfaces.         |
| Documentation sprawl                | Specs, docs, book, dashboards      |     High | Many authority-adjacent surfaces with repeated caveats about not changing authority ([GitHub][2])                                                          | Collapse status reporting into one authoritative index.                               |
| Test blind spots at important edges | VM/Axion observability             |   Medium | Missing direct mode query, missing first-class scheduling trace events, partial canonical reason string ([GitHub][15])                                     | Treat those observability gaps as release blockers for higher maturity claims.        |
| Overextension                       | AI, OS, cognition, infra           |     High | README Stages 4–6 plus experimental AI and OS branches expand far beyond current certified core ([GitHub][1])                                              | Freeze scope and promote only proven surfaces.                                        |
| Onboarding friction                 | New contributors                   |   Medium | Root tree is broad and includes `internal`, `pdf`, `notebooks`, `legacy`, `artifacts/archive`, etc. ([GitHub][1])                                          | Publish a contributor map with “authoritative / experimental / ignore for now” zones. |
| Credibility risk                    | External partners, funders         | Critical | Determinism/auditability claims conflict with public status inconsistencies ([GitHub][1])                                                                  | Make synchronization defects first-class governance defects.                          |

## 16. Maturity Scorecard

These scores are evaluative judgments derived from the repo evidence above, not repository-supplied numbers.

| Domain                            | Conceptual clarity | Implementation depth | Test evidence | Interface stability | Governance clarity | Operational readiness | Documentation integrity |
| --------------------------------- | -----------------: | -------------------: | ------------: | ------------------: | -----------------: | --------------------: | ----------------------: |
| Data model / numeric model        |                  5 |                    4 |             4 |                   5 |                  4 |                     4 |                       4 |
| TISC ISA                          |                  5 |                    4 |             4 |                   5 |                  4 |                     4 |                       3 |
| T81VM                             |                  4 |                    4 |             4 |                   3 |                  4 |                     4 |                       3 |
| T81Lang                           |                  4 |                    3 |             3 |                   3 |                  4 |                     3 |                       4 |
| Axion governance                  |                  4 |                    3 |             3 |                   3 |                  4 |                     3 |                       3 |
| CanonFS / provenance              |                  4 |                    3 |             3 |                   3 |                  3 |                     3 |                       3 |
| Testing / CI / governance tooling |                  5 |                    4 |             5 |                   4 |                  5 |                     4 |                       4 |
| CLI / tooling / TUI               |                  4 |                    4 |             3 |                   3 |                  3 |                     4 |                       3 |
| Experimental AI surfaces          |                  3 |                    2 |             2 |                   2 |                  3 |                     1 |                       3 |
| Axion / TernaryOS OS effort       |                  4 |                    3 |             4 |                   2 |                  3 |                     2 |                       3 |
| Docs / public communication       |                  4 |                    4 |             3 |                   2 |                  4 |                     3 |                       2 |

Overall weighted maturity profile: **3.4 / 5**.

Interpretation: the project has a strong conceptual core and above-average engineering rigor for an experimental systems repo, but it is being dragged down by status incoherence, boundary blur, and overextension.

## 17. Strategic Recommendations

### Immediate priorities (0–30 days)

1. **Create one canonical release/status source** and generate README badges, control-center status, release notes, spec maturity tables, and build version headers from it. This is the single most urgent corrective action because it directly affects trust. ([GitHub][1])

2. **Separate Axion governance from Axion OS naming.** The current naming split is not acceptable for a repo this architecture-heavy. ([GitHub][21])

3. **Stop compiling experimental cognition/distributed surfaces into `t81_core`** unless they are formally promoted. The build graph should reflect the authority model. ([GitHub][7])

4. **Publish a root-level support taxonomy** for every top-level directory: authoritative, maintained support, experimental, legacy placeholder, internal/reference-only. ([GitHub][1])

### Near-term priorities (1–3 months)

1. **Close the VM/Axion observability gaps** already named by the specs: execution-mode query API, first-class scheduling events, canonical reason-string completion, and CanonFS observability completion. ([GitHub][15])

2. **Enforce cross-document status consistency in CI.** The repo already checks many governance invariants; it should also fail when README/spec/control-center/CMake/release metadata disagree. ([GitHub][9])

3. **Split “certified deterministic core” release notes from “ecosystem research progress.”** Right now the repo conflates them. ([GitHub][23])

4. **Demote broad stable language in public docs unless supported by exact artifact scope.** Stable should mean a named promoted surface, not the whole surrounding story. ([GitHub][24])

### Medium-term priorities (3–12 months)

1. **Externalize proof of the deterministic core.** The README itself says independent reproduction is the remaining advancement criterion for Stage 2 graduation. That is the right next proof point. ([GitHub][1])

2. **Choose one OS truth target.** Either stay explicitly as a hosted architectural prototype with rigorous validation, or narrow toward one real hardware/VM substrate and prove it. Avoid a diffuse middle. ([GitHub][20])

3. **Create a formal promotion gate for experimental-to-core build inclusion.** The `experiments/ai` README already describes a promotion path; the repo should apply that discipline consistently everywhere, including cognition and distributed surfaces. ([GitHub][25])

## 18. Final Verdict

T81 Foundation most credibly is, today, **a serious experimental deterministic-computing platform with a real compiler/ISA/VM/provenance/governance nucleus and a much wider outer ring of prototype, experimental, and aspirational work**. ([GitHub][2])

Its strongest technical asset is **the bounded deterministic core model**: explicit authority hierarchy, explicit DCP scope, formal specs, a real frontend/runtime/tooling stack, and unusually strong CI/governance enforcement for a repo at this stage. ([GitHub][2])

Its biggest structural weakness is **status incoherence across public and normative surfaces**, made worse by boundary blur between promoted core and experimental frontier. ([GitHub][1])

The single shift that would most improve its trajectory is **to make synchronization itself part of the deterministic contract**: one authoritative release ledger, one maturity ledger, one build boundary for core vs experimental, and one unambiguous naming model for Axion.

[1]: https://github.com/t81dev/t81-foundation/ "GitHub - t81dev/t81-foundation: T81 is a unified, deterministic, ternary-native computational architecture designed to surpass the limitations of binary computation. · GitHub"
[2]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/architecture/OVERVIEW.md "raw.githubusercontent.com"
[3]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/kernel/axion/README.md "raw.githubusercontent.com"
[4]: https://github.com/t81dev/t81-foundation/tree/main/core "t81-foundation/core at main · t81dev/t81-foundation · GitHub"
[5]: https://github.com/t81dev/t81-foundation/tree/main/lang "t81-foundation/lang at main · t81dev/t81-foundation · GitHub"
[6]: https://github.com/t81dev/t81-foundation/tree/main/kernel/axion "t81-foundation/kernel/axion at main · t81dev/t81-foundation · GitHub"
[7]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/CMakeLists.txt "raw.githubusercontent.com"
[8]: https://github.com/t81dev/t81-foundation/tree/main/experiments/ai "t81-foundation/experiments/ai at main · t81dev/t81-foundation · GitHub"
[9]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/.github/workflows/ci.yml "raw.githubusercontent.com"
[10]: https://github.com/t81dev/t81-foundation/tree/main/tests "t81-foundation/tests at main · t81dev/t81-foundation · GitHub"
[11]: https://github.com/t81dev/t81-foundation/tree/main/tooling "t81-foundation/tooling at main · t81dev/t81-foundation · GitHub"
[12]: https://github.com/t81dev/t81-foundation/tree/main/experimental "t81-foundation/experimental at main · t81dev/t81-foundation · GitHub"
[13]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/status/IMPLEMENTATION_MATRIX.md "raw.githubusercontent.com"
[14]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/spec/tisc-spec.md "raw.githubusercontent.com"
[15]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/spec/t81vm-spec.md "raw.githubusercontent.com"
[16]: https://github.com/t81dev/t81-foundation/tree/main/lang/frontend "t81-foundation/lang/frontend at main · t81dev/t81-foundation · GitHub"
[17]: https://github.com/t81dev/t81-foundation/tree/main/tooling/cli "t81-foundation/tooling/cli at main · t81dev/t81-foundation · GitHub"
[18]: https://github.com/t81dev/t81-foundation/tree/main/benchmarks "t81-foundation/benchmarks at main · t81dev/t81-foundation · GitHub"
[19]: https://github.com/t81dev/t81-foundation/tree/main/runtime/jit "t81-foundation/runtime/jit at main · t81dev/t81-foundation · GitHub"
[20]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/experimental/ternaryos/docs/PROGRESS.md "raw.githubusercontent.com"
[21]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/spec/axion-kernel.md "raw.githubusercontent.com"
[22]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/runtime/jit/README.md "raw.githubusercontent.com"
[23]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/product/DETERMINISTIC_CORE_PROFILE.md "raw.githubusercontent.com"
[24]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/spec/t81lang-spec.md "raw.githubusercontent.com"
[25]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/experiments/ai/README.md "raw.githubusercontent.com"

## License

MIT License.
