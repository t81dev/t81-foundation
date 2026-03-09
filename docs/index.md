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

- **[Researcher's Guide](records/archive/temporal-guides/how-to/research-guide.md)**
  - An in-depth exploration of the mathematical foundations of balanced ternary and the five cognitive tiers of execution.

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

- **[Tensor Library Guide](records/archive/temporal-guides/how-to/tensor-guide.md)**
  - An in-depth guide to the concepts and API of the T81 tensor library.

______________________________________________________________________

## 3. Developer Guides & API

For contributors looking to modify the codebase, these resources provide detailed information.

- **[Guide: Adding a Language Feature](developer-guide/contributing/adding-a-language-feature.md)**
  - The lifecycle of a T81Lang feature, from lexer to IR generator.

- **[CLI User Manual](user-guide/reference/cli-user-manual.md)**
  - Command and operator reference for the shipped `t81` binary.

- **[Guide: Match Expression Demo](records/archive/temporal-guides/guides/match-example.md)**
  - A live example that compiles and runs an `Option`/`Result` match through the CLI and HanoiVM.

- **[Tutorial: Secure Deployment](records/archive/temporal-guides/guides/secure-deployment-tutorial.md)**
  - An end-to-end guide to building, securing with Axion policies, and auditing T81 applications.
- **[Guide: Weight & Model Integration](records/archive/temporal-guides/guides/weights-integration.md)**
  - How `t81 weights load`, the new `weights.load("<tensor>")` builtin, and the HanoiVM `WeightsLoad` opcode cooperate to keep `.t81w` tensors zero-copy inside the interpreter.
- **[Guide: Data Types Overview](developer-guide/internals/data-types-overview.md)**
  - A runnable sample that exercises primitive and structural data types inside `examples/data_types.t81`, plus dedicated high-rank tensor and graph demos for exploring multidimensional indexing behaviors.
- **[Demo Gallery](records/archive/temporal-guides/guides/demo-gallery.md)**
  - A quick menu of the match/data-type demos, the `scripts/run-demos.sh` automation, and the IR inspector utility.
- **[Benchmark Report](../benchmarks/results/archive/benchmarks.md)**
  - Auto-generated archive of T81/binary throughput/latency comparisons produced by `./build/t81 internal benchmark`.
- **Benchmark Highlights (README)**(`../README.md#benchmark-highlights`)
-  - Summary badges and table excerpted from the latest `docs/../benchmarks/results/archive/benchmarks.md` results so visitors see at-a-glance which families currently lead.

- **[Guide: VM Opcodes](developer-guide/internals/vm-opcodes.md)**
  - The process for extending the virtual machine with new instructions.
- **[Guide: Setun Bridge](records/archive/temporal-guides/guides/setun-bridge.md)**
  - Setun-style assembly translation to TISC, including label resolution and deterministic diagnostics.
- **[Guide: Runtime Contract Helpers](records/archive/temporal-guides/guides/runtime-contract-helpers.md)**
  - Opcode matrix helpers and VM recursion/contradiction safety counters used by hardened runtime checks.

- **[C++ API overview](user-guide/reference/public-api-overview.md)** – describes the canonical headers and the `t81::v1` surface; run `cmake --build build --target docs` to generate the Doxygen HTML under `build/api/html/index.html`.
  - Auto-generated, detailed reference for every class and method in the source code. *(Run `cmake --build build --target docs` to generate).*

______________________________________________________________________

## 4. Project Governance & Contribution

- **[CONTRIBUTING.md](../CONTRIBUTING.md)**
  - The rules and guidelines for contributing code, including the RFC process for proposing changes.

- **[ROADMAP.md](process/roadmaps-plans/ROADMAP.md)**
  - The high-level plan and priorities for the project's development.
