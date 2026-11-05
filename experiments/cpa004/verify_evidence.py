#!/usr/bin/env python3
"""Verify retained raw observations; requires jsonschema==4.23.0, no guest."""
import gzip
import hashlib
import json
from pathlib import Path

from jsonschema import Draft202012Validator

ROOT = Path(__file__).resolve().parent


def require(condition, message):
    if not condition:
        raise RuntimeError(message)


def main():
    contract = json.loads((ROOT / 'contract.json').read_text())
    require(hashlib.sha256((ROOT / 'contract.json').read_bytes()).hexdigest() ==
            (ROOT / 'contract.sha256').read_text().split()[0], 'contract identity')
    for filename, key in [('program.raw13', 'program_sha256'),
                          ('expected.json', 'expected_states_sha256'),
                          ('observation.schema.json', 'schema_sha256')]:
        require(hashlib.sha256((ROOT / filename).read_bytes()).hexdigest() == contract[key],
                filename + ' identity')
    expected = json.loads((ROOT / 'expected.json').read_text())
    schema = json.loads((ROOT / 'observation.schema.json').read_text())
    Draft202012Validator.check_schema(schema)
    validator = Draft202012Validator(schema)
    for record in expected:
        validator.validate(record)
    for run in range(1, 4):
        directory = ROOT / 'evidence/qemu' / f'boot-{run}'
        result = json.loads((directory / 'result.json').read_text())
        raw = gzip.decompress((directory / 'serial.log.gz').read_bytes())
        require(hashlib.sha256(raw).hexdigest() == result['raw_sha256'], 'serial hash')
        require(result['disposition'] == 'PASS' and not result['timeout'], 'guest outcome')
        require(b'CPA004_ARTIFACT_HEX=' + (ROOT / 'program.raw13').read_bytes().hex().encode()
                + b'\r\n' in raw, 'actual input buffer')
        observations = [json.loads(line.split(b'=', 1)[1]) for line in raw.splitlines()
                        if line.startswith(b'CPA004_OBSERVATION=')]
        require(observations == expected, 'all guest states')
        for record in observations:
            validator.validate(record)
        raw = gzip.decompress((ROOT / f'evidence/hosted/hosted-{run}.jsonl.gz').read_bytes())
        summary = json.loads((ROOT / 'evidence/hosted/summary.json').read_text())['runs'][run-1]
        require(hashlib.sha256(raw).hexdigest() == summary['raw_sha256'], 'hosted hash')
        require(summary['exit_code'] == 0, 'hosted outcome')
        hosted = [json.loads(line) for line in raw.splitlines()]
        require(len(hosted) == len(expected), 'hosted record count')
        for record, wanted in zip(hosted, expected):
            state = record['state']
            require(len(state['register_values']) == len(state['register_tags']) == 81,
                    'hosted register shape')
            values, tags = state.pop('memory_values'), state.pop('memory_tags')
            require(len(values) == len(tags) == 1563, 'hosted memory shape')
            state['register_values'] = list(map(str, state['register_values']))
            state['memory'] = dict(word_count=1563, default_value='0', default_tag=0,
                overrides=[dict(address=i, value=str(v), tag=t)
                           for i, (v, t) in enumerate(zip(values, tags)) if v or t])
            normalized = {key: record[key] for key in
                          ('sequence', 'pc_before', 'instruction', 'status', 'trap', 'state')}
            normalized.update(schema=wanted['schema'],
                              program_sha256=contract['program_sha256'])
            require(normalized == wanted, 'all hosted states')
    print('PASS: frozen identities, 100 schema records, 3 guest and 3 hosted traces')


if __name__ == '__main__':
    main()
