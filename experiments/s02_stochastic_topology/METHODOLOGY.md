# Methodology: Experiment S02 — Stochastic State–Topology Computation

**Date:** April 2026
**Target Surface:** `experiments/s02_stochastic_topology/`

---

## 1. Problem Definition & Graph Generation

We formulate probabilistic constraint-satisfaction tasks on a 2D spin glass graph $G = (V, E)$ with local magnetic fields $h_i \in [-0.5, +0.5]$ and coupling constants $J_{ij} \in \{-1.0, +1.0\}$.

The total system energy is given by:
$$E(s) = -\sum_{i} h_i s_i - \sum_{(i,j) \in E_{top}} J_{ij} s_i s_j$$

Deterministic problem instances are generated using a 64-bit Linear Congruential Generator (`SimplePRNG`) with seed $S$ ensuring bit-exact problem graph agreement across C++ and Python environments.

---

## 2. Model Implementations

1. **Model A (Binary Fixed):** $s_i \in \{-1, +1\}$, static graph topology $E_{top}(t) = E_{top}(0)$.
2. **Model B (Ternary Fixed):** $s_i \in \{-1, 0, +1\}$, static graph topology.
3. **Model C (Binary Adaptive Local):** $s_i \in \{-1, +1\}$, local edge rewiring every 10 steps when an edge $(u, v)$ is frustrated ($J_{uv} s_u s_v < 0$).
4. **Model D (Ternary Adaptive Local):** $s_i \in \{-1, 0, +1\}$, local edge rewiring on frustrated edges.
5. **Centralized Baseline:** Global spectral re-alignment of top $K$ energy-minimizing edges at regular intervals.
6. **Control 1 (Random Adaptive):** Uniformly random edge rewiring independent of frustration or energy.
7. **Control 2 (Deterministic Descent):** Zero-temperature greedy local energy descent.

---

## 3. Explicit Cost Model

Computational effort is measured as:
$$\text{Total Cost } = N_{\text{spin\_updates}} \cdot 1.0 + N_{\text{comm\_ops}} \cdot 1.0 + N_{\text{topo\_updates}} \cdot C_{\text{top}}$$

where $C_{\text{top}} \in [0.0, 5.0]$ is the topology restructuring penalty.
