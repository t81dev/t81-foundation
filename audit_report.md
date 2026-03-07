# Architectural Coherence Audit of T81 Foundation

## Executive Summary

The T81 Foundation project presents a fascinating and rigorous approach to deterministic computing, using balanced ternary representation (Base-81) simulated over a binary substrate. This audit aimed to evaluate whether the repository achieves its goal of a fully integrated, deterministic ternary computing substrate end-to-end, spanning its language (T81Lang), instruction set (TISC), virtual machine (T81VM), runtime semantics, determinism enforcement, governance (Axion), and AI-native execution surfaces (RFC-0026).

Based on an exhaustive review of the codebase, specifications, status documentation, CI scripts, and experimental AI boundaries, the system is **mostly coherent but incomplete in its integration**. The core components—data types, ISA, and VM—are exceptionally well-aligned, rigorously tested, and demonstrably deterministic. Governance via the Axion kernel operates effectively within the execution path. However, the language layer (T81Lang) and the integration of experimental AI execution surfaces remain in varying states of maturity, with the AI functionality explicitly isolated from the core rather than seamlessly integrated as a unified whole.

## Architecture Overview

The T81 architecture is organized into a strictly layered model ("Layer Cake"):

1.  **T81Lang / Stdlib:** The high-level frontend that compiles to TISC bytecode.
2.  **TISC ISA:** The binary-encoded, ternary-semantic instruction set.
3.  **T81VM:** The reference interpreter for executing TISC.
4.  **Axion Policy Engine:** The governance layer invoked by the VM to evaluate execution constraints (e.g., `AXREAD`, `AXSET`, `AXVERIFY`).
5.  **CanonFS:** The deterministic persistence layer.
6.  **Experimental Tiers:** Including Cognitive Tiers, AI Inference (RFC-0026), and JIT compilation, which are explicitly marked as outside the Deterministic Core Profile (DCP).

This structure is enforced by a strict Dependency Firewall (`docs/architecture/DEPENDENCY_FIREWALL.md`), ensuring that core numeric types have no upward dependencies, and the VM depends only on the ISA and types.

## Layer-by-Layer Findings

### Balanced Ternary Representation (Data Types)

**Status: Frozen & Verified**
The system successfully represents balanced ternary values (-1, 0, +1) using a 2-bit packed representation (SWAR - SIMD Within A Register). This allows for efficient execution on binary hardware while maintaining strict ternary semantics.

*   **Evidence:** `include/t81/types/T81Int.hpp` implements `T81Int` with rigorous arithmetic operations built on ternary logic.
*   **Determinism:** Canonical serialization (`to_canonical_string`, `to_trit_string`) is consistently implemented. Recent audits (Feb 2026) fixed edge cases like `Cell` overflow undefined behavior (UB) and `T81Float` signed-zero canonicalization, ensuring bit-exact representation.

### Language Layer (T81Lang)

**Status: Beta (Implementation) / Draft (Spec)**
T81Lang aims to be a ternary-native language compiling down to TISC.

*   **Evidence:** The frontend accurately parses ternary primitives (including BigInt literals) and handles complex types like `Map` and `Set` via native VM handles (`MapHandle`, `SetHandle`).
*   **Gaps:** The compiler's bytecode emission is only "partially traceable," with full spec-section traceability still incomplete. While specific deterministic fixtures pass, general bit-exact compilation across all source variations is an open risk (`DRIFT_DECOMPOSITION.md` marks this as "Open"). Deeper container types (`T81Tree`, `T81Graph`) currently rely on string vector polyfills (`STRVECNEW`) during IR lowering, showing a gap between the type system and VM native support.

### Instruction Set (TISC)

**Status: Frozen & Verified**
TISC is genuinely ternary-aware in its semantics, though encoded using binary layouts (three 32-bit signed integer operands A, B, C).

*   **Evidence:** `include/t81/isa/opcodes.hpp` defines a robust set of operations, including ternary arithmetic primitives (`TNot`, `TAnd`, `TOr`, `TXor`) and branching logic.
*   **Determinism:** Opcode semantics are bit-exact and immutable under the v1.x profile. This is enforced by `check_tisc_freeze_integrity.py` in CI.

### Virtual Machine (T81VM)

**Status: Stable (Interpreter)**
The VM reference interpreter faithfully executes TISC instructions with deterministic runtime behavior.

*   **Evidence:** The non-JIT path is verified. It avoids platform-dependent drift by explicitly truncating towards zero for division and modulo, and strictly avoiding non-deterministic structures like `std::unordered_map`.
*   **Determinism:** Fault behavior uses explicit trap semantics rather than C++ UB.

### Governance and Policy Layer (Axion)

**Status: Stable (Bounded) / Alpha**
Axion is integrated deeply into the VM dispatch loop, evaluating syscall contexts before protected effects occur.

*   **Evidence:** `kernel/axion/policy_engine.cpp` implements `evaluate_internal`, returning verdicts (`Allow`, `Warn`, `Deny`). It correctly enforces policies like restricting model loading (`TLOADHASH`) based on approved tensor hashes.
*   **Gaps:** While the hooks exist, some evidence closures (M5-M7) are still pending Beta review. The governance limits are well documented but still maturing.

### AI-Native Execution Surfaces (RFC-0026)

**Status: Experimental (Governed non-DCP)**
The project includes AI-native opcodes (e.g., `ATTN`, `QMATMUL`, `EMBED`) and inference capabilities.

*   **Evidence:** These are defined in `include/t81/isa/opcodes.hpp` and integrated into the VM switch. A robust set of CI scripts (`check_ai_*.py`) validates benchmark matrices and policy expectations.
*   **Gaps:** Crucially, this layer is *not* cleanly integrated into the deterministic substrate. As per `STABLE_BASELINE.md` (established Mar 4, 2026), AI experimental work is explicitly sandboxed in `/experiments/ai/` to protect the core. It is an optional build (`-DT81_ENABLE_AI_EXPERIMENTS=ON`) and is excluded from DCP guarantees.

## Determinism Risk Analysis

The project demonstrates exceptional discipline in defining its determinism claims. The "Determinism Surface Registry" explicitly bounds claims to verified surfaces (e.g., TISC execution, tritwise ops, canonical encoding).

**Risks:**
1.  **T81Lang Compiler Emission:** Full bit-exact compilation from source to bytecode is incomplete.
2.  **Platform-Dependent Floating Point:** Host-native `cmath` functions are gated behind `#ifndef T81_DETERMINISTIC`, throwing errors in deterministic builds. This is a functional limitation rather than a drift, but it limits deterministic usability for advanced math.
3.  **JIT Equivalence:** JIT optimizations are excluded from DCP as they lack proven equivalence with the reference interpreter.

## Integration Gaps

The primary integration gap lies between the "Stable Baseline" (the deterministic core) and the AI/Cognitive Tiers (the experimental features). The project maintains a deliberate "firewall" between them. While this protects core determinism, it means the repository functions as two loosely coupled systems: a highly rigorous ternary VM and a sandboxed set of AI inference tools. They are not yet one unified substrate.

Furthermore, within the core, there is a gap between the language's high-level types (Trees, Graphs) and native VM opcodes, relying on workarounds (string vectors) during lowering.

## Governance Coverage

Governance is strong in documentation and intent, and increasingly strong in code.
*   **Enforcement:** Axion hooks are present in the VM.
*   **CI Validation:** Numerous `check_ai_*.py` scripts validate policy contracts, keyrings, and RFC readiness.
*   **Maturity:** Axion is still considered Alpha, indicating that while mechanisms exist, their comprehensive enforcement across all system states is not yet guaranteed for Beta/Production.

## CI Enforcement Coverage

The CI infrastructure is arguably the strongest part of the architecture's coherence.
*   **Determinism Gates:** `t81lang_repro_gate.py`, `t3k_repro_gate.py`, and `check_architecture_coherence.py` rigorously test reproducibility and architectural invariants.
*   **Regression Tests:** The suite boasts 100% test success (285/285 tests passing as of recent fixes).
*   CI enforces that governance rules and capability expectations match the code state.

## Evidence of Deterministic Behavior

The repository provides concrete, verifiable evidence that the core substrate behaves deterministically:
*   **Hash Ledgers:** Output from `tests/fixtures/t81lang_determinism/t81lang_ast_ir_repro_hash.txt` enforces reproducible builds.
*   **Instruction Tests:** Property testing in `tests/cpp/vm_determinism_property_test.cpp` explicitly proves bit-identity across architectures.
*   **Canonical Serialization:** Binary stability tests (`tisc_binary_io_determinism_test.cpp`) prove canonical output encodings (P2).

## System Maturity Assessment

*   **Data Types & ISA:** Frozen / Production-ready.
*   **VM (Interpreter):** Beta / Stable.
*   **Language (T81Lang):** Beta (Implementation) / Draft (Spec).
*   **Axion Governance:** Alpha.
*   **AI Integration:** Experimental / Sandboxed.

## Final Verdict

**B. Mostly coherent but incomplete integration**

**Justification:** The repository successfully implements a deterministic ternary computing substrate at the core levels (Data Types, ISA, VM). The commitment to canonical representation, explicit fault semantics, and CI-enforced determinism is exemplary. However, the system cannot be considered "fully coherent" end-to-end because:
1.  The language layer (T81Lang) compilation is not yet fully guaranteed to be bit-exact across all sources.
2.  High-level language constructs (Graphs/Trees) rely on suboptimal lowerings rather than native ternary VM handles.
3.  The flagship AI inference capabilities (RFC-0026) are explicitly quarantined in `/experiments/ai/` and excluded from the deterministic core guarantees. Until these are promoted through the governance gates and integrated natively without breaking determinism, the stack remains functionally bifurcated.

## Recommendations

To achieve a fully coherent deterministic ternary computing substrate, the following prioritized roadmap is recommended:

1.  **Close the T81Lang Compilation Drift (High Priority):** Expand the fixture corpus and spec-section traceability to guarantee bit-exact compilation from T81 source to TISC bytecode. This is the largest risk to the "end-to-end" determinism claim.
2.  **Native Container Opcode Lowering (Medium Priority):** Replace the `STRVECNEW` string-vector polyfills for `T81Tree` and `T81Graph` with native VM handles and dedicated opcodes, aligning them with the successful implementation of `T81Map` and `T81Set`.
3.  **Promote Axion to Beta (Medium Priority):** Complete the Axion evidence milestones (1.1, 1.3, 1.10) to promote the governance engine from Alpha to Beta, ensuring policy enforcement is as robust as the VM execution it guards.
4.  **Define the AI Integration Path (Long-Term):** Establish clear technical requirements for moving AI-native opcodes out of the `/experiments/ai/` sandbox into the Deterministic Core Profile. This requires proving that tensor operations and quantizations (`QMATMUL`, etc.) can execute with bit-identity across architectures without violating the VM's threat model.
5.  **Expand Soft-Float Coverage (Documentation/Minor):** Document alternative deterministic implementations for transcendental math functions that currently throw `std::domain_error` under `-DT81_DETERMINISTIC`.