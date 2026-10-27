#!/usr/bin/env python3
"""
Ablation Study Orchestration & Benchmark Harness for Investigation 08.
Executes paired process benchmarks across 30+ randomized observations
for empty-store and populated-store conditions.
"""

import json
import os
import random
import subprocess
import time
from typing import Dict, Any, List
from baseline_stack import ConventionalTaskRunner, T81W2Model

class AblationHarness:
    def __init__(self, repo_root: str, work_dir: str):
        self.repo_root = os.path.abspath(repo_root)
        self.work_dir = os.path.abspath(work_dir)
        self.t81_bin = os.path.join(self.repo_root, "build", "t81")
        self.demo_builder = os.path.join(self.repo_root, "build", "t81_make_assess_fixed_demo")
        os.makedirs(self.work_dir, exist_ok=True)

    def prepare_fixtures(self) -> Dict[str, Any]:
        model_dir = os.path.join(self.work_dir, "model")
        os.makedirs(model_dir, exist_ok=True)
        model_path = os.path.join(model_dir, "assess-fixed-demo.t81w")
        tokenizer_path = os.path.join(model_dir, "tokenizer.json")

        if os.path.exists(self.demo_builder):
            subprocess.run([self.demo_builder, model_dir], check=True, stdout=subprocess.DEVNULL)

        with open(model_path, "rb") as f:
            model_bytes = f.read()

        with open(tokenizer_path, "r", encoding="utf-8") as f:
            tokenizer_data = json.load(f)

        parsed_model = T81W2Model(model_bytes)
        model_hash = f"sha3-512:{parsed_model.file_checksum}"

        apl_policy_path = os.path.join(self.work_dir, "policy.apl")
        apl_policy_text = f'(policy (tier 1) (allowed-ternary-model-hashes ["{model_hash}"]) (require-axion-event (reason "task:assess_fixed.v1")))\n'
        with open(apl_policy_path, "w") as f:
            f.write(apl_policy_text)

        return {
            "model_path": model_path,
            "model_bytes": model_bytes,
            "tokenizer_path": tokenizer_path,
            "tokenizer_data": tokenizer_data,
            "model_hash": model_hash,
            "apl_policy_path": apl_policy_path,
            "apl_policy_text": apl_policy_text
        }

    def run_config_a(self, fixtures: Dict[str, Any], input_text: str, canon_root: str) -> Dict[str, Any]:
        """Config A — Full Integrated T81 Stack (Fresh Process Launch)"""
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

        t0_read = time.perf_counter_ns()
        read_cmd = [
            self.t81_bin, "ai", "task", "read-field", ref,
            "--field", "decision", "--canonfs-root", canon_root
        ]
        subprocess.run(read_cmd, capture_output=True, text=True, check=True)
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

    def run_config_b(self, fixtures: Dict[str, Any], input_text: str, cas_root: str) -> Dict[str, Any]:
        """Config B — Conventional Baseline Stack (In-Process Execution)"""
        runner = ConventionalTaskRunner(cas_root)

        t0 = time.perf_counter_ns()
        result = runner.run_assess_fixed(
            model_bytes=fixtures["model_bytes"],
            input_text=input_text,
            policy_input=fixtures["apl_policy_text"],
            tokenizer_data=fixtures["tokenizer_data"],
            mode="strict_deterministic"
        )
        t1 = time.perf_counter_ns()

        replay_metrics = runner.verify_and_replay(
            result["bundle_ref"],
            policy_input=fixtures["apl_policy_text"],
            tokenizer_data=fixtures["tokenizer_data"],
            model_bytes_override=fixtures["model_bytes"]
        )

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

    def run_paired_benchmarks(self, fixtures: Dict[str, Any], input_text: str, iterations: int = 30) -> Dict[str, Any]:
        """Runs 30+ paired observations with randomized execution order for empty and populated stores."""
        empty_store_a = []
        empty_store_b = []
        populated_store_a = []
        populated_store_b = []

        # Empty-store trials (fresh store per iteration)
        for i in range(iterations):
            canon_a = os.path.join(self.work_dir, f"canon_empty_{i}")
            cas_b = os.path.join(self.work_dir, f"cas_empty_{i}")
            os.makedirs(canon_a, exist_ok=True)
            os.makedirs(cas_b, exist_ok=True)

            order = ["A", "B"]
            random.shuffle(order)

            res_a = None
            res_b = None
            for item in order:
                if item == "A":
                    res_a = self.run_config_a(fixtures, input_text, canon_a)
                else:
                    res_b = self.run_config_b(fixtures, input_text, cas_b)

            empty_store_a.append(res_a["cold_start_latency_ms"])
            empty_store_b.append(res_b["execution_latency_ms"])

        # Populated-store trials (reusing persistent stores)
        canon_pop = os.path.join(self.work_dir, "canon_populated")
        cas_pop = os.path.join(self.work_dir, "cas_populated")
        os.makedirs(canon_pop, exist_ok=True)
        os.makedirs(cas_pop, exist_ok=True)

        for i in range(iterations):
            order = ["A", "B"]
            random.shuffle(order)

            res_a = None
            res_b = None
            for item in order:
                if item == "A":
                    res_a = self.run_config_a(fixtures, input_text, canon_pop)
                else:
                    res_b = self.run_config_b(fixtures, input_text, cas_pop)

            populated_store_a.append(res_a["cold_start_latency_ms"])
            populated_store_b.append(res_b["execution_latency_ms"])

        def calc_stats(vals: List[float]) -> Dict[str, float]:
            mean = sum(vals) / len(vals)
            variance = sum((x - mean) ** 2 for x in vals) / len(vals)
            stddev = variance ** 0.5
            return {
                "count": len(vals),
                "mean_ms": mean,
                "stddev_ms": stddev,
                "min_ms": min(vals),
                "max_ms": max(vals)
            }

        return {
            "empty_store": {
                "config_a_t81": calc_stats(empty_store_a),
                "config_b_conventional": calc_stats(empty_store_b)
            },
            "populated_store": {
                "config_a_t81": calc_stats(populated_store_a),
                "config_b_conventional": calc_stats(populated_store_b)
            }
        }

    def run_config_c(self, fixtures: Dict[str, Any], input_text: str) -> Dict[str, Any]:
        return {
            "config": "Config C (T81 with Binary Arithmetic Substitution)",
            "status": "UNIMPLEMENTED",
            "reason": "No production C++ VM seam exists to substitute binary arithmetic while retaining the T81 execution stack without modifying production code."
        }

    def run_config_d(self, fixtures: Dict[str, Any], input_text: str) -> Dict[str, Any]:
        return {
            "config": "Config D (T81 with Conventional File Storage Substitution)",
            "status": "UNIMPLEMENTED",
            "reason": "No storage substitution seam exists in the production binary without modifying production code."
        }

    def run_config_e(self, fixtures: Dict[str, Any], input_text: str) -> Dict[str, Any]:
        return {
            "config": "Config E (T81 with Policy Engine Substitution)",
            "status": "UNIMPLEMENTED",
            "reason": "Axion policy enforcement is a mandatory runtime invariant in T81; bypass traps execution via SecurityFault."
        }

    def run_config_f(self, fixtures: Dict[str, Any], input_text: str) -> Dict[str, Any]:
        return {
            "config": "Config F (Selected Subsystem Pairings)",
            "status": "UNIMPLEMENTED",
            "reason": "Subsystem pairing substitution seam unavailable in production binary without modifying production code."
        }
