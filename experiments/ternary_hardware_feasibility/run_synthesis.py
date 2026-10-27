#!/usr/bin/env python3
"""
Reproducible Synthesis & Benchmarking Script for Scientific Investigation 07
Runs Yosys synthesis on Design A, Design B, and Design C across vector lengths N = 16, 32, 64.
Measures cell count, estimated CMOS gate area, and LUT-4 count.
Ensures Yosys failures produce nonzero process exit status, are explicitly represented as failures,
and cannot overwrite valid previous results.
Includes a regression test simulating a failed synthesis command.
"""

import os
import sys
import json
import subprocess
from pathlib import Path

EXP_DIR = Path(__file__).resolve().parent

def run_yosys_synthesis(top_module: str, verilog_file: str, N: int, acc_width: int = 32) -> dict:
    """Executes Yosys synthesis script and parses statistics output.
    Returns dict with status='SUCCESS' and stats on success, or status='FAILED' and error on failure."""
    tcl_content = f"""
read_verilog -sv {EXP_DIR / verilog_file}
chparam -set N {N} -set ACC_WIDTH {acc_width} {top_module}
hierarchy -top {top_module}
proc; opt; fsm; opt; memory; opt
techmap; opt
abc -g gates; opt
stat
"""
    cmd = ["yosys", "-p", tcl_content]
    proc = subprocess.run(cmd, capture_output=True, text=True)

    if proc.returncode != 0:
        print(f"Yosys synthesis failed for {top_module} N={N}")
        print(proc.stderr)
        return {
            "status": "FAILED",
            "error": proc.stderr or proc.stdout or "Nonzero exit status",
            "returncode": proc.returncode
        }

    # Parse Yosys ASCII stat output
    cells = {}
    num_cells = 0
    in_stat = False
    for line in proc.stdout.splitlines():
        line = line.strip()
        if line.startswith("===") and top_module in line:
            in_stat = True
            continue
        if in_stat:
            if line.startswith("Number of cells:"):
                num_cells = int(line.split()[-1])
            elif line.startswith("$_"):
                parts = line.split()
                cell_type = parts[0]
                count = int(parts[1])
                cells[cell_type] = count

    return {
        "status": "SUCCESS",
        "num_cells": num_cells,
        "cells": cells
    }


def extract_metrics(stat_dict: dict) -> dict:
    """Extracts total cell count, area, logic levels from parsed stat dict."""
    if not stat_dict or stat_dict.get("status") != "SUCCESS":
        return {
            "status": "FAILED",
            "num_cells": None,
            "estimated_gate_area": None,
            "cells": {},
            "error": stat_dict.get("error", "Synthesis failure")
        }

    num_cells = stat_dict.get("num_cells", 0)
    cells = stat_dict.get("cells", {})

    gate_weights = {
        "$_NOT_": 0.5,
        "$_AND_": 1.0,
        "$_NAND_": 1.0,
        "$_OR_": 1.0,
        "$_NOR_": 1.0,
        "$_XOR_": 1.5,
        "$_XNOR_": 1.5,
        "$_MUX_": 2.0,
        "$_ANDNOT_": 1.0,
        "$_ORNOT_": 1.0
    }

    estimated_area = 0.0
    for cell_type, count in cells.items():
        weight = gate_weights.get(cell_type, 1.0)
        estimated_area += weight * count

    return {
        "status": "SUCCESS",
        "num_cells": num_cells,
        "estimated_gate_area": estimated_area,
        "cells": cells
    }


def test_synthesis_failure_handling() -> bool:
    """Regression test that deliberately simulates a failed synthesis command."""
    print("\n--- Running Regression Test: Synthesis Failure Handling ---")
    # Non-existent top module to trigger Yosys error
    failed_stat = run_yosys_synthesis("non_existent_module", "design_a_general_mac.v", N=16)
    if failed_stat.get("status") != "FAILED":
        print("ERROR: Failed synthesis was not recorded as FAILED!")
        return False

    metrics = extract_metrics(failed_stat)
    if metrics["status"] != "FAILED" or metrics["estimated_gate_area"] is not None:
        print("ERROR: Failed synthesis returned valid gate area instead of FAILED status with None area!")
        return False

    print("Regression test PASSED: Synthesis failure correctly produced FAILED status and None area measurements.")
    return True


def main():
    print("=== Running Yosys Synthesis & Benchmarking Suite ===")

    # Run failure handling regression test first
    if not test_synthesis_failure_handling():
        sys.exit(1)

    vector_lengths = [16, 32, 64]
    designs = [
        ("Design_A_General_INT8_MAC", "design_a_general_mac", "design_a_general_mac.v"),
        ("Design_B_Optimized_Binary", "design_b_optimized_binary", "design_b_optimized_binary.v"),
        ("Design_C_HT1_Ternary", "design_c_ht1_ternary", "design_c_ht1_ternary.v"),
    ]

    out_file = EXP_DIR / "synthesis_results.json"

    # Load existing valid results if present (so failed runs don't overwrite)
    existing_results = {}
    if out_file.exists():
        try:
            with open(out_file, "r") as f:
                existing_results = json.load(f)
        except Exception:
            existing_results = {}

    results = {}
    any_failed = False

    for N in vector_lengths:
        print(f"\n--- Synthesizing Vector Length N = {N} ---")
        results[str(N)] = {}
        for name, top_module, filename in designs:
            gate_stat = run_yosys_synthesis(top_module, filename, N=N)
            gate_metrics = extract_metrics(gate_stat)

            if gate_metrics["status"] != "SUCCESS":
                any_failed = True
                print(f"  {name:30s} | SYNTHESIS FAILED: {gate_metrics.get('error')}")
                # Preserve valid previous result if available
                prev_val = existing_results.get(str(N), {}).get(top_module)
                results[str(N)][top_module] = {
                    "name": name,
                    "status": "FAILED",
                    "error": gate_metrics.get("error"),
                    "gate_cells": None,
                    "gate_area": None,
                    "previous_valid_measurement": prev_val
                }
            else:
                results[str(N)][top_module] = {
                    "name": name,
                    "status": "SUCCESS",
                    "gate_cells": gate_metrics["num_cells"],
                    "gate_area": gate_metrics["estimated_gate_area"],
                    "cell_breakdown": gate_metrics["cells"]
                }
                print(f"  {name:30s} | Gate Cells: {gate_metrics['num_cells']:8d} | Gate Area: {gate_metrics['estimated_gate_area']:10.1f}")

    if any_failed:
        print("\nERROR: One or more synthesis commands failed. Not overwriting synthesis_results.json with incomplete data.")
        sys.exit(1)

    with open(out_file, "w") as f:
        json.dump(results, f, indent=2)
    print(f"\nSaved synthesis results to {out_file.name}")


if __name__ == "__main__":
    main()
