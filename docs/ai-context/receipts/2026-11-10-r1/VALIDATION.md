# Evidence and validation — T81-CONTINUITY-2026-11-10-R1

**READY FOR INDEPENDENT REVIEW — DO NOT MERGE.** This Codex assistant collected GitHub metadata, inspected pinned source and authored this reconciliation. User-supplied task history and Jules/Codex findings were prior context. No independent-person/model scientific acceptance is claimed. Local schema checks are actual execution, distinct from GitHub-reported CI and author-reported arithmetic runs.

Baseline: `dbf107fda83a3755fa21d6b95caa3eb8ac450dd3`. Correction inspected: `3969002e9d684bbf67b01aab485648aa2cd8c0d0`. Local platform: macOS 26.5.2 ARM64, Python 3.14.3 (Clang 17); Bash 3.2.57. Temporary venv: `/private/tmp/t81-continuity-venv`; pytest 9.2.0, jsonschema 4.26.0, Playwright 1.65.0, Chromium 1254 (Chrome for Testing 158.0.8127.18). Linux, Windows, native x86_64, additional ARM64 hosts, hosted CI for this new draft and production CMake builds were **NOT RUN**; these documentation/data changes introduce no platform execution path.

## Commands and outcomes

Commands below ran from `/private/tmp/t81-continuity-r1` unless otherwise specified. Logs are retained beside this report. Hashes establish artifact identity, not authenticity or factual truth.

```sh
git fetch origin main
git rev-parse origin/main
# PASS: dbf107fda83a3755fa21d6b95caa3eb8ac450dd3

gh api 'repos/t81dev/t81-foundation/pulls?state=open&per_page=100'
# PASS: 69 open PRs, 39 drafts; complete response fits one 100-item page.
gh api --paginate 'repos/t81dev/t81-foundation/branches?per_page=100' --jq '.[] | select(.name | test("scg08";"i")) | {name,sha:.commit.sha}'
# PASS: four published SCG-08 branch tips retained in pr-inventory.json.
git fetch origin 3969002e9d684bbf67b01aab485648aa2cd8c0d0
git diff --unified=0 dbf107fda83a3755fa21d6b95caa3eb8ac450dd3 3969002e9d684bbf67b01aab485648aa2cd8c0d0 -- experiments/scg08_arithmetic_work scripts/ci/validate_scg08_gate1_evidence.py
# PASS: seven-file correction diff retained in correction-candidate.patch.
# Zero-context form avoids trailing-space context lines in the outer PR diff.

python3 scripts/validate_current_state.py
# PASS: existing stdlib schema validator.
python3 scripts/ci/validate_architecture_visualization.py --drift-check
# PASS: all data invariants; zero missing paths, stale lines,
# unsupported references or inconsistencies. Historical 18/35 coverage unchanged.
python3 scripts/ci/check_tisc_freeze_integrity.py
# PASS: 209 opcodes, bitwise invariants, reserved range integrity.
python3 docs/ai-context/receipts/2026-11-10-r1/validate_reconciliation.py
# PASS: PR identities/lifecycles/counts, dashboard compatibility,
# selected-view/history preservation, canonical fields and new local links.
git diff --check dbf107fda83a3755fa21d6b95caa3eb8ac450dd3
# PASS after the retained patch was regenerated without blank context lines.
# Initial staged check flagged those literal patch-context spaces; corrected.

/private/tmp/t81-continuity-venv/bin/python -m pytest -q tests/ci/test_current_state_contract.py tests/ci/test_architecture_visualization.py tests/ci/test_architecture_engineering_diagnostics.py tests/ci/test_architecture_dashboard.py -k 'not browser_dashboard_interaction_and_2d_fallback and not bash_arithmetic_evaluation_defect_reproduction'
# PASS: 52 passed, 2 deselected, 9 subtests passed (data-contract-tests.log).
PLAYWRIGHT_BROWSERS_PATH=/private/tmp/t81-continuity-browsers /private/tmp/t81-continuity-venv/bin/python -m pytest -q tests/ci/test_architecture_dashboard.py -k browser_dashboard_interaction_and_2d_fallback
# PASS: 1 passed, 2 deselected (browser-test.log); ran outside sandbox after
# macOS bootstrap launch was denied inside it. Initial attempt also lacked
# Chromium; installed it in the temporary directory. Test-created tracked
# screenshots were restored byte-for-byte; no visual artifact changes published.
```

Initial full four-file pytest invocation without `-k` returned **51 passed, 3 failed, 9 subtests passed**. One failure exposed the new view's omission of the historical M1 finding; the appended view now carries that finding's original revision and observation timestamp with an explicit historical label, and the complete current-state contract tests pass. Browser failure was environmental (missing Chromium, then sandbox bootstrap permission); the final existing browser test passes after setup. The third failure is preserved below, not hidden as a passing check.

**Existing platform failure reproduced:**

```sh
git archive dbf107fda83a3755fa21d6b95caa3eb8ac450dd3 tests/ci/test_architecture_engineering_diagnostics.py | tar -x -C /private/tmp/t81-scg08-correction
/private/tmp/t81-continuity-venv/bin/python -m pytest -q /private/tmp/t81-scg08-correction/tests/ci/test_architecture_engineering_diagnostics.py -k bash_arithmetic_evaluation_defect_reproduction
# FAIL: 1 failed, 19 deselected. Unchanged baseline test expects exit 1;
# local Bash 3.2 returns 0 / COMPLETED_REACHED (baseline-bash-reproduction.log).
```

This is a pre-existing test/platform discrepancy. It was not repaired because unrelated PR repairs and production/test-code changes are outside this task. The final non-browser slice explicitly deselects that one reproduced failure; it does not establish a fully passing suite. The existing hosted Foundation CI failures are separately retained as GitHub metadata; no root-cause attribution to this Bash failure is asserted.

## Schema-only correction inspection

No arithmetic suite, Markov simulator, benchmark or Gate 2 trial was executed. The following commands execute only evidence validation and malformed-document fixtures:

```sh
mkdir -p /private/tmp/t81-scg08-correction
git archive 3969002e9d684bbf67b01aab485648aa2cd8c0d0 experiments/scg08_arithmetic_work scripts/ci/validate_scg08_gate1_evidence.py | tar -x -C /private/tmp/t81-scg08-correction
python3 /private/tmp/t81-scg08-correction/scripts/ci/validate_scg08_gate1_evidence.py --test-negative
# PASS: all 12 rejection fixtures, not the 11 reported in the supplied summary.
python3 /private/tmp/t81-scg08-correction/scripts/ci/validate_scg08_gate1_evidence.py
# PASS: candidate validator accepts its own checked-in receipt.
/private/tmp/t81-continuity-venv/bin/python docs/ai-context/receipts/2026-11-10-r1/correction_schema_probe.py /private/tmp/t81-scg08-correction
# PASS of adversarial probe: two malformed band-field types ACCEPTED by
# candidate validator, each REJECTED by Draft202012Validator. This is a
# negative finding about conformance, not a candidate acceptance pass.
```

The probe source, exact [negative-fixture output](correction-negative-validation.log), [candidate evidence validation output](correction-evidence-validation.log), [schema discrepancy output](correction-schema-probe.log), and [candidate diff](correction-candidate.patch) are retained. Arithmetic counters, same-run trial linkage and scientific acceptance remain unverified/blocked, as explained in the [operational snapshot](../../T81_RESEARCH_STATUS_2026-11-10_R1.md).

Commons discovery initially could not run under bare Python because jsonschema was absent. After isolated dependency installation, this exact discovery command succeeded:

```sh
/private/tmp/t81-continuity-venv/bin/python -B tools/research_commons/commons.py --request '{"protocol":"t81.research-commons.v1","operation":"list_research_tracks","arguments":{}}'
```

It returned R/F/M from pinned Commons baseline `3d395cb604ddd49da3354d17f0505f2ed033757e`; discovery is historical evidence access, not experiment authorization. GitHub connector calls independently retrieved metadata, reviews, threads and head workflow runs for 82 PRs; complete returned bodies are retained in [github-observations.json](github-observations.json). Search results alone were capped at 20; the 69 count uses the complete REST open inventory. Returned `merge_commit_sha` on open PRs can be a synthetic test merge; normalized actual merge SHAs remain null unless `merged=true`.

## Changed-file inventory and scope

All changed files are within documentation, receipts or existing visualization status datasets. No outside-scope production, normative, TernaryOS, topology, frontend, coverage or frozen artifact file is changed.

- `docs/ai-context/START_HERE.md` — prepended pointer only.
- `docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md` — prepended pointer only.
- `docs/ai-context/CURRENT_STATE.json` — append schema-compatible view; selection/history unchanged.
- `docs/ai-context/T81_RESEARCH_STATUS_2026-11-10_R1.md` — dated operational snapshot.
- `docs/architecture/visualization/research_programs.json` — SCG-08 research status and observation source, historical baseline preserved.
- `docs/architecture/visualization/pull_requests_snapshot.json` — refresh 24 mapped PR records from observed metadata, historical baseline preserved.
- `docs/ai-context/receipts/2026-11-10-r1/github-observations.json` — retained GitHub responses and limitations.
- `docs/ai-context/receipts/2026-11-10-r1/pr-inventory.json` — normalized 82-record inventory and complete open set.
- `docs/ai-context/receipts/2026-11-10-r1/correction-candidate.patch` — inspected seven-file candidate diff, evidence only.
- `docs/ai-context/receipts/2026-11-10-r1/correction_schema_probe.py` — reproducible schema-only adversarial probe.
- `docs/ai-context/receipts/2026-11-10-r1/correction-schema-probe.log` — two residual type failures.
- `docs/ai-context/receipts/2026-11-10-r1/correction-negative-validation.log` — 12 negative fixtures.
- `docs/ai-context/receipts/2026-11-10-r1/correction-evidence-validation.log` — candidate's own receipt acceptance.
- `docs/ai-context/receipts/2026-11-10-r1/baseline-bash-reproduction.log` — retained unchanged-baseline test failure.
- `docs/ai-context/receipts/2026-11-10-r1/data-contract-tests.log` — final data/contract test result.
- `docs/ai-context/receipts/2026-11-10-r1/browser-test.log` — final browser result.
- `docs/ai-context/receipts/2026-11-10-r1/visualization-validation.log` — data integrity and zero drift.
- `docs/ai-context/receipts/2026-11-10-r1/validate_reconciliation.py` — task-local preservation and inventory checks.
- `docs/ai-context/receipts/2026-11-10-r1/VALIDATION.md` — this report.

No merge, Gate 2, measurement, governance activation or scientific acceptance is authorized by these records. Recommended next action: publish and repair the pinned SCG-08 correction candidate, obtain the missing authoritative contract/authorization record, then conduct independent exact-head Gate 1 review.
