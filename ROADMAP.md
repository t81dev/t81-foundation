# Project Roadmap

## 1. Overview

### 1.1 Vision and Scope

**Vision**: T81 Foundation is building a post-binary computing stack that treats “thinking” as a first-class concern. It uses ternary logic (−1, 0, +1) and base-81 encodings to create a safer, more expressive foundation for advanced AI systems. The goal is a civilization-grade platform where AI workloads can run deterministically, transparently, and auditable from the instruction set to cognitive-level behavior.

**Scope for v1.0**: The v1.0 release will deliver a stable, documented, and production-ready implementation of the core T81 stack. This includes the T81Lang compiler, the TISC instruction set, the HanoiVM virtual machine, foundational data types (BigInt, Fraction, Tensor), and the baseline Axion Kernel for determinism verification. The target audience for v1.0 is systems developers and AI/PL researchers.

### 1.2 Current Maturity Level (Honest Status Snapshot)

The project is at a **late-alpha / early-beta stage**.

**Strengths**:
*   **Clear Vision & Specification**: The project's goals are exceptionally well-defined in `/spec/`. The architecture is clear and the core concepts are constitutionally enshrined.
*   **Strong Foundations**: Core data types like `T81BigInt` and the basic VM instruction processing loop are implemented and tested.
*   **Modern C++ Toolchain**: The new compiler frontend (`/src/frontend/`) is a solid starting point, with a lexer, parser, and basic IR generator. End-to-end tests for simple language features exist.
*   **Test-Aware Culture**: There is a significant body of unit and integration tests, and the build system (`CMakeLists.txt`) reflects a commitment to testing.

**Weaknesses**:
*   **Spec vs. Implementation Gap**: The `t81lang-spec.md` is far ahead of the implementation. Key features like `loop`, `match`, generics (`Option[T]`, `Result[T,E]`), purity analysis, and tier annotations are specified but not yet implemented in the compiler.
*   **Incomplete Compiler**: The compiler frontend can only handle basic `let` statements, arithmetic, and `if` conditions. Type checking, error handling, and control flow normalization are minimal.
*   **Fragmented Core Libraries**: The `CMakeLists.txt` reveals many small, somewhat disconnected libraries (`t81`, `t81_io`, `t81_frontend`, etc.). A more unified library structure would improve coherence.
*   **Stubbed Components**: Critical components like the Axion Kernel (`src/axion/engine.cpp`) and CanonFS (`src/canonfs/in_memory_driver.cpp`) are present but are likely minimal stubs.
*   **Legacy Code**: The presence of `/legacy/hanoivm/` and older C-style components (`src/lang/compiler.cpp`) alongside the new C++20 frontend suggests an ongoing, incomplete migration.

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

### 2.1 High-Level Architecture Diagram (described in text)

The intended architecture is a clean, layered stack:

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

### 2.2 Core Components and Responsibilities

*   **T81Lang Compiler (`/src/frontend/`, `/src/tisc/`)**: Parses T81Lang source, performs type checking, and emits TISC binary code. This is the primary user-facing component.
*   **TISC (Ternary Instruction Set Computer)**: The low-level assembly language specified in `spec/tisc-spec.md`. It is the compilation target for T81Lang.
*   **HanoiVM (`/src/vm/`)**: The virtual machine that executes TISC binary programs. It is responsible for the stack, registers, and instruction dispatch.
*   **Axion Kernel (`/src/axion/`)**: The safety and determinism supervisor. It observes the VM to enforce rules about resource usage, recursion depth, and side effects.
*   **Core Data Types (`/src/bigint/`, `/src/tensor/`)**: Implementations of fundamental ternary-native data structures.
*   **CanonFS (`/src/canonfs/`)**: A content-addressed, immutable storage system for deterministic I/O.

### 2.3 Known Gaps / Inconsistencies Between Spec and Code

*   **T81Lang Grammar**: The parser in `src/frontend/parser.cpp` only implements a small subset of the grammar in `spec/t81lang-spec.md`. Missing: `loop`, `match`, full expression hierarchy, generics, `Option/Result`, `fn` definitions with return types.
*   **Type System**: The compiler lacks a dedicated type-checking stage (Stage 3 in the spec). Type handling is likely ad-hoc and incomplete.
*   **IR & Lowering**: The IR generator (`src/frontend/ir_generator.cpp`) is basic. It does not appear to handle SSA, purity analysis, or complex control flow normalization as specified.
*   **VM Opcodes**: The VM likely does not implement opcodes for newer language features like `Option` or `Result` manipulation (`MAKE_OPTION_SOME`, `OPTION_IS_SOME`, etc.).
*   **Axion Integration**: The Axion kernel is a stub. The VM does not appear to have the specified `AXREAD`/`AXSET` hooks integrated into its dispatch loop.
*   **Legacy Code**: `src/lang/compiler.cpp` and `src/lang/parser.cpp` seem to be from an older implementation and are distinct from the new `/src/frontend/` toolchain. This legacy path needs to be fully deprecated and removed.

## 3. Workstream Breakdown

### 3.1 Workstream: T81Lang Compiler Implementation

*   **Description**: Fully implement the C++20 compiler toolchain to match the `t81lang-spec.md`. This is the largest and most critical body of work.
*   **Current State**: A skeleton exists (`lexer`, `parser`, `ir_generator`). It can handle `let`, simple arithmetic, and `if` statements. End-to-end tests confirm this basic functionality.
*   **v1.0 Goal**: A compiler that correctly parses, type-checks, and emits TISC for the entire T81Lang v0.2 feature set, with robust error reporting.
*   **Key Tasks**:
    *   **[L] Parser Expansion**: Implement the full T81Lang grammar, including `loop`, `match`, `fn` declarations, and the generic type syntax `Type[...]`.
    *   **[XL] Semantic Analysis & Type Checking**: Build a dedicated semantic analysis pass that walks the AST. Enforce all type rules from the spec, including numeric widening, structural types (`Option/Result`), and generic instantiation.
    *   **[L] Control Flow Implementation**: Implement lowering for `loop` statements and `match` expressions into TISC conditional jumps and labels.
    *   **[M] Function Calls**: Implement the function calling convention (stack-based argument passing) in the IR generator and binary emitter.
    *   **[M] Error Reporting**: Implement a robust error reporting system that provides clear, actionable error messages with line and column numbers for both parsing and type errors.
    *   **[S] Deprecate Legacy Compiler**: Remove the old `src/lang/compiler.cpp` and `src/lang/parser.cpp` files and update `CMakeLists.txt`.
*   **Dependencies**: HanoiVM must support all required TISC opcodes.
*   **Risks**: The complexity of the type checker is high and may be underestimated.
*   **Suggested Owner Profile**: A senior engineer with deep experience in compiler construction, C++, and language design.

### 3.2 Workstream: HanoiVM & TISC Runtime

*   **Description**: Ensure the virtual machine is a correct, efficient, and robust implementation of the TISC specification, fully integrated with the Axion Kernel.
*   **Current State**: A functional VM exists that can execute a core set of arithmetic and control flow instructions. It is tested at the instruction level.
*   **v1.0 Goal**: A fully compliant, well-tested, and observable TISC runtime.
*   **Key Tasks**:
    *   **[M] Implement Missing Opcodes**: Add VM support for opcodes related to new language features, particularly for `Option[T]` and `Result[T, E]` (`MAKE_OPTION_SOME`, `OPTION_IS_SOME`, etc.).
    *   **[L] Axion Kernel Integration**: Integrate the specified Axion hooks (`AXREAD`, `AXSET`, etc.) into the VM's main dispatch loop. The VM must yield to the kernel at these points.
    *   **[M] VM Hardening**: Improve fault handling. Ensure all illegal operations (e.g., division by zero, out-of-bounds memory access) result in deterministic, specified faults.
    *   **[M] Performance Profiling**: Add basic instrumentation to the VM to measure execution performance and identify instruction hotspots.
*   **Dependencies**: T81Lang Compiler needs these features to test its output.
*   **Risks**: Integrating the Axion kernel without significant performance degradation could be challenging.
*   **Suggested Owner Profile**: A systems programmer with experience in virtual machines, interpreters, and low-level C++.

### 3.3 Workstream: Axion Kernel & Safety

*   **Description**: Transform the Axion Kernel from a conceptual stub into a functional safety supervisor.
*   **Current State**: A placeholder `src/axion/engine.cpp` exists but appears to have no meaningful logic.
*   **v1.0 Goal**: An Axion Kernel that can enforce basic determinism policies on a running HanoiVM instance.
*   **Key Tasks**:
    *   **[M] Define the VM-Axion ABI**: Formalize the API between the HanoiVM and the Axion Kernel. This includes the data passed during hooks and the veto/continue responses from the kernel.
    *   **[L] Implement Core Policies**: Implement the first set of safety policies:
        *   **Recursion Depth Limiter**: Track `CALL` instructions and halt execution if a configurable depth is exceeded.
        *   **Instruction Counter**: Halt execution if a program runs for too many instructions (prevents unbounded loops).
        *   **Overflow Trap**: Verify arithmetic operations and trap on overflow, as required by the spec.
    *   **[S] Configuration**: Allow policies (like recursion depth) to be configured when the VM is initialized.
*   **Dependencies**: HanoiVM must be integrated with Axion hooks.
*   **Risks**: The design of a flexible yet performant policy engine is non-trivial.
*   **Suggested Owner Profile**: An engineer with a background in security, operating systems, or runtime safety.

### 3.4 Workstream: Tooling, Documentation, & Onboarding

*   **Description**: Improve the developer experience, provide clear documentation, and create the necessary tooling to make the project accessible and usable.
*   **Current State**: Documentation exists but is heavily focused on specification. Doxygen is configured but API comments are sparse. There is no user-facing CLI.
*   **v1.0 Goal**: A project that is easy to build, learn, and contribute to, with a polished public-facing presence.
*   **Key Tasks**:
    *   **[M] Create a `t81` CLI Tool**: Build a user-facing command-line tool that can:
        *   `t81 compile <file.t81>`: Compile a file to TISC binary.
        *   `t81 run <file.tisc>`: Execute a TISC binary in the HanoiVM.
        *   `t81 check <file.t81>`: Run the parser and type checker without generating code.
    *   **[L] Write Comprehensive Documentation**:
        *   Create a "Getting Started" guide in `/docs/` that walks a new user through installing, compiling, and running their first T81Lang program.
        *   Write a T81Lang language reference guide.
        *   Fully document the public C++ API in `/include/t81/` using Doxygen comments.
    *   **[S] Improve CI**: Add CI jobs for documentation linting and checking for TODOs.
    *   **[M] Project Website/Book**: Use a static site generator (like mdBook or Sphinx) to present the `/spec/` and `/docs/` content in a polished, user-friendly format.
*   **Dependencies**: Relies on the compiler and VM being stable.
*   **Risks**: Documentation can often be neglected; requires dedicated effort.
*   **Suggested Owner Profile**: A developer advocate or an engineer with strong technical writing skills and a passion for developer experience.

## 4. Milestones & Phases

### 4.1 Phase 0 – Stabilize & Document Current State (1-2 months)

*   **Goals**: Eliminate legacy code, establish a single canonical toolchain, document all existing public APIs, and ensure the current limited feature set is robustly tested.
*   **Entry Criteria**: Project is in its current state.
*   **Exit Criteria**:
    *   Legacy compiler (`src/lang/`) is deleted.
    *   All public functions in `/include/t81/` have Doxygen comments.
    *   Test coverage for the existing `t81_frontend` and `t81_vm` libraries exceeds 80%.
    *   A basic `t81` CLI tool exists for running the current compiler and VM.
*   **Key Tasks**: Deprecate Legacy Compiler [S], Write Comprehensive Documentation (API part) [L], Create `t81` CLI [M].

### 4.2 Phase 1 – Core Feature Completeness (4-8 months)

*   **Goals**: Bring the T81Lang compiler and HanoiVM to full spec compliance for all language features, including control flow, functions, and structural types.
*   **Entry Criteria**: Phase 0 is complete.
*   **Exit Criteria**:
    *   The T81Lang compiler can parse and type-check the entire grammar from `spec/t81lang-spec.md`.
    *   End-to-end tests exist for every major language feature (`loop`, `match`, functions, `Option/Result`).
    *   HanoiVM implements all opcodes required by the compiler.
*   **Key Tasks**: Parser Expansion [L], Semantic Analysis & Type Checking [XL], Control Flow Implementation [L], Function Calls [M], Implement Missing Opcodes [M].

### 4.3 Phase 2 – Hardening, Optimization, and Observability (3-6 months)

*   **Goals**: Integrate the Axion kernel, harden the system against errors, and improve performance and tooling.
*   **Entry Criteria**: Phase 1 is complete.
*   **Exit Criteria**:
    *   The Axion Kernel is integrated with the VM and enforces recursion and instruction count limits.
    *   The compiler provides high-quality, user-friendly error messages for common mistakes.
    *   The VM has been profiled and obvious performance bottlenecks have been addressed.
    *   The test suite includes property-based tests for the compiler and VM.
*   **Key Tasks**: Axion Kernel Integration [L], Implement Core Policies [L], Error Reporting [M], Performance Profiling [M].

### 4.4 Phase 3 – Public v1.0 Release Readiness (1-2 months)

*   **Goals**: Finalize documentation, tutorials, and release infrastructure. Polish the project for public consumption.
*   **Entry Criteria**: Phase 2 is complete.
*   **Exit Criteria**:
    *   All items in the "Target v1.0 Definition" are complete.
    *   The "Getting Started" guide is complete and has been validated by a new developer.
    *   The release process is automated.
*   **Key Tasks**: Write Comprehensive Documentation (Guides part) [L], Improve CI [S], Project Website/Book [M].

## 5. Testing & Quality Strategy

### 5.1 Current Test Coverage and Gaps

The project has a solid foundation of tests, including unit tests for data types and end-to-end tests for the compiler. However, significant gaps exist:
*   **Compiler Error Paths**: No tests seem to exist for verifying that the compiler *correctly rejects* invalid code.
*   **Type Checker**: As this component doesn't fully exist, it has no tests. This is the largest testing gap.
*   **Property-Based Testing**: The complexity of the compiler and VM would benefit greatly from property-based tests (e.g., generating random valid programs and ensuring they don't crash the compiler/VM).
*   **Axion Policies**: The Axion kernel has no tests.

### 5.2 Testing Priorities per Workstream

*   **T81Lang Compiler**:
    1.  **Correctness (Positive Tests)**: For every feature added, create end-to-end tests that compile and run valid code, asserting the correct output.
    2.  **Robustness (Negative Tests)**: For every feature, add tests that ensure the compiler fails with a clear error message on invalid input.
    3.  **Fuzzing**: Create a property-based test that generates random (but syntactically valid) ASTs and ensures the type checker and IR generator don't crash.
*   **HanoiVM**:
    1.  **Instruction Tests**: Continue adding tests for each opcode in isolation.
    2.  **Fault Tests**: Add tests that deliberately trigger VM faults (div by zero, invalid opcode, etc.) and verify the VM behaves as specified.
*   **Axion Kernel**:
    1.  **Policy Tests**: Write tests that configure a policy (e.g., max recursion depth of 5), run a program that violates it, and assert that the Axion Kernel correctly traps the VM.

### 5.3 Non-Functional Requirements

*   **Performance**: While not a primary v1.0 goal, the VM should be performant enough for interactive use. A benchmark suite should be created using `Google Benchmark` or similar, testing hotspots like instruction dispatch and bigint arithmetic.
*   **Safety**: Safety is paramount. The primary validation method is the Axion Kernel integration and its corresponding test suite. All unsafe operations MUST be mediated by Axion.
*   **Reliability**: The compiler and VM must not crash on user input. This will be validated by extensive negative testing and fuzzing.

## 6. Contributor Guide for Roadmap Execution

### 6.1 “If You’re New, Start Here”

1.  Read the `README.md` and the `spec/t81-overview.md`.
2.  Build and test the project using the following commands from the root directory:
    ```bash
    cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
    cmake --build build --parallel
    ctest --test-dir build --output-on-failure
    ```
3.  Run the `e2e_let_statement_test` and `e2e_arithmetic_test` executables from the `build/` directory to see the current compiler in action.
4.  A great place to start contributing is by adding more tests or improving documentation. Look for tasks tagged **[S]** or **[M]** in the **Tooling, Documentation, & Onboarding** workstream.

### 6.2 Good First Issues vs Advanced Tasks

*   **Good First Issues**:
    *   Add more unit tests for existing data types.
    *   Add more "negative" test cases that ensure the parser rejects invalid syntax.
    *   Improve Doxygen comments on public headers in `/include/t81/`.
    *   Add a new instruction-level test to the HanoiVM.
*   **Advanced Tasks**:
    *   Implementing the type checker (Workstream 3.1, Task: Semantic Analysis & Type Checking [XL]).
    *   Designing the VM-Axion ABI (Workstream 3.3, Task: Define the VM-Axion ABI [M]).
    *   Implementing the `match` expression in the compiler (Workstream 3.1, Task: Control Flow Implementation [L]).

### 6.3 Coding Standards / Architectural Invariants That MUST Be Respected

*   **Spec is Law**: All implementations must strictly adhere to the semantics defined in the `/spec/` directory.
*   **Determinism is Sacred**: No feature may introduce nondeterminism. This means no reliance on external state, wall-clock time, or unseeded randomness in the core stack.
*   **Follow the Build**: All new code must be integrated into the CMake build system, and all tests must be runnable via `ctest`.
*   **Modern C++**: Use C++20 features where appropriate. Adhere to the style of the existing modern codebase (`/src/frontend/`). Avoid raw pointers and manual memory management.

### 6.4 How to Propose Changes

Follow the process defined in `CONTRIBUTING.md`:
*   For implementation work, create a pull request with a clear description, linking it to a task in this roadmap. Ensure all CI checks pass.
*   For changes that would alter the specification, a formal RFC must be submitted in `/spec/rfcs/`. This is for significant changes, not for implementing the existing spec.

## 7. Open Questions & Decision Log

### 7.1 Known Open Design Questions

*   **Memory Model**: The spec mentions the T81VM memory model, but its precise layout (e.g., heap vs. stack allocation for composite types) needs to be more rigorously defined as the compiler starts handling more complex types. **Next Step**: Create an RFC that details the v1.0 memory model.
*   **Standard Library**: What does the T81Lang standard library look like for v1.0? What built-in functions should be available? **Next Step**: Propose a minimal standard library in an RFC, focusing on I/O and basic data structure manipulation.
*   **Foreign Function Interface (FFI)**: How will T81Lang interoperate with C/C++ or other languages? This is likely out of scope for v1.0 but should be considered in the design of the calling convention. **Next Step**: Defer until after v1.0, but keep the C API workstream in mind.

### 7.2 Assumptions You Had to Make While Writing This Roadmap

*   **Team Capacity**: This roadmap assumes a small team of experienced systems engineers (2-4 people) working on this over the next 6-18 months. The timeline estimates will need adjustment based on actual team size and experience.
*   **Priority of C++**: This roadmap prioritizes the C++ toolchain as the one true path to v1.0, as implied by `AGENTS.md`. Other language bindings (Rust, WASM) are considered post-v1.0 work.
*   **Stability of Spec**: It is assumed that the core specifications for v1.0 are stable and will not undergo major revisions, allowing the implementation to proceed with a fixed target.

### 7.3 Areas Where Specification Is Missing or Ambiguous

*   **Error Handling Semantics**: While the spec mentions `Result[T, E]`, the language-level semantics for error handling (e.g., a `try` or `?` operator) are not yet defined. For v1.0, `match` will be sufficient, but this is a future language evolution point.
*   **Module System**: The spec mentions imports but does not define a module or package management system. For v1.0, we will assume a single-file model, which is sufficient for initial implementation. This will need to be specified for multi-file projects.
*   **I/O Model**: How does a T81Lang program perform I/O? The spec mentions "VM I/O channels" but does not define them. **Next Step**: A concrete I/O model (e.g., via special Axion hooks or mapped memory regions) needs to be proposed in an RFC.
