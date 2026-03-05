# Approval Delegation Policy

Status: Active
Version: 1.0.0
Owner: Governance
Last Updated: 2026-03-05

## Purpose

Reduce single-owner approval concentration for GO/HOLD and governance-critical
decisions while preserving freeze and determinism controls.

## Scope

This policy applies to:

- Release GO/HOLD decisions
- Deterministic Core Profile (DCP) claim approvals
- Freeze exceptions
- Governance packet sign-off (month-close, release-readiness)

This policy does not weaken required technical controls in:

- `docs/governance/FREEZE_ENFORCEMENT.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/governance/SPEC_AUTHORITY_MODEL.md`

## Roles

- Primary approver: `@t81dev`
- Deputy approver(s): maintainers designated in `CODEOWNERS` and listed in
  release/governance packets.

## Deputy Eligibility

A deputy must satisfy all criteria:

1. Active maintainer in the affected module surface.
2. No unresolved conflict-of-interest on the change.
3. Familiarity with freeze and determinism policy boundaries.
4. Able to provide same-day incident response for reverted or blocked stamps.

## Delegation Rules

1. Deputy approval is allowed when the primary approver is unavailable.
2. Deputy approval is allowed for time-bound gates (release cutoff, month-close)
   when waiting would miss the window.
3. At least one approval on frozen or DCP-sensitive changes must come from either:
   primary approver, or a deputy plus a second maintainer.
4. Freeze exceptions still require two approvals and ADR evidence.
5. A deputy cannot self-approve their own authored GO/HOLD packet.

## Required Evidence for Deputy GO/HOLD

Deputy approval must include:

1. Candidate SHA.
2. Required CI context status (`quality gate / required`, `Analyze (cpp)`).
3. Non-required failures and disposition (block/defer) with rationale.
4. Link to updated decision log entry and release/governance packet.
5. Explicit stamp text: `GO (deputy)` or `HOLD (deputy)`.

## Escalation and Revocation

1. Primary approver may revoke delegation for a specific decision window.
2. Any post-approval evidence conflict triggers automatic HOLD pending re-review.
3. Incidents tied to delegated approvals are reviewed at next governance cadence.

## Audit Trail

Every delegated decision must be logged in:

- `docs/status/DECISION_LOG.md`
- Relevant audit packet under `docs/records/audits/`

## Cross-References

- `docs/governance/MODULE_OWNERSHIP_CHARTER.md`
- `docs/governance/FREEZE_ENFORCEMENT.md`
- `docs/governance/SPEC_AUTHORITY_MODEL.md`
- `docs/status/GOVERNANCE_REVIEW_CADENCE.md`

## Versioning Statement

This is governance policy. Any boundary-loosening change requires explicit
rationale and review in the decision log.
