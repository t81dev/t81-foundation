# Strategic Direction

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Strategic Direction](#strategic-direction)
  - [Purpose](#purpose)
  - [Scope](#scope)
  - [Definitions](#definitions)
  - [Direction Statement](#direction-statement)
  - [Boundary Conditions](#boundary-conditions)
  - [Cross-References](#cross-references)
  - [Versioning Statement](#versioning-statement)

<!-- T81-TOC:END -->


Status: Active
Owner: Product/Governance
Last Updated: 2026-02-25

## Purpose

Define the product-level strategic direction for T81 as a deterministic,
ternary-based architecture suitable for governed AGI workloads.

## Scope

This document frames product direction and boundary intent. It does not expand
determinism guarantees, redefine freeze policy, or override normative `/spec`
content.

## Definitions

- Governed AGI workloads: Higher-level cognitive or autonomous systems that are
  required to operate within explicit policy, auditability, and safety-control
  boundaries.
- Deterministic substrate: Verified execution surfaces with reproducibility
  guarantees constrained by the determinism registry and DCP.

## Direction Statement

T81 is oriented toward a deterministic ternary-based architecture that provides
a governance-first substrate for advanced intelligent systems.

This direction means:

1. Core execution and representation surfaces prioritize reproducibility over
   opportunistic nondeterministic behavior.
2. Governance controls (authority model, freeze enforcement, incident response,
   release discipline) are treated as product requirements, not optional process
   overlays.
3. AGI-oriented or cognitive-tier capabilities are developed as bounded layers
   above the deterministic core, with explicit promotion criteria before any
   guarantee expansion.

## Boundary Conditions

1. Determinism claims are limited to surfaces marked **Verified** in
   `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`.
2. DCP defines the release-certified deterministic subset; experimental modules
   are out of scope unless promoted.
3. Freeze boundaries remain unchanged unless approved through governance and ADR
   process.
4. This strategy does not imply universal determinism for all repository
   surfaces or all AGI behaviors.

## Cross-References

- `docs/governance/SPEC_AUTHORITY_MODEL.md`
- `docs/governance/FREEZE_ENFORCEMENT.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/governance/DETERMINISM_THREAT_MODEL.md`
- `docs/product/DETERMINISTIC_CORE_PROFILE.md`
- `docs/product/RELEASE_DISCIPLINE.md`
- `docs/governance/INCIDENT_RESPONSE.md`

## Versioning Statement

Strategic guidance only. Authority hierarchy remains:
/spec > docs/architecture/OVERVIEW.md > /docs > /book.
