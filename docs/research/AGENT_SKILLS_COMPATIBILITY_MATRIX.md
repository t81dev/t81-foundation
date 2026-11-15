# Agent Skills Compatibility Matrix

**Pinned baseline:** `fabc7aa90f70ead227ec257a1ae6833fa16ced40`. Inspected 12 definitions, 69 distinct
package/command names (71 syntax variants), and all 66 shell files. The three
missing files are documented commands, not inferred implementations.

Each row shows the **intended** command path and whether a matching handler exists.
An existing handler is not evidence that the wrapper reaches it or accepts the
advertised arguments. Real T81 operations and fake-CLI probes are distinguished
in the [audit](OPERATIONAL_CAPABILITY_AUDIT.md) and [JSON inventory](evidence/operational_capabilities.json).

All current wrappers are unsuitable for untrusted agent inputs. Bare import and
build succeeded on the tested macOS/Bash3.2 path only; build has incompatible
optional arguments/output promises. No generic OpenClaw integration was executed.

## ai-tasks

Definition: [skills/ai-tasks/SKILL.md](../../skills/ai-tasks/SKILL.md); main wrapper: [skills/ai-tasks/assess.sh](../../skills/ai-tasks/assess.sh).

Advertised assess/route/classify omit -fixed and required --model-file/--input contracts; chain/status/history absent. Optional output is wrongly required to pre-exist. Fixed-family implementation is not reached by these paths.

**Parser options inspected:** `--model`, `--policy`, `--output`, `--routes`, `--classes`, `--chain`, `--bundle-output`, `--input`, `--detailed`, `--provenance`, `--limit`, `--filter`, `--verbose\|-v`.

| Command / entry | Advertised syntax | Intended CLI / actual trace | Verification and limits |
| --- | --- | --- | --- |
| [assess.sh](../../skills/ai-tasks/assess.sh) | `assess <input_data> [--model <model_file>] [--policy <policy_file>] [--output <output_file>]` | `t81 ai task assess`; **no matching handler**; fake argv starts `ai task assess` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [route.sh](../../skills/ai-tasks/route.sh) | `route <input_data> [--routes <route_config>] [--policy <policy_file>] [--output <output_file>]` | `t81 ai task route`; **no matching handler**; `exec` loses mode; fake argv starts `ai task assess` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [classify.sh](../../skills/ai-tasks/classify.sh) | `classify <input_data> [--classes <class_config>] [--policy <policy_file>] [--output <output_file>]` | `t81 ai task classify`; **no matching handler**; `exec` loses mode; fake argv starts `ai task assess` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [chain.sh](../../skills/ai-tasks/chain.sh) | `chain <chain_config> [--input <input_data>] [--policy <policy_file>] [--bundle-output <bundle_file>]` | `t81 ai task chain`; **no matching handler**; `exec` loses mode; fake argv starts `ai task assess` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [status.sh](../../skills/ai-tasks/status.sh) | `status <task_id> [--detailed] [--provenance]` | `t81 ai task status`; **no matching handler**; `exec` loses mode; fake argv starts `ai task assess` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [history.sh](../../skills/ai-tasks/history.sh) | `history [--limit <count>] [--filter assess\|route\|classify] [--json]` | `t81 ai task history`; **no matching handler**; `exec` loses mode; fake argv starts `ai task assess` | Fake-CLI argv only; no successful real operation established. See package limits. |

## bundle-creator

Definition: [skills/bundle-creator/SKILL.md](../../skills/bundle-creator/SKILL.md); main wrapper: [skills/bundle-creator/create-bundle.sh](../../skills/bundle-creator/create-bundle.sh).

No bundle top-level dispatch. Existing typed artifact/bundle code is not these create/sign/merge/extract commands. Unquoted EXECUTION_RESULT is evaluated; duplicate --output branches; --check-integrity not parsed.

**Parser options inspected:** `--output`, `--policy`, `--include-provenance`, `--input`, `--bundle-output`, `--detailed`, `--verify-integrity`, `--output`, `--key`, `--certificate`, `--check-signature`, `--check-provenance`, `--conflict-resolution`, `--component`, `--verbose\|-v`.

| Command / entry | Advertised syntax | Intended CLI / actual trace | Verification and limits |
| --- | --- | --- | --- |
| [create-bundle.sh](../../skills/bundle-creator/create-bundle.sh) | `create-bundle <execution_result> [--output <bundle_file>] [--policy <policy_file>] [--include-provenance]` | `t81 bundle create`; **no matching handler**; fake argv starts `bundle create /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [bundle-from-execution.sh](../../skills/bundle-creator/bundle-from-execution.sh) | `bundle-from-execution <program_file> [--input <input_data>] [--policy <policy_file>] [--bundle-output <bundle_file>]` | `t81 bundle from-execution`; **no matching handler**; `exec` loses mode; fake argv starts `bundle create /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [bundle-info.sh](../../skills/bundle-creator/bundle-info.sh) | `bundle-info <bundle_file> [--detailed] [--verify-integrity] [--output <info_file>]` | `t81 bundle info`; **no matching handler**; `exec` loses mode; fake argv starts `bundle create /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [sign-bundle.sh](../../skills/bundle-creator/sign-bundle.sh) | `sign-bundle <bundle_file> [--key <key_file>] [--certificate <cert_file>]` | `t81 bundle sign`; **no matching handler**; `exec` loses mode; fake argv starts `bundle create /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [verify-bundle.sh](../../skills/bundle-creator/verify-bundle.sh) | `verify-bundle <bundle_file> [--check-signature] [--check-provenance] [--check-integrity]` | `t81 bundle verify`; **no matching handler**; `exec` loses mode; fake argv starts `bundle create /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [merge-bundles.sh](../../skills/bundle-creator/merge-bundles.sh) | `merge-bundles <bundle1> <bundle2> [--output <merged_bundle>] [--conflict-resolution keep\|merge\|fail]` | `t81 bundle merge`; **no matching handler**; `exec` loses mode; fake argv starts `bundle create /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [extract-bundle.sh](../../skills/bundle-creator/extract-bundle.sh) | `extract-bundle <bundle_file> [--component result\|provenance\|metadata] [--output <output_file>]` | `t81 bundle extract`; **no matching handler**; `exec` loses mode; fake argv starts `bundle create /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |

## bundle-validator

Definition: [skills/bundle-validator/SKILL.md](../../skills/bundle-validator/SKILL.md); main wrapper: [skills/bundle-validator/validate.sh](../../skills/bundle-validator/validate.sh).

No bundle top-level dispatch; validate-batch wrapper absent. Verifier aliases execute validate.sh, losing their mode. No advertised replay/signature verification established.

**Parser options inspected:** `--strict`, `--policy`, `--output`, `--canonfs-root`, `--replay-count`, `--verbose\|-v`.

| Command / entry | Advertised syntax | Intended CLI / actual trace | Verification and limits |
| --- | --- | --- | --- |
| [validate.sh](../../skills/bundle-validator/validate.sh) | `validate <bundle_file> [--strict] [--policy <policy_file>] [--output json\|summary]` | `t81 bundle validate`; **no matching handler**; fake argv starts `bundle validate /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [verify-provenance.sh](../../skills/bundle-validator/verify-provenance.sh) | `verify-provenance <bundle_file> [--canonfs-root <path>]` | `t81 bundle verify-provenance`; **no matching handler**; `exec` loses mode; fake argv starts `bundle validate /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [check-determinism.sh](../../skills/bundle-validator/check-determinism.sh) | `check-determinism <bundle_file> [--replay-count <count>]` | `t81 bundle check-determinism`; **no matching handler**; `exec` loses mode; fake argv starts `bundle validate /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| `validate-batch.sh` **absent** | `validate-batch <bundle_dir> [--recursive] [--strict]` | `t81 bundle validate-batch`; **no matching handler** | Documentation only; no executable entry. |

## canonfs-export

Definition: [skills/canonfs-export/SKILL.md](../../skills/canonfs-export/SKILL.md); main wrapper: [skills/canonfs-export/export.sh](../../skills/canonfs-export/export.sh).

Only export is a matching route; list should be ls. --verify is rejected; no batch/search/export-with-meta/verify-export routes. Paths lack a workspace allowlist. Denied native export writes provenance, although it does not materialize payload.

**Parser options inspected:** `--output`, `--canonfs-root`, `--verify`, `--output-dir`, `--exported-file`, `--format`, `--filter`, `--field`, `--max-results`, `--include-provenance`, `--verbose\|-v`.

| Command / entry | Advertised syntax | Intended CLI / actual trace | Verification and limits |
| --- | --- | --- | --- |
| [export.sh](../../skills/canonfs-export/export.sh) | `export <canon_hash> [--output <output_file>] [--canonfs-root <path>] [--verify] [--json]` | `t81 canonfs export`; matching native handler; fake argv starts `canonfs export /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [export-batch.sh](../../skills/canonfs-export/export-batch.sh) | `export-batch <hash1> <hash2> <hash3> [--output-dir <dir>] [--canonfs-root <path>]` | `t81 canonfs export-batch`; **no matching handler**; `exec` loses mode; fake argv starts `canonfs export /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [verify-export.sh](../../skills/canonfs-export/verify-export.sh) | `verify-export <canon_hash> [--exported-file <file>] [--canonfs-root <path>]` | `t81 canonfs verify-export`; **no matching handler**; `exec` loses mode; fake argv starts `canonfs export /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [list.sh](../../skills/canonfs-export/list.sh) | `list [--canonfs-root <path>] [--format json\|table] [--filter <pattern>]` | `t81 canonfs list`; **no matching handler**; `exec` loses mode; fake argv starts `canonfs export /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [search.sh](../../skills/canonfs-export/search.sh) | `search <query> [--canonfs-root <path>] [--field name\|hash\|type] [--max-results <count>]` | `t81 canonfs search`; **no matching handler**; `exec` loses mode; fake argv starts `canonfs export /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [export-with-meta.sh](../../skills/canonfs-export/export-with-meta.sh) | `export-with-meta <canon_hash> [--output <file>] [--include-provenance] [--canonfs-root <path>]` | `t81 canonfs export-with-meta`; **no matching handler**; `exec` loses mode; fake argv starts `canonfs export /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |

## canonfs-import

Definition: [skills/canonfs-import/SKILL.md](../../skills/canonfs-import/SKILL.md); main wrapper: [skills/canonfs-import/import.sh](../../skills/canonfs-import/import.sh).

Bare import works on tested Bash3.2, but eval interprets hostile path syntax before Axion. import-batch/list wrappers absent. mkdir occurs before missing policy rejection; denied native import still writes provenance.

**Parser options inspected:** `--canonfs-root`, `--policy`, `--json`, `--verbose\|-v`.

| Command / entry | Advertised syntax | Intended CLI / actual trace | Verification and limits |
| --- | --- | --- | --- |
| [import.sh](../../skills/canonfs-import/import.sh) | `import <file_path> [--canonfs-root <path>] [--policy <policy_file>] [--json]` | `t81 canonfs import`; matching native handler; fake argv starts `canonfs import /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Real bare invocation succeeds; eval unsafe; host store writes. |
| `import-batch.sh` **absent** | `import-batch <file1> <file2> <file3> [--canonfs-root <path>] [--policy <policy_file>]` | `t81 canonfs import-batch`; **no matching handler** | Documentation only; no executable entry. |
| `list.sh` **absent** | `list [--canonfs-root <path>] [--format json\|table]` | `t81 canonfs list`; **no matching handler** | Documentation only; no executable entry. |

## governance-demo

Definition: [skills/governance-demo/SKILL.md](../../skills/governance-demo/SKILL.md); main wrapper: [skills/governance-demo/demo-governance.sh](../../skills/governance-demo/demo-governance.sh).

No demo top-level dispatch. All five aliases invoke the basic governance mode; zero-argument fake-CLI probes confirm misrouting. No compliance certification or industry policy engine established.

**Parser options inspected:** `--policy`, `--model`, `--interactive`, `--scenario`, `--show-violations`, `--iterations`, `--compare-results`, `--level`, `--examples`, `--policy-template`, `--test-scenarios`, `--industry`, `--regulations`, `--verbose\|-v`.

| Command / entry | Advertised syntax | Intended CLI / actual trace | Verification and limits |
| --- | --- | --- | --- |
| [demo-governance.sh](../../skills/governance-demo/demo-governance.sh) | `demo-governance [--policy <policy_file>] [--model <model_file>] [--interactive]` | `t81 demo governance`; **no matching handler**; fake argv starts `demo governance --json` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [demo-policy.sh](../../skills/governance-demo/demo-policy.sh) | `demo-policy [--scenario basic\|advanced\|financial\|healthcare] [--show-violations]` | `t81 demo policy`; **no matching handler**; `exec` loses mode; fake argv starts `demo governance --json` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [demo-determinism.sh](../../skills/governance-demo/demo-determinism.sh) | `demo-determinism [--iterations <count>] [--compare-results]` | `t81 demo determinism`; **no matching handler**; `exec` loses mode; fake argv starts `demo governance --json` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [tutorial-policy.sh](../../skills/governance-demo/tutorial-policy.sh) | `tutorial-policy [--level beginner\|intermediate\|advanced] [--examples]` | `t81 demo tutorial-policy`; **no matching handler**; `exec` loses mode; fake argv starts `demo governance --json` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [interactive-policy.sh](../../skills/governance-demo/interactive-policy.sh) | `interactive-policy [--policy-template <template>] [--test-scenarios]` | `t81 demo interactive-policy`; **no matching handler**; `exec` loses mode; fake argv starts `demo governance --json` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [compliance-demo.sh](../../skills/governance-demo/compliance-demo.sh) | `compliance-demo [--industry finance\|healthcare\|government] [--regulations]` | `t81 demo compliance`; **no matching handler**; `exec` loses mode; fake argv starts `demo governance --json` | Fake-CLI argv only; no successful real operation established. See package limits. |

## model-loader

Definition: [skills/model-loader/SKILL.md](../../skills/model-loader/SKILL.md); main wrapper: [skills/model-loader/load-hf.sh](../../skills/model-loader/load-hf.sh).

Actual model interface is import/diff, not these seven routes. HF downloads, security scan/quarantine and general converters are not implemented here. Duplicate --output cases shadow probe/report settings.

**Parser options inspected:** `--output`, `--canonfs-root`, `--policy`, `--detailed`, `--layer-info`, `--output`, `--check-determinism`, `--security-scan`, `--output`, `--to-format`, `--output-dir`, `--recursive`, `--validate`, `--verbose\|-v`.

| Command / entry | Advertised syntax | Intended CLI / actual trace | Verification and limits |
| --- | --- | --- | --- |
| [load-hf.sh](../../skills/model-loader/load-hf.sh) | `load-hf <model_name_or_path> [--output <output_file>] [--canonfs-root <path>] [--policy <policy_file>]` | `t81 model load-hf`; **no matching handler**; fake argv starts `model load-hf /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [load-gguf.sh](../../skills/model-loader/load-gguf.sh) | `load-gguf <gguf_file> [--output <output_file>] [--canonfs-root <path>] [--validate]` | `t81 model load-gguf`; **no matching handler**; `exec` loses mode; fake argv starts `model load-hf /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [load-safetensors.sh](../../skills/model-loader/load-safetensors.sh) | `load-safetensors <safetensors_file> [--output <output_file>] [--canonfs-root <path>]` | `t81 model load-safetensors`; **no matching handler**; `exec` loses mode; fake argv starts `model load-hf /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [probe-model.sh](../../skills/model-loader/probe-model.sh) | `probe-model <model_file> [--detailed] [--layer-info] [--output <probe_file>]` | `t81 model probe`; **no matching handler**; `exec` loses mode; fake argv starts `model load-hf /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [validate-model.sh](../../skills/model-loader/validate-model.sh) | `validate-model <model_file> [--check-determinism] [--security-scan] [--output <report_file>]` | `t81 model validate`; **no matching handler**; `exec` loses mode; fake argv starts `model load-hf /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [convert-model.sh](../../skills/model-loader/convert-model.sh) | `convert-model <input_file> [--to-format t81w\|safetensors\|gguf] [--output <output_file>]` | `t81 model convert`; **no matching handler**; `exec` loses mode; fake argv starts `model load-hf /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [batch-load.sh](../../skills/model-loader/batch-load.sh) | `batch-load <model_directory> [--recursive] [--output-dir <dir>] [--canonfs-root <path>]` | `t81 model batch-load`; **no matching handler**; `exec` loses mode; fake argv starts `model load-hf /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |

## policy-checker

Definition: [skills/policy-checker/SKILL.md](../../skills/policy-checker/SKILL.md); main wrapper: [skills/policy-checker/check.sh](../../skills/policy-checker/check.sh).

Only axion simulate exists but consumes a .tisc program and executes it; it is not the advertised policy dry-run API. Policy syntax validate lives under policy validate. Generic parser requires two positionals even for advertised list/validate.

**Parser options inspected:** `--policy`, `--context`, `--canonfs-root`, `--dry-run`, `--verbose\|-v`.

| Command / entry | Advertised syntax | Intended CLI / actual trace | Verification and limits |
| --- | --- | --- | --- |
| [check.sh](../../skills/policy-checker/check.sh) | `check <operation> <resource> [--policy <policy_file>] [--context <context_file>]` | `t81 axion check`; **no matching handler**; fake argv starts `axion check import` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [validate-policy.sh](../../skills/policy-checker/validate-policy.sh) | `validate-policy <policy_file> [--strict]` | `t81 axion validate-policy`; **no matching handler**; `exec` loses mode; fake argv starts `axion check import` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [test-rule.sh](../../skills/policy-checker/test-rule.sh) | `test-rule <policy_file> <rule_name> [--test-data <data_file>]` | `t81 axion test-rule`; **no matching handler**; `exec` loses mode; fake argv starts `axion check import` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [list-policies.sh](../../skills/policy-checker/list-policies.sh) | `list-policies [--canonfs-root <path>]` | `t81 axion list-policies`; **no matching handler**; `exec` loses mode; fake argv starts `axion check import` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [simulate.sh](../../skills/policy-checker/simulate.sh) | `simulate <operation> <resource> [--policy <policy_file>] [--dry-run]` | `t81 axion simulate`; matching native handler; `exec` loses mode; fake argv starts `axion check import` | Fake-CLI argv only; no successful real operation established. See package limits. |

## t81-compiler

Definition: [skills/t81-compiler/SKILL.md](../../skills/t81-compiler/SKILL.md); main wrapper: [skills/t81-compiler/build.sh](../../skills/t81-compiler/build.sh).

Only code build/disasm match intended routes. All five aliases dispatch build; --json is not parsed by wrapper; emitted build success is text despite its --json default. --optimize 2 and --format binary are incompatible.

**Parser options inspected:** `--output`, `--optimize`, `--optimization`, `--target`, `--format`, `--addresses`, `--preserve-semantics`, `--output-dir`, `--recursive`, `--metrics`, `--complexity`, `--policy-check`, `--verbose\|-v`.

| Command / entry | Advertised syntax | Intended CLI / actual trace | Verification and limits |
| --- | --- | --- | --- |
| [build.sh](../../skills/t81-compiler/build.sh) | `build <source_file> [--output <output_file>] [--optimize level] [--json]` | `t81 code build`; matching native handler; fake argv starts `code build /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Real bare invocation succeeds; optional flags/JSON contract fail; output writes. |
| [compile.sh](../../skills/t81-compiler/compile.sh) | `compile <source_file> [--optimization space\|time\|determinism] [--target arch]` | `t81 code compile`; **no matching handler**; `exec` loses mode; fake argv starts `code build /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [disasm.sh](../../skills/t81-compiler/disasm.sh) | `disasm <bytecode_file> [--format binary\|ternary\|mixed] [--addresses]` | `t81 code disasm`; matching native handler; `exec` loses mode; fake argv starts `code build /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [optimize.sh](../../skills/t81-compiler/optimize.sh) | `optimize <bytecode_file> [--level 1\|2\|3] [--preserve-semantics]` | `t81 code optimize`; **no matching handler**; `exec` loses mode; fake argv starts `code build /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [build-batch.sh](../../skills/t81-compiler/build-batch.sh) | `build-batch <source_dir> [--recursive] [--output-dir <dir>]` | `t81 code build-batch`; **no matching handler**; `exec` loses mode; fake argv starts `code build /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [analyze.sh](../../skills/t81-compiler/analyze.sh) | `analyze <source_file> [--metrics] [--complexity] [--policy-check]` | `t81 code analyze`; **no matching handler**; `exec` loses mode; fake argv starts `code build /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |

## t81-debugger

Definition: [skills/t81-debugger/SKILL.md](../../skills/t81-debugger/SKILL.md); main wrapper: [skills/t81-debugger/debug.sh](../../skills/t81-debugger/debug.sh).

debug top-level alias is removed; vm debug/code debug exist with different contracts. Delegators lose session action, multiple required positionals fail; ALL can be unbound, PROFILE_FILE is never assigned.

**Parser options inspected:** `--policy`, `--breakpoints`, `--trace-level`, `--condition`, `--policy-only`, `--enabled-only`, `--count`, `--into`, `--over`, `--out`, `--to`, `--until`, `--state`, `--address`, `--range`, `--start`, `--stop`, `--save`, `--output`, `--filter`, `--detailed`, `--suggest-fixes`, `--granularity`, `--verbose\|-v`, `--all`, `--into\|--over\|--out`.

| Command / entry | Advertised syntax | Intended CLI / actual trace | Verification and limits |
| --- | --- | --- | --- |
| [debug.sh](../../skills/t81-debugger/debug.sh) | `debug <program_file> [--policy <policy_file>] [--breakpoints <addr1,addr2>] [--trace-level basic\|full\|policy]` | `t81 debug start`; **no matching handler**; fake argv starts `debug start /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [breakpoint.sh](../../skills/t81-debugger/breakpoint.sh) | `breakpoint set <address> [--condition <condition>] [--policy-only]`<br>`breakpoint remove <address> [--all]`<br>`breakpoint list [--enabled-only]` | `t81 debug breakpoint`; **no matching handler**; `exec` loses mode; fake argv starts `debug start /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [step.sh](../../skills/t81-debugger/step.sh) | `step [--count <n>] [--into] [--over] [--out]` | `t81 debug step`; **no matching handler**; `exec` loses mode; fake argv starts `debug start /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [continue.sh](../../skills/t81-debugger/continue.sh) | `continue [--to <address>] [--until <condition>]` | `t81 debug continue`; **no matching handler**; `exec` loses mode; fake argv starts `debug start /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [inspect.sh](../../skills/t81-debugger/inspect.sh) | `inspect [--state registers\|memory\|stack\|policy] [--address <addr>] [--range <start-end>]` | `t81 debug inspect`; **no matching handler**; `exec` loses mode; fake argv starts `debug start /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [trace.sh](../../skills/t81-debugger/trace.sh) | `trace [--start] [--stop] [--save <trace_file>] [--filter policy\|memory\|instructions]` | `t81 debug trace`; **no matching handler**; `exec` loses mode; fake argv starts `debug start /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [analyze-violations.sh](../../skills/t81-debugger/analyze-violations.sh) | `analyze-violations [--detailed] [--suggest-fixes] [--output <report_file>]` | `t81 debug analyze-violations`; **no matching handler**; `exec` loses mode; fake argv starts `debug start /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [profile.sh](../../skills/t81-debugger/profile.sh) | `profile [--start] [--stop] [--output <profile_file>] [--granularity instruction\|function\|policy]` | `t81 debug profile`; **no matching handler**; `exec` loses mode; fake argv starts `debug start /var/folders/m9/2qsp_jmj3hl5lmh13__wt1h00000gn/T/t81-skills-audit-5qmfszhn/fixture.t81` | Fake-CLI argv only; no successful real operation established. See package limits. |

## t81vm-runner

Definition: [skills/t81vm-runner/SKILL.md](../../skills/t81vm-runner/SKILL.md); main wrapper: [skills/t81vm-runner/run.sh](../../skills/t81vm-runner/run.sh).

Run/debug/trace are real native routes; inspect/run-batch absent. All five wrappers fail on unset RECURSIVE under tested Bash3.2. Native vm parser ignores args.policy; code run missing policy logs then continues. --trace-level/--breakpoints and .t81 source claims mismatch native .tisc contract.

**Parser options inspected:** `--policy`, `--weights-model`, `--trace`, `--trace-level`, `--output`, `--output-dir`, `--breakpoints`, `--state`, `--address`, `--recursive`, `--verbose\|-v`.

| Command / entry | Advertised syntax | Intended CLI / actual trace | Verification and limits |
| --- | --- | --- | --- |
| [run.sh](../../skills/t81vm-runner/run.sh) | `run <program_file> [--policy <policy_file>] [--weights-model <model_file>] [--trace] [--json]` | `t81 vm run`; matching native handler | Bash3.2 probe fails: RECURSIVE unbound. No real wrapper execution succeeds. |
| [debug.sh](../../skills/t81vm-runner/debug.sh) | `debug <program_file> [--breakpoints <addr1,addr2>] [--policy <policy_file>]` | `t81 vm debug`; matching native handler; `exec` loses mode | Bash3.2 probe fails: RECURSIVE unbound. No real wrapper execution succeeds. |
| [run-batch.sh](../../skills/t81vm-runner/run-batch.sh) | `run-batch <program_dir> [--recursive] [--policy <policy_file>] [--output-dir <dir>]` | `t81 vm run-batch`; **no matching handler**; `exec` loses mode | Bash3.2 probe fails: RECURSIVE unbound. No real wrapper execution succeeds. |
| [trace.sh](../../skills/t81vm-runner/trace.sh) | `trace <program_file> [--trace-level basic\|full\|policy] [--output <trace_file>]` | `t81 vm trace`; matching native handler; `exec` loses mode | Bash3.2 probe fails: RECURSIVE unbound. No real wrapper execution succeeds. |
| [inspect.sh](../../skills/t81vm-runner/inspect.sh) | `inspect <program_file> [--state registers\|memory\|stack] [--address <addr>]` | `t81 vm inspect`; **no matching handler**; `exec` loses mode | Bash3.2 probe fails: RECURSIVE unbound. No real wrapper execution succeeds. |

## tensor-ops

Definition: [skills/tensor-ops/SKILL.md](../../skills/tensor-ops/SKILL.md); main wrapper: [skills/tensor-ops/tensor-math.sh](../../skills/tensor-ops/tensor-math.sh).

Actual tensor CLI is canonize/hash/inspect. No six advertised standalone routes. Five aliases lose mode, reshape/stat arguments misparsed. VM tensor operations are not proof of these commands.

**Parser options inspected:** `--output`, `--precision`, `--check-integrity`, `--check-determinism`, `--to-format`, `--stats-file`, `--histogram`, `--detailed`, `--output-dir`, `--recursive`, `--verbose\|-v`.

| Command / entry | Advertised syntax | Intended CLI / actual trace | Verification and limits |
| --- | --- | --- | --- |
| [tensor-math.sh](../../skills/tensor-ops/tensor-math.sh) | `tensor-math <operation> <tensor_a> <tensor_b> [--output <output_file>] [--precision ternary\|binary]` | `t81 tensor math`; **no matching handler**; fake argv starts `tensor math add` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [tensor-reshape.sh](../../skills/tensor-ops/tensor-reshape.sh) | `tensor-reshape <tensor_file> <new_shape> [--output <output_file>]` | `t81 tensor reshape`; **no matching handler**; `exec` loses mode; fake argv starts `tensor math add` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [tensor-validate.sh](../../skills/tensor-ops/tensor-validate.sh) | `tensor-validate <tensor_file> [--check-integrity] [--check-determinism]` | `t81 tensor validate`; **no matching handler**; `exec` loses mode; fake argv starts `tensor math add` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [tensor-convert.sh](../../skills/tensor-ops/tensor-convert.sh) | `tensor-convert <tensor_file> [--to-format t81w\|safetensors\|gguf] [--output <output_file>]` | `t81 tensor convert`; **no matching handler**; `exec` loses mode; fake argv starts `tensor math add` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [tensor-stats.sh](../../skills/tensor-ops/tensor-stats.sh) | `tensor-stats <tensor_file> [--detailed] [--histogram] [--output <stats_file>]` | `t81 tensor stats`; **no matching handler**; `exec` loses mode; fake argv starts `tensor math add` | Fake-CLI argv only; no successful real operation established. See package limits. |
| [tensor-batch.sh](../../skills/tensor-ops/tensor-batch.sh) | `tensor-batch <operation> <tensor_dir> [--recursive] [--output-dir <dir>]` | `t81 tensor batch`; **no matching handler**; `exec` loses mode; fake argv starts `tensor math add` | Fake-CLI argv only; no successful real operation established. See package limits. |

## Coverage and portability

No existing test or workflow reference to `skills/`/OpenClaw was found in the
searched CMake, tests, scripts and workflows. The CLI manual parity guard passes
but does not inspect SKILL definitions. Underlying tests do not validate wrapper
argument construction, shell semantics or their JSON templates.

Local wrapper execution used macOS ARM64 and `/bin/bash` 3.2.57. The wrappers
advertise darwin/linux only; Windows, WSL/OpenClaw gateways, Linux shells, x86-64,
and shell-version comparisons were not executed. In current GNU Bash semantics,
`((i++))` initially returns nonzero under `set -e`, creating an earlier parser-exit
risk; the Bash3.2 results must not be generalized to those hosts. See the
[GNU Bash set builtin](https://www.gnu.org/software/bash/manual/html_node/The-Set-Builtin.html)
and [conditional constructs](https://www.gnu.org/software/bash/manual/html_node/Conditional-Constructs.html).

No wrappers are assigned DCP status by this audit. The governance register
classifies underlying surfaces, not these shell adapters or arbitrary host side effects.
