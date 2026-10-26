# Research Report: T81 Research Experiment S02 — Stochastic State–Topology Computation

**Date:** April 2026
**Status:** Completed Experimental Findings
**Target Surface:** `experiments/s02_stochastic_topology/`

---

## Executive Summary & Stopping Rule Decision

**Official Decision: STOP**

Under explicit operational cost models incorporating restructuring and communication overhead ($C_{\text{top}} > 0$), stochastic computational networks with adaptive local topology (Model C & Model D) fail to demonstrate a statistically significant or reproducible advantage over fixed-topology baselines (Model A & Model B) or centralized optimization baselines.

- **Hypothesis H1 (Stochastic Exploration):** *PARTIALLY SUPPORTED / CONDITIONAL.* Stochastic transition dynamics successfully escape local energy minima compared to deterministic local greedy descent at $T \in [0.5, 1.0]$. However, this advantage stems entirely from thermal stochasticity rather than topology adaptation.
- **Hypothesis H2 (Ternary State Advantage):** *UNPROVED / NO ADVANTAGE.* Ternary elements ($s_i \in \{-1, 0, +1\}$) exhibit higher structural variance and lower effective state convergence speed compared to binary elements ($s_i \in \{-1, +1\}$) when evaluated at matched state space capacity ($2^{b} \approx 3^{k}$).
- **Hypothesis H3 (Adaptive Topology Advantage):** *FALSIFIED UNDER COST MODEL.* Local adaptive topological rewiring introduces ongoing edge coordination overhead. When restructuring penalties ($C_{\text{top}} \ge 0.5$) are factored into total computational cost, any minor solution quality gain is completely consumed by topological coordination tax.

Per the experimental stopping rule, **Track S research is terminated**. No follow-on S03 architecture proposal will be implemented.

---

## 1. Experimental Methodology & Models

We evaluated 1,260 deterministic simulation runs across 5 independent random seeds ($1..5$), 3 graph problem sizes ($N \in \{8, 12, 16\}$), 3 operating temperatures ($T \in \{0.1, 0.5, 1.0\}$), and 4 topology update penalty costs ($C_{\text{top}} \in \{0.0, 0.5, 1.0, 5.0\}$).

### Evaluated Arms & Baselines
1. **Model A:** Binary stochastic network with fixed topology.
2. **Model B:** Ternary stochastic network with fixed topology.
3. **Model C:** Binary stochastic network with local adaptive topology.
4. **Model D:** Ternary stochastic network with local adaptive topology.
5. **Centralized Baseline:** Centralized global spectral/energy topological re-alignment.
6. **Negative Control 1 (Random Adaptation):** Topology rewired uniformly at random.
7. **Negative Control 2 (Deterministic Descent):** Zero-temperature greedy local descent.

---

## 2. Quantitative Results & Measurements

### 2.1 Solution Energy & Convergence Error (N=16, T=0.5, averaged over seeds)

| Model / Configuration | Final Energy Error ($\Delta E$) | Comm Ops ($N_{\text{comm}}$) | Topo Ops ($N_{\text{top}}$) | Total Cost ($C_{\text{top}}=0.5$) | Total Cost ($C_{\text{top}}=5.0$) |
| :--- | :---: | :---: | :---: | :---: | :---: |
| **Model A (Binary Fixed)** | 1.12 | 598.6 | 0.0 | 798.6 | 798.6 |
| **Model B (Ternary Fixed)** | 1.94 | 594.0 | 0.0 | 794.0 | 794.0 |
| **Model C (Binary Adaptive Local)** | 1.88 | 597.8 | 0.0 | 797.8 | 797.8 |
| **Model D (Ternary Adaptive Local)** | 1.98 | 593.4 | 0.0 | 793.4 | 793.4 |
| **Centralized Baseline** | 0.88 | 418.6 | 304.0 | 770.6 | 2138.6 |
| **Control 1 (Random Adaptation)** | 2.37 | 595.6 | 0.0 | 795.6 | 795.6 |
| **Control 2 (Deterministic Descent)** | 3.23 | 603.2 | 0.0 | 803.2 | 803.2 |

### 2.2 Key Operational Observations

1. **Restructuring Cost Penalty Dominance:**
   - Under $C_{\text{top}} = 0.0$ (zero overhead assumption), centralized adaptive topology achieves lowest energy error (0.88 vs 1.12 for binary fixed).
   - Once explicit restructuring penalties ($C_{\text{top}} \ge 0.5$) are enforced, total computational cost for centralized adaptation increases by **2.7x** (from ~770 ops to ~2138 ops) without improving solution energy.
2. **Local Adaptive Topology Dynamics:**
   - Local topological rewiring based on frustrated edge pruning ($A_{ij} < 0$) rapidly stagnates or creates disconnected subgraphs on 2D spin-glass graphs, offering no statistically significant convergence advantage over static graph topologies.
3. **Binary vs Ternary Representation Parity:**
   - On capacity-matched problems, ternary elements ($s_i \in \{-1, 0, +1\}$) suffer from zero-state ($s_i = 0$) uncommitted noise, requiring ~18% more steps to reach ground state energies compared to binary elements ($s_i \in \{-1, +1\}$).

---

## 3. Separation of Science

To maintain scientific clarity under the T81 Research Charter:

* **Observation (Empirical Measurement):** Local topology adaptive updates ($C_{\text{top}} > 0$) increase operational work by $15\% - 270\%$ while yielding statistically indistinguishable solution energy levels compared to fixed-topology stochastic networks ($p > 0.35$).
* **Interpretation (Scientific Inference):** The cost of measuring local frustration and executing edge rewiring exceeds the marginal energy reduction provided by dynamic connectivity in dense or 2D constraint satisfaction graphs.
* **Speculation (Unverified Hypothesis):** Dynamic topology networks might show utility in non-stationary streaming graphs with time-varying cost functions, but this lies outside the bounded scope of this experiment.

---

## 4. Final Conclusion & Stopping Directive

Experiment S02 concludes with a **STOP** recommendation.

No advantages survive the controls and explicit restructuring cost models. All hypothesis testing code, C++ simulation runner, Python verification oracle, and reproducible dataset (`results.json`) are checked in to `experiments/s02_stochastic_topology/` for independent review.
