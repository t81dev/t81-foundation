#!/usr/bin/env python3
"""Read-only diagnostic fixtures pinned to the merged Phase 1 main commit."""
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
BASELINE = 'ee0c7d90537eaa2dc4b522c1624fcabebf6e999b'
records = []


def source(path):
    return subprocess.check_output(['git', 'show', f'{BASELINE}:{path}'], cwd=ROOT, text=True)


def run(label, code, *args, env=None):
    result = subprocess.run(['bash', '-c', code, 'fixture', *args],
                            text=True, capture_output=True, env=env)
    record = dict(label=label, command=code, args=args, exit=result.returncode,
                  stdout=result.stdout, stderr=result.stderr)
    records.append(record)
    return record


run('bash version', 'bash --version')
run('D1 isolated unsafe increment', 'set -e; n=0; ((n++)); echo REACHED')
run('D2 masked nonzero child', 'set -e; bash -c \'exit 37\' & QEMU_PID=$!; wait $QEMU_PID 2>/dev/null || true; echo COMPLETED')
run('D2 masked interrupted child', 'set -e; bash -c \'kill -TERM $$\' & QEMU_PID=$!; wait $QEMU_PID 2>/dev/null || true; echo COMPLETED')
run('D1 isolated safe increment', 'set -e; n=0; n=$((n + 1)); echo "REACHED: $n"')
with tempfile.TemporaryDirectory(prefix='t81-baseline-') as temp:
    for index, path in enumerate(('drivers/qemu/scripts/automated_verification.sh',
                                  'ternaryos/automated_verification.sh')):
        text = source(path)
        summary = text[text.index('TOTAL_TESTS=5'):text.index('# Display summary')]
        for mode in ('missing', 'empty', 'failure', 'stale'):
            out = Path(temp) / str(index) / mode
            out.mkdir(parents=True)
            if mode != 'missing':
                for log in ('monitor_test.log', 'memory_analysis.log', 'boot_debug.log',
                            'hardware_test.log', 'integration_monitor.log'):
                    log_path = out / log
                    log_path.write_text('ERROR: failed\n' if mode == 'failure' else '')
                    if mode == 'stale':
                        os.utime(log_path, (1, 1))
            result = run(f'{path}: D1/D2 summary {mode}',
                         'set -e; OUTPUT_DIR="$1"; ' + summary + '\necho REACHED', str(out))
            report = out / 'verification_summary.txt'
            result['summary'] = report.read_text() if report.exists() else None
    caller = source('drivers/qemu/scripts/verify_production.sh')
    callee = source('drivers/qemu/scripts/automated_verification.sh')
    caller_assignment = next(line for line in caller.splitlines() if line.startswith('OUTPUT_DIR='))
    callee_assignment = next(line for line in callee.splitlines() if line.startswith('OUTPUT_DIR='))
    env = dict(os.environ, TMPDIR=temp)
    env.pop('OUTPUT_DIR', None)
    run('D3 unexported caller/callee assignments', caller_assignment +
        '\nprintf "caller=%s\\n" "$OUTPUT_DIR"\nbash -c \' ' + callee_assignment +
        '; printf "callee=%s\\n" "$OUTPUT_DIR"\'', env=env)
    for script in ('experimental/ternaryos/qemu_monitor_test.sh',
                   'experimental/ternaryos/memory_analyzer.sh',
                   'experimental/ternaryos/automated_verification.sh'):
        result = subprocess.run(['git', 'cat-file', '-e', f'{BASELINE}:{script}'],
                                cwd=ROOT, text=True, capture_output=True)
        records.append(dict(label=f'D4 path existence: {script}', exit=result.returncode,
                            stderr=result.stderr))
print(json.dumps(dict(baseline=BASELINE, records=records), indent=2))
