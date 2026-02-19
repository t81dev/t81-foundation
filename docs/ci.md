---
layout: page
title: CI & Test Matrix
---

# T81 Foundation: CI & Test Matrix

> **Source of Truth:** This document maps the Continuous Integration (CI) and local testing workflows for the project. For reproducibility details, see [REPRODUCIBILITY.md](REPRODUCIBILITY.md).

**Last Updated:** February 19, 2026

______________________________________________________________________

## 1. Local Commands (Must-run before PR)

To ensure your changes pass CI, follow this ritual:

1. **Configure & build**
   ```bash
   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
   cmake --build build --parallel
   ```

2. **Unit tests**
   ```bash
   ctest --test-dir build --output-on-failure
   ```
   - Default language mode is C++23.
   - Executes the full CTest matrix (214+ tests).

3. **C++20 compatibility lane**
   ```bash
   cmake -S . -B build-cxx20 -G Ninja -DCMAKE_BUILD_TYPE=Release -DT81_USE_CXX23=OFF
   cmake --build build-cxx20 --parallel
   ctest --test-dir build-cxx20 --output-on-failure
   ```

4. **Formatting Check**
   ```bash
   find src include tests examples \( -name "*.cpp" -o -name "*.hpp" -o -name "*.c" -o -name "*.h" \) -print0 | xargs -0 clang-format-18 --dry-run --Werror
   ```
   - **Note:** CI enforces `clang-format-18` strictly. Run with `-i` instead of `--dry-run` to fix violations locally.

5. **Reproducibility Gates**
   - **T81Lang AST/IR Hash:**
     ```bash
     python3 scripts/ci/t81lang_repro_gate.py --t81-bin build/t81 --fixtures-dir tests/fixtures/t81lang_determinism --check
     ```
   - **T3_K Cross-Arch Hash:**
     ```bash
     # Requires specific toolchain setup, usually run in CI
     python3 scripts/ci/t3k_repro_gate.py ...
     ```

6. **Governance Audit**
   ```bash
   make audit-governance
   ```
   - Checks workflow permissions, action pinning, and legacy numeric policy compliance.

## 2. GitHub Workflows

| Workflow | Triggers | Key steps |
| --- | --- | --- |
| `.github/workflows/ci.yml` | pushes/PRs | Builds core (`x86_64` + `arm64`), runs `ctest`, validates C++23/C++20 matrix, runs T3_K/T81Lang repro gates. |
| `.github/workflows/format.yml` | pushes/PRs | Enforces `clang-format-18` style on all C++ source files. |
| `.github/workflows/codeql.yml` | nightly + PRs | Runs GitHub CodeQL analysis for security vulnerabilities. |
| `.github/workflows/bench.yml` | `workflow_dispatch` | Builds benchmarks and updates `docs/benchmarks.md`. |
| `.github/workflows/repro-ledger.yml` | weekly + `dispatch` | Generates full reproducibility ledger and performance snapshot. |
| `.github/workflows/runtime-contract.yml` | pushes/PRs + nightly | Monitors drift between `t81-foundation` specs and `t81-vm` runtime. |
| `.github/workflows/t81lang-repro-hash-refresh.yml` | `dispatch` | Regenerates canonical AST/IR hashes if valid changes occur. |
| `.github/workflows/pdf.yaml` | push (pdf/) | Generates PDF versions of specifications using Marp. |
| `.github/workflows/release.yml` | tags (`v*`) | Builds release artifacts and documentation. |
| `.github/workflows/static.yml` | pushes | Updates the documentation search index. |

## 3. Troubleshooting CI Failures

- **Build errors:** Rerun `cmake --build build --verbose` to see full compiler flags.
- **Test failures:** Run the specific failing binary (e.g., `./build/tests/cpp/t81_test`) to see direct output.
- **Formatting errors:** Run `clang-format-18 -i <file>` locally.
- **Repro gate failures:** Ensure no nondeterministic pointers or timestamps are leaking into AST/IR output. Check `tests/fixtures/t81lang_determinism/`.

## 4. Required Checks for Merging

Branch protection requires the following checks to pass:

- `gate / t3k cross-arch bit-identity`
- `gate / t81lang cross-arch bit-identity`
- `format / clang-format`
- `build / linux-x86_64 / clang`
- `build / linux-arm64 / clang`
