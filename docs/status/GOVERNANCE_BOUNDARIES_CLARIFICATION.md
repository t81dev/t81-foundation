# Governance Boundaries Clarification

## Evidence review — 2026-10-27

Purpose: distinguish guarantees, admission and authorization. [Surface register](../governance/GOVERNANCE_SURFACE_REGISTER.md) and [determinism registry](../governance/DETERMINISM_SURFACE_REGISTRY.md) control classification.

| Property                        | Current boundary                                                                                                                                               |
| ------------------------------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Interpreter determinism         | Registered DCP behavior under defined input/configuration; not all execution or host timing.                                                                   |
| Cross-platform determinism      | Requires matched platform artifacts; not established by one local pass.                                                                                        |
| JIT equivalence                 | Bounded tests/fallbacks exist; whole-surface promotion unverified.                                                                                             |
| Policy admission                | CLI hash/marker checks admit fixed tasks; not evidence of every APL constraint inside the probe.                                                               |
| Execution/storage authorization | Configured VM/backend/driver paths enforce their specific checks. Empty storage hook explicitly selects bootstrap mode; externally serialized driver contract. |
| Integrity                       | Payload hash/read validation; read verification can be disabled.                                                                                               |
| Authenticity / truth            | Requires trusted signer/verification and semantic evidence beyond hashing.                                                                                     |
| Reproducibility                 | Requires pinned source, fixtures, commands and meaningful assertions.                                                                                          |

The retained sentence “I/O operations are strictly bounded and supervised by the kernel” is **Unsupported** as a repository-wide assertion: [driver bootstrap mode](../../include/t81/canonfs/canon_driver.hpp) and hosted CLI operations are explicit counterexamples to automatic universal supervision. No governance rule is changed by this qualification.

Review basis: audited main `c930b8c2ee7478782bf3c6d231bdd59984595ef2`. This editorial review does not grant promotion or release approval. See the [reconciliation report](STATUS_RECONCILIATION_2026-10-27.md) for executed checks and limitations.

## Retained record

## Overview
This document clarifies the governance boundaries and deterministic profile enforcement for the T81VM and associated components.

Primary operational classification source:

- [GOVERNANCE_SURFACE_REGISTER.md](/docs/governance/GOVERNANCE_SURFACE_REGISTER.md)

## Boundaries

- Core Deterministic Profile (DCP) strictly isolates experimental features.
- Axion Governance Kernel is a governed non-DCP surface: it enforces policy and carries bounded determinism evidence, but it does not by itself expand the DCP boundary.
- Experimental TernaryOS + DPE epoch execution/audit parity is CI-enforced through `axion-epoch-determinism`, but that proof lane does not promote the broader Axion/TernaryOS runtime into DCP.
- I/O operations are strictly bounded and supervised by the kernel.
