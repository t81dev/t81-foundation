# CI Workflow Confirmation

## Evidence review — 2026-10-27

Last Updated: 2026-10-27

Purpose: workflow/freshness marker, not certification that downstream assertions ran. [ai-experiments-ci.yml](../../.github/workflows/ai-experiments-ci.yml) exists, uses push/PR path filters and checks freshness before code configuration. Many runtime/evidence steps are conditional on `steps.build_ai.outputs.has_target == 'true'`; skipped steps are not Tested.

The October 21 incident below is Historical; the bounded production repair is merged as recorded in the [consolidation ledger](../research/consolidation/README.md). Main CI state and required aggregate dependencies are in [CI_GATE_STATUS](CI_GATE_STATUS.md). Local research regressions are separately scoped in the [audit report](STATUS_RECONCILIATION_2026-10-27.md). Updating this reviewed marker does not establish every AI lane passing or activate a promotion.

Review basis: audited main `c930b8c2ee7478782bf3c6d231bdd59984595ef2`. This editorial review does not grant promotion or release approval. See the [reconciliation report](STATUS_RECONCILIATION_2026-10-27.md) for executed checks and limitations.

## Retained record

Status: Active
Last Updated: 2026-10-21


## October 21 enforcement and CI audit

The merged PR #537 experiment found that the configured CanonFS text-policy hook
allowed its denied, revoked, and malformed-policy output cases. Historical
implementation/compliance entries below do not establish enforcement at that
boundary. Production hardening and a separately labeled arm D are under test;
no full inventory or system-wide security result is claimed.

AI Experiments CI run [39644888844](https://github.com/t81dev/t81-foundation/actions/runs/39644888844)
failed in the status-document freshness check (three documents aged eight days,
limit seven), before configuration or compilation. This failure is independent
of the opt-in experiment. These status notes record that audit without changing
freshness limits or claiming that subsequent CI stages have passed.


## AI Experiments CI Workflow

This document confirms the status of the AI Experiments CI Workflow.

### Triggers

The workflow is triggered on push and pull requests affecting relevant directories.
