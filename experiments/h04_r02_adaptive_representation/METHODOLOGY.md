# H04-R02 Methodology: Workload Definitions, Comparator Fairness, and Cost Accounting

**Document ID:** `experiments/h04_r02_adaptive_representation/METHODOLOGY.md`
**Date:** October 2026
**Status:** Complete / Final Research Packet

---

## 1. Comparator Fairness Protocol

To eliminate baseline asymmetries present in earlier Hanoi/T81 benchmarks:

1. **Symmetric Adaptive Control:**
   Binary is provided with an adaptive baseline (**AB**) capable of switching between 8-bit, 16-bit, 32-bit, and 64-bit precision modes. It is never restricted to fixed 64-bit mode while ternary is allowed adaptive precision.

2. **Packing & Alignment Neutrality:**
   No artificial byte-alignment penalty (e.g., forcing 8-bit alignment on binary while granting sub-byte trit packing to ternary) is imposed. Storage slots are reported in exact raw symbol counts (bits for binary, trits for ternary).

3. **Domain Parity:**
   All four arms process identical signed integer trace arrays and produce exact, un-truncated, mathematically identical outputs verified against 64-bit integer standard arithmetic.

---

## 2. Workload Families

The experimental suite includes 8 predeclared workload trace families (100 values per trace):

1. `constant_small`: Values constrained to $[-100, 99]$ (Tier 0).
2. `constant_large`: Values exceeding $10^{18}$ (Tier 3).
3. `phased`: Sequential blocks transitioning from Tier 0 to Tier 1, Tier 2, and Tier 3.
4. `alternating`: Oscillating values between Tier 0 ($42$) and Tier 3 ($3 \times 10^{18}$).
5. `boundary_oscillation`: Oscillating between $127$ (Binary T0 max) and $128$ (Binary T1 min).
6. `signed_extremes`: Sweeping signed boundary extremes $[-2^{63}, -2^{31}, -128, 0, 127, 2^{63}-1]$.
7. `sparse_large`: 98% small values (Tier 0) with 2% sparse large spikes (Tier 3).
8. `adversarial_transition`: Cycling through Tiers 0, 1, 2, 3 on every value to maximize transition penalty.

---

## 3. Cost Accounting Vector

Total cost $C_{total}$ for an arm is defined as:

$$C_{total} = S_{weighted} + M_{meta} + T_{trans} + C_{select}$$

where:
- $S_{weighted} = \text{symbol\_slots} \cdot c_{radix}$ (where $c_{radix} = 1.0$ for binary, $c_{radix} = C_3/C_2$ for ternary).
- $M_{meta} = \text{block\_count} \cdot \text{meta\_bits\_per\_block} \cdot c_{radix}$.
- $T_{trans} = \sum |\Delta \text{tier}| \cdot C_{trans}$.
- $C_{select} = \text{checks} \cdot C_{conv}$.
