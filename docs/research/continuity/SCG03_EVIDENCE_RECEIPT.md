# T81 SCG-03 Evidence Receipt — Correctness Repair for C05 and C11

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 SCG-03 Evidence Receipt — Correctness Repair for C05 and C11](#t81-scg-03-evidence-receipt-—-correctness-repair-for-c05-and-c11)
  - [1. Executive Summary & Root Cause Analysis](#1-executive-summary-&-root-cause-analysis)
    - [1.1 Contradiction C05: Intermediate Truncation and False Overflow in `T81Int<N>` Multiplication](#11-contradiction-c05-intermediate-truncation-and-false-overflow-in-`t81int<n>`-multiplication)
    - [1.2 Contradiction C11: Ineffective Assertion-Based Test Verdicts Under Release Builds](#12-contradiction-c11-ineffective-assertion-based-test-verdicts-under-release-builds)
    - [1.3 Post-Merge Formatting Reconciliation](#13-post-merge-formatting-reconciliation)
  - [2. Inventory of Changed Files and Cryptographic Hashes](#2-inventory-of-changed-files-and-cryptographic-hashes)
  - [3. Verification Evidence](#3-verification-evidence)
    - [3.1 CTest & Regression Suite Execution](#31-ctest-&-regression-suite-execution)
    - [3.2 Injected-Failure Verification](#32-injected-failure-verification)
    - [3.3 GitHub Actions CI Results & Post-Merge Observations](#33-github-actions-ci-results-&-post-merge-observations)
  - [4. Constraint & Non-Regression Attestation](#4-constraint-&-non-regression-attestation)

<!-- T81-TOC:END -->


**Document:** `docs/research/continuity/SCG03_EVIDENCE_RECEIPT.md`
**Date:** 2026-11-08
**Task Anchor:** SCG-03 Correctness Repair for C05 and C11 (Post-Merge Reconciliation)
**Authoritative Baseline:** Current `origin/main` pinned to `2c3501e2807bb893076c5037231c3126b7f2bc72`
**Base Commit SHA:** `2c3501e2807bb893076c5037231c3126b7f2bc72` (merged PR #729)
**Overall Disposition:** **READY FOR INDEPENDENT REVIEW**
**Phase C Authorization Status:** **STRICTLY FROZEN / UNAUTHORIZED**
**Verification Label:** **AUTHOR-REPORTED (EXECUTED IN-ENVIRONMENT BY JULES; NOT INDEPENDENT THIRD-PARTY REPRODUCTION)**

---

## 1. Executive Summary & Root Cause Analysis

This evidence receipt documents the corrective implementation resolving contradiction items C05 and C11 from the SCG-01 contradiction register and SCG-02 reproduction report, reconciled atop merged PR #729 (`2c3501e2807bb893076c5037231c3126b7f2bc72`).

### 1.1 Contradiction C05: Intermediate Truncation and False Overflow in `T81Int<N>` Multiplication
- **Root Cause:** In `include/t81/types/T81Int.hpp`, `operator*` formerly evaluated partial products via in-place shift-and-add on `T81Int<N>` (`T81Int tmp = a; tmp <<= i;`). The shift operator `shift_left(k)` dropped high trits beyond `N`. Consequently:
  - For representable products such as `T81Int<2>(2) * T81Int<2>(2) == 4` (representable in $N=2$ $[-4, 4]$), shifted partial product $2 \ll 1$ truncated high trits, turning $+6$ into $-3$ and throwing a false `std::overflow_error`.
  - For unrepresentable products such as `T81Int<2>(3) * T81Int<2>(3) == 9` (unrepresentable in $N=2$ $[-4, 4]$), shifted partial products truncated to $0$, silently returning $0$ instead of detecting overflow.
- **Repair:** Implemented full $2N$-trit convolution accumulation into `std::array<std::int64_t, 2 * kNumTrits> conv{}`. Each position $k$ accumulates at most $N$ products of trits in $\{-1, 0, 1\}$, so $|conv[k]| \le N \le 2048$. Carries are fully propagated across all $2N$ positions into `std::array<std::int8_t, 2 * kNumTrits> prod_digits{}` to normalize the complete mathematical product before checking whether the final value fits the signed $N$-trit range.
- **Overflow Criterion:** If any normalized digit at $k \ge N$ is nonzero, or if residual carry-out is nonzero, `axion::trap_overflow("T81Int overflow")` is invoked, preserving standard `std::overflow_error` behavior.
- **Constexpr & Safety:** Multiplication remains `constexpr`, incurs zero dynamic allocation, and eliminates signed integer undefined behavior.

### 1.2 Contradiction C11: Ineffective Assertion-Based Test Verdicts Under Release Builds
- **Root Cause:** In `tests/cpp/test_bitwise.cpp` and `tests/cpp/test_tritwise_backend_equivalence.cpp`, verification checks used `<cassert>` `assert(...)`, which compile away to no-ops when built under `-DNDEBUG` (Release builds).
- **Repair:** Replaced all test assertions with always-active fail-closed checks (`require(...)` calling `std::abort()` on false condition).
- **Injected Failure Verification:** Verified with injected failures under `-DNDEBUG` that `require(...)` actively aborts (SIGABRT, exit code 134), while preserving all existing effective checks.

### 1.3 Post-Merge Formatting Reconciliation
- Reconciled clang-format-18 compliance across all modified C++ files (`include/t81/types/T81Int.hpp`, `tests/cpp/test_t81int.cpp`, `tests/cpp/test_bitwise.cpp`, and `tests/cpp/test_tritwise_backend_equivalence.cpp`).
- Verified via `clang-format-18 --dry-run --Werror` that zero formatting violations exist across all touched source and test files.
- Verified via `git diff -w` that modifications are strictly formatting/whitespace adjustments with zero semantic changes.

---

## 2. Inventory of Changed Files and Cryptographic Hashes

| File | Change Description | SHA-256 Digest |
|---|---|---|
| `include/t81/types/T81Int.hpp` | Repaired `operator*` via full $2N$-trit convolution and balanced ternary normalization; clang-formatted | `42a666a422d10e157b5a9f508c854182478f8d6cef80722a322c78c95913256b` |
| `tests/cpp/test_bitwise.cpp` | Replaced `assert()` with always-active fail-closed `require()`; clang-formatted | `286019d1e392826786c8cc84b792eac3493425992d5ceb2cc0781b9bb2d0dd2d` |
| `tests/cpp/test_t81int.cpp` | Added C05 regression cases, boundary tests, exhaustive $N=1..4$ oracle checks, and `consteval` static_assert; clang-formatted | `43bdbed129af3804e4058e527dcc72eda3b7a6adfe6e31cce4c433f4ba2ed5f4` |
| `tests/cpp/test_tritwise_backend_equivalence.cpp` | Replaced all test `assert()` calls with always-active `require()`; clang-formatted | `88c1110b56d7a698724918856dbf353bf0d7eeb95068e55452a2c844f8907016` |
| `docs/research/continuity/SCG03_EVIDENCE_RECEIPT.md` | This SHA-pinned evidence receipt | Pinned upon commit |

---

## 3. Verification Evidence

### 3.1 CTest & Regression Suite Execution
All affected correctness tests pass without regressions:
- `t81_t81int_test`: PASS (INT64_MIN, INT64_MAX, basic arithmetic, C05 regressions, exhaustive $N=1..4$ small-width checks against exact-integer oracle, compile-time `consteval` static assertion).
- `test_bitwise`: PASS (BitAnd, BitOr, BitXor, BitNot, BitShl, BitShr, BitUShr with always-active verdicts).
- `t81_tritwise_backend_equivalence_test`: PASS (Scalar vs SWAR vs SIMD backend equivalence across all sizes, TXor fallback routing, public in-place semantics with always-active verdicts).
- `t81_t81int_to_binary_test`: PASS.
- `t81_t81int_overflow_test`: PASS.
- `t81_t81int_generic_div_test`: PASS.
- `t81_int_sign_trit_test`: PASS.
- `t81_int_shift_test`: PASS.
- `check_tisc_freeze_integrity.py`: PASS (209 opcodes verified, reserved range clean).
- EXP-H04-R03 Python regression suite (82 tests): PASS (82/82 OK).

### 3.2 Injected-Failure Verification
- Tested injected failure under explicit `-DNDEBUG` compilation:
  ```
  FATAL: injected failure under NDEBUG
  Aborted (core dumped) -> exit code 134
  ```
  Confirms that test verdicts are guaranteed active in both Debug and Release configurations.

### 3.3 GitHub Actions CI Results & Post-Merge Observations
In the post-merge CI workflow runs on `main` following commit `2c3501e2807bb893076c5037231c3126b7f2bc72`, two non-passing jobs were evaluated and classified:
- **CodeQL autobuild failure:** Attributed to external network dependency timeout during CMake fetch of FTXUI (`HTTP 504 Gateway Timeout` when fetching archive `https://github.com/ArthurSonzogni/FTXUI`). The build environment failed before reaching source compilation.
- **Documentation link (lychee) failure:** Attributed to external HTTP 503 errors when crawling status history links pointing to transient GitHub Actions runs (`HTTP 503 Service Unavailable` on run `39644888844`).
- **Classification:** Both failures are classified as external infrastructure-related events. Neither check is represented as passing, and their root causes are documented here as environmental external factors rather than functional or arithmetic defects.

---

## 4. Constraint & Non-Regression Attestation

- **TISC Frozen Semantics:** Zero modification to opcode registry (`spec/tisc/opcode-registry.md`) or TISC state transitions.
- **API Signatures:** Public C++ exception APIs preserved; no conversion to `Result`.
- **Refactoring:** Scope strictly isolated to `include/t81/types/T81Int.hpp`, `tests/cpp/test_bitwise.cpp`, `tests/cpp/test_t81int.cpp`, `tests/cpp/test_tritwise_backend_equivalence.cpp`, and this receipt.
- **Benchmarking & Phase C:** Strictly disabled and frozen.
- **Review Boundary:** This evidence receipt is author-reported (executed in-environment by Jules) and explicitly prepared for independent review.
