---
layout: page
title: CI & Test Matrix
---

# T81 Foundation: CI & Test Matrix

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Foundation: CI & Test Matrix](#t81-foundation-ci-&-test-matrix)
  - [1. Local Commands (Must-run before PR)](#1-local-commands-must-run-before-pr)
  - [2. GitHub Workflows](#2-github-workflows)
  - [3. Troubleshooting CI Failures](#3-troubleshooting-ci-failures)
  - [4. Questions for Maintainers](#4-questions-for-maintainers)
  - [5. Required Checks Setup](#5-required-checks-setup)

<!-- T81-TOC:END -->


This doc explains how to reproduce the core CI workflows locally and which tests are run in GitHub Actions before merging.

______________________________________________________________________

## 1. Local Commands (Must-run before PR)

1. **Configure & build**
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --parallel
   ```
   - Shortcut: `make cmake-configure && make cmake-build`
2. **Unit tests**
   ```bash
   ctest --test-dir build --output-on-failure
   ```
   - Shortcut: `make cmake-test`
   - The suite executes the full CTest matrix declared in `CMakeLists.txt`.
   - Default language mode is C++23.
3. **C++20 compatibility lane**
   ```bash
   cmake -S . -B build-cxx20 -DCMAKE_BUILD_TYPE=Release -DT81_USE_CXX23=OFF
   cmake --build build-cxx20 --parallel
   ctest --test-dir build-cxx20 --output-on-failure
   ```
4. **Extended suite (optional but recommended for releases)**
   ```bash
   ctest --test-dir build -R "fuzz|property|axion" --schedule-random
   ```
5. **Docs**
   ```bash
   cmake --build build --target docs
   ```
   - Regenerates `build/api/html`; open `build/api/html/index.html` to inspect generated pages.
6. **Optional helpers**
   - `./build/t81 internal benchmark` to refresh `docs/reference/benchmarks.md`.
   - `cmake --build build --target t81` to recompile the CLI after changes.
   - `./build/t81 internal repro-hash tests/fixtures/t81lang_determinism` to run the T81Lang fixture reproducibility helper and print the current aggregate hash.
   - `python3 scripts/ci/t81lang_repro_gate.py --t81-bin build/t81 --fixtures-dir tests/fixtures/t81lang_determinism --workdir build/t81lang-repro --hash-out build/t81lang-repro/hash.txt --expected-hash-file tests/fixtures/t81lang_determinism/t81lang_repro_hash.txt` to run the authoritative T81Lang compile reproducibility gate locally.
   - `python3 ../../scripts/ci/generate_repro_dashboard.py ...` to synthesize the reproducibility ledger report (see `../REPRODUCIBILITY.md`).
   - `python3 ../../scripts/ci/audit_workflow_actions.py --markdown-out ../audits/2026-02-workflow-action-audit.md` to snapshot workflow action pinning posture and migration candidates.
   - `python3 ../../scripts/ci/audit_workflow_permissions.py --markdown-out ../audits/2026-02-workflow-permissions-audit.md` to snapshot workflow permissions posture and least-privilege drift.
   - `python3 ../../scripts/ci/audit_workflow_actions.py --max-tagged 0 --max-unknown 0` to enforce CI pinning policy (no tag/unclassified `uses:` references).
   - `python3 ../../scripts/ci/audit_workflow_permissions.py --max-missing 0` to enforce explicit permissions on all workflows.
   - `python3 ../../scripts/ci/check_legacy_core_numeric_includes.py` to enforce the compatibility-shim policy (no new includes of `t81/core/{bigint,fraction}.hpp` outside allowlisted files).
   - `python3 ../../scripts/ci/check_legacy_core_numeric_type_usage.py` to enforce the compatibility-shim policy (no new `core::BigInt` / `core::Fraction` type usage outside allowlisted files).
   - `python3 ../../scripts/ci/check_legacy_v1_numeric_includes.py` to enforce consolidation policy (no new includes of migration-only `t81/core/{T81BigInt,T81Fraction}.hpp` outside allowlisted files).
   - `python3 ../../scripts/ci/check_core_numeric_wrapper_thinness.py` to enforce thin-wrapper discipline in `../../core/types/{bigint,fraction}.cpp` (no arithmetic implementation tokens in compatibility adapter files).
   - `python3 ../../scripts/ci/check_v1_canonical_numeric_alias_usage.py` to enforce alias-based migration style in `../../tests/cpp/v1*_*.cpp` (use `t81::v1::CanonicalBigInt` / `t81::v1::CanonicalFraction`, avoid direct `t81::T81BigInt` / `t81::T81Fraction` there).
   - `../../scripts/ci/run_workflow_audits.sh` to run the governance audit bundle (workflow pinning, workflow permissions, and legacy numeric compatibility policy checks) in strict mode with one command.
   - `make cmake-ritual` to run the single-threaded local build/test ritual end-to-end.

## 2. GitHub Workflows

| Workflow | Triggers | Key steps |
| --- | --- | --- |
| `../../.github/workflows/ci.yml` | pushes/PRs on `main` | validates docs/spec structure (including ARCHITECTURE target-table sync and legacy numeric include policy), runs deterministic-profile enforcement markers, configures CMake, builds `t81` and Google Benchmark, runs `ctest`, runs a C++ standard compatibility matrix (`-DT81_USE_CXX23=ON/OFF`) on linux clang, runs T3_K and T81Lang reproducibility gates on linux clang (`x86_64` + `arm64`), and compares cross-arch gate hashes. |
| `../../.github/workflows/codeql.yml` | nightly + pull requests | runs CodeQL analysis for C/C++ on incoming changes. |
| `../../.github/workflows/bench.yml` | `workflow_dispatch` + pushes on benchmark/runtime-path changes | builds the shared benchmark runner, enforces the VM workload guardrail, and refreshes benchmark-facing badge/README outputs. |
| `../../.github/workflows/benchmark_packed_trit_vector.yml` | weekly + `workflow_dispatch` | runs the specialized packed-trit SIMD regression slice as a non-PR benchmark lane, preserving the signal without keeping a second push/PR-sensitive benchmark workflow. |
| `../../.github/workflows/inference-bench.yml` | weekly + release tags + `workflow_dispatch` | regenerates and commits the published inference comparison benchmark report; kept outside the PR-sensitive benchmark lane because it is a publication/reporting workflow rather than a narrow regression guard. |
| `../../.github/workflows/repro-ledger.yml` | weekly + `workflow_dispatch` | runs build/test + T3_K reproducibility gate + Axion trace capture + benchmark snapshot and publishes `reproducibility-ledger` dashboard artifacts. |
| `../../.github/workflows/runtime-contract.yml` | pushes/PRs + nightly + `workflow_dispatch` | validates `../../contracts/runtime-contract.json` against `t81-vm` contract/tag/pin, requires explicit approval for marker drift, and remains standalone because it is a cross-repo contract gate with separate approval semantics. |
| `../../.github/workflows/t81lang-repro-hash-refresh.yml` | `workflow_dispatch` | regenerates `../../tests/fixtures/t81lang_determinism/t81lang_repro_hash.txt` and opens an automated PR. |
| `../../.github/workflows/release.yml` | tag pushes (`vX.Y.Z`) | production build, docs PDF generation, exposure of release assets (see `../roadmaps-plans/RELEASING.md`). |
| `../../.github/workflows/static.yml` | pushes + `workflow_dispatch` | builds and publishes the docs search index (Node/Lunr/Cheerio) via an automated PR flow. |

## 3. Troubleshooting CI Failures

- **Build errors:** rerun `cmake --build build --verbose` and inspect the compiler output for missing includes or changed flags.  
- **Test failures:** run the failing binary from `build/tests/cpp/...` to see stdout/stderr.  
- **CodeQL / static issues:** check GitHub comments for file references; run `codeql` locally if needed (install via GitHub CLI).  
- **Docs mismatch:** ensure `../../build/api/html` and `benchmarks.md` match the current `build` artifacts before pushing.

## 4. Questions for Maintainers

- Are there additional sanitizers (ASan, UBSan) or platforms (Linux, macOS) we should add to `ci.yml`?  
- Should `ci.yml` publish artifacts (docs PDFs, benchmarks) for downstream users? Document expectations here if so.

## 5. Required Checks Setup

Branch protection cannot be declared from repository source. Configure it in
GitHub settings and mark these checks as required:

- `gate / t3k cross-arch bit-identity`
- `gate / t81lang cross-arch bit-identity`

Recommended additional required checks:

- `build / linux-x86_64 / clang`
- `build / linux-arm64 / clang`
