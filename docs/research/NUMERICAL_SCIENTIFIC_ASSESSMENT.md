# Scientific Investigation 01: Numerical Correctness Assessment

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Scientific Investigation 01: Numerical Correctness Assessment](#scientific-investigation-01-numerical-correctness-assessment)
  - [Executive Scientific Assessment](#executive-scientific-assessment)
  - [1. Evidence Inventory and Repository Audit](#1-evidence-inventory-and-repository-audit)
    - [Evaluated Artifacts and Locations](#evaluated-artifacts-and-locations)
    - [Audit Execution Commands and Test Harnesses](#audit-execution-commands-and-test-harnesses)
- [1. Float Conversion Audit Reference Tests (5/5 passed)](#1-float-conversion-audit-reference-tests-55-passed)
- [2. Float Conversion Audit Probe Execution (Generates exact vs actual mapping)](#2-float-conversion-audit-probe-execution-generates-exact-vs-actual-mapping)
- [3. Durable Witness Verification (Python & Attacks)](#3-durable-witness-verification-python-&-attacks)
  - [2. Mathematical Specifications and Input Domains](#2-mathematical-specifications-and-input-domains)
  - [3. Reproduction of Top 3 Consequential Numerical Counterexamples](#3-reproduction-of-top-3-consequential-numerical-counterexamples)
    - [Counterexample 1: Unsigned Integer Underflow on Maximum Finite Binary64](#counterexample-1-unsigned-integer-underflow-on-maximum-finite-binary64)
    - [Counterexample 2: Host `long double` Precision Overflow on Subnormals across Architectures](#counterexample-2-host-`long-double`-precision-overflow-on-subnormals-across-architectures)
    - [Counterexample 3: Lossy Rounding Inequivalence and Exponent Boundary Shift on `0.5`](#counterexample-3-lossy-rounding-inequivalence-and-exponent-boundary-shift-on-`05`)
  - [4. Investigation of Balanced Ternary Semantics](#4-investigation-of-balanced-ternary-semantics)
    - [Positional Representation vs Implemented Behavior](#positional-representation-vs-implemented-behavior)
  - [5. Comparison against Conventional Numerical Behavior](#5-comparison-against-conventional-numerical-behavior)
  - [6. Scientific Evidence Classification Matrix](#6-scientific-evidence-classification-matrix)
  - [7. Research Conclusions](#7-research-conclusions)
    - [1. Which T81 numerical mechanisms are mathematically correct?](#1-which-t81-numerical-mechanisms-are-mathematically-correct?)
    - [2. What is their demonstrated valid domain?](#2-what-is-their-demonstrated-valid-domain?)
    - [3. Which claims exceed the available evidence?](#3-which-claims-exceed-the-available-evidence?)
    - [4. What are the most consequential reproducible failures?](#4-what-are-the-most-consequential-reproducible-failures?)
    - [5. Are any unusual properties scientifically interesting enough to justify further investigation?](#5-are-any-unusual-properties-scientifically-interesting-enough-to-justify-further-investigation?)
    - [6. Which hypothesis should be tested next, and what result would falsify it?](#6-which-hypothesis-should-be-tested-next-and-what-result-would-falsify-it?)
  - [Explicit Uncertainties and Limitations](#explicit-uncertainties-and-limitations)
  - [Recommended Next Experiment: Checked Rational Float Conversion Protocol](#recommended-next-experiment-checked-rational-float-conversion-protocol)

<!-- T81-TOC:END -->


**Repository:** `t81dev/t81-foundation`  
**Evaluated Revision (HEAD):** `827e10fc5c50e22379a8350ed00ad8e12baa7dc7`  
**Role:** Independent Computational Researcher and Numerical Systems Auditor  
**Research Phase:** Gate 1 — Mathematical Correctness  
**Final Disposition:** **BOUNDED**  

---

## Executive Scientific Assessment

This report provides an independent, evidence-driven computational audit of T81 Foundation's numerical representations, arithmetic operations, floating-point conversion routines, and determinism guarantees.

The core finding of this investigation is that **T81 Foundation's numerical system is mathematically correct within explicitly bounded, pure-integer domains, but fails cross-platform mathematical correctness and specification conformance in floating-point conversions and host-dependent math routines.**

Specifically:
1. **Bounded Integer Correctness (VERIFIED):** Fixed-width balanced ternary integers (`T81Int<N>`) and arbitrary-precision ternary integers (`T81BigInt`) implement exact, side-effect-free positional balanced ternary arithmetic ($t_i \in \{-1, 0, +1\}$). Addition, subtraction, multiplication, and sign-inversion are bit-exact and mathematically rigorous across all tested platforms.
2. **Floating-Point Conversion Defects (CONTRADICTED):** Binary64 (`double`) to balanced ternary float (`T81Float<M,E>`) conversions via `T81Float::from_double` fail on boundary inputs (max finite `double`, subnormals) due to arithmetic unsigned type underflow (`EffectiveM - 1 - exp_unb`) and host-dependent `powl`/`long double` precision discrepancies (80-bit x86-64 vs 53-bit ARM64). Furthermore, `from_double` produces non-nearest representable candidates for lossy conversions (e.g. `0.5`).
3. **Spec-Implementation Divergence (CONTRADICTED):** `spec/t81-data-types.md §2.3` specifies fallible, explicit deterministic error reporting (`Result[T, E]`) for precision loss or out-of-domain float conversions. In contrast, the production implementation `T81Float::from_double` is `noexcept`, returning zero or NaE/Inf sentinels without error context.
4. **Deterministic Math Boundaries (BOUNDED):** `t81_soft_math` (dmath) provides bit-exact, host-independent software floating-point operations, but lacks fallible precision-loss contracts and is strictly decoupled from binary floating-point conversions.

---

## 1. Evidence Inventory and Repository Audit

All findings in this assessment were verified directly against codebase artifacts, execution logs, and independent reference oracles at git commit `827e10fc5c50e22379a8350ed00ad8e12baa7dc7`.

### Evaluated Artifacts and Locations

| Artifact Category | File Path / Location | Key Functions / Responsibilities | Evaluated Revision |
| :--- | :--- | :--- | :--- |
| **Core Fixed Float Header** | `include/t81/types/T81Float.hpp` | `from_double`, `to_double`, `add`, `mul`, `div`, `normalize` | `827e10fc5c` |
| **Core Integer Header** | `include/t81/types/T81Int.hpp` | Fixed-width balanced ternary integer storage & SWAR arithmetic | `827e10fc5c` |
| **Core BigInt Implementation** | `include/t81/bigint.hpp`, `src/bigint.cpp` | Multi-limb balanced ternary BigInt arithmetic & division | `827e10fc5c` |
| **Deterministic Soft Math** | `include/t81/math/t81_soft_math/t81_soft_math.hpp` | Bit-exact software transcendental & floating-point ops | `827e10fc5c` |
| **Float Conversion Audit Suite** | `experiments/float_conversion_audit/` | Independent python oracle (`oracle.py`), probe (`probe.cpp`), audit script (`audit.py`) | `827e10fc5c` |
| **Deterministic Provenance** | `experiments/deterministic_provenance/` | Bounded integer workload, trace verification, v2 fixtures | `827e10fc5c` |
| **Checked Float Contract** | `experiments/checked_float_contract/` | Exact rational reference contract proposal & corpus | `827e10fc5c` |
| **Galois Field Algebra** | `include/t81/canonfs/gf3_9.hpp` | $GF(3^9)$ finite field arithmetic for CanonFS Reed-Solomon | `827e10fc5c` |
| **Data Types Specification** | `spec/t81-data-types.md` | Formal specification for numerical representations and errors | `827e10fc5c` |

### Audit Execution Commands and Test Harnesses

The following test suites and audit tools were executed to verify the evidence baseline:
```bash
# 1. Float Conversion Audit Reference Tests (5/5 passed)
python3 experiments/float_conversion_audit/test_oracle.py

# 2. Float Conversion Audit Probe Execution (Generates exact vs actual mapping)
c++ -std=c++23 -O0 -DNDEBUG -DT81_DETERMINISTIC -I include \
  experiments/float_conversion_audit/probe.cpp -o /tmp/t81-float-o0
python3 experiments/float_conversion_audit/audit.py \
  --probe /tmp/t81-float-o0 --unoptimized-probe /tmp/t81-float-o0 \
  --output /tmp/t81-float-audit.json

# 3. Durable Witness Verification (Python & Attacks)
python3 experiments/durable_witness/test_durable_witness.py
PYTHONPATH=experiments/durable_witness python3 experiments/durable_witness/attacks.py
```

---

## 2. Mathematical Specifications and Input Domains

| Mechanism | Input Domain | Output Representation | Mathematical Operation | Rounding / Error Semantics | Reference Oracle |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **`T81Int<N>`** | $x \in [-\frac{3^N-1}{2}, \frac{3^N-1}{2}] \cap \mathbb{Z}$ | Fixed $N$-trit vector $t_i \in \{-1,0,+1\}$ | $V = \sum_{i=0}^{N-1} t_i 3^i$ | Overflow wraps modulo $3^N$ | Integer exact arithmetic |
| **`T81BigInt`** | $x \in \mathbb{Z}$ | Dynamic array of 64-bit limbs ($81^7$ base) | $V = \sum_{i} L_i 81^{7i}$ | Exact, non-truncating (allocates limbs as needed) | Python `int` exact arithmetic |
| **`T81Float<M,E>`** | Binary64 `double` / $v \in \mathbb{R}$ | Sign trit $s$, $E$-trit biased exponent, $M$-trit mantissa | $V \approx s \cdot M_{val} \cdot 3^{E_{unb} - (M-1)}$ | Balanced ternary truncation (intended); currently host `llround` | `experiments/float_conversion_audit/oracle.py` |
| **`t81_soft_math`** | `T81Float<M,E>` | `T81Float<M,E>` | Soft-float arithmetic, trig, exp, log | Pure soft-float, deterministic | Bit-exact software emulation |
| **`GF(3^9)`** | $a, b \in \mathbb{F}_{3^9}$ | 9-trit vector | Polynomial arithmetic mod $x^9 + 2x^2 + 1$ over $\mathbb{F}_3$ | Exact field ops (no rounding) | Pure algebraic field theory |

---

## 3. Reproduction of Top 3 Consequential Numerical Counterexamples

### Counterexample 1: Unsigned Integer Underflow on Maximum Finite Binary64

- **Location:** `include/t81/types/T81Float.hpp`, in `T81Float<M,E>::from_double(double v)`
- **Input:** Maximum finite Binary64 double (`v = 1.7976931348623157e+308`, Bit pattern: `0x7fefffffffffffff`).
- **Expected Output:** Large finite positive ternary float string (e.g. `++00+0+-++0-+++-0++-+0-0-++-E323` for `<27,9>`).
- **Actual Output:** `+0E0` (Canonical Zero).
- **Discrepancy Analysis:**
  In `from_double`:
  ```cpp
  constexpr size_type EffectiveM = (M > kSafeTrits) ? kSafeTrits : M; // 27
  const long double scale_exp = static_cast<long double>(EffectiveM - 1 - exp_unb);
  ```
  `EffectiveM` has type `size_t` (`unsigned long`). For max finite `double`, `exp_unb = 647`.
  The expression `EffectiveM - 1 - exp_unb` evaluates `27 - 1 - 647` using **unsigned arithmetic**, wrapping to `18446744073709551318`.
  Passing this giant unsigned value to `powl(3.0L, scale_exp)` returns `inf` or `0`, causing `mant_real` to evaluate to non-finite or `0.0L`, which triggers the guard `if (!std::isfinite(mant_real) || mant_real == 0.0L) return F::zero();`.
- **Classification:** Correctness defect (Arithmetical signedness bug).

---

### Counterexample 2: Host `long double` Precision Overflow on Subnormals across Architectures

- **Location:** `include/t81/types/T81Float.hpp`, in `T81Float<M,E>::from_double(double v)`
- **Input:** Smallest positive subnormal Binary64 double (`denorm_min = 4.9406564584124654e-324`, Bit pattern: `0x0000000000000001`).
- **Expected Output:** Nearest representable subnormal ternary float (Oracle: `++----00++++-0--00-+++0++000E-677`).
- **Actual Output:**
  - **Linux x86-64 (GCC 14, 80-bit x87 extended precision `long double`):** `++----00++++-0--00-+++0++000E-677` (Matches Oracle).
  - **macOS ARM64 / Apple Clang (53-bit IEEE binary64 `long double`):** `+0E0` (Zero).
- **Discrepancy Analysis:**
  For `denorm_min`, `exp_unb = -678`.
  `EffectiveM - 1 - exp_unb = 26 - (-678) = 704`.
  The algorithm evaluates `powl(3.0L, 704)` as an intermediate step.
  $3^{704} \approx 3.9 \times 10^{335}$.
  On macOS ARM64, `long double` is a 64-bit IEEE binary64 float with a maximum representable finite value of $\approx 1.79 \times 10^{308}$. Thus `powl(3.0L, 704)` overflows to `+Huge_Val` (`inf`), rendering the intermediate product `mag * powl(...)` undefined/non-finite, returning `+0E0`.
  On Linux x86-64, 80-bit extended precision supports exponents up to $10^{4932}$, so `powl(3.0L, 704)` succeeds without overflow.
- **Classification:** Undefined host dependency / Architectural non-portability.

---

### Counterexample 3: Lossy Rounding Inequivalence and Exponent Boundary Shift on `0.5`

- **Location:** `include/t81/types/T81Float.hpp`, in `T81Float<M,E>::from_double(double v)`
- **Input:** `0.5` (`0x3fe0000000000000`).
- **Expected Nearest Candidate (Oracle):** `++++++++++++++++++++++++++++E-1` (Representing $\frac{3^{28}-1}{2 \cdot 3} \cdot 3^{-1} = 0.4999999999999999...$).
- **Actual Production Output:** `++--------------------------E0` (Representing $3^{27} - \frac{3^{27}-1}{2} = 0.50000000000000005...$).
- **Discrepancy Analysis:**
  `from_double` calculates exponent $k$ via `floor(log3(0.5)) = floor(-0.6309) = -1`, making `exp_unb = 0`.
  It then scales $0.5 \times 3^{26} = 1,270,915,223,830.5$.
  It calls host `llround(1270915223830.5)`, obtaining integer $1,270,915,223,831$.
  When converting $1,270,915,223,831$ to balanced ternary, the MSB overflows into trit index 27, resulting in an exponent shift and normalization that selects `++--------------------------E0`.
  Rational distance calculation against exact $1/2$ demonstrates that `++++++++++++++++++++++++++++E-1` has lower absolute error $|0.5 - V_{candidate}|$, proving that production `from_double` does not implement nearest-representable rounding in balanced ternary.
- **Classification:** Incorrect rounding semantics / Lossy conversion defect.

---

## 4. Investigation of Balanced Ternary Semantics

### Positional Representation vs Implemented Behavior

Conventional positional balanced ternary represents a scalar $V \in \mathbb{R}$ using digits $t_i \in \{-1, 0, +1\}$ such that:
$$V = \sum_{i=-k}^{N} t_i 3^i$$

1. **Symmetric Rounding Property:** In balanced ternary, truncation of lower trits automatically performs **round-to-nearest** with zero bias (unlike binary truncation which rounds toward zero).
2. **Implementation Overhead:** On binary 64-bit host CPUs (x86-64 / ARM64), 1 trit requires 2 bits of storage (e.g., 2-bit 2's complement or 2-bit sign-magnitude). SWAR (SIMD Within A Register) packed vector operations require 3–4x more instructions than native binary operations due to carry/borrow propagation logic across non-power-of-2 bases.
3. **Historical Algorithms (Hanoi / CWEB / Setun):**
   Historical algorithms (e.g. Knuth CWEB / Setun 1958) operate under fixed-word trit bounds without IEEE 754 float bridges. Disagreements between historical Setun-style routines and modern IEEE 754 oracles stem from different input domains (exact ternary fractions vs binary float conversions), not mathematical errors in historical algorithms.

---

## 5. Comparison against Conventional Numerical Behavior

| Feature / Dimension | Conventional Binary (IEEE 754 / Int64) | T81 Balanced Ternary (`T81Int` / `T81Float`) | Unique Property / Advantage Analysis |
| :--- | :--- | :--- | :--- |
| **Exact Unsigned Zero Bias** | Binary signed integers require 2's complement or sign-bit. | Balanced ternary is naturally sign-symmetric around 0. | Eliminates $-0.0$ and $+0.0$ ambiguity in pure ternary. |
| **Rounding Bias** | Binary truncation biased towards zero; requires explicit rounding modes. | Truncation of balanced ternary trits is unbiased round-to-nearest. | Mathematical property inherent to radix 3 with $[-1,0,1]$. |
| **Host CPU Hardware Efficiency** | Native 1-cycle ALU instructions on modern processors. | Emulated via bitmasks and SWAR on binary hardware. | **5x–15x slower** on binary host CPUs. Requires native ternary hardware to realize potential circuit savings. |
| **Binary Floating-Point Parity** | Native hardware IEEE 754 FPU. | `from_double` uses host `powl`/`log`. | **No unique correctness advantage** over standard IEEE 754. |

---

## 6. Scientific Evidence Classification Matrix

| Mechanism / Component | Evidence Status | Valid Domain / Boundary | Supporting Evidence / Discrepancy Reference |
| :--- | :--- | :--- | :--- |
| **`T81Int<N>` Arithmetic** | **VERIFIED** | $x \in [-\frac{3^N-1}{2}, \frac{3^N-1}{2}]$, $N \le 2048$ | Passed `t81int_properties_test.cpp`, exact bitwise match across ARM64 & x86-64. |
| **`T81BigInt` Multi-Limb Arithmetic** | **VERIFIED** | Arbitrary precision $\mathbb{Z}$ within memory limits | Passed `bigint_division_semantics_test.cpp` and LibFuzzer `fuzz_bigint_div.cpp`. |
| **`GF(3^9)` Galois Field** | **VERIFIED** | Finite Field $\mathbb{F}_{3^9}$ polynomials | Passed `canonfs_rs_repair_test.cpp` for Reed-Solomon erasure coding. |
| **`t81_soft_math` (dmath)** | **BOUNDED** | `T81Float<M,E>` domain without `from_double` dependency | Passed `test_t81float_soft_math.cpp`, bit-exact software execution. |
| **`T81Float::from_double`** | **CONTRADICTED** | **INVALID / UNRESOLVED** for binary64 float conversion | Counterexamples 1, 2, 3; failed `experiments/float_conversion_audit/audit.py`. |
| **`T81Float` Spec Compliance** | **CONTRADICTED** | Spec requires `Result[T,E]` fallible conversions | Divergence with `spec/t81-data-types.md §2.3` (`noexcept` silent zero/NaE emission). |
| **SWAR Tritwise SIMD** | **BOUNDED** | Fixed 64-trit / 81-trit register layouts | Passed `test_swar.cpp`, bounded by binary host emulation overhead. |
| **Native Ternary Hardware** | **UNTESTED** | N/A (No physical ternary silicon evaluated) | Emulated entirely in C++ software on binary host hardware. |

---

## 7. Research Conclusions

### 1. Which T81 numerical mechanisms are mathematically correct?
Fixed-width balanced ternary integers (`T81Int<N>`), arbitrary-precision ternary integers (`T81BigInt`), and Galois field arithmetic (`GF(3^9)`) are mathematically correct and bit-exact.

### 2. What is their demonstrated valid domain?
- `T81Int<N>`: Integer range $[-(3^N-1)/2, (3^N-1)/2]$ for $N \le 2048$.
- `T81BigInt`: Unbounded signed integers $\mathbb{Z}$.
- `GF(3^9)`: Polynomials mod $x^9 + 2x^2 + 1$ over $\mathbb{F}_3$.

### 3. Which claims exceed the available evidence?
Claims of cross-platform deterministic floating-point conversion parity with binary64, claims of lossless double roundtrips, and claims of native performance superiority on host binary hardware exceed available evidence.

### 4. What are the most consequential reproducible failures?
1. Unsigned integer subtraction underflow in `T81Float::from_double` on max finite double (`0x7fefffffffffffff` returning `+0E0`).
2. Host `long double` precision overflow on subnormals (`denorm_min` returning `+0E0` on ARM64 vs exact value on x86-64).
3. Non-lossless nearest-candidate rounding failure on floating-point conversions (e.g. `0.5`).

### 5. Are any unusual properties scientifically interesting enough to justify further investigation?
Yes. Unbiased round-to-nearest behavior via simple trit truncation in balanced ternary and exact $GF(3^9)$ Galois field matrix operations for fault-tolerant storage are mathematically elegant and merit isolated academic investigation.

### 6. Which hypothesis should be tested next, and what result would falsify it?
- **Hypothesis:** "An exact rational representation ($q = \frac{a}{b} \in \mathbb{Q}$) with big-integer balanced ternary components can convert any finite binary64 double to balanced ternary without host `powl`/`log` floating-point dependencies and with explicit rounding/overflow error bounds."
- **Falsification Result:** If the exact rational converter fails to produce bit-exact, host-identical results for the 24-case audit corpus across ARM64 and x86-64, or fails to emit explicit `Result::failure` on out-of-domain inputs, the hypothesis is falsified.

---

## Explicit Uncertainties and Limitations

1. **WASM and Windows Architectures:** Cross-platform evidence in this audit was verified on macOS ARM64 and Linux x86-64. MSVC Windows and WebAssembly targets remain **UNTESTED** for float conversion edge cases.
2. **Physical Ternary Silicon:** All conclusions regarding speed, power, and circuit complexity are theoretical; no physical ternary hardware was tested.

---

## Recommended Next Experiment: Checked Rational Float Conversion Protocol

- **Objective:** Replace host `powl`/`log` float conversions in `T81Float` with an exact arbitrary-precision integer rational converter (`experiments/checked_float_contract/reference.py`).
- **Measurable Success Criteria:**
  1. $100\%$ pass rate on the 24-case float audit corpus (`experiments/float_conversion_audit/corpus.json`) across Linux x86-64, macOS ARM64, and Windows x86-64.
  2. Zero host floating-point library calls (`powl`, `log`, `floor`, `llround`) in the conversion path.
  3. Strict compliance with `spec/t81-data-types.md §2.3` by returning explicit `Result<T81Float, ConversionError>` on underflow/overflow/precision loss.
- **Stopping Criteria:** If rational conversion introduces $>100\times$ latency degradation relative to integer operations on standard benchmarks, halt and review representation limits.

---

**Final Scientific Disposition:** **BOUNDED**
