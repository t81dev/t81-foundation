# H04-R02 Experimental Results, Break-Even Ratios, and Discrepancy Reconciliation

**Document ID:** `experiments/h04_r02_adaptive_representation/RESULTS.md`
**Date:** October 2026 (Audit Corrected)
**Status:** Canonical Release (Supersedes PR #569)

---

## 1. Executive Summary

An adversarial validity audit of PR #569 identified 8 defects in the published H04-R02 experiment (see `DISCREPANCY_LEDGER.md`). Original historical results are archived in `HISTORICAL_RESULTS.md`. Corrected canonical figures derived from first principles are documented in `CORRECTED_RESULTS.md`.

The audit reclassifies H04-R02 from **Classification B** to **Classification D (Invalid Experimental Methodology)** due to structural defects in original control accounting and comparator capacity symmetry. However, the scientific conclusion remains firm: **adaptive balanced ternary provides no storage, operational, or physical performance advantage over adaptive binary in modern binary hardware or physically realizable encodings.**

---

## 2. Corrected Summary Ledger Table ($C_3/C_2 = 1.0$, $M_{meta}=2$, $C_{trans}=1$, $C_{conv}=0.5$)

| Workload Family | FB Cost | AB Cost (Slots) | FT Cost | AT Cost (Slots) | Payload $C_3/C_2$ Break-even | Total Cost $C_3/C_2$ Break-even |
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

## 3. Key Findings

1. **Information-Theoretic Limit:** All non-asymmetric break-even ratios ($1.3333 - 1.5610$) are below $\log_2(3) \approx 1.585$. Any hardware encoding requiring $\ge 1.585$ bits per trit renders adaptive binary superior.
2. **Attribution Method Boundaries:** The claim that "over 96% of savings arise from adaptation" is specific to `constant_small` and does not generalize to large-magnitude workloads where adaptation savings are negative or neutral.
3. **No Hardware/Software Performance Extrapolation:** Abstract symbol slot counts must not be extrapolated to CPU cycle speedups or physical device energy savings.
