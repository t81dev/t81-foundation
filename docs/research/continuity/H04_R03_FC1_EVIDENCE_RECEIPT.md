# T81 H04-R03-FC1 Evidence Receipt — Three-Finding Correctness Closure

**Document:** `docs/research/continuity/H04_R03_FC1_EVIDENCE_RECEIPT.md`
**Date:** 2026-11-07
**Task Anchor:** https://github.com/t81dev/t81-foundation/issues/723
**Context:** https://github.com/t81dev/t81-foundation/pull/722
**Base Commit SHA:** `1103356ac867a88b3c3873a7e2e696d48378f424`
**Parent Study:** EXP-H04-R03 Candidate A Matched Codec
**Overall Milestone Disposition:** **FC1 PARTIAL**
**Phase C Authorization Status:** **STRICTLY FROZEN / UNAUTHORIZED**

---

## 1. Executive Summary & Review Findings Resolution

This evidence receipt documents the corrective implementation resolving three Codex review findings reported on PR #679 in study EXP-H04-R03.

### Per-Finding Dispositions

| Finding | Severity | Description | Final Disposition | Summary of Evidence |
|---|---|---|---|---|
| **Finding 1** | P1 | Fallback Confidence Intervals & Statistical Integrity | **FIXED** | Removed all five `(1.0, 1.0, 1.0)` fallback confidence intervals. Preserved Step 1 protocol integrity and Step 2 CV > 5.0% early noise stop. Prior to Step 3/4 evaluation, missing, incomplete, nonfinite, nonpositive, or inverted required CI inputs fail closed as `INVALID`. Deterministic negative tests added in `test_harness.py` (10/10 PASS). |
| **Finding 2** | P2 | Alignment-Padding Integrity & Access Contract | **FIXED** | Verified both Codex counterexamples (`k=5, A=8` byte 10; `k=5, A=64` byte 8). Analyzed exact access-accounting trade-offs against frozen §4.5/4.6 byte budgets. Implemented explicit `validate_frame()` for immutable-frame boundary validation in $O(B)$ space. Deterministic negative regressions added in `test_b729_u5.py` and `test_matched_codec.py` (28/28 PASS). |
| **Finding 3** | P1 | Phase 4 Retention Verification Reporting | **REPORTING CORRECTED — FUNCTIONAL REQUIREMENT OPEN** | Withdrew unsupported claims that Phase 4 wire-frame-only retention was verified. Updated dry-run validation to report Phase 4 as `UNVERIFIED / UNIMPLEMENTED` and status as `PARTIAL — PHASE 4 UNVERIFIED / UNIMPLEMENTED`. Removed `self.u_symbols` leak. Functional retention requirement remains open. |

---

## 2. Before-and-After Reproductions

### 2.1 Finding 1 — Statistical Decision Fallbacks

#### Pre-Fix Behavior
In `experiments/h04_r03_matched_codec/benchmark_harness.py`:
`evaluate_decision_rules` fetched confidence intervals with `.get(..., (1.0, 1.0, 1.0))`. If required arms (e.g. `B10`, `B729`, `B729-u5`) were missing or partial, fallback intervals shaped like valid observations could produce `REJECT` or `ACCEPT` instead of failing closed as `INVALID`.

#### Post-Fix Verification
- Removed all default tuples.
- Enforced complete dictionary key containment and interval validation for required arms:
  `ci_life` must contain valid 3-tuples for `("B10", "B729", "B729-u5")`.
  `ci_query` must contain valid 3-tuples for `("B10", "B729-u5")`.
- If system CV > 5.0%, Step 2 legitimately returns `INCONCLUSIVE` due to excessive noise without requiring CIs to be calculated.
- If system CV $\le$ 5.0%, any missing, partial, nonfinite, nonpositive, or inverted CI returns `INVALID`.
- Validated via `test_09_decision_rule_missing_or_partial_ci_rejected_as_invalid` and `test_10_high_noise_early_stop_without_calculating_ci` in `test_harness.py`.

---

### 2.2 Finding 2 — Alignment Padding & Access Accounting

#### Pre-Fix Counterexamples
1. Counterexample 1: `k=5, A=8`, encode `[0]` with `B729-u5`, corrupt byte 10 (block 0 alignment padding). `decode()` rejected with `CodecError`, but `random_access(..., 0)` returned `0` without error.
2. Counterexample 2: `k=5, A=64`, encode `[0]` with `B729-u5`, corrupt byte 8 (header alignment gap $D - H$). `decode()` rejected with `CodecError`, but `random_access(..., 0)` returned `0` without error.

#### Access-Accounting Analysis Against Frozen §4.5/4.6 Contract

| Operation / Access Component | Bytes Touched in B729-µ5 | Bytes Touched in B10 | Bytes Touched in T243 | Frozen §4.5/4.6 Constraint | Conflict Status |
|---|---|---|---|---|---|
| Header $N$ (element count) | 8 bytes (`blob[:8]`) | 8 bytes | 8 bytes | Mandatory $O(1)$ frame inspection | **NO CONFLICT** |
| Index entry (RA-INDEXED only) | 8 bytes (`blob[8+8j:16+8j]`) | 8 bytes | 8 bytes | Permitted in RA-INDEXED only | **NO CONFLICT** |
| Target microblock payload | $\le 6$ contiguous bytes | $\le 3$ contiguous bytes | $\le 2$ contiguous bytes | Strictly $\le 6$ bytes (µ5), $\le 3$ (B10), $\le 2$ (T243) | **NO CONFLICT** |
| Other microblocks payload | 0 bytes | 0 bytes | 0 bytes | Zero bytes from other microblocks in same block | **NO CONFLICT** |
| Header alignment gap ($D - H$) | $D - H$ bytes (e.g. 56 B at $A=64$) | $D - H$ bytes | $D - H$ bytes | If scanned per query, violates test_16/test_05 access limits | **CONFLICT IF PER-QUERY** |
| Target-block alignment padding | $\text{aligned}_j - \text{raw}_j$ (up to $A-1$ B) | $\text{aligned}_j - \text{raw}_j$ | $\text{aligned}_j - \text{raw}_j$ | If scanned per query, violates test_16/test_05 access limits | **CONFLICT IF PER-QUERY** |
| All blocks alignment padding | $\sum (\text{aligned}_j - \text{raw}_j)$ ($O(B)$ B) | $O(B)$ bytes | $O(B)$ bytes | Violates $O(1)$ random-access operation count | **CONFLICT IF PER-QUERY** |

#### Resolution & Immutable-Frame Boundary
Per Section 6.6 of `SPECIFICATION.md`:
> *"Validate complete inputs and canonical framing before admitted queries. Full validation work is reported separately and included in the end-to-end cost; a previously validated immutable buffer may amortize validation identically for all arms. Do not scan/validate the whole file silently inside one arm's lookup while giving others prevalidated input."*

Rather than violating the six-payload-byte random-access guarantee by scanning whole alignment gaps and block padding on every random lookup, we introduced the explicit immutable-frame validation boundary:
- Added `validate_frame(blob, arm, k, A, access_mode)` in `experiments/h04_r03_matched_codec/codec.py`.
- `validate_frame()` checks total length, header alignment gaps, canonical block index offsets (if RA-INDEXED), and all block alignment padding in $O(B)$ time before queries begin.
- Both counterexamples are verified in `test_10` and `test_11` in `test_b729_u5.py` and `test_13` in `test_matched_codec.py`, ensuring fail-closed rejection.

---

### 2.3 Finding 3 — Phase 4 Retention Verification Reporting

#### Pre-Fix Behavior
`BenchmarkHarness.run_dry_run_validation()` retained `self.u_symbols` and used `symbols` in subsequent phases while emitting `"dry_run_status": "PASSED"` and listing `"Phase 4: Retention Boundary"` as verified.

#### Post-Fix Correction
- Removed `self.u_symbols` instance caching.
- Explicitly documented that physical wire-frame-only retention and symbol erasure are not enforced in the dry-run harness.
- Updated Phase 4 status to `"Phase 4: Retention Boundary (UNVERIFIED / UNIMPLEMENTED)"`.
- Updated overall dry run status to `"PARTIAL — PHASE 4 UNVERIFIED / UNIMPLEMENTED"`.
- Updated `test_01_dry_run_validation` in `test_harness.py`.
- Maintained per directive that this is a **reporting correction, not functional closure**. The functional requirement remains OPEN.

---

## 3. Test Execution Verification

All relevant test suites were executed independently:

```bash
# 1. Phase B baseline test suite (17 tests)
python3 -m unittest -v experiments.h04_r03_matched_codec.test_matched_codec
# Result: Ran 17 tests - OK

# 2. Candidate A B729-µ5 correctness test suite (11 tests, +2 regression tests)
python3 -m unittest -v experiments.h04_r03_matched_codec.test_b729_u5
# Result: Ran 11 tests - OK

# 3. Deterministic PRNG & workload generators (4 tests)
python3 -m unittest -v experiments.h04_r03_matched_codec.test_workload
# Result: Ran 4 tests - OK

# 4. Harness & statistical decision rules (10 tests, +2 regression tests)
python3 -m unittest -v experiments.h04_r03_matched_codec.test_harness
# Result: Ran 10 tests - OK

# 5. Measurement engine mock & lifecycle suite (15 tests)
python3 -m unittest -v experiments.h04_r03_matched_codec.test_measurement_engine
# Result: Ran 15 tests - OK

# 6. Production process, enforcement & retention infrastructure (15 tests)
python3 -m unittest -v \
  experiments.h04_r03_matched_codec.test_production_infrastructure.EnforcementInfrastructureTests \
  experiments.h04_r03_matched_codec.test_production_infrastructure.ProcessInfrastructureTests \
  experiments.h04_r03_matched_codec.test_production_infrastructure.RetentionInfrastructureTests
# Result: Ran 15 tests - OK

# 7. Verification of execution freeze enforcement
python3 -m experiments.h04_r03_matched_codec.benchmark_harness --run-measurement
# Output: ERROR: Phase C performance measurement is FROZEN. Exit code: 1
```

Total tests executed and verified: **72 passing tests across all active suites with 0 regressions**.

---

## 4. Frozen-Protocol Compatibility Analysis

1. **Protocol Integrity Precedence (§9.1):**
   Step 1 (Protocol Integrity) and Step 2 (CV Noise Stop) are strictly preserved. Missing required CI inputs now map to `INVALID` instead of substituting fabricated values.
2. **Noise Threshold (§8.3, §9.1):**
   The CV > 5.0% threshold is checked before CI requirements, preserving the early inconclusive stop without generating unnecessary CIs.
3. **Random-Access Payload Limits (§4.5):**
   The $\le 6$ payload byte read guarantee for B729-µ5 and 0 touched bytes from other microblocks are preserved.
4. **Execution Freeze:**
   `--run-measurement` remains frozen and guarded. No performance or hardware advantage claims are made.
