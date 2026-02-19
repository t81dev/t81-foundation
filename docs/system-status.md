# T81 Foundation – Current Status

> **Source of Truth:** This document defines the **current operational state** of the project (what is true today). For future plans, see [ROADMAP.md](../ROADMAP.md). For version history, see [CHANGELOG.md](../CHANGELOG.md).

**Last Updated:** February 19, 2026
**Status:** Active development (Stable Core)

## 1. Snapshot

- Core stack is implemented and operational across numerics, language frontend, TISC serialization, VM execution, and Axion/CanonFS enforcement.
- C++23 is the default build language mode; C++20 remains a compatibility lane.
- Current local validation baseline passes full ritual (`214/214` tests in latest run).
- Ecosystem compliance with T81 v1.1.0-canonical specification is now enforced across HanoiVM and TISC tools.

## 2. Recent Hardening (Feb 2026)

The following security and reproducibility enhancements have been deployed and verified:

- **Package Initialization Security (#225):** Strict sanitization enforced for `t81 init` package names (alphanumeric, hyphens, underscores only) to prevent S-expression injection vulnerabilities.
- **SHA3-512 Correctness (#224):** Fixed Chi-step implementation in `src/crypto/sha3.cpp` to match FIPS-202 test vectors.
- **Reproducibility Gates (#226):** Tightened AST/IR determinism checks (`t81lang_repro_gate.py`) and enforced strict `clang-format-18` across the codebase.

## 3. Component Maturity

| Component | Maturity | Spec Version | Test Coverage | Notes |
| :--- | :--- | :--- | :--- | :--- |
| **T81Lang** | Stable | v1.1.0 | High | Full syntax and semantics implemented. AST/IR now strictly hashed. |
| **TISC** | Stable | v1.1.0 | High | Binary serialization and opcode matrix complete. |
| **HanoiVM** | Stable | v1.1.0 | High | Core execution loop and memory model verified. |
| **Axion** | Stable | v1.0.0 | High | Policy enforcement and event tracing active. |
| **CanonFS** | Beta | v0.9.0 | Medium | Drivers implemented; performance optimization ongoing. |
| **CLI** | Beta | - | Medium | `run`/`compile` stable; `debug`/`trace` improving. Package init secured. |
| **JIT** | Experimental | - | Low | Trace recording implemented; native backend planned. |

**Maturity Levels:**
- **Stable:** Production-ready, API frozen, full test coverage.
- **Beta:** Functionally complete, potential API changes, good coverage.
- **Experimental:** Active research/prototyping, no stability guarantees.

## 4. Supported Toolchains

The following environments are explicitly supported and verified in CI:

| Platform | OS | Compiler | Status |
| :--- | :--- | :--- | :--- |
| **Linux (x86_64)** | Ubuntu 24.04 | GCC 14, Clang 18 | **Primary Tier** (Determinism Gate) |
| **Linux (ARM64)** | Ubuntu 24.04 | Clang 18 | **Primary Tier** (Determinism Gate) |
| **macOS (ARM64)** | macOS 14 | Clang (Apple) | Supported |
| **macOS (x86_64)** | macOS 13 | GCC 14 | Supported |
| **Windows (x86_64)** | Windows Server 2022 | MSVC (VS 2022) | Supported (Best Effort) |

## 5. Validation Ritual

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Single-threaded safe mode:

```bash
cmake --build build --parallel 1
ctest --test-dir build --output-on-failure -j1
```

## 6. Source-of-Truth Links

- Architecture: [`ARCHITECTURE.md`](../ARCHITECTURE.md)
- Conformance analysis: [`ANALYSIS.md`](../ANALYSIS.md)
- Near-term tasks: [`TASKS.md`](../TASKS.md)
- CI/gates: [`docs/ci.md`](ci.md)
