# RFC-00D7-F01 preparation-only verification

Status: source prepared; A–F NOT EXECUTED; Linux NOT QUALIFIED.

The human authorized non-production harness preparation and static verification.
No candidate/oracle function, control, case generator or execution entry point
was invoked. No A–F inputs were enumerated and no result packet was created.
JSON schemas, source recipes and installed dependency metadata are preparation
artifacts, not experimental instances or findings.

## Exact identities

- Reviewed protocol before clarification: `f59f97c0eedb9dc92e56a34b76e608667b7715f5`.
- Clarified protocol commit: `c8ae2200bc37064282ffc4aaa683ed21d94d567c`.
- Clarified protocol SHA-256: `0c7e7da921d3af3c6c314ec4a8bf23478f9e05695b28b808721a45475106975c`.
- Contract source: `ec22a7588282d37ad027cff61c807bcf159ac2e0`.
- Harness sources: pinned by `identities.json` and the containing Git commit.
  This check record and the identity manifest itself are covered by Git rather
  than recursively including themselves in that manifest.

## Commands and observed outcomes

Run from the experimental worktree root:

```sh
/private/tmp/t81-pr795-parity-venv/bin/python -B experiments/rfc_00d7_f01/static_check.py
/private/tmp/t81-pr795-parity-venv/bin/python -B tools/research_commons/commons.py --validate-contribution docs/research/proposals/RFC-00D7-F01/proposal.json
git diff --cached --check
```

Static-check output (exit 0):

```json
{"artifact_hashes":4,"candidate_oracle_invocations":0,"compiled_modules":8,"cross_module_calls_checked":48,"execution_qualified":false,"experimental_states_enumerated":0,"locked_distributions":5,"model_imports":0,"preparation_only":true,"schemas_checked":2,"source_pins":9}
```

The zero-execution fields describe this checker's code path; they are not
experimental coverage counters. Compilation used Python `ast.parse` and
`compile` in memory without `exec`, model imports or writing bytecode.
Schema checks used `Draft202012Validator.check_schema` on definitions, never
F's synthetic manifests. The checker verified nine Git-object/source pins,
four proposal artifact hashes, prepared-file hashes, five installed distribution
file inventories, schema-copy identity, and import separation by AST inspection. Forty-eight cross-module call
signatures were checked against function definitions without model imports.
Commons validation returned `valid_proposal_shape`, `execution: unavailable`.
Whitespace checks cover tabs/trailing spaces in Python sources and staged diff
formatting. No Black/Ruff executable was available; no formatter was installed.
No runtime regression, sanitizer, platform, resource-probe or A–F test was run.

The environment is macOS 26.5.2 ARM64, CPython 3.14.3, using the existing named
virtual environment. A first attempt to collect dependency metadata with default
`python3` failed because `jsonschema` was not installed there; the existing
virtual environment was then used. No dependency installation occurred.

## Limits of these checks

All model/control expectations remain predictions pending execution and
independent review. Same-author source separation is not independent scientific
validation. State reservations count explicit comparison slots and instrumented transitions;
they are exact budget charges, not claimed completed-state measurements after
an interrupted call.
Hard live-memory enforcement and aggregate CPU/deadline cleanup have not been
qualified. The macOS dependency lock intentionally blocks Linux execution until
a reviewed Linux re-lock. Linux x86_64/ARM64, macOS execution and Windows execution
are all unverified; only proposed Linux execution is supported by the gate.

Do not infer readiness from syntax/schema checks or publication of this branch.
See `LINUX_QUALIFICATION.md` for the separate proposed qualification and one-run
authorization procedure. All research/release holds remain unchanged.
