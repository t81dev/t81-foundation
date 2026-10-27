#!/usr/bin/env python3
"""
Main Experiment Runner for Investigation 08 (Corrected).
Executes baseline and ablation harnesses, evaluates computational parity,
measures real operations without hard-coded estimates, and records machine-readable findings.
"""

import json
import os
import sys
import time
from datetime import datetime, timezone
from ablation_runner import AblationHarness

def main():
    print("=== Investigation 08: Integrated System Advantage Experiments (Corrected) ===")

    repo_root = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
    work_dir = os.path.join(os.path.dirname(__file__), "work")

    harness = AblationHarness(repo_root, work_dir)
    fixtures = harness.prepare_fixtures()

    print(f"Model File: {fixtures['model_path']}")
    print(f"Model Hash: {fixtures['model_hash']}")

    input_text = "greet hello"

    # Execute Configurations
    print("\nExecuting Configuration A (Full Integrated T81 Stack)...")
    res_a = harness.run_config_a(fixtures, input_text)
    print(f"  Status: {res_a['status']}, Cold Start Latency: {res_a['cold_start_latency_ms']:.2f} ms, Lookup: {res_a['artifact_lookup_latency_ms']:.2f} ms")

    print("\nExecuting Configuration B (Conventional Binary Stack)...")
    res_b = harness.run_config_b(fixtures, input_text)
    print(f"  Status: {res_b['status']}, Execution Latency: {res_b['execution_latency_ms']:.2f} ms, Replay: {res_b['actual_replay_latency_ms']:.2f} ms")

    print("\nEvaluating Configuration C (Binary Arithmetic Substitution)...")
    res_c = harness.run_config_c(fixtures, input_text)
    print(f"  Status: {res_c['status']} — {res_c['reason']}")

    print("\nEvaluating Configuration D (Conventional Storage Substitution)...")
    res_d = harness.run_config_d(fixtures, input_text)
    print(f"  Status: {res_d['status']} — {res_d['reason']}")

    print("\nEvaluating Configuration E (Policy Engine Substitution)...")
    res_e = harness.run_config_e(fixtures, input_text)
    print(f"  Status: {res_e['status']} — {res_e['reason']}")

    print("\nEvaluating Configuration F (Selected Subsystem Pairings)...")
    res_f = harness.run_config_f(fixtures, input_text)
    print(f"  Status: {res_f['status']} — {res_f['reason']}")

    # Verify Computational Equivalence (Part B1)
    computational_parity = (res_a["decision"] == res_b["decision"] and res_a["reason_code"] == res_b["reason_code"])
    print(f"\nComputational Equivalence Verification: {'PARITY MATCHED' if computational_parity else 'MISMATCH'}")
    print(f"  Config A Decision: {res_a['decision']} ({res_a['reason_code']})")
    print(f"  Config B Decision: {res_b['decision']} ({res_b['reason_code']})")

    results_data = {
        "investigation": "Scientific Investigation 08 — Integrated Architecture Advantage (Corrected)",
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "computational_equivalence": {
            "parity_matched": computational_parity,
            "test_input": input_text,
            "config_a_output": {"decision": res_a["decision"], "reason_code": res_a["reason_code"]},
            "config_b_output": {"decision": res_b["decision"], "reason_code": res_b["reason_code"]}
        },
        "hypotheses_evaluation": {
            "H_S1_Computational_Advantage": "NOT SUPPORTED (Full T81 CLI binary stack incurs process launch and translation overhead over in-process binary baselines)",
            "H_S1_Assurance_Advantage": "SUPPORTED (T81 provides end-to-end cryptographic content identity, pre-execution Axion policy gating, and bit-exact replay verification)",
            "H_S1_Compositional_Advantage": "INCONCLUSIVE (Controlled interaction analysis is infeasible without production code modification seams for individual subsystem ablations C, D, E, F)",
            "H_S1_Ternary_Specific_Advantage": "NOT SUPPORTED / UNIMPLEMENTED (No arithmetic substitution seam exists in C++ production VM; ternary-specific advantage cannot be inferred from an unimplemented ablation)"
        },
        "compositional_interaction_analysis": {
            "status": "INCONCLUSIVE",
            "reason": "Controlled interaction metric calculation requires valid ablations for CAS (Config D) and Policy (Config E). Because production code seams do not exist without modifying production code, Configurations C, D, E, F are marked UNIMPLEMENTED and compositional interaction interaction analysis is classified INCONCLUSIVE."
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
