---
layout: page
title: Documentation Hub
---

# T81 Foundation Documentation Hub

Welcome to the central documentation hub for the T81 Foundation. This site provides the technical specifications, architectural diagrams, developer guides, and status reports needed to understand and contribute to the project.

______________________________________________________________________

## 1. Getting Started

If you are new to the project, start here. These documents provide the high-level overview and practical steps needed to get started with the C++ codebase.

- **[C++ Quickstart Guide](./cpp-quickstart.md)**
  - A hands-on guide to cloning, building, and running the core tests and demos. The fastest way to get the code working.

- **[System Status Report](./system-status.md)**
  - A dashboard of the current implementation status of each major component, measured against its formal specification.

- **[Architecture Overview](../ARCHITECTURE.md)**
  - A high-level description of the system's structure, libraries, and data flow, from T81Lang source code to VM execution.

______________________________________________________________________

## 2. Core Concepts & Specifications

These documents are the "constitution" of the T81 Foundation. They define the normative behavior of every component in the stack.

- **[Master Specification Index](../spec/index.md)**
  - The root index that links to all formal specification documents (TISC ISA, VM, T81Lang, Data Types, etc.).

- **[Design Principles](../DESIGN.md)**
  - The core design philosophy, including balanced ternary, spec-first development, and deterministic semantics.

- **[Tensor Library Guide](./tensor-guide.md)**
  - An in-depth guide to the concepts and API of the T81 tensor library.

______________________________________________________________________

## 3. Developer Guides & API

For contributors looking to modify the codebase, these resources provide detailed information.

- **[Guide: Adding a Language Feature](./guides/adding-a-language-feature.md)**
  - The lifecycle of a T81Lang feature, from lexer to IR generator.

- **[Guide: Match Expression Demo](./guides/match-example.md)**
  - A live example that compiles and runs an `Option`/`Result` match through the CLI and HanoiVM.
- **[Guide: Data Types Overview](./guides/data-types-overview.md)**
  - A runnable sample that exercises primitive and structural data types inside `examples/data_types.t81`, plus dedicated high-rank tensor and graph demos for exploring multidimensional indexing behaviors.
- **[Demo Gallery](./guides/demo-gallery.md)**
  - A quick menu of the match/data-type demos, the `scripts/run-demos.sh` automation, and the IR inspector utility.

- **[Guide: VM Opcodes](./guides/vm-opcodes.md)**
  - The process for extending the virtual machine with new instructions.

- **[C++ API Reference (Doxygen)](./api/html/index.html)**
  - Auto-generated, detailed reference for every class and method in the source code. *(Run `cmake --build build --target docs` to generate).*

______________________________________________________________________

## 4. Project Governance & Contribution

- **[CONTRIBUTING.md](../CONTRIBUTING.md)**
  - The rules and guidelines for contributing code, including the RFC process for proposing changes.

- **[ROADMAP.md](../ROADMAP.md)**
  - The high-level plan and priorities for the project's development.
