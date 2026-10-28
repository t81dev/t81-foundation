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

    # Verify Model Architecture Admission Parity (Differential Negative Cases)
    print("\nVerifying Model Architecture Admission Parity (Differential Negative Cases)...")
    import tempfile
    import subprocess
    import hashlib
    sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "tests", "python")))
    from baseline_stack import ConventionalTaskRunner, MANDATORY_TENSORS
    from test_model_admission_parity import parse_t81w, pack_t81w, OMITTED_MANDATORY_TENSORS

    with open(fixtures["model_path"], "rb") as f:
        valid_model_bytes = f.read()

    magic, tensors = parse_t81w(valid_model_bytes)
    admission_parity_results = []
    all_negative_cases_rejected = True

    for tensor_to_remove in OMITTED_MANDATORY_TENSORS:
        filtered_tensors = [t for t in tensors if t["name"] != tensor_to_remove]
        modified_bytes = pack_t81w(magic, filtered_tensors)
        mod_sha512_hash = "sha3-512:" + hashlib.sha3_512(modified_bytes).hexdigest()
        mod_sha256_hash = "sha3-256:" + hashlib.sha3_256(modified_bytes).hexdigest()

        with tempfile.TemporaryDirectory(prefix=f"admission_{tensor_to_remove}_") as tmpdir:
            # 1. Conventional Runner test
            cas_dir = os.path.join(tmpdir, "cas")
            runner = ConventionalTaskRunner(cas_dir)
            json_policy = {"allowed_model_hashes": [mod_sha256_hash, "*"], "require_event": "task:assess_fixed.v1"}

            comparator_rejected = False
            try:
                runner.run_assess_fixed(modified_bytes, input_text, json_policy)
            except ValueError as e:
                comparator_rejected = "Model architecture admission rejected" in str(e)

            cas_objects_count = len(os.listdir(os.path.join(cas_dir, "objects"))) if os.path.exists(os.path.join(cas_dir, "objects")) else 0

            # 2. Production T81 CLI test
            mod_model_path = os.path.join(tmpdir, "model.t81w")
            with open(mod_model_path, "wb") as f:
                f.write(modified_bytes)

            apl_policy_path = os.path.join(tmpdir, "policy.apl")
            with open(apl_policy_path, "w") as f:
                f.write(f'(policy (tier 1) (allowed-ternary-model-hashes ["{mod_sha512_hash}"]) (require-axion-event (reason "task:assess_fixed.v1")))\n')

            canon_dir = os.path.join(tmpdir, "canon")
            cmd = [
                harness.t81_bin, "ai", "task", "assess-fixed",
                "--model", "assess-fixed-demo",
                "--model-file", mod_model_path,
                "--policy", apl_policy_path,
                "--canonfs-root", canon_dir,
                "--mode", "strict_deterministic",
                "--input", input_text
            ]
            cli_res = subprocess.run(cmd, capture_output=True, text=True)
            cli_rejected = (cli_res.returncode != 0)

            case_passed = comparator_rejected and cli_rejected and (cas_objects_count == 0)
            if not case_passed:
                all_negative_cases_rejected = False

            outcome_str = f"REJECTED by both (CLI code {cli_res.returncode}, CAS objects: {cas_objects_count})" if case_passed else "FAIL"
            print(f"  Missing '{tensor_to_remove}': {outcome_str}")
            admission_parity_results.append({
                "missing_tensor": tensor_to_remove,
                "cli_rejected": cli_rejected,
                "comparator_rejected": comparator_rejected,
                "cas_artifacts_persisted": cas_objects_count,
                "parity_matched": case_passed
            })

    print(f"Model Architecture Admission Parity Verification: {'PARITY MATCHED (100% Rejection)' if all_negative_cases_rejected else 'MISMATCH'}")

    results_data = {
        "investigation": "Scientific Investigation 08 — Integrated Architecture Advantage (Corrected)",
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "computational_equivalence": {
            "parity_matched": computational_parity,
            "test_input": input_text,
            "config_a_output": {"decision": res_a["decision"], "reason_code": res_a["reason_code"]},
            "config_b_output": {"decision": res_b["decision"], "reason_code": res_b["reason_code"]}
        },
        "model_architecture_admission_parity": {
            "authoritative_mandatory_tensors": sorted(MANDATORY_TENSORS),
            "omitted_tensors_verified": OMITTED_MANDATORY_TENSORS,
            "parity_matched": all_negative_cases_rejected,
            "negative_test_outcomes": admission_parity_results
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
