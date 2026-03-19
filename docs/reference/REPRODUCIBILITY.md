# Reproducibility Guide

> **Source of Truth:** This document provides the exact instructions to reproduce
> the currently supported T81Lang determinism evidence surfaces and verify them
> against the checked-in aggregate fixture hash.

**Last Updated:** March 8, 2026

## 1. Goal

This guide does not claim that every CLI command or every host-dependent build
artifact is universally bit-identical across all environments.

The supported local ritual in this document verifies the T81Lang compiler
fixture corpus and its aggregate reproducibility hash. That is the operator
entry point for the determinism evidence surfaced by the CLI and CI gates.

For the newer governed CLI workflow replay ritual, see
[CLI_REPLAY_BUNDLES.md](/Users/t81dev/Code/t81-foundation/docs/reference/CLI_REPLAY_BUNDLES.md).

## 2. Supported Environment

To reproduce the canonical T81Lang fixture hash, use the following known-good
environment:

| Component | Version | Notes |
| :--- | :--- | :--- |
| **OS** | Ubuntu 24.04 LTS (x86_64 or ARM64) | Standard GitHub Runner Image |
| **Compiler** | Clang 18 | `clang-18`, `clang++-18` |
| **CMake** | 3.28+ | |
| **Build Type** | Release | `-DCMAKE_BUILD_TYPE=Release` |

## 3. Reproduction Ritual (Canonical)

Build the CLI, then run the T81Lang reproducibility gate with explicit fixture,
work, and output paths.

```bash
# 1. Configure
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=clang++-18 \
  -DCMAKE_C_COMPILER=clang-18

# 2. Build
cmake --build build --parallel

# 3. Run the exact T81Lang reproducibility gate
mkdir -p build/t81lang-repro
python3 scripts/ci/t81lang_repro_gate.py \
  --t81-bin build/t81 \
  --fixtures-dir tests/fixtures/t81lang_determinism \
  --workdir build/t81lang-repro \
  --hash-out build/t81lang-repro/hash.txt \
  --expected-hash-file tests/fixtures/t81lang_determinism/t81lang_repro_hash.txt
```

For a quick operator check, the CLI also exposes:

```bash
./build/t81 internal repro-hash tests/fixtures/t81lang_determinism
```

That helper is convenient for local use, but the Python gate invocation above is
the authoritative release/review ritual because it also checks the expected
aggregate hash file directly.

## 4. Expected Artifact Hashes

For release review, prefer the checked-in fixture hash and CI artifacts over
hand-maintained tables.

Primary local reference:

| Artifact | Source |
| :--- | :--- |
| T81Lang aggregate repro hash | `tests/fixtures/t81lang_determinism/t81lang_repro_hash.txt` |
| T81Lang aggregate repro gate output | `build/t81lang-repro/hash.txt` |

Historical release packets may also record the hash observed for a specific
release candidate:

| Artifact | Example reference |
| :--- | :--- |
| Release readiness packet | `docs/records/status-history/RELEASE_READINESS_PACKET_2026-02.md` |

Hashes vary by commit. For authoritative current values, use the checked-in
fixture hash file and the latest CI reproducibility artifacts.

## 5. Known Nondeterminism Surfaces

We actively mitigate the following sources of nondeterminism:

- **Timestamps:** We strip build timestamps from binaries where possible or use `SOURCE_DATE_EPOCH`.
- **File Order:** We sort source file lists in build and compile inputs.
- **Concurrency:** Parallel builds (`--parallel`) must not affect output linking order determinism.
- **ASLR/mmap:** JIT compilation uses deterministic memory mapping strategies (see `spec/vm/jit-determinism.md`).

Out of scope for this guide:

- host inventory commands such as `t81 env ...`
- shell completion and manpage installation flows
- local feedback/log/report files
- experimental or non-DCP surfaces such as `t81 internal llama-run`

If you find a nondeterministic result, please file a [Bug Report](../../.github/ISSUE_TEMPLATE/bug_report.md).
