# Validation — post-#765 follow-up to draft PR #764

**READY FOR INDEPENDENT REVIEW — DO NOT MERGE.** Observation checkpoint: **2026-11-11T01:02:53Z**, November 10 at 20:02:53 EST. Fetched main/base: `36b3849aa1e6297e88de2ec1655777793ab0b59e`; original #764 head preserved as ancestor: `48dc1435e1753b426258eeb2bb14dfd036ad686b`. This follow-up merged current main into the existing branch without conflicts. Its diff against current main changes only documentation, receipts and the two supported visualization status datasets. No arithmetic correction was authored/copied as a new branch delta, no new PR was created, and no merge of #764 is authorized.

Environment: macOS 26.5.2 ARM64, Python 3.14.3, pytest 9.2.0, jsonschema 4.26.0, Playwright 1.65.0, Chromium 1254; Bash 3.2.57. Existing isolated venv `/private/tmp/t81-continuity-venv` and Chromium `/private/tmp/t81-continuity-browsers` reused. Linux, Windows and other physical hosts, production CMake builds, arithmetic smoke suites, benchmarks, Markov simulations and Gate 2 were **NOT RUN**. Hosted CI is metadata only; no new scientific acceptance or independent-person replication claimed. This is the same Codex assistant that authored R1, with the user/Jules/Codex reports as prior context.

## Commands and outcomes

Run from `/private/tmp/t81-continuity-r1` except the explicit historical replay:

```sh
git fetch origin main codex/continuity-2026-11-10-r1
git rev-parse origin/main
# PASS: 36b3849aa1e6297e88de2ec1655777793ab0b59e
git merge --no-edit origin/main
# PASS: conflict-free integration; previous #764 commits retained.
git diff --quiet 3969002e9d684bbf67b01aab485648aa2cd8c0d0 origin/main -- experiments/scg08_arithmetic_work scripts/ci/validate_scg08_gate1_evidence.py
# PASS: merged correction paths byte-identical to inspected R1 candidate.

gh api --paginate 'repos/t81dev/t81-foundation/branches?per_page=100' --jq '.[] | select(.name | test("scg08";"i")) | {name,sha:.commit.sha}'
gh api 'repos/t81dev/t81-foundation/pulls?state=open&per_page=100' --jq '[.[] | {number,title,draft,html_url,head_sha:.head.sha,base_sha:.base.sha}]'
# PASS: five published SCG-08 branch tips; 70 open PRs / 40 drafts.
# No newer SCG-08 PR/advanced branch tip found at this checkpoint.
# GitHub connector retrieved #765/#764 metadata, #765 reviews/threads/comments
# and exact-head workflow metadata; complete observations retained in JSON.

python3 scripts/validate_current_state.py
# PASS: existing stdlib schema validator.
python3 scripts/ci/validate_architecture_visualization.py --drift-check
# PASS: data invariants and zero drift; 18/35 directory coverage unchanged.
python3 scripts/ci/check_tisc_freeze_integrity.py
# PASS: 209 opcodes, reserved range and bitwise invariants preserved.
python3 docs/ai-context/receipts/2026-11-10-r2/validate_followup.py
# PASS: immutable R1 bytes, 82 records, prior views/selection/pointers,
# #765 merge/two findings, current open-list counts, 24 historical rows plus
# one new row, canonical fields, architecture/frontend/topology and links.

git worktree add --detach /private/tmp/t81-continuity-r1-historical 48dc1435e1753b426258eeb2bb14dfd036ad686b
# In that historical worktree:
python3 docs/ai-context/receipts/2026-11-10-r1/validate_reconciliation.py
# PASS: original R1 preservation/82-record checks at their original revision.

git diff --check origin/main
# PASS: whitespace/formatting.
```

The historical R1 validation script intentionally requires only the R1 view; running it against the new R2 tree rejects the additional view. That script remains byte-identical and passes in the pinned historical worktree. The new focused preservation checker compares every R1 snapshot/receipt file against the original commit and checks the extended current state. Its initial comparison of symlink targets as regular file contents was corrected to compare link text; final checks pass. Neither event is an implementation or experimental finding.

```sh
/private/tmp/t81-continuity-venv/bin/python -m pytest -q tests/ci/test_current_state_contract.py tests/ci/test_architecture_visualization.py tests/ci/test_architecture_engineering_diagnostics.py tests/ci/test_architecture_dashboard.py -k 'not browser_dashboard_interaction_and_2d_fallback and not bash_arithmetic_evaluation_defect_reproduction'
# PASS: 52 passed, 2 deselected, 9 subtests passed (data-contract-tests.log).
PLAYWRIGHT_BROWSERS_PATH=/private/tmp/t81-continuity-browsers /private/tmp/t81-continuity-venv/bin/python -m pytest -q tests/ci/test_architecture_dashboard.py -k browser_dashboard_interaction_and_2d_fallback
# PASS: 1 passed, 2 deselected (browser-test.log).
# Run outside filesystem sandbox for the established macOS Chromium bootstrap
# requirement. Test-generated screenshots restored to exact baseline bytes.
```

The browser run exercises current dataset consumption without a frontend/layout change. No complete hosted CI pass, new coverage audit or scientific validation is inferred.

## Separate known Bash 3.2 discrepancy

```sh
/private/tmp/t81-continuity-venv/bin/python -m pytest -q tests/ci/test_architecture_engineering_diagnostics.py -k bash_arithmetic_evaluation_defect_reproduction
# FAIL: 1 failed, 22 deselected (baseline-bash-reproduction.log).
```

The unchanged test expects Bash exit 1, but local Bash 3.2 returns exit 0 and `COMPLETED_REACHED`. This is the same discrepancy retained in R1, not a new follow-up regression. The test file is identical to current main and original R1. It is executed separately and explicitly deselected in the passing data slice; no full-suite pass is claimed. No unrelated test repair was made.

## Evidence and acceptance limits

[GitHub observations](github-observations.json) retain the actual #765 head `2dd3cc62e526942cdcbed6d100770c08c26ab2fe`, merge `36b3849aa…`, `COMMENTED` review and two unresolved threads. PR-body test/head claims remain source-reported; the actual metadata head differs from the SHA in that body. At the observation, Format Check reported success; Foundation CI queued, CodeQL and cross-platform checks in progress. Pending is not passing, and those results are not local reproduction.

Remaining P1 evidence revision binding and P2 scalar band type-validation findings block acceptance. R1 accounting/same-run provenance concerns, missing authoritative Gate 1 authorization record and conditional Phase 0 statistical readiness are retained. Gate 1 scientific acceptance and Gate 2 remain withheld. Any further prepared Jules correction remains **MISSING EVIDENCE / UNVERIFIED** absent a later published exact-source observation. See the [R2 operational follow-up](../../T81_RESEARCH_STATUS_2026-11-10_R2.md).

All original R1 evidence remains byte-identical: snapshot plus 13 receipt files, including its 82-record inventory. Existing current-state operational views and historical snapshots are unchanged; only `2026-11-10-r2` is appended, and `selected_view` is unchanged. Research/PR status observations preserve their historical source baselines and original R1 rows rather than relabeling them as freshly audited.

## Changed-file inventory

The exact cumulative #764 diff against current main follows. R1 snapshot/receipt files are existing unchanged PR deliverables; follow-up changes update five existing status/pointer files and add the R2 snapshot/receipts. All paths are documentation, receipts or supported status datasets; no out-of-scope files require justification.

```text
docs/ai-context/ACTIVE_RESEARCH_HANDOFF.md
docs/ai-context/CURRENT_STATE.json
docs/ai-context/START_HERE.md
docs/ai-context/T81_RESEARCH_STATUS_2026-11-10_R1.md
docs/ai-context/T81_RESEARCH_STATUS_2026-11-10_R2.md
docs/ai-context/receipts/2026-11-10-r1/VALIDATION.md
docs/ai-context/receipts/2026-11-10-r1/baseline-bash-reproduction.log
docs/ai-context/receipts/2026-11-10-r1/browser-test.log
docs/ai-context/receipts/2026-11-10-r1/correction-candidate.patch
docs/ai-context/receipts/2026-11-10-r1/correction-evidence-validation.log
docs/ai-context/receipts/2026-11-10-r1/correction-negative-validation.log
docs/ai-context/receipts/2026-11-10-r1/correction-schema-probe.log
docs/ai-context/receipts/2026-11-10-r1/correction_schema_probe.py
docs/ai-context/receipts/2026-11-10-r1/data-contract-tests.log
docs/ai-context/receipts/2026-11-10-r1/github-observations.json
docs/ai-context/receipts/2026-11-10-r1/pr-inventory.json
docs/ai-context/receipts/2026-11-10-r1/validate_reconciliation.py
docs/ai-context/receipts/2026-11-10-r1/visualization-validation.log
docs/ai-context/receipts/2026-11-10-r2/VALIDATION.md
docs/ai-context/receipts/2026-11-10-r2/baseline-bash-reproduction.log
docs/ai-context/receipts/2026-11-10-r2/browser-test.log
docs/ai-context/receipts/2026-11-10-r2/data-contract-tests.log
docs/ai-context/receipts/2026-11-10-r2/github-observations.json
docs/ai-context/receipts/2026-11-10-r2/validate_followup.py
docs/architecture/visualization/pull_requests_snapshot.json
docs/architecture/visualization/research_programs.json
```

Total: 26 files.
