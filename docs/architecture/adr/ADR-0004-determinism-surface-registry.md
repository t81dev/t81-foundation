# ADR-0004: Determinism Surface Registry as Release Gate Artifact

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [ADR-0004: Determinism Surface Registry as Release Gate Artifact](#adr-0004-determinism-surface-registry-as-release-gate-artifact)
  - [Purpose](#purpose)
  - [Scope](#scope)
  - [Context](#context)
  - [Decision](#decision)
  - [Alternatives Considered](#alternatives-considered)
  - [Consequences](#consequences)
  - [References](#references)
  - [Versioning Statement](#versioning-statement)

<!-- T81-TOC:END -->


Status: Accepted
Date: 2026-02-25
Version: 1.0.0

## Purpose

Institutionalize the determinism surface registry as the canonical release gate
artifact for determinism claims.

## Scope

Applies to determinism verification, release readiness, and incident handling.

## Context

Determinism guarantees must be scoped to explicit verified surfaces. Releases
and incident response depend on consistent registry interpretation.

## Decision

Treat `docs/governance/DETERMINISM_SURFACE_REGISTRY.md` as mandatory evidence
for:

1. DCP release claims.
2. Freeze exception evaluation on deterministic surfaces.
3. Post-incident status updates for breached surfaces.

## Alternatives Considered

- Test-only evidence without registry lifecycle: rejected due to missing
  governance state tracking.
- Release claims based on informal summaries: rejected due to audit ambiguity.

## Consequences

- Tight coupling between testing evidence and governance status.
- Requires registry updates when determinism status changes.
- Improves release and postmortem traceability.

## References

- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/governance/DETERMINISM_THREAT_MODEL.md`
- `docs/governance/INCIDENT_RESPONSE.md`
- `docs/product/RELEASE_DISCIPLINE.md`

## Versioning Statement

Registry-governance changes that alter release gate interpretation require a
superseding ADR.
