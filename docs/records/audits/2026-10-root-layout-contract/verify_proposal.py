#!/usr/bin/env python3
"""Verify inactive audit data, not a repository root or an authorization grant.

Run with a Python environment containing jsonschema 4.26.0. The repository
validator does not import this file. Requires full Git history and the exact
baseline objects; it never mutates the repository.
"""
import hashlib
import json
from pathlib import Path
import subprocess

import jsonschema


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
BASELINE = 'f4ffa85bf0bcaf028b51462113bf19c2eebb0091'


def no_duplicate_keys(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f'duplicate JSON key: {key}')
        result[key] = value
    return result


def load(name):
    return json.loads((HERE / name).read_text(), object_pairs_hook=no_duplicate_keys)


def main():
    data = load('proposed-contract.json')
    schema = load('proposed-contract.schema.json')
    jsonschema.Draft202012Validator.check_schema(schema)
    jsonschema.Draft202012Validator(schema).validate(data)
    assert data['baseline'] == BASELINE
    assert data['activation_authority'] is None
    assert data['status'] == 'PROPOSED_NOT_AUTHORIZED'
    assert not data['approved_optional_entries']
    assert not data['grandfathered_historical_entries']
    raw = subprocess.check_output(['git', 'ls-tree', '-z', BASELINE], cwd=ROOT)
    observed = {}
    for item in raw.split(b'\0'):
        if not item:
            continue
        meta, name = item.decode().split('\t', 1)
        mode, kind, oid = meta.split(' ')
        observed[name] = {'mode': mode, 'object_type': kind, 'oid': oid}
    entries = (data['required_directories'] + data['required_files']
               + data['unresolved_entries'])
    paths = [e['path'] for e in entries]
    assert len(paths) == len(set(paths))
    assert set(paths) == set(observed)
    for entry in entries:
        assert entry['baseline_git'] == observed[entry['path']]
        assert (entry['type'] == 'directory') == (observed[entry['path']]['mode'] == '040000')
    for entry in data['unresolved_entries']:
        candidate = entry['preservation_candidate']
        assert candidate['status'] == 'NOT_GRANTED'
        assert candidate['expected_mode'] == entry['baseline_git']['mode']
        assert candidate['expected_object'] == entry['baseline_git']['oid']
    assert {e['path'] for e in data['obsolete_required_files']} == {'package.json', 'package-lock.json'}
    assert {'src', 'experiments', 'development_tests'} <= {e['path'] for e in data['required_directories']}
    assert 'Makefile' in {e['path'] for e in data['unresolved_entries']}
    inventory = load('inventory.json')
    assert len(inventory) == 27
    assert sum(r['exit'] == 0 for r in inventory) == 18
    regressions = load('regressions.json')
    assert len(regressions) == 6
    assert regressions[0]['exit'] == 2
    assert all(r['exit'] == 0 for r in regressions[1:])
    assert not load('pre-inventory-clean.json')['status']
    assert not load('post-check-status.json')['status']
    current = {}
    for item in subprocess.check_output(['git', 'ls-tree', '-z', 'HEAD'], cwd=ROOT).split(b'\0'):
        if not item:
            continue
        meta, name = item.decode().split('\t', 1)
        current[name] = meta
    for name, entry in observed.items():
        if name != 'docs':
            assert current[name] == f"{entry['mode']} {entry['object_type']} {entry['oid']}"
    digests = {name: hashlib.sha256((HERE / name).read_bytes()).hexdigest()
               for name in ['proposed-contract.json', 'proposed-contract.schema.json']}
    print(json.dumps({'baseline': BASELINE, 'schema': 'draft-2020-12 validated',
                      'coverage': len(paths), 'proposed_required': len(paths) - len(data['unresolved_entries']),
                      'unresolved': len(data['unresolved_entries']),
                      'activated_exceptions': 0, 'authority_granted': False,
                      'inventory_pass': 18, 'inventory_fail': 9,
                      'regression_programs_passed': 5,
                      'baseline_root_objects_unchanged_except_audit_docs': len(observed) - 1,
                      'review_artifact_sha256_not_trust_anchors': digests}, indent=2))


if __name__ == '__main__':
    main()
