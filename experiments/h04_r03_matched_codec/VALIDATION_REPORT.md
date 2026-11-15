# EXP-H04-R03 Phase B Validation Report

**Experiment:** EXP-H04-R03 (Matched Codec Space-Time Tradeoff & Bounded-Access Discriminator)
**Phase:** Phase B — Deterministic Correctness & Reference Implementation
**Status:** COMPLETE (Deterministic Correctness Passed; Performance Benchmarking Strictly Stopped; Phase B Status: **ACCEPTED — DETERMINISTIC CORRECTNESS VERIFIED**)
**Repository Baseline Commit SHA:** `e108fdad078f4d6dfcc427f1fd419ecbe48b4e05`
**Execution Date:** 2026-11-01
**Authority:** Bounded implementation and validation only. No merge, performance benchmarking, or production activation.

---

## 1. Executive Summary & Response to Review Findings

This report documents the deterministic reference implementations for EXP-H04-R03 Phase B and details the exact corrections applied in response to the three review findings:

### 1. Bounded Byte Access in Random Access
- **Previous defect:** `random_access()` copied/sliced the entire containing block before extracting a symbol.
- **Correction:** `random_access()` in `codec.py` now slices directly from `blob` without allocating or copying whole blocks:
  - **B10:** Slices at most 3 payload bytes directly from `blob[block_offset + start_byte : block_offset + start_byte + 3]`.
  - **T243 & B243-CLONE:** Slices at most 2 payload bytes directly from `blob[block_offset + start_byte : block_offset + end_byte + 1]`.
  - **B729:** Retains bounded target-block decode (since finite block integer conversion requires the block bytes), strictly bounded by block size $k$.
- **Verification:** Verified via `test_16_deterministic_operation_count_random_access` with an instrumented buffer verifying that touched payload bytes for B10 ($\le 3$) and T243 ($\le 2$) are bounded and strictly independent of sequence length $N$ (comparing $N=10$ and $N=1000$).

### 2. Pre-Allocation Exact Canonical Framing
- **Previous defect:** Frames with malformed or truncated payloads but declared large element counts $N$ could trigger index parsing or allocations before detecting wire mismatches.
- **Correction:** Implemented `compute_expected_total_bytes()` in $O(1)$ space and time. `decode()` validates that `len(blob)` matches the exact expected canonical total length *before* calling `compute_layout()` or allocating/parsing index tables.
- **Verification:** Verified via `test_09_adversarial_malformed_frames_and_corruptions`, explicitly asserting rejection of:
  - $N = 2^{64}-1$ maximum uint64 count before allocation.
  - Inflated declared counts on truncated blobs ($N=10^7$ with 20-byte blob).
  - Malformed $N=10000, k=1, A=64$ examples: RA-FIXED 7,508-byte frame (expected 640,064 bytes) and RA-INDEXED 80,064-byte frame (expected 720,064 bytes) rejected immediately with zero allocation.

### 3. Independent Asymmetric Witnesses & Digit-Order Mutation Check
- **Previous defect:** Known-answer vectors were symmetric/palindromic, which would not detect a paired reversal of six-trit digit order.
- **Correction:** Created `experiments/h04_r03_matched_codec/independent_witness.py` with standalone decoders and asymmetric known-answer vectors, including $x = -363 \implies u = 1$ (digits $[1, 0, 0, 0, 0, 0]$, whose palindromic reversal is $[0, 0, 0, 0, 0, 1] \implies u = 243 \implies x = -121$) and $x = 10 \implies u = 374$ ($[2, 1, 2, 1, 1, 1]$).
- **Verification:** Verified via `test_15_known_answer_vectors_and_independent_decoders` and `test_17_adversarial_digit_order_mutation_rejection` demonstrating that a paired digit-order mutation fails the independent witness.

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
  - Phase B status is designated: **ACCEPTED — DETERMINISTIC CORRECTNESS VERIFIED** (per independent PM acceptance review).

---

## 3. Validation Results and Test Accounting

### Test Execution Command
```bash
python3 experiments/h04_r03_matched_codec/test_matched_codec.py
```

### Results Summary
- **Total Test Cases:** 17
- **Passed:** 17
- **Failed:** 0
- **Skipped:** 0
- **Duration:** 1.46s

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
| `test_09_adversarial_malformed_frames_and_corruptions` | Strict rejection of truncated headers, indices, payloads, max uint64 ($2^{64}-1$), and malformed N=10,000 k=1 A=64 frames | **PASS** |
| `test_10_adversarial_t243_forbidden_bytes` | Strict rejection of T243 payload bytes $\ge 243$ | **PASS** |
| `test_11_adversarial_b10_prohibited_states` | Strict rejection of B10 field values $729..1023$ | **PASS** |
| `test_12_adversarial_b729_integer_overflow_and_high_padding` | Strict rejection of B729 integer overflow and nonzero high padding bits | **PASS** |
| `test_13_adversarial_alignment_padding_corruption` | Strict rejection of nonzero alignment gap padding | **PASS** |
| `test_14_adversarial_random_access_bounds` | Strict rejection of out-of-bounds random access queries ($<0, \ge N$) | **PASS** |
| `test_15_known_answer_vectors_and_independent_decoders` | Validation against asymmetric known-answer vectors and independent decoders | **PASS** |
| `test_16_deterministic_operation_count_random_access` | Verification of deterministic bounded-byte touch count for random access | **PASS** |
| `test_17_adversarial_digit_order_mutation_rejection` | Rejection of six-trit digit-order reversal mutation by independent witness | **PASS** |

---

## 4. Diagnostic Audit of Repository Baseline CI Failures

The GitHub Actions CI check suite reported 12 failed check runs on the PR merge branch. A comprehensive root cause analysis was conducted across runner logs to determine their relationship to this PR:

1. **`build / sanitizers` (cancelled at 30m timeout):**
   - The sanitizer build timed out after running 367 tests successfully up to `t81_radix_comparison_test`. Prior to timeout, test 294 (`t81_cli_contract_test_runner`) failed with: `test check failed: (fs::exists(safetensors_fixture)) at tests/cpp/cli_contract_test.cpp:491`.
2. **`cxx-std / linux-x86_64 / clang / {ON, OFF}` (exit code 8):**
   - Both jobs completed 409 of 410 CTest suites (99% pass rate), with the single failure being test 294 (`t81_cli_contract_test_runner`) due to the missing fixture `models/tiny-random-llama-exported.safetensors` on lines 491 and 524 of `tests/cpp/cli_contract_test.cpp`.
3. **`gate / tritwise-determinism / avx2-asan` (exit code 1):**
   - Failed during compilation of GoogleBenchmark under GCC 15.2.0: `_deps/googlebenchmark-src/src/benchmark_register.cc` triggered `-Werror=maybe-uninitialized` from `<regex>` headers in GCC 15 (`/usr/include/c++/15/bits/std_function.h:407:42: error: '*(std::function<bool(char)>*)((char*)&__tmp + ...).std::function<bool(char)>::_M_invoker' may be used uninitialized`).
4. **`build / windows-x86_64 / {msvc, clang-cl}` (exit code 1):**
   - Both Windows jobs failed in CMake generation at `src/direct_api/CMakeLists.txt:26 (add_executable)`: `Cannot find source file: D:/a/t81-foundation/t81-foundation/development_tests/test_real_direct_api.cpp`.
5. **`build / linux-x86_64 / gcc` (exit code 1):**
   - Failed at `scripts/ci/run_model_artifact_review_smoke.sh`: `error: missing source fixture at models/tiny-random-llama/model.safetensors`.
6. **`build / linux-arm64 / clang` & `build / linux-x86_64 / clang` (exit code 8):**
   - Completed 409 of 410 tests (99% pass rate), failing only on test 294 (`t81_cli_contract_test_runner`) due to the missing `models/` safetensors fixture.
7. **`build / macos-arm64 / clang` (exit code 1):**
   - Build failed in `experiments/radix_comparison/radix_comparison.hpp:43` because `experiments/radix_comparison` does not specify C++17 in CMake, causing default C++98/03 mode to reject `constexpr`, `enum class`, and `std::tuple`.
8. **`build / macos-x86_64 / gcc` (exit code 1):**
   - Linker failure on Darwin: GCC on macOS linking against Homebrew libc++ vs libstdc++ with undefined symbols `std::__1::basic_string...`.
9. **`quality gate / required` (exit code 1):**
   - Aggregate gate job aggregating the above pre-existing check results.

**Conclusion:**
Every failing check run in the CI suite is a pre-existing defect on `origin/main` (`e108fda`):
- None of the failures involve Python, the Research module, or `experiments/h04_r03_matched_codec/`.
- EXP-H04-R03 deterministic correctness tests run 100% cleanly (17/17 PASS, 0 failures, 0 skips).
- Per the continuity charter, T81 PM directives, and ADM governance, Phase B changes are strictly isolated to `experiments/h04_r03_matched_codec/` and do not alter unrelated core C++ targets or test fixtures.

---

## 5. Phase C Prerequisites (Pre-Registration & Gates)

Phase B deterministic correctness is **ACCEPTED** (per independent PM review on PR #658). In accordance with T81 PM directives, **all work stops here**.

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
