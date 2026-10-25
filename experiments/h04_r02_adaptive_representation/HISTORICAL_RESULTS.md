# H04-R02 Historical Experimental Results (Merged PR #569)

**Document ID:** `experiments/h04_r02_adaptive_representation/HISTORICAL_RESULTS.md`
**Date:** October 2026 (Preserved Historical Artifact)
**Status:** Archived Baseline Data (Flawed / Superseded)

---

## 1. Published Summary Ledger Table ($C_3/C_2 = 1.0$, $M_{meta}=2$, $C_{trans}=1$, $C_{conv}=0.5$)

*Note: The table below represents the published figures in PR #569 before the adversarial validity audit. These figures contained mathematical errors (DEF-01 metadata corruption and DEF-02 division error).*

| Workload Family | FB Cost | AB Cost (Slots) | FT Cost | AT Cost (Slots) | Published Payload $C_3/C_2$ Break-even | Published Total Cost $C_3/C_2$ Break-even |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **`constant_small`** | 6400 | 1050 (800) | 4100 | 850 (600) | $1.3333$ ($4/3$) | $1.6667$ ($5/3$) |
| **`constant_large`** | 6400 | 6650 (6400) | 4100 | 4350 (4100) | $1.5610$ ($64/41$) | $1.6098$ ($66/41$) |
| **`phased`** | 6400 | 3253 (3000) | 4100 | 2228 (1975) | $1.5190$ ($120/79$) | $1.6203$ |
| **`alternating`** | 6400 | 4147 (3600) | 4100 | 2897 (2350) | $1.5319$ | $1.6170$ |
| **`boundary_oscillation`**| 6400 | 1549 (1200) | 4100 | 850 (600) | $2.0000$ | $2.3333$ |
| **`signed_extremes`** | 6400 | 3405 (3056) | 4100 | 2359 (2010) | $1.5204$ | $1.6199$ |
| **`sparse_large`** | 6400 | 1171 (912) | 4100 | 929 (670) | $1.3612$ | $1.6597$ |
| **`adversarial_transition`**| 6400 | 3397 (3000) | 4100 | 2372 (1975) | $1.5190$ | $1.6203$ |

---

## 2. Identified Defect Annotations

1. **Total Cost Break-Even Error (DEF-02):** The published $1.6667$ figure for `constant_small` was calculated as $(1050 - 50)/600$, incorrectly excluding metadata from the denominator while including it in the numerator.
2. **Metadata Corruption (DEF-01):** AT total cost scaled metadata bits by $C_3/C_2$, contaminating break-even surfaces as $C_3/C_2$ varied.
3. **Capacity Asymmetry (DEF-03):** The $2.0000$ boundary oscillation break-even resulted from 6-trit Tier 0 having $2.85\times$ the capacity of 8-bit Tier 0.
