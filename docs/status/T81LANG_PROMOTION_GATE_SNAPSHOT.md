# T81Lang Promotion Gate Snapshot

Generated (UTC): 2026-03-10 10:19:50Z
Generator: `scripts/governance/t81lang_promotion_gate_snapshot.py`

## Gate Criteria Status

| Criterion | Status | Basis |
| :--- | :--- | :--- |
| TG-01 | Fail | Governance hygiene check command status |
| TG-02 | Pass | Repro gate command status |
| TG-03 | Pass | Conformance/semantic/determinism/Axion ctest slices |
| TG-04 | Fail | BG-01..BG-05 completion status in backlog |
| TG-05 | Fail | Gate/matrix/audit artifacts present |
| TG-06 | Pass | Registry + DCP references present |

## Promotion Readiness

- Snapshot Date: 2026-03-10
- Result: Not Ready

## Backlog Statuses

- BG-01: Missing
- BG-02: Missing
- BG-03: Missing
- BG-04: Missing
- BG-05: Missing

## Command Runs

### Docs Governance Hygiene

- Status: Fail
- Command: `/opt/homebrew/opt/python@3.14/bin/python3.14 scripts/governance/check_docs_governance_hygiene.py`

```text
governance hygiene check FAILED:
- missing required README: docs/benchmarks/README.md
- missing required README: docs/how-to/README.md
- missing required README: docs/migration/README.md
- missing required README: docs/policies/README.md
- missing required README: docs/releases/README.md
- missing required README: docs/rfcs/README.md
- missing required README: docs/tutorials/README.md
- missing queue file: docs/status/T81LANG_IMPLEMENTATION_TASK_QUEUE_2026-03.md
- status label coherence check failed
- coherence: - missing required file: docs/status/SYSTEM_STATUS.md
- coherence: - missing required file: docs/status/T81LANG_PROMOTION_GATE.md
- root structure check failed
- root structure: root structure check FAILED
- root structure: - missing:
- root structure:   - legacy
- root structure: - unexpected:
- root structure:   - _config.yml
- root structure:   - bigint_demo.tisc
- root structure:   - hello_world.tisc
- root structure:   - t81_exploration_report.md
- translation semantic alignment check failed
- translation semantic alignment: translation semantic alignment check FAILED
- translation semantic alignment: - README.es.md: missing required semantic token: superficies verificadas
- translation semantic alignment: - README.es.md: missing required semantic token: registro de determinismo
- translation semantic alignment: - README.es.md: missing required semantic token: Trace-JIT
- translation semantic alignment: - README.es.md: missing required semantic token: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
- translation semantic alignment: - README.es.md: missing required semantic token: python3 scripts/ci/t81lang_repro_gate.py --t81-bin build/t81 --fixtures-dir tests/fixtures/t81lang_determinism --workdir build/t81lang-repro --hash-out build/t81lang-repro/hash.txt --expected-hash-file tests/fixtures/t81lang_determinism/t81lang_repro_hash.txt
- translation semantic alignment: - README.es.md: missing required section heading: ## Características
- translation semantic alignment: - README.es.md: missing required section heading: ## Inicio Rápido
- translation semantic alignment: - README.es.md: missing required section heading: ## Plataformas Soportadas
- translation semantic alignment: - README.es.md: missing required section heading: ## Ejemplos CLI
- translation semantic alignment: - README.es.md: missing required section heading: ## Mapa de Autoridad Documental
- translation semantic alignment: - README.pt-BR.md: missing required semantic token: superfícies verificadas
- translation semantic alignment: - README.pt-BR.md: missing required semantic token: registro de determinismo
- translation semantic alignment: - README.pt-BR.md: missing required semantic token: Trace-JIT
- translation semantic alignment: - README.pt-BR.md: missing required semantic token: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
- translation semantic alignment: - README.pt-BR.md: missing required semantic token: python3 scripts/ci/t81lang_repro_gate.py --t81-bin build/t81 --fixtures-dir tests/fixtures/t81lang_determinism --workdir build/t81lang-repro --hash-out build/t81lang-repro/hash.txt --expected-hash-file tests/fixtures/t81lang_determinism/t81lang_repro_hash.txt
- translation semantic alignment: - README.pt-BR.md: missing required section heading: ## Recursos
- translation semantic alignment: - README.pt-BR.md: missing required section heading: ## Início Rápido
- translation semantic alignment: - README.pt-BR.md: missing required section heading: ## Plataformas Suportadas
- translation semantic alignment: - README.pt-BR.md: missing required section heading: ## Exemplos CLI
- translation semantic alignment: - README.pt-BR.md: missing required section heading: ## Mapa de Autoridade Documental
- translation semantic alignment: - README.ru.md: missing required semantic token: верифицированных поверхностях
- translation semantic alignment: - README.ru.md: missing required semantic token: реестром детерминизма
- translation semantic alignment: - README.ru.md: missing required semantic token: Trace-JIT
- translation semantic alignment: - README.ru.md: missing required semantic token: Beta
- translation semantic alignment: - README.ru.md: missing required semantic token: Alpha
- translation semantic alignment: - README.ru.md: missing required semantic token: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
- translation semantic alignment: - README.ru.md: missing required semantic token: python3 scripts/ci/t81lang_repro_gate.py --t81-bin build/t81 --fixtures-dir tests/fixtures/t81lang_determinism --workdir build/t81lang-repro --hash-out build/t81lang-repro/hash.txt --expected-hash-file tests/fixtures/t81lang_determinism/t81lang_repro_hash.txt
- translation semantic alignment: - README.ru.md: missing required section heading: ## Возможности
- translation semantic alignment: - README.ru.md: missing required section heading: ## Быстрый Старт
- translation semantic alignment: - README.ru.md: missing required section heading: ## Поддерживаемые Платформы
- translation semantic alignment: - README.ru.md: missing required section heading: ## Примеры CLI
- translation semantic alignment: - README.ru.md: missing required section heading: ## Карта Авторитетной Документации
- translation semantic alignment: - README.zh-CN.md: missing required semantic token: 已验证表面
- translation semantic alignment: - README.zh-CN.md: missing required semantic token: 确定性注册表
- translation semantic alignment: - README.zh-CN.md: missing required semantic token: Trace-JIT
- translation semantic alignment: - README.zh-CN.md: missing required semantic token: cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
- translation semantic alignment: - README.zh-CN.md: missing required semantic token: python3 scripts/ci/t81lang_repro_gate.py --t81-bin build/t81 --fixtures-dir tests/fixtures/t81lang_determinism --workdir build/t81lang-repro --hash-out build/t81lang-repro/hash.txt --expected-hash-file tests/fixtures/t81lang_determinism/t81lang_repro_hash.txt
- translation semantic alignment: - README.zh-CN.md: missing required section heading: ## 特性
- translation semantic alignment: - README.zh-CN.md: missing required section heading: ## 快速开始
- translation semantic alignment: - README.zh-CN.md: missing required section heading: ## 支持平台
- translation semantic alignment: - README.zh-CN.md: missing required section heading: ## CLI 示例
- translation semantic alignment: - README.zh-CN.md: missing required section heading: ## 文档权威地图
- public api semver lock check failed
- public api semver lock: public API semver lock check FAILED
- public api semver lock: - version mismatch: config=1.3.2, lock=1.1.0 (update lock with --write-lock after approved semver bump)
- public api semver lock: - public API surface digest mismatch for include/t81/**/*.hpp (update lock with --write-lock after approved API change)
- cognitive-tier boundary check failed
- cognitive-tier boundary: cognitive-tier boundary check FAILED
- cognitive-tier boundary: - docs/status/IMPLEMENTATION_MATRIX.md: missing required boundary marker: Experimental, non-DCP, non-verified unless promoted through governance
- cognitive-tier boundary: - missing required status boundary file: docs/status/SYSTEM_STATUS.md
```

### Conformance + Semantics Slice

- Status: Pass
- Command: `ctest --test-dir /Users/t81dev/Code/t81-foundation/build -R t81lang_conformance_baseline_test|t81lang_conformance_edge_semantics_test|t81_semantic_analyzer_match_test|t81_semantic_analyzer_loop_test|t81_semantic_analyzer_diagnostic_precision_test|t81_semantic_analyzer_diagnostic_location_test|t81_semantic_analyzer_cascade_suppression_test --output-on-failure`

```text
Test project /Users/t81dev/Code/t81-foundation/build
    Start   8: t81lang_conformance_edge_semantics_test
1/7 Test   #8: t81lang_conformance_edge_semantics_test ...........   Passed    0.00 sec
    Start  93: t81lang_conformance_baseline_test
2/7 Test  #93: t81lang_conformance_baseline_test .................   Passed    0.01 sec
    Start 186: t81_semantic_analyzer_match_test
3/7 Test #186: t81_semantic_analyzer_match_test ..................   Passed    0.00 sec
    Start 191: t81_semantic_analyzer_loop_test
4/7 Test #191: t81_semantic_analyzer_loop_test ...................   Passed    0.00 sec
    Start 193: t81_semantic_analyzer_diagnostic_location_test
5/7 Test #193: t81_semantic_analyzer_diagnostic_location_test ....   Passed    0.00 sec
    Start 194: t81_semantic_analyzer_cascade_suppression_test
6/7 Test #194: t81_semantic_analyzer_cascade_suppression_test ....   Passed    0.00 sec
    Start 195: t81_semantic_analyzer_diagnostic_precision_test
7/7 Test #195: t81_semantic_analyzer_diagnostic_precision_test ...   Passed    0.00 sec

100% tests passed, 0 tests failed out of 7

Total Test time (real) =   0.04 sec
```

### Compile Determinism Slice

- Status: Pass
- Command: `ctest --test-dir /Users/t81dev/Code/t81-foundation/build -R e2e_compile_determinism_test|e2e_ast_ir_canonical_determinism_test|e2e_enum_metadata_determinism_test --output-on-failure`

```text
Test project /Users/t81dev/Code/t81-foundation/build
    Start 207: e2e_compile_determinism_test
1/3 Test #207: e2e_compile_determinism_test ............   Passed    0.01 sec
    Start 209: e2e_ast_ir_canonical_determinism_test
2/3 Test #209: e2e_ast_ir_canonical_determinism_test ...   Passed    0.01 sec
    Start 210: e2e_enum_metadata_determinism_test
3/3 Test #210: e2e_enum_metadata_determinism_test ......   Passed    0.01 sec

100% tests passed, 0 tests failed out of 3

Total Test time (real) =   0.03 sec
```

### Axion Metadata Slice

- Status: Pass
- Command: `ctest --test-dir /Users/t81dev/Code/t81-foundation/build -R axion_policy_match_guard_test|axion_policy_segment_event_test|axion_match_metadata_test|axion_enum_guard_test|e2e_axion_trace_test --output-on-failure`

```text
Test project /Users/t81dev/Code/t81-foundation/build
    Start 222: axion_policy_match_guard_test
1/5 Test #222: axion_policy_match_guard_test .....   Passed    0.01 sec
    Start 223: axion_policy_segment_event_test
2/5 Test #223: axion_policy_segment_event_test ...   Passed    0.00 sec
    Start 228: axion_match_metadata_test
3/5 Test #228: axion_match_metadata_test .........   Passed    0.01 sec
    Start 229: axion_enum_guard_test
4/5 Test #229: axion_enum_guard_test .............   Passed    0.01 sec
    Start 242: e2e_axion_trace_test
5/5 Test #242: e2e_axion_trace_test ..............   Passed    0.01 sec

100% tests passed, 0 tests failed out of 5

Total Test time (real) =   0.04 sec
```

### Repro Gate

- Status: Pass
- Command: `/opt/homebrew/opt/python@3.14/bin/python3.14 scripts/ci/t81lang_repro_gate.py --t81-bin /Users/t81dev/Code/t81-foundation/build/t81 --fixtures-dir /Users/t81dev/Code/t81-foundation/tests/fixtures/t81lang_determinism --workdir build/t81lang-repro-promotion-gate --hash-out build/t81lang-repro-promotion-gate/hash.txt`

```text
T81Lang gates passed: fixtures=21 hash=c8a7a5e4879fefa1c469c60846ded76d09ceb730db7a3624a9966a3c0b0b8391
```
