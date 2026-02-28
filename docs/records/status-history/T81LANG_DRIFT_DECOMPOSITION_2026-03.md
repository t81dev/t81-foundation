# T81Lang Drift Decomposition (2026-03)

Status: Active
Owner: @t81dev
Last Updated: 2026-02-25
Target Completion: 2026-03-12 (A1 planning milestone)

## Purpose

Decompose T81Lang high-drift status into concrete milestones with measurable
status targets and governance evidence paths.

## Scope

This decomposition addresses status drift between:

- `spec/t81lang-spec.md`
- implementation surfaces under `lang/frontend/` and `lang/stdlib/`
- determinism/reproducibility evidence in CI and fixture gates

It does not redefine T81Lang semantics, freeze boundaries, or DCP scope.

## Baseline Signals

- `docs/status/IMPLEMENTATION_MATRIX.md`: T81Lang marked Experimental / High drift.
- `docs/status/VERIFIED_SURFACE_AUDIT.md`: compiler bytecode emission listed as
  Partial Traceability with a spec-gap note for deterministic compilation
  profile details.
- `scripts/ci/t81lang_repro_gate.py` and
  `tests/fixtures/t81lang_determinism/`: active reproducibility evidence path.

## Milestone Checklist

### M1 — Drift Surface Inventory Lock

- Target Date: 2026-03-03
- Objective:
  - Freeze a documented inventory of T81Lang spec sections vs implementation
    coverage categories (implemented / partial / missing / experimental).
- Acceptance Criteria:
  - Inventory table committed in this document.
  - Table references concrete code or test evidence for each entry.
- Evidence:
  - This file
  - `docs/status/IMPLEMENTATION_MATRIX.md`

### M2 — Deterministic Compilation Profile Gap Plan

- Target Date: 2026-03-06
- Objective:
  - Convert current verified-surface audit gap statement into a bounded action
    plan for deterministic compilation-profile documentation.
- Acceptance Criteria:
  - Planned spec-update scope documented (no semantic expansion in this step).
  - Evidence path for future implementation/test linkage identified.
- Evidence:
  - `docs/status/VERIFIED_SURFACE_AUDIT.md`
  - `spec/t81lang-spec.md` (planned section target only)

### M3 — Conformance and Repro Evidence Mapping

- Target Date: 2026-03-09
- Objective:
  - Map existing T81Lang conformance and reproducibility checks to drift
    categories so status can be objectively updated.
- Acceptance Criteria:
  - Explicit mapping of fixture gate, conformance test(s), and frontend tests to
    decomposition categories.
  - Coverage gaps captured as follow-up items, not claims.
- Evidence:
  - `scripts/ci/t81lang_repro_gate.py`
  - `tests/fixtures/t81lang_determinism/`
  - `tests/cpp/t81lang_conformance_baseline_test.cpp`
  - `tests/cpp/frontend_*`

### M4 — Matrix and Governance Sync Update

- Target Date: 2026-03-12
- Objective:
  - Update status artifacts to reflect decomposed milestones and current
    progress state.
- Acceptance Criteria:
  - `IMPLEMENTATION_MATRIX.md` T81Lang notes reference this decomposition.
  - March governance review note includes A1 progress snapshot.
- Evidence:
  - `docs/status/IMPLEMENTATION_MATRIX.md`
  - `docs/records/audits/2026-03-governance-review.md`

## Milestone Status Snapshot

- M1 Drift Surface Inventory Lock: Completed (2026-02-25, refreshed 2026-02-25 with Appendix A parser coverage matrix)
- M2 Deterministic Compilation Profile Gap Plan: Completed (2026-02-25)
- M3 Conformance and Repro Evidence Mapping: Completed (2026-02-25)
- M4 Matrix and Governance Sync Update: Completed (2026-02-25)

## A1 Closure State (A1-CODE-06)

Matrix and governance synchronization for this decomposition are complete as of
2026-02-25. The implementation queue, matrix notes, and March governance review
snapshot now reflect M1-M4 closure status without changing spec authority or
determinism claim boundaries.

## Decomposition Table (M1 Inventory Lock)

| Area | Spec Anchor | Implementation/Surface | Coverage Category | Evidence |
| :--- | :--- | :--- | :--- | :--- |
| Core grammar and precedence | `spec/t81lang-spec.md` section 1 and Appendix A | `lang/frontend/lexer.cpp`, `lang/frontend/parser.cpp` | Partial | `tests/cpp/frontend_lexer_test.cpp`, `tests/cpp/frontend_parser_test.cpp`, `tests/cpp/test_parser_regression_audit.cpp` |
| Type system and generic syntax | `spec/t81lang-spec.md` section 2 | `lang/frontend/semantic_analyzer.cpp` | Partial | `tests/cpp/semantic_analyzer_generic_test.cpp`, `tests/cpp/frontend_parser_generics_test.cpp`, `tests/cpp/frontend_parser_legacy_rejection_test.cpp` |
| Structural types (`Option` / `Result`) | `spec/t81lang-spec.md` section 2.5 and section 6.2 | parser/semantic/IR frontend surfaces | Implemented (bounded) | `tests/cpp/semantic_analyzer_option_result_test.cpp`, `tests/cpp/e2e_option_result_test.cpp`, `tests/cpp/e2e_option_result_function_test.cpp` |
| Purity/effects and tier annotations | `spec/t81lang-spec.md` section 3 | parser + semantic metadata surfaces | Partial | `tests/cpp/t81lang_conformance_baseline_test.cpp`, `tests/cpp/semantic_analyzer_loop_test.cpp` |
| Name resolution and scoping | `spec/t81lang-spec.md` section 4 | `lang/frontend/symbol_table.cpp`, semantic analyzer | Partial | `tests/cpp/semantic_analyzer_diagnostic_precision_test.cpp`, `tests/cpp/semantic_analyzer_diagnostic_location_test.cpp` |
| Deterministic compile pipeline and lowering | `spec/t81lang-spec.md` section 5 | parser/semantic/IR generator + CLI compile path | Partial (traceability gap remains) | `tests/cpp/frontend_ir_generator_test.cpp`, `tests/cpp/e2e_compile_determinism_test.cpp`, `scripts/ci/t81lang_repro_gate.py` |
| Control flow (`if`/`match`/`loop`) | `spec/t81lang-spec.md` section 6 | parser/semantic/IR control-flow handling | Partial | `tests/cpp/e2e_if_statement_test.cpp`, `tests/cpp/e2e_match_expression_test.cpp`, `tests/cpp/e2e_loop_statement_test.cpp`, `tests/cpp/semantic_analyzer_match_test.cpp` |
| Axion integration metadata | `spec/t81lang-spec.md` section 7 | metadata and policy-related frontend paths | Partial | `tests/cpp/axion_loop_metadata_test.cpp`, `tests/cpp/axion_match_metadata_test.cpp`, `tests/cpp/e2e_axion_trace_test.cpp` |
| Stdlib surface alignment | `spec/t81lang-spec.md` section 8 | `lang/stdlib/std/*.t81` + CLI fixtures | Partial | `tests/cpp/cli_std_text_fixtures_test.cpp`, `tests/cpp/cli_std_bytes_fixtures_test.cpp`, `tests/cpp/cli_std_collections_fixtures_test.cpp`, `tests/cpp/cli_std_tensor_fixtures_test.cpp`, `tests/cpp/cli_std_runtime_fixtures_test.cpp`, `tests/cpp/cli_std_symbol_fixtures_test.cpp` |

## Appendix A Parser Coverage Matrix (A1-CODE-01)

| Appendix Production Area | Parser Coverage Status | Evidence |
| :--- | :--- | :--- |
| A.1 lexical elements (`identifier`, integer/float/string literals, comments) | Covered | `tests/cpp/frontend_lexer_test.cpp`, `tests/cpp/frontend_parser_appendix_coverage_test.cpp` |
| A.2 types (`primitive_type`, `generic_type`, `type_parameter_list`) | Covered | `tests/cpp/frontend_parser_generics_test.cpp`, `tests/cpp/frontend_parser_legacy_rejection_test.cpp`, `tests/cpp/frontend_parser_appendix_coverage_test.cpp` |
| A.3 expressions (assignment/logical/bitwise/shift/term/factor/exponent/unary/call/primary) | Covered | `tests/cpp/test_parser_regression_audit.cpp`, `tests/cpp/frontend_expression_features_test.cpp`, `tests/cpp/frontend_parser_appendix_coverage_test.cpp` |
| A.3 vector literals (list and repeat forms) | Covered | `tests/cpp/frontend_parser_appendix_coverage_test.cpp`, `tests/cpp/semantic_analyzer_vector_literal_test.cpp` |
| A.3 `match_expression` and arm guards | Covered | `tests/cpp/frontend_parser_appendix_coverage_test.cpp`, `tests/cpp/frontend_parser_recovery_test.cpp`, `tests/cpp/semantic_analyzer_match_test.cpp` |
| A.3 block expressions | Covered | `tests/cpp/frontend_parser_appendix_coverage_test.cpp` |
| A.3 if expressions | Covered | `tests/cpp/e2e_if_statement_test.cpp`, `tests/cpp/frontend_parser_appendix_coverage_test.cpp` (explicit `if_expression` parser coverage) |
| A.4 statements (`let`, `var`, expression, return, if, loop, block`) | Covered | `tests/cpp/frontend_parser_test.cpp`, `tests/cpp/frontend_parser_appendix_coverage_test.cpp`, `tests/cpp/frontend_parser_recovery_test.cpp` |
| A.5 top-level declarations (`fn`, `type`, `record`, `enum`) | Covered | `tests/cpp/frontend_parser_appendix_coverage_test.cpp`, `tests/cpp/t81lang_conformance_baseline_test.cpp` |

## M2 Deterministic Compilation Profile Gap Plan

Planned section target in `spec/t81lang-spec.md`:

- Section 5 ("Compilation Pipeline"), with profile-level constraints for
  bytecode emission determinism (no semantic expansion).

Bounded gap closure scope:

1. Document deterministic emission invariants already implied by existing
   lowering and reproducibility gate behavior.
2. Map each invariant to existing evidence paths (IR tests, e2e compile
   determinism, fixture-hash gate).
3. Keep determinism guarantees bounded to registry status and DCP scope.

## M3 Conformance and Repro Evidence Mapping

| Drift Category | Primary Check(s) | Scope Signal | Open Gap |
| :--- | :--- | :--- | :--- |
| Grammar/parsing | `frontend_parser_*`, `test_parser_regression_audit.cpp` | Operator precedence and legacy syntax rejection tested | Closed for current cycle via Appendix A coverage matrix evidence (`A1-CODE-01`, `A1B-CODE-01`) |
| Semantic typing | `semantic_analyzer_*`, `t81lang_conformance_baseline_test.cpp` | Numeric widening, Option/Result, diagnostics covered | Closed for current cycle via section-level semantic index (`A1B-CODE-02`) |
| IR/lowering determinism | `frontend_ir_generator_*`, `e2e_*`, `test_frontend_logical_lowering.cpp` | Deterministic lowering behaviors tested | Compile-profile trace statement now anchored in `spec/t81lang-spec.md` section 5; residual state follows registry status |
| Compile reproducibility | `scripts/ci/t81lang_repro_gate.py`, `tests/fixtures/t81lang_determinism/` | Two-pass byte identity + aggregate hash gate active | Fixture pack expanded to 16 canonical programs; repro and AST/IR hash references refreshed |

## Stage 3 Semantic Coverage Tightening (A1-CODE-02)

| Rule Area (`spec/t81lang-spec.md` section 5, Stage 3) | Coverage Status | Evidence |
| :--- | :--- | :--- |
| Modulo restricted to integer operands | Covered | `tests/cpp/semantic_analyzer_stage3_rules_test.cpp`, `tests/cpp/semantic_analyzer_option_result_test.cpp` |
| Comparison constraints for non-numeric types (`Symbol` relational rejection; equality handling) | Covered | `tests/cpp/semantic_analyzer_stage3_rules_test.cpp`, `tests/cpp/spec_compliance_test.cpp` |
| Assignment/call/return widening rules (`int` to `T81Float`/`T81Fraction`) | Covered | `tests/cpp/semantic_analyzer_stage3_rules_test.cpp`, `tests/cpp/t81lang_conformance_baseline_test.cpp` |
| Implicit narrowing rejection (call/return mismatch) | Covered | `tests/cpp/semantic_analyzer_stage3_rules_test.cpp`, `tests/cpp/t81lang_conformance_baseline_test.cpp` |

## Section-Level Semantic Coverage Index (A1B-CODE-02)

| Spec Anchor | Rule/Surface | Coverage Status | Evidence |
| :--- | :--- | :--- | :--- |
| `spec/t81lang-spec.md` section 2.1 | Generic type syntax acceptance/rejection in semantic flow | Covered | `tests/cpp/semantic_analyzer_generic_test.cpp`, `tests/cpp/frontend_parser_generics_test.cpp`, `tests/cpp/frontend_parser_legacy_rejection_test.cpp` |
| `spec/t81lang-spec.md` section 2.5 (Structural Types) | `Option[T]` / `Result[T, E]` typing and payload constraints | Covered | `tests/cpp/semantic_analyzer_option_result_test.cpp`, `tests/cpp/e2e_option_result_test.cpp`, `tests/cpp/e2e_option_result_function_test.cpp` |
| `spec/t81lang-spec.md` section 2.5 (Numeric Widening) | Widening acceptance and narrowing rejection across call/return/assignment surfaces | Covered | `tests/cpp/semantic_analyzer_numeric_test.cpp`, `tests/cpp/semantic_analyzer_stage3_rules_test.cpp`, `tests/cpp/t81lang_conformance_baseline_test.cpp` |
| `spec/t81lang-spec.md` section 5 Stage 3 | Modulo/comparison/type-compatibility semantic rules | Covered | `tests/cpp/semantic_analyzer_stage3_rules_test.cpp`, `tests/cpp/spec_compliance_test.cpp` |
| `spec/t81lang-spec.md` section 5 Stage 3 diagnostics | Diagnostic location and precision behavior for semantic failures | Covered | `tests/cpp/semantic_analyzer_diagnostic_location_test.cpp`, `tests/cpp/semantic_analyzer_diagnostic_precision_test.cpp`, `tests/cpp/semantic_analyzer_cascade_suppression_test.cpp` |

## Control-Flow Edge Conformance Coverage (A1-CODE-05)

| Rule Area (`spec/t81lang-spec.md` sections 3 and 6) | Coverage Status | Evidence |
| :--- | :--- | :--- |
| Match guard must be boolean | Covered | `tests/cpp/t81lang_conformance_edge_semantics_test.cpp`, `tests/cpp/semantic_analyzer_match_test.cpp` |
| Result/Option match payload and guard edge handling | Covered | `tests/cpp/t81lang_conformance_edge_semantics_test.cpp`, `tests/cpp/e2e_option_result_test.cpp` |
| Loop annotation required (`@bounded`) | Covered | `tests/cpp/t81lang_conformance_edge_semantics_test.cpp`, `tests/cpp/semantic_analyzer_loop_test.cpp` |
| Guarded loop condition must be boolean | Covered | `tests/cpp/t81lang_conformance_edge_semantics_test.cpp`, `tests/cpp/semantic_analyzer_loop_test.cpp` |
| Tier annotation + bounded loop + match interaction | Covered | `tests/cpp/t81lang_conformance_edge_semantics_test.cpp` |

## Section-Level Purity/Control-Flow Coverage Index (A1C-CODE-01)

| Spec Anchor | Rule/Surface | Coverage Status | Evidence |
| :--- | :--- | :--- | :--- |
| `spec/t81lang-spec.md` section 3.1 (Pure Functions) | Pure/default function behavior and deterministic evaluation expectations in frontend conformance paths | Covered | `tests/cpp/t81lang_conformance_baseline_test.cpp`, `tests/cpp/e2e_match_expression_test.cpp` |
| `spec/t81lang-spec.md` section 3.2 (Effectful Functions) | Effect/annotation-aware semantic checks and boundary behavior in control-flow paths | Covered | `tests/cpp/t81lang_conformance_edge_semantics_test.cpp`, `tests/cpp/semantic_analyzer_loop_test.cpp` |
| `spec/t81lang-spec.md` section 3.3 (Tiered Purity) | Tier annotation handling and bounded-loop interaction behavior | Covered | `tests/cpp/t81lang_conformance_edge_semantics_test.cpp`, `tests/cpp/semantic_analyzer_loop_test.cpp` |
| `spec/t81lang-spec.md` section 6.2 (Match) | Match-arm/guard semantic constraints and edge behavior | Covered | `tests/cpp/semantic_analyzer_match_test.cpp`, `tests/cpp/t81lang_conformance_edge_semantics_test.cpp`, `tests/cpp/e2e_match_expression_test.cpp` |
| `spec/t81lang-spec.md` section 6.3 (Loop) | Loop boundedness requirements and guard-condition semantic constraints | Covered | `tests/cpp/semantic_analyzer_loop_test.cpp`, `tests/cpp/t81lang_conformance_edge_semantics_test.cpp` |

## Section 7 Axion Integration Evidence Index (A1C-CODE-02)

| Spec Anchor | Rule/Surface | Coverage Status | Evidence |
| :--- | :--- | :--- | :--- |
| `spec/t81lang-spec.md` section 7.1 (Tier Metadata) | Tier metadata emission and trace integration from frontend to runtime surfaces | Covered | `tests/cpp/axion_loop_metadata_test.cpp`, `tests/cpp/axion_match_metadata_test.cpp`, `tests/cpp/e2e_axion_trace_test.cpp` |
| `spec/t81lang-spec.md` section 7.2 (Safety Hooks) | Axion policy hook behavior over match/control-flow operations and guard metadata | Covered | `tests/cpp/axion_policy_match_guard_test.cpp`, `tests/cpp/e2e_axion_trace_test.cpp` |

## Section 7 Guard/Segment Metadata Traceability Addendum (A1F-CODE-01)

| Spec Anchor | Rule/Surface | Coverage Status | Evidence |
| :--- | :--- | :--- | :--- |
| `spec/t81lang-spec.md` section 7.1 (Tier Metadata) | Language-origin guard metadata traceability through Axion policy verdict paths | Covered | `tests/cpp/axion_policy_match_guard_test.cpp`, `tests/cpp/e2e_axion_trace_test.cpp` |
| `spec/t81lang-spec.md` section 7.2 (Safety Hooks) | Segment-event metadata traceability for policy-required guard/segment reasons | Covered | `tests/cpp/axion_policy_segment_event_test.cpp`, `tests/cpp/axion_segment_trace_test.cpp`, `tests/cpp/e2e_axion_trace_test.cpp` |

## Section 5 IR/Lowering Determinism Evidence Index (A1D-CODE-01)

| Spec Anchor | Rule/Surface | Coverage Status | Evidence |
| :--- | :--- | :--- | :--- |
| `spec/t81lang-spec.md` section 5 (Deterministic Compilation Profile traceability) | End-to-end compile byte-identity and deterministic artifact behavior across repeated compilation passes | Covered | `tests/cpp/e2e_compile_determinism_test.cpp`, `tests/cpp/e2e_ast_ir_canonical_determinism_test.cpp`, `scripts/ci/t81lang_repro_gate.py` |
| `spec/t81lang-spec.md` section 5 stage 6/7 (IR + TISC lowering) | Frontend IR generation and lowering path determinism evidence for stable compilation profile surfaces | Covered | `tests/cpp/frontend_ir_generator_test.cpp`, `tests/cpp/e2e_compile_determinism_test.cpp`, `tests/cpp/e2e_ast_ir_canonical_determinism_test.cpp` |

## Section 5 Compilation-Profile Maintenance Evidence Addendum (A1F-CODE-02)

| Spec Anchor | Rule/Surface | Coverage Status | Evidence |
| :--- | :--- | :--- | :--- |
| `spec/t81lang-spec.md` section 5 (Deterministic Compilation Profile maintenance) | Reproducibility-gate continuity for compile artifacts across repeated runs and fixture sets | Covered | `scripts/ci/t81lang_repro_gate.py`, `tests/cpp/e2e_compile_determinism_test.cpp`, `tests/cpp/e2e_ast_ir_canonical_determinism_test.cpp` |
| `spec/t81lang-spec.md` section 5 (Conformance-scope maintenance linkage) | Compilation-profile evidence remains bounded to current conformance baseline without guarantee expansion | Covered | `tests/cpp/t81lang_conformance_baseline_test.cpp`, `tests/cpp/e2e_compile_determinism_test.cpp` |

## Section 5 Reproducibility-Hash Maintenance Addendum (A1G-CODE-02)

| Spec Anchor | Rule/Surface | Coverage Status | Evidence |
| :--- | :--- | :--- | :--- |
| `spec/t81lang-spec.md` section 5 (Reproducibility hash continuity) | Fixture-hash continuity maintenance for reproducibility gate outputs and canonical hash artifacts | Covered | `scripts/ci/t81lang_repro_gate.py`, `tests/fixtures/t81lang_determinism/t81lang_repro_hash.txt`, `tests/fixtures/t81lang_determinism/t81lang_ast_ir_repro_hash.txt` |
| `spec/t81lang-spec.md` section 5 (Compile determinism continuity) | Reproducibility-hash maintenance remains aligned with compile determinism checks and current fixture scope | Covered | `tests/cpp/e2e_compile_determinism_test.cpp`, `scripts/ci/t81lang_repro_gate.py` |

## Section 8 Stdlib Alignment Evidence Index (A1D-CODE-02)

| Spec Anchor | Rule/Surface | Coverage Status | Evidence |
| :--- | :--- | :--- | :--- |
| `spec/t81lang-spec.md` section 8 (Interoperability Summary) | Standard library fixture behavior alignment across text, bytes, symbol, and collection surfaces | Covered | `tests/cpp/cli_std_text_fixtures_test.cpp`, `tests/cpp/cli_std_bytes_fixtures_test.cpp`, `tests/cpp/cli_std_symbol_fixtures_test.cpp`, `tests/cpp/cli_std_collections_fixtures_test.cpp` |
| `spec/t81lang-spec.md` section 8 (Interoperability Summary) | Standard library fixture behavior alignment across tensor/runtime surfaces | Covered | `tests/cpp/cli_std_tensor_fixtures_test.cpp`, `tests/cpp/cli_std_runtime_fixtures_test.cpp` |

## Section 4 Name-Resolution/Scoping Evidence Index (A1E-CODE-01)

| Spec Anchor | Rule/Surface | Coverage Status | Evidence |
| :--- | :--- | :--- | :--- |
| `spec/t81lang-spec.md` section 4 (Name Resolution) | Diagnostic precision for unresolved/ambiguous symbol scenarios and scope-bound failures | Covered | `tests/cpp/semantic_analyzer_diagnostic_precision_test.cpp`, `tests/cpp/t81lang_conformance_baseline_test.cpp` |
| `spec/t81lang-spec.md` section 4 (Name Resolution) | Stable diagnostic location reporting for resolution and scoping errors | Covered | `tests/cpp/semantic_analyzer_diagnostic_location_test.cpp`, `tests/cpp/semantic_analyzer_cascade_suppression_test.cpp` |

## Residual Section 3/6 Alignment Evidence Addendum (A1E-CODE-02)

| Spec Anchor | Rule/Surface | Coverage Status | Evidence |
| :--- | :--- | :--- | :--- |
| `spec/t81lang-spec.md` section 3.2 (Effectful Functions) | Effect metadata traceability through semantic control-flow checks in loop/match-heavy paths | Covered | `tests/cpp/t81lang_conformance_edge_semantics_test.cpp`, `tests/cpp/semantic_analyzer_loop_test.cpp`, `tests/cpp/semantic_analyzer_match_test.cpp` |
| `spec/t81lang-spec.md` section 6.2 (Match) and section 6.3 (Loop) | Residual control-flow alignment for guard and boundedness metadata behavior in frontend and e2e surfaces | Covered | `tests/cpp/semantic_analyzer_match_test.cpp`, `tests/cpp/semantic_analyzer_loop_test.cpp`, `tests/cpp/e2e_match_expression_test.cpp` |

## Section 2/6 Structural-Type Control-Flow Traceability Addendum (A1G-CODE-01)

| Spec Anchor | Rule/Surface | Coverage Status | Evidence |
| :--- | :--- | :--- | :--- |
| `spec/t81lang-spec.md` section 2.5 (Structural Types) | Option/Result structural typing traceability in semantic analysis and function-level e2e control-flow paths | Covered | `tests/cpp/semantic_analyzer_option_result_test.cpp`, `tests/cpp/e2e_option_result_test.cpp`, `tests/cpp/e2e_option_result_function_test.cpp` |
| `spec/t81lang-spec.md` section 6.2 (Match) | Structural-type payload and control-flow traceability for match behavior over Option/Result-driven paths | Covered | `tests/cpp/e2e_option_result_test.cpp`, `tests/cpp/e2e_option_result_function_test.cpp`, `tests/cpp/e2e_match_expression_test.cpp` |

## Executable Code Task Queue

- `docs/status/T81LANG_IMPLEMENTATION_TASK_QUEUE_2026-03.md`

## Post-A1 Evidence Deltas (Next Cycle Inputs)

These deltas are bounded follow-on items derived from current decomposition
tables and open-gap notes. They do not change semantics, freeze boundaries, or
determinism claim scope.

1. Add explicit parser coverage evidence for expression-form `if` grammar paths. Completed (A1B-CODE-01, 2026-02-25).
2. Publish section-level semantic coverage index tied to section 2/5 rule
   anchors and existing test assets. Completed (A1B-CODE-02, 2026-02-25).
3. Refresh matrix/audit notes after follow-on evidence updates are applied.
   Completed (A1B-CODE-03, 2026-02-25).

## Next-Cycle Candidate Deltas (A1C Seed)

These planned deltas extend drift-reduction evidence coverage without changing
runtime semantics, determinism scope, or freeze boundaries.

1. Publish section-level coverage index for section 3/6 purity and control-flow
   semantic anchors using existing conformance/semantic suites.
   Completed (A1C-CODE-01, 2026-02-25).
2. Publish section 7 Axion-integration metadata evidence index tied to existing
   metadata and e2e trace tests. Completed (A1C-CODE-02, 2026-02-25).
3. Sync matrix/governance artifacts after A1C evidence updates.
   Completed (A1C-CODE-03, 2026-02-25).

## Post-A1C Candidate Deltas (A1D Seed)

These planned deltas continue evidence-driven drift reduction for remaining
traceability surfaces without changing runtime semantics, determinism scope, or
freeze boundaries.

1. Publish section 5 IR/lowering determinism evidence index tied to existing
   frontend IR and e2e determinism/repro checks.
   Completed (A1D-CODE-01, 2026-02-25).
2. Publish section 8 stdlib alignment evidence index tied to existing CLI std
   fixture suites. Completed (A1D-CODE-02, 2026-02-25).
3. Sync matrix/governance artifacts after A1D evidence updates.
   Completed (A1D-CODE-03, 2026-02-25).

## Post-A1D Candidate Deltas (A1E Seed)

These planned deltas continue bounded evidence-index closure for residual
partial-alignment areas without changing runtime semantics, determinism scope,
or freeze boundaries.

1. Publish section 4 name-resolution/scoping evidence index tied to existing
   diagnostics and conformance checks. Completed (A1E-CODE-01, 2026-02-25).
2. Publish residual section 3/6 alignment evidence addendum focused on
   purity/effects metadata traceability. Completed (A1E-CODE-02, 2026-02-25).
3. Sync matrix/governance artifacts after A1E evidence updates.
   Completed (A1E-CODE-03, 2026-02-25).

## Post-A1E Candidate Deltas (A1F Seed)

These planned deltas continue bounded, evidence-only drift reduction for
remaining language/runtime traceability seams without changing runtime
semantics, determinism scope, or freeze boundaries.

1. Publish section 7 guard/segment metadata traceability addendum aligned to
   existing Axion policy/trace tests for language-origin metadata surfaces.
   Completed (A1F-CODE-01, 2026-02-25).
2. Publish section 5 compilation-profile maintenance evidence addendum tying
   reproducibility gate artifacts to current conformance scope language.
   Completed (A1F-CODE-02, 2026-02-25).
3. Sync matrix/governance artifacts after A1F evidence updates.
   Completed (A1F-CODE-03, 2026-02-25).

## Post-A1F Candidate Deltas (A1G Seed)

These planned deltas continue bounded, evidence-only maintenance for residual
traceability consistency without changing runtime semantics, determinism scope,
or freeze boundaries.

1. Publish section 2/6 structural-type control-flow traceability addendum using
   existing Option/Result and match conformance suites.
   Completed (A1G-CODE-01, 2026-02-25).
2. Publish section 5 reproducibility-hash maintenance addendum tied to current
   fixture-hash artifact continuity.
   Completed (A1G-CODE-02, 2026-02-25).
3. Sync matrix/governance artifacts after A1G evidence updates.
   Completed (A1G-CODE-03, 2026-02-25).

## Risks and Controls

- Risk: Overclaiming deterministic guarantees for T81Lang surfaces.
  - Control: Keep registry status language authoritative; avoid guarantee
    expansion in decomposition updates.
- Risk: Planning drift without evidence updates.
  - Control: Each milestone requires artifact-level evidence.

## Cycle Closure Gate

The next drift-reduction cycle may be seeded only after the current cycle is
closed under all of the following conditions:

1. All queue tasks for the cycle are marked `Completed` in
   `docs/status/T81LANG_IMPLEMENTATION_TASK_QUEUE_2026-03.md`.
2. `docs/status/IMPLEMENTATION_MATRIX.md` reflects cycle closure status.
3. `docs/records/audits/2026-03-governance-review.md` records closure evidence.
4. `docs/status/EXECUTION_PLAN_2026-03.md` immediate-next-action text no longer
   targets the closed cycle.

Any override requires governance-audit documentation in the current month
artifact.

## Cross-References

- `docs/status/EXECUTION_PLAN_2026-03.md`
- `docs/status/IMPLEMENTATION_MATRIX.md`
- `docs/status/VERIFIED_SURFACE_AUDIT.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/governance/SPEC_AUTHORITY_MODEL.md`

## Versioning Statement

This decomposition is a planning-control artifact. It does not alter normative
specification authority or deterministic guarantee boundaries.
