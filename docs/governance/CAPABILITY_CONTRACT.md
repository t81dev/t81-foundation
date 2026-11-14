# T81 Capability Contract & Governance

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Capability Contract & Governance](#t81-capability-contract-&-governance)
  - [Capability Contract](#capability-contract)
    - [1. Determinism](#1-determinism)
    - [2. Stability](#2-stability)
    - [3. Security](#3-security)
    - [4. Contract Authority](#4-contract-authority)
  - [Governance Model](#governance-model)
    - [Roles](#roles)
    - [Decision Making](#decision-making)

<!-- T81-TOC:END -->


## Capability Contract

The T81 Foundation is committed to providing a stable, secure, and deterministic platform. This contract defines the guarantees we provide to users and developers.

### 1. Determinism
Determinism guarantees are scoped to verified surfaces defined by governance artifacts (especially the determinism surface registry and CI repro gates). Determinism is not globally guaranteed for all runtime paths or all hardware/libc combinations.

### 2. Stability
*   **TISC ISA**: Versioned and frozen. Breaking changes require a major version bump.
*   **Public APIs**: Semantically versioned. Deprecation warnings provided one cycle in advance.

### 3. Security
Capability-based runtime controls are enforced by Axion within process-level software boundaries. T81 does not currently provide hardware isolation or OS-level sandboxing as part of the core runtime guarantee.

### 4. Contract Authority
The authoritative implementation-bound capability statement is:

* `docs/reference/CAPABILITY_CONTRACT.md`

If this governance summary conflicts with the reference capability contract, the reference capability contract governs.

## Governance Model

The project is governed by a meritocratic process.

### Roles
*   **Maintainers**: Review and merge PRs, steer technical direction.
*   **Contributors**: Submit PRs, report issues.
*   **Users**: Provide feedback.

### Decision Making
Technical decisions are made through the RFC (Request for Comments) process.
