#!/usr/bin/env python3
import subprocess
import sys
import os

def run_test(t81_bin, test_file):
    print(f"Running {test_file}...")
    try:
        result = subprocess.run([t81_bin, "code", "run", test_file], capture_output=True, text=True)
        if result.returncode == 0:
            print(f"SUCCESS: {os.path.basename(test_file)} verified.")
            return True
        else:
            print(f"FAILURE: {os.path.basename(test_file)} failed.")
            print("STDOUT:", result.stdout)
            print("STDERR:", result.stderr)
            return False
    except Exception as e:
        print(f"Error executing t81: {e}")
        return False

def main():
    print("--- T81 Ternary Logic Verification ---")

    repo_root = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    t81_bin = os.path.join(repo_root, "build", "t81")

    tests = [
        os.path.join(repo_root, "examples", "ternary_verification.t81"),
        os.path.join(repo_root, "examples", "bigint_fraction_verification.t81")
    ]

    if not os.path.exists(t81_bin):
        print(f"Error: {t81_bin} not found. Please build the project first.")
        sys.exit(1)

    all_success = True
    for test in tests:
        if not run_test(t81_bin, test):
            all_success = False

    if all_success:
        print("OVERALL SUCCESS: All ternary logic invariants verified.")
        sys.exit(0)
    else:
        print("OVERALL FAILURE: Some verification steps failed.")
        sys.exit(1)

if __name__ == "__main__":
    main()
