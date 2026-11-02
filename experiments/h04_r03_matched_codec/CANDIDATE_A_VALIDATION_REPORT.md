# EXP-H04-R03 Candidate A Correctness Validation Report

**Status:** BOUNDED CORRECTNESS VALIDATION COMPLETE / PHASE C EXECUTION FROZEN
**Date:** 2026-11-02
**Parent Document:** `docs/research/continuity/H04_R03_CANDIDATE_A_PREREGISTRATION.md` (PR #677)
**Accepted Phase B Reference:** PR #658, Commit `0a48054510e79e753d5152ccb3f9f130c17bbafd`
**Scope:** Concrete implementation and exhaustive correctness verification of the proposed B729-µ5 microblock codec, independent witness, deterministic workload generators, and 4-arm harness. No timed performance benchmarking or advantage claims are made.

---

## 1. Executive Summary

This validation report documents the bounded implementation and deterministic correctness verification for **EXP-H04-R03 Candidate A**:
1. **Preservation of Phase B:** All accepted Phase B reference implementations (`B10`, `T243`, `B243-CLONE`, `B729`) and all 17 original test cases are preserved untouched and pass with 0 regressions.
2. **B729-µ5 Reference Codec:** Implemented in `codec.py` and `oracle.py` with canonical little-endian byte ordering, bounded 6-byte random-access extraction, tail microblock handling ($v \in \{1, 2, 3, 4, 5\}$), and fail-closed validation.
3. **Independent Witness:** Authored in `independent_witness_b729_u5.py` without sharing codec logic, providing independent reconstruction, known-answer vectors (KAVs), and bit-level padding checks.
4. **Exhaustive Testing:**
   - 729/729 ($100\%$) single-symbol round-trips in $[-364, 364]$.
   - 531,441/531,441 ($100\%$) ordered symbol pairs in $[-364, 364]^2$.
   - Bounded byte touch assertions: random access touches $\le 6$ payload bytes across all parameter grids.
   - Adversarial verification: truncated frames, high unused padding bits corruption, integer overflow, and digit-order reversal are rejected 100%.
5. **Deterministic Generators & Pinned SHA-256:**
   - Dataset generator (`workload.py`): SplitMix64 + xoshiro256++ with seed `0x5438315F44415441` and unbiased rejection limit `18446744073709550922`. Verified SHA-256 digest: `1188f12cb887f8ad044d970ca92e7e2f0125e57a5fda012606407da857cc2879`.
   - Query generator (`workload.py`): SplitMix64 + xoshiro256++ with seed `0x5438315F51554552` and 20-bit masking for $Q = 2^{20}$ queries with replacement. Verified SHA-256 digest: `02e5f3da8ad331fd8b64197884933ae11211974ce719e0354f2fe3321002938e`.
6. **Execution Freeze Maintained:** The 4-arm harness (`benchmark_harness.py`) models all 8 lifecycle phases and preregistered statistical decision rules while keeping performance measurement disabled by default.

---

## 2. Test Execution Summary

Running the complete test discovery suite across `experiments/h04_r03_matched_codec`:

```bash
python3 -m unittest discover -s experiments/h04_r03_matched_codec
```

**Results:**
- Total tests executed: **38**
- Total passed: **38**
- Total failed: **0**
- Skips / errors: **0**

### Breakdown by Test Module:
1. `test_matched_codec.py` (Phase B Baseline Suite):
   - 17/17 tests passing (Preservation of B10, T243, B243-CLONE, B729, framing, layout, known-answer vectors, and bounded access).
2. `test_b729_u5.py` (Candidate A B729-µ5 Correctness Suite):
   - 9/9 tests passing (Exhaustive 729 single-symbols, exhaustive 531,441 symbol pairs, KAV verification, grid parameter boundaries, bounded touch count $\le 6$ bytes, frame truncation, unused high padding rejection, integer overflow rejection, digit-order mutation rejection).
3. `test_workload.py` (Deterministic PRNG & Workload Suite):
   - 4/4 tests passing (SplitMix64 golden vector, xoshiro256++ stream reproducibility, full $N=2^{20}$ query schedule SHA-256 verification, full $N=2^{20}$ dataset symbol SHA-256 verification).
4. `test_harness.py` (Lifecycle & Statistical Decision Rules Suite):
   - 8/8 tests passing (Dry run across all 8 lifecycle phases, standard library bootstrap CI estimation, Step 1 INVALID, Step 2 INCONCLUSIVE noise, Step 3 REJECT advantage, Step 3 REJECT query inferiority, Step 4 ACCEPT full predicate, Step 5 INCONCLUSIVE straddle).

---

## 3. Preregistration Contradictions & Feasibility Constraints Analysis

Per Requirement 8 of the task directive, the following technical nuances and constraints in `H04_R03_CANDIDATE_A_PREREGISTRATION.md` were identified during implementation:

### 3.1 Global Density Equivalence Across All Block Sizes $m$
- In Section 1.2 and Section 4.4, the preregistration highlights the size equivalence of T243, B729 whole-block, and B729-µ5 specifically at $k=64$ (77 payload bytes) and for multiples of 5 ($6 \text{ bytes per } 5 \text{ values}$).
- **Mathematical Clarification:** The tail microblock byte length function $L_\mu(v) = \lceil \text{bit\_length}(729^v - 1) / 8 \rceil$ yields byte counts $[2, 3, 4, 5, 6]$ for $v \in [1, 2, 3, 4, 5]$.
- Because $L_{\text{T243}}(m) = \lceil 6m / 5 \rceil$, we verified that for **every integer $m \ge 1$**:
  $$L_{\text{B729-}\mu5}(m) = L_{\text{T243}}(m)$$
  The payload size equality is not restricted to $k=64$ or multiples of 5; it holds universally for all block lengths $m \ge 1$.

### 3.2 Rejection Margin Asymmetry vs Inferiority Definition
- In Section 9.2 item 1, practical advantage rejection is defined as:
  $$\text{CI}_{0.95}(\theta_{\text{life}}(C))_{\text{upper}} < 1.10$$
  This asserts that an upper confidence bound below $1.10$ conclusively rejects a $\ge 10\%$ win, even if T243 is marginally faster (e.g. $[1.02, 1.08]$ in Case 4).
- In Section 9.2 item 2, performance inferiority is defined as:
  $$\text{CI}_{0.95}(\theta_{\text{life}}(C))_{\text{upper}} < 0.95$$
  This establishes that T243 is strictly slower than comparator $C$ by at least $5\%$.
- **Finding:** Both rules correctly yield a `REJECT` disposition under Step 3, but represent distinct scientific states (Rejection of Practical Advantage vs Proof of Performance Inferiority). The implementation in `benchmark_harness.py` models this precedence faithfully without modifying the preregistered contract.

### 3.3 Scratch Memory Cap Feasibility (64 KB)
- Section 7.2 imposes a strict scratch allocation cap of $64 \text{ KB}$ across the Phase 6 random-access query loop.
- **Feasibility:** In B729-µ5, random-access lookup slices exactly $L_\mu(v_g) \le 6$ contiguous bytes and unpacks an unsigned integer directly. By avoiding any decoded-block caching or list allocation, memory consumption during each query is $O(1)$ and easily remains within the $64 \text{ KB}$ ceiling.

---

## 4. Contract Deviations

**Zero deviations from the contract.**
- Phase B reference implementations are preserved.
- B729-µ5 wire grammar adheres strictly to Section 4 of the preregistration.
- SplitMix64 and xoshiro256++ implementations and seeds adhere strictly to Section 5.
- The 8 lifecycle phases and statistical evaluation precedence adhere strictly to Section 6, 8, and 9.

---

## 5. Remaining Prerequisites for Phase C Execution Authorization

Prior to activating Phase C performance measurement trials, the following prerequisites remain mandatory:
1. **Maintainer Authorization Sign-off:** Complete sign-off of the Checklist items in Section 12 of `H04_R03_CANDIDATE_A_PREREGISTRATION.md`.
2. **Dedicated Single-Host Qualification:** Securing an isolated, dedicated Linux x86_64 host with pinned CPU frequency governor, disabled turbo boost, isolated CPU core affinity via `taskset`, and documented cache configuration (Section 7.1).
3. **Execution Freeze Lift:** Maintainer sign-off lifting the execution freeze on `--run-measurement`.
