# ADR-0003: Public API Boundary at `include/t81/**`

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [ADR-0003: Public API Boundary at `include/t81/**`](#adr-0003-public-api-boundary-at-`includet81**`)
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

Fix the stable public API boundary to a single auditable include surface.

## Scope

Applies to source-compatibility and review policy for C++ public API changes.

## Context

Without an explicit API boundary, internal headers can be treated as stable by
mistake, increasing compatibility and freeze risk.

## Decision

Define public API as headers under `include/t81/**` only. Any signature or
layout change affecting this boundary is freeze-sensitive and requires
governance review discipline.

## Alternatives Considered

- Mixed boundary across `src/` and `include/`: rejected due to ambiguity.
- No explicit API boundary: rejected due to compatibility drift risk.

## Consequences

- Clarifies external integration contract.
- Requires stricter review on `include/t81/**` changes.
- Simplifies compatibility and release classification.

## References

- `docs/governance/FREEZE_ENFORCEMENT.md`
- `docs/governance/SPEC_AUTHORITY_MODEL.md`
- `docs/governance/MODULE_OWNERSHIP_CHARTER.md`

## Versioning Statement

Any redefinition of the public API boundary requires a superseding ADR and
explicit migration guidance.
