#!/usr/bin/env python3
"""
Ablation Study Orchestration for Investigation 08 (H-S1 Hypothesis - Corrected).
Evaluates configurations with strict adherence to experimental controls:
  Config A: Full Integrated T81 Stack
  Config B: Conventional Binary Equivalent Stack
  Config C: T81 with Binary Arithmetic Substitution (Marked UNIMPLEMENTED: no arithmetic substitution seam exists in C++ production VM without code changes)
  Config D: T81 with Conventional OS File Storage Substitution (Marked UNIMPLEMENTED / INCONCLUSIVE: omitting --canonfs-root does not substitute CAS implementation in production binary)
  Config E: T81 Policy Invariant (Marked UNIMPLEMENTED: Axion policy check is an immutable runtime invariant)
  Config F: Selected Subsystem Pairings (Marked UNIMPLEMENTED: decoupled subsystem pairing seam unavailable in production binary)
"""

import json
import os
import subprocess
import time
from typing import Dict, Any
from baseline_stack import ConventionalTaskRunner

class AblationHarness:
    def __init__(self, repo_root: str, work_dir: str):
        self.repo_root = os.path.abspath(repo_root)
        self.work_dir = os.path.abspath(work_dir)
        self.t81_bin = os.path.join(self.repo_root, "build", "t81")
        self.demo_builder = os.path.join(self.repo_root, "build", "t81_make_assess_fixed_demo")
        os.makedirs(self.work_dir, exist_ok=True)

    def prepare_fixtures(self) -> Dict[str, str]:
        model_dir = os.path.join(self.work_dir, "model")
        os.makedirs(model_dir, exist_ok=True)
        model_path = os.path.join(model_dir, "assess-fixed-demo.t81w")

        # Build demo model if binary exists
        if os.path.exists(self.demo_builder):
            subprocess.run([self.demo_builder, model_dir], check=True, stdout=subprocess.DEVNULL)

        # Get SHA3 model hash
        res = subprocess.run([self.t81_bin, "determinism", "hash", model_path], capture_output=True, text=True, check=True)
        model_hash = res.stdout.strip().split()[0]

        # Write APL policy
        apl_policy_path = os.path.join(self.work_dir, "policy.apl")
        with open(apl_policy_path, "w") as f:
            f.write(f'(policy (tier 1) (allowed-ternary-model-hashes ["sha3-512:{model_hash}"]) (require-axion-event (reason "task:assess_fixed.v1")))\n')

        # Write JSON policy for baseline runner
        json_policy_path = os.path.join(self.work_dir, "policy.json")
        json_policy = {
            "tier": 1,
            "allowed_model_hashes": [f"sha3-256:{model_hash}", "*"],
            "require_event": "task:assess_fixed.v1"
        }
        with open(json_policy_path, "w") as f:
            json.dump(json_policy, f)

        return {
            "model_path": model_path,
            "model_hash": model_hash,
            "apl_policy_path": apl_policy_path,
            "json_policy_path": json_policy_path
        }

    def run_config_a(self, fixtures: Dict[str, str], input_text: str) -> Dict[str, Any]:
        """Config A — Full Integrated T81 Stack"""
        canon_root = os.path.join(self.work_dir, "canon_a")
        os.makedirs(canon_root, exist_ok=True)

        # 1. Cold-start CLI process execution
        t0_cold = time.perf_counter_ns()
        cmd = [
            self.t81_bin, "ai", "task", "assess-fixed",
            "--model", "assess-fixed-demo",
            "--model-file", fixtures["model_path"],
            "--policy", fixtures["apl_policy_path"],
            "--canonfs-root", canon_root,
            "--mode", "strict_deterministic",
            "--input", input_text
        ]
        res = subprocess.run(cmd, capture_output=True, text=True, check=True)
        t1_cold = time.perf_counter_ns()

        out = json.loads(res.stdout)
        ref = out.get("result_ref")

        # 2. Measured sub-component: Artifact lookup / read-field cost
        t0_read = time.perf_counter_ns()
        read_cmd = [
            self.t81_bin, "ai", "task", "read-field", ref,
            "--field", "decision", "--canonfs-root", canon_root
        ]
        read_res = subprocess.run(read_cmd, capture_output=True, text=True, check=True)
        t1_read = time.perf_counter_ns()

        return {
            "config": "Config A (Full Integrated T81 Stack)",
            "status": "SUCCESS",
            "cold_start_latency_ms": (t1_cold - t0_cold) / 1e6,
            "artifact_lookup_latency_ms": (t1_read - t0_read) / 1e6,
            "decision": out.get("decision"),
            "reason_code": out.get("reason_code"),
            "result_ref": ref,
            "provenance_ref": out.get("provenance_ref")
        }

    def run_config_b(self, fixtures: Dict[str, str], input_text: str) -> Dict[str, Any]:
        """Config B — Conventional Binary Equivalent Stack"""
        cas_root = os.path.join(self.work_dir, "cas_b")
        runner = ConventionalTaskRunner(cas_root)

        with open(fixtures["model_path"], "rb") as f:
            model_bytes = f.read()
        with open(fixtures["json_policy_path"], "r") as f:
            policy_data = json.load(f)

        t0 = time.perf_counter_ns()
        result = runner.run_assess_fixed(model_bytes, input_text, policy_data)
        t1 = time.perf_counter_ns()

        # Distinguish artifact lookup from replay (Part B5)
        replay_metrics = runner.verify_and_replay(result["bundle_ref"])

        return {
            "config": "Config B (Conventional Binary Equivalent Stack)",
            "status": "SUCCESS",
            "execution_latency_ms": (t1 - t0) / 1e6,
            "artifact_lookup_latency_ms": replay_metrics["lookup_latency_ns"] / 1e6,
            "actual_replay_latency_ms": replay_metrics["replay_latency_ns"] / 1e6,
            "decision": result["decision"],
            "reason_code": result["reason_code"],
            "result_ref": result["result_ref"],
            "bundle_ref": result["bundle_ref"],
            "replay_valid": replay_metrics["valid"]
        }

    def run_config_c(self, fixtures: Dict[str, str], input_text: str) -> Dict[str, Any]:
        """
        Config C — T81 with Binary Arithmetic Substitution (Part B2)
        Marked UNIMPLEMENTED: No arithmetic substitution seam exists in the production
        T81 C++ VM without modifying production code.
        """
        return {
            "config": "Config C (T81 with Binary Arithmetic Substitution)",
            "status": "UNIMPLEMENTED",
            "reason": "No production C++ VM seam exists to substitute binary arithmetic while retaining the T81 execution stack without modifying production code."
        }

    def run_config_d(self, fixtures: Dict[str, str], input_text: str) -> Dict[str, Any]:
        """
        Config D — T81 with Conventional File Storage Substitution (Part B3)
        Marked UNIMPLEMENTED: Omitting --canonfs-root does not substitute storage implementation;
        it merely defaults to .t81_canonfs in the current working directory.
        """
        return {
            "config": "Config D (T81 with Conventional File Storage Substitution)",
            "status": "UNIMPLEMENTED",
            "reason": "No storage substitution seam exists in the production binary without modifying production code. Omitting --canonfs-root uses default .t81_canonfs CAS rather than OS filesystem substitution."
        }

    def run_config_e(self, fixtures: Dict[str, str], input_text: str) -> Dict[str, Any]:
        """
        Config E — T81 with Policy Engine Substitution
        Marked UNIMPLEMENTED: Axion policy check is an immutable runtime invariant in T81.
        Bypassing policy checks traps execution via SecurityFault.
        """
        return {
            "config": "Config E (T81 with Policy Engine Substitution)",
            "status": "UNIMPLEMENTED",
            "reason": "Axion policy enforcement is a mandatory runtime invariant in T81; bypass traps execution via SecurityFault."
        }

    def run_config_f(self, fixtures: Dict[str, str], input_text: str) -> Dict[str, Any]:
        """
        Config F — Subsystem Pairings (T81 VM + OS FS + APL Policy)
        Marked UNIMPLEMENTED: Decoupled subsystem pairing seam unavailable without production code changes.
        """
        return {
            "config": "Config F (Selected Subsystem Pairings)",
            "status": "UNIMPLEMENTED",
            "reason": "Subsystem pairing substitution seam unavailable in production binary without modifying production code."
        }
