# Architectural Coherence Audit: T81 Deterministic Ternary Computing Substrate

**Date:** 2026-03-06
**Auditor:** Senior Systems Architect / Deterministic Computing Auditor
**Target Repository:** T81 Foundation (https://github.com/t81dev/t81-foundation)
**Objective:** Determine whether the repository implements a coherent deterministic ternary computing substrate end-to-end.

---

## 1. Executive Summary

This audit assesses the architectural coherence of the T81 Foundation stack, focusing on its ability to provide a unified deterministic ternary computing substrate. The repository demonstrates an exceptionally rigorous approach to deterministic execution, canonical representation, and execution governance.

Unlike many systems that retrofit determinism, T81 explicitly engineers its entire stack—from custom primitive data types (`T81BigInt`, `T81Float`) to a ternary-native Instruction Set (TISC), a deterministic Virtual Machine (T81VM), and a policy enforcement engine (Axion)—to ensure bit-exact reproducibility across platforms.

Based on exhaustive review of the specification suite (`/spec`), the runtime implementation (`core/`, `src/`), the governance artifacts (`kernel/axion/`), and the continuous integration determinism gates (`scripts/ci/`), the T81 system demonstrates a highly coherent and deeply integrated deterministic substrate.

## 2. Architecture Overview

The T81 architecture is organized into a strict dependency hierarchy (the "Layer Cake"), governed by a normative Dependency Firewall (`docs/architecture/DEPENDENCY_FIREWALL.md`):

1.  **Data Representation Layer (`core/types/`)**: Base-81 / balanced ternary primitives.
2.  **Instruction Set Architecture (TISC) (`core/isa/`)**: Ternary-aware opcodes.
3.  **Virtual Machine (T81VM) (`core/vm/`)**: Interpreter that dispatches TISC instructions.
4.  **Governance Kernel (Axion) (`kernel/axion/`)**: Execution policy and ethics engine.
5.  **Persistence (CanonFS) (`src/canonfs/`)**: Deterministic storage.
6.  **Language Frontend (T81Lang) (`lang/frontend/`)**: Compiler targeting TISC.

This layered design ensures that higher-level abstractions (like the compiler or Axion policy engine) rely strictly on the deterministic guarantees provided by the underlying core layers. The architecture explicitly bridges ternary semantics with binary host hardware using SIMD Within A Register (SWAR) and 2-bit packed trits, acknowledging the reality of modern hardware while maintaining strict ternary invariants.

## 3. Layer-by-Layer Findings

### 3.1. Balanced Ternary Representation
*   **Implementation:** The system defines custom primitive types, most notably `T81BigInt` (arbitrary-precision base-81 integer), `T81Float` (balanced ternary floating-point), and `Cell` (for raw trit sequences).
*   **Canonical Encoding:** The `T81Float` type (`include/t81/types/T81Float.hpp`) enforces a canonical string representation (e.g., `+0E0`, `-0E0`) and strictly defines biased exponents and mantissas.
*   **Finding:** The representation layer is remarkably consistent. The 2-bit packing (`include/t81/types/packing.hpp`) efficiently maps ternary values to binary hardware without leaking non-deterministic behavior into the observable semantic layer. The data types layer is marked as "Frozen" and fully verified.

### 3.2. Language Layer (T81Lang)
*   **Implementation:** T81Lang provides a high-level syntax that compiles down to TISC bytecode.
*   **Determinism:** The compiler features a bit-exact reproducibility gate (`scripts/ci/t81lang_repro_gate.py`) which ensures that multiple passes over the same source produce identical `.tisc` binaries.
*   **Finding:** The semantic analyzer correctly handles native ternary types. Recent updates (as noted in `DRIFT_DECOMPOSITION.md`) resolved earlier parser/semantic issues, achieving 100% test coverage. T81Lang is functionally coherent with the underlying TISC, though its status is still Beta.

### 3.3. Instruction Set (TISC)
*   **Implementation:** TISC (`include/t81/isa/opcodes.hpp`) defines over 100 opcodes. Arithmetic instructions (`FADD`, `TAND`, `TOR`) act natively on the ternary types.
*   **Fault Semantics:** The ISA strictly eschews undefined behavior. Opcodes that could cause faults (e.g., bounds violation, type mismatch) trigger deterministic traps rather than relying on host OS behavior.
*   **Finding:** The ISA is ternary-aware and completely stable (Status: Frozen). It effectively isolates the execution model from the host architecture.

### 3.4. Virtual Machine (T81VM)
*   **Implementation:** The T81VM is the execution engine for TISC bytecode. It maintains the canonical state of registers and memory.
*   **Governance Bridge:** The VM is tightly integrated with Axion. Opcode dispatch, particularly for privileged operations (`AXREAD`, `AXSET`, `AXVERIFY`), delegates to the policy engine before modifying state.
*   **Finding:** The VM faithfully executes the deterministic contract of TISC. The non-JIT interpreter path is verified as part of the Deterministic Core Profile (DCP).

### 3.5. AI-Native Execution Surfaces (RFC-0026)
*   **Implementation:** TISC defines opcodes for AI workloads (`ATTN`, `QMATMUL`, `EMBED`). The CI gate `check_architecture_coherence.py` specifically verifies that these opcodes are present in both the ISA definition and the VM dispatch loop.
*   **Integration:** The system includes a deterministic quantization spec (`spec/t3k-quantization-spec.md`) and a corresponding CI reproducibility gate (`scripts/ci/t3k_repro_gate.py`).
*   **Finding:** The AI surface is integrated into the deterministic core, extending the canonical representation to complex tensor operations.

## 4. Determinism Risk Analysis
*   **Floating-Point Drift:** The spec acknowledges that complex transcendentals (e.g., `sin`, `cos` in `T81Float.hpp`) may fall back to host math if the deterministic soft-math backend (`dmath`) is disabled. The project uses a macro (`T81_DETERMINISTIC`) to explicitly control this.
*   **JIT Execution:** The JIT compilation path is explicitly marked as out-of-scope for current determinism guarantees (Non-DCP). This is correctly bounded and documented.
*   **Hash Maps:** The `T81Map` (`include/t81/types/T81Map.hpp`) uses `std::hash` for non-symbol keys. While this introduces non-deterministic iteration order, the `serialize_canonical()` method forces key-sorting, ensuring that the *observable* serialization remains deterministic.

## 5. Integration Gaps
*   **T81Lang — Compiler Bytecode Traceability:** While the compiler emits deterministic output for tested fixtures, the comprehensive traceability between the specification language and the deterministic compilation profile is currently marked as "Partial" (tracked via `DRIFT_DECOMPOSITION.md`).
*   **Axion — Tier Transition:** Full cognitive-tier promotion orchestration governed by Axion is currently deferred, as experimental cognitive tiers remain outside the core governance framework.

## 6. Governance Coverage
*   **Implementation:** Axion (`kernel/axion/policy_engine.cpp`) evaluates execution traces against defined policies. It monitors instruction counts, recursion depth, stack limits, and ethical constraints (e.g., Theta-1 to Theta-9).
*   **Finding:** Axion is not merely a monitoring tool; it is an active gatekeeper within the VM's step loop. If an instruction violates policy (e.g., `TLOADHASH` failing against the `allowed_tensor_hashes` list), Axion forces a deterministic fault. The governance coverage is rigorously enforced, tracking explicit reasons (e.g., `AI_POLICY_ALLOW_WLOAD_POLICY_GATE`).

## 7. CI Enforcement Coverage
The repository features an extraordinary suite of CI scripts under `scripts/ci/`:
*   `t81lang_repro_gate.py`: Ensures compiler determinism.
*   `check_architecture_coherence.py`: Ensures ISA and VM synchronization, Axion logging compliance, and dependency rules.
*   Extensive python scripts (`check_ai_wload_policy_evidence.py`, `check_ai_inference_capability_matrix.py`) monitor Axion evidence.
The CI gates actively protect the architectural invariants, moving them from conceptual goals to enforced contracts.

## 8. Evidence of Deterministic Behavior
The repository actively proves determinism using verifiable artifacts:
*   Extensive C++ test suites (`tests/determinism/` containing `test_float.cpp`, `test_vector.cpp`, etc.) validate data type boundaries, arithmetic, and strict execution.
*   `tests/cpp/vm_determinism_property_test.cpp` enforces deterministic execution guarantees on the VM level.
*   The `t81lang_repro_gate.py` asserts bit-exact output reproduction over multiple compilations.
These verifications provide strong evidence that identical inputs to the system reliably yield bit-exact results across architectures.

## 9. System Maturity Assessment
*   **Frozen/Verified:** Data Types, TISC ISA.
*   **Beta/Stable:** T81VM (Interpreter), T81Lang.
*   **Alpha:** Axion Kernel.
*   **Experimental:** Cognitive Tiers, JIT, Distributed Execution.

The project clearly delineates its maturity levels (`docs/status/IMPLEMENTATION_MATRIX.md`), accurately reflecting what is guaranteed and what is experimental.

## 10. Final Verdict

**A. Fully coherent deterministic ternary computing substrate**

**Justification:** The T81 Foundation repository is not a conceptual mockup; it is a functioning, technically rigorous implementation of a deterministic architecture. The integration between the language, ISA, VM, and governance layers is deep and functional. Critical determinism risks (such as floating-point behavior and hash map iteration) are structurally mitigated or explicitly gated. The extensive CI verification ensures that the deterministic core remains bit-exact and immune to architectural drift. The separation of the Frozen Core from Experimental features proves a mature architectural management strategy.

## 11. Recommendations

To maintain and expand this coherent substrate, the following actions are recommended:

1.  **Fully Promote Axion to Beta:** Axion is currently Alpha but is a critical dependency for the VM's security model. Prioritize closing the AX-M5 to M7 evidence loops.
2.  **Transcendental Math Hardening:** Complete the deterministic software math (`dmath`) implementations for all remaining trigonometric functions in `T81Float` to completely sever the reliance on host hardware math for the deterministic profile.
3.  **T81Lang Specification:** Advance the T81Lang specification from Draft to Beta to match the implementation maturity.
4.  **JIT Trace Equivalence:** As JIT execution matures, formalize the proof of semantic equivalence between the JIT and the reference interpreter to eventually bring the JIT into the Deterministic Core Profile.