# T81 H04-R03-FC1 Evidence Receipt — Three-Finding Correctness Closure & Phase 4 Wire-Frame-Only Retention Enforcement

**Document:** `docs/research/continuity/H04_R03_FC1_EVIDENCE_RECEIPT.md`
**Date:** 2026-11-07
**Task Anchor:** https://github.com/t81dev/t81-foundation/issues/723
**Context:** PR #724 (merged baseline `50087972e078a09d7aff79cf6d8ae15db45c17ba`) and PR #725
**Base Commit SHA:** `50087972e078a09d7aff79cf6d8ae15db45c17ba`
**Parent Study:** EXP-H04-R03 Candidate A Matched Codec
**Overall Milestone Disposition:** **VERIFIED (CORRECTNESS & RETENTION BOUNDARY ENFORCED)**
**Phase C Authorization Status:** **STRICTLY FROZEN / UNAUTHORIZED**
**Verification Label:** **AUTHOR-REPORTED (EXECUTED IN-ENVIRONMENT BY JULES; NOT INDEPENDENT THIRD-PARTY REPRODUCTION)**

---

## 1. Executive Summary & Review Findings Resolution

This evidence receipt documents the corrective implementation resolving three Codex review findings reported on PR #679 in study EXP-H04-R03, reconciled with PR #724's merged baseline and finalizing Phase 4 wire-frame-only retention enforcement.

### Per-Finding Dispositions

| Finding | Severity | Description | Final Disposition | Summary of Evidence |
|---|---|---|---|---|
| **Finding 1** | P1 | Fallback Confidence Intervals & Statistical Integrity | **FIXED (DEFENSIVE HARDENING)** | Merged in PR #724 (commit `eb6d543`). Removed all five `(1.0, 1.0, 1.0)` fallback confidence intervals in `benchmark_harness.py`. Clarified that required CI validation returns `INVALID` for missing mandatory intervals. Hardened CI validation with `_is_valid_ci()` so non-tuple, non-numeric, boolean, nonpositive, or nonfinite values fail closed as `INVALID` without raising uncaught exceptions. Preserved Step 1 protocol integrity and Step 2 CV > 5.0% early noise stop (`INCONCLUSIVE`). Negative tests in `test_harness.py` pass (13/13 PASS). |
| **Finding 2** | P2 | Alignment-Padding Integrity & Admission Boundary | **FIXED** | Merged in PR #724 (commit `8ebff41`). Verified counterexamples (`k=5, A=8` byte 10; `k=5, A=64` byte 8; `k=10, A=1`, 6 symbols, corrupted byte 15). Implemented an explicit admission boundary: `validate_frame()` returns a validated, conventionally immutable `ValidatedFrame` for trusted in-process callers (with $O(\text{total\_bytes})$ immutable copy and $O(N)$ complete canonical payload validation accounted for in Phase 5); `admitted_random_access()` requires a `ValidatedFrame` and refuses raw unvalidated bytes; older `random_access()` is explicitly documented as an unvalidated raw primitive. Deterministic regression tests in `test_b729_u5.py` and `test_matched_codec.py` pass (30/30 PASS). |
| **Finding 3** | P1 | Phase 4 Retention Verification Reporting & Enforcement | **VERIFIED (WIRE-FRAME-ONLY RETENTION ENFORCED)** | Implemented in PR #725 on top of PR #724 baseline. Closed the open Phase 4 retention requirement. Monitored raw input symbols and mapped buffers via weak references in `retention.py` (`OwnedSymbols`, `OwnedBuffer`, `RetentionBarrier`). Required native ordering fence compilation and verification without optional pure-Python bypasses. Precomputed independent witness digests for random access queries and full decoded output prior to Phase 4. At Phase 4 boundary, explicitly released all raw symbol arrays and mapped buffers, triggered garbage collection, and confirmed zero live aliases remain. Subsequent Phase 6 queries and Phase 7 decoded output verify strictly against independent witness digests without retaining any raw symbol array. Explicitly labeled dry-run execution as smoke-test scale (`n=1000`, `num_queries=50`). Added negative tests in `test_harness.py` for alias leaks (`test_11`), corrupted queries (`test_12`), and untracked representation boundaries (`test_13`). Overall status reports `dry_run_status: "PASSED"` with all 8 phases verified. |

---

## 2. Retention Implementation & Boundary Analysis

### 2.1 Wire-Frame-Only Retention Boundary Enforcement
1. **Weak-Referenceable Owned Buffers:** Added `OwnedSymbols` and `OwnedArray` in `experiments/h04_r03_matched_codec/retention.py` alongside `OwnedBuffer`. `RetentionBarrier` strictly requires `fence_path` and `fence_sha256`, compiling and invoking the native C11 ordering fence (`retention_fence.c`) to ensure execution ordering before verifying weak references.
2. **Witness Preparation Prior to Phase 4:** Precomputed independent verification digests before dropping input objects:
   - `expected_query_witness`: SHA-256 digest of little-endian signed 64-bit query outputs for the query schedule.
   - `expected_output_witness`: SHA-256 digest of the little-endian signed 64-bit reconstructed stream.
   - `expected_xxh64`: xxHash64 digest of the output stream matching canonical witness conventions.
3. **Phase 4 Retention Boundary Execution:**
   - Raw input symbols (`symbols`), raw byte buffer (`raw_bytes`), and temporary structures are explicitly deleted (`del symbols, raw_bytes`).
   - `gc.collect()` is triggered.
   - `barrier.verify_release()` executes the native fence and asserts that `live_owned_buffers == 0`, failing closed with `ProtocolError` if any raw or mapped alias remains live.
4. **Wire-Frame-Only Query & Decode Verification:**
   - In Phase 6, `admitted_random_access()` operates exclusively on wire frames via `ValidatedFrame`. Queried values are hashed into `arm_q_hasher` and checked against `expected_query_witness`. No second full-symbol array is retained.
   - In Phase 7, frames are decoded via `decode()`, packed into 64-bit int buffers, and checked against `expected_output_witness` and `expected_xxh64`.
   - In Phase 8, all frames and buffers are cleanly deleted.
5. **Workload Scale Disambiguation:**
   - `BenchmarkHarness.run_dry_run_validation()` explicitly returns `"workload_mode": "SMOKE_TEST"` with `"smoke_configuration"` (`n=1000`, `num_queries=50`, `k=64`, `a=1`) and `"frozen_workload_reference"` (`n=1048576`, `num_queries=1048576`, `k=64`, `a=1`). It does not claim full-scale frozen H04-R03 workload verification.
6. **Retention Guarantees and Limitations:**
   - Weak references track only registered Python-owned objects, not arbitrary undiscovered aliases, external closures, C heap arenas, or OS physical memory reclamation.
   - Tested in `test_13_retention_limitation_untracked_surviving_representation`: if an untracked container or closure captures input symbols without registering in `RetentionBarrier`, weak reference tracking alone cannot observe it. Retention guarantees are therefore bounded to registered application buffers within cooperative in-process lifecycle contracts, not kernel-level memory erasure.
   - Allocator page retention (`glibc ptmalloc`) is not inferred to be reclaimed.
   - Phase C performance measurement remains strictly frozen.

---

## 3. Test Execution Verification & Provenance

### Execution Environment
- **Executor:** Jules (autonomous software engineer; author-reported execution, not independent third-party audit).
- **Environment:** Linux devbox `6.8.0 #1 SMP PREEMPT_DYNAMIC x86_64`, Ubuntu 24.04 toolchain (`gcc 13.3.0`, C11 standard library).
- **Python Runtime:** CPython 3.12.13 with `xxhash==3.5.0`.
- **Baseline Git Pin:** `50087972e078a09d7aff79cf6d8ae15db45c17ba`.

### Test Commands and Results
All test suites executed cleanly with zero regressions:

```bash
# 1. Phase B baseline test suite (17 tests)
python3 -m unittest -v experiments.h04_r03_matched_codec.test_matched_codec
# Result: Ran 17 tests - OK

# 2. Candidate A B729-µ5 correctness test suite (13 tests)
python3 -m unittest -v experiments.h04_r03_matched_codec.test_b729_u5
# Result: Ran 13 tests - OK

# 3. Deterministic PRNG & workload generators (4 tests)
python3 -m unittest -v experiments.h04_r03_matched_codec.test_workload
# Result: Ran 4 tests - OK

# 4. Harness, retention, & statistical decision rules (13 tests)
python3 -m unittest -v experiments.h04_r03_matched_codec.test_harness
# Result: Ran 13 tests - OK

# 5. Measurement engine mock & lifecycle suite (15 tests)
python3 -m unittest -v experiments.h04_r03_matched_codec.test_measurement_engine
# Result: Ran 15 tests - OK

# 6. Production process, enforcement & retention infrastructure (20 tests)
python3 -m unittest -v experiments.h04_r03_matched_codec.test_production_infrastructure
# Result: Ran 20 tests - OK

# 7. Verification of execution freeze enforcement
python3 -m experiments.h04_r03_matched_codec.benchmark_harness --run-measurement
# Output: ERROR: Phase C performance measurement is FROZEN. Exit code: 1
```

Total tests verified across all active suites: **82 tests passing with zero regressions**.

---

## 4. Frozen-Protocol Compatibility Analysis

1. **Protocol Integrity Precedence (§9.1):**
   Step 1 (Protocol Integrity) and Step 2 (CV Noise Stop) are strictly preserved.
2. **Noise Threshold (§8.3, §9.1):**
   CV > 5.0% early stop operates cleanly without generating unneeded CIs.
3. **Random-Access Payload Limits (§4.5):**
   The $\le 6$ payload byte read guarantee for B729-µ5 and 0 touched bytes from other microblocks are preserved by decoupling admission validation from element retrieval.
4. **Execution Freeze:**
   `--run-measurement` remains frozen and guarded. No performance or hardware advantage claims are made.
