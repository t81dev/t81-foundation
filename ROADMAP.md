# Project Roadmap

**Last Updated:** December 4, 2025

## 1. Vision for v1.0

The v1.0 release of the T81 Foundation will deliver a stable, documented, and production-ready implementation of the core T81 stack. The target audience is systems developers and AI/PL researchers who need a deterministic, auditable platform for advanced computation.

"Done" for a v1.0 component means it is fully implemented per the spec, comprehensively tested, documented, and builds cleanly.

## 2. Current Status (Beta)

The project is in a healthy beta state. The T81Lang compiler is feature-complete, the core numerics are robust, and the `t81` command-line tool provides a solid developer experience.

- **Strengths:** The C++20 compiler frontend is the project's core strength. It implements the full T81Lang specification, including a comprehensive semantic analysis pass that performs full type checking for all language features, including generics like `Option[T]` and `Result[T, E]`. The `t81` CLI provides `compile`, `run`, and `check` commands, backed by a robust error-reporting system.
- **Weaknesses:** While the compiler is mature, the runtime systems are not. The HanoiVM's memory model is still under development, and its fault-handling capabilities are incomplete. The Axion Kernel and CanonFS remain non-functional stubs.

For a detailed breakdown, see the [**System Status Report**](docs/system-status.md) and [`ANALYSIS.md`](ANALYSIS.md).

## 3. Strategic Priorities

With the compiler now stable, the critical path to v1.0 has shifted to the runtime environment.

- **[P0] HanoiVM & TISC Runtime:** The highest priority is to harden the HanoiVM. This involves implementing the full memory model as defined in the specification, adding deterministic fault handling for all illegal operations, and integrating the Axion Kernel hooks into the VM's main execution loop.
- **[P1] Axion Kernel & CanonFS:** Once the VM is stable, the next priority is to implement the Axion Kernel and CanonFS. This will transform them from stubs into functional components that can enforce the system's safety and determinism guarantees.
- **[P2] Language & Toolchain Polish:** While the compiler is feature-complete, there is still room for improvement. This includes expanding the parser to support planned keywords like `loop` and `match`, improving the `IRGenerator`'s output, and adding more end-to-end tests.

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
