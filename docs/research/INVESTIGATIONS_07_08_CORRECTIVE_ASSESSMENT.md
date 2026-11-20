# Scientific Investigations 07 and 08 — Corrective Assessment & Experimental Integrity Audit

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Scientific Investigations 07 and 08 — Corrective Assessment & Experimental Integrity Audit](#scientific-investigations-07-and-08-—-corrective-assessment-&-experimental-integrity-audit)
  - [1. Executive Summary & Purpose](#1-executive-summary-&-purpose)
  - [2. Revised Scientific Dispositions Summary](#2-revised-scientific-dispositions-summary)
  - [3. Toolchain & Environment Specifications](#3-toolchain-&-environment-specifications)
  - [4. Findings Traceability Matrix](#4-findings-traceability-matrix)
    - [4.1 Investigation 07 Findings (PR #595)](#41-investigation-07-findings-pr-#595)
    - [4.2 Investigation 08 Findings (PR #596)](#42-investigation-08-findings-pr-#596)
  - [5. Machine-Readable Results Summary](#5-machine-readable-results-summary)
    - [5.1 Investigation 07 Machine-Readable Results (`experiments/ternary_hardware_feasibility/synthesis_results.json`)](#51-investigation-07-machine-readable-results-`experimentsternary_hardware_feasibilitysynthesis_resultsjson`)
    - [5.2 Investigation 08 Machine-Readable Results (`experiments/integrated_system_advantage/results.json`)](#52-investigation-08-machine-readable-results-`experimentsintegrated_system_advantageresultsjson`)
  - [6. Unresolved Confounding Factors & Limitations](#6-unresolved-confounding-factors-&-limitations)
  - [7. Final Verification Commands Executed](#7-final-verification-commands-executed)

<!-- T81-TOC:END -->


**Repository:** T81 Foundation (`t81dev/t81-foundation`)
**Document ID:** `docs/research/INVESTIGATIONS_07_08_CORRECTIVE_ASSESSMENT.md`
**Date:** October 2026
**Role:** Principal Experimental Systems Engineer and Research Reproducibility Investigator
**Source PRs:** #595 (Investigation 07), #596 (Investigation 08)

---

## 1. Executive Summary & Purpose

This corrective assessment addresses and resolves all experimental integrity findings identified by Codex in PRs #595 and #596.

The goal of this intervention is not to produce favorable or biased metrics, but to establish defensible, reproducible scientific truth regarding:
1. **Investigation 07:** Ternary Dot-Product Hardware Feasibility & Matched Binary Baselines.
2. **Investigation 08:** Integrated Architecture, Emergent Advantages, and System-Level Evaluation.

---

## 2. Revised Scientific Dispositions Summary

| Investigation / Hypothesis | Original Disposition | Corrected Disposition | Justification Summary |
| :--- | :--- | :--- | :--- |
| **Investigation 07: H-T1 (Ternary Hardware Feasibility)** | CLOSED & TERMINATED (Overbroad claim) | **`NOT SUPPORTED` (Bounded to H-T1 Overlay)** | Design C yields only a $1.098\times$ gate area reduction over optimized binary control Design B, failing the mandatory $1.25\times$ threshold. The H-T1 sign-and-mask overlay design is rejected without permanently closing untested alternative physical/circuit architectures. |
| **Investigation 08: H-S1a (Computational Advantage)** | NOT SUPPORTED | **`NOT SUPPORTED`** | T81 cold-start CLI execution incurs process boot and translation overhead ($\sim 5.3 \text{ s}$) relative to in-process binary baselines ($\sim 0.65 \text{ ms}$). |
| **Investigation 08: H-S1b (Assurance Advantage)** | SUPPORTED | **`SUPPORTED`** | Delivers end-to-end cryptographic content identity (SHA-3/256), pre-execution Axion policy gating, and bit-exact replay verification. |
| **Investigation 08: H-S1c (Compositional Advantage)** | SUPPORTED (Unconditional claim) | **`INCONCLUSIVE`** | Individual subsystem substitution seams (Configs C, D, E, F) do not exist in the C++ production binary without modifying production code. Interaction analysis is classified INCONCLUSIVE. |
| **Investigation 08: H-S1d (Ternary-Specific Advantage)** | NOT SUPPORTED | **`NOT SUPPORTED / UNIMPLEMENTED`** | No production arithmetic substitution seam exists in T81VM (Config C is UNIMPLEMENTED). Ternary-specific advantage cannot be inferred from an unimplemented ablation. |

---

## 3. Toolchain & Environment Specifications

- **OS / Kernel:** Linux 6.6.137+ (x86_64)
- **Python Runtime:** Python 3.12.13
- **Verilog Simulator:** Icarus Verilog v12.0 (`iverilog -g2012`)
- **Synthesis Tool:** Yosys 0.33 (`yosys -p ...`, ABC gate mapper)
- **C++ Compiler & Build System:** GCC 13.2.0 / CMake 3.28.3 (`-std=c++20`)

---

## 4. Findings Traceability Matrix

### 4.1 Investigation 07 Findings (PR #595)

| Finding ID | Codex Finding Description | Corrective Action Taken | Verification Evidence |
| :--- | :--- | :--- | :--- |
| **A1** | **Verification Parameterization:** Testbench parameter `N` did not match generated vector length because `-DN=<N>` preprocessor macro was used instead of parameter override. | Updated `run_verification.py` to invoke Icarus Verilog with root-module parameter override `-Ptb_dot_product.N=<N>`. Tested N=16, 32, 64. | `python3 experiments/ternary_hardware_feasibility/run_verification.py` executed successfully across all lengths. |
| **A2** | **Synthesis Failure Handling:** Yosys failures did not produce nonzero exit codes, could record zero-area measurements, or overwrite valid results. | Updated `run_synthesis.py` to check Yosys exit codes, return `status="FAILED"` with `None` gate area, preserve previous results, and added `test_synthesis_failure_handling()` regression test. | `python3 experiments/ternary_hardware_feasibility/run_synthesis.py` ran regression test verifying failure handling. |
| **A3** | **Scientific Conclusion Bounding:** Conflated gate count/area with unmeasured throughput and energy efficiency, and overclaimed permanent termination of all ternary hardware research. | Updated `FINDINGS_REPORT.md` and `results.json` to categorize raw gate cells and gate area as measured, explicitly mark throughput and energy efficiency as **`UNMEASURED`**, and restrict negative conclusions strictly to the H-T1 overlay design. | Refactored `FINDINGS_REPORT.md` and `results.json`. |

### 4.2 Investigation 08 Findings (PR #596)

| Finding ID | Codex Finding Description | Corrective Action Taken | Verification Evidence |
| :--- | :--- | :--- | :--- |
| **B1** | **Actual Computational Equivalence:** Baseline substituted a word-count heuristic rather than executing matched model computation. | Updated `baseline_stack.py` to evaluate input tokens against model vocabulary mapping (`"greet hello"` $\to$ `ALLOW (GREETING_PAIR)`). Verified output parity before performance comparison. | `run_experiments.py` verified `PARITY MATCHED` between Config A and Config B. |
| **B2** | **Arithmetic Ablation:** Config C invoked Config B instead of substituting arithmetic inside T81 execution stack. | Marked Config C **`UNIMPLEMENTED`** because no C++ production VM seam exists for arithmetic substitution without modifying production code. | `results.json` records Config C as `UNIMPLEMENTED`. |
| **B3** | **Storage Ablation:** Config D omitted `--canonfs-root` argument, which merely defaulted to `.t81_canonfs` in CWD rather than substituting storage. | Marked Config D **`UNIMPLEMENTED`** because no storage substitution seam exists in the production binary without modifying production code. | `results.json` records Config D as `UNIMPLEMENTED`. |
| **B4** | **Fabricated Measurements:** Hard-coded verification timings (e.g. 0.05 ms) were present in ablation runners. | Removed hard-coded timings. Measured real execution and lookup operations or marked unimplemented. | `ablation_runner.py` uses real `time.perf_counter_ns()` measurements. |
| **B5** | **Artifact Lookup vs Replay:** Artifact lookup was conflated with actual re-execution replay. | Updated `baseline_stack.py` and `ablation_runner.py` to measure artifact lookup (`read-field`) and actual re-execution replay separately. | `results.json` reports `artifact_lookup_latency_ms` and `actual_replay_latency_ms` as distinct metrics. |
| **B6** | **Compositional Effect Analysis:** Unconditional SUPPORTED verdict based on flawed ablations. | Re-classified compositional interaction hypothesis (H-S1c) as **`INCONCLUSIVE`** because individual subsystem ablations C, D, E, F cannot be executed without production code changes. | Updated `INTEGRATED_SYSTEM_ADVANTAGE_REPORT.md` and `results.json`. |
| **B7** | **Baseline Fairness & Cold Start:** Conflated cold-start CLI process startup overhead ($\sim 5.3 \text{ s}$) with steady-state in-process execution times ($\sim 0.65 \text{ ms}$). | Explicitly separated cold-start process launch latency from steady-state in-process execution and lookup latencies across baseline and T81 runners. | `run_experiments.py` reports cold-start latency separately. |

---

## 5. Machine-Readable Results Summary

### 5.1 Investigation 07 Machine-Readable Results (`experiments/ternary_hardware_feasibility/synthesis_results.json`)

```json
{
  "16": {
    "design_a_general_mac": {"name": "Design_A_General_INT8_MAC", "status": "SUCCESS", "gate_cells": 7715, "gate_area": 9050.0},
    "design_b_optimized_binary": {"name": "Design_B_Optimized_Binary", "status": "SUCCESS", "gate_cells": 1430, "gate_area": 1581.5},
    "design_c_ht1_ternary": {"name": "Design_C_HT1_Ternary", "status": "SUCCESS", "gate_cells": 1188, "gate_area": 1436.5}
  },
  "32": {
    "design_a_general_mac": {"name": "Design_A_General_INT8_MAC", "status": "SUCCESS", "gate_cells": 15419, "gate_area": 18062.5},
    "design_b_optimized_binary": {"name": "Design_B_Optimized_Binary", "status": "SUCCESS", "gate_cells": 2927, "gate_area": 3240.5},
    "design_c_ht1_ternary": {"name": "Design_C_HT1_Ternary", "status": "SUCCESS", "gate_cells": 2442, "gate_area": 2951.0}
  },
  "64": {
    "design_a_general_mac": {"name": "Design_A_General_INT8_MAC", "status": "SUCCESS", "gate_cells": 31173, "gate_area": 36547.5},
    "design_b_optimized_binary": {"name": "Design_B_Optimized_Binary", "status": "SUCCESS", "gate_cells": 5924, "gate_area": 6564.5},
    "design_c_ht1_ternary": {"name": "Design_C_HT1_Ternary", "status": "SUCCESS", "gate_cells": 4948, "gate_area": 5980.5}
  }
}
```

### 5.2 Investigation 08 Machine-Readable Results (`experiments/integrated_system_advantage/results.json`)

```json
{
  "investigation": "Scientific Investigation 08 — Integrated Architecture Advantage (Corrected)",
  "computational_equivalence": {
    "parity_matched": true,
    "test_input": "greet hello",
    "config_a_output": {"decision": "ALLOW", "reason_code": "GREETING_PAIR"},
    "config_b_output": {"decision": "ALLOW", "reason_code": "GREETING_PAIR"}
  },
  "hypotheses_evaluation": {
    "H_S1_Computational_Advantage": "NOT SUPPORTED",
    "H_S1_Assurance_Advantage": "SUPPORTED",
    "H_S1_Compositional_Advantage": "INCONCLUSIVE",
    "H_S1_Ternary_Specific_Advantage": "NOT SUPPORTED / UNIMPLEMENTED"
  },
  "results": {
    "Config_A": {"config": "Config A (Full Integrated T81 Stack)", "status": "SUCCESS", "cold_start_latency_ms": 5362.52, "artifact_lookup_latency_ms": 4.57},
    "Config_B": {"config": "Config B (Conventional Binary Equivalent Stack)", "status": "SUCCESS", "execution_latency_ms": 0.65, "actual_replay_latency_ms": 0.44},
    "Config_C": {"config": "Config C (T81 with Binary Arithmetic Substitution)", "status": "UNIMPLEMENTED"},
    "Config_D": {"config": "Config D (T81 with Conventional File Storage Substitution)", "status": "UNIMPLEMENTED"},
    "Config_E": {"config": "Config E (T81 with Policy Engine Substitution)", "status": "UNIMPLEMENTED"},
    "Config_F": {"config": "Config F (Selected Subsystem Pairings)", "status": "UNIMPLEMENTED"}
  }
}
```

---

## 6. Unresolved Confounding Factors & Limitations

1. **Absence of Production Subsystem Substitution Seams:** As mandated by scope restrictions, production VM, ISA, CanonFS, and Axion code were not modified. Consequently, individual subsystem ablations (Configs C, D, E, F) cannot be executed inside the `t81` binary and are properly marked `UNIMPLEMENTED`.
2. **Cold-Start CLI vs In-Process Comparison:** Process creation, binary loading, and C++ static initialization dominate CLI execution time ($\sim 5.3 \text{ s}$). Evaluating T81 in a long-running daemon / server mode would isolate pure in-process VM execution latency.
3. **Unconstrained Combinational Synthesis:** Yosys gate counts and estimated gate area provide structural gate complexity comparisons, but do not incorporate clock constraints, technology timing arcs ($F_{\text{max}}$), or physical cell layout parasitic RC extraction (SPEF). Dynamic power and throughput-normalized area remain explicitly `UNMEASURED`.

---

## 7. Final Verification Commands Executed

1. `python3 experiments/ternary_hardware_feasibility/run_verification.py` — PASSED across N=16, 32, 64.
2. `python3 experiments/ternary_hardware_feasibility/run_synthesis.py` — PASSED synthesis and failure handling regression test.
3. `python3 experiments/integrated_system_advantage/run_experiments.py` — PASSED computational equivalence check and generated machine-readable results.
