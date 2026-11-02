# PR #656 implementation validation

Disposition: **request changes at the reviewed head**. The local correction resolves the reproduced defects in the tested corpus; it requires maintainer review and grants no merge or governance approval.

Reviewed PR: https://github.com/t81dev/t81-foundation/pull/656

Latest head checked at start and again after testing: `7daafeb55ae241e21cfbe4bdb4a74bb911595fa7`, branch `research/current-state-v1`. Base: `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`. See `pr-metadata.json`. Corrections are prepared for publication on branch `review/pr-656-validator-portable` with the original candidate as its parent; the original checkout was left alone. Corrected source byte identities are in `corrected-results.json`, so results do not depend on an unrecorded working-tree version.

Review relationship: Codex implementation review requested by the maintainer. No PR authorship or earlier review context was supplied in this session; I did not author the reviewed head in this session. I executed these checks independently of the PR's reported validation, then authored the corrections and replay harness. This is AI-assisted implementation review, not independent human scientific validation. Exact served model identity is not known. No scientific experiment, historical evidence edit, merge, publication, or governance activation was performed.

## Findings at the pinned head

1. **P1 — optimized Python bypasses validation.** `scripts/validate_current_state.py:14–38,41` uses `assert` for validation. `python3 -O` removes constant, enum, type, required-field, property, pattern, minimum and purpose checks. For example, changing `authorization.merge` to `true` or the observed SHA to a non-SHA returns exit 0 and the normal success message. Parsing and some incidental operations can still fail; this is not a claim that every malformed input passes. The malformed/trailing-comma JSON fixtures still fail parsing.
2. **P2 — false authorization accepts numeric zero.** At validator line 14, Python's `0 == False` accepts `0` for each of `merge`, `performance_measurement`, and `production_activation`, while JSON Schema `const: false` rejects it. Under `-O`, `true`, `1`, strings, arrays and objects can also pass these fields.
3. **P2 — dates and schema semantics disagree.** `date.fromisoformat` at line 35 accepts `20261031` and `2026-W44-6` on Python 3.14, while Draft 2020-12 date checking rejects them. Both `observed_date` schema fields have no format constraint, so both implementations accept impossible dates there. The schema accepts purpose strings that the Python-only prefix assertion rejects. The Python integer check rejects `count: 1.0`, which JSON Schema treats as an integer. The SHA regex's `$` permits a trailing newline in the reference schema validator, while Python `fullmatch` rejects it. These are contract mismatches even without optimization.
4. **P2 — unsupported authorization claim.** The new state says `protocol_amendment: "user_delegated_with_reason"`, without naming an approving human, decision, reason, scope, date or source receipt. Its schema allows any string, including `"authorized"`. Neither this string nor this review establishes prior delegation. The local correction uses the constant `not_granted_by_this_index`; applicable external human decisions remain separate.

## Commands, environment and retained outputs

Primary test interpreter: Python 3.14.3. Host identity, architecture, operating-system build and local installation paths are intentionally excluded from the published review package. The independent schema reference was `jsonschema==4.26.0`, `Draft202012Validator` with **explicit `FormatChecker()`**. Without format assertions, date agreement is not the tested contract. The production validator remains standard-library only. Full dependency versions are pinned in `requirements.txt`.

Portable reproduction setup (run from repository root; substitute your environment activation convention as needed):

```sh
git fetch origin pull/656/head
python3 -m venv .venv-pr656
.venv-pr656/bin/python -m pip install --no-cache-dir -r review/pr-656/requirements.txt
```

Run from repository root, both **before and after** correction:

```sh
python3 scripts/validate_current_state.py
python3 -O scripts/validate_current_state.py
```

Each returned exit 0 and exactly:

```text
CURRENT_STATE: structure valid; provenance and freshness require live verification
```

This positive fixture alone does not expose the bypass. Adversarial replay:

```sh
.venv-pr656/bin/python review/pr-656/check.py --original --output review/pr-656/original-results.json
.venv-pr656/bin/python review/pr-656/check.py --output review/pr-656/corrected-results.json
```

Both harness commands exited 0. The original run intentionally records discrepancies rather than treating their reproduction as a harness failure. Outputs:

```text
original: 131 fixtures; 278 executions; 107 schema/exit or cwd mismatches
  optimized=False: 9 mismatches
  optimized=True: 98 mismatches
corrected: 131 fixtures; 278 executions; 0 schema/exit or cwd mismatches
  optimized=False: 0 mismatches
  optimized=True: 0 mismatches
```

The JSON receipts retain **every exact fixture payload, exit code, schema verdict/error and source SHA-256, plus path-normalized commands, working directories, stdout and stderr**, including original tracebacks. The harness reads original bytes with `git show <pinned-head>:<path>` and creates temporary fixture repositories; it never corrupts real state to test a rejection. Only runtime path prefixes are normalized: `${FIXTURE_ROOT}` names the disposable layout and `${PYTHON_STDLIB}` names the interpreter standard library. These tokens apply inside tracebacks as well as commands and cwd fields. Python version and dependency versions remain because they affect reproduction; host identifiers and operating-system build strings are omitted. All other diagnostic text is preserved. The path-normalization change does not alter the fixture construction or checked source identities.

Coverage includes deletion of every required property at every represented object, unexpected properties at every represented object, malformed/trailing-comma JSON, non-JSON NaN, invalid SHA alphabet/case/length/type/newline, impossible/basic/week/timestamp dates on all three date fields, incorrect authorization values/types, invalid enum/type/minimum, purpose strings, and integral/fractional numeric controls. Agreement is established for this corpus and inspected schema vocabulary, not all possible future schemas or all JSON inputs.

There are 262 fixture executions (normal and `-O`) plus 16 valid-fixture path checks per source version. All 16 path checks pass in both versions: repository root, nested `docs`, unrelated directory, filesystem `/`, each invoked using the absolute script path and a symlink, in both Python modes. This verifies that lookup uses the resolved script location rather than the caller's cwd. Running a relative `scripts/...` path from outside the repo naturally requires an appropriate script path.

Additional corrected positive smoke checks, all exit 0 with the same success output:

```sh
python3.12 scripts/validate_current_state.py
python3.12 -O scripts/validate_current_state.py
python3.9 scripts/validate_current_state.py
python3.9 -O scripts/validate_current_state.py
```

Those interpreters report Python 3.12.13 and 3.9.6 respectively. The full adversarial matrix ran on Python 3.14.3 only. Other platforms and hosted CI were not executed; no cross-platform pass is claimed. No C++ runtime change was made, so CMake/CTest were not run.

Research Commons read-only context check:

```sh
.venv-pr656/bin/python -B tools/research_commons/commons.py --request '{"protocol":"t81.research-commons.v1","operation":"list_research_tracks","arguments":{}}'
```

Exit 0; exact canonical JSON stdout retained in `commons.stdout.txt`; `commons.stderr.txt` is empty. It returns Tracks R/F/M with canonical H01–H08 and explicitly disclaims independent scientific/live-CI verification. This is contextual discovery, not experiment authorization.

Operational failures retained here: the first sandboxed `git fetch` failed with `error: cannot open '.git/FETCH_HEAD': Operation not permitted` (255), then succeeded with escalation. The first `import jsonschema` and the first system-Python Commons invocation failed with `ModuleNotFoundError: No module named 'jsonschema'` (1); the temporary venv resolved this. The initial un-escalated pip install encountered DNS/network retries. A shell inventory command incorrectly used `command -v -a`, yielding `zsh:1: command not found: -v` (127); later explicit interpreter paths were used. A Commons wrapper used zsh's read-only `status` variable and failed with `zsh:1: read-only variable: status` (1); the command was rerun with `review_exit` and returned 0. None of these setup/wrapper failures is counted as successful validation or a product defect.

## Freshness, provenance and authority limits

Read-only GitHub calls were `github.get_pr_info(repository_full_name="t81dev/t81-foundation", pr_number=656)` and `github.fetch` on the exact `head_source` and open-PR API URLs in the state. These are observations, not authenticated provenance receipts.

At review time, live `main` was `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`, matching the dated state (receipt: `main-observation.json`). The open-PR endpoint returned **53** entries; the dated state says **49**. `live-github.json` retains retrieval time, endpoint, numbers, heads and states; all eight listed critical PRs were open in this response. This later difference does not prove the historical count was false. The count/date were deliberately preserved rather than silently refreshed. Date-only timestamps cannot establish observation ordering or freshness within a day.

The two scientific source paths were inspected with:

```sh
git show e108fdad:docs/research/INVESTIGATION_10A_PHYSICAL_STORAGE_EVIDENCE_ASSESSMENT.md
git show e108fdad:experiments/h04_r02_adaptive_representation/CORRECTED_RESULTS.md
```

Both returned exit 0. Their content supports the index's bounded *source-reported* descriptions: physical characterization remains blocked, and H04-R02's conditional abstract-cost result is not hardware proof. No scientific result was independently reproduced here. Source strings lack their own revision/hash receipts; existence and structural validity do not establish authorship, truth, source identity or independent execution. Even the `independently_reproduced` enum value is merely structurally permitted, not verified by this validator. The PR's phrase “provenance-aware” must be read as recording source references, not validating provenance. The CLI and appended START_HERE paragraph correctly disclaim authenticity, live freshness and authority.

## Corrective scope and disposition

Corrections replace assertions with explicit failures, distinguish JSON boolean constants from numbers, reject non-JSON constants, require calendar-valid YYYY-MM-DD dates, align integral numbers and regex matching with JSON Schema, and move purpose checking into the schema. SHA `maxLength: 40` closes the schema's trailing-newline gap. Date formats now cover both observation dates. The unreferenced delegation string becomes a no-authority sentinel. UTF-8 reads are explicit. Unsupported schema keywords fail when encountered; this remains a bounded validator for the checked-in vocabulary, not a general JSON Schema implementation.

No dated scientific findings, historical artifacts, observed SHA/count/date, accepted RFC, policy boundary or governance control was changed. The original defect receipts remain separate from corrected receipts. `git diff --check` passed (exit 0, empty stdout).

Keep #656 draft / request changes until these corrections are reviewed. The corrected corpus supports targeted implementation acceptance after review, not live-data certification or merge approval. Future state updates still require source and human-decision checks outside this validator.

Final record checks: the three adjacent Commons-schema records validated successfully (`independent-review: schema valid`, `reproducibility-report: schema valid`, `evidence-discrepancy: schema valid`), and the executed corrected byte hashes were rechecked (`Corrected source hashes match executed receipts`).


## Corrective integration publication checks

The candidate pin remains `7daafeb55ae241e21cfbe4bdb4a74bb911595fa7`. The unpublished local correction was verified to have that candidate as its direct parent; its identity is retained only in local review records. Its three corrective implementation files are preserved byte-for-byte; publication preparation changes only the review package and normalizes host paths in fresh receipts. The unsupported delegation label is absent from the actual state, which uses `not_granted_by_this_index`; the old label remains only as an explicitly rejected test input and reported defect.

Both full matrices were rerun for publication: 131 fixtures and 278 executions for each version; original discrepancies reproduce (9 normal, 98 optimized), corrected discrepancies remain zero in both modes. All 16 cwd checks pass for each version. Payloads, exit codes, schema verdicts and checked source identities were compared with the pre-publication receipts to detect unintended changes during normalization. No original candidate file was deleted; the START_HERE addition remains intact. The proposed publication commit is based directly on the candidate, avoiding publication of the original corrective commit's host-specific receipt history. The original corrective commit remains local. The final publication head will be reported separately after Git records its content identity.

The published replacement supersedes the unpublished local corrective candidate. Publication is authorized only to the remote review branch; it grants no merge or governance authority. The original corrective SHA and intermediate preparation commits are excluded from the outgoing ancestry and review package.

Final authorized rerun: both complete receipt files reproduced byte-for-byte after documented path normalization. Original: 131 fixtures, 278 executions, 9 normal and 98 optimized discrepancies. Corrected: 131 fixtures, 278 executions, zero normal or optimized discrepancies. Both versions passed all 16 working-directory checks. The five candidate commits and the complete replacement diff were inspected; the replacement has the candidate as its sole parent. Destination: `review/pr-656-validator-portable`; normal push only.
