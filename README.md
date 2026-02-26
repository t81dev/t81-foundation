<p align="center">
  <img src="assets/banner.png" alt="T81 Foundation — Deterministic Ternary Architecture" width="100%">
</p>

<p align="center">
  <a href="https://github.com/t81dev/t81-foundation/releases/latest"><img src="https://img.shields.io/github/v/release/t81dev/t81-foundation?style=for-the-badge&label=Latest%20Release&color=blueviolet" alt="Latest Release"></a>
  <a href="https://github.com/t81dev/t81-foundation/actions/workflows/ci.yml"><img src="https://img.shields.io/github/actions/workflow/status/t81dev/t81-foundation/ci.yml?branch=main&style=for-the-badge&logo=github&label=CI" alt="CI Status"></a>
  <a href="https://github.com/t81dev/t81-foundation/blob/main/LICENSE"><img src="https://img.shields.io/badge/License-MIT-yellow.svg?style=for-the-badge" alt="License: MIT"></a>
  <a href="https://en.cppreference.com/w/cpp/23"><img src="https://img.shields.io/badge/Language-C%2B%2B23-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white" alt="Language: C++23"></a>
</p>

<p align="center">
  <strong>The Deterministic Ternary Computing Stack</strong><br>
  <em>Bit-exact reproducibility. Ternary-native logic. Auditable AI governance.</em>
</p>

<p align="center">
  <a href="README.md">English</a> •
  <a href="README.zh-CN.md">简体中文</a> •
  <a href="README.es.md">Español</a> •
  <a href="README.ru.md">Русский</a> •
  <a href="README.pt-BR.md">Português</a>
</p>

---

## What is T81?

**T81** is a sovereign computing stack built from the ground up for **determinism** and **ternary logic**. It eliminates the non-determinism of modern floating-point arithmetic and provides a mathematically rigorous foundation for high-stakes AI, cryptography, and scientific modeling.

Where traditional systems drift across architectures, T81 guarantees that **every instruction, every float, and every tensor operation produces the exact same bit-pattern on every machine, forever.**

### The Core Promise: Verified Determinism

| Feature | The Problem (Binary/IEEE 754) | The T81 Solution |
| :--- | :--- | :--- |
| **Arithmetic** | Floating-point drift across CPU/GPU architectures. | **Deterministic Soft-Float:** Bit-exact math on x86, ARM, & RISC-V. |
| **Logic** | Boolean (True/False) loses nuance. | **Balanced Ternary:** {-1, 0, +1} logic for efficient, drift-free decision trees. |
| **Safety** | AI models are black boxes with no runtime guarantees. | **Axion Kernel:** Enforceable, audit-grade governance policies at the opcode level. |
| **Stability** | Constant breaking changes and dependency hell. | **Frozen Specs:** The TISC ISA and Data Types are immutable standards. |

---

## 🏗️ Architecture

T81 is organized into strict layers of authority and abstraction.

```mermaid
graph TD
    subgraph "Application Layer"
        Lang[T81Lang Source]
        Cognitive[Cognitive Tiers]
    end

    subgraph "Governance Layer"
        Axion[Axion Policy Kernel]
    end

    subgraph "Execution Layer"
        VM[T81VM Interpreter]
        JIT[Trace-JIT (Experimental)]
    end

    subgraph "Foundation Layer (Frozen)"
        ISA[TISC ISA]
        Types[Ternary Data Types]
    end

    Lang --> VM
    Cognitive --> Axion
    VM --> Axion
    Axion --> ISA
    VM --> ISA
    ISA --> Types
```

*   **Foundation Layer:** The "Frozen" core. `T81BigInt`, `T81Float`, and the **TISC** (Ternary Instruction Set Computer) ISA. Changes here require a major version bump.
*   **Execution Layer:** The **T81VM** executes TISC bytecode. It includes a deterministic interpreter and an experimental Trace-JIT that preserves bit-exactness.
*   **Governance Layer:** The **Axion Kernel** intercepts execution to enforce safety policies, resource limits, and ethical guardrails defined in configuration.

---

## 🚀 Quick Start

Build the T81 stack from source in under 60 seconds.

### Prerequisites
*   **CMake** 3.16+
*   **C++ Compiler** supporting C++20/23 (Clang 18+, GCC 14+, MSVC)

### Installation

```bash
# 1. Clone the repository
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation

# 2. Configure and Build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel

# 3. Verify Installation (Runs the Determinism Gate)
python3 scripts/ci/t81lang_repro_gate.py --t81-bin build/t81 --check
```

### Hello World (Ternary Style)

Create a file named `hello.t81`:

```t81
fn main() {
    print("Hello, Deterministic World!");
    let a: trit = 1;
    let b: trit = -1;
    print(a + b); // Outputs "0"
}
```

Compile and run:

```bash
# Compile to TISC bytecode
./build/t81 compile hello.t81 -o hello.tisc

# Execute via VM
./build/t81 run hello.tisc
```

---

## 📚 Documentation

The T81 ecosystem is documented across several authority levels.

| Resource | Description | Authority |
| :--- | :--- | :--- |
| **[The Monograph](book/book-en/README.md)** | The definitive book on T81's philosophy, architecture, and usage. **Start Here.** | High |
| **[Normative Specs](spec/)** | The absolute law. Defines the TISC ISA, Data Types, and VM behavior. | **Absolute** |
| **[Architecture](docs/architecture/OVERVIEW.md)** | The "North Star" document defining system boundaries and invariants. | High |
| **[Status Dashboard](docs/status/PROJECT_CONTROL_CENTER.md)** | Live tracking of system health, active gates, and verified surfaces. | Live |
| **[Governance](docs/governance/)** | Policies on Spec Drift, Release Discipline, and Threat Models. | High |

### Key Topics
*   **[TISC Instruction Set](spec/tisc-spec.md)** - The frozen ISA specification.
*   **[Ternary Data Types](spec/t81-data-types.md)** - Understanding `trit`, `tryte`, and `T81Float`.
*   **[Axion Policy Engine](spec/axion-kernel.md)** - Configuring runtime safety.

---

## 🧩 Components & Status

| Component | Status | Description |
| :--- | :--- | :--- |
| **TISC ISA** | 🧊 **Frozen** | The instruction set is verified and immutable (v1). |
| **Data Types** | 🧊 **Frozen** | Core arithmetic types are stable and bit-exact. |
| **T81VM** | ✅ **Stable** | The interpreter is fully functional and governed. |
| **Axion** | 🚧 **Beta** | Policy engine is active but evolving. |
| **T81Lang** | 🚧 **Beta** | The high-level language compiler is functional. |
| **Trace-JIT** | 🧪 **Experimental** | JIT compilation for speed (opt-in). |
| **Hanoi VM** | 🧪 **Concept** | Recursive distributed compute layer. |

> **Note:** "Frozen" components are contractually guaranteed not to change without a major version bump (e.g., 2.0).

---

## 🤝 Community & Contributing

We welcome contributors who share our passion for rigorous, deterministic systems.

*   **[Contributing Guide](CONTRIBUTING.md):** Read this before sending a PR.
*   **[Code of Conduct](CODE_OF_CONDUCT.md):** We adhere to a strict standard of professional conduct.
*   **[Discussions](https://github.com/t81dev/t81-foundation/discussions):** Ask questions and share ideas.

### The "Repro Gate"
All Pull Requests are automatically checked against the **Reproducibility Gate**. If your change affects the deterministic output of the compiler or VM, the gate will fail. This is a feature, not a bug.

---

## 📄 License

T81 is open-source software licensed under the **[MIT License](LICENSE)**.

Copyright © 2024-2026 T81 Foundation.
