# T81Lang Implementation Task Queue (2026-03)

Status: Active
Owner: @t81dev
Last Updated: 2026-02-25
Scope: Execution tasks derived from A1 drift decomposition

## Purpose

Convert T81Lang drift decomposition into executable engineering tasks tied to
spec anchors, code paths, and validation tests.

## Task Queue

| Task ID | Work Item | Spec Anchor | Code Surface | Validation Target | Target Date | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| A1-CODE-01 | Publish parser coverage matrix by section/production | `spec/t81lang-spec.md` section 1 + Appendix A | `lang/frontend/parser.cpp`, `lang/frontend/lexer.cpp` | `tests/cpp/frontend_parser_test.cpp`, `tests/cpp/test_parser_regression_audit.cpp`, `tests/cpp/frontend_parser_appendix_coverage_test.cpp` | 2026-03-03 | Completed (2026-02-25) |
| A1-CODE-02 | Tighten semantic/type coverage mapping to declared numeric/structural rules | `spec/t81lang-spec.md` section 2 and section 5 (Stage 3) | `lang/frontend/semantic_analyzer.cpp` | `tests/cpp/semantic_analyzer_numeric_test.cpp`, `tests/cpp/semantic_analyzer_option_result_test.cpp`, `tests/cpp/t81lang_conformance_baseline_test.cpp`, `tests/cpp/semantic_analyzer_stage3_rules_test.cpp` | 2026-03-06 | Completed (2026-02-25) |
| A1-CODE-03 | Codify deterministic compilation-profile invariants in status/spec trace docs (no runtime changes) | `spec/t81lang-spec.md` section 5 | `docs/status/VERIFIED_SURFACE_AUDIT.md`, related spec references | `tests/cpp/e2e_compile_determinism_test.cpp`, `tests/cpp/e2e_ast_ir_canonical_determinism_test.cpp`, `scripts/ci/t81lang_repro_gate.py` | 2026-03-06 | Completed (2026-02-25) |
| A1-CODE-04 | Expand reproducibility fixture coverage for currently high-drift grammar/semantic cases | `spec/t81lang-spec.md` sections 1, 2, 6 | `tests/fixtures/t81lang_determinism/` | `scripts/ci/t81lang_repro_gate.py`, `tests/cpp/e2e_compile_determinism_test.cpp`, `tests/cpp/e2e_ast_ir_canonical_determinism_test.cpp` | 2026-03-09 | Completed (2026-02-25) |
| A1-CODE-05 | Add conformance tests for uncovered match/loop/annotation edge semantics identified by matrix | `spec/t81lang-spec.md` sections 3 and 6 | frontend parser/semantic/IR paths | `tests/cpp/semantic_analyzer_match_test.cpp`, `tests/cpp/semantic_analyzer_loop_test.cpp`, `tests/cpp/e2e_match_expression_test.cpp`, `tests/cpp/t81lang_conformance_edge_semantics_test.cpp` | 2026-03-12 | Completed (2026-02-25) |
| A1-CODE-06 | Sync matrix and governance evidence after task closures | `spec/t81lang-spec.md` cross-section | status/audit artifacts | `docs/status/IMPLEMENTATION_MATRIX.md`, `docs/records/audits/2026-03-governance-review.md` | 2026-03-12 | Completed (2026-02-25) |

## Follow-On Queue (Post-A1)

| Task ID | Work Item | Spec Anchor | Code Surface | Validation Target | Target Date | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| A1B-CODE-01 | Add parser coverage for expression-form `if` paths to close remaining Appendix A control-flow parser gap evidence | `spec/t81lang-spec.md` section 1 + Appendix A (`if_expression`) | `lang/frontend/parser.cpp` | `tests/cpp/frontend_parser_appendix_coverage_test.cpp`, parser-focused frontend tests | 2026-03-18 | Completed (2026-02-25) |
| A1B-CODE-02 | Publish section-level semantic coverage index for section 2/5 rule anchors using existing conformance and semantic suites | `spec/t81lang-spec.md` sections 2 and 5 | `lang/frontend/semantic_analyzer.cpp` (evidence mapping only) | `tests/cpp/semantic_analyzer_*`, `tests/cpp/t81lang_conformance_baseline_test.cpp`, `tests/cpp/semantic_analyzer_stage3_rules_test.cpp` | 2026-03-22 | Completed (2026-02-25) |
| A1B-CODE-03 | Sync matrix and governance evidence after post-A1 follow-on updates | `spec/t81lang-spec.md` cross-section | status/audit artifacts | `docs/status/IMPLEMENTATION_MATRIX.md`, `docs/records/audits/2026-03-governance-review.md`, `docs/status/T81LANG_DRIFT_DECOMPOSITION_2026-03.md` | 2026-03-25 | Completed (2026-02-25) |

## Next-Cycle Queue (Post-A1/A1B)

| Task ID | Work Item | Spec Anchor | Code Surface | Validation Target | Target Date | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| A1C-CODE-01 | Publish section-level purity/control-flow semantic coverage index for sections 3 and 6 using existing semantic/conformance tests | `spec/t81lang-spec.md` sections 3 and 6 | `lang/frontend/semantic_analyzer.cpp` (evidence mapping only) | `tests/cpp/t81lang_conformance_edge_semantics_test.cpp`, `tests/cpp/semantic_analyzer_loop_test.cpp`, `tests/cpp/semantic_analyzer_match_test.cpp`, `tests/cpp/e2e_match_expression_test.cpp` | 2026-03-28 | Completed (2026-02-25) |
| A1C-CODE-02 | Publish section 7 Axion integration metadata evidence index using existing metadata and trace tests | `spec/t81lang-spec.md` section 7 | Axion metadata surfaces (evidence mapping only) | `tests/cpp/axion_loop_metadata_test.cpp`, `tests/cpp/axion_match_metadata_test.cpp`, `tests/cpp/e2e_axion_trace_test.cpp`, `tests/cpp/axion_policy_match_guard_test.cpp` | 2026-03-30 | Completed (2026-02-25) |
| A1C-CODE-03 | Sync matrix and governance evidence after A1C updates | `spec/t81lang-spec.md` cross-section | status/audit artifacts | `docs/status/IMPLEMENTATION_MATRIX.md`, `docs/records/audits/2026-03-governance-review.md`, `docs/status/T81LANG_DRIFT_DECOMPOSITION_2026-03.md` | 2026-03-31 | Completed (2026-02-25) |

## Post-A1C Queue (A1D Seed)

| Task ID | Work Item | Spec Anchor | Code Surface | Validation Target | Target Date | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| A1D-CODE-01 | Publish section 5 IR/lowering determinism evidence index for residual traceability gaps using existing frontend/e2e determinism suites | `spec/t81lang-spec.md` section 5 | frontend IR/lowering surfaces (evidence mapping only) | `tests/cpp/frontend_ir_generator_test.cpp`, `tests/cpp/e2e_compile_determinism_test.cpp`, `tests/cpp/e2e_ast_ir_canonical_determinism_test.cpp`, `scripts/ci/t81lang_repro_gate.py` | 2026-04-02 | Completed (2026-02-25) |
| A1D-CODE-02 | Publish section 8 stdlib alignment evidence index using existing CLI std fixture suites | `spec/t81lang-spec.md` section 8 | `lang/stdlib/std/` and CLI fixture evidence surfaces | `tests/cpp/cli_std_text_fixtures_test.cpp`, `tests/cpp/cli_std_bytes_fixtures_test.cpp`, `tests/cpp/cli_std_collections_fixtures_test.cpp`, `tests/cpp/cli_std_tensor_fixtures_test.cpp`, `tests/cpp/cli_std_runtime_fixtures_test.cpp`, `tests/cpp/cli_std_symbol_fixtures_test.cpp` | 2026-04-04 | Completed (2026-02-25) |
| A1D-CODE-03 | Sync matrix and governance evidence after A1D updates | `spec/t81lang-spec.md` cross-section | status/audit artifacts | `docs/status/IMPLEMENTATION_MATRIX.md`, `docs/records/audits/2026-03-governance-review.md`, `docs/status/T81LANG_DRIFT_DECOMPOSITION_2026-03.md` | 2026-04-05 | Completed (2026-02-25) |

## Post-A1D Queue (A1E Seed)

| Task ID | Work Item | Spec Anchor | Code Surface | Validation Target | Target Date | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| A1E-CODE-01 | Publish section 4 name-resolution/scoping evidence index for residual partial alignment surfaces | `spec/t81lang-spec.md` section 4 | frontend symbol and semantic surfaces (evidence mapping only) | `tests/cpp/semantic_analyzer_diagnostic_precision_test.cpp`, `tests/cpp/semantic_analyzer_diagnostic_location_test.cpp`, `tests/cpp/semantic_analyzer_cascade_suppression_test.cpp`, `tests/cpp/t81lang_conformance_baseline_test.cpp` | 2026-04-08 | Completed (2026-02-25) |
| A1E-CODE-02 | Publish residual section 3/6 alignment evidence addendum focusing on purity/effects metadata traceability | `spec/t81lang-spec.md` sections 3 and 6 | frontend semantic/control-flow surfaces (evidence mapping only) | `tests/cpp/t81lang_conformance_edge_semantics_test.cpp`, `tests/cpp/semantic_analyzer_loop_test.cpp`, `tests/cpp/semantic_analyzer_match_test.cpp`, `tests/cpp/e2e_match_expression_test.cpp` | 2026-04-10 | Completed (2026-02-25) |
| A1E-CODE-03 | Sync matrix and governance evidence after A1E updates | `spec/t81lang-spec.md` cross-section | status/audit artifacts | `docs/status/IMPLEMENTATION_MATRIX.md`, `docs/records/audits/2026-03-governance-review.md`, `docs/status/T81LANG_DRIFT_DECOMPOSITION_2026-03.md` | 2026-04-11 | Completed (2026-02-25) |

## Post-A1E Queue (A1F Seed)

| Task ID | Work Item | Spec Anchor | Code Surface | Validation Target | Target Date | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| A1F-CODE-01 | Publish section 7 guard/segment metadata traceability addendum for language-origin metadata surfaces | `spec/t81lang-spec.md` section 7 | Axion policy/metadata evidence surfaces (mapping only) | `tests/cpp/axion_policy_match_guard_test.cpp`, `tests/cpp/axion_policy_segment_event_test.cpp`, `tests/cpp/axion_segment_trace_test.cpp`, `tests/cpp/e2e_axion_trace_test.cpp` | 2026-04-14 | Completed (2026-02-25) |
| A1F-CODE-02 | Publish section 5 compilation-profile maintenance evidence addendum tied to reproducibility gate artifacts | `spec/t81lang-spec.md` section 5 | reproducibility and compile-determinism evidence surfaces (mapping only) | `scripts/ci/t81lang_repro_gate.py`, `tests/cpp/e2e_compile_determinism_test.cpp`, `tests/cpp/e2e_ast_ir_canonical_determinism_test.cpp`, `tests/cpp/t81lang_conformance_baseline_test.cpp` | 2026-04-16 | Completed (2026-02-25) |
| A1F-CODE-03 | Sync matrix and governance evidence after A1F updates | `spec/t81lang-spec.md` cross-section | status/audit artifacts | `docs/status/IMPLEMENTATION_MATRIX.md`, `docs/records/audits/2026-03-governance-review.md`, `docs/status/T81LANG_DRIFT_DECOMPOSITION_2026-03.md` | 2026-04-18 | Completed (2026-02-25) |

## Post-A1F Queue (A1G Seed)

| Task ID | Work Item | Spec Anchor | Code Surface | Validation Target | Target Date | Status |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| A1G-CODE-01 | Publish section 2/6 structural-type control-flow traceability addendum using existing Option/Result and match conformance suites | `spec/t81lang-spec.md` sections 2 and 6 | structural-type and control-flow evidence surfaces (mapping only) | `tests/cpp/semantic_analyzer_option_result_test.cpp`, `tests/cpp/e2e_option_result_test.cpp`, `tests/cpp/e2e_option_result_function_test.cpp`, `tests/cpp/e2e_match_expression_test.cpp` | 2026-04-21 | Completed (2026-02-25) |
| A1G-CODE-02 | Publish section 5 reproducibility-hash maintenance addendum tied to fixture-hash continuity artifacts | `spec/t81lang-spec.md` section 5 | reproducibility fixture/hash evidence surfaces (mapping only) | `scripts/ci/t81lang_repro_gate.py`, `tests/fixtures/t81lang_determinism/t81lang_repro_hash.txt`, `tests/fixtures/t81lang_determinism/t81lang_ast_ir_repro_hash.txt`, `tests/cpp/e2e_compile_determinism_test.cpp` | 2026-04-23 | Completed (2026-02-25) |
| A1G-CODE-03 | Sync matrix and governance evidence after A1G updates | `spec/t81lang-spec.md` cross-section | status/audit artifacts | `docs/status/IMPLEMENTATION_MATRIX.md`, `docs/records/audits/2026-03-governance-review.md`, `docs/status/T81LANG_DRIFT_DECOMPOSITION_2026-03.md` | 2026-04-25 | Completed (2026-02-25) |

## Execution Rule

Tasks in this queue must not alter freeze boundaries, determinism claim scope,
or CI policy. Any boundary-impacting change requires ADR/governance escalation.

## Cycle Progression Gate

No new T81Lang drift-reduction cycle queue (for example `A1D-*`) may be opened
until all tasks in the active cycle are marked `Completed` and the following
artifacts are synchronized:

1. `docs/status/T81LANG_DRIFT_DECOMPOSITION_2026-03.md`
2. `docs/status/IMPLEMENTATION_MATRIX.md`
3. `docs/records/audits/2026-03-governance-review.md`

Exceptions require explicit governance recording in the monthly audit artifact.

## Cross-References

- `docs/status/T81LANG_DRIFT_DECOMPOSITION_2026-03.md`
- `docs/status/EXECUTION_PLAN_2026-03.md`
- `docs/status/IMPLEMENTATION_MATRIX.md`
- `docs/records/audits/2026-03-governance-review.md`

## Versioning Statement

This queue is an operational planning artifact and does not override `/spec` or
governance authority.
