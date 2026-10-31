# Monthly Governance Review Checklist

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Monthly Governance Review Checklist](#monthly-governance-review-checklist)
  - [Purpose](#purpose)
  - [Scope](#scope)
  - [Review Inputs](#review-inputs)
  - [Checklist](#checklist)
  - [Output Artifact](#output-artifact)
  - [Versioning Statement](#versioning-statement)

<!-- T81-TOC:END -->


Status: Active
Version: 1.0.0
Owner: Governance / Project Management
Cadence: Monthly
Last Updated: 2026-02-25

## Purpose

Define a repeatable monthly governance checkpoint to keep authority, freeze,
determinism, and release controls synchronized.

## Scope

Applies to governance, status, and release-control artifacts in `docs/`.

## Review Inputs

- `docs/governance/SPEC_AUTHORITY_MODEL.md`
- `docs/governance/FREEZE_ENFORCEMENT.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/governance/DETERMINISM_THREAT_MODEL.md`
- `docs/governance/INCIDENT_RESPONSE.md`
- `docs/product/DETERMINISTIC_CORE_PROFILE.md`
- `docs/product/RELEASE_DISCIPLINE.md`
- `docs/status/PROJECT_CONTROL_CENTER.md`
- `docs/status/IMPLEMENTATION_MATRIX.md`
- `docs/status/DRIFT_DECOMPOSITION.md`
- `docs/records/audits/DETERMINISTIC_CORPUS_MANIFEST.md`

## Checklist

1. Authority and freeze controls
   - [ ] No conflicting authority statements across README/docs/spec references
   - [ ] No freeze-boundary relaxations without ADR and explicit governance review

2. ADR and architecture governance
   - [ ] New boundary-impacting decisions recorded in `docs/architecture/adr/`
   - [ ] ADR status transitions are linked and internally consistent

3. Determinism governance
   - [ ] Verified-surface statuses in registry are current
   - [ ] Threat model reflects new determinism surfaces or risk changes
   - [ ] Incident log/actions (if any) are closed or tracked with owners

4. Release discipline
   - [ ] Release template and release-discipline manifest remain aligned
   - [ ] No unresolved Severity 2/3 determinism incidents for active release line

5. Status and planning
   - [ ] Project Control Center updated for current cycle
   - [ ] System Status and Implementation Matrix are mutually consistent
   - [ ] High-drift subsystems have owner and target date tracked in planning artifacts

6. Documentation hygiene
   - [ ] Root-level documentation remains limited to approved entrypoint files
   - [ ] New audit/report artifacts are placed under `docs/records/`

## Output Artifact

Create or update a monthly review note in:

- `docs/records/audits/YYYY-MM-governance-review.md`

Minimum content:

- review date
- reviewer(s)
- checklist exceptions
- remediation actions with owners and due dates

## Versioning Statement

This checklist is governance process policy. Any reduction in control scope
requires explicit review and recorded justification.
