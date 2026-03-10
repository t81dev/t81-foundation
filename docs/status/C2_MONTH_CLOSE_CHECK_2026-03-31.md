# C2 Month-Close Check Report (2026-03-31 Runbook)

Generated (UTC): 2026-03-10 10:19:50Z
Generator: `scripts/governance/c2_month_close_check.py`
Overall: FAIL

## Summary

| Check | Status |
| :--- | :--- |
| Governance hygiene check | FAIL |
| Promotion gate snapshot refresh | FAIL |
| Markdown link-target sweep | PASS |

## Derived Fields

- Promotion snapshot timestamp: 2026-03-10 10:19:50Z

## Command Outputs

### Governance hygiene check

- Status: FAIL
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

### Promotion gate snapshot refresh

- Status: FAIL
- Command: `/opt/homebrew/opt/python@3.14/bin/python3.14 scripts/governance/t81lang_promotion_gate_snapshot.py`

```text
Wrote snapshot: docs/status/T81LANG_PROMOTION_GATE_SNAPSHOT.md
Overall result: NOT_READY
```

### Markdown link-target sweep

- Status: PASS
- Command: `internal:link-target-sweep`

```text
link-target sweep passed
```
