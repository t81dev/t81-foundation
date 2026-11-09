# SCG-07 Phase 6: Consolidated Arithmetic Qualification Report

**Document ID:** `T81-SCG-07-PHASE6-QUALIFICATION-001-R1`
**Classification:** INDEPENDENT QUALIFICATION & VERIFICATION ASSESSMENT
**Baseline Anchor Commit:** `ecb1e32b9458a1a1df51d7cdd4a19496c0584103` (`origin/main`)
**Task Classification:** Correctness-Only Independent Subsystem Acceptance
**Status:** COMPLETE — READY FOR MAINTAINER DECISION

---

## 1. Executive Disposition

### **QUALIFIED WITH LIMITATIONS**

The T81 arithmetic subsystem (`T81Int`, `T81Limb27`, `T81Limb54`, `T81BigInt`, and `T81Fraction`) satisfies all required correctness and exception-safety acceptance criteria on the pinned baseline `ecb1e32b9458a1a1df51d7cdd4a19496c0584103`. All historical defect reproducers F1–F7 pass unconditionally when evaluated against an independent, arbitrary-precision exact-integer oracle (`ExactBigIntOracle`, base $10^9$) decoupled from T81 internal arithmetic implementations.

The qualification is subject to two explicitly documented scope limitations that do not invalidate the accepted contracts:
1. **F3 Causal Attribution Boundary:** Finding F3 (`T81Limb54::karatsuba`) is mathematically and empirically verified correct under the 54-trit modular residue contract ($A \times B \pmod{3^{54}}$). Intermediate 28th-trit carry discard produces zero numerical error modulo $3^{54}$. However, historical causal attribution of the 299/300 SCG-06 failures remains incomplete; F1 Booth table scaling is the leading explanation, but exclusive attribution cannot be asserted without an isolated before/after experiment.
2. **F7 Floating-Point Precision Boundary:** Finding F7 (`T81Fraction::to_double()`) is verified exception-safe and numerically bounded across all supported fraction widths (13..324 trits) within binary64 floating-point precision (~53 bits mantissa, relative error tolerance $10^{-12}$). Correctly-rounded binary64 conversion remains deferred to RFC-00D5 / SCG-04 D2 and is not claimed under the existing contract.

---

## 2. Baseline Pinning & Provenance

The qualification assessment evaluates the exact git state of `origin/main` at:
- **Pinned Baseline SHA:** `ecb1e32b9458a1a1df51d7cdd4a19496c0584103`
- **Confirmed Integrated Pull Requests:**
  - **PR #729:** Commit `2c3501e2807bb893076c5037231c3126b7f2bc72` (SCG-03: C05 `T81Int` mul convolution & overflow, C11 test verdicts).
  - **PR #736:** Commit `e24eba723c221dc895e20307d41a8c4ba97405d6` (SCG-07 Phase 1: F6/F7 `T81Fraction` exception safety).
  - **PR #737:** Commit `1604fd204346c0ca09d14abe6be6f2417170f741` (SCG-07 Phase 2: F5 `T81BigInt` signed subtraction).
  - **PR #738:** Commit `5cc8d9457fd19ff2d85a5bda1fc1678d8e0124d1` (SCG-07 Phase 3: F1 `T81Limb27` Booth table scaling repair).
  - **PR #739:** Commit `8497ffb8742266d499e0cf8c826857776238db98` (SCG-07 Phase 4: F2 `T81Limb54::reference_mul` carry normalization).
  - **PR #740:** Commit `ecb1e32b9458a1a1df51d7cdd4a19496c0584103` (SCG-07 Phase 5: F3 `T81Limb54::karatsuba` verification & modular invariance).

### Cryptographic SHA-256 Pinning of Audited Artifacts

| Artifact | Filepath | SHA-256 Digest |
|---|---|---|
| SCG-06 Defect Register | `docs/research/continuity/SCG06_ARITHMETIC_DEFECT_REPRODUCTION_REPORT.md` | `1382eec631b588defc9e9b93993601c4649d128398dfe9b3252d7d030d5a811c` |
| SCG-03 Receipt | `docs/research/continuity/SCG03_EVIDENCE_RECEIPT.md` | `01fd5528df173bc5affc9239364e6590c35c4772b4b8fdd96211f22032b53ac1` |
| SCG-07 Phase 1 Receipt | `docs/research/continuity/SCG07_PHASE1_EVIDENCE_RECEIPT.md` | `10520bebaedc0912f0dd5e9c5afec7d2b69c171f6aed65171bd2fafbbdc3bf3d` |
| SCG-07 Phase 2 Receipt | `docs/research/continuity/SCG07_PHASE2_EVIDENCE_RECEIPT.md` | `b87a13e6863ba3810da875ac65a175b321b3639e7c14618bbf6814bfaa09cca3` |
| SCG-07 Phase 3 Receipt | `docs/research/continuity/SCG07_PHASE3_EVIDENCE_RECEIPT.md` | `b03d35b2d4cfce77c9503de193113f27af549e6c412e72042af0adc5d995159f` |
| SCG-07 Phase 4 Receipt | `docs/research/continuity/SCG07_PHASE4_EVIDENCE_RECEIPT.md` | `1e1264b7091014853578f44a5ce8762b5082621f16468162750c549c0ee1db3e` |
| SCG-07 Phase 5 Receipt | `docs/research/continuity/SCG07_PHASE5_EVIDENCE_RECEIPT.md` | `2cb39e423450ad6e92e9707b890752fd667d7244c1336d29a6c59766236efee6` |
| SCG-07 Preparation Doc | `docs/research/continuity/SCG07_TRUSTED_NUMERIC_FOUNDATION_PREPARATION.md` | `17cbef17d0e6814e7c8912d5e5d6d0e2c48d82da3e24ccfe18e5b2ff1954aea6` |
| SCG-04 Decision Record | `docs/research/continuity/SCG04_MAINTAINER_DECISION_RECORD.md` | `da20d459700b45f5b2be53973c514d83eb00a689dd7ac6f4110628838b50a92f` |
| Implementation: T81Int | `include/t81/types/T81Int.hpp` | `42a666a422d10e157b5a9f508c854182478f8d6cef80722a322c78c95913256b` |
| Implementation: T81Limb | `include/t81/types/T81Limb.hpp` | `75e760fc4ec9f8441b7cc9a89fbc29d3b21d22b20ad0f42229c41fd8e5f4e2cb` |
| Implementation: T81BigInt| `include/t81/types/T81BigInt.hpp` | `8b4ebeb5f4a7cfae128d32c5eb62e5bd35502d9b2e9f5fbfae5a4e71e28b79de` |
| Implementation: T81Fraction| `include/t81/types/T81Fraction.hpp` | `ed344e406be5ef59859e771477161ab4ad9347e367c9bdc87c4f05a85d50bc5a` |

---

## 3. Exact Changed-File Inventory (Phase 6 PR)

The scope of this qualification PR is strictly confined to 3 files:
1. `tests/cpp/test_scg07_phase6_qualification.cpp`: Consolidated multi-component qualification suite.
2. `CMakeLists.txt`: Registration of `t81_phase6_qualification_test` target in build and test lists.
3. `docs/research/continuity/SCG07_PHASE6_CONSOLIDATED_QUALIFICATION.md`: Authoritative qualification assessment report.

Zero production source files, ISA opcode tables, or normative specifications were modified.

---

## 4. Consolidated F1–F7 Evidence Matrix

| Finding | Original Defect & Reproducer | Historical Failure Count (SCG-06) | Repair PR & Baseline Commit | Current Implementation & Contract | Independent Oracle Methodology | Multi-Configuration Coverage | Current Status | Remaining Limitations | Qualification Disposition |
|---|---|---|---|---|---|---|---|---|---|
| **F1** | `T81Limb27` Booth table patterns $\pm 2, \pm 4$ set `shift_trytes = 1` ($3^3 = 27$). Reproducer: $1 \times 2 = 27$. | Pre-repair: 100% on affected patterns. | PR #738 (`5cc8d945`) | Direct balanced ternary convolution + carry normalization. Contract: $A \times B \in [-(3^{54}-1)/2, (3^{54}-1)/2]$. | `ExactBigIntOracle` arbitrary-precision multiplication. | Debug, Release (`-DNDEBUG`), ASan/UBSan. | 10,000 randomized trials + edge cases: 100% pass. | None within 27-trit domain. | **QUALIFIED** |
| **F2** | `T81Limb54::reference_mul` single-step subtraction carry loop emitted non-trits ($\pm 2, \pm 3$) and value mismatches. | 130/300 non-trits; 141/300 value mismatches. | PR #739 (`8497ffb8`) | Exact balanced ternary carry normalization across 54 trits. Contract: $A \times B \pmod{3^{54}}$. | `ExactBigIntOracle` exact product + balanced ternary modulo $3^{54}$. | Debug, Release (`-DNDEBUG`), ASan/UBSan. | 10,000 randomized trials + edge cases: 100% pass. | None within 54-trit modular residue contract. | **QUALIFIED** |
| **F3** | `T81Limb54::karatsuba` intermediate sum $x_0 + x_1$ 28th-trit carry discard. | 299/300 trials failed (68/68 carry-loss, 231/232 no-carry-loss). | PR #740 (`ecb1e32b`) | Mathematical analysis proves intermediate carry contributes $3^{54} \equiv 0 \pmod{3^{54}}$. Zero logic changes. | `ExactBigIntOracle` exact product modulo $3^{54}$ + differential check vs `reference_mul`. | Debug, Release (`-DNDEBUG`), ASan/UBSan. | 300/300 historical cases pass; 10,000 randomized trials pass. | Applies strictly to 54-trit modular contract, not 108-trit full product. Causal attribution incomplete. | **QUALIFIED WITH LIMITATIONS** |
| **F4** | `T81Int<N>::operator*` silent truncation/wrapping to 0 on overflow. | 100% silent data corruption on overflow. | PR #729 (`2c3501e2`) | Full $2N$-trit convolution accumulation and carry normalization before range check; throws `std::overflow_error`. | Exact integer comparison and `try/catch` exception assertion. | Debug, Release (`-DNDEBUG`), constexpr. | Small widths, boundary widths, exhaustive $N=1..4$: pass. | None within fixed-width integer specification. | **QUALIFIED** |
| **F5** | `T81BigInt` signed subtraction stored negative limbs with `negative_ = false`, causing `std::terminate()` in Debug or `std::logic_error` in Release. | 100% on $X - Y$ ($X < Y$). | PR #737 (`1604fd20`) | Single-limb fallback inspects `sign_trit() == Trit::N`, setting `negative_ = true` and storing `-val`. | `ExactBigIntOracle` arbitrary-precision signed subtraction. | Debug, Release (`-DNDEBUG`), ASan/UBSan. | 1,200 trials (800 oracle comparisons + 400 properties): 100% pass. | None within multi-precision integer specification. | **QUALIFIED** |
| **F6** | `T81Fraction` arithmetic operators were `noexcept`, converting `T81Int` `std::overflow_error` into `std::terminate()`. | Process abort (exit code 134) on numerator/denominator overflow. | PR #736 (`e24eba72`) | Removed `noexcept` from throwing operators, constructors, and `canonicalize()`. | Exception-safety assertions requiring strictly `std::overflow_error`. | Debug, Release (`-DNDEBUG`), ASan/UBSan. | Multi-width boundary checks: 100% catchable. | Exceptions must be handled by callers. | **QUALIFIED** |
| **F7** | `T81Fraction::to_double()` delegated to `to_int64()`, throwing `std::overflow_error` on $> 40$ trits out of a `noexcept` function. | Process abort (exit code 134) on fractions with $> 40$ trits. | PR #736 (`e24eba72`) | Implemented direct balanced-ternary accumulator `int_to_double() noexcept` across all widths 13..324. | `ExactBigIntOracle` rational conversion to `double` and analytical comparison. | Debug, Release (`-DNDEBUG`), ASan/UBSan. | Multi-width checks (13, 40, 41, 81, 324 trits): 100% pass. | Finite-range IEEE-754 binary64 approximation (~53-bit mantissa); correct rounding deferred to RFC-00D5. | **QUALIFIED WITH LIMITATIONS** |

---

## 5. Mathematical Contract Audit Matrix

| Component | Operation | Contract Type | Formal Contract Specification | Pre-Repair Status | Current Baseline Status | Verification Verdict |
|---|---|---|---|---|---|---|
| `T81Int<N>` | `operator*` | Exact & Trapping | Computes exact mathematical product $\prod = a \times b$. If $\prod \in [-(3^N-1)/2, (3^N-1)/2]$, returns canonical `T81Int<N>`. Otherwise invokes `axion::trap_overflow` throwing `std::overflow_error`. | Silent wrap to 0 | Exact product & reliable exception | **CONFORMANT** |
| `T81Limb27` | `operator*` | Wide Exact | Computes exact product of two 27-trit balanced ternary limbs into a wide 54-trit limb `T81Limb54`. All output trits $\in \{-1, 0, 1\}$. Range $[-(3^{54}-1)/2, (3^{54}-1)/2]$. | Scaled $\pm 2, \pm 4$ by 27 | Exact 54-trit canonical result | **CONFORMANT** |
| `T81Limb54` | `reference_mul` | Modular Residue | Computes canonical balanced-ternary residue $a \times b \pmod{3^{54}}$. All output trits $\in \{-1, 0, 1\}$. | Emitted non-trits ($\pm 2, \pm 3$) & mismatches | Exact canonical modular residue | **CONFORMANT** |
| `T81Limb54` | `karatsuba` | Modular Residue | Computes canonical balanced-ternary residue $a \times b \pmod{3^{54}}$ via 27-trit Karatsuba decomposition. Invariance under intermediate 28th-trit carry loss proven. | 299/300 failures | Exact canonical modular residue | **CONFORMANT** |
| `T81BigInt` | `operator-` | Exact Arbitrary-Precision | Computes exact signed difference $a - b$. Stored limbs have non-negative MSB trit; sign represented by `negative_`. Satisfies `verify_invariants()`. | Abort in Debug, logic error in Release | Exact arbitrary-precision signed difference | **CONFORMANT** |
| `T81Fraction` | Operators (`+`, `-`, `*`, `/`) | Exception Propagating | Exact rational arithmetic. On integer overflow in numerator or denominator, propagates `std::overflow_error` to caller. Not `noexcept`. | `std::terminate()` abort | Clean exception propagation | **CONFORMANT** |
| `T81Fraction` | `to_double()` | Finite-Range Approximation | Converts rational $n/d$ to binary64 floating-point across widths $13 \le N \le 324$. `noexcept`. Relative precision bounded by binary64 mantissa (~53 bits, tol $10^{-12}$). Correct rounding not guaranteed. | `std::terminate()` on $> 40$ trits | Safe conversion within binary64 precision | **CONFORMANT (WITH BOUNDS)** |

---

## 6. Historical versus Current Results Comparison

| Scope / Test Battery | Historical Baseline (SCG-06, `2c3501e2`) | Current Baseline (Phase 6, `ecb1e32b`) | Delta / Resolution |
|---|---|---|---|
| F1 Reproducer ($1 \times 2$) | Output: 27 (FAIL) | Output: 2 (PASS) | Fixed by PR #738 |
| F2 SplitMix64 300 Trials | 130 non-canonical trits, 141 value mismatches | 0 non-canonical trits, 0 value mismatches (300/300 PASS) | Fixed by PR #739 |
| F3 SplitMix64 300 Trials | 299/300 failures (68 carry-loss, 231 no-carry-loss) | 300/300 PASS (0 failures across both cohorts) | Verified by PR #740 |
| F4 Small-Width Overflow | Silent wrap to 0 (FAIL) | Throws `std::overflow_error` (PASS) | Fixed by PR #729 |
| F5 Subtraction ($X < Y$) | `std::terminate()` (Debug) / `logic_error` (Release) | Exact signed difference: -985392... (PASS) | Fixed by PR #737 |
| F6 Fraction Mul Overflow | `std::terminate()` process abort (FAIL) | Throws catchable `std::overflow_error` (PASS) | Fixed by PR #736 |
| F7 Fraction $> 40$ Trits | `std::terminate()` process abort (FAIL) | Evaluates double approximation safely (PASS) | Fixed by PR #736 |
| Limb27 Randomized Tests | Not tested | 10,000 trials vs `ExactBigIntOracle` (100% PASS) | Verified |
| Limb54 Ref Randomized Tests | Not tested | 10,000 trials vs `ExactBigIntOracle` (100% PASS) | Verified |
| Limb54 Karatsuba Randomized | Not tested | 10,000 trials vs `ExactBigIntOracle` (100% PASS) | Verified |
| BigInt Subtraction Randomized| Not tested | 1,200 trials vs `ExactBigIntOracle` (100% PASS) | Verified |
| Phase 6 Consolidated Battery | Not existing | 100% PASS across all F1–F7 tests unconditionally | Verified |

---

## 7. Independent Oracle Methodology & Soundness

To prevent circular dependencies where defective T81 routines validate each other, all Phase 6 evaluations enforce strict oracle independence:
1. **Mathematical Base-$10^9$ Decomposition:** `ExactBigIntOracle` represents integers as little-endian vectors of unsigned 32-bit digits with radix $10^9$. Multi-digit addition, subtraction, multiplication, and balanced ternary division with remainder are implemented purely using host scalar arithmetic (`uint64_t` / `int64_t`).
2. **Decoupled Input/Output:** Operands are passed as plain `std::span<const int8_t>` balanced-ternary digit arrays or standard `int64_t` scalars.
3. **Balanced Ternary Conversion:** The oracle computes balanced ternary residues via mathematical Euclidean division:
   $$\text{rem} \in \{-1, 0, 1\}, \quad q = \frac{v - \text{rem}}{3}$$
   without referencing any T81 library functions.
4. **Hardened Arithmetic:** Hardened for `INT64_MIN` via unsigned arithmetic casting (`(v == INT64_MIN) ? 9223372036854775808ULL : -v`).

---

## 8. Cross-Component Integration Verification

The components of the arithmetic subsystem were verified working in composition:
1. **Limb27 $\to$ Limb54 Karatsuba:** `T81Limb54::karatsuba` decomposes operands into 27-trit limbs, calls `T81Limb27::operator*` for low, high, and middle cross-products, and shifts/accumulates into 54 trits. Tested directly against `ExactBigIntOracle` across 10,000 randomized trials with zero discrepancies.
2. **Karatsuba vs Reference Multiplication Differential Check:** Across 10,000 randomized 54-trit pairs and dense carry chains, `karatsuba(a, b) == reference_mul(a, b)` with 100% bitwise trit identity.
3. **BigInt Subtraction $\to$ Addition Single-Limb Fallback:** `T81BigInt::operator-(a, b)` negates `b` and invokes `operator+(a, -b)`. In single-limb cases where $a < b$, the fallback block properly captures negative limbs, sets `negative_ = true`, and stores non-negative limb magnitudes, preserving invariants across multi-limb borrow chains.
4. **Fraction $\to$ T81Int Throwing Propagation:** Rational multiplication $f_1 \times f_2$ exercises underlying `T81Int<N>::operator*`. When the mathematical product exceeds $N$ trits, the thrown `std::overflow_error` unwinds cleanly across `T81Fraction` without catching or converting to `std::terminate()`.

---

## 9. Configuration Validation Matrix

| Target / Suite | Configuration | Compiler | Flags | Test Count | Result |
|---|---|---|---|---|---|
| `t81_phase6_qualification_test` | Debug | GCC 13.3.0 | `-O0 -g` | 5,300+ checks | **PASS (100%)** |
| `t81_phase6_qualification_test` | Release | GCC 13.3.0 | `-O3 -DNDEBUG` | 5,300+ checks | **PASS (100%)** |
| `t81_phase6_qualification_test` | ASan + UBSan | GCC 13.3.0 | `-fsanitize=address,undefined -g` | 5,300+ checks | **PASS (0 errors, 0 leaks)** |
| `t81_limb54_karatsuba_test` | Debug / Release | GCC 13.3.0 | Default / CTest | 10,300+ checks | **PASS (100%)** |
| `t81_limb54_reference_mul_test` | Debug / Release | GCC 13.3.0 | Default / CTest | 10,300+ checks | **PASS (100%)** |
| `t81_limb27_mul_test` | Debug / Release | GCC 13.3.0 | Default / CTest | 10,000+ checks | **PASS (100%)** |
| `t81_bigint_sub_test` | Debug / Release | GCC 13.3.0 | Default / CTest | 1,200+ checks | **PASS (100%)** |
| `t81_t81fraction_test` | Debug / Release | GCC 13.3.0 | Default / CTest | Standalone | **PASS (100%)** |
| `t81_t81int_test` | Debug / Release | GCC 13.3.0 | Default / CTest | Exhaustive $N=1..4$ | **PASS (100%)** |

### Assertion Effectiveness Under `-DNDEBUG`
All qualification assertions in `tests/cpp/test_scg07_phase6_qualification.cpp` and preceding SCG-07 regression suites use explicit, fail-closed `QUAL_REQUIRE` / `require` macros calling `std::abort()` upon failure. No standard `<cassert>` `assert()` macros are used in the qualification path, guaranteeing 100% active enforcement under `-DNDEBUG`.

---

## 10. CI Failure Attribution

Baseline CI checks on `origin/main` at `ecb1e32b9458a1a1df51d7cdd4a19496c0584103` exhibit known inherited failures that are independent of the arithmetic subsystem:
1. **Independently Confirmed Baseline Regressions:**
   - `t81_cli_contract_test_runner`: Fails on missing test fixture `models/tiny-random-llama/model.safetensors` (`cli_contract_test.cpp:549`).
   - `t81_root_structure_policy_test`: Fails because untracked directory `review/` exists in repository root (committed during PR #656 review).
   - Windows CI workflow: CMake generation fails due to sparse checkout omitting `/development_tests/**`.
2. **Environmental Limitations:**
   - Host `Ubuntu clang version 18.1.3` exhibits an internal frontend segmentation fault (exit code 139) when parsing GNU `libstdc++-13` header `/usr/include/c++/13/ext/alloc_traits.h` (`std::basic_string::swap`), even on a minimal one-line `#include <string>` test. This is an environment packaging incompatibility between Clang 18 and GCC 13 libstdc++ headers on Ubuntu 24.04, not a defect in T81 code. GCC 13.3.0 compiles and verifies cleanly across Debug, Release, and Sanitizer configurations.
3. **Zero Subsystem Regressions:** All 7 dedicated arithmetic test targets in CTest (`t81_phase6_qualification_test`, `t81_limb54_karatsuba_test`, `t81_limb54_reference_mul_test`, `t81_limb27_mul_test`, `t81_bigint_sub_test`, `t81_t81fraction_test`, `t81_t81int_test`) pass 100%.

---

## 11. Evidence Discrepancies & Audit Findings

During the Phase F audit of SCG-07 evidence receipts (Phases 1–5), the following observations were noted:
1. **Assertion Styles in Historical Suites:** `tests/cpp/test_t81int.cpp` (from SCG-03) uses standard `assert()` macros which compile away under `-DNDEBUG`. However, all SCG-07 suites (Phases 1–5) and Phase 6 qualification use unconditional `require()` / `std::abort()` checks.
2. **F3 Historical Attribution Framing:** Earlier discussions in SCG-06 hypothesized intermediate carry loss as a defect in Karatsuba. Phase 5 and Phase 6 rigorously proved that intermediate carry discard is mathematically invariant modulo $3^{54}$. The qualification record properly bounds this finding without claiming exclusive attribution for the historical 299/300 failures.
3. **No Retrospective Rewriting:** All historical receipts in `docs/research/continuity/` remain byte-for-byte unmodified; this qualification report serves as the consolidated authoritative synthesis.

---

## 12. Residual Risks & Untested Domains

1. **Wider Full-Precision Multiplication ($> 54$ trits):** `T81Limb54::karatsuba` and `reference_mul` are verified strictly for modular residue arithmetic ($A \times B \pmod{3^{54}}$). Computing a full 108-trit non-modular product requires preserving intermediate carries and is not covered by the current limb contract.
2. **Correctly-Rounded Binary64 Floating-Point Conversion:** `T81Fraction::to_double()` is an approximation bounded by standard binary64 precision. Exact directed rounding (IEEE-754 roundTiesToEven) for arbitrary rational ternary inputs remains deferred to future maintainer decision under RFC-00D5 / SCG-04 D2.
3. **Dual-Surface Checked Arithmetic:** Checked arithmetic APIs returning `Result<T, E>` under SCG-04 D1=1B and D4=4B remain unauthorized for implementation and are not part of the current production surface.

---

## 13. Reproducible Commands & Deterministic Seeds

### Compilation & Test Execution
```bash
# 1. CMake Debug Build & CTest
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build --target t81_phase6_qualification_test -j$(nproc)
ctest --test-dir build --output-on-failure -R "t81_phase6_qualification_test"

# 2. Standalone Release Build (-O3 -DNDEBUG)
g++ -std=c++20 -O3 -DNDEBUG -Iinclude tests/cpp/test_scg07_phase6_qualification.cpp build/libt81_core.a -o /tmp/qual_release
/tmp/qual_release

# 3. Standalone Address & Undefined Sanitizer Build
g++ -std=c++20 -fsanitize=address,undefined -g -Iinclude tests/cpp/test_scg07_phase6_qualification.cpp build/libt81_core.a -o /tmp/qual_asan
/tmp/qual_asan

# 4. Clang-Format Style Conformance Gate
clang-format-18 --dry-run --Werror tests/cpp/test_scg07_phase6_qualification.cpp
```

### Deterministic Pseudo-Random Seeds
- **F1 (T81Limb27 multiplication):** `SplitMix64(0xDEADBEEF01ULL)` (2,000 trials).
- **F2 & F3 (T81Limb54 multiplication):** `SplitMix64(0xCAFEBABEF00DULL)` (2,500 trials).
- **F5 (T81BigInt signed subtraction):** `SplitMix64(0x8899AABBCCDDEEFFULL)` (800 trials).

---

## 14. Final Qualification Boundaries

The qualification boundaries are established as follows:
- **Scope Qualified:** Core arithmetic operations (`T81Int` multiplication/overflow, `T81Limb27` multiplication, `T81Limb54` reference & Karatsuba modular multiplication, `T81BigInt` signed subtraction, and `T81Fraction` arithmetic exception safety).
- **Scope Excluded:** Hardware PMU benchmarking, speculative CPU speedup claims, non-modular 108-trit multiplication, and unapproved ISA opcode modifications.

---

## 15. Recommended Next Research Step

With the trusted numeric foundation qualified on `origin/main`, the arithmetic subsystem is stable. The recommended next research step is:
- **Proceed with SCG-04 Architecture Policy Formalization:** Under maintainer guidance, draft formal RFCs for dual-surface checked arithmetic (D1=1B) and generic `Result<T, E>` specification (D4=4B) without prematurely authorizing production code modifications until normative RFC reviews are concluded.
