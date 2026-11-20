# T81Lang Standard Library Change Policy

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81Lang Standard Library Change Policy](#t81lang-standard-library-change-policy)
  - [1. Purpose](#1-purpose)
  - [2. Change Classes](#2-change-classes)
  - [3. Required Evidence By Class](#3-required-evidence-by-class)
  - [4. Mandatory Gates](#4-mandatory-gates)
  - [5. Claim Discipline](#5-claim-discipline)

<!-- T81-TOC:END -->


Date: 2026-02-26  
Status: Active Governance Policy

## 1. Purpose

Define how `std.*` module changes are classified and what evidence is required
before merge/release.

## 2. Change Classes

1. Breaking
   - Removes/renames exported functions or changes observable semantics.
   - Requires semver-major stdlib surface bump record in status artifacts.
2. Non-breaking
   - Adds backward-compatible functions or tightens diagnostics without changing
     prior valid program behavior.
   - Requires semver-minor stdlib surface bump record.
3. Patch
   - Internal fixes with no exported surface change and no intended observable
     behavior drift.
   - Requires semver-patch stdlib surface bump record.
4. Experimental-bound
   - Changes in modules/symbols already marked bounded or experimental in the
     stdlib promotion snapshot.
   - Requires explicit boundary labels and no DCP claim expansion.

## 3. Required Evidence By Class

1. Breaking
   - updated module docs in `docs/standards/standard-library.md`
   - fixture updates for affected `tests/fixtures/t81lang_std_*`
   - relevant `cli_std_*_fixtures_test` updates
   - release note entry and migration note
2. Non-breaking
   - updated module docs
   - fixture/test evidence for new behavior
3. Patch
   - targeted tests proving bug-fix behavior
   - no surface drift per baseline checks
4. Experimental-bound
   - updated `docs/status/STDLIB_PROMOTION_SNAPSHOT_2026-03.md`
   - determinism-boundary statement remains intact

## 4. Mandatory Gates

1. `scripts/governance/check_stdlib_surface_baseline.py`
2. `scripts/governance/check_stdlib_promotion_snapshot.py`
3. Relevant stdlib CLI fixture tests
4. `scripts/ci/run_determinism_slice.sh build`
5. Overclaim guardrails

## 5. Claim Discipline

No stdlib change may broaden determinism claims beyond documented DCP/registry
verified surfaces.

