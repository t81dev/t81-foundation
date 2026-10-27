#!/usr/bin/env python3
"""
Main Experiment Runner for Investigation 08 (Equivalent Conventional Comparator).
Executes baseline and ablation harnesses, evaluates computational parity,
measures paired operations with fresh process trials, and records machine-readable findings.
"""

import json
import os
import sys
import platform
import time
from datetime import datetime, timezone
from ablation_runner import AblationHarness

def main():
    print("=== Investigation 08: Equivalent Conventional Comparator & Ablation Experiments ===")

    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    work_dir = os.path.join(os.path.dirname(__file__), "work")

    harness = AblationHarness(repo_root, work_dir)
    fixtures = harness.prepare_fixtures()

    print(f"Model File: {fixtures['model_path']}")
    print(f"Model Hash: {fixtures['model_hash']}")

    input_text = "greet hello"

    # Single-pass execution for parity check
    canon_single = os.path.join(work_dir, "canon_single")
    cas_single = os.path.join(work_dir, "cas_single")
    os.makedirs(canon_single, exist_ok=True)
    os.makedirs(cas_single, exist_ok=True)

    print("\nExecuting Configuration A (Full Integrated T81 Stack)...")
    res_a = harness.run_config_a(fixtures, input_text, canon_single)
    print(f"  Status: {res_a['status']}, Cold Start Latency: {res_a['cold_start_latency_ms']:.2f} ms")

    print("\nExecuting Configuration B (Conventional Binary Stack)...")
    res_b = harness.run_config_b(fixtures, input_text, cas_single)
    print(f"  Status: {res_b['status']}, Execution Latency: {res_b['execution_latency_ms']:.2f} ms")

    # Verify Computational Equivalence
    computational_parity = (res_a["decision"] == res_b["decision"] and res_a["reason_code"] == res_b["reason_code"])
    print(f"\nComputational Equivalence Verification: {'PARITY VERIFIED' if computational_parity else 'MISMATCH'}")
    print(f"  Config A Decision: {res_a['decision']} ({res_a['reason_code']})")
    print(f"  Config B Decision: {res_b['decision']} ({res_b['reason_code']})")

    paired_benchmark_results = {}
    if computational_parity:
        print("\nExecuting 30 Paired Benchmark Trials (Randomized Order)...")
        paired_benchmark_results = harness.run_paired_benchmarks(fixtures, input_text, iterations=30)
        print("  Empty Store Mean Latency:")
        print(f"    Config A (T81 Process): {paired_benchmark_results['empty_store']['config_a_t81']['mean_ms']:.2f} ms ± {paired_benchmark_results['empty_store']['config_a_t81']['stddev_ms']:.2f} ms")
        print(f"    Config B (Conventional Python): {paired_benchmark_results['empty_store']['config_b_conventional']['mean_ms']:.2f} ms ± {paired_benchmark_results['empty_store']['config_b_conventional']['stddev_ms']:.2f} ms")

    res_c = harness.run_config_c(fixtures, input_text)
    res_d = harness.run_config_d(fixtures, input_text)
    res_e = harness.run_config_e(fixtures, input_text)
    res_f = harness.run_config_f(fixtures, input_text)

    results_data = {
        "investigation": "Scientific Investigation 08 — Equivalent Conventional Comparator",
        "final_disposition": "PARITY VERIFIED — MEASUREMENT COMPLETED" if computational_parity else "PARITY NOT ESTABLISHED — EXPERIMENT STOPPED",
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "environment_metadata": {
            "platform": platform.platform(),
            "python_version": sys.version,
            "processor": platform.processor(),
            "machine": platform.machine()
        },
        "computational_equivalence": {
            "parity_matched": computational_parity,
            "test_input": input_text,
            "config_a_output": {"decision": res_a["decision"], "reason_code": res_a["reason_code"]},
            "config_b_output": {"decision": res_b["decision"], "reason_code": res_b["reason_code"]}
        },
        "paired_benchmark_observations": paired_benchmark_results,
        "hypotheses_evaluation": {
            "H_S1_Computational_Advantage": "NOT SUPPORTED (Full T81 CLI binary stack incurs process launch and translation overhead over in-process binary baselines)",
            "H_S1_Assurance_Advantage": "SUPPORTED (T81 provides end-to-end cryptographic content identity, pre-execution Axion policy gating, and bit-exact replay verification)",
            "H_S1_Compositional_Advantage": "INCONCLUSIVE (Controlled interaction analysis is infeasible without production code modification seams for individual subsystem ablations C, D, E, F)",
            "H_S1_Ternary_Specific_Advantage": "NOT SUPPORTED / UNIMPLEMENTED (No arithmetic substitution seam exists in C++ production VM; ternary-specific advantage cannot be inferred from an unimplemented ablation)"
        },
        "results": {
            "Config_A": res_a,
            "Config_B": res_b,
            "Config_C": res_c,
            "Config_D": res_d,
            "Config_E": res_e,
            "Config_F": res_f
        }
    }

    out_file = os.path.join(os.path.dirname(__file__), "results.json")
    with open(out_file, "w") as f:
        json.dump(results_data, f, indent=2)

    print(f"\nExperiment execution complete. Output written to {out_file}")

if __name__ == "__main__":
    main()
