# SCG-07 Phase 5 Evidence Receipt: T81Limb54 Karatsuba Multiplication Correctness Verification (F3)

**Document ID:** `T81-SCG-07-PHASE5-RECEIPT-001-R1`
**Date:** 2026-11-09
**Repository Baseline Anchor:** `8497ffb8742266d499e0cf8c826857776238db98` (`origin/main`, merging PR #738 and PR #739)
**Task Classification:** Correctness Verification & Test Suite Integration (SCG-07 Phase 5, Finding F3)
**Final Disposition:** `F3 CORRECT ON CURRENT BASELINE — historical causal attribution incomplete`

---

## 1. Executive Summary

In T81 Foundation task SCG-06 (`SCG06_ARITHMETIC_DEFECT_REPRODUCTION_REPORT.md`), Finding F3 documented that `T81Limb54::karatsuba` failed exact product comparison in 299 out of 300 trials (100% in the 68 carry-loss cases and 231/232 in the non-carry-loss cases). SCG-06 hypothesized that radical intermediate carry discard in `x0 + x1` (`T81Limb27::operator+`) was a causal defect mechanism for high-magnitude operands, while also noting that `T81Limb27::operator*` Booth table scaling (F1) cascaded into Karatsuba.

In Phase 5 of SCG-07:
1. **Historical Observation:** In SCG-06 across 300 pseudo-random trials, `T81Limb54::karatsuba` failed 299/300 cases against the mathematical oracle.
2. **Current Observation:** On baseline `8497ffb8742266d499e0cf8c826857776238db98` (which incorporates Phase 3 F1 repair `ca7a6b7a` and Phase 4 F2 repair `e4e32244`), the exact historical 300-trial battery was rerun with zero failures (300/300 passed, 0 non-canonical trits, 0 mathematical mismatches).
3. **Independent Verification:** A focused regression test suite (`tests/cpp/test_t81_limb54_karatsuba.cpp`) evaluated `T81Limb54::karatsuba` against an independent base-$10^9$ arbitrary-precision integer oracle (`ExactBigIntOracle`) across 10,000 randomized trials and comprehensive deterministic edge cases, achieving 100% agreement and strict canonicality.
4. **Mathematical Explanation:** Under the established 54-trit modular multiplication contract ($A \times B \pmod{3^{54}}$), intermediate 28th-trit carries in $x_0 + x_1$ and $y_0 + y_1$ carry weight $\ge 3^{27}$. When multiplied in $Z_1$ and shifted left by 27 trits ($3^{27}$), their positional weight is $\ge 3^{54} \equiv 0 \pmod{3^{54}}$. Therefore, the discarded 28th-trit carry does not affect the canonical result modulo $3^{54}$. This mathematical invariance applies strictly to the 54-trit modular contract and is not generalized to full-precision multiplication ($108$ trits) or wider contexts.
5. **Causal Attribution:** While the 299/300 historical failures and 0/300 current failures are established, asserting exclusive causal attribution to F1 requires an isolated F1-only before/after experiment. Without that isolated evidence, historical causal attribution remains incomplete, though F1 is the leading explanation for historical failures.

---

## 2. Scope Inventory

The diff is strictly constrained to 4 files against baseline `8497ffb8742266d499e0cf8c826857776238db98`:

| File | Change Description |
|---|---|
| `include/t81/types/T81Limb.hpp` | Added mathematical documentation comments to `T81Limb54::karatsuba` detailing the modular invariance of $3^{27} Z_1$ and $3^{54} Z_2 \equiv 0 \pmod{3^{54}}$. Zero arithmetic or logic modifications. |
| `tests/cpp/test_t81_limb54_karatsuba.cpp` | Dedicated regression test suite verifying historical 300 trials, F3 reproducers, small values, powers of three, alternating patterns, cross-term cancellation, dense carry chains, truncation overflow, commutativity, and 10,000 randomized trials against `ExactBigIntOracle`. |
| `CMakeLists.txt` | Registered `t81_limb54_karatsuba_test` executable and test runner in CMake configuration and added to `T81_TEST_TARGETS`. |
| `docs/research/continuity/SCG07_PHASE5_EVIDENCE_RECEIPT.md` | Formal evidence receipt and verification report for independent audit. |

---

## 3. Mathematical Analysis & Root-Cause Disentanglement

### 3.1 Karatsuba Decomposition Modulo $3^{54}$
Let $Z = 3^{27}$. In the ring $\mathbb{Z} / 3^{54}\mathbb{Z}$ (canonical 54-trit modular residue arithmetic):
Any 54-trit balanced-ternary operand $A$ decomposes into two 27-trit halves $A = A_0 + Z A_1$, where $A_0, A_1 \in [-(Z-1)/2, (Z-1)/2]$.
Similarly, $B = B_0 + Z B_1$.

The product evaluates to:
$$A \times B = (A_0 + Z A_1)(B_0 + Z B_1) = A_0 B_0 + Z(A_0 B_1 + A_1 B_0) + Z^2(A_1 B_1)$$
Since $Z^2 = (3^{27})^2 = 3^{54} \equiv 0 \pmod{3^{54}}$:
$$A \times B \equiv A_0 B_0 + Z(A_0 B_1 + A_1 B_0) \pmod{3^{54}}$$

In Karatsuba multiplication:
$$Z_0 = A_0 B_0$$
$$Z_2 = A_1 B_1$$
$$Z_1 = (A_0 + A_1)(B_0 + B_1) - Z_0 - Z_2 = A_0 B_1 + A_1 B_0$$
$$A \times B = Z_0 + Z Z_1 + Z^2 Z_2 \equiv Z_0 + Z Z_1 \pmod{3^{54}}$$

### 3.2 Analysis of Intermediate 28th-Trit Carry Discard
In `T81Limb54::karatsuba`:
```cpp
auto x0 = T81Limb27::from_low_27(x);
auto x1 = T81Limb27::from_high_27(x);
auto y0 = T81Limb27::from_low_27(y);
auto y1 = T81Limb27::from_high_27(y);

auto z0 = T81Limb54(x0 * y0);
auto z2 = T81Limb54(x1 * y1);
auto mid = T81Limb54((x0 + x1) * (y0 + y1));
auto z1 = (mid - z0) - z2;

T81Limb54 result = z0;
result = result + z1.shift_left_trytes(9);
result = result + z2.shift_left_trytes(18);
return result;
```
When $x_0 + x_1$ is evaluated via `T81Limb27::operator+`, the 28th-trit carry $c_x \in \{-1, 0, 1\}$ is discarded:
$$(x_0 + x_1)_{\mathbb{Z}} = s_x + Z c_x$$
where $s_x$ is the 27-trit limb returned by `operator+`.
Similarly:
$$(y_0 + y_1)_{\mathbb{Z}} = s_y + Z c_y$$
The intermediate product computed is $\text{mid} = s_x \times s_y$.
Substituting into integer arithmetic:
$$s_x \times s_y = (x_0 + x_1 - Z c_x)(y_0 + y_1 - Z c_y) = (x_0 + x_1)(y_0 + y_1) - Z [c_x (y_0 + y_1) + c_y (x_0 + x_1)] + Z^2 c_x c_y$$
Multiplying by $Z = 3^{27}$ (corresponding to `shift_left_trytes(9)`):
$$Z (s_x \times s_y) = Z (x_0 + x_1)(y_0 + y_1) - Z^2 [c_x (y_0 + y_1) + c_y (x_0 + x_1)] + Z^3 c_x c_y$$
In the ring $\mathbb{Z} / 3^{54}\mathbb{Z}$:
$$Z^2 \equiv 3^{54} \equiv 0 \pmod{3^{54}}$$
$$Z^3 \equiv 3^{81} \equiv 0 \pmod{3^{54}}$$
Therefore:
$$Z (s_x \times s_y) \equiv Z (x_0 + x_1)(y_0 + y_1) \pmod{3^{54}}$$
**Conclusion:** Discarding the 28th-trit carry produces zero numerical error modulo $3^{54}$. The historical attribution of failure to carry loss was mathematically incorrect for 54-trit modular multiplication.

### 3.3 Historical Attribution Reconciled
In SCG-06:
- 68/68 carry-loss cases failed.
- 231/232 non-carry-loss cases failed.
Both cohorts exhibited mathematical mismatches. F1 (`T81Limb27::operator*` Booth table scaling error) was present and is the leading explanation for the historical failures. However, because multiple components were updated between pre-repair baselines and the current baseline, exclusive causal attribution to F1 is not asserted without an isolated F1-only before/after experiment.

---

## 4. Verification Evidence

### 4.1 Comparison Across SCG-06 300 Trials

| Metric / Cohort | SCG-06 Baseline (`2c3501e2`) | Current Baseline (`8497ffb8`) | Status |
|---|---|---|---|
| Total Trials Tested | 300 | 300 | Complete |
| Cohort with Carry Loss ($c_x \ne 0$ or $c_y \ne 0$) | 68 cases (68 failed) | 68 cases (0 failed) | **100% PASS** |
| Cohort without Carry Loss ($c_x = 0, c_y = 0$) | 232 cases (231 failed) | 232 cases (0 failed) | **100% PASS** |
| Emitted Non-Canonical Trits | 0 | 0 | Strictly Canonical |
| Mathematical Value Mismatches modulo $3^{54}$ | 299 (99.7%) | 0 (0.0%) | **100% PASS** |

### 4.2 Independent Oracle & Randomized Verification
`tests/cpp/test_t81_limb54_karatsuba.cpp` tests:
- Exact 300 SCG-06 SplitMix64 reproduction cases.
- Small values: $\pm 1, \pm 2, \pm 3, \pm 4, \pm 9, \pm 13, \pm 26, \pm 27, 0$.
- Zero, identity ($v \times 1 = v$, $v \times (-1) = -v$, $v \times 0 = 0$), and sign combinations.
- Powers of three across all $54 \times 54$ index pairs with signs $\pm 1$.
- Alternating patterns and boundary extremes ($\pm (3^{54}-1)/2$).
- Cross-term cancellation and 27-trit split carry overflow.
- Dense cascading carry chains (lengths 2..54).
- Truncation overflow outside 54 trits.
- Commutativity ($a \times b = b \times a$).
- 10,000 randomized trials against `ExactBigIntOracle` (base $10^9$).
- Differential check against repaired `T81Limb54::reference_mul`.

### 4.3 Test Execution Matrix

| Test Suite | Configuration | Command | Result |
|---|---|---|---|
| `t81_limb54_karatsuba_test` | Debug | `cmake --build build --target t81_limb54_karatsuba_test && ./build/t81_limb54_karatsuba_test` | **PASS (100%)** |
| `t81_limb54_karatsuba_test` | Release (`-DNDEBUG -O3`) | `g++ -std=c++20 -O3 -DNDEBUG -Iinclude tests/cpp/test_t81_limb54_karatsuba.cpp -o /tmp/test_karatsuba_release && /tmp/test_karatsuba_release` | **PASS (100%)** |
| `t81_limb54_karatsuba_test` | Address/Undefined Sanitizer | `g++ -std=c++20 -fsanitize=address,undefined -g -Iinclude tests/cpp/test_t81_limb54_karatsuba.cpp -o /tmp/test_karatsuba_asan && /tmp/test_karatsuba_asan` | **PASS (zero errors/leaks)** |
| `t81_limb27_mul_test` | Phase 3 F1 suite | `./build/t81_limb27_mul_test` | **PASS (10,000 trials)** |
| `t81_limb54_reference_mul_test` | Phase 4 F2 suite | `./build/t81_limb54_reference_mul_test` | **PASS (10,000 trials)** |
| `t81_bigint_sub_test` | Phase 2 F5 suite | `./build/t81_bigint_sub_test` | **PASS (1,200 trials)** |
| `t81_t81fraction_test` | Phase 1 F6/F7 suite | `./build/t81_t81fraction_test` | **PASS** |
| `t81_limb54_booth_mul_test` | Existing Booth suite | `./build/t81_limb54_booth_mul_test` | **PASS (4,096 trials)** |
| `t81_limb54_add_test` | Existing limb54 addition | `./build/t81_limb54_add_test` | **PASS** |
| `t81_limb_bohemian_add_test` | Existing limb addition | `./build/t81_limb_bohemian_add_test` | **PASS** |
| `t81_limb_add_test` | Existing limb addition | `./build/t81_limb_add_test` | **PASS** |
| `clang-format-18` | Style conformance | `clang-format-18 --dry-run --Werror include/t81/types/T81Limb.hpp tests/cpp/test_t81_limb54_karatsuba.cpp` | **PASS (0 violations)** |

---

## 5. Inherited Baseline Failures & Scope Limitations

1. **Inherited Baseline CI Regressions:** Known baseline regressions on `origin/main` at `8497ffb8742266d499e0cf8c826857776238db98` remain unchanged and independent of this change (missing `models/tiny-random-llama/model.safetensors`, committed `review/pr-656/`, Windows sparse checkout omitting `/development_tests/**`, and runner/toolchain warnings in GCC 15 / macOS).
2. **Contract Specificity:** The proof of carry-loss invariance is strictly bound to the 54-trit modular multiplication contract ($A \times B \pmod{3^{54}}$). For a wide $108$-trit full-precision product, intermediate carries would contribute to trits 54..80 and could not be discarded.
3. **Defect Findings:** Findings F1, F2, F4, F5, F6, and F7 remain in their closed/verified state.
4. **Final Disposition:** `F3 CORRECT ON CURRENT BASELINE — historical causal attribution incomplete`.
