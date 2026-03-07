# Changelog

> **Source of Truth:** This document defines the **history of changes** released to the public. For future plans, see [../process/roadmaps-plans/ROADMAP.md](../process/roadmaps-plans/ROADMAP.md).

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

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
