#!/usr/bin/env python3
import os
import sys
import json

def audit_include(include_dir):
    errors = []
    if not os.path.exists(include_dir):
        return ["include directory missing"]

    items = os.listdir(include_dir)
    allowed = ['t81', 'README.md']

    for item in items:
        if item not in allowed:
            # Check if it's a directory (vendored lib) or file
            path = os.path.join(include_dir, item)
            if os.path.isdir(path):
                 errors.append(f"Unexpected directory in include/: {item}. Only 't81' is allowed.")
            else:
                 errors.append(f"Unexpected file in include/: {item}.")
    return errors

def main():
    report_path = "artifacts/ci_reports/include_audit_report.json"
    errors = audit_include("include")

    os.makedirs(os.path.dirname(report_path), exist_ok=True)

    report = {
        "status": "fail" if errors else "pass",
        "errors": errors
    }

    with open(report_path, 'w') as f:
        json.dump(report, f, indent=2)

    if errors:
        print("Include Directory Audit Failed (Soft-Fail):")
        for e in errors:
            print(f"  - {e}")
        # Soft fail means we might exit 0 but report failure?
        # The prompt says "exit nonzero only for hard-fail and soft-fail conditions".
        # So exit 1.
        sys.exit(1)
    else:
        print("Include Directory Audit Passed.")
        sys.exit(0)

if __name__ == "__main__":
    main()
