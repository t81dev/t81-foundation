# CI Workflow Confirmation

Status: Active
Last Updated: 2026-11-13


## November 13 enforcement and CI audit

- **Retained PR #537 boundary limitation:** The merged PR #537 experiment found that the configured CanonFS text-policy hook allowed its denied, revoked, and malformed-policy output cases. Historical compliance notes do not establish enforcement at that boundary; no full inventory or system-wide security result is claimed.
- **Observed `AI Experiments CI` freshness failure on PR #782:** Run [43529007245](https://github.com/t81dev/t81-foundation/actions/runs/43529007245/job/132241756918) (job `132241756918`, `test-ai-experiments` at `c1fddc53dd03990b3475f5986ba4e53d8a12c00e`) failed at `Validate AI status document freshness` (`scripts/ci/check_ai_status_doc_freshness.py`) with `age_days=8 > max_age_days=7` on `docs/status/AI_RFC_BACKLOG.md`, `docs/status/CI_WORKFLOW_CONFIRMATION.md`, and `docs/status/CI_GATE_STATUS.md`.
- **Freshness gate ordering limitation:** Because `Validate AI status document freshness` executes before CMake configuration and test execution in `.github/workflows/ai-experiments-ci.yml`, stale status documents fail the entire `test-ai-experiments` job independently of the PR's code changes. Refreshing these documents does not alter `max_age_days=7` or establish that downstream AI stages pass before a run completes on the refreshed state.


## AI Experiments CI Workflow

This document confirms the active trigger and gating posture of `.github/workflows/ai-experiments-ci.yml` (`AI Experiments CI`).

### Triggers

- **Push:** Runs on `main` and `feature/ai-*` when changes match AI experiment, AI CI script/keyring, `CMakeLists.txt`, or tracked AI status document paths (`docs/status/AI_RFC_BACKLOG.md`, `docs/status/CI_WORKFLOW_CONFIRMATION.md`, `docs/status/CI_GATE_STATUS.md`, etc.).
- **Pull requests:** Runs on PRs touching the same path-filtered surface (or when shared workflow inputs such as `CMakeLists.txt` or tracked status/script files are present in the PR diff).
