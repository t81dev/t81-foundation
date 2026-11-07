# T81 H04-R03-FC1 Evidence Receipt — Three-Finding Correctness Closure & Phase 4 Wire-Frame-Only Retention Enforcement

**Document:** `docs/research/continuity/H04_R03_FC1_EVIDENCE_RECEIPT.md`
**Date:** 2026-11-07
**Task Anchor:** https://github.com/t81dev/t81-foundation/issues/723
**Context:** https://github.com/t81dev/t81-foundation/pull/722
**Base Commit SHA:** `1103356ac867a88b3c3873a7e2e696d48378f424`
**Parent Study:** EXP-H04-R03 Candidate A Matched Codec
**Overall Milestone Disposition:** **VERIFIED**
**Phase C Authorization Status:** **STRICTLY FROZEN / UNAUTHORIZED**

---

## 1. Executive Summary & Review Findings Resolution

This evidence receipt documents the corrective implementation resolving three Codex review findings reported on PR #679 in study EXP-H04-R03 and closing the Phase 4 wire-frame-only retention enforcement requirement.

### Per-Finding Dispositions

| Finding | Severity | Description | Final Disposition | Summary of Evidence |
|---|---|---|---|---|
| **Finding 1** | P1 | Fallback Confidence Intervals & Statistical Integrity | **FIXED (DEFENSIVE HARDENING)** | Removed all five `(1.0, 1.0, 1.0)` fallback confidence intervals in `benchmark_harness.py`. Clarified that at base `1103356`, required CI validation already returned `INVALID` for missing mandatory intervals; removal of dead fallback code eliminates misleading dead branches. Hardened CI validation with `_is_valid_ci()` so non-tuple, non-numeric, boolean, nonpositive, or nonfinite values fail closed as `INVALID` without raising uncaught exceptions. Preserved Step 1 protocol integrity and Step 2 CV > 5.0% early noise stop (`INCONCLUSIVE`). Negative tests added in `test_harness.py` (12/12 PASS). |
| **Finding 2** | P2 | Alignment-Padding Integrity & Admission Boundary | **FIXED** | Verified both original Codex counterexamples (`k=5, A=8` byte 10; `k=5, A=64` byte 8) and added a third multi-microblock regression (`k=10, A=1`, 6 symbols, corrupted byte 15). Analyzed access-accounting trade-offs against frozen §4.5/4.6 byte budgets: scanning alignment gaps and block padding inside individual queries would violate the $\le 6$ payload byte limit and $O(1)$ random access operation count. Implemented an explicit admission boundary: `validate_frame()` returns a validated, conventionally immutable `ValidatedFrame` for trusted in-process callers (with $O(\text{total\_bytes})$ immutable copy and $O(N)$ complete canonical payload validation accounted for in Phase 5); `admitted_random_access()` requires a `ValidatedFrame` and refuses raw unvalidated bytes; older `random_access()` is explicitly documented as an unvalidated raw primitive. Added deterministic regression tests in `test_b729_u5.py` and `test_matched_codec.py` (30/30 PASS). |
| **Finding 3** | P1 | Phase 4 Retention Verification Reporting & Enforcement | **VERIFIED (WIRE-FRAME-ONLY RETENTION ENFORCED)** | Closed the open Phase 4 retention requirement. Monitored raw input symbols and mapped buffers via weak references in `retention.py` (`OwnedSymbols`, `OwnedBuffer`, `RetentionBarrier`). Precomputed independent witness digests for random access queries and full decoded output prior to Phase 4. At Phase 4 boundary, explicitly released all raw symbol arrays and mapped buffers, triggered garbage collection, and confirmed zero live aliases remain. Subsequent Phase 6 queries and Phase 7 decoded output verify strictly against independent witness digests without retaining any raw symbol array. Added negative tests in `test_harness.py` verifying alias leaks fail closed. Overall status reports `dry_run_status: "PASSED"` with all 8 phases verified. |

---

## 2. Before-and-After Reproductions & Analysis

### 2.1 Finding 1 — Statistical Decision Fallbacks & Defensive Hardening

#### Baseline Context & Evaluation
In `benchmark_harness.py` at pinned base `1103356`:
`evaluate_decision_rules` fetched confidence intervals with `.get(..., (1.0, 1.0, 1.0))`. However, the harness already checked:
```python
if not all(
    arm in intervals and len(intervals[arm]) == 3
    ...
    for intervals, arms in ((ci_life, ARMS[1:]), (ci_query, ("B10", "B729-u5")))
    for arm in arms
):
    return {"classification": "INVALID", "reason": "Incomplete or invalid confidence intervals"}
```
Because this validation ran before the `.get(..., (1.0, 1.0, 1.0))` lookups, missing required arms were already caught and returned `INVALID`. The fallback tuples were dead code that misleadingly suggested missing evidence could produce `ACCEPT` or `REJECT`. Furthermore, malformed types (e.g. non-tuples, non-numeric values, booleans) could cause uncaught `TypeError` exceptions.

#### Corrective Action Taken
- Completely removed all five `(1.0, 1.0, 1.0)` fallback tuples.
- Implemented `_is_valid_ci(ci)` helper ensuring every required CI is a 3-element tuple or list of finite, positive numbers (`int` or `float`, strictly excluding `bool`), with `ci[1] <= ci[2]`.
- Enforced that if `ci_life` or `ci_query` are not dictionaries, or if any required arm is missing or invalid, `evaluate_decision_rules` fails closed as `INVALID`.
- Preserved Step 2 early noise stop: when run-to-run variation CV > 5.0%, the harness returns `INCONCLUSIVE` without requiring CI calculations.
- Verified in `test_harness.py` via `test_09` and `test_10`.

---

### 2.2 Finding 2 — Alignment-Padding Integrity & Admission Boundary

#### Counterexample Reproduction
1. **Counterexample 1 (Block Alignment Padding):** `k=5, A=8`, encode `[0]` with `B729-u5`, corrupt byte 10. `decode()` failed closed with `CodecError`, but unvalidated `random_access(..., 0)` returned `0` without error.
2. **Counterexample 2 (Header Alignment Gap):** `k=5, A=64`, encode `[0]` with `B729-u5`, corrupt byte 8. `decode()` failed closed with `CodecError`, but unvalidated `random_access(..., 0)` returned `0` without error.
3. **Counterexample 3 (Multi-Microblock Payload Corruption):** `k=10, A=1`, encode `[0]*6` with `B729-u5` (6 symbols across 2 microblocks: 5 symbols in µ0, 1 symbol in µ1). Corrupt bit 10 in byte 15 (unused high bit of µ1). If unvalidated, `random_access(..., 0)` targeting µ0 returns `0` without error because it reads only bytes 8..13.

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

#### Implemented Resolution: Enforced Validated-Frame Pathway
Per `SPECIFICATION.md` Section 6.6:
> *"Validate complete inputs and canonical framing before admitted queries. Full validation work is reported separately and included in the end-to-end cost; a previously validated immutable buffer may amortize validation identically for all arms. Do not scan/validate the whole file silently inside one arm's lookup while giving others prevalidated input."*

To satisfy both padding integrity and the frozen $\le 6$ payload byte access limit:
1. **Conventionally Immutable Admission Object:** Implemented `ValidatedFrame` in `codec.py` as a validated, conventionally immutable object designed for trusted, cooperative in-process Python callers—not a cryptographically authenticated or hostile-caller-proof security boundary. Utilizes a private token (`_VALIDATED_FRAME_CONSTRUCTOR_TOKEN`) to prevent direct public instantiation. Its internal attributes are read-only properties with `__setattr__` and `__delattr__` overrides preventing post-instantiation mutation. An immutable snapshot (`bytes(blob)`) is taken before validation begins.
2. **Comprehensive Admission Validation & Accounting:** `validate_frame(blob, arm, k, A, access_mode)` performs complete frame and payload validation:
   - Snapshot copy: takes an immutable byte copy ($O(\text{total\_bytes})$ memory copy cost).
   - Framing: verifies 8-byte element count header $N$ and canonical expected total wire length.
   - Alignment gaps: verifies header-data alignment gap ($D - H$) is strictly zero-padded.
   - Block offset index: verifies canonical index offsets if RA-INDEXED.
   - Block alignment padding: verifies all block tail alignment paddings are zero across all blocks.
   - Complete canonical payload decoding: verifies all payload contents across all $B$ blocks ($O(N)$ decode operations via `ARM_DECODERS[arm]`), ensuring microblock high padding bits, integer range bounds ($I_\mu < 729^v$), and value domains are completely checked before admission.
   - Full validation cost is accounted for separately in Phase 5 (Frame Validation) rather than hidden inside individual queries.
3. **Admitted Random Access:** `admitted_random_access(frame: ValidatedFrame, index: int)`:
   - Strictly requires a `ValidatedFrame` token; refuses raw unvalidated `bytes` or `bytearray` (raises `TypeError`).
   - Slices only the target microblock (at most 6 payload bytes for B729-µ5).
   - Older `random_access()` is explicitly documented in docstrings as an unvalidated raw primitive for low-level testing that bypasses admission validation.
   - Proven in `test_10`, `test_11`, `test_12`, and `test_13` in `test_b729_u5.py`: corrupted frames cannot produce a `ValidatedFrame` or reach an admitted query.

---

### 2.3 Finding 3 — Phase 4 Retention Verification Reporting & Enforcement

#### Pre-Fix Behavior
`BenchmarkHarness.run_dry_run_validation()` retained `self.u_symbols` and used `symbols` in subsequent phases while emitting `"dry_run_status": "PASSED"` and listing `"Phase 4: Retention Boundary"` in `phases_verified`.

#### Post-Fix Implementation and Closure
1. **Weak-Referenceable Owned Buffers:** Added `OwnedSymbols` and `OwnedArray` in `experiments/h04_r03_matched_codec/retention.py` alongside `OwnedBuffer`. Made `RetentionBarrier` flexible to support optional native fence verification or pure in-process weak reference verification.
2. **Witness Preparation Prior to Phase 4:** Precomputed independent verification digests:
   - `expected_query_witness`: SHA-256 digest of little-endian signed 64-bit query outputs for the query schedule ($Q=50$ indices).
   - `expected_output_witness`: SHA-256 digest of the little-endian signed 64-bit reconstructed stream.
   - `expected_xxh64`: xxHash64 digest of the output stream matching canonical witness conventions.
3. **Phase 4 Retention Boundary Execution:**
   - Raw input symbols (`symbols`), raw byte buffer (`raw_bytes`), and temporary structures are explicitly deleted (`del symbols, raw_bytes`).
   - `gc.collect()` is triggered.
   - `barrier.verify_release()` executes and asserts that `live_owned_buffers == 0`, failing closed with `ProtocolError` if any raw or mapped alias remains live.
4. **Wire-Frame-Only Query & Decode Verification:**
   - In Phase 6, `admitted_random_access()` operates exclusively on wire frames via `ValidatedFrame`. Queried values are hashed into `arm_q_hasher` and checked against `expected_query_witness`. No second full-symbol array is retained.
   - In Phase 7, frames are decoded via `decode()`, packed into 64-bit int buffers, and checked against `expected_output_witness` and `expected_xxh64`.
   - In Phase 8, all frames and buffers are cleanly deleted.
5. **Status Reporting:**
   - `run_dry_run_validation()` reports `dry_run_status: "PASSED"`.
   - `phase_status["Phase 4: Retention Boundary"]` is `"VERIFIED"`.
   - `phases_verified` includes all 8 phases.
   - `retention_evidence` is included in the output dictionary.
6. **Negative Tests:**
   - `test_11_retention_boundary_negative_alias_leak_rejected`: confirms that an alias captured across Phase 4 triggers `ProtocolError`.
   - `test_12_query_witness_negative_corrupted_query_detected`: confirms that corrupted random-access queries fail closed against the query witness.

#### Limitations
- In-process weak-reference alias monitoring proves that Python application objects have zero live references and are collected. As documented in `ENFORCEMENT_AND_PROCESS.md` and `H04_R03_ENFORCEMENT_FEASIBILITY_AUDIT.md`, this does not claim OS-level physical memory page erasure or allocator page return to the kernel (`MADV_DONTNEED`).
- Phase C performance measurement remains strictly frozen.

---

## 3. Test Execution Verification

All relevant test suites were executed independently:

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

# 4. Harness & statistical decision rules (12 tests)
python3 -m unittest -v experiments.h04_r03_matched_codec.test_harness
# Result: Ran 12 tests - OK

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

Total tests verified across all active suites: **81 tests passing with zero regressions**.

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
