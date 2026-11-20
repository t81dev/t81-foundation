# Determinism & Security Incident Response Plan

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Determinism & Security Incident Response Plan](#determinism-&-security-incident-response-plan)
  - [Purpose](#purpose)
  - [Scope](#scope)
  - [Definitions](#definitions)
  - [1. Determinism Breach Definition](#1-determinism-breach-definition)
  - [2. Immediate Actions](#2-immediate-actions)
  - [3. Disclosure Policy](#3-disclosure-policy)
  - [3A. Governed AGI Incident Triggers](#3a-governed-agi-incident-triggers)
  - [4. Postmortem Template](#4-postmortem-template)
    - [Incident ID](#incident-id)
    - [Summary](#summary)
    - [Timeline](#timeline)
    - [Root Cause](#root-cause)
    - [Corrective Actions](#corrective-actions)
  - [Cross-References](#cross-references)
  - [Versioning Statement](#versioning-statement)

<!-- T81-TOC:END -->


Status: Active
Version: 1.0.0
Owner: Governance/Security
Last Updated: 2026-02-25

## Purpose

Define incident classification and response protocol for determinism and
freeze-boundary breaches.

## Scope

Applies to determinism, freeze, and governance-impacting incidents across
verified and frozen surfaces.

## Definitions

- Determinism breach: Any confirmed mismatch on a verified surface where equal
  input/configuration does not produce bit-identical output across supported
  architectures.
- Structural incident: Breach requiring architecture/governance boundary update.

## 1. Determinism Breach Definition

Determinism and verification scope are defined by:

- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/governance/DETERMINISM_THREAT_MODEL.md`

Severity levels:

- Severity 0: Cosmetic issue with no verified-surface impact.
- Severity 1: Spec drift or documentation/process mismatch without confirmed
  verified-surface regression.
- Severity 2: Determinism regression on a verified surface.
- Severity 3: Freeze violation affecting frozen boundary semantics or public API
  compatibility guarantees.

## 2. Immediate Actions

For Severity 2 and Severity 3 incidents:

1. Freeze `main` merges for impacted scope until containment is validated.
2. Create hotfix branch for incident remediation.
3. Open ADR when structural boundary or policy interpretation changes.
4. Publish advisory note with scope, impact, and temporary mitigations.

## 3. Disclosure Policy

1. Public transparency window:
   - Initial acknowledgement within 48 hours of confirmed Severity 2+ incident.
2. Patch publication timeline:
   - Target patch publication within 7 calendar days for Severity 2.
   - Immediate stabilization and release planning for Severity 3 under freeze
     governance.
3. Registry and governance updates:
   - Update `docs/governance/DETERMINISM_SURFACE_REGISTRY.md` status where
     affected.
   - Update relevant release status and audit docs before closure.

## 3A. Governed AGI Incident Triggers

Treat as Severity 2 minimum when any of the following is confirmed on an
AGI-facing path:

1. Policy bypass: execution path bypasses required Axion policy checks.
2. Boundary misrepresentation: release or docs claim deterministic guarantees
   for non-verified AGI-oriented surfaces.
3. Audit failure: trace metadata is insufficient to reconstruct safety-relevant
   AGI behavior for a reported incident.

Additional actions for AGI-triggered Severity 2+ incidents:

1. Reclassify affected surface in release notes as experimental/non-DCP until
   revalidation completes.
2. Re-run governed AGI promotion gates before restoring prior status.
3. Record remediation linkage to
   `docs/status/GOVERNED_AGI_PROMOTION_PIPELINE.md`.

## 4. Postmortem Template

Use the following template for every Severity 2+ incident.

### Incident ID

`INC-YYYYMMDD-<slug>`

### Summary

- Detection time (UTC):
- Severity:
- Affected surfaces:
- User/release impact:

### Timeline

- Detection:
- Containment:
- Root-cause confirmation:
- Remediation:
- Verification:
- Closure:

### Root Cause

- Primary fault:
- Contributing factors:
- Why existing controls did not prevent this:

### Corrective Actions

- Code/process changes:
- Added regression coverage:
- Registry/governance updates:
- Follow-up owners and due dates:

## Cross-References

- `docs/governance/FREEZE_ENFORCEMENT.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/governance/DETERMINISM_THREAT_MODEL.md`
- `docs/product/RELEASE_DISCIPLINE.md`
- `docs/status/GOVERNED_AGI_PROMOTION_PIPELINE.md`

## Versioning Statement

This response plan is versioned governance policy. Changes to severity handling
or disclosure obligations require explicit review and ADR traceability.
