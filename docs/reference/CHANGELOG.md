# Changelog

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Changelog](#changelog)
  - [[Unreleased]](#[unreleased])
  - [[2.3.0] - 2026-11-27](#[230]---2026-11-27)
    - [Added](#added)
    - [BREAKING CHANGES](#breaking-changes)
    - [Security](#security)
  - [[2.2.0] - 2026-11-22](#[220]---2026-11-22)
    - [Added](#added)
  - [[2.1.0] - 2026-11-19](#[210]---2026-11-19)
    - [Added](#added)
  - [[2.0.0] - 2026-11-17](#[200]---2026-11-17)
    - [Changed](#changed)
  - [[1.5.0] - 2026-03-14](#[150]---2026-03-14)
    - [Added](#added)
    - [Changed](#changed)
    - [Fixed](#fixed)
    - [Build/CI](#buildci)
  - [[1.3.2] - 2026-03-08](#[132]---2026-03-08)
    - [Added](#added)
    - [Fixed](#fixed)
    - [Changed](#changed)
    - [Added](#added)
    - [Fixed](#fixed)
    - [Changed](#changed)
    - [Build/CI](#buildci)
    - [Notes](#notes)

<!-- T81-TOC:END -->


> **Source of Truth:** This document defines the **history of changes** released to the public. For future plans, see [../process/roadmaps-plans/ROADMAP.md](../process/roadmaps-plans/ROADMAP.md).

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

## [2.3.0] - 2026-11-27

See full release notes in [`docs/releases/v2.3.0.md`](../releases/v2.3.0.md).

### Added

- **Native C++23 Ed25519 Receipts & Wycheproof Hardening (`src/crypto/ed25519_bundle.cpp`):** RFC 8032 signing/verification + `verify_dag`, Wycheproof canonical `y < p` and 8 small-order torsion point rejections, cross-compiler GCC/Clang/MSVC support, and atomic offline forensic bundle export (`t81 receipt export`).
- **DPE + MCP Transactional Unification & Three-Plane Contract (`#888`, `#897`, `#901`):** `DpeMcpCoordinator` (`tx_begin`/`tx_commit`/`tx_abort`), KEV/advisory fail-closed MCP gating (`ERR_KEV_ADVISORY_BLOCKED`, `DEFER_ADVISORY_REVIEW`, `ERR_MANIFEST_DIGEST_MISMATCH`), Three-Plane `ERR_PLANE_BYPASS_DENIED` + `plane_transition` evidence schema, TLA+ `DpeBatchCommit` & `PlaneTransitionInvariant`, and cross-platform differential oracles (`cross_platform_differential_test.py`, `t81_three_plane_differential_test`).
- **M3 Python SDK, BitNet-2B Streaming & Enclave Runner (`#887`, `#888`, `#898`):** `PyProviderGovernor` governed Python SDK bindings, `AcceleratorBackend` abstraction, `--chunk-bytes` 2B BitNet streaming (`4.0×` vs INT8 storage) with Apple M2 `R3` Metal dispatch receipt, and experimental `t81 enclave run`.
- **Stratified AI OS Blueprint & Governance Hygiene (`#896`, `#899`, `#900`, `#851`, `#853`, `#854`, `#859`, `#902`):** `docs/architecture/AI_OS_STRATIFIED_BLUEPRINT.md` (`DEC-004`, Horizon D — *Planned / Design Target*), architecture & root docs reconciliation (19 broken links repaired), RFC index & ADR collision checker (`114` RFCs, `7` ADRs), CI `set -o pipefail` exit-code propagation, `SCG-08` Gate 1 reproduction audit record & warning cleanup, and 16 archived historical research reports under `docs/research/archive/`.

### BREAKING CHANGES

- **MCP filesystem confinement.** `t81 mcp serve` file tools resolve `..` and symlinks and refuse paths outside `--root` (the working directory when `--root` is omitted), including absolute paths and symlinks that leave the root. Percent-encoded dots and slashes stay literal name characters. An embedded NUL is rejected. Reads stop at 64 MiB. Migration: [mcp-serve-weightsload-fail-closed.md](../migration/mcp-serve-weightsload-fail-closed.md).
- **MCP policy required.** `t81.run_subgraph` and `t81.tx_commit` require `policy_path`. A commit without a policy leaves the backup in place.
- **Governed `WeightsLoad` fails closed.** With a `PolicyEngine` attached, `WeightsLoad` is a `SecurityFault` unless the tensor name is an exact entry of `allowed-tensor-hashes` or `allowed-ternary-model-hashes`. An empty allowlist, an empty name, or a model checksum that is not the tensor name does not admit the load. A VM with no policy still uses `AllowAllEngine`, and that ungoverned path is unchanged. Later model-backed ops still check `allowed-ternary-model-hashes` against the model checksum.

### Security

- Confined `t81 mcp serve` file reads and writes to the configured roots and closed the empty-name `WeightsLoad` allow when some other allowlist entry was present.

## [2.2.0] - 2026-11-22

See full release notes in [`docs/releases/v2.2.0.md`](../releases/v2.2.0.md).

### Added
- **Python SDK (`t81==2.2.0`, 31 PyPI files including CPython `3.14` wheels):** `CanonRef` digest reconstruction and `CanonRef.to_cli_handle()`, `StepLimitExceeded` pause/resume with `vm.halted` and `compile_and_run(..., max_steps=...)`, read-only `vm.axion_events` snapshots, dynamic tensor element-count validation, fixed-size tensor negative indexing, and `int()` conversion for `T81Int`/`BigInt`.
- **Post-`v2.2.0` `main` Integrations (`v2.2.0` → `8aa086906`):** TernaryOS v0.2 MCP multi-agent transactional isolation + `ai_usability_eval.py` (`23/23` adversarial trials, PRs `#861`/`#872`), TLA+ `TernaryOSTransactions` TLC CI (`14,227,489` states / `0` errors, PR `#862`), native `x86_64` AVX2 L3 & off-L3 DRAM sweeps (PRs `#863`/`#864`), `M1` `ProviderGovernor` (`t81.provider.evidence.v1`, `AT-01..AT-10`, PR `#865`), and `H06-MEM-04` Phase 3 sparse-aware Apple Metal/CPU regime dispatch (`PRs #873/#880/#882`).

## [2.1.0] - 2026-11-19

See full release notes in [`docs/releases/v2.1.0.md`](../releases/v2.1.0.md).

### Added
- **Hardened 15-Tool Native C++ MCP Server (`t81 mcp serve`):** `ScopedJsonParser`, `t81.verify_subgraph`, `t81.diff_check`, `t81.memory_search`, `t81.adapter_apply`, and fail-closed halting `t81.run_subgraph`.
- **`T81VM` Dual-Rail `(P, N)` `TMatMul` & `CanonFS` `SBDR1T2D` Caching:** NEON + AVX2 Dual-Rail popcount with block-zero activation skipping and lazy `CanonDualRailRhsCache` (`t81_dualrail_sbdr_vm_test`).
- **BitNet Safetensors Ingester:** `BF16` decoding, `absmean_gamma`, `--emit-sbdr`, and `--emit-sbdr2d` verified on `models/tiny-random-llama-exported.safetensors`.
- **Governance & CLI Parity:** Grouped `t81 --help` (`24` topics) and `18/18` CI governance scripts unmasked under `set -euo pipefail`.

## [2.0.0] - 2026-11-17

See full release notes in [`docs/releases/v2.0.0.md`](../releases/v2.0.0.md).

### Changed
- **Major Version & Public API Lock Baseline:** Aligned `CMakeLists.txt`, `pyproject.toml`, `include/t81/config.hpp`, and `PUBLIC_API_LOCK.json` to `2.0.0`; switched persistent `CanonFS` stores to `h1-` lowercase-hex (`t81-canonfs-hex-v1`); defaulted `AxionVerdict` to fail-closed `Deny`.
- **Native C++ MCP Server & Accountable Decision Bundles:** Initial `t81 mcp serve` (`~109 µs` warm `gate_check`), `t81 ai checkpoint-verify`, and draft `RFC-00D4` receipt sidecar (`11/11` gauntlet scenarios).

## [1.5.0] - 2026-03-14

### Added
- **Axion Governance Kernel Runtime**: Complete kernel runtime layer with service contracts, device arbitration, and fault management
- **RFC-00B4**: Axion userland service contract specification defining ABI, service boundaries, and lifecycle management
- **QEMU ARMv8 EFI Support**: Full ARMv8 EFI boot support with startup reports and artifact generation
- **Enhanced Axion Shell**: Object-native reads, durable inspection commands, session management, and typed input
- **Radix Page Table**: Implementation of Axion MMU with radix table diagnostics and permission faults
- **Executive Storage**: CanonFS-backed executable storage with auto-attachment and external repository binding
- **VirtualBox Support**: x86_64 VirtualBox HAL scaffolding with VMSVGA and E1000 guest bindings
- **Service Lifecycle**: Complete service suspend/resume lifecycle with health transitions and recovery reports
- **IPC Messaging**: Message-passing IPC with kernel runtime scheduler and device arbitration
- **Diagnostics**: Comprehensive kernel service diagnostics, fault reporting, and recovery status tracking

### Changed
- **Axion Naming**: Officially adopted "Axion" as the working name for the ternary-native OS
- **RFC Organization**: Moved Axion RFCs into the spec tree with normalized metadata
- **Experimental Layout**: Reorganized Axion experimental directory structure for better navigation
- **Documentation**: Updated root README to expose Axion v0.1.0-alpha status and architecture

### Fixed
- **Pager Work**: Coalesced duplicate pager work and added comprehensive backlog tracking
- **Memory Management**: Fixed allocation pathologies in T81BigInt division operations
- **Shell Stability**: Stabilized kernel service runtime and tightened request semantics

### Build/CI
- **Phase 4 Proofs**: Strengthened QEMU display, network, persistence, and recovery proofs
- **Testing**: Added comprehensive Axion kernel, shell, and MMU test coverage

## [1.3.2] - 2026-03-08

### Added
- `code profile` compile-and-profile workflow for `.t81` and `.tisc` inputs.
- `tisc stats`, `trace filter`, `determinism multi-run`, `ir validate`, `axion audit`, `env clean`, `weights export`, and the `tier` command family.
- `--dry-run` support for `canonfs rollback` and `canonfs gc`.

### Fixed
- `project build`, `project run`, and `project test` now dispatch correctly instead of falling through to `Unknown command: code`.
- `memory-stats` is accepted consistently by argument parsing and now emits the expected legacy alias warning.
- Help routing for `env check`, `env diag`, `env feedback`, and `project build/run/test` now resolves to the correct subcommand help.
- CLI help output now goes to `stdout`, restoring normal pipe and redirection behavior.
- Global parse errors no longer call `std::exit(1)` directly.
- `--json` failures now emit a JSON error envelope with schema `t81.error.v1`.

### Changed
- Help dispatch is deduplicated across `help` and `--help` entry paths.
- `lang` help now explicitly documents that it is a compatibility alias for `code` plus `ir`.
- CLI reference manual and contract tests were updated to match the shipped command surface and help/output behavior.

### Added
- Support for assignment to array indices (e.g., `arr[i] = x`) and object fields (e.g., `obj.field = x`).
- `T81Int::significant_trits()` method to determine the number of used trits.
- `std::span` optimization in `T81BigInt` Karatsuba multiplication, avoiding unnecessary vector copies.
- Fast path in `T81BigInt` multiplication and addition to avoid `Axion trap` logs for expected overflows.
- Implemented `gumbel_add` in `T81Prob` using `T81Entropy` for deterministic Gumbel noise injection.
- Regression tests for `T81BigInt` division allocation pathology (`t81_bigint_allocation_pathology_test`, `t81_bigint_allocation_guardrail_test`).
- Implemented `extended_gcd` and `modular_inverse` for `T81BigInt`, enabling basic cryptographic primitives.

### Fixed
- Allocation pathology in `T81BigInt::div_mod` (and `to_std_chunks`) by reserving vector capacity, preventing quadratic growth during division of large numbers.

### Changed
- Documentation baseline aligned to C++23-default reality across root/docs guides and status reports.
- Root `.tisc` artifacts moved into `examples/tisc/` to keep repository root clean.
- Repository audit snapshots refreshed (`docs/records/inventories/repo_tree*.txt`, `docs/records/inventories/repo_inventory.tsv`).
- Added RFC draft `RFC-0024` to track spec/governance wording alignment for C++23 default + C++20 compatibility lane.

### Build/CI
- Single-threaded local ritual support is documented and validated for host-stability workflows.

### Notes
- Historical completed work is retained in commit history and supporting docs (`../explanation/ANALYSIS.md`, `system-status.md`).
