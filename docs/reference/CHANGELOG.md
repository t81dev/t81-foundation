# Changelog

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Changelog](#changelog)
  - [[Unreleased]](#[unreleased])
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
