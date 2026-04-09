---
layout: page
title: Developer Onboarding
---

# T81 Foundation: Developer Onboarding

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Foundation: Developer Onboarding](#t81-foundation-developer-onboarding)
  - [1. Prerequisites](#1-prerequisites)
  - [2. Repository Layout at a Glance](#2-repository-layout-at-a-glance)
  - [3. Build, Test, and Docs Workflow](#3-build-test-and-docs-workflow)
  - [3.1 Canonical Ecosystem Consumer Path](#31-canonical-ecosystem-consumer-path)
  - [4. CLI Cheat Sheet](#4-cli-cheat-sheet)
  - [5. First Bug Fix / Feature Path](#5-first-bug-fix--feature-path)
  - [6. Troubleshooting Tips](#6-troubleshooting-tips)
  - [7. Further Reading](#7-further-reading)

<!-- T81-TOC:END -->


This guide is the deterministic portal: it lays out the ledgered path from cloning the repo to shipping a change, complete with Axion policies, canonical builds, and reproducibility checkpoints.

______________________________________________________________________

## 1. Prerequisites

- macOS/Linux with a recent Clang/GCC (C++23 default; temporary C++20 compatibility lane via `-DT81_USE_CXX23=OFF`).
- CMake 3.16 or newer.
- Ninja (recommended) or Unix `make`.
- Python 3.9+ for optional scripts (e.g., benchmark reports).
- Git configured with your `user.name`/`user.email`; ensure your commits carry identity so the ledger stays auditable.

## 2. Repository Layout at a Glance

| Area | Purpose |
| --- | --- |
| `include/t81/` | Public headers (`t81::v1`) — add APIs that respect Axion and ternary invariants. |
| `src/` | Implementation for compiler, VM, tools, and deterministic tooling with canonical builds. |
| `tests/cpp/` | Regression proofs; every manifest change gets tests. |
| `docs/` | Guides, portal narratives, and quickstarts (Doxygen output is generated under `build/api`). |
| `spec/` | Normative constitution; updates require RFCs under `spec/rfcs/`. |
| `benchmarks/` | Benchmark runners and report generation scripts that feed `docs/reference/benchmarks.md`. |
| `tools/`, `scripts/` | Utility helpers used in reproducibility workflows (Axion trace capture, policy generation). |

Always refer to `../explanation/ARCHITECTURE.md` and `../explanation/DESIGN.md` before modifying cross-cutting subsystems.

## 3. Build, Test, and Docs Workflow

```bash
git clone https://github.com/t81dev/t81-foundation.git
cd t81-foundation
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release      # this config is the baseline manifest entry
cmake --build build --parallel
ctest --test-dir build --output-on-failure
cmake --build build --target docs                  # regenerates build/api/html
```

- `T81_BUILD_TESTS`, `T81_BUILD_BENCHMARKS`, and `T81_BUILD_EXAMPLES` can be passed to `cmake` as ON/OFF to scope the build and keep the ledger concise.
- Use `ctest --test-dir build -R <regexp>` to rerun a subset (e.g., `-R tensor`) when verifying targeted invariants.
- Doxygen output is generated under `build/api`; open `build/api/html/index.html` locally after running the `docs` target to confirm the manifest matches the implementation.

## 3.1 Canonical Ecosystem Consumer Path

For migration-aligned runtime validation, run the canonical e2e bundle from
`t81-examples`:

```bash
scripts/run-canonical-runtime-demo.sh
```

Use `t81-foundation/examples` as reference and research demos; contract
promotion evidence should use the `t81-examples` runtime bundle.

## 4. CLI Cheat Sheet

The primary entrypoint is the `t81` binary built under `build/`.

```text
t81 compile <file.t81> [-o <file.tisc>]
t81 run <file.t81|.tisc>
t81 check <file.t81> (syntax + semantic validation)
t81 benchmark [benchmark flags]
t81 weights import <safetensors|gguf> [...]
```

- Semantic errors reported by `t81 compile` now include a `file:line:column` snippet so you can locate the problem immediately.

- Running `t81 compile`/`run` exercises the frontend→TISC→VM pipeline covered in `../explanation/ARCHITECTURE.md`.
- `t81 weights quantize … --to-gguf` produces GGUF models for `llama.cpp`.
- Inspect CLI help with `./build/t81 --help`.

## 5. First Bug Fix / Feature Path

1. **Pick a target:** For example, `tests/cpp/t81_tensor_*` exercises tensors; fixes here have fast feedback loops.
2. **Reproduce:** Build `t81_tensor_matmul_test` and confirm failure with `ctest -R tensor_matmul`.
3. **Code change:** Modify headers under `include/t81/types` or `core/types` keeping RAII/no-exceptions rules.
4. **Add regression test:** Every semantics change needs a test addition under `tests/cpp/`.
5. **Docs touch:** Update `docs/how-to/tensor-guide.md` or relevant doc to describe new behavior.
6. **Verify:** `cmake --build build --target t81_tensor_matmul_test` plus `ctest -R tensor_matmul` and rerun `cmake --build build --target docs`.

## 6. Troubleshooting Tips

- If `cmake` caches stale options, delete `build/` and re-configure.
- On Apple Silicon, AVX2 flags are skipped by default; expect different warnings than on x86_64.
- Compiler warnings (unused vars, missing-initializer) are plentiful; focus on new additions and run `clang-tidy` later if desired.
- When tests fail under `ctest`, run the executable directly (`./build/t81_tensor_matmul_test`) to see stdout.

## 7. Further Reading

- Specs: `../../spec/t81lang-spec.md`, `../../spec/tisc-spec.md`, `../../spec/t81vm-spec.md`, `../../spec/t81-data-types.md`.
- Architecture & design: `../explanation/ARCHITECTURE.md`, `../explanation/DESIGN.md`, `../reference/system-status.md`.
- Contribution process: `../../CONTRIBUTING.md`, `../../spec/rfcs/template.md`.
- Onboarding support: Ask in repo issues or review `../guides/adding-a-language-feature.md`.

Keeping this guide up-to-date is critical: each release should confirm the commands still work and the CLI workflow matches `../../README.md`.
