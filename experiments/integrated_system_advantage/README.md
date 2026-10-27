# Scientific Investigation 08 — Integrated System Advantage Experiment Suite & Conventional Baseline

This directory contains the executable experiments, independent conventional baseline comparator implementation, differential test suite, and benchmark runners for **Scientific Investigation 08** ("Integrated Architecture, Emergent Advantages, and System-Level Evaluation").

## Overview

The benchmark harness implements and validates an independent conventional baseline comparator against the reference `t81 ai task assess-fixed` T81VM implementation:

- **`baseline_stack.py`**: Pure Python / conventional baseline implementation providing independent T81W2 model weight parsing and SHA3-512 checksum validation, Axion APL policy parser and authorization evaluator, candidate sampling window calculation (`history_seed_window_start`), 2-layer attention slice matrix multiplication kernel with integer accumulation, token-to-decision mapping (`kAssessFixedVocabulary`), CAS object storage, and `verify_and_replay` evidence verification.
- **`test_differential_parity.py`**: Comprehensive differential regression test suite validating zero semantic mismatches across 10 scenario groups against the compiled `t81` binary.
- **`ablation_runner.py`**: Orchestrates paired process execution benchmarks across 30+ randomized observations in empty-store and populated-store conditions.
- **`run_experiments.py`**: Main experiment runner executing the scientific parity gate check, running fresh-process paired benchmarks, and recording machine-readable results in `results.json`.

## Scientific Acceptance Gate & Equivalence

1. **Computational & Behavioral Parity**: Verified zero unexplained differences across:
   - Classification decision and reason code (`ALLOW:GREETING_PAIR`, `DENY:WORLD_SEQUENCE`, `REVIEW:PREFIX_ONLY`, `UNKNOWN:UNMAPPED_TOKEN`).
   - Selected decision token ID and candidate score ordering.
   - Text input string normalization (`normalize_task_input_text`).
   - Axion APL authorization checking (`policy_allows_single_model_hash` & `policy_requires_task_marker`).
   - SHA3-512 model fingerprinting and SHA3-256 artifact hashing (`canon_sha3_256_ref` with Base-81 encoding).
2. **Replay & Evidence Linkage**: Verified that CAS artifact retrieval lookup latency is strictly distinguished from re-execution replay latency.

## Final Disposition

**PARITY VERIFIED — MEASUREMENT COMPLETED**

- **Computational Equivalence**: PARITY MATCHED (Zero semantic mismatches across all differential parity test scenarios).
- **Performance Summary**: Full T81 CLI binary stack process launch incurs translation/process overhead compared to in-process conventional execution (~5,294 ms CLI process launch vs ~2.87 ms in-process execution). Per experimental protocol guidelines, process launch latency differences between Python and C++ binaries are not interpreted as intrinsic T81 architectural advantage.

## Reproduction Commands

To run the differential parity test suite:

```bash
python3 experiments/integrated_system_advantage/test_differential_parity.py
```

To run the paired benchmark suite and update `results.json`:

```bash
python3 experiments/integrated_system_advantage/run_experiments.py
```

All determinism claims comply with the [Determinism Surface Registry](/docs/governance/DETERMINISM_SURFACE_REGISTRY.md).
