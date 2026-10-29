# T81 Scientific Investigation 03 — Exact Rational C++ Prototype Findings Report

**Repository:** https://github.com/t81dev/t81-foundation
**Evaluated Revision:** `17f835453c1e16639ad7b9bff87bd15225e8912d`
**Role:** Independent Numerical Computing Research Engineer
**Research Phase:** Gate 1B — Exact Rational Conversion Prototype Validation
**Final Scientific Disposition:** **PROCEED** (Experimental Prototype Conformance Verified)

---

### Executive Summary

This investigation implements, evaluates, and benchmarks an isolated, self-contained C++23 conversion prototype (`experiments/exact_rational_prototype/exact_rational.hpp`) for binary64-to-balanced-ternary checked conversion using pure arbitrary-precision rational arithmetic.

The prototype was constructed without modifying production `T81Float.hpp` or public production APIs, eliminating all reliance on host floating-point hardware or `libm` transcendental functions (`std::log`, `std::floor`, `powl`, `std::llround`).

1. **Corpus Conformance:** The prototype achieved a **100% pass rate (104/104 cases)** on `experiments/checked_float_contract/corpus.json`.
2. **Counterexample Resolution:** All three Gate 1 counterexamples (`0x7fefffffffffffff`, `0x0000000000000001`, and `0x3fe0000000000000`) were successfully resolved, matching the exact rational reference values byte-for-byte in `nearest_even` mode and failing with explicit `inexact` error in `exact_only` mode.
3. **Boundary Verification:** 7/7 additional mathematical boundary cases (underflow, subnormal minimums, overflow thresholds, NaNs, and infinities) passed as expected.
4. **Performance Benchmark:** Measured C++23 prototype throughput on Linux x86-64:
   - `from_binary64` (`<27,9>` `nearest_even`): **525.75 $\mu\text{s}$ per operation** (1,902.05 ops/sec), representing a **3.95x speedup** over the Python `reference.py` baseline (2,076.81 $\mu\text{s}$/op).
   - `to_binary64` (`<27,9>` `nearest_even`): **6.66 $\mu\text{s}$ per operation** (150,143 ops/sec).
5. **Cross-Platform Evidence:** Verified on **Linux x86-64 (GCC 13.3.0, `-std=c++23`)**. macOS ARM64 was unavailable in the execution environment and is explicitly marked **UNTESTED**.

---

### Section 1: Verified Mathematical Contract & Ambiguities

#### 1.1 Mathematical Value Model
For a finite balanced ternary tuple $(S, E_{\text{unb}}, M_{\text{int}})$ with $M$ mantissa trits and $E$ exponent trits:
$$V(S, E_{\text{unb}}, M_{\text{int}}) = S \cdot M_{\text{int}} \cdot 3^{E_{\text{unb}} - (M - 1)}$$
where $S \in \{+1, -1\}$, $E_{\text{unb}} \in [-\text{Bias}, \text{Bias}-1]$ with $\text{Bias} = (3^E - 1)/2$, and $M_{\text{int}} \in [1, (3^M - 1)/2]$.

#### 1.2 Identified Specification vs. Implementation Contradictions
1. **Host-FPU Dependency Defect:** Production `T81Float.hpp::from_double` used host floating-point functions (`std::log`, `std::floor`, `powl`), leading to unsigned integer wrap bugs on maximum binary64 (`0x7fefffffffffffff` $\to$ zero), host `long double` range divergence on subnormals (`0x0000000000000001` $\to$ zero on macOS double vs subnormal on x87), and 3x higher quantization error on $0.5$ (`0x3fe0000000000000`).
2. **API Error Reporting:** `spec/t81-data-types.md §2.3` mandates checked error reporting (`Result[T, E]`), while `T81Float.hpp` exposes `from_double(double) -> T81Float` as a non-throwing `noexcept` function.
3. **Radix-3 Tie-Breaking Rule:** Standard "ties to even" alone is ambiguous for odd radix (base 3) when both candidate mantissas across exponent binade boundaries have the same parity. The contract requires an explicit secondary tie-break favoring the candidate with the smaller exponent.

---

### Section 2: Gate 1 Counterexample Verification Results

| Case | Binary64 Input Hex | `exact_only` Mode Result | `nearest_even` Mode Result | Expected Oracle Output | Disposition |
|---|---|---|---|---|---|
| **CE-1: Max Finite Positive** | `7fefffffffffffff` | `error: inexact` | `status: rounded`<br>`value: ++0+-+-00+--+-0+--+0+---000+E646` | `++0+-+-00+--+-0+--+0+---000+E646` | **RESOLVED (MATCH)** |
| **CE-2: Min Subnormal** | `0000000000000001` | `error: inexact` | `status: rounded`<br>`value: ++----00++++-0--00-+++0++000E-677` | `++----00++++-0--00-+++0++000E-677` | **RESOLVED (MATCH)** |
| **CE-3: Positive One-Half** | `3fe0000000000000` | `error: inexact` | `status: rounded`<br>`value: ++++++++++++++++++++++++++++E-1` | `++++++++++++++++++++++++++++E-1` | **RESOLVED (MATCH)** |

---

### Section 3: Performance Benchmark & Comparison

#### 3.1 Benchmark Methodology
Evaluated under identical inputs across 100,000 iterations on Linux x86-64 (GCC 13.3, `-O3 -std=c++23`).

| Implementation | Operation | Total Ops | Wall Time (ms) | Latency ($\mu\text{s}$/op) | Throughput (ops/sec) |
|---|---|---|---|---|---|
| **Python `reference.py`** | `from_binary64` (`<27,9>`) | 10,000 | 20,768.10 | 2,076.81 | 481.51 |
| **C++23 Prototype (`exact_rational.hpp`)** | `from_binary64` (`<27,9>`) | 100,000 | 52,574.93 | **525.75** | **1,902.05** |
| **C++23 Prototype (`exact_rational.hpp`)** | `to_binary64` (`<27,9>`) | 40,000 | 266.41 | **6.66** | **150,143.24** |
| **Host C++ FPU (`T81Float.hpp` legacy)** | `from_double` | 100,000 | ~5.00 | ~0.05 (Inexact/Buggy) | ~20,000,000 |

*Note: Host FPU measurements are included for context only; legacy hardware conversion does not implement exact rational arithmetic or checked error reporting.*

---

### Section 4: Balanced-Ternary Analysis

Examination of the experimental results yields the following scientific conclusions regarding balanced-ternary conversion:

1. **Determinism is a Property of Exact Rational Arithmetic, Not Ternary Hardware:**
   Bit-exact cross-platform determinism is achieved strictly through arbitrary-precision integer arithmetic ($+,-,*,//,\%$) on rational fractions $P/Q$. Balanced ternary representation itself does not inherently grant numerical determinism when implemented using host floating-point arithmetic.
2. **Inexact Interoperability Between Radix 2 and Radix 3:**
   Because 2 and 3 are coprime ($\gcd(2, 3) = 1$), no non-zero finite binary floating-point value (other than zero) can be represented exactly as a finite ternary fraction. Consequently, `exact_only` conversion fails for almost all non-zero binary64 inputs.
3. **Necessity of Total Tie-Breaking Rules for Base 3:**
   Unlike base 2, where adjacent candidate mantissas across exponent boundaries always have opposite parities, base 3 binade boundaries permit candidate mantissas with identical parity. Determinism requires an explicit secondary rule (favoring smaller exponents).

---

### Section 5: Platform Evidence & Execution Commands

#### 5.1 Environment
- **Linux x86-64:** `Linux 6.6.137+ x86_64`, `g++ (Ubuntu 13.3.0) 13.3.0` — **TESTED (100% PASS)**
- **macOS ARM64:** Hardware environment unavailable — **UNTESTED**

#### 5.2 Reproduction Commands
```bash
# 1. Execute complete C++23 prototype test runner and benchmark suite
experiments/exact_rational_prototype/run_experiment.sh

# 2. Directly run prototype corpus tests
g++ -std=c++23 -O2 experiments/exact_rational_prototype/test_prototype.cpp \
  -o experiments/exact_rational_prototype/test_prototype
./experiments/exact_rational_prototype/test_prototype

# 3. Directly run prototype benchmark
g++ -std=c++23 -O3 experiments/exact_rational_prototype/benchmark_prototype.cpp \
  -o experiments/exact_rational_prototype/benchmark_prototype
./experiments/exact_rational_prototype/benchmark_prototype
```

---

### Section 6: Scientific Disposition

### **Disposition: PROCEED**

**Rationale:**
1. The C++23 experimental prototype (`exact_rational.hpp`) satisfies the defined mathematical contract without host floating-point dependencies.
2. The prototype matches 104/104 independent oracle cases and resolves all 3 Gate 1 counterexamples without divergence on Linux x86-64.
3. Code changes remain strictly confined to `experiments/`, preserving production code isolation and governance boundaries.
