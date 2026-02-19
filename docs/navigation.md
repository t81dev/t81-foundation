# T81 Foundation Navigation Map

> **Source of Truth:** This document maps the living entry points and archived material in the documentation tree.

**Last Updated:** February 19, 2026

## 1. Primary User Pathways

- **Getting Started:**
  - [`docs/index.md`](index.md): The main documentation hub.
  - [`docs/cpp-quickstart.md`](cpp-quickstart.md): Build and run the C++ codebase.
  - [`docs/ai-quickstart.md`](ai-quickstart.md): Using T81 with AI models.
  - [`docs/system-status.md`](system-status.md): Current implementation status and hardening report.

- **Specification Layers:**
  - [`spec/index.md`](../spec/index.md): The Master Specification Index.
  - [`spec/tisc-spec.md`](../spec/tisc-spec.md): TISC Instruction Set Architecture.
  - [`spec/t81vm-spec.md`](../spec/t81vm-spec.md): Virtual Machine specification.
  - [`spec/t81lang-spec.md`](../spec/t81lang-spec.md): T81 Language specification.
  - [`spec/axion-kernel.md`](../spec/axion-kernel.md): Axion Policy Engine.

- **Semantics & Governance:**
  - [`docs/runtime-semantics-boundary.md`](runtime-semantics-boundary.md): Foundation vs. VM contract ownership.
  - [`docs/terminology-alignment.md`](terminology-alignment.md): Canonical term map.
  - [`GOVERNANCE.md`](../GOVERNANCE.md): Decision-making process.

- **Architecture & APIs:**
  - [`ARCHITECTURE.md`](../ARCHITECTURE.md): System design overview.
  - [`docs/guides/public-api-overview.md`](guides/public-api-overview.md): C++ API reference.

## 2. Domain-Focused Indexes

- **Guides:** See [`docs/guides/README.md`](guides/README.md) for a categorized Table of Contents (CLI, Axion, Semantics, Weights).
- **Axion:** See [`docs/guides/axion/README.md`](guides/axion/README.md) for policy authoring and trace analysis.
- **Benchmarks:** See [`docs/benchmarks.md`](benchmarks.md) for the latest performance results.

## 3. Artifacts & Tests

- **Axion Logs:** Located in [`build/artifacts/`](../build/artifacts/) (local builds).
- **Regression Tests:** Located in [`tests/`](../tests/).

## 4. Archive & Cleanup

- **Archived Files:** Files ending in `.archived` under `docs/archive/` are preserved for historical reference but should not be treated as current documentation.
- **Search:** Use the [`docs/search/index.html`](search/index.html) page (after building the site) or `grep` to find specific topics.
