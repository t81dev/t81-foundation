# Ecosystem and Interoperability

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Ecosystem and Interoperability](#ecosystem-and-interoperability)
  - [1. Expanding Language Bindings](#1-expanding-language-bindings)
    - [Strategy: Preserve Determinism](#strategy-preserve-determinism)
  - [2. Runtime Boundary Tooling](#2-runtime-boundary-tooling)
    - [Goal: Strengthen Downstream Integration](#goal-strengthen-downstream-integration)
  - [3. Deterministic Observability](#3-deterministic-observability)

<!-- T81-TOC:END -->


**Last Updated:** February 10, 2026

This document outlines the strategy for expanding the T81 Foundation ecosystem and ensuring robust interoperability with external tools and languages.

## 1. Expanding Language Bindings

### Strategy: Preserve Determinism

Our primary goal when expanding bindings is to preserve the deterministic guarantees of the T81 core.

-   **Existing C++ API**: `include/t81/` serves as the foundational C++ API.
-   **Python Bindings**: We plan to expose `T81Int`, `T81BigInt`, and `T81Tensor` to Python via `pybind11` or similar, ensuring that Python code can invoke T81 primitives deterministically.
-   **Rust Integration**: Investigating `cxx` or `bindgen` for Rust interop, leveraging Rust's safety guarantees alongside T81's determinism.
-   **WASM Target**: Compiling the T81 runtime to WebAssembly to enable browser-based T81 execution and visualization.

## 2. Runtime Boundary Tooling

### Goal: Strengthen Downstream Integration

We aim to make it easier for downstream consumers (`t81-foundation`, `t81-vm`, `examples`) to build on top of the T81 stack.

-   **`t81-foundation` -> `t81-vm`**: Ensuring strict versioning and ABI compatibility between the core library and the VM runtime.
-   **Example Gallery**: Expanding `examples/` to serve as canonical references for integration patterns.
-   **CLI Improvements**: Enhancing the `t81` CLI to better support scripting and automation in larger pipelines.

## 3. Deterministic Observability

All bindings must expose the Axion trace hooks correctly.

-   **Trace Propagation**: When an external language calls into T81, the resulting operations must be logged to the active Axion trace.
-   **Error Handling**: Mapping T81 exceptions/results to native language constructs while preserving the error codes for deterministic replay.
