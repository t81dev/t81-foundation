# T81 Research Continuity — SYSTEM_ARCHITECTURE

**Document ID:** `docs/ai-context/SYSTEM_ARCHITECTURE.md`
**Version:** 1.0.1 · **Last Reviewed:** 2026-10-29
**Source Snapshot:** `main` at `f8ac77159ded2c4b93f4cc83abd48f6f35655468` (checked 2026-10-29; refresh before use)
**Status:** Non-normative architecture index; not a complete integration trace or production conformance certificate

---

## 1. Subsystem Architecture Map

T81 has documented policy, runtime, model-intake, and artifact surfaces with different maturity levels. The diagram is a **conceptual map**, not one verified end-to-end call path or a universal claim of bit-exact cross-platform behavior.

```mermaid
graph TD
    UserCLI[T81 CLI / Tooling] --> T81Lang[T81Lang Compiler / AST / IR]
    UserCLI --> AIBridge[AI Model Intake Bridge GGUF/Safetensors]

    T81Lang --> TISCBytecode[TISC Bytecode / Artifacts]
    AIBridge --> ModelTensors[Model Weights & Tensors]

    subgraph "Deterministic Execution Boundary (DCP)"
        TISCBytecode --> T81VM[T81VM Interpreter / State Core]
        ModelTensors --> T81VM

        T81VM <--> AxionEngine[Axion Policy Kernel eval_axion_call]
        T81VM <--> SoftFloat[Deterministic Soft-Float CORDIC RFC-0030]
        T81VM <--> CanonFS[CanonFS Content-Addressed VFS RFC-00D1]
    end

    CanonFS -. proposed evidence interface .-> Evidence[Experimental evidence bundles / draft RFC-00D4]

    subgraph "DAIOS Draft Target Layers (RFC-00D2)"
        L1[1. Deterministic Execution Substrate - REAL]
        L2[2. Policy & Admission Layer - REAL]
        L3[3. Immutable Artifact & Provenance Layer - REAL]
        L4[4. AI Object Layer - REAL / BOUNDED]
        L5[5. AI Runtime Layer - PARTIAL]
        L6[6. System / Bare-Metal Layer - DEFERRED]
    end

    T81VM -. Emulation .-> BinaryHost[Binary Host OS / Hardware x86_64 / ARM64 / WASM]
```

---

## 2. Core Subsystems and Responsibilities

### 2.1 TISC Instruction Set Architecture (v1.9.0)
- **Path:** `include/t81/isa/opcodes.hpp`, `src/isa/`
- **Role:** Frozen 81-opcode instruction set operating on a 64-bit virtual register state machine (`STATE = (R, PC, SP, FLAGS, MEM, META)`).
- **Maturity:** **FROZEN (v1.9.0)**. Opcode values and state encoding are protected against casual modification.
- **Key Opcode Domains:** Arithmetic, logic, flow control, CanonFS I/O, Axion policy calls, and tensor descriptors.

### 2.2 T81VM Virtual Machine Core
- **Path:** `src/vm/vm.cpp`, `include/t81/vm/vm.hpp`
- **Role:** Deterministic Core Profile (DCP) execution engine. Decodes TISC bytecode, manages VM memory frames, and enforces pre-dispatch policy checks (`eval_axion_call`).
- **Maturity:** **BETA RUNTIME**. Specific deterministic call paths have reported tests; parity across all programs, features and platforms is not established by this index.

### 2.3 Axion Policy Governance Engine
- **Path:** `src/axion/`, `include/t81/kernel/axion/`
- **Role:** Pre-side-effect access control kernel. Evaluates authorization rules prior to state mutations, filesystem I/O, or external FFI dispatches.
- **Maturity:** **ALPHA GOVERNANCE**. Fails closed; logs explicit rejection reasons in `vm->state().axion_log` on denial.

### 2.4 CanonFS Content-Addressed Identity File System
- **Path:** `src/core/fs/`, `include/t81/fs/`
- **Role:** Content-addressed VFS implementing cryptographic hash verification (CanonHash-81) and foreign file system interchange ([RFC-00D1](../../spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md)).
- **Maturity:** **BETA IDENTITY FS**. Provides immutable artifact storage, provenance manifests, and export/import contracts (`t81.canonfs-import.v1`, `t81.canonfs-export.v1`).

### 2.5 T81Lang Tooling & Compiler
- **Path:** `src/lang/`, `include/t81/lang/`
- **Role:** High-level source lexer, parser, AST builder, IR generator, and TISC bytecode compiler.
- **Maturity:** **BETA LANGUAGE**. Supports structured control flow, domain assertions, and TISC compilation.

### 2.6 Deterministic Soft-Float Subsystem (RFC-0030)
- **Path:** `src/core/isa/math/`, `vm/determinism/`
- **Role:** Software implementation of transcendental math functions (sin, cos, exp, log, CORDIC) to eliminate platform-dependent `libm` floating-point rounding discrepancies.
- **Maturity:** **FUNCTIONAL / DCP**. Essential for bit-exact cross-platform neural network trace replay.

### 2.7 AI Model Intake & Tensor Bridge
- **Path:** `src/tools/model/`, `tools/cli/ai/`
- **Role:** Metadata bridge for inspecting GGUF and Safetensors model headers, inventorying weight tensors, and packing 2-bit tryte weights (`.t81w`).
- **Maturity:** **METADATA BRIDGE**. Inspects tensor geometry and converts weights; CPU execution uses SWAR emulation.

---

## 3. DAIOS Target Architecture & Layering (RFC-00D2)

Draft/informational [RFC-00D2](../../spec/rfcs/RFC-00D2-daios-target-architecture-and-sequencing.md) **proposes** a six-layer target architecture. It guides planning (*design ahead, implement behind proof*) and does not accept new runtime APIs or certify each layer as integrated.

| Layer ID | Layer Name | Material Implementation Reality | Current Build-Against Scope |
| :--- | :--- | :--- | :--- |
| **Layer 1** | **Deterministic Substrate** | **REAL** | T81VM, DCP determinism, soft-float math (RFC-0030), cross-platform CTest suites. |
| **Layer 2** | **Policy & Admission** | **REAL** | Axion pre-side-effect evaluation, fail-closed policy checks, audit logging. |
| **Layer 3** | **Immutable Artifacts** | **REAL** | CanonFS content-addressed storage, provenance manifests (RFC-00D1). |
| **Layer 4** | **AI Object Layer** | **REAL / BOUNDED** | Bounded family (`assess-fixed`, `route-fixed`, `classify-fixed`). |
| **Layer 5** | **AI Runtime Layer** | **PARTIAL** | Model intake bridge, Safetensors header parsing, deterministic trace cache. |
| **Layer 6** | **System / Bare-Metal** | **DEFERRED** | Bare-metal kernel / TernaryOS deferred until Layers 1–4 are fully stabilized. |

---

## 4. Implementation Maturity Matrix

| Component / Subsystem | Source Path | Target RFC / Spec | Test Verification Target | Maturity Level |
| :--- | :--- | :--- | :--- | :--- |
| **TISC ISA Core** | `src/isa/` | `spec/tisc-spec.md` | CTest #391–396 | **Frozen (v1.9.0)** |
| **T81VM DCP Engine** | `src/vm/` | `spec/vmspec.md` | `vm_determinism_property_test` | **Beta Runtime** |
| **Axion Policy Engine** | `src/axion/` | `spec/rfcs/RFC-00D2-daios-target-architecture-and-sequencing.md` | CTest #154 | **Alpha Governance** |
| **CanonFS Interchange** | `src/core/fs/` | `spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md` | CTest #84–85 | **Beta Identity FS** |
| **Soft-Float CORDIC** | `src/core/isa/math/` | `spec/rfcs/RFC-0030-deterministic-math-subsystem.md` | CTest #355 | **Functional / DCP** |
| **Accountable Evidence** | `examples/governance/` | [RFC-00D4 draft](../../spec/rfcs/RFC-00D4-accountable-decision-bundle.md) | Bounded precursor tests; confirm exact CTest invocation | **Experimental / not RFC-conformant by default** |
| **Ternary Hardware RTL** | `rtl/core/` | `t81-hardware` repo | Verilog / SystemVerilog simulation | **Experimental / RTL** |
| **2-Bit MatVec SIMD** | `experiments/` | `PACKED_TERNARY_MATVEC_PERFORMANCE_REPORT.md` | `t81_packed_ternary_matvec` | **Experimental / Frozen** |

---

## 5. Architectural Limitations & Integration Gaps

1. **Software CPU Emulation Tax:** Binary vector ALUs (AVX2/NEON) incur heavy bit-shuffle overhead when unpacking 2-bit tryte fields during MatVec execution, limiting performance to $75.3\%$ of INT8 AVX2.
2. **Durable Witness Isolation:** As demonstrated in `experiments/durable_witness/`, an isolated SQLite WAL witness store remains vulnerable to snapshot rollback unless paired with external monotonic sequence counters or append-only HMAC receipt ledgers.
3. **Draft Interchange Boundaries:** While RFC-00D1 v1 JSON seed contracts are stable enough for tooling, full foreign filesystem interop, symlink postures, and archive bundle exports remain draft surfaces.
4. **Hardware Substrate Disconnect:** RTL in `rtl/core/t81_top.sv` uses 2-bit binary representations per trit; no physical 3-state semiconductor characterization data exists in the repository.
