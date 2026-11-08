# T81 SCG-06: Independent Arithmetic Defect Reproduction and Regression Attribution Report

**Document ID:** `T81-SCG-06-REPRODUCTION-001-R1`
**Date:** 2026-11-08
**Task Classification:** READ-ONLY REPRODUCTION AND ROOT-CAUSE ASSESSMENT
**Source Issue:** [Issue #717](https://github.com/t81dev/t81-foundation/issues/717)
**Pre-Repair Baseline:** `64941caff9372f2e76dd3a0fb79aa6711a48b171` (tree `0af4d7d76c43f117a0e3cb0ab534cb19a712f743`, merged PR #727)
**Post-Repair Baseline:** `2c3501e2807bb893076c5037231c3126b7f2bc72` (tree `47672f37dbb568297cd9b11173fd838a410428f4`, merged PR #729)
**Authoritative Final Disposition:** **SCG-06 — REPRODUCTION COMPLETE / REPAIR NOT AUTHORIZED**
**Phase C Authorization Status:** **STRICTLY FROZEN / UNAUTHORIZED**
**Execution Context & Review Boundary:** Executed in-environment by Jules in an isolated Linux x86_64 container. Codex review did not complete due to platform usage limits; this report represents author-reported empirical evidence prepared for maintainer review.
**Verification Scope:** Mathematical and executable verification of defects F1–F7 across immutable git baselines in an isolated Linux x86_64 environment. Zero production source files, normative specifications, or TISC opcode registry definitions were modified.

---

## 1. Executive Summary

This report delivers an independent, empirical assessment of the seven arithmetic defects F1–F7 reported in Issue #717, evaluating both the pre-SCG-03 baseline (`64941caf`) and the post-SCG-03 baseline (`2c3501e2`).

### Key Findings
1. **Defects Corrected by SCG-03:**
   - **F4 (`T81Int<N>::operator*` silent overflow to 0):** **CONFIRMED FIXED**. SCG-03 (resolving C05) replaced the truncating shift-and-add implementation with a full $2N$-trit convolution accumulation and balanced ternary carry normalization. Overflow is reliably detected and traps via `axion::trap_overflow` (throwing `std::overflow_error`), matching specification requirements.
2. **Defects with Failure-Mode Change:**
   - **F6 (`T81Fraction<81>` numerator overflow):** **CONFIRMED FAILURE-MODE CHANGE**. Under pre-SCG-03, multiplying fractions whose mathematical product exceeds 81 trits (e.g. $3^{45} \times 3^{45} = 3^{90}$) resulted in silent data corruption (the numerator wrapped modulo $3^{81}$ to 0). Under post-SCG-03, `T81Int::operator*` correctly detects overflow and throws `std::overflow_error`. However, because `T81Fraction::operator*` is explicitly marked `noexcept`, the thrown exception escapes a `noexcept` boundary, immediately invoking `std::terminate()` (process abort with SIGABRT, exit code 134). While the arithmetic corruption is halted, process-level survivability is compromised. This is a confirmed failure-mode change, not an automatically proven new arithmetic defect.
3. **Defects Remaining Unresolved (Unchanged):**
   - **F1 (`T81Limb27` Booth table scaling):** **CONFIRMED UNCHANGED**. In `build_booth27_table()`, patterns $\pm 2$ and $\pm 4$ assign `shift_trytes = 1` (a tryte shift equals 3 trits, or scaling by $3^3 = 27$), mis-scaling low-magnitude patterns by a factor of 27 instead of multiplying by 2 or 4.
   - **F2 (`T81Limb54::reference_mul` non-trits):** **CONFIRMED UNCHANGED**. Schoolbook multiplication over 54 trits yields accumulators up to $\pm 54$. The single-pass carry correction only handles $|s| \le 4$, leaving invalid ternary digits (e.g. $\pm 3$) in 300 out of 300 pseudo-random test cases, and failing exact-integer product comparison in 300/300 cases.
   - **F3 (`T81Limb54::karatsuba` carry discard):** **CONFIRMED UNCHANGED**. Karatsuba radical sum $x_0 + x_1$ produces a 28th-trit carry in `addc()`. However, `T81Limb27::operator+` discards the carry, causing intermediate carry loss in 100 out of 300 pseudo-random cases, and end-to-end product mismatch against the exact-integer oracle in 300 out of 300 cases.
   - **F5 (`T81BigInt` subtraction termination):** **CONFIRMED UNCHANGED**. In Debug builds, subtracting single-limb values where the subtrahend is larger sets `negative_ = false` and calls `normalize()`, triggering `verify_invariants()` which terminates the process via `std::terminate()` upon encountering a negative most significant limb. In Release builds (`-DNDEBUG`), invariant checks compile away and `to_std_chunks()` throws `std::runtime_error("borrow beyond MSB")`.
   - **F7 (`T81Fraction::to_double()` `noexcept` termination):** **CONFIRMED UNCHANGED**. `to_double()` is marked `noexcept`, but delegates to `num_.to_int64()`. If the numerator exceeds 40 trits, `to_int64()` throws `std::overflow_error`, which aborts the process via `std::terminate()`.
4. **Attribution of GitHub Actions CI Failures:**
   - The nine check failures reported by external reviewers around commit `2c3501e` were conflated across PR head and merge runs. An exhaustive audit reveals exactly 4 independent root failures (1 repair-induced code-hygiene clang-format violation resolved in `b6ed9322`, 2 external network timeouts on CodeQL FTXUI HTTP 504 and lychee HTTP 503, and 1 runner CMake configure failure during FTXUI download in `determinism / linux-x86_64`) plus 1 dependent required quality gate failure, totaling 5 failed jobs on merge commit `2c3501e`. The external reviewer's "nine failing checks" count is formally classified as having an unresolved historical basis.

---

## 2. Baseline & Environment Inventory

All reproduction tests were executed in an isolated, immutable worktree environment without touching the working branch tree.

| Property | Value |
|---|---|
| Operating System | Linux (Ubuntu 24.04 LTS container) |
| Kernel Version | 6.8.0 #1 SMP PREEMPT_DYNAMIC Fri Feb 20 20:38:43 UTC 2026 x86_64 |
| Primary C++ Compiler | `g++ (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0` |
| Secondary C++ Compiler | `Ubuntu clang version 18.1.3 (1ubuntu1)` |
| Python Environment | Python 3.12.13 |
| Pre-Repair Baseline Commit | `64941caff9372f2e76dd3a0fb79aa6711a48b171` (Tree: `0af4d7d76c43f117a0e3cb0ab534cb19a712f743`) |
| Post-Repair Baseline Commit | `2c3501e2807bb893076c5037231c3126b7f2bc72` (Tree: `47672f37dbb568297cd9b11173fd838a410428f4`) |
| Automated Bootstrap Script | `docs/research/continuity/evidence/scg06/run_all_reproductions.sh` |

---

## 3. Independent Mathematical Oracles & Reproduction Methodology

### 3.1 Independent Oracle Principles
To prevent circular testing traps (where two defective T81 routines validate each other, as identified in Issue #717 regarding `booth_mul` and `reference_mul`), all expectations were computed using:
1. **Python Arbitrary-Precision Exact Integer Oracle (`oracle.py`):**
   - Pure Python exact mathematical integers ($a \times b$, $a - b$, $a + b$).
   - Exact conversion functions between arbitrary integers and balanced ternary trit arrays $(\sum t_i 3^i)$ and tryte arrays $(\sum y_i 27^i)$ with rigorous range and carry invariants.
2. **C++ Independent Exact-Integer Oracles:**
   - Exhaustive comparison against 64-bit and 128-bit native arithmetic with explicit mathematical bounds checking.

### 3.2 Process-Safe Subprocess Harness (`test_runner.py` / `reproduce_f1_f7.py`)
Because F5, F6, and F7 can trigger fatal signals (`SIGABRT`) via `std::terminate()`, all test cases were executed inside isolated subprocesses with:
- 15-second bounded execution timeouts.
- Capture of exit code, termination signal, `stdout`, and `stderr`.
- Dual execution across Debug builds (assertions and `verify_invariants()` active) and Release builds (`-DNDEBUG` active).

---

## 4. Findings F1–F7 Evidence Matrix

| Finding | Component | Pre-SCG-03 (`64941caf`) | Post-SCG-03 (`2c3501e2`) | Classification | Primary Evidence Digest |
|---|---|---|---|---|---|
| **F1** | `T81Limb27` | Mis-scales pattern $\pm 2, \pm 4$ by 27; $1 \times 2 \to 27$ | Mis-scales pattern $\pm 2, \pm 4$ by 27; $1 \times 2 \to 27$ | **CONFIRMED UNCHANGED** | `detailed_counterexamples.json` (F1) |
| **F2** | `T81Limb54` | Emits non-trits ($\pm 3$); 300/300 fail exact oracle | Emits non-trits ($\pm 3$); 300/300 fail exact oracle | **CONFIRMED UNCHANGED** | `reproduction_results.json` (F2) |
| **F3** | `T81Limb54` | Discards carry in $x_0 + x_1$ (100/300); 300/300 fail oracle | Discards carry in $x_0 + x_1$ (100/300); 300/300 fail oracle | **CONFIRMED UNCHANGED** | `reproduction_results.json` (F3) |
| **F4** | `T81Int<N>` | $243 \times 243$ silently overflows to 0 | Traps with `std::overflow_error: T81Int overflow` | **CONFIRMED FIXED** | `reproduction_results.json` (F4) |
| **F5** | `T81BigInt` | Debug: `std::terminate()`<br>Release: `borrow beyond MSB` | Debug: `std::terminate()`<br>Release: `borrow beyond MSB` | **CONFIRMED UNCHANGED** | `reproduction_results.json` (F5) |
| **F6** | `T81Fraction<81>` | $(3^{45}) \times (3^{45}) \to 0$ (silent wrap) | Throws into `noexcept` $\to$ `std::terminate()` | **CONFIRMED FAILURE-MODE CHANGE** | `reproduction_results.json` (F6) |
| **F7** | `T81Fraction` | $> 40$ trits throws in `noexcept` $\to$ `std::terminate()` | $> 40$ trits throws in `noexcept` $\to$ `std::terminate()` | **CONFIRMED UNCHANGED** | `reproduction_results.json` (F7) |

---

## 5. Detailed Reproduction Analysis

### 5.1 Finding F1: `T81Limb27` Booth Table Scaling
- **Target File:** `include/t81/types/T81Limb.hpp:108` (`build_booth27_table`) and `line 771` (`operator*`).
- **Mathematical Mechanism:**
  The Booth table precomputes decisions based on tryte pair `b0 + 3*b1`:
  ```cpp
  if (pattern == 1 || pattern == 3) {
    mul = 1;
  } else if (pattern == 2 || pattern == 4) {
    mul = 1;
    shift = 1;
  }
  ```
  In `T81Limb27::operator*`, the multiplier shifts trytes: `shift_trits = decision.shift_trytes * 3`.
  When `pattern == 2`, `shift_trytes` is set to `1` (which shifts by 3 trits = multiplication by $3^3 = 27$).
  Thus, multiplying $1 \times 2$ evaluates as:
  $$\text{Observed} = 1 \times 27^1 = 27$$
  $$\text{Expected} = 1 \times 2 = 2$$
- **Reproduced Counterexample:**
  Inputs: $A = [1, 0, \dots, 0]$ (value 1), $B = [2, 0, \dots, 0]$ (value 2).
  Observed output trits: `[0, 0, 0, 1]` (integer 27). Expected output trits: `[-1, 1, 0, 0]` (integer 2).
- **Classification:** **CONFIRMED UNCHANGED**. SCG-03 did not touch `T81Limb.hpp`.

### 5.2 Finding F2: `T81Limb54::reference_mul` Carry Normalization
- **Target File:** `include/t81/types/T81Limb.hpp:515` (`reference_mul`).
- **Mathematical Mechanism:**
  Schoolbook multiplication accumulates products of 54 trits. Accumulator entries reach $\pm 54$.
  The normalization loop performs:
  ```cpp
  int sum = accum[i] + carry;
  if (sum >= 2)       { accum[i] = sum - 3; carry =  1; }
  else if (sum <= -2) { accum[i] = sum + 3; carry = -1; }
  else                { accum[i] = sum;     carry =  0; }
  ```
  This is a single-step subtraction. If $\text{accum}[i] = 54$ and $\text{carry} = 0$, $\text{accum}[i]$ becomes $51$ and $\text{carry}$ becomes $1$. Subsequent passes do not reduce $51$.
  When encoded back into trits via `int8_t` chunks, invalid digit values (such as $\pm 3, \pm 5, \dots$) are emitted directly into the result array.
- **Reproduced Counterexample & Exact Oracle Comparison:**
  Across 300 deterministic pseudo-random 54-trit inputs evaluated against the independent exact-integer oracle (`oracle.py`), 300/300 produced invalid non-trit output digits (max absolute digit observed = 3), and 300/300 failed exact-integer mathematical product equivalence.
- **Classification:** **CONFIRMED UNCHANGED**.

### 5.3 Finding F3: `T81Limb54::karatsuba` Intermediate Carry Discard and Product Equivalence
- **Target File:** `include/t81/types/T81Limb.hpp:894` (`karatsuba`).
- **Mathematical Mechanism:**
  Karatsuba multiplication splits 54 trits into two 27-trit halves $x_0, x_1$ and evaluates:
  $$\text{mid} = (x_0 + x_1) \times (y_0 + y_1)$$
  Adding two 27-trit limbs can produce a 28th-trit carry: $x_0 + x_1 \in [-(3^{27}-1), 3^{27}-1]$.
  However, `T81Limb27::operator+` is implemented as:
  ```cpp
  [[nodiscard]] constexpr T81Limb27 operator+(const T81Limb27& other) const noexcept {
    return addc(other).first;
  }
  ```
  The carry-out (`addc().second`) is silently discarded.
- **Reproduced Counterexample & Exact Oracle Comparison:**
  In 100 out of 300 pseudo-random cases, $x_0 + x_1$ produced a non-zero carry-out, confirming intermediate carry loss. In 300 out of 300 cases, end-to-end `karatsuba()` execution failed exact-integer equivalence against the independent oracle.
- **Classification:** **CONFIRMED UNCHANGED**.

### 5.4 Finding F4: `T81Int<N>::operator*` Overflow Handling
- **Target File:** `include/t81/types/T81Int.hpp:404`.
- **Pre-SCG-03 Behavior:**
  Multiplication used shift-and-add:
  ```cpp
  T81Int tmp = a;
  tmp <<= i;
  if (tb == Trit::P) r += tmp;
  ```
  The shift operator truncated trits $> N$, silently wrapping intermediate calculations modulo $3^N$.
  In `T81Int<10>`, $243 \times 243 = 59049$. Since $59049 \equiv 0 \pmod{3^{10}}$, the function returned 0 with zero diagnostic.
- **Post-SCG-03 Behavior:**
  SCG-03 replaced this with full $2N$-trit convolution accumulation into `std::array<std::int64_t, 2 * kNumTrits>`, followed by complete carry normalization across all $2N$ digit positions. If any digit at $k \ge N$ is non-zero, it invokes `axion::trap_overflow("T81Int overflow")`.
- **Reproduction Result:**
  Pre-SCG-03: `VAL:0` (exit code 0).
  Post-SCG-03: `TRAP:T81Int overflow` (caught `std::overflow_error`, exit code 0).
- **Classification:** **CONFIRMED FIXED**.

### 5.5 Finding F5: `T81BigInt` Subtraction Termination
- **Target File:** `include/t81/types/T81BigInt.hpp:1040` (`operator+` single-limb fallback).
- **Mathematical Mechanism:**
  When subtracting two single-limb numbers where $a < b$ (e.g. $X = 3^{79}+7, Y = 3^{79}+11$, difference $-4$):
  ```cpp
  Limb rhs = b.negative_ ? -b.limbs_[0] : b.limbs_[0];
  res.limbs_[0] = res.limbs_[0] + rhs;
  res.negative_ = false;
  res.normalize();
  ```
  Here, `res.limbs_[0]` holds the negative difference $-4$. The fallback hardcodes `res.negative_ = false` and calls `normalize()`.
  In `normalize()`, `verify_invariants()` is invoked:
  ```cpp
  if (!limbs_.empty() && limbs_.back().sign_trit() == Trit::N) {
    std::terminate();
  }
  ```
- **Debug vs Release Distinction:**
  - **Debug Build (`NDEBUG` undefined):** `verify_invariants()` is active. It finds `limbs_.back().sign_trit() == Trit::N` and calls `std::terminate()`. The calling `try ... catch (const std::overflow_error&)` cannot intercept `std::terminate()`, causing immediate process abortion with `SIGABRT` (exit code 134).
  - **Release Build (`-DNDEBUG` defined):** `verify_invariants()` compiles away. The negative limb is retained with `negative_ = false`. Later operations (e.g. `to_std_chunks` in division or string formatting) encounter the negative limb and throw:
    `std::runtime_error: to_std_chunks: borrow beyond MSB (input effectively negative)`
- **Classification:** **CONFIRMED UNCHANGED**.

### 5.6 Finding F6: `T81Fraction<81>` Numerator Overflow and Failure-Mode Change
- **Target File:** `include/t81/types/T81Fraction.hpp:148` (`operator*`).
- **Causal Chain & Root Cause:**
  1. `T81Fraction<81>` is defined as `(num_, den_)` where `num_` is `T81Int<81>`.
  2. `T81Fraction::operator*` is defined as:
     ```cpp
     [[nodiscard]] T81Fraction operator*(const T81Fraction& o) const noexcept {
       return T81Fraction(num_ * o.num_, den_ * o.den_);
     }
     ```
     Note the `noexcept` specification.
  3. **Pre-SCG-03:** `T81Int<81>::operator*` truncated overflow silently. Evaluating $(3^{45}) \times (3^{45}) = 3^{90}$ silently wrapped the numerator to 0, producing fraction `0 / 1` without throwing.
  4. **Post-SCG-03:** SCG-03 corrected `T81Int<81>::operator*` so that it calls `axion::trap_overflow("T81Int overflow")`, throwing `std::overflow_error`.
  5. **Failure-Mode Escalation:** Because `T81Fraction::operator*` is declared `noexcept`, the C++ runtime intercepts the uncaught exception at the function boundary and immediately calls `std::terminate()`.
- **Reproduction Result:**
  - Pre-SCG-03: `NORMAL:is_zero=1` (silent corruption, exit code 0).
  - Post-SCG-03: `std::terminate()` called after throwing `std::overflow_error: T81Int overflow` (process abort, `SIGABRT`, exit code 134).
- **Classification:** **CONFIRMED FAILURE-MODE CHANGE**. SCG-03 corrected the underlying integer overflow, but exposed the unsafe `noexcept` contract in `T81Fraction`. This is a failure-mode change, not an automatically proven new arithmetic defect.

### 5.7 Finding F7: `T81Fraction::to_double()` Exception Across `noexcept` Boundary
- **Target File:** `include/t81/types/T81Fraction.hpp:126` (`to_double`).
- **Mathematical Mechanism:**
  ```cpp
  [[nodiscard]] double to_double() const noexcept {
    const double n = static_cast<double>(num_.to_int64());
    const double d = static_cast<double>(den_.to_int64());
    return n / d;
  }
  ```
  `T81Int<81>::to_int64()` throws `std::overflow_error("T81Int::to_int64(): value out of range (trits > 40)")` whenever $|num| > \frac{3^{40}-1}{2}$.
  Because `to_double()` is marked `noexcept`, the exception cannot propagate to caller catch blocks, resulting in an immediate process abort via `std::terminate()`.
- **Reproduction Result:**
  Aborts with `SIGABRT` (exit code 134) under both pre-SCG-03 and post-SCG-03.
- **Classification:** **CONFIRMED UNCHANGED**.

---

## 6. Evaluation of Issue #717 Proposed Remediations

| Finding | Contributor Proposed Repair | Independent Technical Assessment | Implementation Risk | Test Prerequisites |
|---|---|---|---|---|
| **F1** | Fix table shift/mul scaling | In `build_booth27_table()`, replace tryte shift with proper trit weight or replace Booth table with direct balanced ternary multiply-accumulate. | Low. Localized to `T81Limb.hpp`. | Exhaustive $27 \times 27$ test matrix against exact integer oracle. |
| **F2** | Apply multi-pass division by 3 loop | Correct. Carry normalization over arbitrary accumulators requires iterative division by 3 until all trits are in $\{-1, 0, 1\}$. | Low. Improves test stability and eliminates circular assertion bugs. | 1000 randomized 54-trit multiplications checked for valid trit ranges. |
| **F3** | Propagate 28th trit carry in Karatsuba | Karatsuba radical sum requires widening the intermediate limb to 28 trits or handling `addc().second` in high-order accumulation. | Medium. May affect limb alignment or struct sizing. | Test with maximal limb values ($x_0 = x_1 = \text{max}$). |
| **F4** | Add overflow check to `T81Int::operator*` | Already addressed by SCG-03 (C05) using $2N$-trit convolution. | Resolved. | Covered by `test_t81int.cpp`. |
| **F5** | Preserve sign of negative differences in fallback | Correct. In `operator+` single-limb fallback, check `res.limbs_[0].sign_trit() == Trit::N`; if negative, set `res.negative_ = true` and negate the limb before `normalize()`. | Low. Directly prevents the invalid invariant trip. | Subtraction tests where $a < b$ across 40–80 trit boundaries. |
| **F6** | Add bounds check or remove `noexcept` | Removing `noexcept` allows callers to catch `std::overflow_error`. Alternatively, fraction operations should return `T81Result<T81Fraction>` under checked arithmetic. | Medium. Changing `noexcept` affects API contracts and standard library container traits. | Exception-safety tests and overflow regression suite. |
| **F7** | Remove `noexcept` or guard $> 40$ trits | Guard against $> 40$ trits: convert high-magnitude trits via floating-point scale factor (e.g. `std::ldexp`) instead of failing on `to_int64()`. | Medium. Must preserve IEEE-754 precision guarantees without aborting. | Boundary tests with 40, 41, 80, and 81-trit fractions. |

### Highest-Priority Recommended Repair
**Defect F6 / F7 (`T81Fraction` `noexcept` Boundary Safety):**
While F1–F3 corrupt limb-level routines that are currently internal or experimental, **F6 and F7 directly abort production processes** calling standard fraction arithmetic. Specifically, SCG-03's fix for `T81Int` turned F6 into an active process crash. Aligning `T81Fraction`'s exception contract (either by removing `noexcept` or providing checked conversion) is the highest priority for system stability.

---

## 7. Appendix: SCG-03 CI Attribution Boundary Audit

An external review raised questions regarding nine allegedly failing checks around commit `2c3501e`. To provide rigorous transparency, all workflow runs and check-runs on PR #729 head `a35bd4d707b6d1ef4f8d7a65a9e492f4cd1d1962` and merge commit `2c3501e2807bb893076c5037231c3126b7f2bc72` were inspected via GitHub API evidence receipts.

### 7.1 Exhaustive Check Reconciliation Across Both Commits

| Commit | Check Run / Job ID | Check / Job Name | Step Failed | Failure Classification | Root Cause & Evidence |
|---|---|---|---|---|---|
| **PR 729 Head** (`a35bd4d`) | `128719937822` | `format / clang-format` | Step 4: Run clang-format | **Repair-Induced Code-Hygiene** | Formatting violations in `T81Int.hpp` and `test_t81int.cpp`. Completely resolved in subsequent commit `b6ed9322`. |
| **PR 729 Head** (`a35bd4d`) | `128719938100` | `lint / spec & docs` | Step 6: Lychee link check | **External Infrastructure** | Transient HTTP 503 Service Unavailable when resolving transient GitHub Actions URLs in documentation. |
| **PR 729 Head** (`a35bd4d`) | `128720228889` | `quality gate / required` | Gate evaluation | **Dependent Gate** | Failed solely due to dependency on failing job `lint / spec & docs`. |
| **PR 729 Head** (`a35bd4d`) | `128719937819` | `determinism / linux-x86_64` | Step 5: Configure | **External Infrastructure** | CMake configuration failed during third-party FTXUI download with HTTP 504. Zero determinism tests executed. |
| **PR 729 Head** (`a35bd4d`) | `128720571913` | `CodeQL` | Step 4: Autobuild | **External Infrastructure** | FTXUI download HTTP 504 Gateway Timeout during CodeQL Autobuild. |
| **Merge Commit** (`2c3501e`) | `128720301538` | `format / clang-format` | Step 4: Run clang-format | **Repair-Induced Code-Hygiene** | Formatting-only failure on PR #729 changes. Resolved in follow-up commit `b6ed9322`. |
| **Merge Commit** (`2c3501e`) | `128720301959` | `lint / spec & docs` | Step 6: Lychee link check | **External Infrastructure** | Lychee HTTP 503 link checker failure resolving transient GitHub run URLs. |
| **Merge Commit** (`2c3501e`) | `128720364984` | `quality gate / required` | Gate evaluation | **Dependent Gate** | Failed because required job `lint / spec & docs` failed. |
| **Merge Commit** (`2c3501e`) | `128720301336` | `Analyze (cpp)` | Step 4: Autobuild | **External Infrastructure** | FTXUI repository download HTTP 504 timeout during CMake configure. |
| **Merge Commit** (`2c3501e`) | `128720301586` | `determinism / linux-x86_64` | Step 5: Configure | **External Infrastructure** | CMake configure step failed during third-party FTXUI download with HTTP 504. Zero determinism tests executed. |

### 7.2 Deconstruction of the "Nine Failures" Claim
Direct GitHub API check-run and job inspection identifies exactly 5 non-passing checks on PR #729 head (4 failures + 1 neutral) and exactly 5 failed jobs across workflow runs on merge commit `2c3501e`. The external reviewer's count of "nine failing checks" cannot be reconstructed from individual failed jobs alone. Possible origins include:
1. Combining distinct non-passing checks across both commits.
2. Counting skipped/cancelled matrix sub-jobs triggered by dependent failures.
3. Inspecting an intermediate push state prior to head `a35bd4d`.

Because no authoritative 9-check failure ledger exists in the GitHub API record, the "nine failing checks" claim is formally classified as having an **UNRESOLVED historical basis**.

---

## 8. Cryptographic Digests of Evidence Artifacts

All supporting reproduction code, exact oracles, test runners, and raw outputs are archived in `docs/research/continuity/evidence/scg06/`:

```
08d8487d2dd9e721aad1d09aff50d8e2faf2c8f2f17af5be08fa13928bba6761  ci_failure_attribution.json
655e5487fdda54e4bcb23c383e6d754b549431673ac60055d2b287fe89fe0ca0  detailed_counterexamples.json
69fe979fe2ed4d219e3dac56989b7c9dd38c97956c5046bcd1aee0d3dbc26f2b  environment.json
9b9ec1b40cf1d4e2ee3a1b5074023cf76d96bd19d2e0e230871466fd99b5069d  oracle.py
e3095c5807b5f3eebe768e793681c18f87c4630bd4a1a7c4e76b7add8f6364d2  reproduce_f1_f7.py
af1575dd1494c6755e13e375fe7adeee5c014bd29775839ee4c589fb21c0adba  reproduction_results.json
0918a46975ee6f01e8bea0ab983236648328e5802dc02be7f2d8e0911e7fbb2f  run_all_reproductions.sh
229a6ea8d3344a31e7acce7f88815f6f61e4ab3440325f66c6ebb555f125ac4c  test_f1_exact.py
bb639ac2dd5a6d5146cc6de3077f2efe39bc1eec69fb1bcae08ba6a59b7af964  test_runner.py
```

---

## 9. Verification Limitations & Constraints Attestation

- **Read-Only Scope:** Zero lines of production code under `include/`, `src/`, `vm/`, `isa/`, or `lang/` were modified.
- **Specification Invariance:** Zero normative specifications or TISC ISA opcode tables were modified.
- **No Performance Benchmarking:** All evaluations were strictly correctness-oriented. Phase C performance measurement remains frozen.
- **Follow-On Implementation:** Implementation of fixes for F1–F3, F5, F6, and F7 remains strictly unauthorized pending separate maintainer direction.
