# Scientific Investigation 07 — Ternary Dot-Product Hardware Feasibility & Matched Binary Baselines

**Repository:** T81 Foundation
**Research Foundation:** Scientific Investigation 05, Investigation 06 (PR #593, PR #594), Hypothesis H-T1
**Author:** Independent Digital Arithmetic Architect & RTL Verification Engineer
**Scope:** `/experiments/ternary_hardware_feasibility/`

---

## 1. Executive Summary

This investigation evaluates **Hypothesis H-T1** (Bit-Parallel Unpacked Ternary Dot-Product Custom Hardware Overlay) to determine if a hardware datapath optimized for ternary weights $W \in \{-1, 0, +1\}$ demonstrates a reproducible, measurable efficiency advantage over an equally optimized conventional binary baseline.

### Primary Scientific Finding
Comparing Design C against Design A (a general-purpose INT8 multiplier array) shows a **$6.11\times$ area reduction**, confirming that restricting weights to $\{-1, 0, +1\}$ yields massive savings by eliminating full multiplier trees.

However, when compared against **Design B** (the critical scientific control: a conventional binary design optimized specifically for weights restricted to $\{-1, 0, +1\}$), Design C achieves only a **$1.098\times$ area ratio** ($R_{\text{area}} = \text{Area}_B / \text{Area}_C \approx 1.10$).

This falls significantly short of the mandatory **$1.25\times$ stopping criteria threshold** established in Investigation 06 / PR #594. Furthermore, the minor difference (~10%) is an artifact of input encoding interface (unpacked sign/mask bitplanes vs. 2-bit standard binary code) rather than an intrinsic mathematical or arithmetic advantage of ternary logic.

### Final Scientific Disposition
**`NOT SUPPORTED`** — Design C provides no meaningful architectural advantage over an equally optimized binary baseline (Design B) under matched conditions. Per the explicit stopping criteria of PR #594, all custom ternary hardware research within the T81 Foundation is formally **CLOSED AND TERMINATED**.

---

## 2. Mathematical Contract & Reference Oracle

### 2.1 Operation Specification
$$Y = \sum_{i=0}^{N-1} W_i X_i$$
- **Weights ($W_i$):** $W_i \in \{-1, 0, +1\}$
- **Activations ($X_i$):** $X_i \in [-128, 127]$ (8-bit signed Two's Complement INT8)
- **Accumulation:** Exact integer arithmetic; saturation and wraparound are prohibited.

### 2.2 Accumulator Width Derivation
For vector length $N = 4096$:
- Maximum positive accumulator contribution per element: $(-1) \cdot (-128) = +128$.
- Maximum positive dot-product $Y_{\text{max}} = 4096 \cdot 128 = +524,288$.
- Maximum negative dot-product $Y_{\text{min}} = 4096 \cdot (-128) = -524,288$.

A signed two's complement integer of width $B$ bits spans $[-2^{B-1}, 2^{B-1} - 1]$.
- For $B = 20$ bits: range is $[-524,288, +524,287]$ (exceeds upper boundary $+524,288$ by $1$).
- For $B = 21$ bits: range is $[-1,048,576, +1,048,575]$.

Thus, **21 bits** is the minimum sufficient signed accumulator width for $N = 4096$. All three hardware designs and the independent software reference oracle standardize on a **32-bit signed accumulator** ($B_{\text{acc}} = 32$).

### 2.3 Independent Software Reference Oracle
An independent reference oracle (`reference_oracle.py`) was implemented in Python using standard Python integer arithmetic without simulating hardware bitplanes or logic gates. The oracle contains automated generators for directed edge cases (including `INT8_MIN` negation, zero-heavy vectors, dense vectors, and alternating sign patterns) and deterministic randomized inputs.

---

## 3. Hardware Datapath Implementations

Three parameterized Verilog-2012 hardware datapaths were constructed under identical structural assumptions ($N$-element reduction tree, 32-bit exact accumulation):

### 3.1 Design A — General Binary INT8 MAC
- **Input Encoding:** $X_i \in \text{INT8}$, $W_i \in \text{INT8}$ (8-bit two's complement).
- **Datapath:** $N$ full 8-bit $\times$ 8-bit signed multipliers feeding a 32-bit binary adder reduction tree.
- **Purpose:** Measures the baseline benefit of restricting weights to three values versus general matrix multiplication hardware.

### 3.2 Design B — Optimized Binary Ternary-Weight MAC (Critical Scientific Control)
- **Input Encoding:** $X_i \in \text{INT8}$, $W_i \in \{2'b00: 0, 2'b01: +1, 2'b11: -1\}$.
- **Datapath:** Sign-extends $X_i$ to 32 bits, performs conditional two's complement negation (`-X_ext`), applies zero-gating, and feeds a 32-bit binary adder reduction tree.
- **Purpose:** Represents conventional binary logic optimized for the restricted weight set $\{-1, 0, +1\}$.

### 3.3 Design C — Proposed T81 H-T1 Ternary-Weight Datapath
- **Input Encoding:** $X_i \in \text{INT8}$, $W_{\text{mask}} \in \{0, 1\}^N$, $W_{\text{sign}} \in \{0, 1\}^N$.
- **Datapath (per PR #594 spec):**
  1. $X_{\text{ext}}[i] = \text{SignExtend}_{32}(X[i])$
  2. $M_i = \text{Replicate}_{32}(W_{\text{sign}}[i])$
  3. $T_{\text{signed}}[i] = (X_{\text{ext}}[i] \oplus M_i) + W_{\text{sign}}[i]$
  4. $T[i] = \text{Replicate}_{32}(W_{\text{mask}}[i]) \text{ AND } T_{\text{signed}}[i]$
  5. $Y = \sum_{i=0}^{N-1} T[i]$

---

## 4. Verification Evidence

Verification was conducted using an automated Icarus Verilog testbench (`tb_dot_product.v` / `run_verification.py`). All three hardware designs were simulated concurrently against the independent reference oracle across 127 directed and randomized test vectors.

### Test Categories Executed:
1. All zero inputs ($W=0, X=0$).
2. $+1$ weights with maximum positive activation ($+127$).
3. $+1$ weights with minimum negative activation ($-128$).
4. $-1$ weights with minimum negative activation ($-128 \to +128$ signed expansion).
5. $-1$ weights with maximum positive activation ($+127$).
6. Single-element non-zero combinations across all $W \in \{-1,0,1\}$ and $X \in \{-128, -127, -1, 0, 1, 126, 127\}$.
7. 50 randomized dense weight vectors (seed=42).
8. 50 randomized sparse weight vectors (90% zeros, seed=123).

```text
VERIFICATION COMPLETE: 127 tests executed, 0 errors.
Verification PASSED: All designs (A, B, C) match Reference Oracle identically.
```

---

## 5. Synthesis Results & Benchmark Comparison

Synthesis was performed using **Yosys 0.33** targeting a generic CMOS gate library (`abc -g gates`). All designs were synthesized across vector lengths $N = 16, 32, 64$ with identical optimization passes (`proc; opt; fsm; opt; memory; opt; techmap; opt; abc -g gates; opt`).

### Gate Count and CMOS Area Comparison Table

| Vector Length ($N$) | Design | Gate Cell Count | Estimated Gate Area | Area Ratio vs. Design A ($R_{\text{area, A}}$) | Area Ratio vs. Design B ($R_{\text{area, B}}$) |
| :--- | :--- | :---: | :---: | :---: | :---: |
| **$N = 16$** | **Design A** (General INT8) | 7,715 | 9,050.0 | $1.00\times$ | — |
| | **Design B** (Optimized Binary) | 1,430 | 1,581.5 | $5.72\times$ | $1.00\times$ |
| | **Design C** (H-T1 Ternary) | 1,188 | 1,436.5 | $6.30\times$ | **$1.101\times$** |
| **$N = 32$** | **Design A** (General INT8) | 15,419 | 18,062.5 | $1.00\times$ | — |
| | **Design B** (Optimized Binary) | 2,927 | 3,240.5 | $5.57\times$ | $1.00\times$ |
| | **Design C** (H-T1 Ternary) | 2,442 | 2,951.0 | $6.12\times$ | **$1.098\times$** |
| **$N = 64$** | **Design A** (General INT8) | 31,173 | 36,547.5 | $1.00\times$ | — |
| | **Design B** (Optimized Binary) | 5,924 | 6,564.5 | $5.57\times$ | $1.00\times$ |
| | **Design C** (H-T1 Ternary) | 4,948 | 5,980.5 | $6.11\times$ | **$1.098\times$** |

---

## 6. Evaluation of Hypothesis H-T1

### Area Ratios ($N=64$):
$$R_{\text{area, B/C}} = \frac{\text{Area}_B}{\text{Area}_C} = \frac{6,564.5}{5,980.5} = 1.0976\times$$

$$R_{\text{area, A/C}} = \frac{\text{Area}_A}{\text{Area}_C} = \frac{36,547.5}{5,980.5} = 6.1111\times$$

### Analysis of Architectural Confounding Factors:
1. **Workload Specialization (Design A vs. Design B):** Restricting weights from arbitrary 8-bit integers to $\{-1, 0, +1\}$ yields a $5.57\times$ area reduction. This benefit is entirely due to workload specialization (eliminating 8x8 multipliers) and is achievable using conventional binary logic.
2. **Representation Artifact (Design B vs. Design C):** The minor ~9.8% area reduction of Design C over Design B is due to receiving unpacked Sign and Mask bitplanes ($W_{\text{mask}}, W_{\text{sign}}$) rather than a 2-bit code ($2'b00, 2'b01, 2'b11$). Design C offloads the decoding logic to the interface. Once decoded, both designs use identical adder reduction trees.
3. **Threshold Deficit:** Hypothesis H-T1 fails to achieve the mandatory $1.25\times$ area advantage over the optimized binary control (Design B).

### Unmeasured Metrics Acknowledgment:
- **Dynamic Power / Energy (TOPS/W):** Marked **`UNMEASURED`**. Precise gate-level power profiling requires calibrated cell libraries with parasitic RC extraction and toggle rate VCD waveforms.
- **Critical Path Frequency (GHz):** Marked **`UNMEASURED`**. Unconstrained combinational synthesis without clock constraints or technology timing arcs cannot yield defensible frequency estimates.

---

## 7. Conclusion & Final Recommendation

1. **Hypothesis Evaluation:** Hypothesis H-T1 is **`NOT SUPPORTED`**.
2. **Stopping Criteria Triggered:** PR #594 Criterion 2 mandates at least a $1.25\times$ area or energy efficiency advantage over optimized binary primitives. The measured $1.098\times$ ratio fails this threshold.
3. **Action:** Per T81 Foundation governance guidelines, all custom ternary hardware overlay research is **FORMALLY CLOSED AND TERMINATED**. Ternary representations within T81 remain strictly bounded to 2-bit model weight serialization and Base-81 content addressing in CanonFS.
