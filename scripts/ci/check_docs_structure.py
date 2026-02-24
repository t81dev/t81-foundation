#!/usr/bin/env python3
import os
import sys
import json

def check_structure(base_lang, target_lang):
    base_dir = f"book/{base_lang}"
    target_dir = f"book/{target_lang}"

    if not os.path.exists(target_dir):
        return [] # Target language not present, maybe okay?

    missing = []
    orphaned = []

    # Walk base (en)
    for root, dirs, files in os.walk(base_dir):
        rel_path = os.path.relpath(root, base_dir)
        target_root = os.path.join(target_dir, rel_path)

        if not os.path.exists(target_root):
             # Missing directory
             pass

        for f in files:
            if not f.endswith('.md'): continue

            base_file = os.path.join(root, f)
            target_file = os.path.join(target_root, f)

            if not os.path.exists(target_file):
                missing.append(target_file)

    # Walk target (cn/pt/etc) to find orphans
    for root, dirs, files in os.walk(target_dir):
        rel_path = os.path.relpath(root, target_dir)
        base_root = os.path.join(base_dir, rel_path)

        for f in files:
            if not f.endswith('.md'): continue
            base_file = os.path.join(base_root, f)
            if not os.path.exists(base_file):
                orphaned.append(os.path.join(root, f))

    return missing, orphaned

def main():
    report_path = "artifacts/ci_reports/docs_structure_report.json"
    langs = ['cn', 'es', 'pt', 'ru'] # Add more if found

    errors = []

    # Auto-discover langs in book/
    if os.path.exists("book"):
        for d in os.listdir("book"):
            if d != "en" and os.path.isdir(os.path.join("book", d)):
                if d not in langs:
                    langs.append(d)

    full_missing = {}
    full_orphaned = {}

    for lang in langs:
        m, o = check_structure('en', lang)
        if m: full_missing[lang] = m
        if o: full_orphaned[lang] = o

        if m:
            errors.append(f"Language '{lang}' missing {len(m)} files present in 'en'.")
        if o:
            errors.append(f"Language '{lang}' has {len(o)} orphaned files not present in 'en'.")

    os.makedirs(os.path.dirname(report_path), exist_ok=True)
    report = {
        "status": "warning" if errors else "pass", # Soft-fail per matrix
        "errors": errors,
        "missing": full_missing,
        "orphaned": full_orphaned
    }

    with open(report_path, 'w') as f:
        json.dump(report, f, indent=2)

    if errors:
        print("Docs Structure Check (Soft-Fail):")
        for e in errors:
            print(f"  - {e}")
        # Soft-fail: exit 1
        sys.exit(1)
    else:
        print("Docs Structure Check Passed.")
        sys.exit(0)

if __name__ == "__main__":
    main()
