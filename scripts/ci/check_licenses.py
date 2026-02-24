#!/usr/bin/env python3
import os
import sys
import json

def check_licenses():
    report_path = "artifacts/ci_reports/license_check_report.json"

    # Simple scan for GPL strings in headers
    # Scan src/ include/

    scanned_extensions = ['.cpp', '.hpp', '.c', '.h', '.py', '.sh', '.cmake']
    errors = []

    for root_dir in ["src", "include", "scripts", "cmake"]:
        if not os.path.exists(root_dir): continue

        for root, dirs, files in os.walk(root_dir):
            for f in files:
                ext = os.path.splitext(f)[1]
                if ext not in scanned_extensions: continue

                fpath = os.path.join(root, f)
                if "check_licenses.py" in fpath:
                    continue

                try:
                    with open(fpath, 'r', errors='ignore') as file:
                        # Read first 50 lines
                        for i in range(50):
                            line = file.readline()
                            if not line: break

                            # Check for GPL/AGPL text
                            if "General Public License" in line and "Lesser" not in line and "Classpath" not in line:
                                errors.append(f"Potential GPL license in {fpath}:{i+1}: {line.strip()}")
                            elif "Affero" in line:
                                errors.append(f"Potential AGPL license in {fpath}:{i+1}: {line.strip()}")
                except:
                    pass

    os.makedirs(os.path.dirname(report_path), exist_ok=True)
    report = {
        "status": "fail" if errors else "pass",
        "errors": errors
    }

    with open(report_path, 'w') as f:
        json.dump(report, f, indent=2)

    if errors:
        print("License Check Failed (GPL detected):")
        for e in errors:
            print(f"  - {e}")
        sys.exit(1)
    else:
        print("License Check Passed.")
        sys.exit(0)

if __name__ == "__main__":
    check_licenses()
