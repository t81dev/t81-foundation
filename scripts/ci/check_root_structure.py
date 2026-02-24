#!/usr/bin/env python3
import os
import sys
import json

def check_root():
    report_path = "artifacts/ci_reports/root_structure_report.json"

    allowlist_path = ".t81/root_allowlist.txt"
    if not os.path.exists(allowlist_path):
        print(f"Error: {allowlist_path} missing.")
        sys.exit(1)

    with open(allowlist_path, 'r') as f:
        allowed = [line.strip().rstrip('/') for line in f if line.strip()]

    current = [f for f in os.listdir('.') if f != '.git' and f != '.t81' and f != '.' and f != '..']

    unknowns = []
    for item in current:
        if item not in allowed:
            # Check if ignored
            if item == "artifacts" or item == "build":
                continue
            unknowns.append(item)

    errors = []
    if unknowns:
        errors = [f"Unknown file/dir in root: {u}" for u in unknowns]

    os.makedirs(os.path.dirname(report_path), exist_ok=True)
    report = {
        "status": "fail" if errors else "pass",
        "errors": errors
    }

    with open(report_path, 'w') as f:
        json.dump(report, f, indent=2)

    if errors:
        print("Root Structure Check Failed:")
        for e in errors:
            print(f"  - {e}")
        sys.exit(1)
    else:
        print("Root Structure Check Passed.")
        sys.exit(0)

if __name__ == "__main__":
    check_root()
