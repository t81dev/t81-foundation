# T81Lang Engineering Backlog (2026-03)

Status: Active
Owner: @t81dev
Last Updated: 2026-02-28
Source: Promotion-gate blocking scope, residual high-drift surfaces, and surface inventory gaps identified 2026-02-26..28

## Purpose

Define a ranked, implementation-oriented backlog to reduce T81Lang residual
drift from Draft-spec/Beta-implementation posture and sustain Beta readiness.

## Scope

Backlog items in this file are engineering execution targets (code/tests/tools)
derived from current status evidence and drift decomposition artifacts.

This artifact does not itself change semantics; implementation changes must
remain spec-first and freeze-safe.

## Ranking Method

Priority is determined by:

1. Determinism and release risk impact
2. Spec-alignment drift severity
3. Availability of concrete validation targets

## Ranked Backlog

| Rank | Item ID | Area | Problem Statement | Acceptance Criteria | Validation Target | Owner | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| 1 | BG-01 | Section 5 compile determinism pipeline | Deterministic compile-profile traceability remains partial at subsystem level despite strong evidence addenda; operational closure needs implementation-level hardening. | No known nondeterministic compile output regressions in mapped pipeline; traceability notes updated with concrete implementation links. | `tooling/cli/driver.cpp`, `tests/cpp/e2e_compile_determinism_test.cpp`, `tests/cpp/e2e_ast_ir_canonical_determinism_test.cpp`, `tests/cpp/e2e_enum_metadata_determinism_test.cpp`, `scripts/ci/t81lang_repro_gate.py` | @t81dev | Completed (2026-02-25) |
| 2 | BG-02 | Sections 3/6 control-flow semantics | Control-flow and purity/effects surfaces remain marked partial in matrix/decomposition inventory; implementation hardening is needed beyond evidence indexing. | Loop/match/purity rule behavior gaps reduced with targeted implementation + tests; no regressions in conformance suite. | `lang/frontend/semantic_analyzer.cpp`, `tests/cpp/t81lang_conformance_edge_semantics_test.cpp`, `tests/cpp/semantic_analyzer_loop_test.cpp`, `tests/cpp/semantic_analyzer_match_test.cpp`, `tests/cpp/e2e_match_expression_test.cpp` | @t81dev | Completed (2026-02-25) |
| 3 | BG-03 | Section 4 name-resolution/scoping | Name-resolution/scoping is still partial at subsystem posture despite diagnostic evidence coverage. | Resolution/scoping behavior gaps reduced with deterministic diagnostics retained and test coverage expanded as needed. | `lang/frontend/semantic_analyzer.cpp`, `tests/cpp/semantic_analyzer_match_test.cpp`, `tests/cpp/semantic_analyzer_diagnostic_precision_test.cpp`, `tests/cpp/semantic_analyzer_diagnostic_location_test.cpp`, `tests/cpp/semantic_analyzer_cascade_suppression_test.cpp`, `tests/cpp/t81lang_conformance_baseline_test.cpp` | @t81dev | Completed (2026-02-25) |
| 4 | BG-04 | Section 7 language-to-Axion metadata path | Metadata surfaces are partially aligned and evidence-backed; implementation consistency across all language-origin metadata paths still requires hardening. | Guard/segment metadata behavior is consistent across mapped frontend/runtime paths with no policy-trace regressions. | `tests/cpp/axion_policy_match_guard_test.cpp`, `tests/cpp/axion_policy_segment_event_test.cpp`, `tests/cpp/e2e_axion_trace_test.cpp` | @t81dev | Completed (2026-02-25) |
| 5 | BG-05 | Promotion readiness automation | Promotion decision remains manual; gate should be rerunnable with explicit pass/fail snapshot procedure. | Promotion-gate snapshot updated after each backlog checkpoint with blockers reduced or closed. | `scripts/governance/t81lang_promotion_gate_snapshot.py`, `docs/status/T81LANG_PROMOTION_GATE.md`, `docs/status/T81LANG_PROMOTION_GATE_SNAPSHOT.md`, `docs/status/IMPLEMENTATION_MATRIX.md`, `docs/records/audits/2026-03-governance-review.md` | @t81dev | Completed (2026-02-25) |
| 6 | BG-06 | Collection type determinism tests | `T81List` and `T81Tree` are now first-class in the T81Lang frontend but carry zero determinism test coverage per `docs/status/T81LANG_SURFACE_INVENTORY.md`. Risk: nondeterministic iteration or serialization order may go undetected. | Run-to-run determinism tests added for `T81List` and `T81Tree` (empty and populated construction, canonical serialization, iteration order stability). No regressions in `cli_std_collections_determinism_test`. | `tests/cpp/cli_std_collections_determinism_test.cpp`, `tests/determinism/test_containers.cpp`, `tests/fixtures/t81lang_std_collections/` | @t81dev | Open |
| 7 | BG-07 | T81BigInt arbitrary-precision gap | VM aliases `bigint` operations to 64-bit integer opcodes (`ADD`, `SUB`, etc.); literals beyond 64-bit are silently truncated. Gap documented in `docs/status/T81LANG_SURFACE_INVENTORY.md`. | Define and implement `BIGINT_ADD` / `BIGINT_SUB` native opcodes (or promotion path); add truncation-detection test for >64-bit literals. No ISA freeze changes without ADR. | `core/vm/vm.cpp`, `include/t81/isa/opcodes.hpp`, `lang/frontend/ir_generator.hpp`, `tests/cpp/` | @t81dev | Open |
| 8 | BG-08 | T81Complex binary pool serialization | `T81Complex` is supported in the VM via `MAKE_COMPLEX` but has no binary pool serialization in `core/isa/binary_io.cpp`, blocking persistence of complex-valued constants in program binaries. | `T81Complex` values round-trip through binary emit/load without data loss; existing runtime tests remain green. | `core/isa/binary_io.cpp`, `core/isa/binary_emitter.cpp`, `tests/cpp/` | @t81dev | Open |
| 9 | BG-09 | T81Graph lang-side canonical serialization | Native C++ `T81Graph` implements `serialize_canonical()` but the language runtime does not invoke it — graph values lowered from T81Lang use a handle API that bypasses canonical serialization. Gap documented in `docs/status/T81LANG_SURFACE_INVENTORY.md`. | T81Lang graph literals and operations produce a stable canonical serialization signature verifiable across runs; `tests/fixtures/t81lang_std_collections/` graph fixtures pass determinism checks. | `lang/frontend/ir_generator.hpp`, `include/t81/types/T81Graph.hpp`, `tests/fixtures/t81lang_std_collections/` | @t81dev | Open |
| 10 | BG-10 | Determinism tests for newly exposed types | `T81Quaternion`, `T81Prob`, and `T81Qutrit` were exposed to T81Lang in 2026-02-28 surface hardening but carry no determinism tests (per surface inventory). `T81Uint` is also listed as UNKNOWN. | Determinism tests added for construction and round-trip serialization of each type; surface inventory status updated from NO/UNKNOWN to PARTIAL or YES. No DCP scope expansion without registry update. | `tests/cpp/`, `tests/determinism/`, `docs/status/T81LANG_SURFACE_INVENTORY.md` | @t81dev | Open |

## Execution Constraints

1. No CI policy modification in this backlog.
2. No freeze-boundary weakening.
3. No determinism claim expansion beyond registry-verified surfaces.
4. Any boundary-impacting change requires ADR/governance escalation.

## Cross-References

- `docs/status/T81LANG_PROMOTION_GATE.md`
- `docs/status/T81LANG_DRIFT_DECOMPOSITION_2026-03.md`
- `docs/status/T81LANG_IMPLEMENTATION_TASK_QUEUE_2026-03.md`
- `docs/status/IMPLEMENTATION_MATRIX.md`
- `docs/status/T81LANG_SURFACE_INVENTORY.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`

## Versioning Statement

This backlog is an operational engineering control artifact and does not
override `/spec`, freeze policy, or determinism registry authority.
