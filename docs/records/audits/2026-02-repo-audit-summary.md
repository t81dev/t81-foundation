# Repository Audit Summary (2026-02)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Repository Audit Summary (2026-02)](#repository-audit-summary-2026-02)
  - [Scope and Method](#scope-and-method)
  - [Executive Readout](#executive-readout)
  - [Key Findings](#key-findings)
    - [1. Strong Core Surfaces](#1-strong-core-surfaces)
    - [2. Redundancy Candidates](#2-redundancy-candidates)
    - [3. CI Action Pinning Posture](#3-ci-action-pinning-posture)
    - [4. CI Permissions Posture](#4-ci-permissions-posture)
    - [5. Documentation Surfaces to Keep Tight](#5-documentation-surfaces-to-keep-tight)
  - [Prioritized Actions](#prioritized-actions)
  - [Validation Snapshot](#validation-snapshot)
  - [Next Recommended Audit Pass](#next-recommended-audit-pass)

<!-- T81-TOC:END -->


This report summarizes the current full-repository audit artifacts:

- `docs/records/inventories/repo_tree.txt`
- `docs/records/inventories/repo_tree_expanded.txt`
- `docs/records/inventories/repo_inventory.tsv`
- `2026-02-workflow-action-audit.md`
- `2026-02-workflow-permissions-audit.md`

The intent is to keep an auditable snapshot of repository composition and convert it into concrete housekeeping actions.

## Scope and Method

- Enumerated all tracked repository paths into tree snapshots.
- Classified files by category, relevance, and essentiality in `docs/records/inventories/repo_inventory.tsv`.
- Cross-checked architecture/build documentation against active CMake targets via:
  - `scripts/ci/check_architecture_targets.py`

## Executive Readout

- Overall repository health: **9.2/10**
- Build and CI posture: **strong**
- Determinism/repro gate posture: **strong**
- Documentation consistency: **good**, with a few follow-up cleanup items
- Redundancy/legacy drift: **low**

## Key Findings

### 1. Strong Core Surfaces

- Build graph is CMake-authoritative and aligned with architecture documentation.
- CI workflows cover core test, reproducibility, runtime contract, and static/security lanes.
- Deterministic validation scripts are present and wired into automation.

### 2. Redundancy Candidates

- `ANALYSIS.md.archived` has been retired from the repository.
- Multiple generated/build directories exist in working environments (`build*` variants), which is expected locally but should stay excluded from source control.

### 3. CI Action Pinning Posture

- Latest workflow audit result: `total=49`, `pinned_sha=49`, `tagged=0`, `unknown=0`.
- Hardening progress: all workflow `uses:` references are pinned to immutable SHAs/digests, including the Marp container image.
- Ongoing requirement: keep Dependabot-driven SHA refreshes enabled and re-run the audit after workflow edits.

### 4. CI Permissions Posture

- Latest workflow permissions audit: `total=11`, `explicit=11`, `missing=0`, `write_scoped=7`.
- Hardening progress: restrictive `permissions` blocks are now explicit across all workflows, including read-only scopes on validation-only pipelines.
- Ongoing requirement: keep write scopes constrained to mutation/release workflows only.

### 5. Documentation Surfaces to Keep Tight

- `ARCHITECTURE.md` now includes drift controls and near-term workstreams.
- `TASKS.md` and `TODO.md` remain the canonical open-work trackers and should be kept synchronized with architecture changes.

## Prioritized Actions

1. **P0: Preserve audit trail in-repo**
   - Keep this `docs/records/audits/` folder versioned as the periodic audit snapshot.
   - Refresh snapshots on meaningful repo topology changes.

2. **P1: Keep documentation ownership crisp**
   - Keep active ownership clear for `ARCHITECTURE.md`, `TASKS.md`, and `TODO.md`.

3. **P1: Keep architecture sync gate required**
   - Retain `scripts/ci/check_architecture_targets.py` in CI-required checks to prevent doc/build drift.

4. **P2: Add periodic audit cadence**
   - Recommended cadence: monthly or milestone-based snapshot refresh.
   - Suggested naming convention for future summaries:
     - `docs/records/audits/YYYY-MM-repo-audit-summary.md`

## Validation Snapshot

At the time of this report:

- `scripts/ci/check_architecture_targets.py` passes.
- Single-threaded required ritual passes:
  - `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release`
  - `cmake --build build --parallel 1`
  - `ctest --test-dir build --output-on-failure -j1`

## Next Recommended Audit Pass

Focus the next pass on modernization and risk reduction:

- Dependency/API currency review across Python/Node/CMake tooling.
- Security posture review of CI permissions and least-privilege job scopes.
- Generated artifact policy review (what should remain versioned vs regenerated on demand).
