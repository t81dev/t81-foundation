# T81Lang Traceability Matrix

## Evidence review — 2026-10-27

Purpose: map compiler requirements to meaningful verification. The old single-row REQ-T81-TRACE-01 PASS is **Unsupported as enforcement evidence**: [test source](../../tests/cpp/test_t81lang_traceability_enforcement.cpp) uses void helpers that print errors and return; main can still return success. It also checks one local process, not independent cross-platform execution. No test code is altered here.

| Requirement                      | Source / existing evidence                                                                                                           | Disposition                                                   |
| -------------------------------- | ------------------------------------------------------------------------------------------------------------------------------------ | ------------------------------------------------------------- |
| Grammar/precedence               | [parser](../../lang/frontend/parser.cpp), [Appendix coverage](../../tests/cpp/frontend_parser_appendix_coverage_test.cpp)            | Implemented; not rerun in this audit.                         |
| Type/control-flow semantics      | [analyzer](../../lang/frontend/semantic_analyzer.cpp), [conformance baseline](../../tests/cpp/t81lang_conformance_baseline_test.cpp) | Implemented; bounded tests, not exhaustive proof.             |
| Source-to-bytecode repeatability | [IR generator](../../lang/frontend/ir_generator.cpp), [repro gate](../../scripts/ci/t81lang_repro_gate.py), canonical fixtures       | Registry Partial; no new cross-platform result here.          |
| Policy metadata                  | [AgentInvoke policy test](../../tests/cpp/axion_agent_invoke_policy_test.cpp)                                                        | Scoped VM test, not universal language-to-policy propagation. |

Next step: repair failure signaling separately and demonstrate a deliberately failing negative control before treating a green traceability process as evidence. Historical queue completion and stable spec decisions remain preserved.

Review basis: audited main `c930b8c2ee7478782bf3c6d231bdd59984595ef2`. This editorial review does not grant promotion or release approval. See the [reconciliation report](STATUS_RECONCILIATION_2026-10-27.md) for executed checks and limitations.

The default [compiler policy](../../cmake/Compiler.cmake) adds `NDEBUG` globally, so assertion-only C++ checks can be disabled even outside the usual release flags. This audit verified that generated decoder-test flags required an explicit temporary `-UNDEBUG` override; its assertions-enabled execution remained incomplete. Record assertion execution separately from CTest process success.

## Retained record

## Overview
This document serves as the traceability matrix for T81Lang features and their corresponding verification requirements.

## Matrix

| Feature | Requirement | Verification Status | Notes |
|---|---|---|---|
| Traceability Enforcement | REQ-T81-TRACE-01 | PASS | See `test_t81lang_traceability_enforcement.cpp` |
