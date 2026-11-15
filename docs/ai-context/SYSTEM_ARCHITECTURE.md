# T81 Research Continuity — SYSTEM_ARCHITECTURE

**Document ID:** `docs/ai-context/SYSTEM_ARCHITECTURE.md`
**Version:** 1.0.2 · **Last Reviewed:** 2026-11-15
**Source Snapshot:** freshly fetched `origin/main` at `40695b0ba5cea3e43563b6ed2443c987d2738089` (2026-11-15). Prior 2026-10-29 snapshot: `f8ac77159ded2c4b93f4cc83abd48f6f35655468`; historical evidence remains versioned. Refresh before action.
**Status:** Non-normative architecture index; not a complete integration trace or production conformance certificate

---

## 1. Subsystem Architecture Map

T81 has documented policy, runtime, model-intake, and artifact surfaces with different maturity levels. The diagram is a **conceptual map**, not one verified end-to-end call path or a universal claim of bit-exact cross-platform behavior.

```mermaid
graph TD
    UserCLI[T81 CLI / Tooling] --> T81Lang[T81Lang Compiler / AST / IR]
    UserCLI --> AIBridge[AI Model Intake Bridge GGUF/Safetensors]

    T81Lang --> TISCContract[TISC Instruction Set Architecture Contract]
    AIBridge --> ModelTensors[Model Weights & Tensors]

    subgraph "Execution Profile 1: Hosted T81VM (DCP Boundary)"
        TISCContract --> T81VM[T81VM Interpreter / State Core]
        ModelTensors --> T81VM
        T81VM <--> AxionEngine[Axion Policy Kernel eval_axion_call]
        T81VM <--> SoftFloat[Deterministic Soft-Float CORDIC RFC-0030]
        T81VM <--> CanonFS[CanonFS Content-Addressed VFS RFC-00D1]
        T81VM -. Hosted Execution .-> BinaryHost[Host OS: Linux / macOS / Windows / WASM]
    end

    subgraph "Experimental branch profile: Freestanding Axion OS / TernaryOS"
        TISCContract -. bounded experimental subset .-> FreestandingHAL[cpa004_evaluator - absent from pinned main]
        FreestandingHAL --> BareMetal[Bare-Metal AArch64 / QEMU Virt Post-ExitBootServices]
    end

    CanonFS -. proposed evidence interface .-> Evidence[Experimental evidence bundles / draft RFC-00D4]

    subgraph "Independent Research Direction: DAIOS Target Layers (RFC-00D2)"
        L1[1. Deterministic Execution Substrate - REAL]
        L2[2. Policy & Admission Layer - REAL]
        L3[3. Immutable Artifact & Provenance Layer - REAL]
        L4[4. AI Object Layer - REAL / BOUNDED]
        L5[5. AI Runtime Layer - PARTIAL]
        L6[6. System / Bare-Metal Layer - PROPOSED / INDEPENDENT]
    end
```

---

## 2. Core Subsystems and Responsibilities

### 2.1 TISC Instruction Set Architecture (Computational Contract)
- **Path:** `include/t81/isa/opcodes.hpp`, `src/isa/`, `spec/tisc-spec.md`
- **Role:** The core computational instruction contract defining state transitions on a 64-bit virtual register machine (`STATE = (R, PC, SP, FLAGS, MEM, META)`). It defines opcode semantics across arithmetic, logic, control flow, memory operations, and system interactions.
- **Maturity:** **FROZEN CONTRACT; KNOWN OPERAND DISCREPANCIES UNDER REVIEW**.
- **Execution Profiles:**
  1. **Hosted Profile (T81VM):** C++ runtime implementing full VM interpretation, JIT exploration, FFI, dynamic Axion policy checks, and CanonFS virtual storage on conventional operating systems.
  2. **Experimental Freestanding Profile (Axion OS / HAL):** CPA records report a bounded evaluator on `experiment/cpa005-integration-base`, including post-`ExitBootServices` execution. `ternaryos/hal/cpa004_evaluator.{hpp,cpp}` is absent from pinned main; this is not full hosted-ISA parity or a production OS claim.

### 2.2 T81VM Virtual Machine Core (Hosted Execution Profile)
- **Path:** `vm/vm.cpp`, `include/t81/vm/vm.hpp`
- **Role:** Deterministic Core Profile (DCP) execution engine on hosted platforms. Decodes TISC bytecode, manages VM memory frames, and enforces pre-dispatch policy checks (`eval_axion_call`).
- **Maturity:** **BETA RUNTIME**. Specific deterministic call paths have reported tests; parity across all programs, features and platforms is not established by this index.

### 2.3 Axion OS / TernaryOS HAL (Freestanding Execution Profile)
- **Path on pinned main:** `ternaryos/hal/` contains boot/HAL work. The CPA evaluator path exists on the separately reported experimental branch, not pinned main.
- **Role:** Freestanding TISC execution evaluator embedded into bare-metal UEFI/QEMU boot flows. Evaluates admitted instruction subsets without host C runtime, memory allocators, STL, or dynamic operating system services.
- **Maturity:** **EXPERIMENTAL / BRANCH-BOUNDED REPORTED EVIDENCE**. CPA-004/005 reported matching boundaries for one canonical program; CPA-006 covered 19 frozen cases. Those historical records are not rerun or generalized by this refresh.

### 2.4 Axion Policy Governance Engine
- **Path:** `kernel/axion/`, `include/t81/axion/`
- **Role:** Pre-side-effect access control kernel. Evaluates authorization rules prior to state mutations, filesystem I/O, or external FFI dispatches.
- **Maturity:** **ALPHA GOVERNANCE**. Fails closed; logs explicit rejection reasons in `vm->state().axion_log` on denial.

### 2.5 CanonFS Content-Addressed Identity File System
- **Path:** `fs/`, `include/t81/canonfs/`
- **Role:** Content-addressed VFS implementing cryptographic hash verification (CanonHash-81), portable storage mapping (`storage_key_v1` and `snapshot_storage_key_v1` using `h1-<64_hex_digits>` paths), and foreign file system interchange ([RFC-00D1](../../spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md)).
- **Maturity:** **BETA IDENTITY FS**. Provides immutable artifact storage, provenance manifests, and export/import contracts (`t81.canonfs-import.v1`, `t81.canonfs-export.v1`).

### 2.6 T81Lang Tooling & Compiler
- **Path:** `lang/frontend/`, `include/t81/lang/`
- **Role:** High-level source lexer, parser, AST builder, IR generator, and TISC bytecode compiler.
- **Maturity:** **BETA LANGUAGE**. Supports structured control flow, domain assertions, and TISC compilation.

### 2.7 Deterministic Soft-Float Subsystem (RFC-0030)
- **Path:** `isa/math/t81_soft_math/`, `vm/determinism/`
- **Role:** Software implementation of transcendental math functions (sin, cos, exp, log, CORDIC) on implemented deterministic paths; remaining host-math fallbacks require separate qualification.
- **Maturity:** **FUNCTIONAL / DCP**. Included core arithmetic is governed by DCP contracts; this does not certify every transcendental fallback or neural backend.

### 2.8 AI Model Intake & Tensor Bridge
- **Path:** `tools/model/`, `tools/cli/ai/`
- **Role:** Metadata bridge for inspecting GGUF and Safetensors model headers, inventorying weight tensors, and packing 2-bit tryte weights (`.t81w`).
- **Maturity:** **METADATA BRIDGE**. Inspects tensor geometry and converts weights; model inspection and conversion do not establish inference speed or full backend coverage.

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
| **TISC ISA Core** | `src/isa/` | `spec/tisc-spec.md` | `t81_vm_fault_test`; freeze integrity script | **Frozen contract; version labels and semantics require scoped evidence** |
| **T81VM DCP Engine** | `vm/` | `spec/t81vm-spec.md` | `vm_determinism_property_test` | **Beta Runtime** |
| **Axion Policy Engine** | `kernel/axion/` | `spec/rfcs/RFC-00D2-daios-target-architecture-and-sequencing.md` | `axion_policy_allow_deny_determinism_test` | **Alpha Governance** |
| **CanonFS Interchange** | `fs/` | `spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md` | `canonfs_interchange_test` | **Beta Identity FS** |
| **Soft-Float CORDIC** | `isa/math/t81_soft_math/` | `spec/rfcs/RFC-0030-deterministic-math-subsystem.md` | `test_deterministic_float_boundaries` | **Functional / DCP** |
| **Accountable Evidence** | `examples/governance/` | [RFC-00D4 draft](../../spec/rfcs/RFC-00D4-accountable-decision-bundle.md) | Bounded precursor tests; confirm exact CTest invocation | **Experimental / not RFC-conformant by default** |
| **Ternary Hardware RTL** | external `t81-hardware` repository | external RTL; not a local Foundation source path | Verilog / SystemVerilog simulation | **Experimental / RTL** |
| **2-Bit MatVec SIMD** | `experiments/` | `PACKED_TERNARY_MATVEC_PERFORMANCE_REPORT.md` | `t81_packed_ternary_matvec` | **Experimental / Frozen** |

---

## 5. Architectural Limitations & Integration Gaps

1. **Software CPU Emulation Tax:** Binary vector ALUs (AVX2/NEON) incur heavy bit-shuffle overhead when unpacking 2-bit tryte fields during MatVec execution, with a historical reported result of $75.3\%$ of INT8 AVX2 in the cited packed-matvec study. That measurement is workload/platform scoped, not a universal bound or a new measurement here.
2. **Durable Witness Isolation:** As demonstrated in `experiments/durable_witness/`, an isolated SQLite WAL witness store remains vulnerable to snapshot rollback unless paired with external monotonic sequence counters or append-only HMAC receipt ledgers.
3. **Draft Interchange Boundaries:** While RFC-00D1 v1 JSON seed contracts are stable enough for tooling, full foreign filesystem interop, symlink postures, and archive bundle exports remain draft surfaces.
4. **Hardware Substrate Disconnect:** RTL in external `t81-hardware` RTL uses 2-bit binary representations per trit; no physical 3-state semiconductor characterization data exists in the repository.
5. **Preserved Normative TISC Discrepancies (Unresolved):**
   - **Memory Operands:** `vm/vm.cpp` and `spec/tisc/opcode-semantics.md` use direct immediate addresses for Load and Store, whereas `spec/tisc-spec.md` specifies register-indirect addressing (`R[RS]`).
   - **Branch Operands:** `vm/vm.cpp` and `spec/tisc/opcode-semantics.md` evaluate immediate branch targets and test register $R[B]$ for JZ/JNZ, whereas `spec/tisc-spec.md` specifies register-indirect targets and FLAGS-based zero testing.
   - **TXor Operation:** Runtime evaluates `wrap3(clamp(lhs) - clamp(rhs))`, with mathematical balanced normalization (not signed C++ `%` shorthand); historical prose labels TXor as "XOR / Sum" without a complete truth table.
   - **Opcode Inventories:** Discrepancy between 174 capability-frozen opcodes, 209 registry entries, and 244 named C++ header opcodes (with undefined opcodes 221–223 and 247–255).

---

## 2026-10-31 architecture boundary note

The reported ADM-2 M1 local branch is not an admitted production layer or a verified end-to-end integration. Do not infer expanded DAIOS layer maturity, Axion authority, or DCP coverage from selected macOS ARM64 CTests or successful translation-unit compilation. Architecture changes require inspected diffs, accepted contracts, and scoped validation.

---

## 2026-11-05 execution-profile boundary and pending ISA contract

**Cross-profile experimental boundary:** CPA-004/005 demonstrated the canonical 351-byte program across hosted T81VM and freestanding Axion/TernaryOS after `ExitBootServices`. CPA-006 expanded the *same implemented semantic profile* to 19 frozen cases, including successful Halt, runtime trap, admission rejection and step-budget exhaustion, with 57 hosted and 57 QEMU executions. The freestanding evaluator and CPA-006 artifacts are on the experimental integration branch, **not on current main**. See [PR #710](https://github.com/t81dev/t81-foundation/pull/710), merged into `experiment/cpa005-integration-base`; [PR #709](https://github.com/t81dev/t81-foundation/pull/709) remains an open draft. Do not infer that main contains the evaluator.

**ISA-001 source-level divergence:** Main normative `spec/tisc-spec.md` describes `Load: R[A] = MEM[R[B]]` and `Store: MEM[R[A]] = R[B]`, whereas hosted VM uses direct A/B logical addresses. Main specification calls for register-derived branch targets and FLAGS-tested conditional predicates, whereas hosted VM uses immediate target A and register-tested JZ/JNZ (JN/JP use flags). Scalar TXor implementation is `wrap3(lhs - rhs)` for trit-clamped operands; documentary 'XOR / Sum' wording is ambiguous. Core capability freeze covers bytes 0–173, while header and registry contain conflicting extension assignments. Optional LLVM/MLIR branch lowering also diverges from hosted VM operand conventions; **do not extend CPA conformance claims to these backends**.

**Proposed, not approved:** One forward versioned authoritative TISC transition contract plus explicit historical CPA-profile interpretation. No implicit reinterpretation of raw 13-byte binaries. The new contract must define operands, tags, flags, PC progression, trap/side-effect ordering, opcode profile admission and artifact-to-profile binding before implementation. Preserve CPA evaluators and evidence byte-for-byte. Architecture layers and DAIOS maturity labels above remain unchanged by this assessment.


## 2026-11-15 reconciliation and qualification

The hosted runtime runs on a host OS and uses in-process Axion policy mediation. Experimental Axion OS / TernaryOS boot evidence supplies no OS sandbox or hosted-runtime parity guarantee. `src/isa/` and other `src/*` directories also contain CMake build definitions; the implementation paths above identify actual sources.

PR #799 is merged at pinned main `40695b0ba5cea3e43563b6ed2443c987d2738089`. Its Asio target-scope correction addresses examples-without-tests configurations. Informational ARMv9 cross-compilation succeeded in run `43746367704`, job `132945852419`; this is a cross-build on the PR qualification checkout, not native ARMv9 execution or complete main-head requalification. See [November 15 status](T81_RESEARCH_STATUS_2026-11-15.md) and [retained jobs](receipts/2026-11-15-pr799-all-jobs.json).

Known Load/Store/conditional-branch specification conflicts and ambiguous TXor prose remain unresolved. New differential evidence is an engineering review, not a normative amendment. Frozen opcode arbitrary-precision arithmetic remains separate from currently uncalled wrapping helpers. H01–H08, research negatives, SCG/release holds and non-DCP classifications are unchanged.
