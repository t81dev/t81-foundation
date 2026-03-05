#!/usr/bin/env python3
"""
Governance Metrics Generator
Parses .t81/spec_map.yaml and generates metrics.json
"""

import json
import sys
from pathlib import Path

def parse_yaml_simplified(content):
    """
    Simple YAML parser for spec_map.yaml structure.
    Assumes a list of subsystems with specific keys.
    """
    subsystems = []
    current_subsystem = {}
    lines = content.splitlines()
    mode = None # 'subsystems', 'code_dirs', 'test_dirs'

    for line in lines:
        stripped = line.strip()
        if not stripped or stripped.startswith('#'):
            continue

        if stripped == 'subsystems:':
            mode = 'subsystems'
            continue

        if mode == 'subsystems':
            if stripped.startswith('- name:'):
                if current_subsystem:
                    subsystems.append(current_subsystem)
                current_subsystem = {'code_dirs': [], 'test_dirs': []}
                current_subsystem['name'] = stripped.split(':', 1)[1].strip()
            elif stripped.startswith('spec:'):
                current_subsystem['spec'] = stripped.split(':', 1)[1].strip()
            elif stripped.startswith('declared_status:'):
                current_subsystem['declared_status'] = stripped.split(':', 1)[1].strip()
            elif stripped == 'code_dirs:':
                mode = 'code_dirs'
            elif stripped == 'test_dirs:':
                mode = 'test_dirs'

        elif mode == 'code_dirs':
            if stripped.startswith('- '):
                current_subsystem['code_dirs'].append(stripped[2:].strip())
            elif stripped == 'test_dirs:':
                mode = 'test_dirs'
            elif stripped.startswith('declared_status:'):
                current_subsystem['declared_status'] = stripped.split(':', 1)[1].strip()
                mode = 'subsystems'
            elif stripped.startswith('- name:'):
                if current_subsystem:
                    subsystems.append(current_subsystem)
                current_subsystem = {'code_dirs': [], 'test_dirs': []}
                current_subsystem['name'] = stripped.split(':', 1)[1].strip()
                mode = 'subsystems'

        elif mode == 'test_dirs':
            if stripped.startswith('- '):
                current_subsystem['test_dirs'].append(stripped[2:].strip())
            elif stripped.startswith('declared_status:'):
                current_subsystem['declared_status'] = stripped.split(':', 1)[1].strip()
                mode = 'subsystems'
            elif stripped.startswith('- name:'):
                if current_subsystem:
                    subsystems.append(current_subsystem)
                current_subsystem = {'code_dirs': [], 'test_dirs': []}
                current_subsystem['name'] = stripped.split(':', 1)[1].strip()
                mode = 'subsystems'

    if current_subsystem:
        subsystems.append(current_subsystem)

    return subsystems

def main():
    root = Path.cwd()
    spec_map_path = root / ".t81/spec_map.yaml"

    if not spec_map_path.exists():
        print(f"Error: {spec_map_path} not found")
        sys.exit(1)

    try:
        content = spec_map_path.read_text(encoding='utf-8')
        subsystems = parse_yaml_simplified(content)
    except Exception as e:
        print(f"Error parsing YAML: {e}")
        sys.exit(1)

    metrics = {
        "subsystems": [],
        "totals": {
            "spec_count": 0,
            "implemented": 0,
            "partial": 0,
            "stubbed": 0,
            "concept": 0
        }
    }

    source_extensions = {'.cpp', '.hpp', '.cc', '.c', '.h'}
    test_extensions = {'.cpp', '.hpp', '.cc', '.c', '.h', '.py'}

    for sub in subsystems:
        name = sub.get('name', 'Unknown')
        spec_path = sub.get('spec', '')
        code_dirs = sub.get('code_dirs', [])
        test_dirs = sub.get('test_dirs', [])
        status = sub.get('declared_status', 'unknown')

        # Check spec existence
        spec_full_path = root / spec_path if spec_path else None
        spec_exists = spec_full_path.exists() if spec_full_path else False

        # Count code files
        code_files_count = 0
        for d in code_dirs:
            p = root / d
            if p.exists() and p.is_dir():
                code_files_count += sum(1 for f in p.rglob('*') if f.suffix in source_extensions)

        # Count test files
        test_files_count = 0
        for d in test_dirs:
            p = root / d
            if p.exists() and p.is_dir():
                test_files_count += sum(1 for f in p.rglob('*') if f.suffix in test_extensions)

        metrics['subsystems'].append({
            "name": name,
            "spec_exists": spec_exists,
            "code_files": code_files_count,
            "test_files": test_files_count,
            "status_declared": status
        })

        if spec_exists:
            metrics['totals']['spec_count'] += 1

        if status in metrics['totals']:
            metrics['totals'][status] += 1
        else:
            # Add to dynamic status key if not present (or map to unknown)
            if status not in metrics['totals']:
                metrics['totals'][status] = 0
            metrics['totals'][status] += 1

    # Ensure artifacts dir exists
    output_dir = root / "artifacts/ci_reports"
    output_dir.mkdir(parents=True, exist_ok=True)

    output_file = output_dir / "governance_metrics.json"
    output_file.write_text(json.dumps(metrics, indent=2), encoding='utf-8')
    print(f"Metrics written to {output_file}")

if __name__ == "__main__":
    main()
