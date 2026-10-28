# H-CONV-01: Independent Scientific Review and Research Closure

**Research Identifier:** H-CONV-01
**Target Repository:** `t81dev/t81-foundation`
**Review Baseline:** Commit `d6c0b5f3` (`origin/research/h-conv-01`)
**Evaluator:** Jules (Independent Scientific Review)
**Date:** October 2026

---

## A. Review Disposition

**Disposition:** **SUPPORTED WITHIN TESTED DOMAIN**

### Explanation
The empirical evidence and mathematical analysis confirm that replacing repeated division by two with a single wide division in the IEEE-754 binary32 to balanced-ternary `Fixed<192,80>` conversion (`canonical_fixed_from_float_`) produces bitwise-identical 48-byte packed representations across all 1,778 tested corpus inputs.

1. **Exact Correctness:** Zero output mismatches were observed between the independent exact-rational Python oracle (Method A), the existing repeated-division baseline (Method B), and the single-quotient alternative (Method C).
2. **Operation Reduction:** The source-derived count of wide integer divisions dropped from 87,510 (Method B) to 1,090 (Method C) across the corpus—a 98.75% reduction.
3. **Performance Observation:** Measured native wall-clock timing showed a median latency reduction from 118.518 ms/input (Baseline B) to 1.622 ms/input (Alternative C), representing an exploratory ~73.06× speedup. However, due to host sleep/wake activity during measurement, wall-clock timing is classified strictly as exploratory.

---

## B. Mathematical Assessment

### 1. Proven Properties
* **Capacity and Overflow Safety:** The maximum finite binary32 magnitude $|x| < 2^{128}$. Scaled by $3^{80} < 2^{127}$, the integer target magnitude $|Q(x)| < 2^{255}$, which fits comfortably inside the 192-trit signed capacity ($> 2^{303}$). Intermediate numerators in 384-trit wide integer arithmetic ($> 2^{607}$) avoid overflow for all binary32 inputs.
* **Truncation Rounding Equivalence:** For non-negative integers $a$, integer floor division satisfies $\lfloor \lfloor a / 2^j \rfloor / 2 \rfloor = \lfloor a / 2^{j+1} \rfloor$. Thus, $k$ repeated truncating divisions by 2 (Method B) are mathematically identical to a single truncating division by $2^k$ (Method C).
* **Sign Application:** For negative inputs ($x < 0$), both B and C convert positive magnitudes $|x|$ and negate the result afterwards. Signed zero ($+0.0$ and $-0.0$) maps identically to raw 0 (encoded as 48 bytes of `0x55`).

### 2. Tested Properties
* **Corpus Verification:** 1,778 deduplicated binary32 inputs were evaluated. The corpus includes exponent boundary predecessors/successors, minimum and maximum subnormals, exact integers, rational grid/half-grid neighbors, and seeded pseudorandom patterns.
* **Nonfinite Handling:** Infinities and NaNs evaluate to `std::nullopt` (`nonfinite`) across all methods without raising unhandled exceptions or corrupting state.
* **Subnormals:** Minimum subnormal ($2^{-149}$) evaluates to 0, while the largest positive subnormal evaluates to raw integer 1 under $3^{80}$ grid quantization, matching across all methods.

### 3. Unresolved Questions & Limitations
* **Non-Exhaustive Coverage:** The 1,778 tested patterns represent $< 0.0001\%$ of the $2^{32}$ binary32 domain.
* **Global Rounding Policy Discrepancy:** The scoped private helper (`canonical_fixed_from_float_`) enforces truncation toward zero (`trunc_toward_zero`), while public `T81Fixed` constructors employ nearest rounding (`llround`). System-wide conversion semantics remain un-unified.

---

## C. Performance Assessment

### 1. Source-Derived Operation Accounting
* Baseline Method B performs $k = \max(0, -e)$ wide divisions for negative binary exponents $e < 0$. Across 1,778 corpus inputs, this totals 87,510 wide integer divisions.
* Alternative Method C performs 1 wide division for inputs with $e < 0$, plus $k$ wide denominator doublings (shifts/additions). Across the corpus, Method C executes 1,090 wide divisions and 87,510 wide doublings.
* The 98.75% reduction in wide divisions represents a structural algorithmic improvement rather than compiler optimization artifacts.

### 2. Exploratory Wall-Clock Timing Analysis
* **Summary Latencies:** Baseline B median = 118.518 ms/input; Alternative C median = 1.622 ms/input.
* **Host Sleep Contamination:** Power event logs (`power-events.txt`) document host sleep/wake cycles during the native execution window, introducing significant variance in baseline timing (max 259.45 ms/input).
* **Protocol Compliance:** In accordance with scientific integrity guidelines, raw timing records were preserved without filtering, pruning, or rerunning. Wall-clock measurements provide a valid directional observation but cannot serve as confirmatory micro-architectural benchmark data.

---

## D. Provenance Assessment

1. **Extraction Lineage:** Source snapshots in `source/` (`T81Int.hpp`, `dmath_types.hpp`, `tensor.hpp`) track cleanly to repository baseline commit `0662e0e637f47168e56ca4ddd66e8ee877473b38`. Historical Hanoi/CWEB source (`hanoi-source.cweb`) matches Git blob `7132efaed9346526d1b4ad3bc1c593e0a71ee0d3`.
2. **Manifest and Archive Integrity:** Re-verification using `python3 -B experiments/balanced_ternary_conversion/verify.py` passes all 14 hash, extraction, oracle, and statistical checks. Manifest alignment commit `d6c0b5f3` correctly bound all tracked files to `manifest.json` and `raw-evidence.tar.gz`.
3. **Reproducibility:** Verification runs completely offline without requiring native re-compilation, third-party network access, or external tooling.

---

## E. Material Findings

### BLOCKING Findings
* **None.**

### NONBLOCKING Findings

#### FINDING-NB-01: Host Sleep Contamination in Native Benchmarks
* **Location:** `experiments/balanced_ternary_conversion/power-events.txt`, `performance-summary.json`
* **Reasoning:** Host sleep/wake events coincided with native batch execution, elevating baseline batch variance (maximum batch latency 259.45 ms/input vs median 118.52 ms/input). While retained without data manipulation per research protocol, this limits timing metrics to exploratory status.

#### FINDING-NB-02: Un-unified Fixed-Point Conversion Rounding Policy
* **Location:** `experiments/balanced_ternary_conversion/SPEC.md`, `include/t81/tensor.hpp`, `include/t81/types/T81Fixed.hpp`
* **Reasoning:** Private tensor conversion (`canonical_fixed_from_float_`) truncates toward zero, whereas public `T81Fixed` constructors use `llround` nearest integer rounding. This limits the scope of H-CONV-01 to private tensor conversion and prevents direct generalization to all T81 fixed-point conversions.

### INFORMATIONAL Findings

#### FINDING-INF-01: Sparse Sampling of Binary32 Space
* **Location:** `experiments/balanced_ternary_conversion/data/corpus.json`
* **Reasoning:** The corpus contains 1,778 cases targeting boundary and random patterns. While sufficient for bounded research validation, full proof of equivalence across all $2^{32}$ inputs remains an open question for future exhaustive verification.

#### FINDING-INF-02: Explicit Denominator Doubling Accounting
* **Location:** `experiments/balanced_ternary_conversion/native.cpp` (lines 48–57)
* **Reasoning:** Method C replaces 86,420 wide divisions with 87,510 wide additions. Although wide additions are significantly cheaper than restoring divisions, comprehensive complexity accounting must retain denominator construction costs.

---

## F. Research Closure Recommendation

### Recommendation
**Accept H-CONV-01 as a bounded research result** (`SUPPORTED WITHIN TESTED DOMAIN`).

The experimental evidence establishes mathematical equivalence and a structural operation reduction for the private tensor conversion helper across the tested domain. No production changes or governance modifications should be made based on this exploratory study.

### Falsifiable Follow-up Question
> "Does direct single-quotient conversion (`Method C`) maintain bitwise identity with the independent exact-rational oracle (`Method A`) across an exhaustive sweep of all $2^{32}$ binary32 values when executed in an isolated, sleep-inhibited environment?"
