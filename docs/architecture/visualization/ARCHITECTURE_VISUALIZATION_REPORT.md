# T81 Internal Architecture Mapping & Visualization Data Foundation (`T81-ARCH-VIZ-002`)

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

### 2.4 Coverage Manifest & Evidence Hardening (T81-ARCH-VIZ-002)
- **Top-Level Directory Representation:** 18 of 35 repository top-level directories (51.43%) are mapped to architectural components in `coverage_manifest.json`. Unmapped directories (e.g. experimental benchmarks, customer tools, logs) are explicitly cataloged.
- **Relationship Derivation:** Every edge is categorized as either `STATICALLY_EXTRACTED` (source code includes, symbol references, CMake links) or `MANUALLY_CURATED` (subsystem containment, RFC intent, continuity evidence).
- **Maturity Audit:** Governance artifacts and continuity documents are classified under `GOVERNANCE_AND_ASSURANCE` rather than `PRODUCTION_CORE`.


### 2.7 Corrective Review Diff Composition (PR #749)
The final change set relative to baseline `e5c30560aa77c31184a14b03ed152a6deef5c1a3` exhibits a net line delta of +2,961 / −746 across 10 files:
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

## 6. How to Render the 3D Visualization in ChatGPT

To render this architecture in an interactive 3D browser environment (e.g. Three.js / Canvas):
1. Load `architecture_layers.json` to establish vertical Y planes (Layer 7 at $Y=0$ up to Layer 1 at $Y=600$).
2. Load `architecture_nodes.json` and `visualization_layout.json` to position subsystem cubes/spheres in 3D space.
3. Load `architecture_edges.json` to draw directional dependency curves between nodes, color-coded by relationship type (green for containment, cyan for invocation, purple for policy, orange for test coverage).
4. Load `execution_paths.json` to animate particle pulses traveling along verified call steps (Pathways 1 through 9).
