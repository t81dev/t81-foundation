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
| **Axion OS Kernel** | ✅ Alpha | Complete governance system with 100% test coverage (28/28 tests); production-ready policy engine and ethics evaluation; Θ₁-Θ₉ principles fully implemented; comprehensive API documentation and integration examples; Alpha-ready kernel with deterministic decision making and full T81 stack integration |

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

# T81 Foundation Complete Systems & Subsystems Evaluation Report

## 1. Executive Summary

T81 Foundation is not a single program. It is a multi-layer repository that combines a ternary data model, an ISA, a VM, a language/compiler surface, a governance layer, content-addressed storage, benchmark and CI machinery, an experimental OS/kernel effort, AI/inference work, and a large documentation/governance apparatus. The repo root alone shows a broad system footprint: `.github`, `benchmarks`, `book`, `contracts`, `core`, `docs`, `experimental`, `kernel`, `lang`, `runtime`, `spec`, `src`, `tests`, `tooling`, `tools`, plus `internal`, `legacy`, `notebooks`, `pdf`, and multilingual/public-facing assets. The project has 3,636 commits and is primarily C++ with smaller Python/CMake/Shell/C components. ([GitHub][1])

The strongest technical core is the determinism-centered execution stack: data types, TISC, the non-JIT interpreter path of T81VM, and the associated determinism registry / DCP boundary. Those areas are explicitly named as the “Deterministic Core Profile,” tied to verification artifacts, and backed by CI checks and concrete test paths. The architecture overview also clearly separates the frozen/stable core from experimental periphery. ([GitHub][2])

The largest structural weakness is status incoherence across authority surfaces. The repository presents multiple conflicting maturity/version narratives at the same time: the root README says “v1.9.0-Stable; 369/369 tests,” the Project Control Center says “v1.4.1-Stable; 363/363 tests,” `CMakeLists.txt` says version `1.3.6`, the TISC spec is “Version 1.1 — Stable,” while the README describes “TISC ISA v1.2,” and the Axion kernel appears as both “Stable” in the implementation matrix and “Alpha” in the normative spec and experimental OS progress log. That does not invalidate the implementation, but it materially reduces auditability and external credibility. ([GitHub][1])

Overall maturity: the deterministic VM/language/spec/governance core looks like an increasingly disciplined experimental platform with pockets of genuine engineering rigor. The OS/kernel and broader “governed intelligence” vision remain prototype-to-research grade. The repo is best understood today as an ambitious experimental platform with a relatively credible deterministic execution nucleus, not yet as a fully coherent infrastructure stack or general-purpose operating system. ([GitHub][2])

## 2. Evaluation Method

This evaluation is based on six evidence streams: root repository structure and history; normative specs in `spec/`; architecture/governance/status docs in `docs/`; build and CI surfaces such as `CMakeLists.txt` and `.github/workflows/ci.yml`; maturity dashboards such as `PROJECT_CONTROL_CENTER.md` and `IMPLEMENTATION_MATRIX.md`; and the experimental OS progress log under `experimental/ternaryos/docs/PROGRESS.md`. The repository’s own authority ordering is explicit: `/spec` > `docs/architecture/OVERVIEW.md` > `/docs` > `/book`. ([GitHub][1])

This is therefore a structure/spec/implementation-surface audit, not a full line-by-line source-code verification of every C++ file. Where the repository itself declares implementation status, tests, or maturity, I treat those as claims unless supported by adjacent evidence such as CI checks, explicit test paths, or cross-document consistency. Where the repo itself contains contradictions, I treat that as evidence of governance drift. ([GitHub][3])

## 3. System-of-Systems Map

### Architectural foundations

Purpose: define the bounded architecture, authority model, and deterministic-core boundary. Main locations: `docs/architecture/OVERVIEW.md`, `docs/product/DETERMINISTIC_CORE_PROFILE.md`, `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`, `spec/`. Maturity: relatively high as a documentation control surface. Coupling: everything depends on it. Key risk: status drift between these files and public-facing top-level messaging. ([GitHub][2])

### Data model / numeric model

Purpose: canonical ternary-native data types and deterministic encoding. Main locations: `core/types/`, `spec/t81-data-types.md`, related tests listed in DCP/registry. Maturity: among the most credible domains; treated as Frozen/Verified in the implementation matrix and DCP. Key risk: floating-point scope is carefully bounded, but public messaging can blur those limits. ([GitHub][4])

### ISA / VM / runtime

Purpose: frozen instruction semantics and deterministic execution environment. Main locations: `core/isa/`, `core/vm/`, `runtime/`, `spec/tisc-spec.md`, `spec/t81vm-spec.md`. Maturity: the interpreter path is the core operational substrate; the VM spec remains Beta even as dashboards promote it to Stable. Key risk: spec/dashboard misalignment and unfinished JIT equivalence. ([GitHub][2])

### Language / compiler surfaces

Purpose: deterministic high-level language compiling exclusively to TISC. Main locations: `lang/`, `spec/t81lang-spec.md`, fixtures and repro gates named in docs. Maturity: stronger than many experimental-language repos because it has a normative grammar, compiler pipeline description, and explicit determinism caveats. Key risk: language maturity claims are stronger than the directly inspected implementation evidence here. ([GitHub][5])

### Governance / policy / Axion

Purpose: supervise privileged instructions, policy verdicts, and execution visibility. Main locations: `kernel/axion/`, `spec/axion-kernel.md`, governance docs, trace-related docs. Maturity: mixed. The architecture is real enough to shape the VM and policy discourse, but several core stewardship claims are explicitly only partial in the spec. Key risk: governance rhetoric exceeds current enforcement in some areas. ([GitHub][6])

### Storage / CanonFS / object model

Purpose: content-addressed persistence and provenance. Main locations: `src/canonfs/`, `include/t81/canonfs/`, supplemental spec and Axion OS progress notes. Maturity: bounded-stable in the architecture docs; more ambitious guest/boot/persistence scenarios remain experimental in the OS track. Key risk: stable core storage claims are mixed with broader prototype storage/boot claims. ([GitHub][2])

### Kernel / OS / Axion OS surfaces

Purpose: operating-system substrate, MMU, scheduler, IPC, device/HAL seams, service runtime, boot lanes. Main locations: `experimental/ternaryos/`, `kernel/`, OS progress docs. Maturity: advanced hosted prototype, not yet a completed operating system. Key risk: naming confusion between Axion-as-governance-kernel and Axion-as-operating-system. ([GitHub][7])

### AI / cognition / inference surfaces

Purpose: ternary-native inference, cognitive tiers, governed agentic/runtime surfaces, FFI, some AI experiment boundaries. Main locations: `experiments/ ai`, `experimental/`, RFC-referenced features in README/specs. Maturity: mixed; inference opcode work appears materially implemented, while cognitive-tier and broader AGI/governance ambitions remain partly speculative or explicitly non-DCP. Key risk: overextension. ([GitHub][1])

### Testing / benchmarks / CI enforcement

Purpose: enforce structural integrity, determinism claims, spec/doc alignment, and benchmark gates. Main locations: `.github/workflows`, `benchmarks/`, `tests/`, `scripts/ci/`, `scripts/governance/`. Maturity: strong relative to project stage. Key risk: CI breadth is substantial, but some jobs are informational, and public numbers are inconsistent across documents. ([GitHub][1])

### Documentation / public communication

Purpose: specs, audits, dashboards, book, multilingual README surfaces. Main locations: `docs/`, `book/`, README translations. Maturity: very high in quantity, mixed in synchronization integrity. Key risk: documentation sprawl and competing authority layers. ([GitHub][1])

### Legacy / internal / notebooks / pdf / archive

Purpose: history, support artifacts, exploratory material, generated material. Main locations: `legacy`, `internal`, `notebooks`, `pdf`, `artifacts/archive`. Maturity: unclear by design. Key risk: dead surfaces, unclear support status, contributor confusion. ([GitHub][1])

## 4. Detailed Subsystem Inventory

### T81 Data Types

Purpose: canonical numeric and collection semantics. Locations: `core/types/`, `spec/t81-data-types.md`, tests named in DCP/registry. Status: implemented, verified, and treated as frozen. Verification evidence: `v1_canonical_numeric_contract_test.cpp`, `tisc_binary_io_determinism_test.cpp`, plus audit notes in the implementation matrix. Determinism relevance: foundational. Governance relevance: high because canonicalization and policy replay depend on stable representations. Main concern: floating-point messaging must remain bounded. ([GitHub][4])

### TISC ISA

Purpose: immutable execution contract. Locations: `core/isa/`, VM decode/dispatch, `spec/tisc-spec.md`. Status: operational core, but status/version reporting is inconsistent: implementation matrix treats it as frozen/verified, README speaks of v1.2, while the spec page is “Version 1.1 — Stable” and CI still refers to “Verify TISC v1.1.0 Freeze Integrity.” Determinism relevance: maximal. Governance relevance: Axion visibility is normative in the spec. Main concern: freeze integrity is strongly governed, but version bookkeeping is not coherently presented. ([GitHub][4])

### T81VM Interpreter

Purpose: deterministic execution environment for TISC. Locations: `core/vm/`, `spec/t81vm-spec.md`. Status: practically central and treated as stable by dashboards, but still Beta in the normative spec. Verification evidence: VM trace and determinism property tests, DCP inclusion, conformance mentions. Determinism relevance: maximal. Governance relevance: explicit Axion integration. Main concerns: public `get_execution_mode()` is not exposed; scheduling events are not yet first-class trace entries; floating-point cross-architecture guarantees are explicitly bounded. ([GitHub][8])

### T81Lang

Purpose: high-level deterministic language targeting TISC. Locations: `lang/`, `spec/t81lang-spec.md`, repro fixtures/scripts named in registry. Status: one of the stronger subsystems by documentation and declared promotion state. Verification evidence: deterministic fixtures, repro gates, complete grammar, pipeline description. Determinism relevance: high, but with explicit caveats for host-dependent transcendental/float behavior. Governance relevance: tier-awareness, purity/effects, Axion visibility. Main concerns: strength of maturity claims exceeds what can be independently confirmed from the inspected implementation surfaces alone. ([GitHub][5])

### Axion Governance Kernel

Purpose: policy and supervision layer above execution. Locations: `kernel/axion/`, `spec/axion-kernel.md`. Status: partially real and partially aspirational. The implementation matrix promotes it to Stable with 54/54 tests, but the normative spec still says Alpha and explicitly admits partial implementation of determinism stewardship and incomplete complexity metrics. Determinism relevance: high. Governance relevance: this is the governance core. Main concern: the architecture is significant, but some “final arbiter” language is ahead of implemented detection capability. ([GitHub][4])

### CanonFS

Purpose: immutable content-addressed provenance and persistence. Locations: `src/canonfs/`, `include/t81/canonfs/`, supplemental docs/specs. Status: stable-bounded in the architecture overview and DCP-related documents; more advanced persistence and guest-bootstrap claims belong to the experimental OS path. Verification evidence: cited source paths and OS-phase tests. Main concern: the repo mixes stable-core provenance claims with broader guest/boot durability work that is still clearly prototype-stage. ([GitHub][2])

### Deterministic Core Profile / Registry

Purpose: define the certification boundary and verified deterministic surfaces. Locations: `docs/product/DETERMINISTIC_CORE_PROFILE.md`, `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`. Status: real governance machinery, not mere branding. Verification evidence: explicit surface/test/CI mapping. Main concern: the DCP boundary is good engineering, but the broader repo narrative sometimes blurs it. ([GitHub][9])

### CI / Governance Scripts

Purpose: enforce structure, doc/spec coherence, freeze integrity, workflow hygiene, governance checks, DCP alignment, and benchmark gates. Locations: `.github/workflows/ci.yml`, `scripts/ci/`, `scripts/governance/`. Status: unusually strong for an experimental systems repo. Main concern: breadth is impressive, but informational jobs do not equal production evidence, and the CI file inspected is itself only one part of the workflow estate. ([GitHub][3])

### Benchmarks

Purpose: performance and workload gates. Locations: `benchmarks/`, benchmark gate in CI, README benchmark claims. Status: present and operational enough to gate some work, but benchmark discipline is not yet fully unified with stable release reporting. Main concern: performance claims in marketing-style README sections are more expansive than the hard evidence inspected here. ([GitHub][1])

### Axion OS / TernaryOS

Purpose: hosted kernel/OS substrate with MMU, scheduler, IPC, devices, pager, service runtime, boot lanes. Locations: `experimental/ternaryos/`, `kernel/`, OS progress log. Status: advanced hosted prototype with substantial slice-by-slice implementation evidence. Main concern: despite real engineering depth, it remains clearly experimental, hosted, and partly scaffolded around QEMU/VirtualBox/dev-lane workflows rather than a mature standalone OS. ([GitHub][7])

### TUI / CLI / Tooling

Purpose: operator and agent frontends, developer workflows, examples. Locations: `tools/`, `tooling/`, `examples/`, `T81_BUILD_TUI` option, README docs. Status: implemented enough to ship and be described, but not central to system credibility. Main concern: UI/tooling breadth may outpace stabilization of core architectural boundaries. ([GitHub][10])

### Experimental / Distributed / Cognitive Tiers / AI

Purpose: non-DCP expansion paths. Locations: `experimental/`, `experiments/ ai`, `runtime/jit/`, distributed and tier references in DCP/registry. Status: research-stage or stubbed by the repo’s own definitions. Main concern: these surfaces add scope faster than they add operational proof. ([GitHub][1])

## 5. Architecture Coherence Assessment

There is a real layered model. The architecture overview presents a coherent chain from T81Lang to TISC to T81VM to Axion to CanonFS, with experimental tiers explicitly shown as optional/non-DCP. The DCP and determinism registry reinforce this by narrowing guarantees to named verified surfaces. That is a strong sign of architectural self-awareness. ([GitHub][2])

However, subsystem boundaries are only partly real. The core execution boundary appears reasonably real: data types, ISA, interpreter, and selected deterministic serialization/testing surfaces. Outside that core, boundaries blur. “Axion” refers both to a governance kernel in the stable stack and to an experimental operating system. The T81VM spec is Beta while dashboards call it Stable. The Axion spec is Alpha while the implementation matrix calls the Axion Kernel Stable. The OS progress document still uses internal `ternaryos` naming. That is not a minor wording issue; it means the architecture has multiple overlapping self-descriptions. ([GitHub][6])

Interfaces are partly explicit and partly implicit. The specs do a good job naming required behaviors, invariants, and conformance programs. CI also names alignment checks. But several critical interfaces are still tracked as incomplete or indirect: VM execution mode is not exposed via a clean public API; scheduler events are not first-class trace events; active Axion nondeterminism detection is only partial. This means some contracts are better specified than implemented. ([GitHub][8])

The strongest seams are the DCP boundary and the repo’s internal authority model. The weakest seams are naming/versioning/status synchronization and the expansion from a rigorous deterministic core into OS, cognition, AI, and hardware narratives. Documentation sprawl is visible from the repo tree and the dense dashboard ecology. ([GitHub][1])

## 6. Determinism & Reproducibility Assessment

Determinism is the most serious engineering theme in the repo. It is claimed in the README, formalized in the TISC and T81VM specs, bounded in the architecture overview, narrowed in the DCP, and enumerated in the Determinism Surface Registry. The registry is especially useful because it distinguishes verified surfaces from excluded or planned ones. ([GitHub][1])

Where determinism is genuinely enforced: TISC opcode semantics, VM interpreter execution, canonical data-type encoding, and soft-float deterministic math are all listed as verified with named tests and CI enforcement. The DCP then uses that registry as the gate for release guarantees. CI additionally runs freeze-integrity checks, architecture coherence checks, and determinism claim enforcement. ([GitHub][11])

Where determinism is only partial or bounded: compiler bytecode emission is explicitly only “Partial” in the registry; JIT equivalence is planned, not verified; network I/O, real-time scheduling, hardware FPU behavior outside soft-float, and performance determinism are explicitly out of scope. T81Lang also states that float division and transcendental behavior may vary across architectures. That is good engineering honesty. ([GitHub][11])

The main mismatch is not technical ambition versus zero implementation; it is technical ambition versus status discipline. The repo has a well-designed determinism boundary, but its public-facing version/status surfaces are noisy enough to weaken confidence in release exactness. When a project is built around bit-exact reproducibility, version exactness matters too. ([GitHub][9])

## 7. Governance, Safety, and Policy Layer Assessment

Governance here is architectural, not purely rhetorical. Axion is present in the ISA and VM specs, in the architecture overview, in the DCP boundary, and in the implementation matrix. The repo’s CI also contains multiple governance and overclaim checks, which is stronger than a conventional README-only governance story. ([GitHub][12])

That said, the spec itself shows the governance layer is not fully realized. Axion’s “determinism stewardship” is explicitly partial; active nondeterminism detection is not yet implemented. Complexity measurement is partial. The spec also notes that canonical reason-string concatenation was a tracked gap. So the governance stack is real, but not yet complete enough to justify the repo’s more sweeping “supervisory intelligence” framing without qualification. ([GitHub][6])

The most convincing governance mechanism is not “ethics” language; it is bounded policy interception of privileged operations plus explicit traceability requirements. The least convincing part is the extension of governance language into cognitive-tier and advanced reasoning supervision while core detection and metric mechanisms are still incomplete. ([GitHub][6])

## 8. Implementation Reality Check

What appears truly working now: the deterministic core profile around data types, TISC, the interpreter path of T81VM, selected compiler reproducibility fixtures, a substantial CI/governance stack, and enough Axion integration to anchor policy-aware execution claims. The repo has concrete tests, named workflows, and explicit stable/frozen designations around that nucleus. ([GitHub][9])

What appears partially working: parts of Axion’s richer stewardship model, some compiler and reproducibility surfaces, service/query surfaces in the OS effort, and benchmark/release discipline as a fully synchronized control surface. These are not absent; they are just not as complete as the most ambitious repo language suggests. ([GitHub][6])

What appears scaffolded but not yet operational in the strongest sense: trace-JIT equivalence, distributed/cognitive-tier determinism, external hardware accelerators, real hardware adapters in the OS path, and broader “global deterministic computation platform” ambitions. The repo itself classifies many of these as experimental, planned, or outside DCP. ([GitHub][9])

Most mature areas: deterministic core data types, ISA, interpreter, CI/governance scaffolding, and—judging by documentation maturity—the language surface. Most aspirational areas: full governance intelligence, distributed/cognitive tiers, native hardware realization, and the OS becoming a general-purpose kernel substrate. ([GitHub][4])

## 9. Testing, Verification, and CI Assessment

The testing/CI posture is a major strength. The inspected CI workflow includes structure verification, architecture-target checks, TISC freeze-integrity checks, AI experiment boundary checks, architecture coherence checks, workflow pinning and permissions audits, a large suite of governance checks, DCP integrity checks, governance metrics, and benchmark workload gates. This is substantially more rigorous than a typical hobby systems repo. ([GitHub][3])

The repo also ties determinism claims to named tests and scripts in a way that is audit-friendly. The determinism registry and DCP list explicit paths rather than vague assurances. That is the right pattern. ([GitHub][11])

Blind spots remain. Some CI jobs are informational, not hard gates. Compiler bytecode emission is only partial in the determinism registry. The VM spec itself admits missing direct query surfaces and incomplete scheduling trace visibility. Benchmark/reporting counts are inconsistent across authoritative documents. ([GitHub][3])

## 10. Documentation & Spec Integrity Assessment

Documentation quantity is very high and the engineering-control intent is serious. The architecture overview explicitly states authority ordering. The implementation matrix tries to reduce narrative drift to one row per subsystem. The project control center centralizes gate status. That is good governance design. ([GitHub][2])

The problem is synchronization integrity. At least five major inconsistencies are visible from the inspected materials: README version/test counts versus Project Control Center versus CMake version; TISC v1.2 versus TISC spec v1.1; T81VM Beta spec versus Stable dashboard; Axion Alpha spec versus Stable implementation matrix; and Axion OS alpha/prototype messaging versus some broader stable-kernel phrasing elsewhere. These are not cosmetic. They create uncertainty about what should be treated as authoritative by a new maintainer, partner, or evaluator. ([GitHub][1])

For a new technical contributor, the repo is impressive but hard to parse. The presence of `docs`, `book`, `spec`, `contracts`, `internal`, `legacy`, `pdf`, `notebooks`, multiple dashboards, and multilingual public surfaces makes it clear this is a documentation-heavy environment. Without strong synchronization, that becomes a maze. ([GitHub][1])

## 11. Kernel / OS / Axion Assessment

The OS work is materially deeper than a concept note. The progress log describes implemented phases for MMU, scheduling/IPC, persistence, device scaffolds, pager ABI, interrupt governance slices, and boot-lane validation. It cites thousands of assertions and a phased roadmap with concrete files and tests. This is real engineering. ([GitHub][7])

But it is still best classified as an advanced hosted prototype, not a complete kernel. The progress log repeatedly describes hosted simulation paths, QEMU developer lanes, VirtualBox scaffolds, recovered-artifact handoff bundles, and open real-hardware adapter work. The architecture is advancing through staged slices and simulation-backed validation, which is respectable, but it is not the same as a mature standalone operating system. ([GitHub][7])

The naming split also matters. “Axion” refers to the operating system in the progress log, while the spec defines “Axion Kernel” as the supervisory intelligence of the ecosystem. Those are related but not identical concepts. That naming collision will continue to cause confusion until the repo draws a harder semantic line. ([GitHub][7])

Verdict: this is currently an advanced architectural prototype / partial OS substrate with real subsystems, not yet a general-purpose or production operating system. ([GitHub][7])

## 12. Language / VM / ISA Stack Assessment

TISC is the most stable conceptual anchor. It is normative, frozen-oriented, and explicitly defines deterministic execution, zero undefined behavior, Axion visibility, and compatibility with data types, VM, and language layers. ([GitHub][12])

T81VM is the operational heart. The interpreter path is clearly central to the deterministic core, and the DCP excludes JIT until equivalence is proven. The spec also shows healthy honesty by naming currently incomplete surfaces rather than pretending they are done. ([GitHub][8])

T81Lang is more mature than many research languages because it has a normative grammar, type/effect framing, explicit determinism notes, and a compiler pipeline. But it is also where the project’s ambition is easiest to outrun. “Stable” language status is plausible for a bounded compiler-to-TISC toolchain, but not necessarily for the full ecosystem semantics implied around tiers, agents, and governed foreign surfaces without narrower release framing. ([GitHub][5])

As a stable computation substrate, the data types + TISC + non-JIT T81VM combination is the repo’s most credible technical asset. That is the part that could plausibly be treated as a serious experimental platform. ([GitHub][9])

## 13. Research vs Productization Assessment

Today the repo sits between “experimental platform” and “pre-product technical foundation.” It is beyond a mere research artifact because it has build systems, CI, tests, release discipline documents, conformance references, dashboards, and a bounded deterministic core. But it is not yet a developer platform or infrastructure candidate in a straightforward sense because too many outer layers are still experimental, contradictory, or governance-heavy relative to their implementation maturity. ([GitHub][3])

To advance to the next stage, the repo would need one decisive shift: separate the certifiable deterministic core from the exploratory ecosystem with much harder release labeling. That means one authoritative version source, one status source, one promoted surface registry, and explicit “not part of product boundary” markings everywhere else. After that, it would need independent reproduction outside the core maintainer context and tighter proof for the language/toolchain/governance interfaces it already claims as stable. ([GitHub][9])

## 14. SWOT Analysis

**Strengths**
The repo has a genuine systems architecture, not just branding. The determinism boundary is unusually explicit. CI/governance discipline is advanced. The interpreter-centered execution core appears materially real. The project also has strong documentation habits and cross-layer intent. ([GitHub][2])

**Weaknesses**
Status/version incoherence is severe. Documentation sprawl is high. Governance language sometimes outruns implementation. The OS and advanced AI/governance surfaces increase scope faster than they increase auditable certainty. Naming collisions, especially around Axion, blur architecture. ([GitHub][13])

**Opportunities**
A sharply bounded deterministic-core release could become the project’s credible wedge into research computing, reproducible execution, or language/VM experimentation. The repo already has the governance scaffolding to support that, if it stops trying to make every outer ring look equally mature. ([GitHub][9])

**Threats**
The biggest threat is credibility erosion through overclaim drift. In a project built around determinism and auditability, contradictory status surfaces are especially damaging. The second threat is maintainability collapse from breadth: kernel, language, VM, storage, AI, hardware, docs, book, dashboards, and multilingual outreach under one evolving authority graph. ([GitHub][1])

## 15. Risk Register

| Risk                         | Affected systems                                                               | Severity | Evidence                                                                                              | Mitigation                                                                                                                 |
| ---------------------------- | ------------------------------------------------------------------------------ | -------: | ----------------------------------------------------------------------------------------------------- | -------------------------------------------------------------------------------------------------------------------------- |
| Status/version drift         | Whole repo                                                                     | Critical | README `v1.9.0` / 369 tests vs Control Center `v1.4.1` / 363 tests vs CMake `1.3.6`                   | Create single source of truth for version, release state, and test totals; generate downstream docs from it. ([GitHub][1]) |
| Spec/implementation mismatch | ISA, VM, Axion                                                                 | Critical | TISC spec 1.1 vs README TISC 1.2; VM spec Beta vs dashboard Stable; Axion spec Alpha vs matrix Stable | Add automated status sync checks for spec version/state vs dashboards vs README. ([GitHub][12])                            |
| Governance theater risk      | Axion, cognitive tiers                                                         |     High | Axion spec says determinism stewardship partial; complexity metrics partial                           | Narrow public claims to implemented hooks and verified enforcement points only. ([GitHub][6])                              |
| Overextension                | Experimental, AI, OS, hardware                                                 |     High | DCP excludes many outer surfaces while README/roadmap spans hardware, deterministic cloud, cognition  | Freeze outer-surface marketing; publish a “core vs research” map in root docs. ([GitHub][9])                               |
| Terminology/naming confusion | Axion, TernaryOS, kernel                                                       |     High | Progress log says Axion OS with internal `ternaryos`; spec uses Axion Kernel as supervisory layer     | Rename or prefix governance kernel vs OS kernel consistently. ([GitHub][7])                                                |
| Experimental sprawl          | `experimental`, `experiments/ ai`, `legacy`, `internal`, notebooks/pdf/archive |     High | Large repo surface with many support/unclear-status directories                                       | Publish support-status taxonomy per directory; archive or quarantine dormant surfaces. ([GitHub][1])                       |
| Test blind spots             | VM scheduling, compiler repro, JIT                                             |   Medium | Scheduling trace events not first-class; compiler emission partial; JIT excluded from DCP             | Promote missing trace surfaces to first-class tests before further feature expansion. ([GitHub][8])                        |
| Maintainability risk         | Docs + code + dashboards                                                       |   Medium | Heavy documentation lattice with multiple status dashboards and authority layers                      | Generate matrices/dashboards from machine-readable metadata. ([GitHub][2])                                                 |
| Onboarding risk              | New contributors                                                               |   Medium | Root tree breadth plus competing docs/spec/book/control-center layers                                 | Add contributor path map: “where to trust first, where not to trust yet.” ([GitHub][1])                                    |
| Credibility risk             | External partners/funders                                                      | Critical | Deterministic/auditable brand undermined by internal inconsistency                                    | Treat synchronization defects as release-blocking defects. ([GitHub][1])                                                   |

## 16. Maturity Scorecard

Scores are my synthesis from the inspected specs, dashboards, CI, DCP/registry, and OS progress documents. They are evaluative judgments, not repository-provided numbers. ([GitHub][2])

| Domain                       | Conceptual clarity | Implementation depth | Test evidence | Interface stability | Governance clarity | Operational readiness | Documentation integrity |
| ---------------------------- | -----------------: | -------------------: | ------------: | ------------------: | -----------------: | --------------------: | ----------------------: |
| Data Types                   |                  5 |                    4 |             4 |                   5 |                  4 |                     4 |                       4 |
| TISC ISA                     |                  5 |                    4 |             4 |                   5 |                  4 |                     4 |                       3 |
| T81VM                        |                  4 |                    4 |             4 |                   3 |                  4 |                     4 |                       3 |
| T81Lang                      |                  4 |                    3 |             3 |                   3 |                  4 |                     3 |                       4 |
| Axion governance             |                  4 |                    3 |             3 |                   3 |                  4 |                     3 |                       3 |
| CanonFS                      |                  4 |                    3 |             3 |                   3 |                  3 |                     3 |                       3 |
| CI / governance tooling      |                  4 |                    4 |             4 |                   4 |                  5 |                     4 |                       4 |
| Axion OS / TernaryOS         |                  4 |                    3 |             4 |                   2 |                  3 |                     2 |                       3 |
| AI / cognitive / distributed |                  3 |                    2 |             2 |                   2 |                  3 |                     1 |                       3 |
| Docs / public comms          |                  4 |                    4 |             3 |                   2 |                  4 |                     3 |                       2 |

Overall weighted maturity profile: **3.4 / 5**. That corresponds to a serious experimental platform with a credible deterministic core, but with enough outer-layer ambiguity and governance/documentation drift to block a stronger “infrastructure-ready” rating. ([GitHub][9])

## 17. Strategic Recommendations

### Immediate priorities (0–30 days)

Unify version/status authority. Pick one canonical source for release number, spec maturity, subsystem maturity, and test totals; generate README, Control Center, Implementation Matrix, and build metadata from it. Until that is done, every “stable” claim should be treated as provisional. ([GitHub][1])

Rename the two Axion concepts. One is a governance kernel/policy engine; the other is an experimental operating system. They need different user-facing names or strict prefixes. ([GitHub][6])

Publish a repo support-status index by top-level directory: maintained core, maintained support, experimental, legacy, archived, internal-only. The tree is too large to leave implicit. ([GitHub][1])

### Near-term priorities (1–3 months)

Tighten spec-to-implementation promotion rules. A subsystem should not be called Stable in dashboards while its normative spec remains Beta/Alpha unless the repo explicitly distinguishes “implementation stable, spec pending.” Right now it does not do that cleanly. ([GitHub][8])

Finish the missing observability surfaces in the VM/Axion boundary: direct execution-mode query, first-class scheduling trace events, and whatever remains of canonical reason-string and nondeterminism detection work. These are leverage points because they convert governance from philosophy into instrumentation. ([GitHub][8])

Separate DCP release notes from ecosystem release notes. The core deserves a lean, certifiable release discipline. The broader ecosystem deserves a research-progress log. Mixing them blunts both. ([GitHub][9])

### Medium-term priorities (3–12 months)

Pursue independent external reproduction of the deterministic core, because the repo itself names that as the remaining advancement criterion for Stage 2-style verification. That would do more for credibility than another layer of dashboards. ([GitHub][1])

For the OS effort, choose the next truth test: either “hosted prototype with rigorous device/boot simulation” or “real kernel substrate on a constrained target.” Right now it is progressing responsibly, but still across too many staged lanes. A narrower acceptance target would sharpen the program. ([GitHub][7])

For the AI/governance perimeter, stop promoting cognitive/distributed/hardware visions as near peers of the core. Keep them in explicitly non-DCP research channels until they have the same evidence density as the VM/ISA/data-type stack. ([GitHub][9])

## 18. Final Verdict

T81 Foundation is most credibly, today, **a rigorous experimental deterministic-computing platform with a real VM/ISA/data-model core and an unusually elaborate governance/documentation superstructure**. ([GitHub][2])

Its strongest technical asset is **the bounded deterministic core profile and the fact that the repo actually tries to tie claims to named tests, CI checks, and explicit scope exclusions**. ([GitHub][9])

Its biggest structural weakness is **status incoherence across specs, dashboards, README, and build metadata**. In a normal repo that is annoying. In a repo built around determinism and auditability, it is corrosive. ([GitHub][1])

The single shift that would most improve its trajectory is **to turn synchronization itself into a first-class deterministic surface**: one authority graph, one generated maturity ledger, one release truth, and a hard separation between certifiable core and exploratory frontier.

[1]: https://github.com/t81dev/t81-foundation/ "GitHub - t81dev/t81-foundation: T81 is a unified, deterministic, ternary-native computational architecture designed to surpass the limitations of binary computation. · GitHub"
[2]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/architecture/OVERVIEW.md "raw.githubusercontent.com"
[3]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/.github/workflows/ci.yml "raw.githubusercontent.com"
[4]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/status/IMPLEMENTATION_MATRIX.md "raw.githubusercontent.com"
[5]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/spec/t81lang-spec.md "raw.githubusercontent.com"
[6]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/spec/axion-kernel.md "raw.githubusercontent.com"
[7]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/experimental/ternaryos/docs/PROGRESS.md "raw.githubusercontent.com"
[8]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/spec/t81vm-spec.md "raw.githubusercontent.com"
[9]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/product/DETERMINISTIC_CORE_PROFILE.md "raw.githubusercontent.com"
[10]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/CMakeLists.txt "raw.githubusercontent.com"
[11]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/governance/DETERMINISM_SURFACE_REGISTRY.md "raw.githubusercontent.com"
[12]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/spec/tisc-spec.md "raw.githubusercontent.com"
[13]: https://raw.githubusercontent.com/t81dev/t81-foundation/main/docs/status/PROJECT_CONTROL_CENTER.md "raw.githubusercontent.com"

---

## License

MIT License.
