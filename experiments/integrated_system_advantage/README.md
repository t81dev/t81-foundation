# Scientific Investigation 08 — Integrated System Advantage Experiment Suite (Corrected)

This directory contains the executable experiments, baseline implementations, and benchmark runners for **Scientific Investigation 08** ("Integrated Architecture, Emergent Advantages, and System-Level Evaluation").

## Overview

The benchmark harness compares matched architectural configurations across execution latency, verification overhead, policy enforcement, and interaction effects:

- **`baseline_stack.py`**: Conventional Python/C++ binary architecture providing matched SHA-3/256 Content-Addressed Storage (CAS), JSON policy authorization, FP32/INT8 execution, and structured trace logging.
- **`ablation_runner.py`**: Orchestrates six matched configurations (Configs A through F) evaluating full integration, binary baseline, and controlled subsystem ablations.
- **`run_experiments.py`**: Main experiment runner verifying computational output parity, measuring real execution and lookup operations without fabricated estimates, and writing `results.json`.

## Methodology & Controls

1. **Computational Equivalence**: Output decision parity (`ALLOW (GREETING_PAIR)`) is explicitly verified across test fixtures before latency comparisons are conducted.
2. **Controlled Ablations & Seams**: Where production code substitution seams do not exist without modifying production code (such as arithmetic substitution or bypassing CanonFS / Axion policy checks), the corresponding configurations (Configs C, D, E, F) are marked `UNIMPLEMENTED` rather than simulating false substitutions.
3. **Compositional Analysis**: Interaction effect analysis is classified `INCONCLUSIVE` as controlled individual subsystem ablations cannot be executed without modifying production binaries.

## Usage

To execute the benchmark suite and generate `results.json`:

```bash
python3 experiments/integrated_system_advantage/run_experiments.py
```

All determinism claims comply with the [Determinism Surface Registry](/docs/governance/DETERMINISM_SURFACE_REGISTRY.md).
