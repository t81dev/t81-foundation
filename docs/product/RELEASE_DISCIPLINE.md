# Release Discipline Manifest

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Release Discipline Manifest](#release-discipline-manifest)
  - [Purpose](#purpose)
  - [Scope](#scope)
  - [Definitions](#definitions)
  - [1. Release Preconditions](#1-release-preconditions)
  - [2. Versioning Rules](#2-versioning-rules)
  - [3. Release Checklist](#3-release-checklist)
  - [3A. Release Boundary Classification Policy](#3a-release-boundary-classification-policy)
  - [4. Non-Required Workflow Failure Handling](#4-non-required-workflow-failure-handling)
  - [Cross-References](#cross-references)
  - [Versioning Statement](#versioning-statement)

<!-- T81-TOC:END -->


Status: Active
Version: 1.0.0
Owner: Product/Governance
Last Updated: 2026-02-25

## Purpose

Define release preconditions, versioning discipline, and release evidence for
tagged versions `vX.Y.Z`.

## Scope

Applies to all releases claiming Deterministic Core Profile (DCP) guarantees and
freeze-boundary compliance.

## Definitions

- DCP: Deterministic Core Profile defined in
  `docs/product/DETERMINISTIC_CORE_PROFILE.md`.
- Freeze exception: Explicit, reviewed boundary exception per governance policy.

## 1. Release Preconditions

Before tagging `vX.Y.Z`, all of the following must hold:

1. Determinism registry verified surfaces are current and accurate.
2. DCP compliance is confirmed for release scope.
3. Structural integrity scripts and required checks pass.
4. No open freeze exceptions.
5. No open determinism breach on verified surfaces.

## 2. Versioning Rules

This manifest is interpreted with:

- `docs/governance/FREEZE_ENFORCEMENT.md`
- `docs/product/DETERMINISTIC_CORE_PROFILE.md`

SemVer mapping:

- MAJOR: Frozen boundary violation or backward-incompatible change.
- MINOR: Backward-compatible feature addition.
- PATCH: Bug fix only; no compatibility or deterministic-contract expansion.

## 3. Release Checklist

- [ ] Registry status reviewed: `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- [ ] DCP scope validated: `docs/product/DETERMINISTIC_CORE_PROFILE.md`
- [ ] Structural integrity status validated:
      `docs/status/STRUCTURAL_INTEGRITY_REPORT.md`
- [ ] Verified surface audit current:
      `docs/status/VERIFIED_SURFACE_AUDIT.md`
- [ ] No open freeze exception labels for target release scope
- [ ] No open determinism incident at Severity 2 or Severity 3
- [ ] Release boundary classification completed for all touched surfaces:
      DCP-certified / governed non-DCP / experimental
- [ ] Release notes include determinism hash summary and experimental delta summary

## 3A. Release Boundary Classification Policy

Every release candidate must classify touched surfaces into exactly one class:

1. DCP-certified:
   - Surface is within DCP and remains Verified in determinism registry.
2. Governed non-DCP:
   - Surface is governed by policy/incident controls but not DCP-certified.
3. Experimental:
   - Surface is explicitly non-DCP and carries no deterministic release
     guarantee claims.

Classification requirements:

1. Classification is recorded in release packet/template.
2. Any AGI-facing surface may not be classified as DCP-certified without
   registry + promotion-gate evidence.
3. If classification is ambiguous, default to Experimental until governance
   approval resolves status.

## 4. Non-Required Workflow Failure Handling

Non-required workflow failures do not automatically block release, but they
must be dispositioned before release decision:

1. Classify failure as one of:
   - release-impacting (blocks GO until fixed), or
   - non-release-impacting (eligible for explicit waiver).
2. For waived failures, document:
   - workflow/job name,
   - commit SHA,
   - rationale for non-release impact,
   - owner and target remediation date.
3. Record classification and waiver evidence in the active release-readiness
   packet.
4. If failure intersects DCP, verified determinism surfaces, freeze boundaries,
   or public API contract, it is release-impacting by default and may not be
   waived without governance escalation.

## Cross-References

- `docs/governance/SPEC_AUTHORITY_MODEL.md`
- `docs/governance/FREEZE_ENFORCEMENT.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/governance/DETERMINISM_THREAT_MODEL.md`
- `docs/governance/INCIDENT_RESPONSE.md`
- `docs/product/DETERMINISTIC_CORE_PROFILE.md`
- `docs/status/GOVERNED_AGI_PROMOTION_PIPELINE.md`

## Versioning Statement

This manifest is governance policy. Any relaxation of release preconditions
requires explicit governance review and ADR traceability.
