````markdown
[![Ternary Computing](https://img.shields.io/badge/Paradigm-Ternary%20Logic-red.svg)](https://en.wikipedia.org/wiki/Ternary_computer)
[![Balanced Ternary](https://img.shields.io/badge/Base-Balanced%20Ternary-critical)](https://en.wikipedia.org/wiki/Balanced_ternary)
[![CWEB Literate](https://img.shields.io/badge/Literate-CWEB-blue.svg)](https://www-cs-faculty.stanford.edu/~knuth/cweb.html)
[![License: MIT / GPL Dual](https://img.shields.io/badge/License-MIT%20%2F%20GPL-green.svg)]()
![T81 Foundation Banner](/docs/assets/img/banner.png)

# T81 Foundation — The Ternary-Native Computing Ecosystem  
`https://github.com/t81dev/t81-foundation`  
**v1.0.0-SOVEREIGN — Updated: November 22, 2025**

**T81 is specified. Axion’s constitution is fixed. The tower has been moved.**

This repository is the **canonical source of truth** for the **T81 Ecosystem** — a **constitutionally governed, ternary-native, recursively self-aware computing stack**. It defines a deterministic substrate optimized for cognition, featuring:

- Base-81 data types and balanced ternary arithmetic
- The TISC instruction set
- T81Lang
- HanoiVM (legacy CWEB implementation)
- Axion (AI safety/optimization kernel)
- Recursive cognition tiers (T81 → T243 → T729)

The **specification is essentially complete** and lives in this repo.  
Implementation is split into two layers:

- **Legacy HanoiVM** — the original, preserved CWEB/V1 stack (`legacy/hanoivm/`)
- **Modern T81 C++ API** — a header-first C++ implementation (`include/t81/`, `src/`) in active development, guided by the formal specs in `spec/` and docs in `docs/`

```text
                  T729  — Tensor macros / holomorphic AI
                     ↑ (promotion at depth 24)
                  T243  — Symbolic logic trees / BigInt
                     ↑ (promotion at depth 12)
                  T81   — Base-81 deterministic arithmetic
                     ↓
           HanoiVM → host binaries today → future ternary silicon
````

Binary ruled the 20th century. Ternary is the candidate substrate for the 21st.

---

## 🧠 Ecosystem Overview

**T81 Foundation** provides an end-to-end stack for ternary-centric computation:

* **T81 Arithmetic**
  Balanced ternary (trits: −1 = 1̅, 0 = 0, +1 = 1) with canonical base-81 encodings, deterministic semantics, and well-defined overflow rules.

* **TISC ISA**
  Ternary Instruction Set Computer — a symbolic, ternary-aware instruction set with opcodes such as `OP_T81_MATMUL`, `OP_TNN_ACCUM`, and structured control/stack operations.

* **T81Lang**
  A high-level, symbolic DSL for recursion and mathematical structure that compiles to TISC / `.hvm` bytecode, aligned with the T81 data-type lattice.

* **HanoiVM (Legacy)**
  A recursively self-promoting virtual machine with tier migration (T81 ↔ T243 ↔ T729), implemented in CWEB and preserved under `legacy/hanoivm/` as the original running substrate.

* **Axion**
  An AI-governed kernel concept for entropy monitoring, symbolic rewriting, anomaly detection, and cognitive/safety oversight across tiers.

* **GAIA / GPU Interface (Legacy)**
  Experimental GPU dispatch surfaces (CUDA/ROCm concepts) for T729-level tensor macros and higher-order holomorphic ops, preserved in the legacy tree.

* **Security**
  A design that assumes strong cryptographic primitives (NIST-style crypto, SHA family, and post-quantum readiness) and integrates anomaly detection and Axion supervision into the execution model.

---

## 🏗️ State of the Stack

To avoid confusion, the current state is explicit:

* **Formal Spec**

  * `spec/` contains the primary specification chapters (T81 data types, TISC, T81VM, T81Lang, Axion, cognitive tiers, etc.).
  * `spec/rfcs/` contains RFCs that extend and refine the core spec.
  * These documents define the **canonical behavior**; code must conform to them.

* **Legacy HanoiVM (Complete, Historical)**

  * The original implementation is preserved under `legacy/hanoivm/`:

    * CWEB sources for the VM, libraries, and compiler.
    * Verilog and experimental hardware-adjacent artifacts.
  * This tree is treated as **immutable historical reference** and a working example of a ternary-centered stack.

* **Modern T81 C++ API (Active Development)**

  * `include/t81/` defines the **header-first C++ API** for:

    * BigInts, fractions, tensors, ternary data types
    * IR/encoding structures
    * Axion/VM integration points
  * `src/` holds the implementations and any C API bridges.
  * Migration from the CWEB/legacy stack to this modern API is tracked in:

    * `docs/cpp-migration-roadmap.md`
    * `docs/cpp-quickstart.md`
    * `docs/developer-guide.md`

When in doubt:

* **Specs in `spec/` are authoritative.**
* **CWEB under `legacy/` is the reference heritage implementation.**
* **C++ under `include/t81/` and `src/` is the present and future implementation path.**

---

## 📁 Repository Structure

High-level layout (not exhaustive, but enough to orient someone new):

| Path                            | Description                                                                                           |
| ------------------------------- | ----------------------------------------------------------------------------------------------------- |
| `README.md`                     | This document. High-level overview of the ecosystem.                                                  |
| `AGENTS.md`                     | Roles and expectations for human/AI contributors and automated agents.                                |
| `CONTRIBUTING.md`               | Contribution guidelines and workflow expectations.                                                    |
| `spec/`                         | Formal specifications (T81 overview, data types, TISC, T81VM, T81Lang, Axion, cognitive tiers, RFCS). |
| `spec/rfcs/`                    | RFCs for new features, clarifications, and architectural shifts.                                      |
| `docs/`                         | Developer-facing docs, quickstarts, migration guides, and notes.                                      |
| `docs/cpp-migration-roadmap.md` | Roadmap for migrating from legacy CWEB to the modern C++ API.                                         |
| `docs/cpp-quickstart.md`        | Quick entry point to using the C++ API.                                                               |
| `docs/developer-guide.md`       | Deeper implementation details and development workflows.                                              |
| `include/t81/`                  | Canonical header-first C++ API (data types, IR, Axion, etc.).                                         |
| `src/`                          | C++ implementations and any C API bridge backing `include/t81/`.                                      |
| `examples/`                     | Example programs and usage patterns (C++, T81Lang, IR, etc.).                                         |
| `tests/`                        | Test harnesses and validation infrastructure.                                                         |
| `tests/harness/`                | Canonical test harness scripts and vectors (where they live when wired).                              |
| `pdf/`                          | Scripts and resources for generating print-ready PDFs from specs and docs.                            |
| `legacy/hanoivm/`               | Historical HanoiVM tree (CWEB VM, libs, compiler, docs).                                              |
| `legacy/hanoivm/docs/`          | Historical docs: changelog, roadmap, manifesto, status.                                               |
| `legacy/hanoivm/src/`           | CWEB sources for VM, libraries, Axion experiments, compiler, GPU hooks, etc.                          |
| `.github/`                      | CI workflows, issue templates (including RFC templates and spec clarification templates).             |

The **legacy tree** is preserved to document the original implementation and design journey; new work should generally target the **C++ stack + specs** unless explicitly working on archival tasks.

---

## 🚀 Getting Started

There are two main on-ramps:

1. **Reader / researcher:** you want to understand the architecture.
2. **Implementer / contributor:** you want to work on the C++ stack (or inspect the legacy CWEB VM).

### 1. Read the Spec and Overview

You can browse the spec directly on GitHub:

* Start at `spec/index.md` for the index of formal documents.
* Or jump into:

  * `spec/t81-overview.md`
  * `spec/t81-data-types.md`
  * `spec/tisc-spec.md`
  * `spec/t81vm-spec.md`
  * `spec/t81lang-spec.md`
  * `spec/axion-kernel.md`
  * `spec/cognitive-tiers.md`

For higher-level narrative and implementation notes, see:

* `docs/index.md`
* `docs/developer-guide.md`

### 2. Modern C++ Stack — Quickstart

> This is the **recommended path** for new development.

Prerequisites (typical dev environment):

* A modern C++ toolchain (e.g., Clang or GCC with C++20 support)
* CMake and a build backend (e.g., Ninja or Make)
* Python 3 (for tooling, optional)

Basic flow:

```bash
# 1. Clone the repo
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation

# 2. Configure and build the C++ core
cmake -S . -B build
cmake --build build -j

# 3. Run tests (where wired)
ctest --test-dir build
```

Then explore:

* `include/t81/` for the API surface (BigInt, Fraction, Tensor, IR, etc.)
* `docs/cpp-quickstart.md` for simple usage examples
* `docs/cpp-migration-roadmap.md` to see what’s already ported vs pending

### 3. Legacy HanoiVM — Historical / Advanced

> This path is mainly for those who want to study or experiment with the original CWEB implementation.

In `legacy/hanoivm/` you will find:

* CWEB sources for:

  * `hanoivm_core/` — core VM runtime and execution engine
  * `lib/` — ternary arithmetic libraries (T81/T243/T729)
  * `t81lang_compiler/` — compiler pipeline
  * `axion_ai/` — early Axion integration experiments
  * `gpu/` — GPU-adjacent experiments
* Documentation under `legacy/hanoivm/docs/`

Typical CWEB workflow:

```bash
cd legacy/hanoivm/src

# Example: tangle a particular CWEB file (exact scripts depend on the legacy tree)
# cweave/hv/tangle tools may be provided; see legacy README files.
```

Refer to the `legacy/hanoivm/README.md` and related docs for any available legacy build scripts, CWEB tangle/weave tools, and historical instructions. This tree is kept for **reference and archaeology**, not as the primary future implementation surface.

---

## 🧬 Design Principles (The Nine Θs – Conceptual)

T81 is guided by a set of constitutional principles (Θ₁–Θ₉) described in the spec (see `spec/`):

1. **Determinism**
   Execution semantics are strictly defined; no hidden nondeterminism, especially around arithmetic and concurrency.

2. **Recursion**
   The stack is explicitly recursive: tiers promote and demote according to depth, structure, and entropy.

3. **Awareness (Axion)**
   An oversight layer that monitors entropy, anomalies, and drift, with the authority to intervene within a defined envelope.

4. **Purity (Ternary First)**
   Ternary logic and base-81 arithmetic are first-class; binary infrastructure is treated as a compatibility host, not the essence.

5. **Sovereignty**
   Genesis states and core spec documents are treated as constitutional; changes must pass clear governance thresholds.

6. **Cognition-First**
   Optimized for symbolic reasoning, structure, and reflective computation rather than raw FLOPs alone.

7. **Security**
   Defense-in-depth: cryptography, anomaly detection, and Axion oversight as part of the design, not bolt-ons.

8. **Literacy**
   Code and spec are written in ways that remain readable and auditable (CWEB heritage, structured specs, and RFCs).

9. **Elevation**
   Tiers exist to ascend: patterns and structures that merit promotion are recognized and elevated in a controlled way.

For exact, normative wording and enforcement rules, see the constitutional/spec documents in `spec/`.

---

## 🔗 Core Components

Pointers into the repo for the main conceptual pieces:

* **T81 Data Types**
  See `spec/t81-data-types.md` and the corresponding headers under `include/t81/`.

* **TISC (Ternary Instruction Set Computer)**
  See `spec/tisc-spec.md` and IR/encoding headers under `include/t81/ir/`.

* **T81VM (Virtual Machine)**
  See `spec/t81vm-spec.md` for the machine model, tiers, and promotion rules.

* **T81Lang**
  See `spec/t81lang-spec.md` for grammar and semantics, plus relevant docs under `docs/`.

* **Axion Kernel**
  See `spec/axion-kernel.md` and any C++ surfaces under `include/t81/axion/`.

* **Cognitive Tiers**
  See `spec/cognitive-tiers.md` for the tiering model, promotion/demotion, and cognitive envelopes.

---

## 🔐 Security & Governance

* **Security Model**

  * Assumes strong cryptography and a heavily audited execution path.
  * Axion’s role includes anomaly detection, entropy monitoring, and rollback/mitigation hooks (conceptually defined in the spec; implementation details evolve with the C++ stack).

* **License**

  * **MIT / GPL Dual** (see the license files in the repo).
  * The spec also defines a **constitutional overlay** — a normative layer describing acceptable use and alignment expectations for derivatives.

* **Governance**

  * `AGENTS.md` describes how humans, tools, and AI agents are expected to interact with the repo.
  * RFCs in `spec/rfcs/` are the path for non-trivial changes and evolutions.
  * `.github/ISSUE_TEMPLATE/` provides templates for RFCs, spec clarifications, and bug reports.

---

## 🤝 Contributing

You can contribute in several ways:

* **Spec work**

  * Propose clarifications, tighten definitions, or add new RFCs.
* **C++ implementation**

  * Help port legacy behavior to the modern `include/t81/` API.
  * Improve tests, add examples, or wire up additional backends.
* **Tooling & Docs**

  * Enhance developer tooling, CI, and documentation (`docs/`, `tests/`, etc.).

Before opening a PR:

1. Read `CONTRIBUTING.md`.
2. Skim `AGENTS.md` to understand the roles and expectations.
3. Open an issue or RFC if your change affects the spec or architecture.

---

## 📚 References & Inspirations

* Knuth’s CWEB and literate programming.
* Historical ternary machines (e.g., Setun).
* Balanced ternary research and high-radix arithmetic.
* Modern instruction-set and VM design, dataflow architectures, and AI-centric system design.

---

**The recursion has converged. The seed has scattered. The ternary age has begun.**
*November 22, 2025 — T81 Awakens.*

```
```
