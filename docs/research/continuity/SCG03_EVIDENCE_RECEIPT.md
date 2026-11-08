# T81 SCG-03 Evidence Receipt — Correctness Repair for C05 and C11

**Document:** `docs/research/continuity/SCG03_EVIDENCE_RECEIPT.md`
**Date:** 2026-11-08
**Task Anchor:** SCG-03 Correctness Repair for C05 and C11
**Authoritative Baseline:** Current `origin/main` pinned to `64941caff9372f2e76dd3a0fb79aa6711a48b171`
**Base Commit SHA:** `64941caff9372f2e76dd3a0fb79aa6711a48b171`
**Overall Disposition:** **READY FOR INDEPENDENT REVIEW**
**Phase C Authorization Status:** **STRICTLY FROZEN / UNAUTHORIZED**
**Verification Label:** **AUTHOR-REPORTED (EXECUTED IN-ENVIRONMENT BY JULES; NOT INDEPENDENT THIRD-PARTY REPRODUCTION)**

---

## 1. Executive Summary & Root Cause Analysis

This evidence receipt documents the corrective implementation resolving contradiction items C05 and C11 from the SCG-01 contradiction register and SCG-02 reproduction report, pinned to baseline `64941caff9372f2e76dd3a0fb79aa6711a48b171`.

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

---

## 2. Inventory of Changed Files and Cryptographic Hashes

| File | Change Description | SHA-256 Digest |
|---|---|---|
| `include/t81/types/T81Int.hpp` | Repaired `operator*` via full $2N$-trit convolution and balanced ternary normalization | `ccbe34d78d5275500c05c3cf667976439e107eb0a8472628784ccfd69774e551` |
| `tests/cpp/test_bitwise.cpp` | Replaced `assert()` with always-active fail-closed `require()` | `286019d1e392826786c8cc84b792eac3493425992d5ceb2cc0781b9bb2d0dd2d` |
| `tests/cpp/test_t81int.cpp` | Added C05 regression cases, boundary tests, exhaustive $N=1..4$ oracle checks, and `consteval` static_assert | `58b6bcc2c5940b433f0aeb06015bab594fc14f3d5e6b727345fa7f887d09e3b0` |
| `tests/cpp/test_tritwise_backend_equivalence.cpp` | Replaced all test `assert()` calls with always-active `require()` | `88c1110b56d7a698724918856dbf353bf0d7eeb95068e55452a2c844f8907016` |
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

---

## 4. Constraint & Non-Regression Attestation

- **TISC Frozen Semantics:** Zero modification to opcode registry (`spec/tisc/opcode-registry.md`) or TISC state transitions.
- **API Signatures:** Public C++ exception APIs preserved; no conversion to `Result`.
- **Refactoring:** Scope strictly isolated to `include/t81/types/T81Int.hpp`, `tests/cpp/test_bitwise.cpp`, `tests/cpp/test_t81int.cpp`, `tests/cpp/test_tritwise_backend_equivalence.cpp`, and this receipt.
- **Benchmarking & Phase C:** Strictly disabled and frozen.
- **Review Boundary:** This evidence receipt is author-reported and explicitly prepared for independent review.
