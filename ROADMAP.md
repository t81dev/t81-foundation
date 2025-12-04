# Project Roadmap

**Last Updated:** December 4, 2025

## 1. Vision for v1.0

The v1.0 release of the T81 Foundation will deliver a stable, documented, and production-ready implementation of the core T81 stack. The target audience is systems developers and AI/PL researchers who need a deterministic, auditable platform for advanced computation.

"Done" for a v1.0 component means it is fully implemented per the spec, comprehensively tested, documented, and builds cleanly.

## 2. Current Status (Beta)

The project is in a healthy beta state. The T81Lang compiler is now fully implemented, unit-tested, and emits Axion-friendly metadata for the newly landed match/loop guards, and the CLI tooling (`t81 compile`, `t81 run`, `t81 check`, `t81 repl`) exposes the deterministic guard trace described in the Axion specs.

- **Strengths:** The compiler frontend now supports `loop`/`match`, Option/Result lowering, enum guard metadata, and CLI-level documentation for Axion logs. The full build + `ctest --test-dir build --output-on-failure` suite passes, so the entire P0 compiler path can be considered stable and shipped.
- **Weaknesses:** Runtime components still need work—HanoiVM’s memory model, deterministic fault handling, Axion kernel, and CanonFS are the remaining gaps preventing a v1.0 release.

For a detailed breakdown, see the [**System Status Report**](docs/system-status.md) and [`ANALYSIS.md`](ANALYSIS.md).

## 3. Strategic Priorities

With the compiler/CLI path (former P0) now stabilized, the critical path to v1.0 has shifted to the runtime environment while preserving the compiler documentation/test guarantees already in place.

- **[P0] HanoiVM & TISC Runtime:** Harden the HanoiVM—implement the full memory model, finish deterministic fault handling for every illegal operation, and ensure the Axion kernel hooks integrate cleanly with its execution loop (including the guard-aware metadata we now emit).  
- **[P1] Axion Kernel & CanonFS:** Once the VM is stable, build out Axion policies and CanonFS, turning those stubs into fully functional subsystems that enforce the stack’s determinism guarantees through policy-driven constraints.
- **[P2] Language & Toolchain Polish:** Preserve the compiler/documentation quality by keeping the CLI guides, Axion trace samples, and regression suites in sync as new features or optimizations emerge.

A detailed list of actionable tasks for these workstreams can be found in [`TASKS.md`](TASKS.md).

## 4. Release Phases

### Phase 1: Compiler Feature Completeness

- **Goal:** Ship a spec-compliant T81Lang compiler with rich diagnostics and the `t81` CLI experience.
- **Exit Criteria:** The compiler parses and type-checks the entire grammar, includes coverage for `loop`/`match` control flow, and contributes end-to-end tests for each major language feature. Compiler builds must succeed via `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release` and `cmake --build build --parallel` before any release candidate.

### Phase 2: Runtime Hardening & Integration

- **Goal:** Harden the VM and integrate runtime safety layers such as Axion while validating through the required `ctest --test-dir build --output-on-failure`.
- **Exit Criteria:** The HanoiVM memory model is complete, Axion kernel/CanonFS enforce deterministic safety (instruction counters, recursion guards), and the VM passes deterministic fault and stress tests.

### Phase 3: Public v1.0 Release

- **Goal:** Lock down documentation, tutorials, release scripts, and contributions so the stack is ready for external adoption.
- **Exit Criteria:** All v1.0 Definition of Done items are complete, documentation is aligned with the latest implementation, and the release checklist (build/tests/ctest runs) has been executed on every candidate.
