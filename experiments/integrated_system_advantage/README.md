# Scientific Investigation 08 — Integrated System Advantage Experiment Suite

This directory contains the executable experiments, baseline implementations, and benchmark runners for **Scientific Investigation 08** ("Integrated Architecture, Emergent Advantages, and System-Level Evaluation").

## Overview

The benchmark harness compares six matched architectural configurations across execution latency, verification overhead, policy enforcement, and interaction effects:

- **`baseline_stack.py`**: Conventional Python/C++ binary architecture providing equivalent SHA-3/256 Content-Addressed Storage (CAS), JSON policy authorization, FP32/INT8 execution, and structured trace logging.
- **`ablation_runner.py`**: Orchestrates six matched configurations (Configs A through F) evaluating full integration, binary baseline, arithmetic substitution, OS storage substitution, policy invariant, and subsystem pairings.
- **`run_experiments.py`**: Main experiment runner collecting empirical performance metrics, computing interaction effects, and writing `results.json`.

## Usage

To execute the benchmark suite and generate `results.json`:

```bash
python3 experiments/integrated_system_advantage/run_experiments.py
```

All determinism claims comply with the [Determinism Surface Registry](/docs/governance/DETERMINISM_SURFACE_REGISTRY.md).
