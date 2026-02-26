# Project Control Center

Status: Active Program Management
Last Updated: 2026-02-26
Owner: Project Management / Governance
Version: 2.1.0

## 1. At-a-Glance

| Item | Status |
| :--- | :--- |
| Sprint Focus | March governance close + governed inference hardening |
| Overall Program Health | Green with one scheduled governance gate pending |
| Release Readiness Decision | HOLD (decision stamp: 2026-02-26) |
| Primary Blocker | Required contexts not yet fully satisfied for selected main candidate (`quality gate / required`, `Analyze (cpp)`) |
| Next Hard Date | C2 month-close execution on 2026-03-31 (UTC) |
| Open Task Count (`docs/roadmaps-plans/TASKS.md`) | 1 (`C2 Month-Close`) |

## 2. Purpose

Provide the control-plane view for delivery status, governance posture, active
risks, and near-term execution priorities.

## 3. Scope

This document is a roll-up, not a submodule ledger. It tracks:

- governance and release controls
- determinism and policy boundaries
- execution status and blockers
- escalation-level risks

Authority remains:
`/spec` > `docs/architecture/OVERVIEW.md` > `/docs` > `/book`.

## 4. Current Snapshot

### 4.1 Governance and Release

- March packet decision is `HOLD` in
  `docs/status/RELEASE_READINESS_PACKET_2026-03.md`.
- Required-context verification rule remains unchanged:
  GO only when both required contexts are `completed` + `success`.
- CodeQL push-trigger remediation for `main` has been applied; candidate
  verification still requires a post-fix main commit outcome.

### 4.2 Delivery Progress

- T81Lang posture is synchronized as:
  - spec authority: Draft (`spec/t81lang-spec.md`)
  - implementation maturity: Beta (`docs/status/T81LANG_PROMOTION_GATE.md`)
- A1 through A1G closure tracks are complete and synchronized.
- Governed llama.cpp integration backlog items are complete.
- Behavioral Conformance Expansion Sprint (2026-02-26) is complete:
  - VM/Axion conformance matrix suites expanded.
  - CanonFS integrity matrix expanded (truncate/append/default-read-verify).
  - Workload determinism tiers expanded (policy-heavy + tensor-access).
  - Translation semantic gate upgraded with required section-heading parity checks.
  - CI benchmark gate expanded with VM workload dispatch/native ratio guardrail.
- Behavioral Conformance Expansion Phase 3 is in progress:
  - deterministic VM trap-family matrix coverage added
  - CanonFS read-verify env-contract matrix coverage added
  - deterministic `TLOADHASH` decode-fault matrix coverage added
  - deterministic `TLOADHASH` status classification matrix expanded (`InvalidHash`/`CanonFsMiss`/`DecodeFault`)
  - mixed workload conformance matrix (policy+tensor+memory+branch) added
  - mixed workload matrix now includes deterministic policy-deny branch-path case
  - Axion conformance matrix now includes clause-ordering equivalence invariants with deterministic Axion-event signatures
  - Axion opcode dispatch concentration reduced in VM via extracted `AxCheck`/`AxReport` helper paths
  - blocked-neural and bitwise opcode-family dispatch concentration reduced via extracted helper paths
  - VM trace/log helper extraction advanced in policy-trace bridge
- Experimental backlog implementation items are complete:
  - Cognitive Tier 1..5 closures
  - Runtime JIT prototype backend uplift
  - CanonFS persistent write-path optimization
  - CLI `t81 trace export` (JSON/CSV)
  - Debugger cognitive tier inspection (`t [1|2|3|4|5|all]`)

### 4.3 C2 Month-Close Readiness

- Final C2 execution remains scheduled for 2026-03-31.
- Runbook exists:
  `docs/status/C2_MONTH_CLOSE_RUNBOOK_2026-03-31.md`.
- Consolidated execution helper exists:
  `scripts/governance/c2_month_close_check.py`.
- Latest prep report:
  `docs/status/C2_MONTH_CLOSE_CHECK_2026-03-31.md` (latest run: PASS).

## 5. Active Workstreams

1. C2 month-close finalization
   Date: 2026-03-31
   Exit criteria: final checklist re-run and final stamp in
   `docs/records/audits/2026-03-governance-review.md`.
2. Release candidate required-context closure
   Exit criteria: required contexts satisfied on selected candidate SHA and
   packet decision re-evaluated.
3. Governance continuity maintenance
   Exit criteria: hygiene/link integrity and artifact synchronization sustained
   through close window.

## 6. Risks

### 6.1 High

- Required-context mismatch risk can keep March packet in HOLD.
- Determinism overclaim risk if external summaries omit registry boundaries.

### 6.2 Medium

- Single-owner concentration for governance/release approvals.
- AGI-facing surface growth outpacing promotion evidence updates.

### 6.3 Monitoring

- Experimental scope creep without explicit promotion/retirement actions.
- Benchmark variability and environment-sensitive signal noise.

## 7. Next Decisions

1. 2026-03-31: Execute C2 runbook and stamp final outcome fields.
2. Post-required-context pass: Re-evaluate GO/HOLD in the March packet.

## 8. Governance Directives

1. Spec-first policy remains mandatory for normative behavior changes.
2. Freeze boundaries remain unchanged.
3. Determinism claims remain limited to Verified registry surfaces.
4. Public API contract remains `include/t81/**` only.
5. Root-level documentation additions require explicit governance approval.

## 9. Cross-References

- `docs/roadmaps-plans/TASKS.md`
- `docs/status/BEHAVIORAL_CONFORMANCE_EXPANSION_SPRINT_2026-02-26.md`
- `docs/status/BEHAVIORAL_CONFORMANCE_EXPANSION_PHASE3_2026-02-26.md`
- `docs/status/RELEASE_READINESS_PACKET_2026-03.md`
- `docs/status/C2_MONTH_CLOSE_RUNBOOK_2026-03-31.md`
- `docs/status/C2_MONTH_CLOSE_CHECK_2026-03-31.md`
- `docs/status/COGNITIVE_TIERS_SPEC_COMPLIANCE_2026-02-26.md`
- `docs/records/audits/2026-03-governance-review.md`
- `docs/status/EXECUTION_PLAN_2026-03.md`
- `docs/status/IMPLEMENTATION_MATRIX.md`

## 10. Versioning Statement

This control document is operational governance. Updates must preserve
authority hierarchy and may not weaken freeze or determinism boundaries.
