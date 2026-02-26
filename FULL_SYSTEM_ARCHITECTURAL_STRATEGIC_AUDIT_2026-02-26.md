# Full-System Architectural & Strategic Audit

**Date:** 2026-02-26
**Target:** T81 Foundation Repository
**Status:** Forensic Audit Complete

---

## Executive Summary

The T81 ecosystem presents as a rigorously designed, determinism-first computing stack. The core architecture—rooted in the TISC ISA and canonical ternary data types—is implemented, verified, and contractually frozen.

**Crucially, the system is not vaporware.** The Verified Determinism Surface is real: the VM executes TISC bytecode with bit-exact reproducibility across architectures, supported by a heavy CI regimen (`repro-ledger.yml`, `t3k_repro_gate.py`).

However, a sharp boundary exists between the **Frozen Core** (Foundation Layer) and the **Experimental Periphery** (Cognitive Tiers, Hanoi Kernel). The core is a hardened, production-grade research platform. The higher-level cognitive layers are currently structural stubs or partial implementations, designed to reserve architectural space rather than deliver immediate functionality.

**Strategic Verdict:** T81 is a **Production-Viable Core** wrapped in an **Advanced Research Framework**. It is ready for low-level deterministic workloads but not yet for high-level agentic cognition without significant bespoke development.

---

## 1. Architectural Integrity

### Drift Matrix (Spec vs Implementation)

| Component | Spec Status | Implementation Status | Alignment | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **Data Types** | Frozen (v1.1) | Implemented (`core/types/`) | **High** | Canonical `to_string` delegates correctly; `T81Float` matches spec. |
| **TISC ISA** | Frozen (v1.1) | Implemented (`core/isa/`, `core/vm/`) | **High** | Opcodes in `opcodes.hpp` match `tisc-spec.md`. |
| **T81VM** | Stable (v1.1) | Implemented (`core/vm/`) | **High** | Trace logic and Axion hooks present and active. |
| **Axion** | Stable (v1.0) | Partial (`src/axion/`) | **Moderate** | Engine and Policy structures exist; enforcement relies on VM hooks. |
| **Hanoi Kernel** | Historical/Exp | Stubbed (`experimental/hanoi/`) | **Low** | `in_memory_kernel.cpp` exists but is a skeletal reference. |
| **Cognitive Tiers** | Experimental | Structural (`experimental/tiers/`) | **Low** | Definitions exist; logic is largely placeholder. |

### Layer Violation Analysis
*   **Hygiene:** Excellent. No circular dependencies found between `core` and `experimental`.
*   **Legacy Artifacts:** References to `src/t81_core.h` persist in legacy docs and some build scripts, though the file itself is removed from critical paths.
*   **Experimental Boundary:** `experimental/` namespace is strictly respected. The VM accesses cognitive tiers only via defined opcodes (e.g., `SymLoad`, `SymRewrite`), maintaining isolation.

**Architectural Risk Score:** **2/10** (Low risk; structural integrity is high).

---

## 2. Determinism Validation

### Evidence of Determinism
*   **Bit-Exactness:** Validated via `repro-ledger.yml` running `t3k_repro_gate.py` and `t81lang_repro_gate.py`.
*   **Floating Point:** `T81Float` implementation explicitly bridges to host `double` for transcendentals (`sin`, `cos`), acknowledging the spec's allowance for bounded non-determinism in these specific ops. Basic arithmetic uses soft-float or strict canonical forms.
*   **Traceability:** VM emits canonical `AxionEvent` logs (e.g., `reason="kMemStore"`) ensuring execution path verification.

### Threat Map
*   **Host FPU Leakage:** The reliance on host `cmath` for transcendentals in `T81Float` is a known, documented surface. It is not a bug but a design choice (`v1` profile).
*   **JIT:** The JIT compiler is present but flagged experimental. Its equivalence to the interpreter is a critical future verification surface.

**Determinism Confidence:** **Strong** (within the defined "Deterministic Core Profile").

---

## 3. Instruction Set Coherence (TISC)

*   **Opcode Completeness:** The `opcodes.hpp` enumeration maps 1:1 with the verified spec.
*   **Extension Pressure:** Opcodes for higher-level functions (`TNeuralFwd`, `Gossip`, `InfSeed`) are present in the ISA but often stubbed or blocked in the VM, indicating a "reservation" strategy for future expansion without breaking binary compatibility.
*   **Maturity:** **Freeze-Ready**. The ISA is stable and serves as a reliable target for the compiler.

**Recommended Action:** Formally deprecate or implement the "blocked" opcodes to clear the "unimplemented" debt in the VM switch statement.

---

## 4. VM & Execution Engine

*   **Interpreter:** Robust `switch`-based dispatch in `Interpreter::step()`.
*   **Safety:** Memory access is strictly guarded (`mem_ok`, `log_bounds_fault`). Stack/Heap collision detection is active.
*   **Axion Integration:** `eval_axion_call` is pervasive, intercepting execution before sensitive ops (`AxRead`, `AxSet`, `Call`).
*   **Performance:** `hot_spots_` detection and `JitCompiler` hooks are present, suggesting a hybrid runtime model is in active development.

**Runtime Integrity Score:** **9/10**.
**Production Readiness:** **High** (for the interpreter profile).

---

## 5. Axion Governance & Enforcement

*   **Mechanism:** Axion is not a separate process but a library linked into the VM (`axion_engine_`).
*   **Policy:** `parse_policy` loads rules text. The `DenyWithReasonEngine` provides a safe default-deny fallback.
*   **Trace:** The "reason" strings in `vm.cpp` (`kStackAlloc`, `kHeapCompaction`) are hardcoded and canonical, enabling precise audit trails.
*   **Gaps:** High-level semantic policies (e.g., "no unethical recursion") rely on the Cognitive Tier stubs, which are currently just scaffolding.

**Governance Strength:** **Moderate** (Mechanism is strong; Policy content is nascent).

---

## 6. Documentation vs Reality

*   **README:** Accurately claims "Frozen Specs" and "Bit-exact reproducibility."
*   **Book:** Aspirations in `/book` regarding "Hanoi Kernel" and "Cognitive Tiers" run ahead of the code. These are design documents, not manuals for current features.
*   **Legacy:** Some documentation still references `include/t81/core` which has been restructured.

**Documentation Credibility:** **High** for Core/Spec; **Aspirational** for Tiers/Kernel.

---

## 7. Code Quality & Engineering Discipline

*   **Toolchain:** CMake + Ninja + Clang/GCC setup is standard and robust.
*   **CI Rigor:** `repro-ledger.yml` is a gold-standard artifact for a determinism project. It proves the team "eats their own dogfood."
*   **Style:** `clang-format` enforced. Codebase is clean, modern C++20/23.
*   **Testing:** Extensive unit tests for types (`test_T81Float_arithmetic.cpp`) and VM trace behavior.

**Engineering Maturity:** **Hardened System** (for the Core).

---

## 8. Strategic Position Assessment

**Classification:** **Production-Viable Core**

**Why?**
The T81 Foundation has successfully built a "physics engine for logic." The TISC ISA, Data Types, and VM are not prototypes; they are verified, frozen artifacts capable of executing arbitrary deterministic logic. The "Experimental" layers (Hanoi, Cognitive Tiers) are clearly demarcated extensions, not load-bearing pillars of the current stability.

**Legacy Note:**
If development stopped today, T81 would be remembered as **"The most rigorous attempt to standardize ternary computing and deterministic execution since the Soviet Setun project."** It provides a complete, usable toolchain for anyone wishing to explore post-binary computing.

---

## 9. Hard Truth Section

### 5 Most Serious Structural Risks
1.  **Transcendental Drift:** `T81Float` relying on host `cmath` for `sin/cos` weakens the "universal determinism" claim for advanced math workloads.
2.  **Stubbed Cognition:** The "Cognitive Tiers" are largely interface definitions. Users expecting ready-made AGI guardrails will be disappointed.
3.  **JIT Complexity:** The `JitCompiler` exists but adds a massive surface area for potential non-determinism bugs if not formally verified against the interpreter.
4.  **Legacy Documentation:** Drift in `t81_core.h` references confuses the onboarding path.
5.  **Adoption Barrier:** The stack is insular. Interop with binary systems (besides simple IO) is minimal.

### 5 Most Valuable Strengths
1.  **Verified Determinism:** The `repro-ledger` is a demonstrable, high-value asset that few projects possess.
2.  **Frozen ISA:** TISC v1.1 provides a stable target for compiler writers, guaranteeing longevity.
3.  **Axion Hooks:** The pervasive governance interception in the VM is architecturally sound and security-positive.
4.  **Canonical Data Types:** The `T81Int` / `T81Fraction` implementations are rigorous and correct.
5.  **Clean Architecture:** Strict separation of Core vs. Experimental prevents "infectious" instability.

### Single Most Important Next Move
**Stabilize the Standard Library.** The core is solid. To make it usable, the focus must shift from "kernel internals" to "user-facing standard library" (T81Lang stdlib) to enable real application development without reinventing the wheel.

---
*End of Audit Report*
