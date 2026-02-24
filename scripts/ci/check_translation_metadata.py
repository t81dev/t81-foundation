#!/usr/bin/env python3
import os
import sys
import json
import glob

def check_metadata():
    report_path = "artifacts/ci_reports/translation_metadata_report.json"

    # Check all md files in book/ except en/
    files = glob.glob("book/**/*.md", recursive=True)
    files = [f for f in files if "book/en/" not in f]

    missing_metadata = []

    for fpath in files:
        with open(fpath, 'r') as f:
            try:
                line = f.readline()
                if not line.startswith('---'):
                    missing_metadata.append(fpath)
            except Exception:
                missing_metadata.append(fpath)

    errors = []
    if missing_metadata:
        errors = [f"Missing YAML frontmatter in {f}" for f in missing_metadata]

    os.makedirs(os.path.dirname(report_path), exist_ok=True)
    report = {
        "status": "warning" if errors else "pass",
        "errors": errors
    }

    with open(report_path, 'w') as f:
        json.dump(report, f, indent=2)

    if errors:
        print("Translation Metadata Check (Warning):")
        for e in errors:
            print(f"  - {e}")
        # Warning only -> exit 0
        sys.exit(0)
    else:
        print("Translation Metadata Check Passed.")
        sys.exit(0)

if __name__ == "__main__":
    check_metadata()
