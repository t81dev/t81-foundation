# Reproducibility Guide

> **Source of Truth:** This document provides the exact instructions to **reproduce deterministic build artifacts** and verify them against our published hashes.

**Last Updated:** February 19, 2026

## 1. Goal

We guarantee that our build process and compiler output are deterministic: given the same source, compiler, and build configuration, the output binary and Intermediate Representation (IR) will be bit-identical.

## 2. Supported Environment

To reproduce canonical artifacts, you must use the following environment:

| Component | Version | Notes |
| :--- | :--- | :--- |
| **OS** | Ubuntu 24.04 LTS (x86_64 or ARM64) | Standard GitHub Runner Image |
| **Compiler** | Clang 18 | `clang-18`, `clang++-18` |
| **CMake** | 3.28+ | |
| **Build Type** | Release | `-DCMAKE_BUILD_TYPE=Release` |

## 3. Reproduction Ritual (Canonical)

Use the provided CMake preset for exact flag alignment.

```bash
# 1. Clean build directory
rm -rf build

# 2. Configure with CI preset (or manual flags)
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=clang++-18 -DCMAKE_C_COMPILER=clang-18

# 3. Build
cmake --build build --parallel

# 4. Verify T81Lang Determinism Hash (AST/IR)
python3 scripts/ci/t81lang_repro_gate.py --t81-bin build/t81 --fixtures-dir tests/fixtures/t81lang_determinism --check
```

## 4. Expected Artifact Hashes

We track canonical hashes for critical artifacts.

| Artifact | Verification Method | Canonical Source |
| :--- | :--- | :--- |
| **T81Lang AST/IR** | `t81lang_repro_gate.py` | `tests/fixtures/t81lang_determinism/t81lang_repro_hash.txt` |
| **TISC Binary** | `t81 repro-hash` | *Varies by commit (see CI logs)* |
| **T3_K Model** | `t3k_repro_gate.py` | *Varies by commit (see CI logs)* |

## 5. Known Nondeterminism Surfaces

We actively mitigate the following sources of nondeterminism:

- **Timestamps:** We strip build timestamps from binaries where possible or use `SOURCE_DATE_EPOCH`.
- **File Order:** We sort source file lists in `CMakeLists.txt` and `t81 compile` inputs.
- **Concurrency:** Parallel builds (`--parallel`) must not affect output linking order determinism.
- **Pointer Iteration:** We prohibit iterating over memory addresses (e.g., `std::unordered_map` with pointer keys) in any IR generation path.
- **ASLR/mmap:** JIT compilation uses deterministic memory mapping strategies (see `spec/vm/jit-determinism.md`).

If you find a nondeterministic result, please file a [Bug Report](../.github/ISSUE_TEMPLATE/bug_report.md).
