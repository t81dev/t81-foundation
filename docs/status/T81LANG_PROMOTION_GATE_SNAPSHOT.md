# T81Lang Promotion Gate Snapshot

Generated (UTC): 2026-03-14 03:49:10Z
Generator: `scripts/governance/t81lang_promotion_gate_snapshot.py`

## Gate Criteria Status

| Criterion | Status | Basis |
| :--- | :--- | :--- |
| TG-01 | Pass | Governance hygiene check command status |
| TG-02 | Pass | Repro gate command status |
| TG-03 | Pass | Conformance/semantic/determinism/Axion ctest slices |
| TG-04 | Pass | BG-01..BG-05 completion status in backlog |
| TG-05 | Pass | Gate/matrix/audit artifacts present |
| TG-06 | Pass | Registry + DCP references present |

## Promotion Readiness

- Snapshot Date: 2026-03-14
- Result: Ready for Beta-candidate review

## Backlog Statuses

- BG-01: 2026-02-25
- BG-02: 2026-02-25
- BG-03: 2026-02-25
- BG-04: 2026-02-25
- BG-05: 2026-02-25

## Command Runs

### Docs Governance Hygiene

- Status: Pass
- Command: `/home/jules/.pyenv/versions/3.12.13/bin/python3 scripts/governance/check_docs_governance_hygiene.py`

```text
governance hygiene check PASSED
- required README coverage checked: 35 paths
- task-queue status consistency checked
- stale planned markers checked for completed tasks
- status label coherence checked
- supplemental governance checks (structure/readme/translation/staleness/semantic/license/artifact/api/spec-boundary/stdlib/snapshot/overclaim/rfc-lifecycle) checked
```

### Conformance + Semantics Slice

- Status: Pass
- Command: `ctest --test-dir /app/build -R t81lang_conformance_baseline_test|t81lang_conformance_edge_semantics_test|t81_semantic_analyzer_match_test|t81_semantic_analyzer_loop_test|t81_semantic_analyzer_diagnostic_precision_test|t81_semantic_analyzer_diagnostic_location_test|t81_semantic_analyzer_cascade_suppression_test --output-on-failure`

```text
Internal ctest changing into directory: /app/build
Test project /app/build
    Start   8: t81lang_conformance_edge_semantics_test
1/7 Test   #8: t81lang_conformance_edge_semantics_test ...........   Passed    0.00 sec
    Start  93: t81lang_conformance_baseline_test
2/7 Test  #93: t81lang_conformance_baseline_test .................   Passed    0.01 sec
    Start 187: t81_semantic_analyzer_match_test
3/7 Test #187: t81_semantic_analyzer_match_test ..................   Passed    0.01 sec
    Start 192: t81_semantic_analyzer_loop_test
4/7 Test #192: t81_semantic_analyzer_loop_test ...................   Passed    0.00 sec
    Start 194: t81_semantic_analyzer_diagnostic_location_test
5/7 Test #194: t81_semantic_analyzer_diagnostic_location_test ....   Passed    0.00 sec
    Start 195: t81_semantic_analyzer_cascade_suppression_test
6/7 Test #195: t81_semantic_analyzer_cascade_suppression_test ....   Passed    0.00 sec
    Start 196: t81_semantic_analyzer_diagnostic_precision_test
7/7 Test #196: t81_semantic_analyzer_diagnostic_precision_test ...   Passed    0.00 sec

100% tests passed, 0 tests failed out of 7

Total Test time (real) =   0.05 sec
```

### Compile Determinism Slice

- Status: Pass
- Command: `ctest --test-dir /app/build -R e2e_compile_determinism_test|e2e_ast_ir_canonical_determinism_test|e2e_enum_metadata_determinism_test --output-on-failure`

```text
Internal ctest changing into directory: /app/build
Test project /app/build
    Start 208: e2e_compile_determinism_test
1/3 Test #208: e2e_compile_determinism_test ............   Passed    0.01 sec
    Start 210: e2e_ast_ir_canonical_determinism_test
2/3 Test #210: e2e_ast_ir_canonical_determinism_test ...   Passed    0.01 sec
    Start 211: e2e_enum_metadata_determinism_test
3/3 Test #211: e2e_enum_metadata_determinism_test ......   Passed    0.00 sec

100% tests passed, 0 tests failed out of 3

Total Test time (real) =   0.03 sec
```

### Axion Metadata Slice

- Status: Pass
- Command: `ctest --test-dir /app/build -R axion_policy_match_guard_test|axion_policy_segment_event_test|axion_match_metadata_test|axion_enum_guard_test|e2e_axion_trace_test --output-on-failure`

```text
Internal ctest changing into directory: /app/build
Test project /app/build
    Start 223: axion_policy_match_guard_test
1/5 Test #223: axion_policy_match_guard_test .....   Passed    0.01 sec
    Start 224: axion_policy_segment_event_test
2/5 Test #224: axion_policy_segment_event_test ...   Passed    0.00 sec
    Start 229: axion_match_metadata_test
3/5 Test #229: axion_match_metadata_test .........   Passed    0.00 sec
    Start 230: axion_enum_guard_test
4/5 Test #230: axion_enum_guard_test .............   Passed    0.00 sec
    Start 243: e2e_axion_trace_test
5/5 Test #243: e2e_axion_trace_test ..............   Passed    0.00 sec

100% tests passed, 0 tests failed out of 5

Total Test time (real) =   0.03 sec
```

### Repro Gate

- Status: Pass
- Command: `/home/jules/.pyenv/versions/3.12.13/bin/python3 scripts/ci/t81lang_repro_gate.py --t81-bin /app/build/t81 --fixtures-dir /app/tests/fixtures/t81lang_determinism --workdir build/t81lang-repro-promotion-gate --hash-out build/t81lang-repro-promotion-gate/hash.txt`

```text
T81Lang gates passed: fixtures=21 hash=fb63503cf790c4bb773b97fedbdc1456cde86e19e41aaf2d317b0525007d76dc
```
