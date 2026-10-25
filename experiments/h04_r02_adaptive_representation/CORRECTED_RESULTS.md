# H04-R02 Corrected Experimental Results and First-Principles Break-Even Analysis

**Document ID:** `experiments/h04_r02_adaptive_representation/CORRECTED_RESULTS.md`
**Date:** October 2026 (Audit Corrected Release)
**Status:** Canonical Corrected Data

---

## 1. Corrected Summary Ledger Table ($C_3/C_2 = 1.0$, $M_{meta}=2$, $C_{trans}=1$, $C_{conv}=0.5$)

*All calculations below are regenerated directly from the repaired C++ executable `t81_h04_r02_adaptive` and matched against the independent DP Python oracle under uncorrupted metadata accounting ($M_{meta} = 2.0$ bits/block).*

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

## 2. Key Mathematical and Physical Bounds Analysis

1. **Information-Theoretic Bound vs Realizable Encodings:**
   - The theoretical minimum bit cost per trit is $\log_2(3) \approx 1.58496$.
   - Realizable bit-packing encodings require additional overhead:
     - 5-trits in 8-bits (Base-243): $8/5 = 1.600$ bits/trit.
     - 2-bit packed trits: $2.000$ bits/trit.
   - For all standard non-asymmetric workloads (`constant_small`, `constant_large`, `phased`, `alternating`, `signed_extremes`, `sparse_large`, `adversarial_transition`), the total cost break-even symbol ratio $C_3/C_2$ ranges between $1.3333$ and $1.5610$.
   - Since $1.5610 < \log_2(3) \approx 1.585 < 1.600 < 2.000$, adaptive binary outperforms adaptive balanced ternary under all realizable bit-packing schemes and physical binary hardware encodings.

2. **Analysis of the $127/128$ Boundary Advantage (DEF-03):**
   - On `boundary_oscillation` ($127, 128$), 8-bit Tier 0 AB switches to 16-bit Tier 1, yielding a break-even ratio of $2.0000$ (payload) or $2.1650$ (total cost).
   - Because $1.6667 > 1.585$, a ratio of $1.6667$ under a 10-bit mode comparator ($M=1024$, range $[-512, 511]$) is mathematically above $\log_2(3)$ and therefore *physically possible* under ideal, unaligned fractional bit-packing.
   - However, in realizable hardware encodings ($2.0$ bits/trit or Base-243 at $1.60$ bits/trit plus SIMD decode latency), adaptive binary with a 10-bit or byte-aligned mode retains superior execution and storage efficiency.
   - Thus, the $127/128$ advantage is a precision granularity artifact of omitting sub-byte/10-bit modes in 8-bit step baselines, rather than an inherent property of the balanced-ternary radix.

3. **Scope and Limits of Inferences:**
   - Abstract symbol-slot models ($C_3/C_2$) measure symbolic storage bounds, not measured CPU instruction counts, cache behavior, or silicon energy in Joules.
   - Claims regarding ternary software impossibility are strictly bounded to software emulation on binary ALUs and abstract cost models; they do not establish universal physical impossibility for hypothetical custom ternary hardware ASICs.
