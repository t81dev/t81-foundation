# CI Workflow Confirmation

Status: Active
Last Updated: 2026-03-07
Owner: @t81dev

Confirms the operational state of the AI Experiments CI workflow.

---

## AI Experiments CI Workflow

The `ai-experiments-ci.yml` workflow validates AI-facing surfaces, CLI tooling,
and status document freshness on every push to `main` and on pull requests
touching `docs/status/`, `tooling/`, or `experimental/` paths.

Current state: **Active** — workflow runs on push/PR triggers; all steps passing
as of 2026-03-07 (candidate `b566bff8`).

---

### Triggers

| Trigger | Condition |
| :--- | :--- |
| `push` | Paths: `docs/status/**`, `tooling/**`, `experimental/**`, `.github/workflows/ai-experiments-ci.yml` |
| `pull_request` | Same path filters |
| `workflow_dispatch` | Manual trigger available |

---

### Steps

| Step | Gate | Last Status |
| :--- | :--- | :--- |
| Validate AI status document freshness | Required | Pass ✅ |
| Build AI CLI target (if present) | Informational | Pass ✅ |
| Run AI CLI smoke tests | Gated on build | Pass ✅ |

---

### Required Status Documents

All documents listed in `scripts/ci/ai_status_doc_freshness_expectations.json`
must exist, contain required section headers, and have a `Last Updated:` date
within the declared `max_age_days` window.

| Document | Max Age | Status |
| :--- | :--- | :--- |
| `docs/status/AI_RFC_BACKLOG.md` | 7 days | Pass ✅ |
| `docs/status/CI_WORKFLOW_CONFIRMATION.md` | 7 days | Pass ✅ |
| `docs/status/AI_CLI_MILESTONE_EVIDENCE.md` | 14 days | Pass ✅ |
| `docs/status/CI_GATE_STATUS.md` | 7 days | Pass ✅ |
| `docs/status/EXTENSION_PROFILE.md` | 14 days | Pass ✅ |
| `docs/status/HARDENING_BACKLOG.md` | 14 days | Pass ✅ |

---

## Cross-References

- `scripts/ci/ai_status_doc_freshness_expectations.json`
- `.github/workflows/ai-experiments-ci.yml`
- `docs/status/AI_CLI_MILESTONE_EVIDENCE.md`
- `docs/status/CI_GATE_STATUS.md`
