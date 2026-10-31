# Governance Automation Roadmap

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Governance Automation Roadmap](#governance-automation-roadmap)
  - [1. Automated Surface Audit](#1-automated-surface-audit)
  - [2. Drift Scanner CI Integration](#2-drift-scanner-ci-integration)
  - [3. Determinism Regression Auto-Block](#3-determinism-regression-auto-block)
  - [4. Cross-Platform Hash Diff Automation](#4-cross-platform-hash-diff-automation)
  - [5. Formal Verification Integration](#5-formal-verification-integration)

<!-- T81-TOC:END -->


**Status:** **Planned**
**Objective:** Automate governance compliance to eliminate human error.

This document outlines the roadmap for elevating T81 governance from "Institutional Grade" (manual/scripted audit) to "Elite Tier" (fully automated enforcement).

## 1. Automated Surface Audit

**Goal:** Generate `docs/status/VERIFIED_SURFACE_AUDIT.md` directly from metadata in spec and code files.

**Plan:**
1.  Add `// @audit:verified` annotations to C++ code.
2.  Add `<!-- @audit:spec -->` annotations to Markdown specs.
3.  Write `scripts/governance/generate_audit_matrix.py` to parse these tags.
4.  Fail CI if generated matrix differs from committed file.

## 2. Drift Scanner CI Integration

**Goal:** Block PRs that introduce spec drift.

**Plan:**
1.  Refine `scripts/governance/spec_impl_drift_check.py` to support ignore-lists (`.t81ignore`).
2.  Add `drift-check` job to `.github/workflows/ci.yml`.
3.  Set enforcement level to **Hard-Fail** for `spec/` modifications.

## 3. Determinism Regression Auto-Block

**Goal:** Prevent nondeterministic code from merging.

**Plan:**
1.  Expand `scripts/ci/repro-ledger.py` to run on every PR.
2.  Store "Golden Hashes" of execution traces in `tests/golden/`.
3.  Fail CI if any hash mismatch occurs on *any* supported platform (x86/ARM).

## 4. Cross-Platform Hash Diff Automation

**Goal:** Visualize *where* divergence occurs.

**Plan:**
1.  Instrument VM to dump state hashes every N instructions.
2.  Run identical inputs on x86 and ARM CI runners.
3.  Upload trace logs as artifacts.
4.  Write `scripts/debug/diff_traces.py` to pinpoint the first diverging instruction.

## 5. Formal Verification Integration

**Goal:** Prove correctness mathematically.

**Plan:**
1.  Integrate K-Framework or similar for TISC semantics.
2.  Verify C++ implementation against formal semantics.
