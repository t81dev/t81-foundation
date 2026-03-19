# T81 Foundation Documentation Hub

Welcome to the central documentation hub for the T81 Foundation. This site provides the technical specifications, architectural diagrams, developer guides, and status reports needed to understand and contribute to the project.

______________________________________________________________________

## 1. Getting Started

If you are new to the project, start here. These documents provide the high-level overview and practical steps needed to get started with the C++ codebase.

- **[C++ Quickstart Guide](user-guide/getting-started/cpp-quickstart.md)**
  - A hands-on guide to cloning, building, and running the core tests and demos. The fastest way to get the code working.

- **[System Status Report](reference/system-status.md)**
  - Stable pointer to current status surfaces under `docs/status/` and `docs/reference/STATUS.md`.

- **[Architecture Overview](architecture/OVERVIEW.md)**
  - Canonical architecture summary with layer boundaries, maturity labels, and evidence links.

- **[AI Quickstart Guide](user-guide/getting-started/ai-quickstart.md)**
  - Practical entry point for the governed AI and model-integration workflows that are currently documented on the live user-guide surface.

______________________________________________________________________

## 2. Core Concepts & Specifications

These documents are the "constitution" of the T81 Foundation. They define the normative behavior of every component in the stack.

- **[Master Specification Index](../spec/index.md)**
  - The root index that links to all formal specification documents (TISC ISA, VM, T81Lang, Data Types, etc.).

- **[Project Control Center](status/PROJECT_CONTROL_CENTER.md)**
  - At-a-glance program governance, blockers, and next decision points.

- **[Design Principles](explanation/DESIGN.md)**
  - The core design philosophy, including balanced ternary, spec-first development, and deterministic semantics.
- **[Runtime Semantics Boundary](explanation/runtime-semantics-boundary.md)**
  - Defines ownership boundaries between normative semantics in this repo and executable compatibility in `t81-vm`.
- **[Terminology Alignment Notes](process/policies/terminology-alignment.md)**
  - Canonical term map aligned with `duotronic-whitepaper` and `t81-docs`.

- **[T81Lang Stdlib Reference](user-guide/reference/T81LANG_STDLIB_REFERENCE.md)**
  - Current reference for standard library surfaces, including tensor-related modules and language-level contracts.

______________________________________________________________________

## 3. Developer Guides & API

For contributors looking to modify the codebase, these resources provide detailed information.

- **[Guide: Adding a Language Feature](developer-guide/contributing/adding-a-language-feature.md)**
  - The lifecycle of a T81Lang feature, from lexer to IR generator.

- **[CLI User Manual](user-guide/reference/cli-user-manual.md)**
  - Command and operator reference for the shipped `t81` binary.

- **[Axion Policy Manual](user-guide/tutorials/axion-policy-manual.md)**
  - The current live guide for policy-enforced execution, auditing, and secure operational workflows.
- **[llama.cpp Integration Guide](user-guide/tutorials/llama_cpp_integration_guide.md)**
  - The current live guide for weight/model integration and governed llama.cpp workflows.
- **[Guide: Data Types Overview](developer-guide/internals/data-types-overview.md)**
  - A runnable sample that exercises primitive and structural data types inside `examples/data_types.t81`, plus dedicated high-rank tensor and graph demos for exploring multidimensional indexing behaviors.
- **[Benchmark Report](../benchmarks/results/archive/benchmarks.md)**
  - Auto-generated archive of T81/binary throughput/latency comparisons produced by `./build/t81 internal benchmark`.
- **[Benchmark Highlights (README)](../README.md#benchmark-highlights)**
  - Summary badges and table excerpted from the latest `docs/../benchmarks/results/archive/benchmarks.md` results so visitors see at-a-glance which families currently lead.

- **[Guide: VM Opcodes](developer-guide/internals/vm-opcodes.md)**
  - The process for extending the virtual machine with new instructions.
- **[Axion Trace Guide](developer-guide/internals/axion-trace.md)**
  - Current trace/audit workflow for canonical `verdict.reason` strings and policy-facing runtime evidence.

- **[C++ API overview](user-guide/reference/public-api-overview.md)** – describes the canonical headers and the `t81::v1` surface; run `cmake --build build --target docs` to generate the Doxygen HTML under `build/api/html/index.html`.
  - Auto-generated, detailed reference for every class and method in the source code. *(Run `cmake --build build --target docs` to generate).*

- **Archived Guides**
  - Historical guide material remains under `docs/records/archive/temporal-guides/` for reference, but the links above are the maintained entry points for current workflows.

______________________________________________________________________

## 4. Project Governance & Contribution

- **[CONTRIBUTING.md](../CONTRIBUTING.md)**
  - The rules and guidelines for contributing code, including the RFC process for proposing changes.

- **[ROADMAP.md](process/roadmaps-plans/ROADMAP.md)**
  - The high-level plan and priorities for the project's development.
