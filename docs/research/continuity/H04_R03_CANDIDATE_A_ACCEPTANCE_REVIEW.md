# Independent Acceptance Review: PR #679 (EXP-H04-R03 Candidate A B729-µ5)

**Document:** `docs/research/continuity/H04_R03_CANDIDATE_A_ACCEPTANCE_REVIEW.md`
**Date:** 2026-11-02
**Target Review:** PR #679 — EXP-H04-R03 Candidate A B729-µ5 Implementation
**Reviewed Head Commit:** `e79f26e5a68b3b0a46cf3d50aca035493b8e549b`
**Base Commit (Direct Parent):** `8e44c0f667c62e0d4143bb93169fc3235d5ebd66`
**Accepted Phase B Reference:** PR #658, Commit `0a48054510e79e753d5152ccb3f9f130c17bbafd`
**Parent Preregistration:** `docs/research/continuity/H04_R03_CANDIDATE_A_PREREGISTRATION.md` (PR #677)
**Final Disposition:** **ACCEPT — CORRECTNESS AND PROTOCOL IMPLEMENTATION ONLY**

---

## 1. Executive Summary & Review Scope

This document provides a bounded, independent scientific and implementation acceptance review of **PR #679** (Candidate A B729-µ5 microblock codec implementation and experimental harness for study EXP-H04-R03).

### 1.1 Scope Boundaries & Non-Claims
- **Scope Limit:** This acceptance review evaluates code correctness, mathematical equivalence, workload reproducibility, harness decision logic, and contract preservation against the merged Candidate A preregistration and accepted Phase B baseline.
- **Phase C Execution Frozen:** Phase C performance measurement trials have **not** been executed, authorized, or activated.
- **No Advantage Claims:** This acceptance review does **not** demonstrate, validate, or imply any hardware, throughput, latency, energy, or ternary-specific advantage over binary architectures.
- **No Governance or Infrastructure Mutation:** No unrelated CI, root structure, or historical receipts are altered by this review.

### 1.2 Base Commit & PR Ancestry Verification
- Git ancestry was verified independently:
  - `git rev-parse e79f26e5a68b3b0a46cf3d50aca035493b8e549b^` resolves directly to `8e44c0f667c62e0d4143bb93169fc3235d5ebd66`.
  - Commit `8e44c0f6` represents the merged Candidate A Preregistration document reconciliation on `origin/main`.
  - The PR ancestry is direct and authentic with no baseline substitution.

---

## 2. Evaluation Against Review Requirements

| Requirement | Description | Status | Evidence / Notes |
|---|---|---|---|
| **1. Protocol Fidelity** | Comparison arms, workload parameters, lifecycle phases, and freeze enforcement | **PASS** | Implements all 4 arms (`T243`, `B10`, `B729`, `B729-u5`). Models frozen workload ($N=2^{20}, k=64, A=1, \text{RA-FIXED}, Q=N$) and all 8 lifecycle phases without measurement leaks. |
| **2. Mathematical Verification** | Payload equality $L_{\text{B729-}\mu5}(m) = L_{\text{T243}}(m)$ across block lengths and bounded access | **PASS** | Analytically proven and empirically verified across $m \in [1, 100000)$. Random access extraction strictly bounded to $\le 6$ bytes. |
| **3. Codec Correctness** | 729-symbol domain, exhaustive single/pair checks, padding checks, corruption fail-closed | **PASS** | 729/729 single symbols and 531,441/531,441 symbol pairs round-trip. Independent witness in `independent_witness_b729_u5.py` is genuinely independent. |
| **4. Workload Reproducibility** | SplitMix64 / xoshiro256++, unbiased sampling, serialization, canonical SHA-256 | **PASS** | Exact match on dataset digest (`1188f12c...`) and query digest (`02e5f3da...`). Rejection sampling matches theoretical threshold `18446744073709550922`. |
| **5. Experimental Harness** | Dry-run vs measurement, 5-step decision precedence, bootstrap CI, memory accounting | **PASS** | Measurement execution fails closed by default. Evaluates 5-step decision rules matching Section 9.4. Bounded $O(1)$ memory usage. |
| **6. Preservation** | Preservation of accepted Phase B assets and 17 baseline tests | **PASS** | Phase B codecs and all 17 baseline unit tests preserved byte-for-byte in behavior and passing 100% (38/38 total suite passes). |

---

## 3. Detailed Review Findings

### 3.1 Requirement 1: Protocol Fidelity (PASS)
- **Four Comparison Arms:** `codec.py`, `oracle.py`, and `benchmark_harness.py` register and handle all four comparison arms:
  - Arm 1: `T243` (Base-243 native balanced ternary packing)
  - Arm 2: `B10` (Unpacked 10-bit binary baseline)
  - Arm 3: `B729` (Whole-block Base-729 binary packing)
  - Arm 4: `B729-u5` / `B729-µ5` (Proposed 5-value microblock binary packing)
- **Workload Parameters:** Pinned to $N = 2^{20} = 1,048,576$, $k = 64$, $A = 1$, access mode `RA-FIXED`, and $Q = N$ uniform random-access queries drawn with replacement.
- **8 Lifecycle Phases:** `BenchmarkHarness.run_dry_run_validation()` models all 8 lifecycle phases:
  1. Input Mapping & Domain Conversion ($x \mapsto u = x + 364$)
  2. Frame Allocation & Oracle Layout Verification
  3. Encoding & Packing
  4. Retention Boundary Simulation (in-memory wire byte blob)
  5. Frame Validation & Integrity
  6. Timed Random Access Queries (bounded dry-run)
  7. Output Buffer Assembly & Checksum Verification
  8. Deallocation & Scratch Cleanup
- **Execution Freeze:** Adheres to Phase C execution freeze. CLI flag `--run-measurement` explicitly halts with exit code 1 and error message requiring separate maintainer authorization and dedicated hardware qualification.

### 3.2 Requirement 2: Mathematical Verification (PASS)
- **Analytical Equality Theorem:**
  - Microblock byte length function: $L_\mu(v) = \lceil \text{bit\_length}(729^v - 1) / 8 \rceil$.
  - For $v \in \{1, 2, 3, 4, 5\}$, $729^v - 1$ evaluates to:
    - $v=1$: $728 \implies 10 \text{ bits} \implies L_\mu(1) = 2 \text{ bytes}$
    - $v=2$: $531,440 \implies 20 \text{ bits} \implies L_\mu(2) = 3 \text{ bytes}$
    - $v=3$: $387,420,488 \implies 29 \text{ bits} \implies L_\mu(3) = 4 \text{ bytes}$
    - $v=4$: $282,429,536,480 \implies 39 \text{ bits} \implies L_\mu(4) = 5 \text{ bytes}$
    - $v=5$: $205,891,132,094,648 \implies 48 \text{ bits} \implies L_\mu(5) = 6 \text{ bytes}$
  - In comparison, Base-243 packing payload length is $L_{\text{T243}}(m) = \lceil 6m / 5 \rceil$.
  - For full microblocks ($g = \lfloor m / 5 \rfloor$), both encodings consume exactly $6g$ bytes.
  - For remainder $r = m \pmod 5 \in \{0, 1, 2, 3, 4\}$:
    - $r=0$: $L_\mu(0) = 0$; $\lceil 6(0)/5 \rceil = 0$.
    - $r=1$: $L_\mu(1) = 2$; $\lceil 6(1)/5 \rceil = 2$.
    - $r=2$: $L_\mu(2) = 3$; $\lceil 6(2)/5 \rceil = 3$.
    - $r=3$: $L_\mu(3) = 4$; $\lceil 6(3)/5 \rceil = 4$.
    - $r=4$: $L_\mu(4) = 5$; $\lceil 6(4)/5 \rceil = 5$.
  - Therefore:
    $$\forall m \ge 1, \quad L_{\text{B729-}\mu5}(m) = L_{\text{T243}}(m)$$
  - This equality was verified independently by running an automated assertion loop across all $m \in [1, 100000)$ with 0 mismatches.
- **Bounded Access:** Random-access payload extraction reads at most $L_\mu(v_g) \le 6$ contiguous bytes, unpacking symbol $u = \lfloor I_\mu / 729^p \rfloor \pmod{729}$ in $O(1)$ operations without decoding the entire block.

### 3.3 Requirement 3: Codec Correctness (PASS)
- **Domain Coverage:**
  - Single symbols: Exhaustive test over all 729 integers in $[-364, 364]$ verified encode $\to$ decode and random-access identity.
  - Ordered pairs: Exhaustive test over all $729^2 = 531,441$ pairs in $[-364, 364]^2$ verified complete round-trip identity against both reference decoder and independent witness.
- **Independent Witness Independence:**
  - `independent_witness_b729_u5.py` does not import or call `codec.py`.
  - Implements independent integer assembly (`sum(byte_val * (256**i))`), independent Horner extraction (`divmod(curr, 729)`), hard-coded bit length tables, and independent padding validation.
  - Contains 4 distinct hard-coded known-answer vectors (KAVs), including full microblock vector $u = [0, 182, 364, 546, 728] \implies I_\mu = 205,820,427,722,364 \implies \text{bytes } [0x7C, 0x5E, 0x7B, 0x4D, 0x31, 0xBB]$.
- **Fail-Closed Validation:**
  - Truncated payloads raise `CodecError` / `ValueError`.
  - Nonzero high padding bits in unused microblock bits raise `CodecError` / `ValueError`.
  - Microblock integer overflow ($I_\mu \ge 729^v$) raises `CodecError` / `ValueError`.
  - Digit order and symbol reversal mutations fail independent witness validation.

### 3.4 Requirement 4: Workload Reproducibility (PASS)
- **Algorithms:**
  - SplitMix64 generator implemented with standard golden constants (`0x9E3779B97F4A7C15`, `0xBF58476D1CE4E5B9`, `0x94D049BB133111EB`).
  - xoshiro256++ generator implements standard 4-state rotate-left transitions (`rotl64(..., 23)`, `rotl64(..., 45)`).
- **Unbiased Rejection Sampling:**
  - $2^{64} = 729 \times 25,302,403,348,724,534 + 694$.
  - Rejection limit: $2^{64} - 694 = 18,446,744,073,709,550,922$ (`LIMIT_729`).
- **Canonical Digest Verification:**
  - Dataset ($N=2^{20}$, seed `0x5438315F44415441`, little-endian signed 16-bit integers `<h`):
    `1188f12cb887f8ad044d970ca92e7e2f0125e57a5fda012606407da857cc2879` (100% match).
  - Query schedule ($Q=2^{20}$, seed `0x5438315F51554552`, direct 20-bit masking, little-endian unsigned 32-bit integers `<I`):
    `02e5f3da8ad331fd8b64197884933ae11211974ce719e0354f2fe3321002938e` (100% match).

### 3.5 Requirement 5: Experimental Harness (PASS)
- **Separation of Dry Run vs Measurement:**
  - Automated testing executes only `run_dry_run_validation()`, which operates over reduced sample vectors without high-resolution timers or benchmark loops.
  - `--run-measurement` fails closed with exit code 1 to enforce the execution freeze.
- **Statistical Protocol:**
  - `compute_bootstrap_ci` implements standard paired whole-run resamples with replacement (10,000 bootstrap draws) using standard library `random` and `statistics`.
  - `evaluate_decision_rules` implements exact deterministic precedence per Section 9.1:
    - Step 1: Protocol validation failure $\implies$ `INVALID`
    - Step 2: System stability noise criterion ($\text{CV} > 5.0\%$) $\implies$ `INCONCLUSIVE`
    - Step 3: Conclusive rejection predicate ($\text{CI}_{0.95}(\theta_{\text{life}})_{\text{upper}} < 1.10$ vs any binary control, or $\text{CI}_{0.95}(\theta_{\text{query}})_{\text{upper}} < 0.95$) $\implies$ `REJECT`
    - Step 4: Complete acceptance predicate ($\text{CI}_{0.95}(\theta_{\text{life}})_{\text{lower}} \ge 1.10$ against all 3 binary controls and query non-inferiority $\ge 0.95$) $\implies$ `ACCEPT`
    - Step 5: Exhaustive residual fallback $\implies$ `INCONCLUSIVE`
- **Memory Bounds:** Bounded random-access extraction uses $O(1)$ memory, operating comfortably within the 64 KB scratch budget.

### 3.6 Requirement 6: Preservation (PASS)
- All Phase B reference assets are preserved:
  - `experiments/h04_r03_matched_codec/SPECIFICATION.md` (unchanged)
  - `experiments/h04_r03_matched_codec/VALIDATION_REPORT.md` (unchanged)
  - `experiments/h04_r03_matched_codec/test_matched_codec.py` (all 17 baseline tests unchanged and passing)
  - Existing Phase B codecs (`B10`, `T243`, `B243-CLONE`, `B729`) preserved.
- Total test suite count increased from 17 to 38 tests, all passing with zero failures.

---

## 4. Defect Classification & Findings

- **P0 Findings (Blockers):** None.
- **P1 Findings (Major Defects):** None.
- **P2 Findings (Minor / Informational):**
  - **FINDING-P2-01 (Documentation / Scope Nuance):** The preregistration document in Section 1.2 and Section 4.4 describes payload size equality specifically for $k=64$ and multiples of 5. As mathematically proven in Section 3.2 of this review (and noted in PR #679's validation report), the equality $L_{\text{B729-}\mu5}(m) = L_{\text{T243}}(m)$ is universal across all positive integers $m \ge 1$. This is a positive mathematical clarification that strengthens the experimental foundation and requires no corrective code change.

---

## 5. Independent Validation Execution Log

The following commands were executed in the environment to verify PR #679:

```bash
# 1. Run complete matched codec test suite (38 tests)
python3 -m unittest discover -s experiments/h04_r03_matched_codec
# Output: Ran 38 tests in 12.299s - OK

# 2. Run Phase B baseline test suite (17 tests)
python3 -m unittest -v experiments.h04_r03_matched_codec.test_matched_codec
# Output: Ran 17 tests - OK

# 3. Run B729-µ5 correctness test suite (9 tests)
python3 -m unittest -v experiments.h04_r03_matched_codec.test_b729_u5
# Output: Ran 9 tests - OK

# 4. Run deterministic workload test suite (4 tests)
python3 -m unittest -v experiments.h04_r03_matched_codec.test_workload
# Output: Ran 4 tests - OK

# 5. Run benchmark harness and decision rule test suite (8 tests)
python3 -m unittest -v experiments.h04_r03_matched_codec.test_harness
# Output: Ran 8 tests - OK

# 6. Verify universal mathematical equality for m in [1, 100000)
python3 -c "
from experiments.h04_r03_matched_codec.oracle import L_B729_u5, L_T243
for m in range(1, 100000):
    assert L_B729_u5(m) == L_T243(m)
print('Verified m in [1, 100000)')
"
# Output: Verified m in [1, 100000)

# 7. Verify execution freeze enforcement on measurement flag
python3 -m experiments.h04_r03_matched_codec.benchmark_harness --run-measurement
# Output: ERROR: Phase C performance measurement is FROZEN. Exit code: 1

# 8. Verify root structure governance compliance
python3 scripts/governance/check_root_structure.py
# Output: root structure check PASSED (81 entries checked)
```

---

## 6. Minimal Corrective Actions

**No corrective code modifications are required.**
The implementation in PR #679 is functionally complete, mathematically sound, fails closed on corrupted data, adheres to all preregistered protocols, and maintains the required execution freeze.

---

## 7. Remaining Prerequisites for Phase C Execution Authorization

Candidate A is technically and scientifically qualified at the implementation and unit-testing level. Prior to any separately authorized Phase C execution, the following prerequisites must be met:

1. **Maintainer Authorization Sign-off:** Completion and explicit sign-off of the checklist in Section 12 of `H04_R03_CANDIDATE_A_PREREGISTRATION.md`.
2. **Dedicated Physical Host Qualification:** Securing dedicated x86_64 hardware meeting the single-host specification:
   - Pinned CPU frequency governor (`performance`).
   - Dynamic frequency scaling (Intel Turbo Boost / AMD Core Performance Boost) disabled.
   - Pinned core affinity via `taskset`.
   - Characterized cache hierarchies (L1, L2, L3) and memory bandwidth.
3. **Execution Freeze Lift:** Separate maintainer authorization removing the `--run-measurement` execution freeze.

---

## 8. Final Acceptance Disposition

**ACCEPT — CORRECTNESS AND PROTOCOL IMPLEMENTATION ONLY.**

PR #679 is ready to be merged into the development branch pending maintainer approval, under the continued condition that Phase C measurement execution remains frozen until separately authorized.
