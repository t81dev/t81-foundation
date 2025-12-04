# T81 Foundation: Actionable Task List

**Last Updated:** November 29, 2025

This document lists the concrete, prioritized tasks for the next development cycle, aligned with the strategic priorities in `ROADMAP.md`.

______________________________________________________________________

## How to Contribute

1.  Read the updated [`ROADMAP.md`](./ROADMAP.md) to understand the high-level goals.
2.  Pick a task from the lists below, starting with **P0**.
3.  Follow the guidelines in [`CONTRIBUTING.md`](./CONTRIBUTING.md).

______________________________________________________________________

### [P0] T81Lang Compiler

**Goal:** Fully implement the C++20 compiler to match the `t81lang-spec.md`. This is the critical path to v1.0.

- **[EPIC] Implement Semantic Analysis & Type System:**
    - **[DONE] [M] Task:** Create the foundational `SemanticAnalyzer` class that traverses the AST. Initially, it will only resolve symbols and populate a `SymbolTable`.
    - **[L] Task:** Implement the core type-checking logic within the `SemanticAnalyzer`. Enforce all type rules from the spec, including numeric widening, function signatures, and return types.
    - **[M] Task:** Implement type checking for generic types, focusing on `Option[T]` and `Result[T, E]`.
    - **[S] Task:** Create a new end-to-end test for `Option/Result` that defines a function returning an `Option`, calls it, and verifies the result. This will be the driving test for the type system.

- **[EPIC] Expand Language Feature Support:**
    - **[L] Task:** Expand the `Parser` to recognize the `loop` and `match` keywords and statement structures.
    - **[L] Task:** Implement the lowering of `loop` and `match` expressions in the `IRGenerator` into the correct TISC conditional jumps and labels.
    - **[M] Task:** Persist match metadata (variants, guards, payload shapes) through the compiler/CLI stack and emit it via Axion/trace hooks so downstream tooling can reason about canonical handles.

- **[EPIC] Improve Developer Experience:**
    - **[M] Task:** Implement a robust error reporting system that provides clear, actionable error messages with line and column numbers for both parsing and type errors.
    - **[M] Task:** Create a `t81` command-line tool with `compile` and `run` subcommands to drive the compiler and VM.

______________________________________________________________________

### [P1] HanoiVM & TISC Runtime

**Goal:** Harden the VM and integrate it with other core systems. These tasks can be worked on in parallel with P0, but are secondary.

- **[EPIC] Implement the T81VM Memory Model:**
    - **[L] Task:** Design and implement the full stack and heap memory model as defined in `spec/t81vm-spec.md`.
    - **[M] Task:** Add VM instructions for stack manipulation (push, pop, stack pointers).

- **[EPIC] Harden the VM:**
    - **[M] Task:** Improve VM fault handling. Ensure all illegal operations (e.g., division by zero, out-of-bounds memory access) result in deterministic, spec-compliant faults.
    - **[L] Task:** Add extensive "negative" tests for the VM that deliberately trigger faults and verify the correct behavior.
    - **[S] Task:** Surface match metadata hints in the Axion/trace log to confirm CLI workloads can replay guard coverage during execution.

______________________________________________________________________

### [P2] Axion Kernel & Documentation

**Goal:** Transform stubs into functional components and improve documentation.

- **[EPIC] Implement the Axion Kernel:**
    - **[M] Task:** Formalize the API between the HanoiVM and the Axion Kernel.
    - **[L] Task:** Implement the first set of safety policies in the Axion Kernel (e.g., Recursion Depth Limiter, Instruction Counter).
    - **[M] Task:** Integrate the specified Axion hooks (`AXREAD`, `AXSET`, etc.) into the VM's main dispatch loop.

- **[EPIC] General Documentation & Good First Issues:**
    - **[S] Task:** Add more unit tests for existing data types (`T81Float`, `Tensor`).
    - **[S] Task:** Improve Doxygen comments on public headers in `/include/t81/`.
    - **[M] Task:** Update the `docs/tensor-guide.md` to reflect the current C++ `Tensor` API.
