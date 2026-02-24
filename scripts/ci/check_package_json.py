#!/usr/bin/env python3
import json
import sys
import os

def check_package_json():
    if not os.path.exists("package.json"):
        return [] # No package.json, no problem? Or required? Assume optional if missing.

    with open("package.json", 'r') as f:
        try:
            data = json.load(f)
        except json.JSONDecodeError as e:
            return [f"package.json is invalid JSON: {e}"]

    errors = []
    deps = data.get("dependencies", {})
    if deps:
        errors.append(f"Runtime dependencies found in package.json: {list(deps.keys())}. Move to devDependencies or remove.")

    return errors

def main():
    report_path = "artifacts/ci_reports/package_json_report.json"
    errors = check_package_json()

    os.makedirs(os.path.dirname(report_path), exist_ok=True)

    report = {
        "status": "fail" if errors else "pass",
        "errors": errors
    }

    with open(report_path, 'w') as f:
        json.dump(report, f, indent=2)

    if errors:
        print("Package.json Audit Failed:")
        for e in errors:
            print(f"  - {e}")
        sys.exit(1)
    else:
        print("Package.json Audit Passed.")
        sys.exit(0)

if __name__ == "__main__":
    main()
