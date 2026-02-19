# Documentation Index

> **Source of Truth:** This document maps the documentation hierarchy for T81 Foundation. The [**Codebase**](https://github.com/t81dev/t81-foundation) is the absolute ground truth.

**Last Updated:** February 19, 2026

## 1. Recent Updates
**New in Feb 2026:** System hardened with SHA3-512 corrections (#224), strict Package Init sanitization (#225), and tightened AST/IR reproducibility gates (#226). See [System Status](system-status.md) for details.

## 2. Governance & Policy

- **[GOVERNANCE.md](GOVERNANCE.md):** Project governance model and decision-making process.
- **[MAINTAINERS.md](MAINTAINERS.md):** List of current maintainers and roles.
- **[SUPPORT.md](SUPPORT.md):** Support channels and contact information.
- **[RISKS.md](RISKS.md):** Known risks and mitigation strategies.
- **[VERSIONING.md](../VERSIONING.md):** Release and compatibility policy.

## 3. Technical Guides

- **[C++ Quickstart](cpp-quickstart.md):** Hands-on guide to building and running tests.
- **[System Status](system-status.md):** Current implementation dashboard and hardening report.
- **[Research Guide](research-guide.md):** Deep dive into T81 research goals.
- **[AI Quickstart](ai-quickstart.md):** Using T81 with AI models.
- **[System Integration](system-integration.md):** Integrating T81 into larger systems.
- **[CI/CD](ci.md):** Continuous Integration and deployment.
- **[Runtime Boundary](runtime-semantics-boundary.md):** Defining the boundary between spec and implementation.
- **[EVIDENCE.md](EVIDENCE.md):** Proof of claims matrix.
- **[REPRODUCIBILITY.md](REPRODUCIBILITY.md):** Instructions for reproducible builds.
- **[TESTING.md](TESTING.md):** Testing strategy and taxonomy.

## 4. Reference

- **[Audits](audits/):** Repository audits.
- **[Benchmarks](benchmarks.md):** Performance benchmarks.
- **[Navigation Map](navigation.md):** Detailed map of the documentation tree.

## About this directory

This directory contains the core documentation for T81 Foundation architecture, operations, and user/developer guides.

**Validation Ritual:**
Useful checks before merging doc-heavy changes:
```bash
python3 scripts/ci/check_architecture_targets.py
python3 scripts/check-runtime-contract-sync.py
```
