# H04-R02 Independent Optimization Oracle

**Document ID:** `experiments/h04_r02_adaptive_representation/ORACLE.md`
**Date:** October 2026
**Status:** Complete / Final Research Packet

---

## 1. Oracle Design & Independence

The independent reference optimizer (`experiments/h04_r02_adaptive_representation/oracle.py`) is written in pure Python without calling or importing C++ implementation modules.

It utilizes dynamic programming over finite trace domains to compute the globally optimal, minimum-cost sequence of precision tiers for Adaptive Binary (**AB**) and Adaptive Balanced Ternary (**AT**).

---

## 2. Dynamic Programming Formulation

For a workload trace $V = (v_0, v_1, \dots, v_{N-1})$ and cost parameters $(C_3/C_2, M_{meta}, C_{trans}, C_{conv})$:

Let $T_{req}(v)$ be the minimum allowable tier index in $\{0, 1, 2, 3\}$ for value $v$.

Define $DP[i][t]$ as the minimum total cost for prefix $v_0 \dots v_i$ ending in tier $t \ge T_{req}(v_i)$:

$$DP[0][t] = \text{Slots}(t) \cdot c_{radix} + M_{meta} \cdot c_{radix} + C_{conv}$$

For $i > 0$:

$$DP[i][t] = \min_{t' \ge T_{req}(v_{i-1})} \left( DP[i-1][t'] + \text{Slots}(t) \cdot c_{radix} + M_{meta}(i) \cdot c_{radix} + C_{conv} + C_{trans} \cdot |t - t'| \right)$$

---

## 3. Validation and Verification Outcomes

The C++ evaluator (`t81_h04_r02_adaptive`) was validated against `oracle.py` across all 8 workload trace families:
- 100% agreement on optimal tier sequences.
- 100% agreement on raw slot counts and total weighted cost ledgers.
- Tested via CTest target `t81_h04_r02_oracle_test`.
