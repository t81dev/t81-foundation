# Project Roadmap

**Last Updated:** December 1, 2025

## 1. Vision for v1.0

The v1.0 release of the T81 Foundation will deliver a stable, documented, and production-ready implementation of the core T81 stack. The target audience is systems developers and AI/PL researchers who need a deterministic, auditable platform for advanced computation.

"Done" for a v1.0 component means it is fully implemented per the spec, comprehensively tested, documented, and builds cleanly.

## 2. Current Status (Early-Beta with Emerging Stability)

The project remains early-beta, but the stack is stabilizing: the compiler frontend is complete, the numeric core continues to harden, and systemic documentation is aligned with implementation artifacts.

- **Strengths:** A fully modern C++20 toolchain drives the repo, and the T81Lang frontend now includes a lexer, parser, and semantic pass that already enforces core symbol resolution rules. The emerging `t81` CLI, parser diagnostics, and documented build flow make regression testing and experimentation reliable. Core numerics now implement the canonical integer, float, tensor, and data structure families described in `spec/t81-data-types.md`.
- **Weaknesses:** Type checking (especially inference for `Option`, `Result`, and higher-order generics) is still being ratcheted up, the HanoiVM memory model and deterministic faults remain incomplete, and Axion Kernel/CanonFS implementations are placeholders. More work is needed to close the remaining canonical numeric and VM semantics gaps.

For a detailed breakdown, see the [**System Status Report**](docs/system-status.md) and [`ANALYSIS.md`](ANALYSIS.md).

## 3. Strategic Priorities

To reach v1.0, development is organized into three priority tiers. Work on a higher-priority tier is the primary blocker for all lower-priority tiers.

- **[P0] T81Lang Compiler:** The critical path remains the compiler frontend. Finish the semantic analyzer’s type checking/inference (especially around `Option`, `Result`, and generics), broaden the parser to the latest spec keywords (`loop`, `match`, etc.), and ship the `t81` CLI helpers (`compile`, `run`, `repl`) so contributors can exercise the compiler end-to-end.
- **[P1] HanoiVM & TISC Runtime:** Harden the HanoiVM memory model, add deterministic fault handling, and weave the Axion hooks (`AXREAD`, `AXSET`, instruction counting) into the dispatch loop so runtime safety contracts can be exercised from `t81` binaries.
- **[P2] Axion Kernel & Supporting Systems:** Replace the Axion Kernel and CanonFS stubs with real implementations, finish the documentation/annotated specs that tie them to `spec/axion-kernel.md` & `spec/canonfs-spec.md`, and expand the tutorials/docs for tensor/VM usage so that the stack is demonstrably usable.

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
