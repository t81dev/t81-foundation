# H04-R02 Corrected Experimental Results and First-Principles Break-Even Analysis

**Document ID:** `experiments/h04_r02_adaptive_representation/CORRECTED_RESULTS.md`
**Date:** October 2026 (Audit Corrected Release)
**Status:** Canonical Corrected Data

---

## 1. Corrected Summary Ledger Table ($C_3/C_2 = 1.0$, $M_{meta}=2$, $C_{trans}=1$, $C_{conv}=0.5$)

*All calculations below are derived strictly from first principles under uncorrupted metadata accounting ($M_{meta} = 2.0$ bits/block).*

| Workload Family | FB Cost | AB Cost (Slots) | FT Cost | AT Cost (Slots) | Corrected Payload $C_3/C_2$ Break-even | Corrected Total Cost $C_3/C_2$ Break-even |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **`constant_small`** | 6400 | 1050 (800) | 4100 | 850 (600) | $1.3333$ ($4/3$) | $1.3333$ ($4/3$) |
| **`constant_large`** | 6400 | 6650 (6400) | 4100 | 4350 (4100) | $1.5610$ ($64/41$) | $1.5610$ ($64/41$) |
| **`phased`** | 6400 | 3253 (3000) | 4100 | 2228 (1975) | $1.5190$ ($120/79$) | $1.5190$ ($120/79$) |
| **`alternating`** | 6400 | 4147 (3600) | 4100 | 2897 (2350) | $1.5319$ | $1.5319$ |
| **`boundary_oscillation` (8-bit AB)**| 6400 | 1549 (1200) | 4100 | 850 (600) | $2.0000$ | $2.1650$ |
| **`boundary_oscillation` (10-bit AB)**| 6400 | 1250 (1000) | 4100 | 850 (600) | $1.6667$ | $1.6667$ |
| **`signed_extremes`** | 6400 | 3405 (3056) | 4100 | 2359 (2010) | $1.5204$ | $1.5204$ |
| **`sparse_large`** | 6400 | 1171 (912) | 4100 | 929 (670) | $1.3612$ | $1.3612$ |
| **`adversarial_transition`**| 6400 | 3397 (3000) | 4100 | 2372 (1975) | $1.5190$ | $1.5190$ |

---

## 2. Key First-Principles Discoveries

1. **Equivalence of Payload and Total Cost Break-Even:**
   When metadata overhead ($M_{meta}$) and transition penalties ($T_{trans}$) are identical for AB and AT, setting $\text{Cost}_{AB} = \text{Cost}_{AT}$ yields:
   $$S_{AB} + M_{meta} + T_{trans} + C_{conv} = S_{AT} \cdot (C_3/C_2) + M_{meta} + T_{trans} + C_{conv}$$
   $$S_{AB} = S_{AT} \cdot (C_3/C_2) \implies (C_3/C_2)_{break-even} = \frac{S_{AB}}{S_{AT}}$$
   Thus, under fair control accounting, the total cost break-even point is **identically equal** to the payload slot ratio $S_{AB} / S_{AT}$.

2. **Physical Feasibility Limit:**
   In physical binary hardware, packing 1 trit requires at least $\log_2(3) \approx 1.58496$ bits (or $2.0$ bits in 2-bit packed representation).
   Since all non-asymmetric workload break-even ratios ($1.3333 - 1.5610$) are strictly less than $\log_2(3) \approx 1.585$, **adaptive balanced ternary cannot achieve a storage or cost advantage over adaptive binary in any physically realizable hardware encoding**.

3. **Symmetric Comparator Evaluation:**
   Providing binary with a 10-bit mode tier ($M=1024$, range $[-512, 511]$) matches Tier 0 information capacity ($M=729$) without asymmetry. Under 10-bit mode on `boundary_oscillation`, the break-even ratio drops from $2.0000$ (or $2.1650$) to $1.6667$, which still exceeds realistic physical hardware encoding bounds.
