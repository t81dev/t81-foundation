# PR triage automation (proposed; not activated)

The workflow `.github/workflows/pr-triage.yml` inventories every open PR daily and produces a JSON artifact. It never merges, rebases, changes branch protection, or runs experiments.

## Conservative closure gate

The daily schedule is read-only. The workflow-dispatch job can close a PR only when a maintainer has committed a specific `{ "pr": 123, "merged_successor": 456, "head_sha": "<40-hex SHA>" }` entry to `docs/governance/pr-triage-closures.json`, the successor is confirmed merged, the pinned old head is an ancestor of the successor's merge commit, and the PR head has not changed. The tool comments with evidence before closing. This deliberately refuses squash/reimplementation equivalence: those cases require manual verification and closure. Empty authorizations mean no automatic closures.

## Limitations

A Git ancestor proves commit inclusion, not that research conclusions, source files, evidence, or governance semantics remain scientifically valid. Maintainers MUST separately review preservation and scientific claims before adding a closure authorization. PR review/CI/mergeability are NOT certified by the inventory and every other PR remains HOLD_REVIEW. Workflow activation requires a separate reviewed merge and repository Actions permission to write issues and pull requests. The workflow uses the repository GITHUB_TOKEN only and is fail-closed on API errors.

## Validation before acceptance

Run `python3 -m py_compile scripts/ci/pr_triage.py`; inspect YAML triggers and permissions; run dry-run with read-only token; verify empty closure list cannot mutate; test mismatched SHA, unmerged successor, and non-ancestor cases with controlled API responses; review GitHub Actions run results. No claim of hosted validation is made by this proposal.
