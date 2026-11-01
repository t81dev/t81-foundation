# EXP-H04-R03 Phase B Validation Report

**Experiment:** EXP-H04-R03 (Matched Codec Space-Time Tradeoff & Bounded-Access Discriminator)
**Phase:** Phase B — Deterministic Correctness & Reference Implementation
**Status:** COMPLETE (Deterministic Correctness Passed; Performance Benchmarking Strictly Stopped)
**Repository Baseline Commit SHA:** `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`
**Execution Date:** 2026-11-01
**Authority:** Bounded implementation and validation only. No merge, performance benchmarking, or production activation.

---

## 1. Executive Summary

Phase B implements deterministic reference codecs for the four matched encoding arms over the identical 729-state domain `[-364, 364]`:
1. **B10**: Fixed-width 10-bit binary packing (baseline fixed binary).
2. **T243**: Explicitly constructed Base-243 packing (5 trits per byte in ordinary base-3).
3. **B243-CLONE**: Binary software implementation of the identical Base-243 wire format (constructive equivalence witness).
4. **B729**: Finite-block integer encoding using arbitrary-precision integers ($k \in \{1, 5, 64, 1024\}$).

All arms adhere to canonical framing: 8-byte uint64 count header, optional $B \times 8$-byte little-endian block offset index (RA-INDEXED), alignment padding for $A \in \{1, 8, 64\}$, and random-access query capability without decoding preceding blocks.

Deterministic regression and adversarial validation suites completed with **100% pass rate** (14/14 tests passed, 0 failures, 0 skips).

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
- **Total Test Cases:** 14
- **Passed:** 14
- **Failed:** 0
- **Skipped:** 0
- **Duration:** 1.78s

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
| `test_09_adversarial_malformed_frames_and_corruptions` | Strict rejection of truncated headers, indices, payloads, and trailing garbage | **PASS** |
| `test_10_adversarial_t243_forbidden_bytes` | Strict rejection of T243 payload bytes $\ge 243$ | **PASS** |
| `test_11_adversarial_b10_prohibited_states` | Strict rejection of B10 field values $729..1023$ | **PASS** |
| `test_12_adversarial_b729_integer_overflow_and_high_padding` | Strict rejection of B729 integer overflow and nonzero high padding bits | **PASS** |
| `test_13_adversarial_alignment_padding_corruption` | Strict rejection of nonzero alignment gap padding | **PASS** |
| `test_14_adversarial_random_access_bounds` | Strict rejection of out-of-bounds random access queries ($<0, \ge N$) | **PASS** |

---

## 4. Phase C Prerequisites (Pre-Registration & Gates)

Phase B is **complete**. In accordance with T81 PM directives, **all work stops here**. Phase C (controlled performance benchmarking) requires separate program-management authorization and completion of the following prerequisites:

1. **Pre-Registration of Primary Configuration:**
   - Explicit selection of **one** primary evaluation tuple: $(\text{workload}, N, k, A, \text{access\_mode})$.
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
