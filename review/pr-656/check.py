"""Replay PR 656 fixtures in disposable trees; requires jsonschema==4.26.0."""
import argparse
import copy
import hashlib
import json
import subprocess
import sys
import sysconfig
import tempfile
from pathlib import Path
from jsonschema import Draft202012Validator, FormatChecker

ROOT = Path(__file__).resolve().parents[2]
PIN = '7daafeb55ae241e21cfbe4bdb4a74bb911595fa7'
CORRECTED_PIN = '5e4320c10f6b0f1355f792e7176a247e34fbb2c8'
FILES = ['scripts/validate_current_state.py', 'docs/ai-context/CURRENT_STATE.schema.json',
         'docs/ai-context/CURRENT_STATE.json']
p = argparse.ArgumentParser()
p.add_argument('--original', action='store_true')
p.add_argument('--output', required=True)
a = p.parse_args()
# Historical v1 receipts replay their exact inputs, independent of the live v2 index.
source_pin = PIN if a.original else CORRECTED_PIN
blobs = {f: subprocess.check_output(['git', 'show', f'{source_pin}:{f}'], cwd=ROOT)
         for f in FILES}
state = json.loads(blobs[FILES[2]])
schema = json.loads(blobs[FILES[1]])
Draft202012Validator.check_schema(schema)
oracle = Draft202012Validator(schema, format_checker=FormatChecker())
cases = [('valid', json.dumps(state))]
def put(name, path, value=None, delete=False):
    obj = copy.deepcopy(state)
    target = obj
    for key in path[:-1]:
        target = target[key]
    if delete:
        del target[path[-1]]
    else:
        target[path[-1]] = value
    cases.append((name, json.dumps(obj)))
def walk(obj, rule, path=()):
    if isinstance(obj, dict):
        for key in rule.get('required', []):
            put('missing:' + '.'.join(map(str, path + (key,))), path + (key,), delete=True)
        put('extra:' + '.'.join(map(str, path)), path + ('unexpected',), True)
        for key, value in obj.items():
            walk(value, rule.get('properties', {}).get(key, {}), path + (key,))
    elif isinstance(obj, list):
        for i, value in enumerate(obj):
            walk(value, rule.get('items', {}), path + (i,))
walk(state, schema)
for val in ['', 'a'*39, 'a'*41, 'g'*40, 'A'*40, 'a'*40+'\n', None, 1]:
    put('sha:' + repr(val), ('repository','observed_head'), val)
for path in [('generated_at',), ('repository','observed_date'), ('open_pr_observations','observed_date')]:
    for val in ['2026-02-30', '2026-13-01', '20261031', '2026-W44-6', '2026-1-1', '2026-10-31T00:00:00Z', '0000-01-01', '2026-10-31\n']:
        put('date:'+'.'.join(path)+':'+repr(val), path, val)
for key in ['merge','performance_measurement','production_activation']:
    for val in [True, 0, 1, 'false', None, [], {}]:
        put('auth:'+key+':'+repr(val), ('authorization',key), val)
for val in ['authorized', 'user_delegated_with_reason', False, None]:
    put('protocol:'+repr(val), ('authorization','protocol_amendment'), val)
for val in ['', 'Authorizes experiments', None]:
    put('purpose:'+repr(val), ('purpose',), val)
for val in [-1, True, 1.0, 1.5, '49']:
    put('count:'+repr(val), ('open_pr_observations','count'), val)
put('verification:unknown', ('scientific_boundaries',0,'verification'), 'trusted')
put('wrong:root-array', ('workstreams',), {})
cases += [('root-null', 'null'), ('root-array', '[]'), ('malformed', '{'),
          ('trailing-comma', '{"x":1,}'), ('non-json-NaN', json.dumps(state).replace('49', 'NaN', 1))]
rows = []
with tempfile.TemporaryDirectory(prefix='pr656-fixtures-') as tmp:
    path_aliases = [str(Path(tmp).resolve()), tmp]
    repo = Path(tmp) / 'repo'
    for name, blob in blobs.items():
        dest = repo / name
        dest.parent.mkdir(parents=True, exist_ok=True)
        dest.write_bytes(blob)
    foreign = Path(tmp) / 'unrelated'
    foreign.mkdir()
    for name, payload in cases:
        (repo / FILES[2]).write_text(payload, encoding='utf-8')
        try:
            parsed = json.loads(payload, parse_constant=lambda s: (_ for _ in ()).throw(ValueError(s)))
            errors = [e.message for e in oracle.iter_errors(parsed)]
            accepted = not errors
        except ValueError as e:
            accepted, errors = False, [str(e)]
        for flags in [[], ['-O']]:
            cmd = ['python3', *flags, str(repo / FILES[0])]
            proc = subprocess.run(cmd, cwd=foreign, text=True, capture_output=True)
            rows.append(dict(case=name, payload=payload, command=cmd, cwd=str(foreign),
                             exit=proc.returncode, stdout=proc.stdout, stderr=proc.stderr,
                             schema_accepts=accepted, schema_errors=errors,
                             agreement=(proc.returncode == 0) == accepted))
    # Supplied valid fixture: root, nested directory, filesystem root, unrelated cwd, symlink.
    (repo / FILES[2]).write_bytes(blobs[FILES[2]])
    link = foreign / 'validator.py'
    link.symlink_to(repo / FILES[0])
    for cwd in [repo, repo / 'docs', Path('/'), foreign]:
        for flags in [[], ['-O']]:
            for script in [repo / FILES[0], link]:
                cmd = ['python3', *flags, str(script)]
                proc = subprocess.run(cmd, cwd=cwd, text=True, capture_output=True)
                rows.append(dict(case='cwd-resolution', command=cmd, cwd=str(cwd),
                                 exit=proc.returncode, stdout=proc.stdout, stderr=proc.stderr,
                                 agreement=proc.returncode == 0))
# Keep the historical receipt shape/target pin; sources contains exact input hashes.
result = dict(source_revision=PIN, variant='original' if a.original else 'corrected',
              environment=dict(python='.'.join(map(str, sys.version_info[:3])), jsonschema='4.26.0'),
              sources={k:hashlib.sha256(v).hexdigest() for k,v in blobs.items()}, rows=rows)
# Keep payloads and diagnostic text; replace only host-specific path prefixes.
# Tokens describe the disposable layout and are not literal runnable paths.
result["path_normalization"] = {
    "${FIXTURE_ROOT}": "disposable root containing repo/ and unrelated/",
    "${PYTHON_STDLIB}": "interpreter standard-library directory",
}
def portable(value):
    if isinstance(value, str):
        for prefix in sorted(set(path_aliases), key=len, reverse=True):
            value = value.replace(prefix, "${FIXTURE_ROOT}")
        stdlib = sysconfig.get_path("stdlib")
        for prefix in {stdlib, str(Path(stdlib).resolve())}:
            value = value.replace(prefix, "${PYTHON_STDLIB}")
        return value
    if isinstance(value, list):
        return [portable(item) for item in value]
    if isinstance(value, dict):
        return {key: portable(item) for key, item in value.items()}
    return value
Path(a.output).write_text(json.dumps(portable(result), indent=2) + '\n', encoding='utf-8')
bad = [r for r in rows if not r['agreement']]
print(f"{result['variant']}: {len(cases)} fixtures; {len(rows)} executions; {len(bad)} schema/exit or cwd mismatches")
for flags in [False, True]:
    subset = [r for r in bad if ('-O' in r['command']) == flags]
    print(f"  optimized={flags}: {len(subset)} mismatches")
if not a.original and bad:
    sys.exit(1)
