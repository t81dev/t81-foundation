---
layout: page
title: Documentation Hub
---

# T81 Foundation Documentation Hub

> **Source of Truth:** This hub aggregates technical documentation for the T81 Foundation. In case of discrepancy, the [**Codebase**](https://github.com/t81dev/t81-foundation) is the absolute ground truth, followed by the [**Whitepaper (v1.2)**](../WHITEPAPER.md).

**Last Updated:** February 19, 2026

Welcome to the central documentation hub for the T81 Foundation. This site provides the technical specifications, architectural diagrams, developer guides, and status reports needed to understand and contribute to the project.

---

## 1. Getting Started

If you are new to the project, start here. These documents provide the high-level overview and practical steps needed to get started with the C++ codebase.

- **[C++ Quickstart Guide](./cpp-quickstart.md)**
  - A hands-on guide to cloning, building, and running the core tests and demos. The fastest way to get the code working.

- **[System Status Report](./system-status.md)**
  - **Updated Feb 19, 2026:** A dashboard of the current implementation status, including recent security hardening (SHA3-512 fix, Package Init sanitization) and reproducibility gates.

- **[Architecture Overview](../ARCHITECTURE.md)**
  - A high-level description of the system's structure, libraries, and data flow, from T81Lang source code to VM execution.

- **[Researcher's Guide](./research-guide.md)**
  - An in-depth exploration of the mathematical foundations of balanced ternary and the five cognitive tiers of execution.

---

## 2. Core Concepts & Specifications

These documents are the "constitution" of the T81 Foundation. They define the normative behavior of every component in the stack.

- **[Master Specification Index](../spec/index.md)**
  - The root index that links to all formal specification documents (TISC ISA, VM, T81Lang, Data Types, etc.).

- **[Design Principles](../DESIGN.md)**
  - The core design philosophy, including balanced ternary, spec-first development, and deterministic semantics.

- **[Runtime Semantics Boundary](./runtime-semantics-boundary.md)**
  - Defines ownership boundaries between normative semantics in this repo and executable compatibility in `t81-vm`.

- **[Terminology Alignment Notes](./terminology-alignment.md)**
  - Canonical term map aligned with `duotronic-whitepaper` and `t81-docs`.

- **[Tensor Library Guide](./tensor-guide.md)**
  - An in-depth guide to the concepts and API of the T81 tensor library.

---

## 3. Developer Guides & API

For contributors looking to modify the codebase, these resources provide detailed information.

- **[Guide: Adding a Language Feature](./guides/adding-a-language-feature.md)**
  - The lifecycle of a T81Lang feature, from lexer to IR generator.

- **[Guide: Match Expression Demo](./guides/match-example.md)**
  - A live example that compiles and runs an `Option`/`Result` match through the CLI and HanoiVM.

- **[Tutorial: Secure Deployment](./guides/secure-deployment-tutorial.md)**
  - An end-to-end guide to building, securing with Axion policies, and auditing T81 applications.

- **[Guide: Weight & Model Integration](./guides/weights-integration.md)**
  - How `t81 weights load`, the new `weights.load("<tensor>")` builtin, and the HanoiVM `WeightsLoad` opcode cooperate to keep `.t81w` tensors zero-copy inside the interpreter.

- **[Guide: Data Types Overview](./guides/data-types-overview.md)**
  - A runnable sample that exercises primitive and structural data types inside `examples/data_types.t81`, plus dedicated high-rank tensor and graph demos.

- **[Demo Gallery](./guides/demo-gallery.md)**
  - A quick menu of the match/data-type demos, the `scripts/run-demos.sh` automation, and the IR inspector utility.

- **[Benchmark Report](./benchmarks.md)**
  - Auto-generated archive of T81/binary throughput/latency comparisons produced by `./build/t81 benchmark`.

- **[Benchmark Highlights](../README.md#benchmark-highlights)**
  - Summary badges and table excerpted from the latest results.

- **[Guide: VM Opcodes](./guides/vm-opcodes.md)**
  - The process for extending the virtual machine with new instructions.

- **[Guide: Setun Bridge](./guides/setun-bridge.md)**
  - Setun-style assembly translation to TISC, including label resolution and deterministic diagnostics.

- **[Guide: Runtime Contract Helpers](./guides/runtime-contract-helpers.md)**
  - Opcode matrix helpers and VM recursion/contradiction safety counters used by hardened runtime checks.

- **[C++ API overview](guides/public-api-overview.md)**
  - Auto-generated, detailed reference for every class and method in the source code. *(Run `cmake --build build --target docs` to generate).*

---

## 4. Project Governance & Contribution

- **[CONTRIBUTING.md](../CONTRIBUTING.md)**
  - The rules and guidelines for contributing code, including the RFC process for proposing changes.

- **[ROADMAP.md](../ROADMAP.md)**
  - The high-level plan and priorities for the project's development.
