# /docs/status — Control Surface

Last Updated: 2026-03-26
Authority: `/spec` > `docs/architecture/OVERVIEW.md` > `/docs` > `/book`

This directory is a cockpit, not an archive.

If someone clones this repo and reads only `/docs/status`, they should know —
in under 5 minutes — exactly what is safe, what is experimental, and what is drifting.

Narrative documents, temporal plans, and time-bound reports live in `docs/records/`.

---

## Classification Note

- **DCP / verified deterministic surface** means the surface is inside the deterministic-core boundary and backed by registry/enforcement evidence.
- **Governed non-DCP** means the surface is policy-bounded and important, but not entitled to full deterministic-surface claims.
- **Experimental / non-DCP** means the surface is outside release-grade deterministic guarantees unless later promoted.

---

## The Six Questions

| Question | Document |
| :--- | :--- |
| What is frozen? | `FROZEN_CORE_PROFILE.md` |
| What is drifting? | `DRIFT_DECOMPOSITION.md` |
| What is risky? | `ACTIVE_RISKS.md` |
| What is blocked / being hardened? | `HARDENING_BACKLOG.md` |
| What is governed non-DCP or experimental? | `EXTENSION_PROFILE.md` |
| What is the review protocol? | `GOVERNANCE_REVIEW_CADENCE.md` |

---

## Full Index

### Dashboard

- `PROJECT_CONTROL_CENTER.md` — phase, health, gates, drift summary, top risks, next decisions, and deterministic-surface classification

### Constitutional

- `FROZEN_CORE_PROFILE.md` — directory boundary, opcode whitelist, DCP guarantees, exclusions, firewall

### Alignment Truth

- `IMPLEMENTATION_MATRIX.md` — spec authority, implementation maturity, promotion state, drift risk, last review date per subsystem

### Risk and Decisions

- `ACTIVE_RISKS.md` — short risk table; refreshed weekly
- `DECISION_LOG.md` — what was decided, by whom, when, alternatives considered

### Drift and Hardening

- `DRIFT_DECOMPOSITION.md` — spec claim → implementation reality → closure plan, per surface
- `HARDENING_BACKLOG.md` — structural hardening and determinism tightening only (no features)
- `DETERMINISM_AUDIT_LOG.md` — chronological: what was audited, what failed, what was patched, what remains open

### Experimental Boundary

- `EXTENSION_PROFILE.md` — governed non-DCP and experimental surfaces; what is not frozen, what can break, what has no DCP guarantee
- `AI_RFC_BACKLOG.md` — prioritized AI RFC execution order, ownership, dependencies, and compliance snapshot

### Governance Protocol

- `GOVERNANCE_REVIEW_CADENCE.md` — monthly review checklist, drift protocol, registry update procedure, promotion gate

### Operational

- `CI_GATE_STATUS.md` — required gates, informational gates, known failures, benchmark guardrail, flaky-test inventory
- `DEPENDENCY_HEALTH.md` — dependency versions, upgrade policy, CVE tracking, determinism impact
- `TASKS.md` — active implementation tasks (C2 close, stdlib fixture suites)

---

## What Is Not Here

These were archived to `docs/records/` because they are temporal, aspirational,
or absorbed into the documents above:

| Archived | Absorbed Into / Location |
| :--- | :--- |
| EXPERIMENTAL_SURFACE_INVENTORY.md | `EXTENSION_PROFILE.md` |
| GOVERNED_AGI_PROMOTION_PIPELINE.md | `EXTENSION_PROFILE.md` + `GOVERNANCE_REVIEW_CADENCE.md` |
| RISK_REGISTER.md | `ACTIVE_RISKS.md` |
| SYSTEM_STATUS.md | active concise subsystem/maturity snapshot; keep aligned with `PROJECT_CONTROL_CENTER.md` |
| STRUCTURAL_INTEGRITY_REPORT.md | `DETERMINISM_AUDIT_LOG.md` |
| VERIFIED_SURFACE_AUDIT.md | `DETERMINISM_AUDIT_LOG.md` |
| DETERMINISM_VERIFICATION_REPORT_LANGUAGE_SURFACE.md | `DETERMINISM_AUDIT_LOG.md` |
| DETERMINISTIC_CORPUS_MANIFEST.md | `FROZEN_CORE_PROFILE.md §7` |
| RELEASE_DISCIPLINE_CHECKLIST.md | `GOVERNANCE_REVIEW_CADENCE.md §1` |
| T81LANG_PROMOTION_GATE.md | `GOVERNANCE_REVIEW_CADENCE.md §5` → `records/status-history/` |
| T81LANG_SURFACE_INVENTORY.md | `DRIFT_DECOMPOSITION.md` |
| T81LANG_ENGINEERING_BACKLOG_2026-03.md | `HARDENING_BACKLOG.md` |
| AXION_STATUS.md | `DRIFT_DECOMPOSITION.md` + `IMPLEMENTATION_MATRIX.md` |
| EXECUTION_PLAN_2026-03.md | (completed; `records/`) |
| RELEASE_READINESS_PACKET_2026-03.md | (time-bound; `records/audits/`) |
| C2_MONTH_CLOSE_* | (`records/status-history/`) |
| FULL_SYSTEM_ARCHITECTURAL_STRATEGIC_AUDIT_2026-03.md | (`records/audits/`) |
| BEHAVIORAL_CONFORMANCE_EXPANSION_PHASE3_*.md | (`records/status-history/`) |
| STDLIB_STABILIZATION_PLAN_2026-03.md | (`records/status-history/`) |
| T81LANG_PROMOTION_GATE_SNAPSHOT.md | (`records/status-history/`) |
| STDLIB_PROMOTION_SNAPSHOT_2026-03.md | (`records/status-history/`) |
| STDLIB_PROMOTION_SNAPSHOT_2026-03-14.md | (`records/status-history/`) — supersedes 2026-03-01 snapshot |
| T81LANG_IMPLEMENTATION_TASK_QUEUE_2026-03.md | (`records/status-history/`) |
| T81LANG_TRACEABILITY_MATRIX.md | placeholder — `records/` |
| AUDIT_REMEDIATION_CROSSWALK.md | placeholder — `records/` |
| GOVERNANCE_BOUNDARIES_CLARIFICATION.md | placeholder — `records/` |
| JIT_EQUIVALENCE_GAP.md | placeholder — `records/` |
| AI_CLI_MILESTONE_EVIDENCE.md | milestone complete — `records/` |
| CI_WORKFLOW_CONFIRMATION.md | lightweight freshness-governed workflow marker; detailed gate state lives in `CI_GATE_STATUS.md` |
| RFC_0026_RESIDUAL_EXCEPTIONS.md | point-in-time analysis — `records/status-history/` |
| RFC_TRIAGE_MATRIX.md | triage complete; live status in `spec/rfcs/index.md` — `records/status-history/` |
