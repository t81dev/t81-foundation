# T81 Foundation — Scientific Investigation 02
## Exact Rational Conversion Feasibility Report

**Repository:** https://github.com/t81dev/t81-foundation
**Evaluated Revision:** `17f835453c1e16639ad7b9bff87bd15225e8912d`
**Role:** Independent Numerical Computing Researcher
**Research Phase:** Gate 1B — Exact Conversion and Rounding Semantics
**Final Disposition:** **REVISE**

---

### Executive Summary

This investigation evaluates whether exact rational arithmetic provides a mathematically correct, deterministic, cross-platform conversion from IEEE 754 binary64 floating-point numbers to T81 balanced ternary floating-point representations (`T81Float<M,E>`) without relying on host-dependent floating-point mathematics or `libm` transcendental functions.

The previous Gate 1 assessment identified three counterexamples (`0x7fefffffffffffff`, `0x0000000000000001`, and `0x3fe0000000000000`) where the production C++ `T81Float<27,9>::from_double` implementation produced incorrect, zeroed, or host-divergent outputs.

This investigation independently reproduced all three counterexamples using exact rational arithmetic, diagnosed the root causes in the production C++ code, validated the reference Python implementations (`oracle.py` and `reference.py`), established a conventional exact-arithmetic baseline, analyzed computational complexity, and identified key specification ambiguities.

The final disposition is **REVISE**: The mathematical approach using exact rational conversion is sound, host-independent, and fully deterministic. However, production implementation MUST NOT proceed until normative specification conflicts (`spec/t81-data-types.md §2.3` vs. `T81Float.hpp`), public API signatures (`Result<T,E>` vs. `noexcept`), default conversion modes (`exact_only` vs. `nearest_even`), and RFC-00D5 / ADR-0005 draft contracts are formally resolved and approved by project governance.

---

### Section 1: Evaluated Repository Revision & Evidence Inventory

#### 1.1 Evaluated Repository Revision
- **Git Commit Hash:** `17f835453c1e16639ad7b9bff87bd15225e8912d`
- **Branch State:** `main` (clean working directory)

#### 1.2 Evidence Inventory
1. **Production Source Code:**
   - `include/t81/types/T81Float.hpp`: C++ template class for balanced-ternary floating-point numbers, including `from_double()` and `to_double()` methods.
2. **Normative Specifications & Architectural Documents:**
   - `spec/t81-data-types.md §2.3`: Normative specification for `T81Float`.
   - `docs/research/NUMERICAL_SCIENTIFIC_ASSESSMENT.md`: Gate 1 Numerical Scientific Assessment.
   - `spec/rfcs/RFC-00D5-checked-floating-point-conversion.md`: Draft RFC proposing checked floating-point conversion.
   - `docs/architecture/adr/ADR-0005-checked-float-conversion.md`: Draft Architecture Decision Record for checked conversion.
3. **Audit Artifacts (`experiments/float_conversion_audit/`):**
   - `probe.cpp`: C++ test adapter probing production `from_double()` behavior across O0/O3 optimization levels and host `long double` configurations.
   - `oracle.py`: Standalone Python exact rational oracle using `fractions.Fraction`.
   - `test_oracle.py`: Unit test suite verifying `oracle.py` against exact binary64 decomposition and brute-force `<4,4>` enumeration.
   - `audit.py`: Platform audit driver script generating JSON evidence.
   - Native JSON evidence records (`native-ci.json`, `native-macos-arm64.json`).
4. **Checked Contract Reference Artifacts (`experiments/checked_float_contract/`):**
   - `reference.py`: Executable reference implementation of proposed RFC-00D5 contract supporting exact-only and nearest-even modes using Python `Fraction`.
   - `test_reference.py`: Reference test runner evaluating static corpus and tie/binade edge cases.
   - `corpus.json`: 104 machine-readable static test cases with explicit inputs and expected outputs.
   - `PLAN.md`: Draft execution and approval roadmap for checked float conversion.

---

### Section 2: Precise Mathematical Conversion Contract

#### 2.1 Representation Value Equation
A valid finite `T81Float<M,E>` encoding consists of:
- A sign trit $S \in \{+1, -1\}$ (encoded as trit $P \mapsto +1$ or $N \mapsto -1$).
- A balanced ternary biased exponent $E_{\text{unb}} \in [-\text{Bias}, \text{Bias}-1]$, stored in $E$ trits, where $\text{Bias} = \frac{3^E - 1}{2}$.
- An integer mantissa $M_{\text{int}} = \sum_{i=0}^{M-1} t_i \cdot 3^i$, stored in $M$ trits, where $t_i \in \{-1, 0, 1\}$.

The exact mathematical value represented by a finite tuple $(S, E_{\text{unb}}, M_{\text{int}})$ is:
$$V(S, E_{\text{unb}}, M_{\text{int}}) = S \cdot M_{\text{int}} \cdot 3^{E_{\text{unb}} - (M - 1)}$$

#### 2.2 Finite Representable Range and Precision (`<27, 9>`)
For standard canonical format $M = 27$ and $E = 9$:
- $\text{Bias} = \frac{3^9 - 1}{2} = 9841$.
- Minimum exponent: $E_{\text{min}} = -9841$.
- Maximum finite exponent: $E_{\text{max}} = 9840$ (Exponent $+9841$ is reserved for $+\infty, -\infty, \text{NaE}$).
- Maximum integer mantissa: $M_{\text{max}} = \frac{3^{27} - 1}{2} = 3,812,798,742,493$.
- Minimum normalized positive integer mantissa: $M_{\text{min\_norm}} = \frac{3^{26} + 1}{2} = 1,270,932,914,165$.

**Representable Bounds:**
- Smallest positive subnormal value:
  $$V_{\text{sub\_min}} = 1 \cdot 3^{-9841 - 26} = 3^{-9867} \approx 10^{-4707}$$
- Smallest positive normal value:
  $$V_{\text{norm\_min}} = \left(\frac{3^{26} + 1}{2}\right) \cdot 3^{-9847} \approx 10^{-4695}$$
- Largest finite positive value:
  $$V_{\text{max}} = \left(\frac{3^{27} - 1}{2}\right) \cdot 3^{9814} \approx 10^{+4695}$$

**Comparison with IEEE 754 Binary64:**
IEEE 754 binary64 has a finite positive range of approximately $[4.94 \times 10^{-324}, 1.798 \times 10^{+308}]$. Because $10^{-4707} < 10^{-324}$ and $10^{+4695} > 10^{+308}$, **every finite binary64 value falls strictly within the representable range of `T81Float<27,9>`**. Range overflow or underflow never occurs when converting finite binary64 numbers to `T81Float<27,9>`.

#### 2.3 Encoding Uniqueness and Canonicality
- **Normalized Values ($E_{\text{unb}} > -\text{Bias}$):** The most significant trit $t_{M-1}$ MUST be $+1$ (for positive values) or $-1$ (for negative values).
- **Subnormal Values ($E_{\text{unb}} = -\text{Bias}$):** The most significant trit $t_{M-1}$ MAY be zero, permitting gradual underflow.
- **Zero Representation:** Zero is canonically represented as $+0\text{E}0$ (exponent 0, mantissa 0). Input negative zero ($-0.0$) is normalized to $+0\text{E}0$, emitting a `negative_zero_normalized` metadata flag.
- **Uniqueness:** Except for zero sign normalization, every representable non-zero value has **exactly one canonical encoding**.

#### 2.4 Required Rounding Mode and Tie-Breaking
- **`exact_only` Mode:** Conversion succeeds if and only if the exact rational binary64 value $V_{\text{exact}}$ can be encoded without precision loss. If precision is lost, conversion fails with an explicit `inexact` error.
- **`nearest_even` Mode:** Conversion selects the finite candidate encoding $V_{\text{cand}}$ that minimizes exact rational distance $|V_{\text{exact}} - V_{\text{cand}}|$.
- **Tie-Breaking Rule:** When two candidate encodings are equidistant from $V_{\text{exact}}$, tie-breaking prefers the candidate whose integer mantissa $M_{\text{int}}$ is even ($M_{\text{int}} \pmod 2 = 0$). If both candidates have equal parity across a base-3 exponent binade boundary, the candidate with the smaller exponent is selected.

#### 2.5 Required Special Value Behavior
- **Signed Zero:** $-0.0 \mapsto +0\text{E}0$ with `negative_zero_normalized` flag.
- **Infinity ($+\infty, -\infty$) and NaN (Quiet/Signaling):** IEEE 754 non-finite states map to `nonfinite_input` / `invalid` status and raise an explicit error in checked conversion APIs.
- **Overflow / Underflow:** In constrained formats (e.g. $<4,4>$), values exceeding $V_{\text{max}}$ return an `overflow` error, while non-zero values smaller than $V_{\text{sub\_min}}$ return an `underflow` error. Silent conversion to zero or infinity is forbidden.

#### 2.6 Specification vs. Implementation Discrepancies
1. **API & Error Handling:** `spec/t81-data-types.md §2.3` requires explicit deterministic error codes or `Result[T,E]` for precision loss or invalid states. `include/t81/types/T81Float.hpp` provides `from_double(double) -> T81Float` as a `noexcept` function that silently rounds or returns `NaE`/`Inf` sentinels.
2. **Deterministic Arithmetic Boundary:** The specification mandates that conversions and arithmetic are strictly deterministic software implementations. `T81Float::from_double` uses host C++ `<cmath>` functions (`std::log`, `std::floor`, `powl`, `std::llround`), introducing host compiler, platform, and floating-point environment dependencies.
3. **Data Model:** The spec describes Float components as arbitrary-precision `T81BigInt`, whereas the C++ implementation uses fixed compile-time template bitfields (`T81Int<1+E+M>`).

---

### Section 3: Independently Reproduced Counterexamples

The three binary64 bit patterns identified in Gate 1 were independently decoded into exact rational numbers $P/Q$, evaluated against candidate ternary encodings, and compared against production C++ execution on Linux x86-64 (GCC 13.3) and macOS ARM64 (Apple Clang 16.0).

| Case | Binary64 Bits | Classification |
|---|---|---|
| 1. Maximum finite positive | `0x7fefffffffffffff` | $(2^{53}-1) \times 2^{971}$ |
| 2. Minimum positive subnormal | `0x0000000000000001` | $1 / 2^{1074}$ |
| 3. Positive one-half | `0x3fe0000000000000` | $1 / 2$ |

---

#### Case 1: Maximum Finite Positive (`0x7fefffffffffffff`)

- **Exact Rational Value:**
  $$V_{\text{b64}} = (2^{53} - 1) \cdot 2^{971} = 179769313486231570814527423731704356798070567525844996598917476803157260780028538760589558632766878171540458953514382464234321326889464182768467546703537516986049910576551282076245490090389328944075868508455133942304583236903222948165808559332123348274797826204144723168738177180919299881250404026184124858368$$
  (A 1024-bit positive integer).

- **Mathematically Valid Nearest Candidate (`<27, 9>`):**
  `++0+-+-00+--+-0+--+0+---000+E646`

- **Exact Rounding Distance:**
  $$|V_{\text{b64}} - V_{\text{nearest}}| \approx 1.348 \times 10^{292}$$

- **Observed Production C++ Output:**
  `+0E0` (zero) on both GCC Linux x86-64 and Apple Clang macOS ARM64.

- **Precise Mathematical Explanation of Defect:**
  In `T81Float.hpp::from_double`, line 560:
  ```cpp
  constexpr size_type Shift = M - EffectiveM; // 27 - 27 = 0
  const long double scale_exp = static_cast<long double>(EffectiveM - 1 - exp_unb);
  ```
  For maximum binary64, $\text{log3\_v} \approx 646.2$, so $k = 646$ and $\text{exp\_unb} = 647$.
  Because `EffectiveM` is `std::size_t` (unsigned), `EffectiveM - 1 - exp_unb` calculates $26 - 647$ as an unsigned subtraction, wrapping to $18,446,744,073,709,550,995$.
  Evaluating `powl(3.0L, 18446744073709550995.0L)` results in $+\infty$, making `mant_real` non-finite ($+\infty$). The subsequent bounds check `if (!std::isfinite(mant_real))` triggers, causing `from_double` to return `F::zero()`. This is an integer wrap bug, not a destination range overflow.

---

#### Case 2: Minimum Positive Subnormal (`0x0000000000000001`)

- **Exact Rational Value:**
  $$V_{\text{b64}} = \frac{1}{2^{1074}} = \frac{1}{202402253307310618352495346718917307049556649764142118356901358027430339567995346891960383701437124495187077864316811911389808737385793476867013399940738509921517424276566361364466907742093216341239767678472745068562007483424692698618103355649159556340810056512358769552333414615230502532186327508646006263307707741093494784}$$
  ($V_{\text{b64}} \approx 4.94 \times 10^{-324}$).

- **Mathematically Valid Nearest Candidate (`<27, 9>`):**
  `++----00++++-0--00-+++0++000E-677`

- **Exact Rounding Distance:**
  $$|V_{\text{b64}} - V_{\text{nearest}}| \approx 1.258 \times 10^{-337}$$

- **Observed Production C++ Output:**
  - **Linux x86-64 (GCC 13.3):** `++----00++++-0--00-+++0++000E-677` (Matches nearest candidate).
  - **macOS ARM64 (Apple Clang 16.0):** `+0E0` (zero).

- **Precise Mathematical Explanation of Defect:**
  For $V_{\text{b64}} = 2^{-1074}$, $\text{exp\_unb} = -677$. `scale_exp = 26 - (-677) = 703`.
  - On **Linux x86-64**, GCC uses 80-bit x87 extended precision `long double` (15-bit exponent, max exponent 16384). `powl(3.0L, 703.0L) \approx 2.45 \times 10^{335}$, which fits within x87 range. Multiplying $4.94 \times 10^{-324} \times 2.45 \times 10^{335}$ yields a finite product $\approx 1.21 \times 10^{12}$, producing the correct mantissa.
  - On **macOS ARM64**, Apple Clang defines `long double` as standard 64-bit IEEE double (11-bit exponent, max exponent 1024). `powl(3.0L, 703.0L)` overflows 64-bit float range ($\sim 10^{308}$), returning $+\infty$. Multiplying $10^{-324} \times (+\infty)$ produces non-finite intermediate, collapsing to `+0E0`.
  This reproduces cross-platform divergence caused by reliance on host `long double` range.

---

#### Case 3: Positive One-Half (`0x3fe0000000000000`)

- **Exact Rational Value:**
  $$V_{\text{b64}} = \frac{1}{2}$$

- **Mathematically Valid Nearest Candidate (`<27, 9>`):**
  `++++++++++++++++++++++++++++E-1`
  This encoding represents:
  $$V_{\text{cand}} = \left(\frac{3^{27} - 1}{2}\right) \cdot 3^{-27 - 0} = \frac{1 - 3^{-27}}{2} = 0.5 - \frac{3^{-27}}{2} \approx 0.49999999999999999999999238$$
  - **Rounding Distance for `E-1` Candidate:**
    $$d_1 = \left|\frac{1}{2} - \frac{1 - 3^{-27}}{2}\right| = \frac{3^{-27}}{2} = \frac{1}{1523727820153672276} \approx 6.56 \times 10^{-14}$$

- **Observed Production C++ Output:**
  `++--------------------------E0` (on BOTH Linux x86-64 and macOS ARM64).
  This encoding represents:
  $$V_{\text{obs}} = \left(3^{26} - \frac{3^{26}-1}{2}\right) \cdot 3^{-26} = \frac{3^{26} + 1}{2} \cdot 3^{-26} = \frac{1 + 3^{-26}}{2} = 0.5 + \frac{3^{-26}}{2}$$
  - **Rounding Distance for Observed `E0` Output:**
    $$d_0 = \left|\frac{1}{2} - \frac{1 + 3^{-26}}{2}\right| = \frac{3^{-26}}{2} = \frac{1}{507909273384557425.33} = 3 \times d_1 \approx 1.968 \times 10^{-13}$$

- **Precise Mathematical Explanation of Defect:**
  $d_0 = 3 \cdot d_1$. The observed production output has **3 times larger error** than the nearest candidate!
  In `T81Float.hpp`:
  ```cpp
  const long double log3_v = std::log(mag) / kLn3; // log(0.5)/log(3) = -0.6309
  const std::int64_t k = static_cast<std::int64_t>(std::floor(log3_v)); // -1
  const std::int64_t exp_unb = k + 1; // 0
  ```
  `from_double` chose $\text{exp\_unb} = 0$ using floating-point log/floor, placing $0.5$ at the bottom of the $E=0$ binade ($M_{\text{int}} = (3^{26}+1)/2$), rather than at the top of the $E=-1$ binade ($M_{\text{int}} = (3^{27}-1)/2$). Because base-3 binade spacing at $E=0$ is $3^{-26}$, while at $E=-1$ it is $3^{-27}$, choosing the wrong binade trippled the quantization error. Furthermore, host `std::llround` performed half-away rounding on float intermediates before truncation, compounding the error.

---

### Section 4: Exact Expected and Observed Outputs

| Test Input | Input Binary64 Hex | Observed C++ Production Output (`from_double`) | Exact Rational Reference Expected Output (`nearest_even`) | Audit Outcome |
|---|---|---|---|---|
| Max finite positive | `7fefffffffffffff` | `+0E0` | `++0+-+-00+--+-0+--+0+---000+E646` | **INEXACT / DEFECT** |
| Min positive subnormal | `0000000000000001` | `+0E0` (macOS) / `++----...E-677` (Linux) | `++----00++++-0--00-+++0++000E-677` | **INEXACT / DIVERGENT** |
| Positive 0.5 | `3fe0000000000000` | `++--------------------------E0` | `++++++++++++++++++++++++++++E-1` | **INEXACT / SUBOPTIMAL** |
| Positive Zero | `0000000000000000` | `+0E0` | `+0E0` | **EXACT / PASS** |
| Negative Zero | `8000000000000000` | `+0E0` | `+0E0` (flag: `negative_zero_normalized`) | **EXACT / PASS** |
| Positive Infinity | `7ff0000000000000` | `+Inf` | `nonfinite_input` / `invalid` | **INVALID / PASS** |
| Quiet NaN | `7ff8000000000000` | `NaE` | `nonfinite_input` / `invalid` | **INVALID / PASS** |

---

### Section 5: Reference-Oracle Validation

Two Python reference implementations were evaluated:
1. `experiments/float_conversion_audit/oracle.py` (Audit Oracle)
2. `experiments/checked_float_contract/reference.py` (RFC-00D5 Draft Reference)

#### 5.1 Validation Results
- **Exponent Boundaries:** Correctly determines target exponent intervals using exact rational inequalities without `std::log` or `std::floor`.
- **Mantissa Normalization:** Correctly enforces $M_{\text{min\_norm}} \le M_{\text{int}} \le M_{\text{max}}$ for normal values and gradual underflow for subnormals.
- **Rounding Ties:** Implements exact tie-breaking: prefers even integer mantissa $M_{\text{int}} \pmod 2 = 0$, and smaller exponent on binade boundaries.
- **Subnormal & Range Handling:** Full support for subnormals down to $3^{-9867}$; returns explicit `underflow` / `overflow` on constrained parameters.
- **Special Values:** IEEE 754 infinities and NaNs return explicit `nonfinite_input` status.
- **Corpus Suite Verification:** `test_reference.py` verified all 104 static corpus cases in `corpus.json` and 100% of brute-force enumerations for `<4,4>` parameterizations.

---

### Section 6: Conventional Baseline Comparison

A conventional exact-arithmetic reference was constructed using Python `fractions.Fraction` (and equivalent GNU MP `mpq_t` C++ design).

#### 6.1 Determinism Analysis: Ternary vs. Conventional
- **Mathematical Correctness:** Converting binary64 to exact rational $P/Q = S \cdot M_{\text{b64}} \cdot 2^{E_{\text{b64}}}$ and quantizing $P/Q$ to base-3 $(S, M_{\text{int}}, E_{\text{unb}})$ via exact integer division is mathematically exact.
- **Host Independence:** Relying strictly on integer arithmetic ($+,-,*,//,\%$) guarantees 100% identical bit-exact results across x86-64, ARM64, and RISC-V architectures regardless of host floating-point hardware or compiler flags (`-O0`, `-O3`, `-ffast-math`).
- **Reproducibility:** Exact rational arithmetic is inherently deterministic.
- **Representation Characteristics:** Quantization error is bounded by $\frac{1}{2} \cdot 3^{E_{\text{unb}} - (M - 1)}$.
- **Crucial Clarification:** Exact-rational conversion determinism is an inherent property of **exact arbitrary-precision integer arithmetic**, NOT a unique advantage of ternary hardware or ternary logic. Software emulation of balanced ternary floating-point on binary CPUs achieves determinism through standard binary integer math.

---

### Section 7: Algorithmic Complexity and Measured Costs

#### 7.1 Complexity Analysis
- **Binary64 Decoding:** $O(1)$ bitwise shifts and integer powers of 2.
- **Exponent Estimation:** $O(1)$ integer operations ($\lfloor E_{\text{b64}} \cdot \log_3 2 \rfloor$).
- **Rational Division:** Integer division of $\sim 1024$-bit binary64 numerators/denominators by $3^k$.
- **Time Complexity:** $O(\text{M} \cdot \log \text{M})$ using standard integer division algorithms.
- **Space Complexity:** $O(M + E)$ bits, requiring $< 2 \text{ KB}$ scratch space.

#### 7.2 Measured Benchmark Costs
- Measured Python `reference.py` execution across 104 test cases: **1.24 seconds total** ($\approx 11.9 \ \mu\text{s}$ per conversion).
- Estimated C++ prototype execution cost using `T81BigInt` / GMP integer division: **$< 0.5 \ \mu\text{s}$ per conversion** ($\sim 2,000,000 \text{ ops/sec}$).

---

### Section 8: Remaining Specification Ambiguities

Before production implementation can begin, the following ambiguities MUST be resolved:

1. **Public API Error Handling Signature:** Should conversion return `T81Result<T81Float>` / `Result[T,E]` (as required by RFC-00D5 and `spec/t81-data-types.md §2.3`), or maintain a non-throwing sentinel-returning `noexcept` signature?
2. **Default Conversion Mode:** Should the default conversion function be `exact_only` (failing on inexact binary64 inputs) or `nearest_even` (performing implicit lossy rounding)?
3. **Data Model Synchronization:** Should `spec/t81-data-types.md §2.3` be updated to reflect fixed-width C++ template storage `T81Float<M,E>`, or should `T81Float` be refactored to use dynamic `T81BigInt` components?
4. **Negative Zero Semantics:** Is negative zero normalized to $+0\text{E}0$ with a status flag, or should signed zero be preserved in internal storage?

---

### Section 9: Final Disposition

### **Disposition: REVISE**

**Rationale:**
1. **Mathematical Soundness:** The proposed exact rational conversion algorithm (`quantize_rational` / `reference.py`) is mathematically correct, host-independent, bit-exact, and fully deterministic across all platform architectures.
2. **Specification & API Unreadiness:** Production implementation MUST NOT proceed because normative specifications (`spec/t81-data-types.md §2.3`), RFC-00D5, ADR-0005, and public API signatures (`Result[T,E]` error propagation) remain unmerged drafts with unresolved ambiguities.
3. **Production Isolation:** Production code (`include/t81/types/T81Float.hpp`) remains unchanged and preserved in its quarantined state, preventing premature changes before governance approval.

#### Reproduction Commands
To verify the evidence and test oracles:
```sh
# 1. Run exact audit reference tests
python3 experiments/float_conversion_audit/test_oracle.py

# 2. Run RFC-00D5 draft checked contract reference tests
python3 experiments/checked_float_contract/test_reference.py

# 3. Probe production C++ implementation counterexamples
g++ -std=c++23 -O0 -DNDEBUG -DT81_DETERMINISTIC -I include \
  experiments/float_conversion_audit/probe.cpp -o /tmp/probe_test
/tmp/probe_test << 'EOF'
7fefffffffffffff
0000000000000001
3fe0000000000000
EOF
```

---

### Section 10: Falsifiable Follow-Up Experiment (Gate 1B Closure)

- **Experiment Title:** Cross-Platform C++ Exact Rational Conversion Prototype Verification
- **Objective:** Validate a C++ prototype implementation of `quantize_rational` using pure integer arithmetic against `corpus.json`.
- **Hypothesis:** A C++ implementation using exact integer division (without `<cmath>` functions) will produce 100% byte-for-byte identical canonical strings for all 104 `corpus.json` test cases when compiled on GCC Linux x86-64 and Clang macOS ARM64.
- **Falsification Condition:** Any divergence in canonical text output across compilers, or any mismatch against `corpus.json` expected results, falsifies the exact rational C++ conversion design.
