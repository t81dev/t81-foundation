# Project Roadmap

**Last Updated:** November 28, 2025

## 1. Vision for v1.0

The v1.0 release of the T81 Foundation will deliver a stable, documented, and production-ready implementation of the core T81 stack. The target audience is systems developers and AI/PL researchers who need a deterministic, auditable platform for advanced computation.

"Done" for a v1.0 component means it is fully implemented per the spec, comprehensively tested, documented, and builds cleanly.

## 2. Current Status (Late-Alpha)

The project is in a late-alpha stage.

- **Strengths:** The project has a clear specification, a modern C++20 toolchain, and a robust, well-tested suite of core numeric types (`T81Int`, `Fraction`).
- **Weaknesses:** The primary gap is the **T81Lang compiler**, which lacks a semantic analysis and type-checking pass. Critical components like the **Axion Kernel** and **CanonFS** are non-functional stubs. The VM's memory model is also a primitive placeholder.

For a detailed breakdown, see the [**System Status Report**](docs/system-status.md) and [`ANALYSIS.md`](ANALYSIS.md).

## 3. Strategic Priorities

To reach v1.0, development is organized into three priority tiers. Work on a higher-priority tier is the primary blocker for all lower-priority tiers.

- **[P0] T81Lang Compiler:** Fully implement the C++20 compiler to match the `t81lang-spec.md`. This is the critical path to v1.0. The immediate focus is on building the semantic analysis and type-checking pass.
- **[P1] HanoiVM & TISC Runtime:** Harden the virtual machine by implementing the full memory model, integrating the Axion kernel, and improving fault handling.
- **[P2] Axion Kernel & Supporting Systems:** Transform the Axion Kernel and CanonFS from stubs into functional components that can enforce the safety and storage policies defined in their respective specs.

A detailed list of actionable tasks for these workstreams can be found in [`TASKS.md`](TASKS.md).

## 4. Release Phases

### Phase 1: Compiler Feature Completeness

- **Goal:** Bring the T81Lang compiler to full spec compliance.
- **Exit Criteria:** The compiler can parse and **type-check** the entire T81Lang grammar. End-to-end tests exist for every major language feature. The compiler produces high-quality error messages.

### Phase 2: Runtime Hardening & Integration

- **Goal:** Harden the VM, integrate the Axion kernel, and improve performance.
- **Exit Criteria:** The Axion Kernel is integrated with the VM and enforces basic determinism policies (e.g., instruction counting). The VM's memory model is fully implemented.

### Phase 3: Public v1.0 Release

- **Goal:** Finalize documentation, tutorials, and release infrastructure.
- **Exit Criteria:** All items in the v1.0 "Definition of Done" are complete. The project is ready for public consumption by the target audience.
