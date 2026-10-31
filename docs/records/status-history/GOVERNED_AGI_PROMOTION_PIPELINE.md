# Governed AGI Promotion Pipeline

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Governed AGI Promotion Pipeline](#governed-agi-promotion-pipeline)
  - [Purpose](#purpose)
  - [Scope](#scope)
  - [Promotion States](#promotion-states)
  - [Mandatory Promotion Gates](#mandatory-promotion-gates)
  - [Rejection / Rollback Conditions](#rejection--rollback-conditions)
  - [Cross-References](#cross-references)
  - [Versioning Statement](#versioning-statement)

<!-- T81-TOC:END -->


Status: Active
Owner: Product/Governance
Last Updated: 2026-02-25

## Purpose

Define the promotion lifecycle for AGI-oriented surfaces while preserving
determinism boundaries and freeze controls.

## Scope

Applies to AGI-facing and cognitive-tier surfaces outside DCP-certified core
surfaces, including:

- `experimental/*`
- `runtime/tracing` and AGI-policy-adjacent telemetry surfaces
- `kernel/axion` policy-integration paths

## Promotion States

1. Planned
2. Experimental
3. Verified Candidate
4. Verified

State interpretation:

- Planned: design intent only, no guarantee claims.
- Experimental: implementation active; out of DCP guarantees.
- Verified Candidate: deterministic and governance evidence assembled, pending
  final approval.
- Verified: explicitly listed as Verified in determinism registry and eligible
  for scoped guarantees.

## Mandatory Promotion Gates

Promotion from Experimental to Verified Candidate requires all gates:

1. ADR recorded for boundary-impacting design decisions.
2. Determinism threat-model update for the target surface.
3. Determinism registry entry updated to reflect candidate status and scope.
4. Deterministic test evidence:
   - cross-arch reproducibility checks
   - targeted regression coverage for policy and safety paths
5. Incident-response readiness:
   - detection signal(s) documented
   - rollback/remediation owner assigned
6. Release-readiness packet includes explicit boundary classification:
   - DCP-certified
   - governed non-DCP
   - experimental

Promotion from Verified Candidate to Verified additionally requires:

1. Governance approval recorded in monthly review artifact.
2. No open Severity 2/3 incident for target surface.
3. Release decision packet stamped `GO` with required contexts passing.

## Rejection / Rollback Conditions

Any of the following forces rollback to Experimental:

1. Verified-surface determinism regression.
2. Missing or stale registry/threat-model linkage.
3. Incident-response controls not operationally testable.

## Cross-References

- `docs/product/STRATEGIC_DIRECTION.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/governance/DETERMINISM_THREAT_MODEL.md`
- `docs/governance/INCIDENT_RESPONSE.md`
- `docs/product/RELEASE_DISCIPLINE.md`
- `docs/architecture/adr/`

## Versioning Statement

This pipeline governs promotion process only. It does not expand guarantees
beyond `/spec`, determinism registry status, or DCP boundaries.
