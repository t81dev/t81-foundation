# Experimental Surface Inventory

**Status:** Active Inventory
**Last Updated:** 2026-02-25
**Reference:** `spec/supplemental/deterministic-core-profile-v1.md`

This document lists all components, features, and subsystems that are **NOT**
part of the frozen, verified core. These areas are subject to change without
notice, are out of DCP scope, and are non-verified unless promoted through
governance and registry status upgrade.

## Experimental Inventory

| Component | Location | Freeze Scope | Determinism Scope | Stability |
| :--- | :--- | :--- | :--- | :--- |
| **Cognitive Tiers** | `experimental/tiers/cog/`, `experimental/tiers/` | None (Evolving) | Non-verified (Excluded from DCP) | **Experimental** |
| **Hanoi VM Kernel** | `experimental/hanoi/` | None (Evolving) | Non-verified (Excluded from DCP) | **Experimental** |
| **JIT Compiler** | `runtime/jit/jit_compiler.cpp` | None (Experimental) | Non-verified unless explicitly upgraded in registry | **Alpha / Stub** |
| **Distributed Compute** | `experimental/distributed/` | None (Network) | Non-verified (Excluded from DCP) | **Experimental** |
| **Experimental Headers** | `include/t81/experimental/` | None | Non-verified (Excluded from DCP) | **Experimental** |
| **Notebooks** | `notebooks/` | None | Non-verified (Example scope) | **Example Only** |
| **Examples** | `examples/` | None | Non-verified (Example scope) | **Example Only** |

## Isolation Status

### 1. JIT Compiler
*   **Status**: Present in `runtime/jit/jit_compiler.cpp` but disabled by default.
*   **Isolation**: Must be explicitly enabled via build flag or runtime config.
*   **Risk**: High determinism risk if enabled.

### 2. Cognitive Tiers (Axion/Hanoi)
*   **Status**: Partially implemented in `experimental/tiers/cog/` and `experimental/hanoi/`.
*   **Isolation**: Separate namespace `t81::cog`, `t81::hanoi`.
*   **Risk**: Low impact on core VM unless invoked.

### 3. Distributed Compute
*   **Status**: Headers exist (`include/t81/experimental/distributed/distributed.hpp`), implementation stubbed.
*   **Isolation**: Dependent on network libraries not linked in core.
*   **Risk**: None (compile-time isolation).

## Deterministic Core Boundary

The modules in this document are explicitly out of deterministic core profile
scope (`spec/supplemental/deterministic-core-profile-v1.md`) and are non-verified by
default. Promotion requires governance review and determinism registry status
upgrade.

## Governance Rule

Per `docs/governance/FREEZE_ENFORCEMENT.md`:
> Experimental components MUST NOT be linked from normative specs or core architecture docs as a dependency.
> They MUST NOT affect the behavior of the `DETERMINISTIC_CORE_PROFILE` unless explicitly opted-in.
