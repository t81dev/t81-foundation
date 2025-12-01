---
layout: page
title: CI & Test Matrix
---

# T81 Foundation: CI & Test Matrix

This doc explains how to reproduce the core CI workflows locally and which tests are run in GitHub Actions before merging.

______________________________________________________________________

## 1. Local Commands (Must-run before PR)

1. **Configure & build**
   ```bash
   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build --parallel
   ```
2. **Unit tests**
   ```bash
   ctest --test-dir build --output-on-failure
   ```
   - The suite currently executes 65 targeted binaries (see `tests/cpp/*.cpp` in `CMakeLists.txt`).
3. **Extended suite (optional but recommended for releases)**
   ```bash
   ctest --test-dir build -R "fuzz|property|axion" --schedule-random
   ```
4. **Docs**
   ```bash
   cmake --build build --target docs
   ```
   - Regenerates `docs/api/html`; open `docs/api/html/index.html` to inspect generated pages.
5. **Optional helpers**
   - `./build/t81 benchmark` to refresh `docs/benchmarks.md`.
   - `cmake --build build --target t81` to recompile the CLI after changes.

## 2. GitHub Workflows

| Workflow | Triggers | Key steps |
| --- | --- | --- |
| `.github/workflows/ci.yml` | pushes/PRs on `main` | configures CMake, builds `t81` and Google Benchmark, runs `ctest`, publishes warnings. Mirrors local build/test commands above. |
| `.github/workflows/codeql.yml` | nightly + PR merges | runs CodeQL analysis on main/master. |
| `.github/workflows/bench.yml` | manually via `workflow_dispatch` | builds benchmark runner and pipeline, publishes `docs/benchmarks.md` updates. |
| `.github/workflows/release.yml` | tag pushes (`vX.Y.Z`) | production build, docs PDF generation, exposure of release assets (see `docs/release.md`). |
| `.github/workflows/static.yml` | pushes | runs static checks (format/lint) that are currently placeholder; extend when necessary. |

## 3. Troubleshooting CI Failures

- **Build errors:** rerun `cmake --build build --verbose` and inspect the compiler output for missing includes or changed flags.  
- **Test failures:** run the failing binary from `build/tests/cpp/...` to see stdout/stderr.  
- **CodeQL / static issues:** check GitHub comments for file references; run `codeql` locally if needed (install via GitHub CLI).  
- **Docs mismatch:** ensure `docs/api/html` and `docs/benchmarks.md` match the current `build` artifacts before pushing.

## 4. Questions for Maintainers

- Are there additional sanitizers (ASan, UBSan) or platforms (Linux, macOS) we should add to `ci.yml`?  
- Should `ci.yml` publish artifacts (docs PDFs, benchmarks) for downstream users? Document expectations here if so.
