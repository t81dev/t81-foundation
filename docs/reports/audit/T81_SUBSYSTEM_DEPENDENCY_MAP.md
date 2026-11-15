# T81 Foundation — Subsystem Dependency Map

**Audit Designation:** T81-SIA-001
**Reference Document:** Architectural Call Graph, Inter-System Coupling & Dependency Analysis
**Evaluated Systems:** T81Lang Frontend, TISC Instruction Architecture, T81VM Runtime, Axion Policy Engine, CanonFS Storage Subsystem, and TernaryOS Freestanding Microkernel.

---

## 1. System Integration Call Graph

The diagram below maps the actual compile-time target dependencies and runtime invocation call paths across the six primary systems of the repository.

```
                              ┌────────────────────────┐
                              │  T81Lang Source (.t81) │
                              └───────────┬────────────┘
                                          │ (Compiles AST)
                                          ▼
                              ┌────────────────────────┐
                              │   t81_lang_frontend    │
                              │ (Lexer, Parser, Lower) │
                              └───────────┬────────────┘
                                          │ (Emits Bytecode)
                                          ▼
                              ┌────────────────────────┐
                              │        t81_isa         │
                              │ (TISC v1.9.0 Bin/Text) │
                              └───────────┬────────────┘
                                          │
                     ┌────────────────────┴────────────────────┐
                     │ (Loads Program)                         │ (Stores Artifact)
                     ▼                                         ▼
        ┌─────────────────────────┐               ┌─────────────────────────┐
        │         t81_vm          │               │      t81_canonfs        │
        │   (T81VM Interpreter,   │               │ (Content-Addressed FS,  │
        │   Trace JIT, DPE Graph) │               │ Snapshots, Interchange) │
        └────────────┬────────────┘               └────────────┬────────────┘
                     │                                         │
                     │ (Registers Event Hooks)                 │ (Evaluates Storage Actions)
                     ▼                                         ▼
        ┌───────────────────────────────────────────────────────────────┐
        │                           t81_axion                           │
        │      (Axion Policy Engine, Ethics Evaluator, Fail-Closed)     │
        └───────────────────────────────┬───────────────────────────────┘
                                        │
                                        │ (Hosted Core Target: t81_core)
                                        ▼
        ┌───────────────────────────────────────────────────────────────┐
        │                           t81_core                            │
        │     (Ternary Numerics, SWAR, Cryptography, Config, BigInt)    │
        └───────────────────────────────────────────────────────────────┘
                                        ┆
                                        ┆ [CRITICAL ARCHITECTURAL DISCONNECT]
                                        ┆ No runtime link between t81_core and
                                        ┆ freestanding BOOTAA64.EFI
                                        ▼
        ┌───────────────────────────────────────────────────────────────┐
        │                       ternaryos (HAL)                         │
        │   (UEFI Stub -> Bare Kernel -> C++ Bridge -> EL0 MMU/SVC)     │
        │                 [Standalone PE32+ Executable]                 │
        └───────────────────────────────────────────────────────────────┘
```

---

## 2. CMake Target Link Dependency Matrix

Inspecting the authoritative build system (`CMakeLists.txt` and `ternaryos/CMakeLists.txt`) reveals how libraries are coupled at link time:

| Target | Kind | Direct Target Dependencies (`target_link_libraries`) | Include Directories Exposed |
| :--- | :--- | :--- | :--- |
| `t81_core` | STATIC | `Threads::Threads` (standard threading) | `include/`, `src/` |
| `t81_isa` | STATIC | `t81_core` | `include/t81/isa/` |
| `t81_lang_frontend` | STATIC | `t81_core`, `t81_isa` | `include/t81/lang/` |
| `t81_vm` | STATIC | `t81_core`, `t81_isa`, `t81_axion` | `include/t81/vm/` |
| `t81_axion` | STATIC | `t81_core` | `include/t81/axion/` |
| `t81_canonfs` (in `core`)| STATIC | (Part of `t81_core`) | `include/t81/canonfs/` |
| `t81_tool_cli` | STATIC | `t81_core`, `t81_vm`, `t81_isa`, `t81_lang_frontend` | `tools/cli/` |
| `t81` (CLI Executable)| EXECUTABLE | `t81_tool_cli`, `t81_vm`, `t81_isa`, `t81_lang_frontend`| (Internal) |
| `t81_ternaryos_hal` | STATIC (Hosted) | `t81_core`, `t81_axion` | `ternaryos/`, `include/` |
| `t81_ternaryos_qemu_slice6_efi` | CUSTOM (PE32+) | **NONE** (Compiled with `-nostdlib`, links no hosted libraries) | `ternaryos/hal/` |

---

## 3. Subsystem Coupling & Interface Analysis

### 1. T81Lang → TISC Interface
- **Coupling Mechanism:** Direct C++ abstract syntax tree (AST) lowering.
- **Source Paths:** `lang/frontend/ast.hpp`, `lang/frontend/ir_generator.cpp`, `isa/tisc_builder.hpp`.
- **Integrity Invariant:** T81Lang cannot generate instructions outside the frozen TISC v1.9.0 specification. Parser asserts valid ternary literals (`-`, `0`, `+`), fixed bit widths, and cognitive tier bounds at compile time.
- **Status:** **Tightly Coupled, Correctly Isolated.**

### 2. TISC → T81VM Interface
- **Coupling Mechanism:** Bytecode stream ingestion and decode loop.
- **Source Paths:** `isa/tisc_binary.cpp`, `vm/vm.cpp`, `vm/decoder.cpp`.
- **Integrity Invariant:** Strict Determinism Profile enforces identical register state across execution runs. Register operations use explicit Horner-evaluated integer and SWAR 128-trit routines.
- **Status:** **Tightly Coupled, Conforming.**

### 3. T81VM → Axion Policy Interface
- **Coupling Mechanism:** Runtime callback bridge and register trace observers.
- **Source Paths:** `vm/policy_trace_bridge.cpp`, `kernel/axion/policy_engine.cpp`.
- **Integrity Invariant:** The VM calls `t81::axion::PolicyEngine::evaluate()` before executing side-effecting operations (such as memory writes, tensor loads, or external FFI dispatches).
- **Status:** **Fully Integrated.**

### 4. Axion Policy → CanonFS Interface
- **Coupling Mechanism:** Pre-operation policy guards on storage actions.
- **Source Paths:** `kernel/axion/canonfs_hook.cpp`, `fs/persistent_driver.cpp`, `tools/cli/canonfs_tools/`.
- **Integrity Invariant:** CanonFS import/export and snapshot creation require positive authorization from active policy profiles (`permissive`, `import-only`, `export-only`, `deny-all`). Any policy denial immediately halts storage mutation and leaves the disk state unchanged.
- **Status:** **Fully Integrated.**

### 5. Hosted Runtime vs. Freestanding TernaryOS (The Broken Bridge)
- **Coupling Mechanism:** Zero link-time or binary-level coupling.
- **Observation:**
  - While `t81_ternaryos_hal` exists as a hosted static library in CMake (allowing unit tests like `hal_boot_test` to test kernel data structures under Linux/macOS), the actual freestanding bootable image (`BOOTAA64.EFI`) is built completely separately by a custom clang command with `-ffreestanding -nostdlib`.
  - The freestanding image links only 8 standalone source files (`qemu_armv8_efi_stub.c`, `qemu_slice6_bare_kernel.c`, `qemu_slice6_cpp_bridge.cpp`, `qemu_slice6_bridge_irq.cpp`, `qemu_slice6_el0_mmu.cpp`, `qemu_slice6_el0_svc_bridge.cpp`, `qemu_slice6_freestanding_crt.c`, and `canon_exec_loader.cpp`).
  - **Result:** None of the files in `src/vm/`, `src/lang/`, `src/isa/`, or `kernel/axion/` are linked into `BOOTAA64.EFI`.
- **Status:** **Unintegrated / Architectural Schism.**

---

## 4. Subsystem Dependency Invariants Summary

1. **Compilation Invariant:** $\text{AST}(\text{T81Lang}) \longrightarrow \text{TISC} \text{ Bytecode}$ holds deterministically.
2. **Policy Invariant:** $\text{Action} \xrightarrow{\text{Evaluation}} \text{Deny} \implies \text{No Side Effect}$ holds strictly across hosted operations.
3. **Storage Invariant:** $\text{Hash}(Object) = \text{SHA3-256}(Payload)$ holds across CanonFS object storage.
4. **Freestanding Invariant:** Axion OS provides an independent hardware substrate, but operates in complete isolation from the T81 language and virtual machine runtime.
