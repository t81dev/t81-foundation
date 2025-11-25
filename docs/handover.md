# T81 Foundation: Developer Handover & Progress Report

**Date:** November 25, 2025
**Author:** Jules, AI Software Engineer

## Introduction

This document provides a comprehensive handover for a new developer joining the T81 Foundation project. Its purpose is to detail the project's vision, current status, architecture, development processes, and future roadmap. It is designed to enable a senior engineer to quickly become a productive contributor with minimal verbal walkthrough.

______________________________________________________________________

### 1. Project Overview

- **Purpose:** The `t81-foundation` repository is the home of a "ternary-native, cognition-first computing stack." The project's core thesis is that balanced ternary computing (`-1, 0, +1`) is a superior substrate for artificial intelligence compared to binary, as it natively handles concepts like uncertainty (`unknown`) and provides for more efficient and mathematically pure data representation.

- **Problem Domain:** The project aims to solve problems in the domain of provably safe, deterministic, and highly efficient AI by building a complete, vertically integrated computing ecosystem from the instruction set up to a "self-supervising AI safety kernel" known as Axion.

- **High-Level Architecture:** The system is a five-layer stack, where each layer is defined by a formal, immutable specification.

  ```mermaid
  graph TD
      subgraph "T81 Foundation Stack"
          A[<b>Axion Kernel</b><br/><i>Supervisor & Verifier</i>] --> B[<b>T81Lang</b><br/><i>High-Level Language</i>];
          B --> C[<b>T81VM</b><br/><i>Virtual Machine</i>];
          C --> D[<b>TISC ISA</b><br/><i>Ternary Instruction Set</i>];
          D --> E[<b>Data Types</b><br/><i>Canonical Primitives & Tensors</i>];
      end
      style A fill:#c9daf8
      style B fill:#d9ead3
      style C fill:#fce5cd
      style D fill:#f4cccc
      style E fill:#d0e0e3
  ```

______________________________________________________________________

### 2. Current State

- **Implemented Features:**
  - **Data Types:** The core C++ implementation for balanced ternary arithmetic and base-81 data types is approximately 80% complete and considered production-ready. Some `BigInt` operations are still missing.
  - **TISC ISA & VM:** The TISC instruction set is fully specified. The VM (`HanoiVM`) is partially implemented as an interpreter; a JIT compiler is planned but not started.
  - **T81Lang:** A prototype compiler frontend exists, including a lexer, parser, and basic IR generator. It is in the early stages of development.
  - **Axion Kernel:** The specification is complete, but the implementation is currently a stub that permits all operations.
- **Active Branches:** A static analysis of the repository cannot determine the active feature branches or the full git history. It is recommended to run `git branch -a` and consult the GitHub repository for the latest pull requests.
- **Design Decisions:**
  - **Specification-First:** The project follows a rigorous process where formal specifications (`/spec`) are treated as the "constitution" and must be written or updated before implementation begins.
  - **Modern C++ Core:** The new toolchain is being built in C++23, with a preference for header-only libraries where feasible and a strict prohibition on raw memory management or exceptions in core paths.
  - **Modular Architecture:** The `CMakeLists.txt` reveals a modular design, with functionality separated into libraries like `t81` (core), `t81_frontend`, `t81_tisc`, etc.
- **Dependencies:** The core C++ project is self-contained and has no external library dependencies, which simplifies the build process significantly.

______________________________________________________________________

### 3. Development Process

- **Branching & Git Workflow:** The specifics of the branching strategy are not documented, but the `CONTRIBUTING.md` and `AGENTS.md` files imply a workflow centered on proposing changes via RFCs and pull requests.
- **Testing Setup:** The project uses CTest for its testing framework. A comprehensive suite of unit and integration tests exists in the `/tests` directory. The `AGENTS.md` file mandates that the following commands **must** pass before any code is committed:
  ```bash
  cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
  cmake --build build --parallel
  ctest --test-dir build --output-on-failure
  ```
- **CI/CD & Deployment:** The project has a robust Continuous Integration (CI) pipeline implemented using GitHub Actions (`.github/workflows/ci.yml`).
  - **Triggers:** The CI runs automatically on every push and pull request to the `main` branch.
  - **Validation:** The pipeline performs a comprehensive set of checks:
    1. **Documentation Linting:** Ensures all Markdown files are formatted correctly.
    1. **Broken Link Checking:** Verifies all hyperlinks in the documentation.
    1. **Build & Test:** Compiles the entire C++ project with `clang-18` and runs the full CTest suite.
    1. **API Documentation Generation:** Runs the Doxygen build to ensure the in-code comments can be successfully parsed.
  - **Deployment:** There is no automated deployment process. The project is currently in a research and development phase.

______________________________________________________________________

### 4. Configuration & Environment

- **Configuration:** The project does not require any `.env` or other configuration files for a standard build and test run.
- **Environment Variables:** No specific environment variables are needed.
- **Local Dev Setup:**
  1. Clone the repository: `git clone https://github.com/t81dev/t81-foundation.git`
  1. Navigate to the directory: `cd t81-foundation`
  1. Configure CMake: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
  1. Build the project: `cmake --build build --parallel`
  1. Run the tests: `ctest --test-dir build --output-on-failure`

______________________________________________________________________

### 5. Code Structure & Key Modules

- **Directory Layout:**

  - `/spec/`: The authoritative, immutable specifications for all layers.
  - `/include/t81/`: The public, header-only C++20/23 API.
  - `/src/`: The source code implementations for the C++ libraries.
  - `/tests/`: The CTest-based unit and integration test suite.
  - `/examples/`: Example programs demonstrating usage of the `t81` library.
  - `/legacy/hanoivm/`: A frozen, read-only copy of the original CWEB implementation, used for reference.

- **Key Modules (CMake Libraries):**

  - `t81`: A static library containing the core logic for data types, the legacy VM, and the T81Lang compiler.
  - `t81_frontend`, `t81_tisc`, `t81_vm`, `t81_llvm`: A set of modern, modular libraries for the new C++20 toolchain, separating the compiler and VM into logical components.

- **Key Data Flow:** The primary data flow for the new toolchain is from T81Lang source code to executable VM bytecode. This process is orchestrated by the modular libraries of the modern C++ toolchain.

  ```mermaid
  graph LR
      subgraph "T81Lang Compilation Pipeline"
          A[T81Lang Source<br/><i>(.t81)</i>] -- Lexing --> B{Tokens};
          B -- Parsing --> C[AST<br/><i>(Abstract Syntax Tree)</i>];
          C -- IR Generation --> D[TISC IR<br/><i>(Textual Assembly)</i>];
          D -- Binary Emission --> E[HanoiVM Bytecode<br/><i>(.hvm)</i>];
      end
      style A fill:#d9ead3
      style E fill:#f4cccc
  ```

______________________________________________________________________

### 6. Data & State

- **Data Stores:** The project does not use any external databases. All state is managed in memory or through a custom canonical filesystem layer (`canonfs`).
- **Data Modeling:** The canonical data models are rigorously defined in `spec/t81-data-types.md`. All in-memory and serialized representations must adhere to these specifications.

______________________________________________________________________

### 7. Open Issues / Technical Debt

- **Known Bugs:** This analysis cannot access the project's issue tracker. Please consult the GitHub Issues for a list of known bugs.
- **Technical Debt & Known Gaps:**
  - The `BigInt` implementation is missing some mathematical operations (`pow`, `gcd`).
  - The T81VM is an interpreter only and lacks a planned JIT compiler.
  - The T81Lang compiler is a prototype and requires significant work to become feature-complete.
  - The Axion Kernel is a functional stub.
  - The network stack is specified but not implemented.

______________________________________________________________________

### 8. Backlog & Next Steps

- **Priority Features:** Based on the current state, the next logical priorities are:
  1. Completing the T81Lang compiler and type system.
  1. Hardening the T81VM interpreter and beginning work on the JIT.
  1. Moving the Axion Kernel from a stub to a functional implementation.
- **Backlog:** The `AGENTS.md` file explicitly mentions several desired future enhancements that are considered in-scope:
  - Rust bindings.
  - WASM targets.
  - GPU (CUDA/HIP/ROCm) backends.
  - CI/CD automation.

______________________________________________________________________

### 9. Risks & Challenges

- **Scalability & Complexity:** The project's main challenge is its ambition. Building a completely new, vertically integrated computing stack is a massive undertaking. Success hinges on rigorously maintaining the separation of concerns and adhering to the specifications.
- **Domain Knowledge:** Meaningful contribution requires a deep understanding of compiler design, virtual machine architecture, formal methods, and the principles of ternary computing.
- **Spec vs. Implementation Drift:** The biggest risk is allowing the implementation to diverge from the formal specifications. The "specification-first" process is designed to mitigate this, but it requires constant vigilance.

______________________________________________________________________

### 10. Documentation & Resources

- **Primary Documents:**
  - `README.md`: High-level project overview.
  - `AGENTS.md`: The "operational contract" detailing development rules.
  - `spec/index.md`: The master index for all formal specifications.
  - `docs/system-status.md`: A concise summary of implementation progress.
- **Code Documentation:** The code contains comments, but a formal review and update pass is part of this handover task. Auto-generated documentation (e.g., Doxygen) is a desired future addition.

______________________________________________________________________

### 11. How to Onboard as the New Developer

1. **Read the Constitution:** Read the primary documents in this order: `README.md`, `AGENTS.md`, and `spec/index.md`.
1. **Build and Test:** Follow the local dev setup instructions to build the project and run the test suite. A green test run is the baseline for any change.
1. **Explore the Examples:** Examine the code in `/examples` to see how the core `t81` library is used in practice.
1. **Start with the Frontend:** A good place to start contributing would be the new C++ toolchain, specifically by adding a new language feature to the parser (`src/frontend/parser.cpp`) and a corresponding test case in `tests/cpp/frontend_parser_test.cpp`.

______________________________________________________________________

### 12. Friction Log / Outstanding Questions

- **Incomplete Knowledge:** This static analysis could not determine the CI/CD status, the full backlog from the issue tracker, or the current active development branches.
- **Questions for the Team:**
  - What is the highest immediate priority: T81Lang feature completeness, VM performance/hardening, or Axion kernel implementation?
  - Are there unwritten coding conventions or style guides that supplement the rules in `AGENTS.md`?

______________________________________________________________________

### 13. Appendix / Supporting Materials

- **Glossary:**
  - **TISC:** Ternary Instruction Set Computer. The low-level assembly language of the T81 ecosystem.
  - **Axion:** The AI safety kernel that supervises the VM and enforces determinism.
  - **Canonicalization:** The process of converting data to a standard, unambiguous representation. This is a core principle of the T81 stack.
- **Suggested Diagrams:** To aid understanding, it would be beneficial to create diagrams for:
  - The 5-layer architecture stack.
  - The T81Lang compilation flow from source code to VM bytecode.
