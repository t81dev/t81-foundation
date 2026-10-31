# Third-Party Dependency & Hygiene Policy

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Third-Party Dependency & Hygiene Policy](#third-party-dependency-&-hygiene-policy)
  - [1. Principles](#1-principles)
  - [2. C++ Dependency Management](#2-c++-dependency-management)
    - [Approved Dependencies](#approved-dependencies)
    - [Integration Guidelines](#integration-guidelines)
  - [3. Node/JS Dependency Management](#3-nodejs-dependency-management)
  - [4. Artifact Containment](#4-artifact-containment)
  - [5. Remediation Plan (Immediate)](#5-remediation-plan-immediate)

<!-- T81-TOC:END -->


## 1. Principles
*   **Containment**: All third-party code must be isolated from canonical source.
*   **Reproducibility**: Builds must be offline-capable and bit-exact reproducible.
*   **Licensing**: No GPL/AGPL dependencies in the core. MIT/Apache-2.0/BSD preferred.

## 2. C++ Dependency Management
We utilize CMake's `FetchContent` or `git submodule` for C++ dependencies to ensure version pinning.

### Approved Dependencies
| Library | Usage | Mechanism | Version Policy |
| :--- | :--- | :--- | :--- |
| `asio` | Networking | FetchContent | Pinned Tag (e.g., `asio-1-30-2`) |
| `pybind11` | Python Bindings | System/Submodule | Compatible Range |
| `Google Benchmark` | Benchmarking | Submodule | Pinned SHA |
| `GTest` | Testing | Submodule | Pinned SHA |

### Integration Guidelines
*   **Do not vendor headers directly** into `include/t81` unless absolutely necessary for a header-only library that cannot be fetched (e.g., extremely small single-file libs).
*   **Wrap external types**: Do not expose third-party types (e.g., `asio::io_context`) in the public T81 C++ API. Use the Pimpl idiom or type erasure.

## 3. Node/JS Dependency Management
`package.json` is currently used for documentation and tooling only.

*   **Lockfiles**: `package-lock.json` must be committed.
*   **DevDependencies**: All build/doc tools must be in `devDependencies`.
*   **No Runtime JS**: The core T81 runtime does not depend on Node.js.

## 4. Artifact Containment
*   **Build Outputs**: All build artifacts must be directed to `build/` or `dist/` and ignored via `.gitignore`.
*   **Generated Code**: If code is generated (e.g., from `generate_dummy_safetensors.cpp`), it must be clearly marked and reproducible.

## 5. Remediation Plan (Immediate)
1.  **Stop Vendoring**: Identify any manually copied source files in `src/` and move them to `third_party/` or replace with `FetchContent`.
2.  **GitIgnore**: Ensure `benchmark_results_*.txt` and other root pollution is ignored.
3.  **Vulnerability Scanning**: Enable Dependabot for the repository.
