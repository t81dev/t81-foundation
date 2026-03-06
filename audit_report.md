# Full Architectural Coherence Audit: T81 Foundation

## 1. Executive Summary

The T81 Foundation stack presents an exceptionally ambitious and rigorous attempt to build a **fully deterministic, ternary-native computing substrate** designed specifically for high-assurance AI governance. The architecture is deeply layered, enforcing strict determinism boundaries through a formal "Frozen" core (TISC ISA, core data types), governed by the Axion policy engine, and verified through a stringent CI "Repro Gate" pipeline.

While the conceptual design is robust and the implementation of the core execution loop (T81VM) and instruction set (TISC) is highly mature, the system currently exhibits characteristics of a **Mostly coherent but incomplete integration**. The primary disconnects lie at the boundaries between the language frontend (T81Lang), the experimental cognitive tiers, and the AI-native execution surfaces (RFC-0026). The determinism invariants hold strongly for scalar types and basic VM execution, but complex container types (`T81Map`, `T81Graph`) and experimental AI inference lanes (e.g., hardware-accelerated attention or quantization) still rely on binary fallbacks or display incomplete integration with the Axion governance layer.

Despite these gaps, the project demonstrates an unparalleled commitment to verifiable determinism, backed by comprehensive audits, cross-architecture bit-identity CI gates, and opcode-level policy enforcement.
## 2. Architecture Overview

The T81 architecture is organized into strict layers of authority and abstraction, mapping directly to maturity classifications and determinism guarantees:

*   **Foundation Layer (Frozen):**
    *   **TISC ISA**: The Ternary Instruction Set Computer, representing the immutable instruction set (e.g., `Add`, `Mul`, `TExp`, `TMatMul`, `ATTN`).
    *   **Ternary Data Types**: The core arithmetic primitives (`trit`, `tryte`, `T81Float`, `T81BigInt`), engineered for bit-exact reproducibility and bounded soft-float precision.
*   **Execution Layer (Beta/Stable):**
    *   **T81VM Interpreter**: A deterministic execution engine that dispatches TISC bytecode sequentially. All operations trap explicitly (e.g., `Trap::SecurityFault`, `Trap::TypeFault`) instead of yielding undefined behavior.
    *   **Trace-JIT (Experimental)**: An opt-in JIT compilation path that compiles frequent execution traces, though its equivalence to the interpreter is not formally verified in the Determinism Registry.
*   **Governance Layer (Stable - Bounded):**
    *   **Axion Policy Engine**: An integrated kernel that intercepts every VM instruction (`eval_axion_call`) to evaluate resource bounds (e.g., stack limits, tensor elements, branching entropy) and emit Allow/Warn/Deny verdicts.
*   **Application Layer (Beta):**
    *   **T81Lang**: The frontend compiler and standard library that lowers high-level language constructs into TISC bytecode.
*   **Persistence Layer (Stable - Bounded):**
    *   **CanonFS**: An integrity-controlled file system abstraction for deterministic data storage and tensor loading.
*   **Experimental AI/Cognitive Tiers:**
    *   RFC-0026 inference opcodes (e.g., `ATTN`, `QMATMUL`, `EMBED`) and Cognitive Tiers (e.g., distributed consensus via `Gossip`, `TickSync`) currently act as non-DCP (Deterministic Core Profile) surfaces with partial integration.

The integration model relies on the compiler emitting TISC bytecode, which the VM executes while continuously querying the Axion engine for permission to proceed. This creates a tightly coupled, policy-governed execution environment.

## 3. Layer-by-Layer Findings

### 3.1 Data Representation Layer
The system consistently defines and utilizes ternary arithmetic primitives (e.g., `Trit::Neg`, `Trit::Zero`, `Trit::Pos`). Balanced ternary (-1, 0, 1) mapping is deeply embedded in the execution engine (`clamp_trit` logic, `TNot`, `TAnd`, `TOr`, `TXor`). However, many memory structures (e.g., `T81Vector`, `T81Map`) rely on standard C++ vectors or maps internally, mapping binary representations (like C++ integers, doubles, standard strings) to logical ternary values. For example, `std::vector<std::string>` is used for maps and sets instead of native ternary representations. This indicates a reliance on binary fallbacks for complex data types.

### 3.2 Language Layer (T81Lang)
T81Lang successfully maps primitive types, numerical arithmetic, and comparisons to TISC opcodes. The frontend employs a robust type checker (`SemanticAnalyzer`) that enforces specific generic types (`Map`, `Set`, `Tree`, `Graph`). However, the implementation of these complex types is inconsistent. For instance, `T81Map` relies on `std::hash` for non-symbol keys, which inherently breaks cross-platform determinism invariants. The serialization of `T81Map` requires sorting to enforce a deterministic canonical format (`serialize_canonical()`). The language also features experimental keywords (`recurse`, `train`, `infer`) not yet finalized in the core spec, demonstrating a Beta implementation running ahead of its draft specification.

### 3.3 Instruction Set (TISC)
The TISC ISA is a frozen, well-defined specification containing a comprehensive suite of ternary-aware instructions, bounded soft-float operations (e.g., `FAdd`, `FSin`), and specific meta-reflection ops (`MetaRead`, `MetaWrite`). Recent expansions under RFC-0026 include AI-native inference opcodes (`ATTN`, `QMATMUL`, `EMBED`) directly within the ISA. The semantic rules are strict, and memory bounds checking is enforced by mapping operands to opcodes deterministically.

### 3.4 Virtual Machine (T81VM)
The T81VM execution loop acts as a sequential interpreter. The memory model restricts access to specific segments (`Code`, `Stack`, `Heap`, `Tensor`, `Meta`). Deterministic runtime behavior is strictly enforced—for example, division by zero cleanly traps (`Trap::DivisionFault`), and out-of-bounds stack/heap access triggers explicit `Trap::StackFault` or `Trap::BoundsFault`. However, there are experimental components, such as `JitCompiler` and `DeterminismDetector`, that have indeterminate equivalence with the reference interpreter, operating outside the Deterministic Core Profile. The interpreter evaluates the Axion engine before every instruction, which is architecturally coherent but heavily penalizes performance.

### 3.5 Governance and Policy Layer (Axion)
Axion serves as the bedrock of T81's security guarantees. The `PolicyEngine` enforces limits on recursive calls, instructions, stack frames, shape complexity, branching entropy, and tensor allocation (`MaxTensorsExceeded`). The `eval_axion_call` function intercepts state transitions, issuing Allow, Warn, or Deny verdicts. Crucially, the system checks WLOAD (weights loading) evidence against explicit contract reason codes (`AI_POLICY_ALLOW_WLOAD_POLICY_GATE`, `AI_POLICY_DENY_WLOAD_UNSUPPORTED`), proving that the policy layer is not merely documented but actively enforced in the VM step loop.

### 3.6 AI-Native Execution Surfaces (RFC-0026)
The integration of AI into the core ISA is sophisticated. The VM dispatches `ATTN`, `QMATMUL`, and `EMBED` instructions by unpacking packed operand encodings and invoking native tensor math checks (`tensor_attention_checked`, `tensor_matmul_checked`). This layer explicitly integrates with Axion governance for hardware execution permissions (`TNeuralFwd` blocking). However, this tier is experimental and relies on external capabilities (e.g., `llama.cpp` integration), presenting a significant determinism risk if the underlying matrix multiplication or quantization code deviates from bit-exact soft-float behaviors.

## 4. Determinism Risk Analysis

The T81 stack explicitly guarantees bit-exact reproducible execution for validated surfaces. However, significant determinism risks persist across several structural areas:

*   **Soft-Float Platform Drift (`T81Float`)**: While operations like `FAdd` rely on the standard `double` implementation locally (`ctx.registers[insn.a] = alloc_float(result);`), transcendental functions like `FSin` map to `VMFloat::from_double(*ptr_val)` and call `.sin()`. As documented, `cmath` transcendental functions are gated behind `#ifndef T81_DETERMINISTIC` and throw `std::domain_error` in deterministic builds. This means that either these functions lack true cross-platform bit-exactness, or they strictly trap to prevent drift.
*   **Arbitrary Precision Arithmetic (`T81BigInt`)**: `T81BigInt` precision is deliberately gated to 64-bit limits during literal parsing in `IRGenerator` to prevent unbounded memory allocation and execution scaling within the VM. While `T81Fraction` mitigates precision loss during rational arithmetic, this hard cap points to a risk of silent overflow handling if precision boundaries are exceeded or unsupported operations (like arbitrary-precision exponents) occur.
*   **Container Non-Determinism (`T81Map`, `T81Set`)**: The most critical risk is the non-deterministic iteration order of `T81Map` and `T81Set`. The implementation explicitly relies on C++ standard libraries (e.g., `std::hash`), which produce different key ordering across architectures and runs. The framework requires canonical serialization (`serialize_canonical()`) to sort keys before producing verifiable output, but the in-memory iteration itself remains architecture-dependent, breaking the invariant of deterministic state transition throughout the lifetime of the process.
*   **Trace-JIT Discrepancies**: JIT compilation (`JitCompiler`) serves as a hot-spot optimizer, but the specification lacks proof of complete semantic equivalence with the T81VM reference interpreter. This makes the JIT an unverified, indeterminate surface.

## 5. Integration Gaps

The T81 repository contains several elements that indicate a system "in transit" between an early conceptual design and a fully materialized deterministic engine:

*   **Legacy Components (Hanoi VM)**: The historical experimental concept `legacy/hanoivm/` remains in the repository as a design reference. It utilizes an outdated `MAX_TEMP_LEN` buffer limit and relies on CWEB literate programming, creating architectural size discrepancies and technical debt compared to the modern `T81VM` interpreter stack.
*   **Draft Specs vs. Beta Implementation**: The specification language (`spec/t81lang-spec.md`) is trailing behind the actual C++ Beta implementation. Experimental keywords (`recurse`, `distributed`, `infinite`, `train`, `infer`, `reflect`) and cognitive tiers exist in the parser and `SemanticAnalyzer` but lack normative definition.
*   **Vector Polyfills in Complex Containers**: While the type analyzer (`SemanticAnalyzer`) explicitly handles `Map`, `Set`, `List`, `Tree`, and `Graph`, the VM fundamentally implements `MapNew` and `SetNew` by allocating string vectors (`StrVecNew` equivalent handles). Thus, the structural representation in the VM does not natively match the distinct high-level language types, leaning on standard vector wrappers.

## 6. Governance Coverage

The Axion policy engine establishes an unparalleled degree of execution governance. The T81VM injects policy evaluation into every single instruction step via `eval_axion_call(t81::axion::reasons::kStep, current_pc, insn.opcode);`. If the evaluation is denied, the VM throws `Trap::SecurityFault`, halting execution.

The governance limits stack recursion (`max_recursion`), instruction count (`max_instructions`), shape complexity (`max_shape_complexity`), and memory allocation (`max_tensor_elements`). Crucially, memory transition operations (`AxRead`, `AxSet`, `MetaRead`, `MetaWrite`) log explicit access events. WLOAD operations enforce strict allow/deny code contracts (`AI_POLICY_ALLOW_WLOAD_POLICY_GATE`) verifying that policy is actively evaluated prior to any high-risk load execution, rather than merely logging audit trails post-execution.

## 7. CI Enforcement Coverage

The project enforces architectural invariants directly in the Continuous Integration (CI) pipeline, elevating governance from documentation to code:

*   **Determinism Gates**: Scripts like `t81lang_repro_gate.py` and `t3k_repro_gate.py` ensure the deterministic compilation of T81Lang to TISC bytecode, checking exact hashes against `t81lang_repro_hash.txt`. Any shift in AST output or bytecode compilation strictly breaks the build.
*   **Architecture Coherence**: The script `check_architecture_coherence.py` checks for the presence of specific AI opcodes (`ATTN`, `QMATMUL`, `EMBED`) in both the C++ ISA definitions (`opcodes.hpp`) and the VM dispatch logic (`vm.cpp`). It requires that WLOAD policy tests assert both expected "allow" and "deny" outputs.
*   **Cross-Architecture Gates**: The workflow configurations mandate cross-architecture bit-identity execution to catch platform-dependent compilation issues (`- t3k-cross-arch-bit-identity`, `- t81lang-cross-arch-bit-identity`). This confirms the system protects against standard C++ drift.

## 8. Evidence of Deterministic Behavior

The T81 repository contains exhaustive test suites (`tests/determinism/test_primitives.cpp`, `test_float.cpp`, `test_containers.cpp`) specifically asserting cross-platform stability. The March 2026 Type Audit explicitly resolved known weaknesses like `Cell` overflow and `T81Float` signed-zero canonicalization.

The project achieves bit-exact output via deterministic serializations, generating strict hashes in test fixtures. While fundamental operations (such as scalar math and binary container sizes) demonstrate flawless determinism, container iteration (`T81Map`) and unverified execution layers (Trace-JIT) are out-of-scope for the guaranteed Determinism Surface Registry and remain active risks.

## 9. System Maturity Assessment

The architecture separates its boundaries effectively through varied maturity states:

1.  **Frozen**: The foundational data types (`T81Float`, `T81BigInt`, `tryte`) and TISC ISA specification. Changes are strictly governed.
2.  **Stable (Bounded)**: T81VM Interpreter, Axion Policy Engine, CanonFS. The runtime execution conforms deterministically to policy, but the claims are bounded to explicitly verified surfaces (not JIT, not networking).
3.  **Beta**: T81Lang. The compiler frontend successfully processes types, generates valid bytecodes, and relies on canonical sorting for complex serialization. However, the parser allows experimental keywords out of scope for the Draft specification.
4.  **Experimental**: Trace-JIT execution, Distributed Cognitive Tiers (`Gossip`, `TickSync`), and AI inference RFC-0026 operations. These lack formal determinism guarantees.

## 10. Final Verdict

**Verdict: B. Mostly coherent but incomplete integration**

**Justification:**
The T81 Foundation successfully enforces deterministic ternary computing invariants across its core operations. Its architecture is deeply thought-out, mathematically rigorous, and securely governed by an ISA-integrated policy engine (Axion) mapped directly to its execution VM. The CI gates (cross-arch bit identity) prove that the implementation genuinely aligns with its claims on verified surfaces.

However, full end-to-end integration is incomplete. High-level language constructs (`T81Map`) continue to rely on non-deterministic binary memory primitives (`std::hash`, standard vector fallbacks). The experimental AI lanes (RFC-0026) exist cleanly within the ISA but lack the complete deterministic execution backing of the scalar primitives. Lastly, the presence of experimental cognitive tiers drifting ahead of the formal Draft specification points to a system actively maturing, but not yet presenting a flawless, singular computing substrate from language to hardware.

## 11. Recommendations

1.  **Resolve Container Determinism**: Decouple `T81Map` and `T81Set` from `std::hash`. Implement a pure, deterministic ternary hashing algorithm within the VM, ensuring that iteration order is structurally deterministic at runtime without requiring post-processing canonicalization (`serialize_canonical()`).
2.  **Define JIT Equivalence Specs**: Produce a formal proof or equivalence specification for the Trace-JIT. The JIT must either be guaranteed bit-identical to the T81VM interpreter or be strictly gated off when executing within the Deterministic Core Profile.
3.  **Harden Soft-Float Boundaries**: Ensure all `#ifndef T81_DETERMINISTIC` exclusions in `cmath` transcendental wrappers (`FSin`, `FExp`) have explicitly verified, bit-identical bounded soft-float replacements instead of trapping or deferring to platform hardware.
4.  **Align T81Lang with Specification**: Ensure the T81Lang implementation precisely conforms to the `t81lang-spec.md`. Demarcate or flag experimental parser keywords (`recurse`, `train`) to enforce strict Draft Spec compliance out-of-the-box.
5.  **Remove Legacy Artifacts**: Purge the historical Hanoi VM (`legacy/hanoivm/`) to resolve architecture scope, preventing confusion and maintaining a singular modern codebase.
