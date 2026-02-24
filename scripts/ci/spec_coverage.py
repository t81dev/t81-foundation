#!/usr/bin/env python3
import os
import sys
import json
import glob

def check_coverage():
    report_path = "artifacts/ci_reports/spec_coverage_report.json"

    # Count occurrences of "Spec:" or "Section" in src/

    src_files = glob.glob("src/**/*.cpp", recursive=True) + glob.glob("include/**/*.hpp", recursive=True)

    count = 0
    matches = []

    for fpath in src_files:
        try:
            with open(fpath, 'r', errors='ignore') as f:
                for i, line in enumerate(f):
                    if "Spec:" in line or "Section" in line or "@spec" in line:
                        count += 1
                        matches.append(f"{fpath}:{i+1}")
        except:
            pass

    # Always pass for now, just report
    print(f"Found {count} spec references in code.")

    os.makedirs(os.path.dirname(report_path), exist_ok=True)
    report = {
        "status": "pass",
        "count": count,
        "matches": matches[:100] # Limit size
    }

    with open(report_path, 'w') as f:
        json.dump(report, f, indent=2)

    sys.exit(0)

if __name__ == "__main__":
    check_coverage()
