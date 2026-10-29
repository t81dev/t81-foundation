#!/usr/bin/env python3
"""
Verification Test Suite Runner for Scientific Investigation 07
Generates test vectors using reference_oracle.py, executes Verilog simulation
via iverilog / vvp, and verifies 100% bit-exact arithmetic parity across
Design A, Design B, Design C, and Python Reference Oracle.
"""

import os
import sys
import subprocess
from pathlib import Path
from reference_oracle import dot_product_reference, generate_directed_tests, generate_random_tests

EXP_DIR = Path(__file__).resolve().parent

def export_test_vectors(N: int, vectors_file: Path) -> int:
    """Generates test vectors and writes to test_vectors.txt for Verilog TB."""
    directed = generate_directed_tests(N)
    random_dense = generate_random_tests(N, seed=42, count=50)
    random_sparse = generate_random_tests(N, seed=123, count=50)

    all_tests = directed + random_dense + random_sparse

    with open(vectors_file, 'w') as f:
        for name, W, X, expected in all_tests:
            f.write(f"{expected}\n")
            for w, x in zip(W, X):
                f.write(f"{w} {x}\n")

    return len(all_tests)


def run_iverilog_verification(N: int) -> bool:
    """Compiles and runs iverilog testbench with root-module parameter override -P tb_dot_product.N=<N>."""
    vectors_file = EXP_DIR / "test_vectors.txt"
    test_count = export_test_vectors(N, vectors_file)
    print(f"Exported {test_count} test vectors for N={N} to {vectors_file.name}")

    vvp_bin = EXP_DIR / f"sim_tb_N{N}.vvp"
    compile_cmd = [
        "iverilog",
        "-g2012",
        f"-Ptb_dot_product.N={N}",
        "-o", str(vvp_bin),
        str(EXP_DIR / "design_a_general_mac.v"),
        str(EXP_DIR / "design_b_optimized_binary.v"),
        str(EXP_DIR / "design_c_ht1_ternary.v"),
        str(EXP_DIR / "tb_dot_product.v")
    ]

    print(f"Compiling Verilog TB for N={N}: {' '.join(compile_cmd)}")
    proc = subprocess.run(compile_cmd, cwd=EXP_DIR, capture_output=True, text=True)
    if proc.returncode != 0:
        print(f"Compilation Failed for N={N}!")
        print(proc.stderr)
        return False

    sim_cmd = ["vvp", str(vvp_bin)]
    print(f"Running Simulation for N={N}: {' '.join(sim_cmd)}")
    proc = subprocess.run(sim_cmd, cwd=EXP_DIR, capture_output=True, text=True)
    print(proc.stdout)
    if proc.returncode != 0 or ("VERIFICATION COMPLETE: 0 errors" not in proc.stdout and "0 errors" not in proc.stdout):
        print(f"Simulation Failed for N={N}!")
        print(proc.stderr)
        return False

    # Clean up ephemeral vvp bin and test vectors
    if vvp_bin.exists():
        vvp_bin.unlink()
    if vectors_file.exists():
        vectors_file.unlink()

    return True


if __name__ == "__main__":
    print("=== Running Hardware Verification Suite ===")
    test_lengths = [16, 32, 64]
    all_passed = True
    for N in test_lengths:
        print(f"\n--- Verifying Vector Length N = {N} ---")
        if not run_iverilog_verification(N=N):
            all_passed = False
            break

    if all_passed:
        print("\nVerification PASSED: All designs (A, B, C) match Reference Oracle identically across all tested vector lengths.")
        sys.exit(0)
    else:
        print("\nVerification FAILED.")
        sys.exit(1)
