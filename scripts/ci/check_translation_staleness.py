#!/usr/bin/env python3
import os
import sys
import json
import subprocess
import glob

def get_git_timestamp(path):
    try:
        # Use git log to get last commit timestamp
        ts = subprocess.check_output(['git', 'log', '-1', '--format=%ct', '--', path]).decode().strip()
        return int(ts) if ts else 0
    except:
        return 0

def check_staleness():
    report_path = "artifacts/ci_reports/translation_staleness_report.json"

    # We compare book/LANG/file.md vs book/en/file.md
    # If en is significantly newer than LANG, warn.

    langs = []
    if os.path.exists("book"):
        for d in os.listdir("book"):
            if d != "en" and os.path.isdir(os.path.join("book", d)):
                langs.append(d)

    stale_files = []

    for lang in langs:
        lang_dir = f"book/{lang}"
        for root, dirs, files in os.walk(lang_dir):
            for f in files:
                if not f.endswith('.md'): continue

                lang_file = os.path.join(root, f)
                rel_path = os.path.relpath(lang_file, lang_dir)
                en_file = os.path.join("book/en", rel_path)

                if os.path.exists(en_file):
                    en_ts = get_git_timestamp(en_file)
                    lang_ts = get_git_timestamp(lang_file)

                    # If en is newer by > 30 days (approx 2.6M seconds)
                    diff = en_ts - lang_ts
                    if diff > 2600000:
                        stale_files.append({
                            "file": lang_file,
                            "en_ts": en_ts,
                            "lang_ts": lang_ts,
                            "diff_days": diff / 86400
                        })

    errors = [f"Stale translation: {x['file']} ({x['diff_days']:.1f} days behind)" for x in stale_files]

    os.makedirs(os.path.dirname(report_path), exist_ok=True)
    report = {
        "status": "warning" if errors else "pass",
        "errors": errors,
        "stale_files": stale_files
    }

    with open(report_path, 'w') as f:
        json.dump(report, f, indent=2)

    if errors:
        print("Translation Staleness Check (Warning):")
        for e in errors[:10]: # Limit output
            print(f"  - {e}")
        if len(errors) > 10:
            print(f"  ... and {len(errors)-10} more.")
        sys.exit(0) # Warning only
    else:
        print("Translation Staleness Check Passed.")
        sys.exit(0)

if __name__ == "__main__":
    check_staleness()
