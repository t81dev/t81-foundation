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

.   .   .

[![Reference Platform: macOS • clang](https://img.shields.io/badge/Reference-macOS%20clang-81D4FA?logo=apple&style=for-the-badge)](https://github.com/t81dev/t81-foundation/actions)

.   .   .

[![Negation](https://img.shields.io/badge/Negation-6.52_Gops/s_(faster_than_int64)-brightgreen)](https://github.com/t81-project/t81)
[![Range](https://img.shields.io/badge/Range-40×_greater_than___int128-blue)](https://github.com/t81-project/t81)
[![Overflow](https://img.shields.io/badge/Overflow-NEVER-red)](https://github.com/t81-project/t81)
[![Exact Math](https://img.shields.io/badge/Math-Perfect-yellow)](https://github.com/t81-project/t81)
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

- **Pattern-match demo:** Follow `docs/guides/match-example.md` to compile and run `examples/match_demo.t81` via the `t81` CLI so you can experiment with `Option`/`Result` matches end to end.

- **Data-type demo:** Try `docs/guides/data-types-overview.md` and `examples/data_types.t81` to see how primitive arithmetic, strings, and structural `match` expressions behave in a single example.

- **Fraction demo:** Expand that guide by running `examples/fraction_demo.t81` to explore canonical `T81Fraction` arithmetic.

- **Tensor demo:** Sample `examples/tensor_demo.t81` (also documented in the guide) to see how typed `Tensor` declarations behave; extend the example to examine shapes if desired.

- **BigInt demo:** Try `examples/bigint_demo.t81` to experiment with balanced ternary big integers (addition and multiplication) before lowering to TISC.

- **Float demo:** Use `examples/float_demo.t81` to see how `T81Float` values multiply and subtract to produce canonical results.

- **String demo:** Run `examples/string_demo.t81` to concatenate `T81String` literals and inspect the length operations.

- **Vector & Matrix demos:** Run `examples/vector_demo.t81` / `examples/matrix_demo.t81` to explore dot-products, traces, and how Tensor/Matrix indexing works (shape awareness).

- **Cell & Quaternion demos:** Execute `examples/cell_demo.t81` and `examples/quaternion_demo.t81` to observe canonical `T81Cell` ranges and quaternion conjugate/norm.

- **High-rank Tensor demo:** Run `examples/high_rank_tensor_demo.t81` to trace a three-dimensional tensor literal through indexing and reduction.

- **Graph demo:** Use `examples/graph_demo.t81` to count edges in a small adjacency matrix via deterministic matrix access.

- **Automated demo runner:** Execute `scripts/run-demos.sh` (after building the CLI) to compile and run the match, primitive, fraction, tensor, bigint, float, string, vector, matrix, cell, quaternion, high-rank tensor, and graph demos sequentially; the script reports compilation/runnable output for each.
- **IR Inspector:** Build `./build/ir_inspector` and run it on any `.t81` file to print the IR instructions (including match lowering) emitted before binary emission.
- **Benchmark suite:** Run `./build/t81 benchmark` (with optional benchmark flags) to invoke `build/benchmarks/benchmark_runner`; it runs the core throughput/packing benchmarks and updates `docs/benchmarks.md` with branch/commit metadata plus latency analysis.

## Benchmark Highlights

![Benchmark Report](https://img.shields.io/badge/benchmarks-docs%2Fbenchmarks.md-blueviolet)

![Negation](https://img.shields.io/badge/Negation-6.6_Gops/s_(faster_per_digit_than_int64)-brightgreen)

![T81 Classic column](https://img.shields.io/badge/Docs%2Fbenchmarks.md-T81%20Classic%20column-blueviolet)
![T81 Native column](https://img.shields.io/badge/Docs%2Fbenchmarks.md-T81%20Native%20column-brightgreen)

| Metric | Value |
| --- | --- |
| Largest T81 Classic advantage (latest run) | `BM_LimbArithThroughput_T81Limb` (12.78 Mops/s tryte Kogge-Stone vs 350.95 Mops/s binary) |
| Largest T81 Native advantage (latest run) | `BM_NegationSpeed_T81Native` (50.18 Mops/s PSHUFB negation vs 8.40 Gops/s binary) |
| Largest binary advantage | `BM_ArithThroughput` (0.00x — exact rounding vs. binary carry chains) |
| Table columns | `docs/benchmarks.md` now reports Classic (`T81 Result`) and Native (`T81 Native Result`) throughputs alongside the Binary column |
| Report source | `docs/benchmarks.md` (auto-regenerated whenever `./build/t81 benchmark` runs) |
| Lookup-phase negation | `BM_NegationSpeed_PackedCell` (~2.26 Gops/s) demonstrates constant-time packed negation before we swap the core representation |

These highlights mirror the “Highlights” section in `docs/benchmarks.md`; rerun the CLI benchmark command to refresh the ratios and notes.

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
