# H04-R02 Adversarial Audit Discrepancy Ledger

**Document ID:** `experiments/h04_r02_adaptive_representation/DISCREPANCY_LEDGER.md`
**Date:** October 2026
**Auditor:** Independent Scientific Audit
**Baseline:** PR #569 (Merge commit `01c85019319fd020922402ba356878e392cb2231`)

---

## 1. Discrepancy Ledger Summary

| ID | Category | Summary | Impact |
| :--- | :--- | :--- | :--- |
| **DEF-01** | Confirmed Mathematical Defect | Mode descriptor metadata bits ($M_{meta}$) were incorrectly scaled by $C_3/C_2$ for AT in C++ and Python oracle. | Contaminated AT total cost formulas and break-even calculations. |
| **DEF-02** | Confirmed Mathematical Defect | Published total-cost break-even ratios in `RESULTS.md` and C++ `main.cpp` contained numerical division errors. | Reported false break-even ratios (e.g. $1.6667$ instead of $1.3333$ for `constant_small`). |
| **DEF-03** | Comparator Fairness Defect | 6-trit Tier 0 ($M=729$) provided $2.85\times$ capacity over 8-bit Tier 0 ($M=256$), creating an asymmetric boundary advantage at $127/128$. | Imposed unequal mode tier capacities favoring ternary; 10-bit mode ($M=1024$) yields a ratio of $1.6667$, which exceeds $\log_2(3) \approx 1.585$. |
| **DEF-04** | Implementation Defect | C++ implementation in `adaptive_experiment.hpp` used greedy tiering instead of DP; Python `oracle.py` duplicated C++ metadata scaling error. | C++ did not minimize declared cost model under high transition costs; Python oracle failed independent verification requirement. |
| **DEF-05** | Confirmed Mathematical Defect | Exact 41-trit max bound was hardcoded to $2^{63}-1$ instead of $3^{41}$ capacity limit. | 41 trits covers $[-1.8236 \times 10^{19}, 1.8236 \times 10^{19}]$ ($1.977\times$ 64-bit signed int range). |
| **DEF-06** | Documentation Defect / Missing Evidence | H04-R01 commit `def47c68` cited in `HANDOFF.md` does not exist in git history. | Historical implementation evidence is missing/unavailable. |
| **DEF-07** | Unsupported Inference | Claim that "over 96% of savings arise from adaptation" generalized a single workload (`constant_small`) to all workloads. | On `constant_large`, adaptation savings are negative ($-12.2\%$), invalidating the general claim. |
| **DEF-08** | Unsupported Inference | Extrapolating symbolic storage slot counts ($C_3/C_2$) to total physical software execution performance or hardware efficiency. | Model is an abstract symbol-slot ledger, not a hardware measurement or execution timer. |

---

## 2. Detailed Findings and Discrepancy Analysis

### 2.1 DEF-01: Metadata Scaling Mathematical Defect
- **Description:** Mode descriptor metadata bits (2 bits per block) encode binary tier selection ($00, 01, 10, 11$). In `adaptive_experiment.hpp` and `oracle.py`, `meta_base` for arm AT was computed as `metadata_bits * c3_over_c2`.
- **Finding:** Metadata bits represent control overhead, not ternary payload symbols. Multiplying metadata bits by $C_3/C_2$ treats control metadata as hardware trits, artificially inflating metadata costs as $C_3/C_2$ increases.
- **Correction:** Metadata bits must remain $M_{meta} = \text{blocks} \cdot \text{meta\_bits\_per\_block} \cdot 1.0$ for all arms.

### 2.2 DEF-02: Published Break-Even Ratio Mathematical Defect
- **Description:** `RESULTS.md` published a total-cost break-even ratio of $1.6667$ ($5/3$) for `constant_small`.
- **Finding:** First-principles derivation of total-cost break-even:
  $$\text{Cost}_{AB} = 800 + 200 + 50 = 1050$$
  $$\text{Cost}_{AT}(C_3/C_2) = 600 \cdot (C_3/C_2) + 200 + 50 = 600 \cdot (C_3/C_2) + 250$$
  Setting $\text{Cost}_{AB} = \text{Cost}_{AT}$ yields $600 \cdot (C_3/C_2) = 800 \implies C_3/C_2 = 800/600 = 1.3333$ ($4/3$).
  The published figure of $1.6667$ was derived by calculating $(1050 - 50)/600 = 1000/600 = 1.6667$, which incorrectly included metadata in the numerator while excluding it from the denominator.
- **Correction:** Recompute all break-even ratios strictly from first principles in both C++ executable and Python oracle.

### 2.3 DEF-03: Comparator Capacity Asymmetry and Mathematical Interpretation
- **Description:** Tier 0 binary is 8-bit (range $[-128, 127]$, capacity $256$). Tier 0 ternary is 6-trit (range $[-364, 364]$, capacity $729$).
- **Finding:** On `boundary_oscillation` ($127, 128$), value $128$ forces 8-bit AB into Tier 1 (16-bit, 1200 bits total + 99 level transitions), whereas AT remains in Tier 0 (6 trits, 600 trits total + 0 transitions). This generates a payload ratio of $2.0$. Providing binary with a competitive 10-bit mode tier ($M=1024$, range $[-512, 511]$) keeps $127$ and $128$ in Tier 0b (1000 bits + 0 transitions), yielding a ratio of $1000/600 = 1.6667$.
- **Mathematical Distinction:** A break-even ratio of $1.6667$ exceeds $\log_2(3) \approx 1.585$. Therefore, $1.6667$ does not establish universal physical impossibility. Instead, it demonstrates that under unaligned fractional bit-packing without encoding overhead, a sub-byte/10-bit mode could theoretically achieve a ternary advantage. However, in realizable hardware encodings (e.g. 2-bit packed trits at $2.0$ bits/trit, or Base-243 at $1.60$ bits/trit plus SIMD decode latency), adaptive binary retains superior storage and execution efficiency.

### 2.4 DEF-04: DP vs Greedy Implementation Defect
- **Description:** C++ `adaptive_experiment.hpp` evaluated traces using greedy tier selection per element, whereas Python `oracle.py` implemented dynamic programming.
- **Finding:** Under high transition costs ($C_{trans} \gg 1$), greedy tiering repeatedly switches tiers and incurs massive penalties, whereas DP finds the global cost minimum by holding a higher tier across transient fluctuations.
- **Correction:** C++ `adaptive_experiment.hpp` was updated to implement true DP cost minimization matching `oracle.py`.

### 2.5 DEF-05: Representable Range Representation Limits
- **Description:** `TierLimits` in `adaptive_experiment.hpp` hardcoded Tier 3 ternary limits to $-2^{63}$ and $2^{63}-1$.
- **Finding:** 41 balanced trits represent $[-(3^{41}-1)/2, (3^{41}-1)/2] = [-18236498188585393201, 18236498188585393201]$, which is $1.977\times$ the signed 64-bit integer range.

### 2.6 DEF-06: Missing Historical Commit Evidence
- **Description:** `HANDOFF.md` cites commit `def47c68` (`research/h04-r01-trace-ledger`).
- **Finding:** Commit `def47c68` is absent from the git repository tree. Historical H04-R01 implementation evidence is unavailable.

### 2.7 DEF-07: Invalid Attribution Method Generalization
- **Description:** `RESULTS.md` claimed "over 96% of savings arise from adaptation" based on `constant_small`.
- **Finding:** On `constant_large`, FB cost = 6400, AB cost = 6650 (adaptation savings = $-250$, i.e. $-3.9\%$), AT cost = 4350 (savings = $2050$). Adaptation savings ratio is $-250 / 2050 = -12.2\%$. Generalizing the 96% figure across all workloads is mathematically invalid.

---

## 3. Scientific Reclassification

**Assigned Classification: Classification D — Invalid Experimental Methodology**

*Justification:* While H04-R02 correctly concluded that software balanced-ternary representation provides no advantage over adaptive binary in modern hardware, the underlying experimental methodology contained structural defects: metadata corruption (DEF-01), mathematical break-even calculation errors (DEF-02), comparator capacity asymmetry at boundary thresholds (DEF-03), unvalidated/corrupted oracle implementation (DEF-04), and invalid attribution generalization (DEF-07). Under corrected, mathematically fair bounds ($C_3/C_2 \ge \log_2(3) \approx 1.585$), adaptive binary remains superior or equal across all standard non-asymmetric workloads.
