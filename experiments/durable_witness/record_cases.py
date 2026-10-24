#!/usr/bin/env python3
"""Preserve small corrective cases and detached verification (public fixture key)."""
import argparse
from contextlib import closing
import json
from pathlib import Path
import sqlite3
import subprocess
import sys
from unittest.mock import patch

import runner
from witness import init_db

KEY = b'public-corrective-test-fixture-only'


def record_cases(output_dir, binary):
    output_dir.mkdir(parents=True, exist_ok=False)
    db = output_dir / 'witness.db'
    init_db(db).close()
    rows = []
    for label, auth, q, p in [('approved', 'v2', 5, 100), ('over-limit', 'v2', 100, 600),
                              ('revoked', 'v1', 5, 100), ('malformed', 'v2', 0, 100)]:
        for arm in ('A', 'B'):
            ev = runner.execute_runner(arm, f'{label}-{arm}', auth, q, p, db, output_dir, KEY,
                                       arm_a_bin=binary)
            rows.append({'case': label, **ev})
    for arm, function, value in [
        ('A', 'run_arm_a', {'status': 'committed', 'amount': 499, 'events': ['write.commit']}),
        ('B', 'run_arm_b', 499),
    ]:
        with patch.object(runner, function, return_value=value):
            ev = runner.execute_runner(arm, f'wrong-{arm}', 'v2', 5, 100, db, output_dir, KEY)
        rows.append({'case': 'wrong-result (injected adapter output)', **ev})
    guest = output_dir / 'incorrect.wat'
    guest.write_text('(module (import "host" "submit" (func $submit (param i32) (result i32))) '
                     '(func (export "run") (param i32 i32) (result i32) i32.const 499 call $submit))')
    rows.append({'case': 'wrong-result (actual Wasmtime guest)', **runner.execute_runner(
        'B', 'wrong-guest-B', 'v2', 5, 100, db, output_dir, KEY, wat_file=guest)})
    ev = runner.execute_runner('A', 'unavailable-A', 'v2', 5, 100, db, output_dir, KEY,
                               arm_a_bin=output_dir / 'missing-binary')
    rows.append({'case': 'unavailable', **ev})
    (output_dir / 'cases.json').write_text(json.dumps(rows, indent=2) + '\n')
    with closing(sqlite3.connect(db)) as conn:
        counts = {t: conn.execute(f'SELECT COUNT(*) FROM {t}').fetchone()[0]
                  for t in ('transactions', 'effects')}
    (output_dir / 'row-counts.json').write_text(json.dumps(counts, indent=2) + '\n')
    verdicts = []
    for arm in ('A', 'B'):
        cmd = [sys.executable, str(Path(__file__).with_name('verify.py')),
               str(output_dir / f'evidence_approved-{arm}.json'), '--db', str(db),
               '--secret-key', KEY.decode()]
        res = subprocess.run(cmd, text=True, capture_output=True)
        verdicts.append({'command': cmd, 'returncode': res.returncode,
                         'stdout': res.stdout, 'stderr': res.stderr})
    (output_dir / 'detached-verification.json').write_text(json.dumps(verdicts, indent=2) + '\n')
    if counts != {'transactions': 2, 'effects': 2} or any(v['returncode'] for v in verdicts):
        raise RuntimeError('Unexpected admission count or detached verification failure; evidence preserved')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--output-dir', type=Path, required=True)
    parser.add_argument('--arm-a-bin', type=Path, required=True)
    args = parser.parse_args()
    record_cases(args.output_dir, args.arm_a_bin)
