# Project Roadmap

*Last Updated: 2025-11-27*

## 1. Overview

### 1.1 Vision and Scope

**Vision**: T81 Foundation is building a post-binary computing stack that treats “thinking” as a first-class concern. It uses ternary logic (−1, 0, +1) and base-81 encodings to create a safer, more expressive foundation for advanced AI systems. The goal is a civilization-grade platform where AI workloads can run deterministically, transparently, and auditable from the instruction set to cognitive-level behavior.

**Scope for v1.0**: The v1.0 release will deliver a stable, documented, and production-ready implementation of the core T81 stack. This includes the T81Lang compiler, the TISC instruction set, the HanoiVM virtual machine, foundational data types (BigInt, Fraction, Tensor), and the baseline Axion Kernel for determinism verification. The target audience for v1.0 is systems developers and AI/PL researchers.

### 1.2 Current Maturity Level (Honest Status Snapshot)

The project is at a **late-alpha / early-beta stage**.

**Strengths**:
*   **Clear Vision & Specification**: The project's goals are exceptionally well-defined in `/spec/`. The architecture is clear and the core concepts are constitutionally enshrined.
*   **Strong Foundations**: Core data types like `T81BigInt` and the basic VM instruction processing loop are implemented and tested.
*   **Modern C++ Toolchain**: The project has successfully migrated to a single, modern C++20 compiler toolchain in `/src/frontend/`. The legacy compiler has been fully removed.
*   **Feature-Rich Virtual Machine**: The HanoiVM in `/src/vm/` is more mature than initially assessed. It already implements a wide array of TISC opcodes, including those for advanced types like `Option` and `Result`.
*   **Test-Aware Culture**: There is a significant body of unit and integration tests, and the build system (`CMakeLists.txt`) reflects a commitment to testing.

**Weaknesses**:
*   **Spec vs. Implementation Gap (Compiler Focus)**: The primary gap is in the **T81Lang compiler**. The `t81lang-spec.md` is far ahead of the implementation. Key language features like `loop`, `match`, and full support for generics (`Option[T]`, `Result[T,E]`) are specified but not yet implemented in the compiler.
*   **Incomplete Type System**: The compiler lacks a dedicated semantic analysis and type-checking pass. This is the most significant missing piece needed to support complex language features.
*   **Stubbed Components**: Critical components like the Axion Kernel (`src/axion/engine.cpp`) and CanonFS (`src/canonfs/in_memory_driver.cpp`) are present but are minimal stubs.

### 1.3 Target v1.0 Definition (What “Done” Looks Like)

A component is "v1.0 complete" when it meets the following criteria:
1.  **Fully Implemented**: All features defined for it in the `/spec/` are implemented.
2.  **Fully Tested**: It has comprehensive unit, integration, and property-based tests achieving >90% line coverage for its logic.
3.  **Fully Documented**: Its public API is documented with Doxygen-style comments, and its role is explained in the main `/docs/`.
4.  **No TODOs**: Contains no remaining `// TODO:`, `// FIXME:`, or equivalent markers in its source code.
5.  **Clean CI**: It builds and passes all tests cleanly in the CI pipeline.

**v1.0 Release Checklist**:
*   [ ] T81Lang compiler implements the full v0.2 language specification.
*   [ ] HanoiVM correctly executes all TISC opcodes defined in the spec, including fault conditions.
*   [ ] Axion Kernel can enforce basic determinism (e.g., trap on overflow, bound recursion).
*   [ ] CanonFS provides a stable in-memory and on-disk storage implementation.
*   [ ] Public C++ API in `/include/t81/` is stable and documented.
*   [ ] A C API exists and is stable for basic interoperability.
*   [ ] The project provides a CLI tool for compiling and running T81Lang files.
*   [ ] Documentation includes a "Getting Started" guide, language reference, and architectural overview.
*   [ ] The release process is automated via GitHub Actions.

## 2. Architecture Map (Current vs Intended)

The intended architecture remains a clean, layered stack:

```mermaid
graph TD
    A[T81Lang Source Code (.t81)] --> B(Compiler Frontend);
    B --> C{TISC Intermediate Representation};
    C --> D(Compiler Backend / Emitter);
    D --> E[TISC Binary Program];
    E --> F(HanoiVM);
    F -- Interacts with --> G(Axion Kernel);
    F -- Uses --> H(Core Libraries: Tensors, BigInt);
    F -- Uses --> I(CanonFS for Storage);

    subgraph "T81 Compiler"
        B; C; D;
    end

    subgraph "T81 Runtime"
        F; G; H; I;
    end
```

## 3. Workstream Breakdown (Compiler-First Priority)

### 3.1 [P0] Workstream: T81Lang Compiler Implementation

*   **Description**: Fully implement the C++20 compiler toolchain to match the `t81lang-spec.md`. **This is the highest priority and the critical path to v1.0.**
*   **Current State**: The compiler can parse a subset of the language including `let`, `if`, `while`, functions, and basic expressions. A basic IR generator exists. It does not perform semantic analysis or type checking.
*   **v1.0 Goal**: A compiler that correctly parses, type-checks, and emits TISC for the entire T81Lang v0.2 feature set, with robust error reporting.
*   **Flagship Feature for Next Cycle: `Option[T]` and `Result[T, E]`**: We will focus on implementing full, end-to-end support for `Option` and `Result` types. This is an ideal "spine" feature because it touches all parts of the compiler (parsing, type checking, code generation) and the VM already has the necessary opcodes, allowing for a complete vertical slice of functionality.

*   **Immediate Next Steps**:
    1.  **[M] Compiler: Implement Semantic Analysis Pass**: Create the foundational semantic analysis pass that traverses the AST. Initially, it will resolve symbols and check for basic errors. This pass will be the home for all future type-checking logic.
    2.  **[L] Compiler: Implement `Option/Result` Type Checking**: Within the new semantic analysis pass, implement the type-checking rules for `Option[T]` and `Result[T, E]`. This includes checking for correct instantiation and usage in `let` bindings and function signatures.
    3.  **[M] End-to-End Test for `Option/Result`**: Create a new, dedicated end-to-end test that defines a function returning an `Option`, calls it, and verifies the result. This test will initially fail but will serve as the driver for the implementation work.

*   **Full Task List for v1.0**:
    *   **[XL] Semantic Analysis & Type Checking**: Build the dedicated semantic analysis pass. Enforce all type rules from the spec, including numeric widening, structural types (`Option/Result`), and generic instantiation.
    *   **[L] Parser Expansion**: Implement the remaining T81Lang grammar, primarily `loop` and `match`.
    *   **[L] Control Flow Implementation**: Implement lowering for `loop` and `match` expressions into TISC conditional jumps and labels.
    *   **[M] Error Reporting**: Implement a robust error reporting system that provides clear, actionable error messages with line and column numbers for both parsing and type errors.

### 3.2 [P1] Workstream: HanoiVM & TISC Runtime

*   **Description**: Ensure the virtual machine is a correct, efficient, and robust implementation of the TISC specification, fully integrated with the Axion Kernel.
*   **Current State**: A functional and feature-rich VM exists. It supports a wide range of opcodes, including advanced ones for `Option`, `Result`, and Tensors.
*   **v1.0 Goal**: A fully compliant, well-tested, and observable TISC runtime that seamlessly executes the compiler's output.
*   **Key Tasks**:
    *   **[L] Axion Kernel Integration**: Integrate the specified Axion hooks (`AXREAD`, `AXSET`, etc.) into the VM's main dispatch loop. The VM must yield to the kernel at these points.
    *   **[M] VM Hardening**: Improve fault handling. Ensure all illegal operations (e.g., division by zero, out-of-bounds memory access) result in deterministic, specified faults. Add extensive negative tests for the VM.
    *   **[M] Performance Profiling**: Add basic instrumentation to the VM to measure execution performance and identify instruction hotspots.

### 3.3 [P2] Workstream: Axion Kernel & Safety

*   **Description**: Transform the Axion Kernel from a conceptual stub into a functional safety supervisor.
*   **Current State**: A placeholder `src/axion/engine.cpp` exists that allows all operations by default.
*   **v1.0 Goal**: An Axion Kernel that can enforce basic determinism policies on a running HanoiVM instance.
*   **Key Tasks**:
    *   **[M] Define the VM-Axion ABI**: Formalize the API between the HanoiVM and the Axion Kernel.
    *   **[L] Implement Core Policies**: Implement the first set of safety policies (Recursion Depth Limiter, Instruction Counter, Overflow Trap).
    *   **[S] Configuration**: Allow policies (like recursion depth) to be configured when the VM is initialized.

### 3.4 [P2] Workstream: Tooling, Documentation, & Onboarding

*   **Description**: Improve the developer experience, provide clear documentation, and create the necessary tooling to make the project accessible and usable.
*   **Current State**: Documentation exists but is heavily focused on specification. Doxygen is configured but API comments are sparse. There is no user-facing CLI.
*   **v1.0 Goal**: A project that is easy to build, learn, and contribute to.
*   **Key Tasks**:
    *   **[M] Create a `t81` CLI Tool**: Build a user-facing command-line tool (`t81 compile`, `t81 run`, `t81 check`).
    *   **[L] Write Comprehensive Documentation**: Create a "Getting Started" guide, a T81Lang language reference, and fully document the public C++ API.
    *   **[S] Improve CI**: Add CI jobs for documentation linting and checking for TODOs.

## 4. Milestones & Phases (Revised)

### 4.1 Phase 1 – Compiler Feature Completeness (4-8 months)

*   **Goals**: Bring the T81Lang compiler to full spec compliance for all language features, focusing on the type system and control flow. Solidify the end-to-end test suite for the compiler.
*   **Entry Criteria**: Project is in its current state (Nov 2025).
*   **Exit Criteria**:
    *   The T81Lang compiler can parse and **type-check** the entire grammar from `spec/t81lang-spec.md`.
    *   End-to-end tests exist for every major language feature (`loop`, `match`, functions, `Option/Result`).
    *   The compiler produces high-quality error messages for type mismatches and syntax errors.
    *   A basic `t81` CLI tool exists for running the compiler and VM.
*   **Key Tasks**:
    *   Semantic Analysis & Type Checking [XL]
    *   Parser Expansion [L]
    *   Control Flow Implementation [L]
    *   Error Reporting [M]
    *   Create `t81` CLI [M]

### 4.2 Phase 2 – Hardening, Optimization, and Observability (3-6 months)

*   **Goals**: Integrate the Axion kernel, harden the system against errors, and improve performance and tooling.
*   **Entry Criteria**: Phase 1 is complete.
*   **Exit Criteria**:
    *   The Axion Kernel is integrated with the VM and enforces recursion and instruction count limits.
    *   The test suite includes property-based tests for the compiler and VM.
    *   The VM has been profiled and obvious performance bottlenecks have been addressed.
*   **Key Tasks**: Axion Kernel Integration [L], Implement Core Policies [L], Performance Profiling [M].

### 4.3 Phase 3 – Public v1.0 Release Readiness (1-2 months)

*   **Goals**: Finalize documentation, tutorials, and release infrastructure. Polish the project for public consumption.
*   **Entry Criteria**: Phase 2 is complete.
*   **Exit Criteria**:
    *   All items in the "Target v1.0 Definition" are complete.
    *   The "Getting Started" guide is complete and has been validated by a new developer.
*   **Key Tasks**: Write Comprehensive Documentation (Guides part) [L], Improve CI [S].

## 5. Testing & Quality Strategy

### 5.1 Current Test Coverage and Gaps

The project has a solid foundation of tests. However, significant gaps exist:
*   **Type Checker**: As this component doesn't fully exist, it has no tests. This is the largest testing gap.
*   **Compiler Error Paths**: No tests exist for verifying that the compiler *correctly rejects* invalid code.
*   **Property-Based Testing**: The complexity of the compiler and VM would benefit greatly from property-based tests.
*   **Axion Policies**: The Axion kernel has no tests.

### 5.2 Testing Priorities per Workstream

*   **T81Lang Compiler**:
    1.  **Robustness (Negative Tests)**: For every feature, add tests that ensure the compiler fails with a clear error message on invalid input. This is a top priority.
    2.  **Correctness (Positive Tests)**: For every feature added, create end-to-end tests that compile and run valid code, asserting the correct output.
    3.  **Fuzzing**: Create a property-based test that generates random (but syntactically valid) ASTs and ensures the type checker and IR generator don't crash.
*   **HanoiVM**:
    1.  **Instruction Tests**: Continue adding tests for each opcode in isolation.
    2.  **Fault Tests**: Add tests that deliberately trigger VM faults (div by zero, invalid opcode, etc.) and verify the VM behaves as specified.
*   **Axion Kernel**:
    1.  **Policy Tests**: Write tests that configure a policy (e.g., max recursion depth of 5), run a program that violates it, and assert that the Axion Kernel correctly traps the VM.

## 6. Contributor Guide for Roadmap Execution

### 6.1 “If You’re New, Start Here”

1.  Read the `README.md` and the `spec/t81-overview.md`.
2.  Build and test the project using the standard commands from `AGENTS.md`.
3.  The most impactful area for new contributions is the **T81Lang Compiler**. See the "Immediate Next Steps" in section 3.1. Tackling the end-to-end test for `Option/Result` would be a fantastic first major contribution.

### 6.2 Good First Issues vs Advanced Tasks

*   **Good First Issues**:
    *   Add more unit tests for existing data types.
    *   Add more "negative" test cases that ensure the parser rejects invalid syntax.
    *   Improve Doxygen comments on public headers in `/include/t81/`.
*   **Advanced Tasks**:
    *   Implementing the type checker (Workstream 3.1, Task: Semantic Analysis & Type Checking [XL]).
    *   Designing the VM-Axion ABI (Workstream 3.3, Task: Define the VM-Axion ABI [M]).
    *   Implementing the `match` expression in the compiler (Workstream 3.1, Task: Control Flow Implementation [L]).

## 7. Open Questions & Decision Log

*   **Assumption**: This roadmap assumes a focus on compiler development as the primary blocker for v1.0. Other workstreams are secondary until the compiler is feature-complete.
*   **Assumption**: The timeline estimates (in months) are rough guides and assume a small, focused team. They will be updated as work progresses.
*   **Open Question (Memory Model)**: The spec mentions the T81VM memory model, but its precise layout needs to be more rigorously defined as the compiler starts handling more complex types. **Next Step**: Create an RFC that details the v1.0 memory model.
*   **Open Question (Standard Library)**: What does the T81Lang standard library look like for v1.0? **Next Step**: Propose a minimal standard library in an RFC.
