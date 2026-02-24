#!/usr/bin/env python3
import json
import re
import sys
import os

def parse_opcodes(header_path):
    opcodes = {}
    if not os.path.exists(header_path):
        print(f"Error: Header file {header_path} not found.")
        sys.exit(1)

    with open(header_path, 'r') as f:
        content = f.read()

    enum_content = re.search(r'enum class Opcode : std::uint8_t \{(.*?)\};', content, re.DOTALL)
    if not enum_content:
        print("Error: Could not find enum definition in header.")
        sys.exit(1)

    entries = enum_content.group(1).split(',')
    current_val = 0

    for entry in entries:
        entry = entry.strip()
        if not entry:
            continue

        # Handle comments that might be attached due to split
        # We only care about the code part before //
        code_part = entry.split('//')[0].strip()
        if not code_part:
            continue

        if '=' in code_part:
            name, val_str = code_part.split('=')
            name = name.strip()
            val_str = val_str.strip()
            if val_str.startswith('0x'):
                current_val = int(val_str, 16)
            else:
                current_val = int(val_str)
        else:
            name = code_part.strip()

        opcodes[name] = current_val
        current_val += 1
    return opcodes

def main():
    frozen_path = ".t81/frozen_opcodes.json"
    header_path = "include/t81/tisc/opcodes.hpp"
    report_path = "artifacts/ci_reports/opcode_compat_report.json"

    if not os.path.exists(frozen_path):
        print(f"Frozen opcodes file {frozen_path} not found. Bootstrapping?")
        sys.exit(1)

    with open(frozen_path, 'r') as f:
        frozen_opcodes = json.load(f)

    current_opcodes = parse_opcodes(header_path)

    errors = []

    # Check for modifications or deletions
    for name, val in frozen_opcodes.items():
        if name not in current_opcodes:
            errors.append(f"Opcode '{name}' ({val}) was removed.")
        elif current_opcodes[name] != val:
            errors.append(f"Opcode '{name}' changed value from {val} to {current_opcodes[name]}.")

    # Check for reuse of values (optional, but good for safety)
    # Invert maps
    frozen_val_map = {v: k for k, v in frozen_opcodes.items()}
    current_val_map = {v: k for k, v in current_opcodes.items()}

    for val, name in current_val_map.items():
        if val in frozen_val_map and frozen_val_map[val] != name:
             errors.append(f"Opcode value {val} reused by '{name}' (formerly '{frozen_val_map[val]}').")

    os.makedirs(os.path.dirname(report_path), exist_ok=True)

    report = {
        "status": "fail" if errors else "pass",
        "errors": errors,
        "frozen_count": len(frozen_opcodes),
        "current_count": len(current_opcodes)
    }

    with open(report_path, 'w') as f:
        json.dump(report, f, indent=2)

    if errors:
        print("Opcode Compatibility Check Failed:")
        for e in errors:
            print(f"  - {e}")
        sys.exit(1)
    else:
        print("Opcode Compatibility Check Passed.")
        sys.exit(0)

if __name__ == "__main__":
    main()
