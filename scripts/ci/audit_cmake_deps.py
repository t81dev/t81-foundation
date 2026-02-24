#!/usr/bin/env python3
import re
import sys
import os
import json
import glob

def audit_cmake(root_dir):
    errors = []
    cmake_files = glob.glob(os.path.join(root_dir, "CMakeLists.txt")) + \
                  glob.glob(os.path.join(root_dir, "cmake", "*.cmake")) + \
                  glob.glob(os.path.join(root_dir, "**", "CMakeLists.txt"), recursive=True)

    # Filter out build dirs if any
    cmake_files = [f for f in cmake_files if "build/" not in f and "artifacts/" not in f]

    for fpath in cmake_files:
        with open(fpath, 'r') as f:
            content = f.read()

        # Check for FetchContent_Declare
        # We look for GIT_TAG followed by something that is NOT a hash or version?
        # Actually, let's just find all GIT_TAGs and print them for manual review or heuristics.
        # Heuristic: Tag should not be 'master', 'main', 'dev', 'latest', 'HEAD'

        matches = re.finditer(r'GIT_TAG\s+([^\s\)]+)', content)
        for m in matches:
            tag = m.group(1)
            if tag in ['master', 'main', 'dev', 'latest', 'HEAD']:
                errors.append(f"{fpath}: Invalid GIT_TAG '{tag}'. Must use a specific commit hash or version tag.")

    return errors

def main():
    report_path = "artifacts/ci_reports/cmake_audit_report.json"
    errors = audit_cmake(".")

    os.makedirs(os.path.dirname(report_path), exist_ok=True)

    report = {
        "status": "fail" if errors else "pass",
        "errors": errors
    }

    with open(report_path, 'w') as f:
        json.dump(report, f, indent=2)

    if errors:
        print("CMake Dependency Audit Failed:")
        for e in errors:
            print(f"  - {e}")
        sys.exit(1)
    else:
        print("CMake Dependency Audit Passed.")
        sys.exit(0)

if __name__ == "__main__":
    main()
