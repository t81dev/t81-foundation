#!/usr/bin/env python3
import sys
import argparse
import time
import json
import statistics
from pathlib import Path

from witness import init_db
from runner import execute_runner
from verify import verify_evidence

TEST_SECRET_KEY = b"test-secret-key-for-perf-harness-32bytes"

def run_experiment(output_dir: Path, repetitions: int = 30, arm_a_bin: Path = None, secret_key: bytes = TEST_SECRET_KEY) -> dict:
    output_dir.mkdir(parents=True, exist_ok=False)
    db_path = output_dir / "witness_perf.db"
    init_db(db_path).close()

    results = []

    for i in range(repetitions):
        # Arm A Trial
        tx_a = f"tx-A-{i}"
        t0 = time.perf_counter_ns()
        ev_a = execute_runner("A", tx_a, "v2", 5, 100, db_path, output_dir, secret_key=secret_key, arm_a_bin=arm_a_bin)
        t1 = time.perf_counter_ns()

        ev_file_a = output_dir / f"evidence_{tx_a}.json"
        t2 = time.perf_counter_ns()
        ver_a = verify_evidence(ev_file_a, db_path, secret_key=secret_key)
        t3 = time.perf_counter_ns()

        results.append({
            "arm": "A",
            "trial": i,
            "tx_id": tx_a,
            "exec_time_us": (t1 - t0) / 1000.0,
            "verify_time_us": (t3 - t2) / 1000.0,
            "success": ev_a["success"],
            "verified": ver_a["valid"]
        })

        # Arm B Trial
        tx_b = f"tx-B-{i}"
        t0 = time.perf_counter_ns()
        ev_b = execute_runner("B", tx_b, "v2", 5, 100, db_path, output_dir, secret_key=secret_key)
        t1 = time.perf_counter_ns()

        ev_file_b = output_dir / f"evidence_{tx_b}.json"
        t2 = time.perf_counter_ns()
        ver_b = verify_evidence(ev_file_b, db_path, secret_key=secret_key)
        t3 = time.perf_counter_ns()

        results.append({
            "arm": "B",
            "trial": i,
            "tx_id": tx_b,
            "exec_time_us": (t1 - t0) / 1000.0,
            "verify_time_us": (t3 - t2) / 1000.0,
            "success": ev_b["success"],
            "verified": ver_b["valid"]
        })

    # Stats calculation
    arm_a_exec = [r["exec_time_us"] for r in results if r["arm"] == "A"]
    arm_b_exec = [r["exec_time_us"] for r in results if r["arm"] == "B"]
    arm_a_ver = [r["verify_time_us"] for r in results if r["arm"] == "A"]
    arm_b_ver = [r["verify_time_us"] for r in results if r["arm"] == "B"]

    summary = {
        "repetitions": repetitions,
        "arm_a": {
            "exec_mean_us": statistics.mean(arm_a_exec),
            "exec_stdev_us": statistics.stdev(arm_a_exec) if len(arm_a_exec) > 1 else 0,
            "verify_mean_us": statistics.mean(arm_a_ver),
            "all_verified": all(r["verified"] for r in results if r["arm"] == "A")
        },
        "arm_b": {
            "exec_mean_us": statistics.mean(arm_b_exec),
            "exec_stdev_us": statistics.stdev(arm_b_exec) if len(arm_b_exec) > 1 else 0,
            "verify_mean_us": statistics.mean(arm_b_ver),
            "all_verified": all(r["verified"] for r in results if r["arm"] == "B")
        }
    }

    with open(output_dir / "report.json", "w") as f:
        json.dump(summary, f, indent=2)

    with open(output_dir / "raw.jsonl", "w") as f:
        for r in results:
            f.write(json.dumps(r) + "\n")

    return summary

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=Path, required=True)
    parser.add_argument("--repetitions", type=int, default=30)
    parser.add_argument("--arm-a-bin", type=Path)
    args = parser.parse_args()
    if args.repetitions < 1:
        parser.error("--repetitions must be positive")
    summary = run_experiment(args.output_dir, args.repetitions, args.arm_a_bin)
    print(json.dumps(summary, indent=2))

if __name__ == "__main__":
    main()
