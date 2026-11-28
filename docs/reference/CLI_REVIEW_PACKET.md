# CLI Review Packet

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [CLI Review Packet](#cli-review-packet)
  - [1. Public CLI Surface](#1-public-cli-surface)
  - [2. Determinism Scope](#2-determinism-scope)
  - [3. Minimal Verification Ritual](#3-minimal-verification-ritual)
  - [4. JSON and Exit-Code Contract](#4-json-and-exit-code-contract)
  - [5. Submission Notes](#5-submission-notes)

<!-- T81-TOC:END -->


Status: Active
Last Updated: 2026-03-08
Audience: external technical reviewers

## 1. Public CLI Surface

Treat the following command families as the supported public operator surface:

- `t81 code`
- `t81 project`
- `t81 env`
- `t81 canonfs`
- `t81 determinism`
- `t81 vm`
- `t81 tisc`
- `t81 ir`
- `t81 weights`
- `t81 policy`
- `t81 axion`
- `t81 trace`
- `t81 completion`
- `t81 man`
- `t81 feedback`

Specialized but still supported:

- `t81 lang`
- `t81 tensor`
- `t81 tier`

Operations-focused or experimental:

- `t81 internal ...`
- `t81 internal llama-run`

The authoritative operator contract is
[`docs/user-guide/reference/cli-user-manual.md`](../user-guide/reference/cli-user-manual.md)
plus [`docs/product/CLI_JSON_SCHEMA_CONTRACTS.md`](../product/CLI_JSON_SCHEMA_CONTRACTS.md).

## 2. Determinism Scope

Determinism claims do not apply to every CLI command.

Reviewer entry points:

- [`docs/governance/DETERMINISM_THREAT_MODEL.md`](../governance/DETERMINISM_THREAT_MODEL.md)
- [`docs/reference/CAPABILITY_CONTRACT.md`](./CAPABILITY_CONTRACT.md)
- [`docs/reference/REPRODUCIBILITY.md`](./REPRODUCIBILITY.md)

Commands such as `env`, `completion`, `man`, `feedback`, and most `internal`
flows are host-facing operational tooling rather than deterministic proof
surfaces.

## 3. Minimal Verification Ritual

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/t81_cli_contract_test ./build/t81
python3 scripts/ci/t81lang_repro_gate.py \
  --t81-bin build/t81 \
  --fixtures-dir tests/fixtures/t81lang_determinism \
  --workdir build/t81lang-repro \
  --hash-out build/t81lang-repro/hash.txt \
  --expected-hash-file tests/fixtures/t81lang_determinism/t81lang_repro_hash.txt
```

## 4. JSON and Exit-Code Contract

- `--json` payloads are versioned by schema ID.
- pre-domain failures emit `t81.error.v1`
- canonical command spellings, not removed aliases, define the public contract
- exit-code expectations are documented in the CLI user manual

## 5. Submission Notes

- Archive material under `docs/records/archive/` is historical context, not the
  current CLI contract.
- Remove or clearly label any local-only artifacts before packaging evidence.
- Prefer canonical command spellings in all reviewer transcripts and screenshots.
