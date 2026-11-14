# T81 Internal Architecture Mapping & Visualization Data Foundation (`T81-ARCH-VIZ-002`)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Internal Architecture Mapping & Visualization Data Foundation (`T81-ARCH-VIZ-002`)](#t81-internal-architecture-mapping-&-visualization-data-foundation-`t81-arch-viz-002`)
  - [1. Executive Summary](#1-executive-summary)
  - [2. Pinned Baseline & Census Reconciliation](#2-pinned-baseline-&-census-reconciliation)
    - [2.1 Repository Census](#21-repository-census)
    - [2.2 CMake Target Reconciliation](#22-cmake-target-reconciliation)
    - [2.6 Semantic Definition of Production and Qualification Classifications](#26-semantic-definition-of-production-and-qualification-classifications)
    - [2.5 Scoped Qualification Boundaries](#25-scoped-qualification-boundaries)
    - [2.4 Structural Source Reference Typing & Evidence Hardening (T81-ARCH-VIZ-002)](#24-structural-source-reference-typing-&-evidence-hardening-t81-arch-viz-002)
    - [2.7 Corrective Review Diff Composition (PR #749)](#27-corrective-review-diff-composition-pr-#749)
    - [2.3 Test Suite Reconciliation](#23-test-suite-reconciliation)
  - [3. Seven-Layer Architecture Model](#3-seven-layer-architecture-model)
  - [4. Nine Traced Execution Pathways](#4-nine-traced-execution-pathways)
  - [5. Architectural Integration Opportunities](#5-architectural-integration-opportunities)
  - [6. Interactive Architecture & Research Status Dashboard (`T81-ARCH-VIZ-003`)](#6-interactive-architecture-&-research-status-dashboard-`t81-arch-viz-003`)

<!-- T81-TOC:END -->


**Document ID:** `T81-ARCH-VIZ-002-REPORT-R1`
**Baseline Anchor Commit:** `4f96cb24358bfd260e67387b2c4db2797b01236f`
**Classification:** Source-grounded internal architecture model, dependency graph, and 3D visualization data foundation
**Status:** Complete / Verified under Pinned Baseline

---

## 1. Executive Summary

This report establishes a comprehensive, machine-readable, and source-grounded architectural representation of the T81 Foundation platform. The investigation maps tracked source code, C++ includes, CMake targets, instruction dispatch paths, runtime dataflows, and research continuity receipts into an evidence-based knowledge graph.

The generated dataset resides under `docs/architecture/visualization/` and is designed to allow ChatGPT or interactive browser applications (e.g., Three.js / WebGL) to render a 3D architecture model of T81 without speculation.

Every element in the graph differentiates:
- What T81 **currently implements and tests** (e.g., TISC v1.1.0 interpreter, Axion policy kernel, CanonFS portable storage, SCG-07 qualified balanced ternary arithmetic).
- What has been **experimentally demonstrated under qualification** (e.g., CPA-004 freestanding AArch64 bare-metal execution, RTL ternary dot-product feasibility).
- What remains an **unimplemented architectural proposal** (e.g., DAIOS RFC-00D2 autonomous decision substrate, native physical ternary silicon).

---

## 2. Pinned Baseline & Census Reconciliation

### 2.1 Repository Census
- **Pinned Commit:** `4f96cb24358bfd260e67387b2c4db2797b01236f` on `origin/main`.
- **Tracked Files:** 4,341 files across 39 top-level directories.
- **Languages & Formats:** C++ (.cpp/.hpp: 1,198 files), Markdown (.md: 1,084 files), Python (.py: 229 files), T81Lang (.t81: 215 files), JSON (.json: 320 files), Shell (.sh: 151 files), Assembly (.s/.S: 28 files), Verilog (.v: 4 files).

### 2.2 CMake Target Reconciliation
A repository-wide audit revealed 500 executable declarations and 54 library declarations across root and subdirectory `CMakeLists.txt` files:
- **Executable Targets:** 500 declarations (498 unique target names). Of these, 388 are standalone C++ test executables registered for CTest.
- **Library Targets:** 54 declarations across root and `src/` modular libraries (`t81_core`, `t81_vm`, `t81_axion`, `t81_isa`, `t81_lang_frontend`, `t81_ternaryos_hal`, etc.).
- **CTest Invocations:** 170 `add_test()` declarations mapping test targets to CI check gates.




### 2.6 Semantic Definition of Production and Qualification Classifications
To prevent conflating software presence with external deployment certification:
- **`PRODUCTION_CORE`:** Identifies an implemented, fully compiled, and test-covered architectural subsystem integrated into the primary platform source tree (e.g. `t81_core`, `t81_vm`, `t81_lang_frontend`). It does **not** signify commercial, enterprise, or independently certified production readiness.
- **`QUALIFIED` / `QUALIFIED_WITH_LIMITATIONS`:** Strictly reserved for components with authoritative research receipts and mathematical qualification reports (specifically SCG-07 Phase 6 for balanced ternary arithmetic types, and CPA-004 for freestanding QEMU runtime). Components lacking such continuity receipts remain `UNVERIFIED` for formal qualification regardless of implementation maturity.

### 2.5 Scoped Qualification Boundaries
In strict accordance with T81 research receipts:
- **SCG-07 Phase 6:** Formally qualifies balanced ternary arithmetic primitives under limitations (`T81Limb27/54`, `T81Int`, `T81BigInt`, `T81Fraction`). Unrelated subsystems (compiler frontend, CLI driver, VM interpreter, Axion governance kernel, CanonFS storage, tensors, SIMD routines) lack continuity qualification receipts and are strictly classified as `UNVERIFIED` for formal qualification.
- **CPA-004 Evaluator:** Qualifies the bounded freestanding execution profile under limitations (351-byte program, 17 opcodes on AArch64 QEMU).

### 2.4 Structural Source Reference Typing & Evidence Hardening (T81-ARCH-VIZ-002)
To reconcile source references against the pinned codebase without turning unverified claims into established implementation facts:
- **Reference Taxonomy:** Every line-level source reference is categorized into one of six explicit reference types:
  - `FUNCTION_DEFINITION`: Entry point or body of a C/C++ function.
  - `CALL_SITE`: Specific line where a function or method invocation occurs.
  - `DISPATCH_SITE`: Control flow branching construct (e.g. `switch (insn.opcode)` at `vm/vm.cpp:1612`) that dispatches execution based on opcode or event, distinct from a function call or definition.
  - `DECLARATION`: Struct, class, typedef, or function prototype declaration.
  - `COMMENT`: Non-executable explanatory comments or docstrings.
  - `DESCRIPTIVE_FILE`: Whole-file descriptive anchors without single executable line semantics.
- **Top-Level Directory Representation:** 18 of 35 repository top-level directories (51.43%) are mapped to architectural components in `coverage_manifest.json`. Unmapped directories (e.g. experimental benchmarks, customer tools, logs) are explicitly cataloged.
- **Relationship Derivation:** Every edge is categorized as either `STATICALLY_EXTRACTED` (source code includes, symbol references, CMake links) or `MANUALLY_CURATED` (subsystem containment, RFC intent, continuity evidence).
- **Maturity Audit:** Governance artifacts and continuity documents are classified under `GOVERNANCE_AND_ASSURANCE` rather than `PRODUCTION_CORE`.


### 2.7 Corrective Review Diff Composition (PR #749)
The final change set relative to baseline `e5c30560aa77c31184a14b03ed152a6deef5c1a3` exhibits a net line delta of +2,995 / −746 across 10 files:
1. **Generator Refactoring (`generate_architecture_visualization.py`, +2,333 / −744):** Transitioned dataset generation from monolithic/single-line representations to clean, maintainable multiline dictionaries and incorporated the new `coverage_manifest.json` builder. Zero architectural coverage was removed or lost.
2. **Dataset Evidence Enrichment (`architecture_nodes.json`, `architecture_edges.json`, `execution_paths.json`, `subsystem_summary.json`):** Enriched reference metadata with `reference_type` and line bounds, added structured `maturity_evidence` citations, categorized edge `derivation_method`, and scoped execution path observation steps.
3. **Coverage Manifest Introduction (`coverage_manifest.json`, +76 lines):** Added authoritative manifest mapping 18 represented and 17 unmapped top-level directories and 9 relationship categories.
4. **Validation Hardening (`validate_architecture_visualization.py`, +222 lines):** Replaced assertion checks with unconditional `ValueError` / `FileNotFoundError` exceptions for `python -O` safety, closed trace validation bypasses, and added semantic compatibility and anti-tautology enforcement.
5. **Negative Test Suite Expansion (`test_architecture_visualization.py`, +180 lines):** Added 12 fail-closed negative regression tests covering out-of-bounds line numbers, nonexistent files/nodes, trace hash mismatches, tautological evidence, and optimized-mode execution.

### 2.3 Test Suite Reconciliation
- **Test Source Files:** 419 C++ test files located under `tests/cpp/`.
- **Harness Architecture:** 410 of the 419 files implement standalone `int main()` test drivers using custom `require()` / `QUAL_REQUIRE()` assertions active unconditionally under both Debug and Release (`-DNDEBUG`) builds, rather than external GTest runner frameworks (which account for 74 test cases).

---

## 3. Seven-Layer Architecture Model

The repository is reconstructed into 7 logical architectural layers:

```
+-------------------------------------------------------------------------+
| Layer 1: Language and Development                                       |
| (T81Lang Lexer, Parser, AST, Semantic Analyzer, IRGenerator, CLI Driver)|
+-------------------------------------------------------------------------+
                                    | (Emits TISC IR / Invokes VM)
+-------------------------------------------------------------------------+
| Layer 2: Instruction and Execution                                      |
| (TISC ISA Contract, T81VM Interpreter Loop, System Registers R75..R80)  |
+-------------------------------------------------------------------------+
         | (Arithmetic Calls)                       | (Policy Checks)
+----------------------------------------+ +------------------------------+
| Layer 3: Numerical & Computational     | | Layer 4: System Services     |
| (T81Limb27/54, T81Int, BigInt,         | | (Axion Governance Kernel,    |
|  T81Fraction, Soft Math, Tensors)      | |  CanonFS Portable Storage)   |
+----------------------------------------+ +------------------------------+
                                    | (Hardware Mapping / OS Substrate)
+-------------------------------------------------------------------------+
| Layer 5: Operating-System Architecture                                  |
| (TernaryOS Hal, MMU, Cooperative Scheduler, DAIOS RFC-00D2 Direction)   |
+-------------------------------------------------------------------------+
                                    | (Silicon / Emulation Platforms)
+-------------------------------------------------------------------------+
| Layer 6: Hardware and Acceleration                                      |
| (Verilog RTL Feasibility, Host AVX2 SIMD Primitives, QEMU Drivers)      |
+-------------------------------------------------------------------------+
                                    | (Verification & Governance Gates)
+-------------------------------------------------------------------------+
| Layer 7: Research and Assurance                                         |
| (Hypotheses H01–H08, SCG Continuity Receipts, CI Determinism Gates)     |
+-------------------------------------------------------------------------+
```

---

## 4. Nine Traced Execution Pathways

| ID | Pathway Name | Status | Method | Entry Point | Exit Point |
|---|---|---|---|---|---|
| `path.01` | T81Lang Source to Executable | `STATICALLY_TRACED` | Static Source Trace | `main.cpp:10799` | `ir_generator.cpp:8` |
| `path.02` | TISC Decode & VM Execution | `STATICALLY_TRACED` | Static Source Trace (Trace-Supported) | `vm.cpp:435` | `runtime_state_helpers.cpp:80` |
| `path.03` | Balanced Ternary Limb Mul | `STATICALLY_TRACED` | Static Source Trace | `T81Limb.hpp:173` | `T81Limb.hpp:624` |
| `path.04` | Multi-Precision BigInt Invariant | `STATICALLY_TRACED` | Static Source Trace | `T81BigInt.hpp:1005` | `T81BigInt.hpp:1018` |
| `path.05` | Safetensors Weight Import/Export | `STATICALLY_TRACED` | Static Source Trace | `main.cpp:3884` | `main.cpp:4400` |
| `path.06` | Axion Policy & Ethics Gate Check | `STATICALLY_TRACED` | Static Source Trace | `policy_engine.cpp:217` | `policy_engine.cpp:242` |
| `path.07` | CanonFS Object Storage | `STATICALLY_TRACED` | Static Source Trace | `persistent_driver.cpp:193`| `persistent_driver.cpp:200` |
| `path.08` | Freestanding AArch64 Boot | `STATICALLY_TRACED` | Static Source Trace | `qemu_slice6_bare_kernel.c:101`| `qemu_slice6_cpp_bridge.cpp:1823` |
| `path.09` | Hardware RTL Simulation | `STATICALLY_TRACED` | Static Source Trace | `tb_dot_product.v:1` | `design_c_ht1_ternary.v:55` |

*Note on Tracing Provenance:* In accordance with T81-ARCH-VIZ-002, all 9 execution pathways and 33 steps are classified as `STATICALLY_TRACED` / `STATICALLY_SUPPORTED`. The canonical VM trace `tests/fixtures/canonical_traces/01_bigint_add.trace` (SHA-256 `a00b67b1d76124d4a247a54a737d369dfc3e8326ca5f0d5a99fc0adbd93efbf0`) explicitly proves the emitted instruction stepping PC/opcode sequence, while C++ source-line execution (`vm.cpp:435`, `vm.cpp:1612`) and policy/state transitions are statically supported without direct binary instrumentation.

---

## 5. Architectural Integration Opportunities

Based on source-grounded dependency tracing, the following non-normative integration opportunities are identified for future research (no implementation is authorized):

1. **Direct T81Lang to Freestanding AArch64 Pipeline**:
   - *Current State:* T81Lang programs lower to TISC bytecode, which executes hosted in `t81vm`. Separately, TernaryOS runs an admitted subset of TISC instructions bare-metal on QEMU AArch64 (`qemu_slice6_bare_kernel.c`).
   - *Opportunity:* Establish a verified direct compilation pathway from T81Lang source to the freestanding ELF image payload admitted by the UEFI bootloader.

2. **Unified MLIR Dialect for Ternary Linear Algebra**:
   - *Current State:* `lang/mlir/t81_dialect.cpp` defines basic dialect operations, while `include/t81/tensor/matmul.hpp` implements CPU ternary matmuls independently.
   - *Opportunity:* Lower high-level tensor operations through MLIR transformations to target AVX2 SIMD kernels and external hardware backends uniformly.

3. **CanonFS Persistent Backing for TernaryOS Guest**:
   - *Current State:* TernaryOS implements an in-memory block store (`ternaryos/dev/canon_store.cpp`), while CanonFS `PersistentDriver` operates on the host filesystem (`fs/persistent_driver.cpp`).
   - *Opportunity:* Integrate CanonFS portable storage key indexing (`h1-<64-hex>.blk`) with the VirtIO block device driver (`virtio_blk_mmio.cpp`).

---

## 6. Interactive Architecture & Research Status Dashboard (`T81-ARCH-VIZ-003`)

In task `T81-ARCH-VIZ-003`, the architectural dataset was upgraded into an interactive, browser-based dashboard application located at `docs/architecture/visualization/dashboard/` (with entry point `docs/architecture/visualization/index.html`):

- **Interactive 3D Perspective:** Provides an orbit/pan/zoom canvas view rendering all 35 components and their dependency edges across vertical layer elevation planes using coordinates from `visualization_layout.json`.
- **Accessible 2D Structural Fallback:** Provides an accessible layer-by-layer card grid with keyboard focus management and semantic equivalence, ensuring complete functionality even on headless or non-WebGL systems.
- **Independent Status Overlays:** Separates evidence classifications (`VERIFIED`, `STATICALLY_SUPPORTED`, `UNVERIFIED`, `NOT_MAPPED`), implementation maturity (`IMPLEMENTED`, `EXPERIMENTAL`, `PROPOSED`), research qualification (`QUALIFIED`, `QUALIFIED_WITH_LIMITATIONS`, `BLOCKED`, `NOT EVALUATED`), and engineering pull-request activity.
- **Research Programs Integration:** Directly embeds evidence-backed status for SCG-01..08, TernaryOS ARCH-ENG-002 Phases 1..3, TISC v1.1.0/v2.0, CanonFS, Hardware/RTL, and H01–H08 hypothesis register mappings (`research_programs.json`).
- **PR Activity Snapshot:** Catalogs open and merged PRs with affected subsystem mappings and freshness timestamps (`pull_requests_snapshot.json`).
- **Automated Validation:** Enforced via `scripts/ci/validate_architecture_visualization.py` and `tests/ci/test_architecture_dashboard.py` (including Playwright interaction testing).

See [`DASHBOARD_GUIDE.md`](DASHBOARD_GUIDE.md) for full architecture details, local HTTP server launch instructions, snapshot refresh procedures, and strict governance boundaries.
