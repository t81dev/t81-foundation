# Context reconciliation validation

Pinned base: `40695b0ba5cea3e43563b6ed2443c987d2738089`, freshly fetched November 15, 2026. This PR changes documentation and documentation receipts only. No research, release, governance, source or schema contract is activated.

Executed in the candidate Git worktree:

```sh
python3 scripts/validate_current_state.py
python3 tests/ci/test_current_state_contract.py
python3 scripts/governance/check_docs_structure.py
python3 scripts/governance/check_active_docs_link_hygiene.py
python3 scripts/governance/check_docs_governance_hygiene.py
python3 scripts/ci/check_ai_status_doc_freshness.py --as-of-date 2026-11-15 --out-json docs/ai-context/receipts/2026-11-15-context-validation/freshness.json
git diff --check
```

The stdlib context schema validator passes. Contract tests pass: 14 run, one skipped because `jsonschema` is unavailable locally (13 executed). The optional direct Draft 2020-12 library check could not run for that same reason; no external validator pass is claimed. Active-document path hygiene, changed Markdown relative-link existence, freshness and whitespace checks pass. Freshness reports three existing approaching-staleness warnings; no dates were advanced to suppress them.

The initial attempt to select the new operational view exposed a pre-existing test assumption: it requires five November 2 PR states and a historical cleanup item in whichever view is selected. Rather than change tests in a documentation-only PR or insert stale PR states into a fresh inventory, `selected_view` remains the dated November 2 selection. `next_gate` explicitly directs readers to the new November 15 overlay and status. The final contract-test log is retained; the initial failed selection was corrected before publication.

Documentation structure and broad governance checks fail on both base and candidate. The same missing translated-book directories, stale required-README paths, API lock mismatch, spec-code mapping paths and RFC lifecycle metadata discrepancies are present in an unchanged Git-backed base worktree. `candidate-governance.log` and `base-governance-git-worktree.log` are identical. The initial `base-governance.log` came from an exported archive without `.git` and includes extra environment failures; it is retained as a failed method, not used for the paired baseline conclusion. No locks, RFC statuses or unrelated directories were rewritten to make these checks pass.

`preservation.log` records equality checks for all prior operational views, historical snapshots and authorization fields against base. Earlier scientific observations remain pinned to their original sources; the new view copies those dated observations without claiming fresh scientific execution. PR state observations cover only 13 named PRs, not the full backlog. Closed-without-merge PRs have null merge records even when GitHub supplies a synthetic merge SHA.

PR #799's complete 34-job response was fetched with:

```sh
gh api --paginate 'repos/t81dev/t81-foundation/actions/runs/43746367704/jobs?per_page=100'
```

The connector's first-page response omitted the informational ARMv9 job. The complete retained response reports job `132945852419` success. Its log confirms an `aarch64-linux-gnu-g++` cross-build; no native ARMv9 run is claimed. Those are PR-qualification results, not new local runs or a full pinned-main execution certificate.
