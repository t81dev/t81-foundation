#!/usr/bin/env python3
"""
Verification harness for Experiment S02: Stochastic State-Topology Computation.
Tests C++ executable output against Python IsingOracle exact ground truth.
"""

import argparse
import json
import os
import subprocess
import sys
from oracle import IsingOracle

def run_verification(runner_path: str) -> None:
    test_json = "test_results.json"
    if os.path.exists(test_json):
        os.remove(test_json)

    cmd = [runner_path, test_json]
    print(f"Running simulation binary: {' '.join(cmd)}")
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print(f"Error executing simulator: {res.stderr}")
        sys.exit(1)

    if not os.path.exists(test_json):
        print(f"Output file {test_json} was not created.")
        sys.exit(1)

    with open(test_json, "r") as f:
        data = json.load(f)

    assert "runs" in data, "JSON output missing 'runs' field"
    runs = data["runs"]
    print(f"Total runs collected: {len(runs)}")

    # Spot check ground state energy against oracle for seed=1, num_nodes=8
    checked = 0
    for r in runs:
        if r["num_nodes"] == 8 and r["seed"] == 1:
            oracle = IsingOracle(num_nodes=8, seed=1)
            elem = r["element_type"]
            if elem == "Binary":
                expected_gs, _ = oracle.exact_ground_state_binary()
            else:
                expected_gs, _ = oracle.exact_ground_state_ternary()

            actual_gs = r["ground_state_energy"]
            diff = abs(actual_gs - expected_gs)
            assert diff < 1e-5, f"Ground state mismatch for {elem} N=8 seed=1: C++={actual_gs}, Oracle={expected_gs}"
            checked += 1

    print(f"Verified {checked} runs against Python IsingOracle exact ground truth.")
    print("Verification SUCCESSFUL!")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Test Experiment S02 against Python Oracle")
    parser.add_argument("--runner", required=True, help="Path to t81_s02_stochastic_topology binary")
    args = parser.parse_args()

    run_verification(args.runner)
