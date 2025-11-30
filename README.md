# T81 Foundation: The Ternary-Native Computing Stack

<div align="center">
  <br/>
  <img src="docs/assets/img/banner.png" alt="T81 Foundation" width="100%"/>
  <br/><br/>

[![Paradigm: Ternary Computing](https://img.shields.io/badge/Paradigm-Ternary%20Computing-red?style=flat-square)](https://en.wikipedia.org/wiki/Ternary_computer)
[![Design: Specification-First](https://img.shields.io/badge/Design-Specification%20First-blue?style=flat-square)](#)
[![CI Status](https://github.com/t81dev/t81-foundation/actions/workflows/ci.yml/badge.svg)](https://github.com/t81dev/t81-foundation/actions/workflows/ci.yml)
[![Core: C++20](https://img.shields.io/badge/Core-C%2B%2B20-0d1117?style=flat-square&logo=cplusplus)](#)
[![License: MIT/GPL-3.0](https://img.shields.io/badge/License-MIT%20%2F%20GPL--3.0-green?style=flat-square)](LICENSE-MIT)

...

[![Reference Platform: macOS • clang](https://img.shields.io/badge/Reference-macOS%20clang-81D4FA?logo=apple&style=for-the-badge)](https://github.com/t81dev/t81-foundation/actions)
[![Platforms](https://img.shields.io/badge/Platforms-macOS%20•%20Linux%20•%20Windows%20•%20WASM-81D4FA?logo=cplusplus&style=for-the-badge)](https://github.com/t81dev/t81-foundation/actions)

<br/><br/>

</div>

## 1. What is T81?

The T81 Foundation is building a **post-binary computing stack** from the ground up, based on balanced ternary logic (−1, 0, +1). Our goal is to create a deterministic, transparent, and auditable platform for advanced AI systems.

The project is a C++20 implementation of the principles laid out in the `/spec` directory. It includes:
- A suite of **core data types** for balanced ternary arithmetic (`T81Int`, `Fraction`, `T81Float`).
- A multi-dimensional **Tensor library** for numerical computing.
- A prototype **T81Lang compiler** that translates a high-level language to a custom Ternary Instruction Set Computer (TISC) bytecode.
- An interpreter-based **Virtual Machine (HanoiVM)** for executing TISC programs.
- Stubs for a safety supervisor (**Axion Kernel**) and a content-addressable filesystem (**CanonFS**).

The system is currently in a **late-alpha / early-beta** state. The core numeric libraries are well-tested, while the compiler and VM are still under active development.

______________________________________________________________________

## 2. Getting Started

### Prerequisites

- A C++20 compliant compiler (e.g., GCC 10+, Clang 12+)
- CMake 3.16+
- Ninja (recommended) or Make

### Build and Test

To get started, clone the repository, build the project, and run the test suite.

```bash
# 1. Clone the repository
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation

# 2. Configure the build
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release

# 3. Build all targets
cmake --build build --parallel

# 4. Run the core test suite
ctest --test-dir build --output-on-failure
```
You should see all tests pass.

### Run Demos

The repository includes several demonstration binaries in `build/` that showcase the current capabilities of the system.

```bash
# High-level overview of numerics, tensors, and CanonFS
./build/t81_demo

# Detailed tensor operations
./build/t81_tensor_ops

# Axion kernel stub functionality
./build/axion_demo
```

### Build the Documentation Site

The documentation website is built with [Jekyll](https://jekyllrb.com/). Building it requires a working [Ruby](https://www.ruby-lang.org/en/) environment with the [Bundler](https://bundler.io/) gem installed.

```bash
# Navigate to the docs directory
cd docs

# Install dependencies (first time only)
bundle install

# Serve the site locally at http://localhost:4000
bundle exec jekyll serve
```

______________________________________________________________________

## 3. Repository Structure

The repository is organized into distinct zones with clear boundaries.

| Path            | Description                                               |
| --------------- | --------------------------------------------------------- |
| `/spec/`        | **Source of Truth:** The immutable project constitution.  |
| `/include/t81/` | **Public API:** Modern, header-only C++20 core libraries. |
| `/src/`         | **Implementation:** The compiled `.cpp` source files.       |
| `/tests/`       | **Verification:** The C++ unit and end-to-end test suite. |
| `/examples/`    | **Usage:** Standalone demonstration programs.             |
| `/docs/`        | **Guidance:** Jekyll documentation site for contributors. |
| `/legacy/`      | **Historical:** The immutable CWEB reference implementation.|

______________________________________________________________________

## 4. Where to Go Next

- **To understand the project's vision:** Read the [T81 Overview (`spec/t81-overview.md`)](spec/t81-overview.md).
- **For a technical deep-dive:** Start with the [Architecture Document (`ARCHITECTURE.md`)](ARCHITECTURE.md).
- **To contribute code:** Read the [Contribution Guide (`CONTRIBUTING.md`)](CONTRIBUTING.md).
- **To see the current state of the implementation:** View the [System Status Report (`docs/system-status.md`)](docs/system-status.md).
- **To write your first T81 program:** Follow the [C++ Quickstart Guide (`docs/cpp-quickstart.md`)](docs/cpp-quickstart.md).
