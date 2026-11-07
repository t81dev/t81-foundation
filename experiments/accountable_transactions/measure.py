"""Record raw cold process and steady-owner measurements; no performance winner inference."""
import argparse
import hashlib
import importlib.metadata
import json
from pathlib import Path
import platform
import sqlite3
import statistics
import subprocess
import sys
import uuid
from time import perf_counter_ns as now
from arm_b import Calculator
from verify import canonical, init_state, verify
from witness import provision, Witness

HERE = Path(__file__).resolve().parent


def measure(runner, output, repetitions):
    output.mkdir(parents=True, exist_ok=False)
    rows = []
    for arm in ('A', 'B'):
        for index in range(repetitions):
            command = [str(runner), '100', '250'] if arm == 'A' else [
                sys.executable, str(HERE / 'arm_b.py'), '100', '250']
            start = now()
            proc = subprocess.run(command, capture_output=True, text=True, timeout=30)
            elapsed = now() - start
            row = dict(arm=arm, phase='cold-calculator-process', repetition=index,
                       process_ns=elapsed, stdout=proc.stdout, stderr=proc.stderr, returncode=proc.returncode)
            rows.append(row)
            if proc.returncode or json.loads(proc.stdout)['amount'] != 25000:
                raise RuntimeError('calculator failed')
        root = output / (arm + '-witness')
        trust = provision(root)
        owner = Witness(root)
        state = output / (arm + '-verifier.db')
        init_state(state, trust)
        cold_expected = dict(transaction=str(uuid.uuid4()),
                             request=dict(order='8491', units=100, price=250), policy_version=1)
        expected_path = output / (arm + '-cold-expected.json')
        expected_path.write_bytes(canonical(cold_expected) + b'\n')
        cold_row = dict(arm=arm, phase='cold-witness-transaction', raw=[])
        for operation, flags in [('reserve', ['--expected', str(expected_path)]),
                                 ('commit', ['--transaction', cold_expected['transaction'], '--amount', '25000'])]:
            start = now()
            proc = subprocess.run([sys.executable, str(HERE / 'witness.py'), str(root), operation, *flags],
                                  capture_output=True, text=True, timeout=30)
            cold_row[operation + '_process_ns'] = now() - start
            cold_row['raw'].append(dict(stdout=proc.stdout, stderr=proc.stderr, returncode=proc.returncode))
            if proc.returncode:
                raise RuntimeError('cold witness failure: ' + proc.stdout + proc.stderr)
        verify(json.loads(proc.stdout)['receipt'], trust, cold_expected, state)
        rows.append(cold_row)
        if arm == 'A':
            child = subprocess.Popen([str(runner), '--stream'], stdin=subprocess.PIPE,
                                     stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            def calculate():
                child.stdin.write('100 250\n')
                child.stdin.flush()
                return json.loads(child.stdout.readline())
        else:
            calculator = Calculator()
            def calculate():
                return calculator.run(100, 250)
        try:
            for index in range(repetitions):
                start = now()
                proposal = calculate()
                calculator_ns = now() - start
                expected = dict(transaction=str(uuid.uuid4()), request=dict(order='8491', units=100, price=250),
                                policy_version=1)
                reserved = owner.reserve(expected)
                response = owner.commit(expected['transaction'], proposal['amount'])
                if response['status'] != 'committed':
                    raise AssertionError('denial is not a successful transaction')
                start = now()
                verify(response['receipt'], trust, expected, state)
                verification_ns = now() - start
                rows.append(dict(arm=arm, phase='steady-owner', repetition=index,
                                 calculator_roundtrip_ns=calculator_ns, execution_ns=proposal['execution_ns'],
                                 reserve_ns=reserved['authorization_ns'],
                                 final_authorization_ns=response['authorization_ns'],
                                 commit_including_signing_ns=response['commit_ns'],
                                 signing_ns=response['signing_ns'], verification_consumption_ns=verification_ns,
                                 receipt_bytes=len(canonical(response['receipt']))))
        finally:
            owner.close()
            if arm == 'A':
                child.stdin.close()
                child.wait(timeout=30)
                stderr = child.stderr.read()
                if child.returncode:
                    raise RuntimeError(stderr)
        # Cold restart includes key loading, DB integrity check and retrieving committed outbox.
        start = now()
        proc = subprocess.run([sys.executable, str(HERE / 'witness.py'), str(root), 'recover',
                               '--transaction', expected['transaction']], capture_output=True, text=True, timeout=30)
        rows.append(dict(arm=arm, phase='cold-recovery-process', process_ns=now() - start,
                         returncode=proc.returncode, stdout=proc.stdout, stderr=proc.stderr))
        if proc.returncode or json.loads(proc.stdout)['status'] != 'committed':
            raise AssertionError('restart recovery failed')
    summary = {}
    for arm in ('A', 'B'):
        summary[arm] = {}
        for phase in ('cold-calculator-process', 'cold-witness-transaction', 'steady-owner', 'cold-recovery-process'):
            selected = [r for r in rows if r['arm'] == arm and r['phase'] == phase]
            metrics = sorted({k for r in selected for k in r if k.endswith('_ns')})
            summary[arm][phase] = {metric: dict(
                samples=len(selected), median=statistics.median([r[metric] for r in selected]),
                mean=statistics.mean([r[metric] for r in selected]),
                min=min(r[metric] for r in selected), max=max(r[metric] for r in selected)) for metric in metrics}
    deps = {}
    for name in ('cryptography', 'wasmtime', 'cffi', 'pycparser', 'importlib_resources'):
        dist = importlib.metadata.distribution(name)
        deps[name] = dict(version=dist.version, installed_bytes=sum(
            dist.locate_file(f).stat().st_size for f in dist.files if dist.locate_file(f).is_file()))
    tracked = sorted(p for p in HERE.iterdir() if p.suffix in ('.py', '.cpp', '.txt', '.md'))
    meta = dict(platform=platform.platform(), machine=platform.machine(), python=sys.version,
                sqlite=sqlite3.sqlite_version, source_revision=subprocess.check_output(
                    ['git', 'rev-parse', 'HEAD'], cwd=HERE, text=True).strip(),
                dirty=subprocess.check_output(['git', 'status', '--porcelain'], cwd=HERE, text=True),
                source_sha256={p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in tracked},
                dependency_inventory=deps, executable_bytes=runner.stat().st_size,
                executable_sha256=hashlib.sha256(runner.read_bytes()).hexdigest(),
                storage_bytes={str(p.relative_to(output)): p.stat().st_size for p in output.rglob('*.db')},
                summary=summary, samples=rows)
    (output / 'measurements.json').write_bytes(canonical(meta) + b'\n')
    print(json.dumps(summary, indent=2))


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runner', required=True, type=Path)
    parser.add_argument('--output', required=True, type=Path)
    parser.add_argument('--repetitions', default=30, type=int)
    args = parser.parse_args()
    if args.repetitions < 2:
        parser.error('at least two repetitions required')
    measure(args.runner.resolve(), args.output.resolve(), args.repetitions)
