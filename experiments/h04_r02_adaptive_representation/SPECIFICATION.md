# H04-R02 Specification: Adaptive Representation Optimization and Independent Verification

**Document ID:** `experiments/h04_r02_adaptive_representation/SPECIFICATION.md`
**Date:** October 2026
**Status:** Complete / Final Research Packet
**Target Repository:** T81 Foundation (`t81dev/t81-foundation`)
**Parent Hypothesis:** H04 (Track R: Scientific Computing)
**Refinement:** H04-R02

---

## 1. Formal Experimental Question

Can an adaptive balanced-ternary representation tiering policy reduce representation or total execution cost compared with an equally capable, symmetrically optimized adaptive binary representation policy, after accounting for exact representation information capacity, mode metadata, conversion work, mode selection checks, and transition penalties?

---

## 2. Tested Arms

The experiment evaluates a $2 \times 2$ factorial structure:

| Radix / Precision | Fixed Precision | Adaptive Precision |
| :--- | :--- | :--- |
| **Binary** | **FB** (Fixed Binary: 64-bit) | **AB** (Adaptive Binary: 8, 16, 32, 64-bit) |
| **Balanced Ternary** | **FT** (Fixed Balanced Ternary: 41 trits) | **AT** (Adaptive Balanced Ternary: 6, 11, 21, 41 trits) |

Every arm processes identical signed integer workload traces and produces identical mathematical values.

---

## 3. Assumptions and Standardized Information Capacity

To ensure comparator fairness and eliminate historical evaluation asymmetries:

1. **Information Capacity Equivalence:**
   For each tier $i \in \{0, 1, 2, 3\}$, the binary mode domain is defined by signed 2's complement range $[ -2^{b_i-1}, 2^{b_i-1}-1 ]$. The corresponding ternary mode domain must cover or exceed this exact range using $k_i$ trits in balanced ternary $[ -(3^{k_i}-1)/2, (3^{k_i}-1)/2 ]$.
   - **Tier 0:** 8 bits ($M=256$, range $[-128, 127]$) $\leftrightarrow$ 6 trits ($3^6 = 729$, range $[-364, 364]$)
   - **Tier 1:** 16 bits ($M=65,536$, range $[-32,768, 32,767]$) $\leftrightarrow$ 11 trits ($3^{11} = 177,147$, range $[-88,573, 88,573]$)
   - **Tier 2:** 32 bits ($M=2^{32}$, range $[-2^{31}, 2^{31}-1]$) $\leftrightarrow$ 21 trits ($3^{21} = 10,460,353,203$, range $[-5.23 \times 10^9, 5.23 \times 10^9]$)
   - **Tier 3:** 64 bits ($M=2^{64}$, range $[-2^{63}, 2^{63}-1]$) $\leftrightarrow$ 41 trits ($3^{41} = 3.64 \times 10^{19}$, range $[-1.82 \times 10^{19}, 1.82 \times 10^{19}]$)

2. **Metadata Accounting:**
   Both adaptive arms (**AB** and **AT**) carry 2 bits of mode metadata per block (or per value) to encode 1 of 4 mode tiers. Mode descriptors are charged symmetrically.

3. **Transition & Conversion Overhead:**
   Transitions between mode tiers incur transition level penalties $C_{trans} \cdot |\Delta \text{tier}|$. Mode selection checks incur selector inspection costs $C_{conv}$.

---

## 4. Falsification Criteria

The claimed ternary advantage is **falsified** in any tested domain where:
1. **AT** fails to achieve lower total cost than **AB** when evaluated under equal capacity, metadata, and transition accounting.
2. The apparent advantage of **AT** over **FB** or **FT** is entirely explained by adaptive mode selection (**AB**) rather than the ternary radix.
3. Increasing metadata, conversion, or transition overheads eliminates the payload-only crossover region within realistic physical parameter ranges ($C_3/C_2 \ge 1.0$).
