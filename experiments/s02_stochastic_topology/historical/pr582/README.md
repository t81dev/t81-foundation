# T81 Research Experiment S02 — Stochastic State–Topology Computation

**Status:** Completed Research Cycle
**Final Verdict:** STOP (Track S Research Concluded)

---

## Overview

This repository contains the complete experimental suite for T81 Research Experiment S02. The experiment investigated whether a stochastic computational network with adaptive local topology provides measurable computational advantages over fixed-topology networks and centralized adaptive baselines under explicit operational cost models.

## Structure

- `SPECIFICATION.md`: Formal mathematical specification, energy functions, Glauber transition probabilities, capacity matching rules, and explicit cost models.
- `stochastic_topology.hpp`: Header-only C++17 simulator implementing Models A–D, Centralized Baseline, and Negative Controls.
- `main.cpp`: Standalone CLI executable conducting parameter sweeps across seeds, node counts, temperatures, and cost penalties.
- `CMakeLists.txt`: Build configuration for `t81_s02_stochastic_topology`.
- `oracle.py`: Python reference oracle computing exact ground states and partition functions via exhaustive enumeration.
- `test_oracle.py`: Test harness verifying C++ simulator outputs against exact Python oracle ground truth.
- `results.json`: Full reproducible experimental dataset (1,260 simulation runs).
- `RESEARCH_REPORT.md`: Final research paper detailing empirical observations, interpretations, control comparisons, and stopping rule decision.
- `METHODOLOGY.md`: Methodological details and experimental procedure.
- `VERIFICATION.md`: Correctness proof and verification results.

## Reproduction Steps

```bash
# Build the simulation binary
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_TESTS=ON
cmake --build build --target t81_s02_stochastic_topology

# Run Python oracle verification tests
python3 experiments/s02_stochastic_topology/test_oracle.py --runner build/experiments/s02_stochastic_topology/t81_s02_stochastic_topology

# Generate results dataset
./build/experiments/s02_stochastic_topology/t81_s02_stochastic_topology experiments/s02_stochastic_topology/results.json
```
