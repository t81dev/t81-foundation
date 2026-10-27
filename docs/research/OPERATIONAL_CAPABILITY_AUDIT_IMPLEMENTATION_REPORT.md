# Operational audit implementation and limitations report

Date: 2026-10-27. Author/role: Codex implementation engineer and auditor; exact
underlying model identity unrecorded. This report records executed commands and
retained negative findings, not independent scientific replication.

## Baseline, isolation and artifacts

- Repository baseline: `fabc7aa90f70ead227ec257a1ae6833fa16ced40`, merged PR608.
- Separate branch/worktree: `audit/operational-agent-skills`,
  `/private/tmp/t81-operational-audit`. Original experiment checkout and its
  untracked `build-bench/` were left alone. PR609 and PR610 were not modified.
- Canonical H01–H08 register SHA-256:
  `af8ca65dcef136f294eb33010b5a6621875a52db4f3e60b94a4e28db5318a1c1`.
  Register, charters, specs, production source, wrappers and governance activation
  are unchanged. The JSON source catalog pins inspected files individually.
- Research Commons discovery was inspected separately at draft commit
  `43df109b01fc88f67375f18cd41ff563982830b3`. Its lookup and validation were
  read-only; no proposed correction was promoted into main evidence.

Deliverables: [operational report](OPERATIONAL_CAPABILITY_AUDIT.md),
[69-command matrix](AGENT_SKILLS_COMPATIBILITY_MATRIX.md),
[JSON inventory](evidence/operational_capabilities.json), two isolated probe
collectors, external-schema checker, evidence regression test and retained raw
receipts under [operational-audit](evidence/operational-audit/).

## Exact local environment

macOS 26.5.2 ARM64; Python 3.14.3; AppleClang 21.0.0
(`clang-2100.1.1.101`); CMake 4.3.0; Ninja 1.13.2; Git 2.53.0.
Wrappers executed with `/bin/bash` 3.2.57(1)-release
`arm64-apple-darwin25`. Linux/current Bash, Windows/WSL, x86-64 and the OpenClaw
gateway were not executed. GNU manual-based arithmetic/errexit implications are
classified as portability inference, not local cross-platform results.

A pre-existing Asio source copy was used for an offline build:
`/Users/t81dev/Code/t81-foundation/build/_deps/asio-src`, reporting
`ASIO_VERSION 103002` (1.30.2). Its `asio/include/asio/version.hpp` SHA-256 is
`deaf37654f0e1931af56692f3b0f7b458527cafd63f162cca50bd3ee455bd4ac`.
This version/header pin is not a complete third-party dependency closure.
The built CLI SHA-256 is
`d6a88ee8b445d09a1c93592c7b75323a590839b17d9427873a208050b5e38c0d`.
The binary remains at `/private/tmp/t81-operational-build/t81`; it is not
published as a frozen product artifact or claimed bit-identical across toolchains.

External JSON Schema checks used jsonschema 4.23.0 in the existing temporary
venv `/private/tmp/t81-commons-venv` (no production dependency change). Exact
transitive versions were attrs 26.1.0, jsonschema-specifications 2025.9.1,
referencing 0.37.0 and rpds-py 2026.9.1. Reproduction requires that validator
or explicitly reports the schema test skipped; a skip is not conformance.

## Commands actually executed

From the audit worktree, after explicitly fetching main and creating the isolated
branch (no auto-fetching is added to an interface):

```sh
cmake -S . -B /private/tmp/t81-operational-build -G Ninja -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_TESTS=ON -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_BENCHMARKS=OFF -DT81_BUILD_TUI=OFF -DT81_BUILD_FUZZ_TESTS=OFF -DT81_ENABLE_TERNARYOS=OFF -DFETCHCONTENT_SOURCE_DIR_ASIO=/Users/t81dev/Code/t81-foundation/build/_deps/asio-src
cmake --build /private/tmp/t81-operational-build --parallel 4 --target t81 t81_cli_contract_test canonfs_policy_boundary_test canonfs_identity_contract_test t81_vm_predispatch_policy_deny_logging_test tisc_opcode_family_semantics_test
python3 -B docs/research/evidence/operational-audit/probe_skills.py --output /private/tmp/t81-skills-wrapper-observations-complete.json
python3 -B docs/research/evidence/operational-audit/probe_cli.py --t81-bin /private/tmp/t81-operational-build/t81 --output /private/tmp/t81-operational-cli-observations-final.json
cmake --build /private/tmp/t81-operational-build --parallel 4 --target t81_vm_stubbed_opcode_fail_closed_test
ctest --test-dir /private/tmp/t81-operational-build --output-on-failure --no-tests=error -R '^t81_vm_stubbed_opcode_fail_closed_test$'
python3 scripts/ci/check_cli_docs_parity.py --t81-bin /private/tmp/t81-operational-build/t81 --manual docs/user-guide/reference/cli-user-manual.md
/private/tmp/t81-commons-venv/bin/python -B docs/research/evidence/operational-audit/check_output_schemas.py --output docs/research/evidence/operational-audit/schema-validation.json
/private/tmp/t81-commons-venv/bin/python -B docs/research/evidence/operational-audit/test_evidence.py -v
python3 scripts/ci/check_tisc_freeze_integrity.py
python3 scripts/governance/check_root_structure.py
python3 scripts/governance/check_overclaim_guardrails.py
python3 scripts/governance/check_active_docs_link_hygiene.py
python3 scripts/governance/check_determinism_claims.py
git diff --check
```

The five-test CTest selection was launched from Python with a temporary HOME and
store in the child environment, rather than changing the user's environment:

```text
ctest --test-dir /private/tmp/t81-operational-build --output-on-failure --no-tests=error -R '^(t81_cli_contract_test_runner|canonfs_policy_boundary_test|canonfs_identity_contract_test|t81_vm_predispatch_policy_deny_logging_test|tisc_opcode_family_semantics_test)$'
```

Collectors similarly use fresh scratch HOME/store/output paths and trusted,
self-authored fixtures. The wrapper collector places a fake `t81` first on PATH;
61 ordinary invocations reach that fake backend and five VM runner invocations
fail before it. Fake-backend exit zero is not successful T81 execution. Two
benign `printf` substitution probes and a missing-policy preflight probe are
recorded separately. All 66 scripts pass `bash -n`, which does not validate
execution semantics.

The native collector executes 69 missing-input intended-route probes and records
each exact argv, exit code, stdout/stderr and JSON/text result. It then creates a
small print program, trivial policy, local one-tensor SafeTensors file and
scratch stores for the retained positive/negative cases. It performs no model
inference, downloads, host cleanup, arbitrary supplied execution or external API
call. The policy-negative cases execute only that benign print program.

Collector output paths must not already exist. For a reproduction, select new
output paths and preserve differences instead of overwriting retained evidence.
Review/pin the wrappers before running: a fake CLI is **not an OS sandbox**.
No untrusted future wrapper revision is authorized by these audit scripts.
Scratch snapshots compare filenames and content hashes, not syscall-level writes,
same-byte rewrites, transient effects or all external paths.

## Actual results and interpretation

- Configure/build: **PASS**. Duplicate-library linker warnings remain; compiler
  AVX2/FMA feature probes fail normally on this ARM64 host. No x86 performance
  measurement or native ternary hardware inference follows.
- Real direct operation types: **11 successful** local fixture demonstrations.
  Bare wrapper import/build also succeed (overlap with those operation types),
  but remain unsuitable for untrusted inputs. Only 8/69 intended skill command
  paths have matching CLI handlers; this is a route count, not a compatibility score.
- Existing CTests: **5 passed, 1 failed** across the selected five plus the separate
  stub test. Identity/policy-boundary/pre-dispatch/opcode-family/stub-denial tests
  passed. The CLI contract runner failed at missing model fixture line 491;
  subsequent assertions did not execute. Raw logs remain in `ctest.txt` and
  `stub-test.txt`. No fixture was manufactured to make the existing suite green.
- External schema checks: **1 pass, 5 failures**, retained in
  `schema-validation.json`. CanonFS emitted fields are rejected by companion RFC
  schemas; model-import passes. Runtime validation and external-schema conformance
  are different observed outcomes. No schema was changed.
- The first evidence-test attempt had six passes and one schema validation error.
  That exposed a real contract mismatch. The final **7/7 evidence tests pass** by
  checking source pins, coverage/counts, negative findings, exact recorded schema
  failures and unchanged scientific identities. They certify the audit record's
  consistency, **not** that the failing production contracts are repaired.
- CLI manual parity: **PASS**, 24 topics, 224 manual lines. This guard does not scan
  skills. No pre-existing skills/OpenClaw coverage was found in the searched
  CMake, tests, scripts or workflows at the pinned baseline.
- Freeze registry (209 values), root structure (81 entries), overclaim, active
  links, determinism claim boundaries and whitespace: **PASS**. Registry integrity
  is not executable opcode conformance; its selected test passed separately.
- Fresh draft Commons `--validate` and H08 lookup: **PASS**, 39 sources and manifest
  `a3ee2d288ac7b00176beae32f854c373194b11556d83293396bbcd7c02d688bf`.
  H08 C/E and comparative benefit untested remain explicit. See
  `commons-observation.json`; previous Commons test runs are not recounted as new
  audit executions. Its draft source is distinct from main.

Severity tally: two HIGH, six MEDIUM and one LOW; no CRITICAL finding claimed.
High findings are shell eval and CLI policy binding. The rest include compatibility,
contract and reproducibility issues, rather than nine proven security exploits.
The raw receipts preserve contradictory successes and failures without assigning
research advantage, model quality or general certification.

## Acceptance and remaining limits

This is a new separate audit PR; it does not update PR609/610, repair production
wrappers/CLI/schema behavior, change accepted outcomes, install a transport,
activate governance, or authorize future execution. The operational report's
remediation steps are proposals requiring separate authorization.

No complete CTest run, independent model review, general hostile-host test,
external OpenClaw gateway, Linux/Windows execution or cross-platform performance
comparison occurred. No OS-level network/filesystem isolation claim is made from
these collectors. The missing contract fixture and output-schema discrepancies
remain unresolved. An independent human should review the exact source pins,
negative receipts, baseline competence, host permissions, and remediation scope
before accepting the audit or exposing any interface to external agents.

## Publication baseline check

While this audit was being published, main advanced to
`903ea2fe653df0d636e0543f9788dd8442738045` (PR611 research acceptance/integration
material). The evidence baseline remains the original `fabc7aa9`; no measurements
were silently repinned. A Git tree comparison of `skills`, `tools/cli`,
`tools/model`, `vm`, `fs`, `kernel`, `include`, `spec`, `tests/cpp` and root
`CMakeLists.txt` found **no changes** between those two revisions. Thus the
operational source paths behind the recorded findings are unchanged at that
publication snapshot. Later research/Commons material was not re-audited or
promoted into independent findings. Descriptions of draft Commons above refer
to the explicitly pinned source record; current PR/main status must be checked
separately. PR609 and PR610 were not modified by this audit.
