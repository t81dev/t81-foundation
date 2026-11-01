# EXP-H04-R03 Phase B Validation Report

**Experiment:** EXP-H04-R03 (Matched Codec Space-Time Tradeoff & Bounded-Access Discriminator)
**Phase:** Phase B — Deterministic Correctness & Reference Implementation
**Status:** COMPLETE (Deterministic Correctness Passed; Performance Benchmarking Strictly Stopped)
**Repository Baseline Commit SHA:** `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`
**Execution Date:** 2026-11-01
**Authority:** Bounded implementation and validation only. No merge, performance benchmarking, or production activation.

---

## 1. Executive Summary & Response to Codex Review Findings

This report details the Phase B deterministic implementation and explicitly resolves the three adversarial findings raised during acceptance review:

### Response to Finding 1: Constant-Space Bounded-Block Random Access
- **Defect in previous draft:** `random_access()` called `compute_layout(N, ...)`, constructing an $O(B)$ list of all block offsets across the entire artifact, violating constant-space random access bounds for large $N$.
- **Correction applied:** Refactored `random_access()` in `codec.py` to calculate the target block index $j = \text{index} // k$, block stride, and target block offset $D + j \times \text{stride}$ in strictly $O(1)$ arithmetic space. For `RA-INDEXED`, the function reads solely the 8-byte offset entry at byte offset $8 + 8j$. Preceding blocks are never decoded or enumerated.
- **Verification:** Verified via deterministic assertions and bounds testing (`test_01`, `test_04`, `test_14`) showing touched bytes and operations depend strictly on $k$ and not $N$.

### Response to Finding 2: Pre-Allocation Framing & Declared Count Validation
- **Defect in previous draft:** Framing length checks permitted reading large headers before validating whether the declared count $N$ could physically fit within the supplied byte buffer, exposing decoders to excessive loop counts or denial-of-service on malformed payloads (e.g. $N = 2^{64}-1$).
- **Correction applied:** `parse_framing()` and `random_access()` now enforce strict pre-allocation validation before any loop execution or index parsing. Declared count $N$ is compared against the minimal theoretical byte lower bound for the buffer length ($8 + \lfloor 6N/8 \rfloor \le \text{len}(\text{blob})$ and $N \le 2^{48}$).
- **Verification:** Validated by `test_09_adversarial_malformed_frames_and_corruptions`, explicitly asserting rejection of maximum uint64 count ($2^{64}-1$) and inflated $N$ headers on truncated frames without allocation.

### Response to Finding 3: Independent Correctness Witnesses & Known-Answer Vectors
- **Defect in previous draft:** Correctness testing relied on candidate routines checking against candidate encoders/decoders without a fully detached analytical witness or known-answer vectors.
- **Correction applied:** Created `experiments/h04_r03_matched_codec/independent_witness.py` providing hard-coded analytical known-answer vectors (KAV) and independently coded bit/trit string decoders for B10, T243, B243-CLONE, and B729 without importing or calling `codec.py` routines.
- **Verification:** Verified in `test_15_known_answer_vectors_and_independent_decoders` ensuring bit-for-bit wire parity with independently derived bytes and 100% agreement on decoded payloads.

---

## 2. Scientific Integrity and Falsification Boundary

1. **Retention of Prior Audit Verdicts:**
   - H04-R02 original methodology remains classified as **Classification D (Invalid Experimental Methodology)** due to historical metadata scaling defects, division math errors, and comparator capacity asymmetry.
   - The corrected conditional model finding (`boundary_oscillation`, 10-bit Tier-0 control, $AT = 1210 < AB = 1250$ at $r = 1.60$, crossover $5/3$) remains preserved in `experiments/h04_r02_adaptive_representation/CORRECTED_RESULTS.md`.
   - Scientific Investigation 10A disposition remains **PHYSICAL EVALUATION BLOCKED — MATCHED CHARACTERIZATION DATA UNAVAILABLE**.

2. **No Hardware Superiority Claim:**
   - **B243-CLONE** demonstrates constructive identity: a conventional binary machine emits and decodes the exact same byte language as Base-243 bit-for-bit.
   - Any storage compactness relative to fixed 10-bit fields is a **codec-design property**, **not** a demonstration of native ternary hardware superiority or radix-exclusive advantage.
   - Furthermore, the analytical theorem $L_{\text{B729}}(m) \le L_{\text{T243}}(m)$ holds for **every** block size $m$, proving that Base-243 is never more compact than domain-matched binary block integer coding.

3. **Separation of Concerns:**
   - Mathematical bounds ($b(m) = \text{bit\_length}(729^m - 1)$) are strictly distinguished from implemented byte layouts (which include count headers, index tables, alignment gaps, and final block padding).
   - Deterministic wire correctness is verified; no CPU cycles, execution timings, PMU counters, or energy metrics were collected.

---

## 3. Validation Results and Test Accounting

### Test Execution Command
```bash
python3 experiments/h04_r03_matched_codec/test_matched_codec.py
```

### Results Summary
- **Total Test Cases:** 16
- **Passed:** 16
- **Failed:** 0
- **Skipped:** 0
- **Duration:** 1.56s

### Test Coverage Details

| Test Case | Description | Result |
|---|---|---|
| `test_01_all_729_individual_symbols` | Exhaustive round-trip and random access for all 729 symbols in `[-364, 364]` across all 4 arms | **PASS** |
| `test_02_t243_clone_bit_for_bit_identity` | Bit-for-bit byte equivalence between T243 and B243-CLONE across lengths, block sizes, alignments, and access modes | **PASS** |
| `test_03_exhaustive_small_sequences` | Exhaustive pairwise combinations of boundary states | **PASS** |
| `test_04_block_boundaries_and_incomplete_blocks` | Block boundary checks: $N \in \{0, 1, 2, 3, 4, 5, 6, 63, 64, 65, 1023, 1024, 1025\}$ across $k \in \{1, 5, 64, 1024\}$, $A \in \{1, 8, 64\}$, and RA modes | **PASS** |
| `test_05_deterministic_workloads_round_trip` | Determinism across 6 predefined workloads (uniform, zero, alternating endpoints, boundary concentrated, slow runs, cycle) | **PASS** |
| `test_06_size_inequality_theorem` | Exhaustive proof check: $L_{\text{B729}}(m) \le L_{\text{T243}}(m) \le L_{\text{B10}}(m)$ for all $1 \le m \le 1024$ | **PASS** |
| `test_07_analytical_table_verification` | Exact Section 9 specification table checks ($k \in \{1, 5, 64, 1024\}$) | **PASS** |
| `test_08_adversarial_invalid_symbols_rejected` | Strict rejection of out-of-range symbols ($<-364, >364$) and invalid types | **PASS** |
| `test_09_adversarial_malformed_frames_and_corruptions` | Strict rejection of truncated headers, indices, payloads, max uint64 counts ($2^{64}-1$), and trailing garbage | **PASS** |
| `test_10_adversarial_t243_forbidden_bytes` | Strict rejection of T243 payload bytes $\ge 243$ | **PASS** |
| `test_11_adversarial_b10_prohibited_states` | Strict rejection of B10 field values $729..1023$ | **PASS** |
| `test_12_adversarial_b729_integer_overflow_and_high_padding` | Strict rejection of B729 integer overflow and nonzero high padding bits | **PASS** |
| `test_13_adversarial_alignment_padding_corruption` | Strict rejection of nonzero alignment gap padding | **PASS** |
| `test_14_adversarial_random_access_bounds` | Strict rejection of out-of-bounds random access queries ($<0, \ge N$) | **PASS** |
| `test_15_known_answer_vectors_and_independent_decoders` | Validation against hard-coded known-answer vectors and independent decoders | **PASS** |
| `test_16_deterministic_operation_count_random_access` | Verification of deterministic bounded-byte touch count for random access | **PASS** |

---

## 4. Unresolved Acceptance Requirements & Phase C Prerequisites

Phase B is **complete**. In accordance with T81 PM directives, **all work stops here**.

### Remaining Phase C Prerequisites:
1. **Pre-Registration of Primary Configuration:**
   - Formal maintainer sign-off on **one** primary evaluation tuple: $(\text{workload}, N, k, A, \text{access\_mode})$.
   - Elimination of exploratory grid hunting or selective post-hoc slicing.
2. **Deterministic Toolchain & Environment Pinning:**
   - Hardware runner characterization: exact CPU architecture, core topology, cache hierarchy, OS kernel, and CPU governor/power profile.
   - Pinned compiler toolchain, optimization flags (`-O3`), and memory allocation instrumentation.
3. **Statistical Protocol & Uncertainty Formulation:**
   - Sample size $n$ and independent process-level repetition budget pre-registered.
   - Formal estimand definition (ratio of population medians of throughput).
   - Pre-declared noninferiority margin (e.g., lower bound $\ge 0.95$) and bootstrap confidence interval procedures.
4. **Hardware Performance Monitoring (PMU) Plan:**
   - Explicit instrumentation of hardware performance counters (instructions, cycles, L1/LLC misses, branch mispredictions).
   - Pre-validation of PMU access without privileged virtualization noise.
5. **Human Maintainer Authorization:**
   - Formal sign-off on the Phase C pre-registration artifact prior to executing timed benchmarks.
