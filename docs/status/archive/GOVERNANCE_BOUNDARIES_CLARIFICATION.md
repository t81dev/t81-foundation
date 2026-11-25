# Governance Boundaries Clarification

## Overview
This document clarifies the governance boundaries and deterministic profile enforcement for the T81VM and associated components.

Primary operational classification source:

- [GOVERNANCE_SURFACE_REGISTER.md](/docs/governance/GOVERNANCE_SURFACE_REGISTER.md)

## Boundaries

- Core Deterministic Profile (DCP) strictly isolates experimental features.
- Axion Governance Kernel is a governed non-DCP surface: it enforces policy and carries bounded determinism evidence, but it does not by itself expand the DCP boundary.
- Experimental TernaryOS + DPE epoch execution/audit parity is CI-enforced through `axion-epoch-determinism`, but that proof lane does not promote the broader Axion/TernaryOS runtime into DCP.
- I/O operations are strictly bounded and supervised by the kernel.
