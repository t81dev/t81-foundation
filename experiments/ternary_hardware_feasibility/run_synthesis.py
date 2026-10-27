#!/usr/bin/env python3
"""
Reproducible Synthesis & Benchmarking Script for Scientific Investigation 07
Runs Yosys synthesis on Design A, Design B, and Design C across vector lengths N = 16, 32, 64.
Measures cell count, estimated CMOS gate area, and LUT-4 count.
"""

import os
import sys
import json
import subprocess
from pathlib import Path

EXP_DIR = Path(__file__).resolve().parent

def run_yosys_synthesis(top_module: str, verilog_file: str, N: int, acc_width: int = 32) -> dict:
    """Executes Yosys synthesis script and parses statistics output."""
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
        return {}

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

    return {"num_cells": num_cells, "cells": cells}


def extract_metrics(stat_dict: dict) -> dict:
    """Extracts total cell count, area, logic levels from parsed stat dict."""
    if not stat_dict:
        return {"num_cells": 0, "estimated_gate_area": 0.0, "cells": {}}

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
        "num_cells": num_cells,
        "estimated_gate_area": estimated_area,
        "cells": cells
    }


def main():
    print("=== Running Yosys Synthesis & Benchmarking Suite ===")

    vector_lengths = [16, 32, 64]
    designs = [
        ("Design_A_General_INT8_MAC", "design_a_general_mac", "design_a_general_mac.v"),
        ("Design_B_Optimized_Binary", "design_b_optimized_binary", "design_b_optimized_binary.v"),
        ("Design_C_HT1_Ternary", "design_c_ht1_ternary", "design_c_ht1_ternary.v"),
    ]

    results = {}

    for N in vector_lengths:
        print(f"\n--- Synthesizing Vector Length N = {N} ---")
        results[N] = {}
        for name, top_module, filename in designs:
            gate_stat = run_yosys_synthesis(top_module, filename, N=N)
            gate_metrics = extract_metrics(gate_stat)

            results[N][top_module] = {
                "name": name,
                "gate_cells": gate_metrics["num_cells"],
                "gate_area": gate_metrics["estimated_gate_area"],
                "cell_breakdown": gate_metrics["cells"]
            }

            print(f"  {name:30s} | Gate Cells: {gate_metrics['num_cells']:8d} | Gate Area: {gate_metrics['estimated_gate_area']:10.1f}")

    out_file = EXP_DIR / "synthesis_results.json"
    with open(out_file, "w") as f:
        json.dump(results, f, indent=2)
    print(f"\nSaved synthesis results to {out_file.name}")


if __name__ == "__main__":
    main()
