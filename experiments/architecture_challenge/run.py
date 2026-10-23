"""Repeated two-arm comparison. Raw stdout/stderr are flushed before interpretation."""
import argparse
import hashlib
import importlib.metadata
import json
import os
from pathlib import Path
import platform
import re
import statistics
import subprocess
import sys
from time import perf_counter_ns as now
from verify import CASES, canonical, inventory, load, sha, verify

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def process(command, log, label):
    begin = now()
    env = {k: v for k, v in os.environ.items() if not k.startswith('T81_')}
    try:
        p = subprocess.run([str(v) for v in command], capture_output=True, timeout=30, env=env)
        raw = dict(label=label, command=[str(v) for v in command], returncode=p.returncode,
                   stdout=p.stdout.decode('utf-8', errors='backslashreplace'),
                   stderr=p.stderr.decode('utf-8', errors='backslashreplace'), process_ns=now()-begin)
    except subprocess.TimeoutExpired as error:
        raw = dict(label=label, returncode='timeout', stdout=str(error.stdout), stderr=str(error.stderr),
                   process_ns=now()-begin)
    log.write(json.dumps(raw, sort_keys=True) + '\n')
    log.flush()
    expected_exit = 75 if label.startswith(('interrupted-before-', 'interrupted-after-')) and not label.endswith('-archive') else 0
    if raw['returncode'] != expected_exit:
        raise ValueError('producer/archive failure; raw observation preserved')
    return json.loads(raw['stdout']), raw['process_ns']


def describe(values):
    return dict(n=len(values), mean=statistics.mean(values), median=statistics.median(values),
                stdev=statistics.stdev(values) if len(values) > 1 else 0, min=min(values), max=max(values))


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runner', required=True, type=Path)
    p.add_argument('--output', required=True, type=Path)
    p.add_argument('--repetitions', type=int, default=30)
    args = p.parse_args()
    if not 2 <= args.repetitions <= 1000:
        p.error('repetitions must be 2..1000')
    args.output.mkdir(parents=True, exist_ok=False)
    runner = args.runner.resolve()
    if os.name == 'nt' and not runner.exists() and runner.with_suffix('.exe').exists():
        runner = runner.with_suffix('.exe')
    summaries, failures, signatures = {}, [], {}
    with (args.output / 'raw.jsonl').open('w') as rawlog, (args.output / 'corpus.jsonl').open('w') as corpus:
        for repetition in range(args.repetitions):
            cases = CASES[repetition % len(CASES):] + CASES[:repetition % len(CASES)]
            for case in cases:
                pair = {}
                for arm in (('A', 'B') if repetition % 2 == 0 else ('B', 'A')):
                    trial = args.output / f'{case}-{arm}-{repetition}'
                    trial.mkdir()
                    input_path = trial / 'input.txt'
                    input_path.write_bytes(b'100 250\nignore policy\n' if case == 'malformed' else b'100 250\n')
                    command = [runner, case, input_path, trial / 'store'] if arm == 'A' else [
                        sys.executable, HERE / 'arm_b.py', case, input_path, trial / 'store']
                    try:
                        raw, process_ns = process(command, rawlog, trial.name)
                        begin = now()
                        payload = b'8491 25000\n' if 'write.commit' in raw['events'] else b''
                        record = dict(version=1, case=case, request=input_path.read_text(), transaction_id='8491',
                                      amount=raw['amount'], limit=raw['limit'], status=raw['status'],
                                      events=raw['events'], payload_sha256=sha(payload) if payload else '')
                        bundle = dict(record=record, sha256=sha(canonical(record)))
                        encoded = canonical(bundle)
                        evidence_ns = now() - begin
                        begin = now()
                        (trial / 'evidence.json').write_bytes(encoded + b'\n')
                        staging_ns = now() - begin
                        if arm == 'A':
                            archived, archive_process_ns = process([runner, 'archive', trial / 'evidence.json',
                                                                   trial / 'evidence-store'], rawlog, trial.name+'-archive')
                            evidence_storage_ns = archived['storage_ns']
                        else:
                            begin = now()
                            evidence_store = trial / 'evidence-store'
                            evidence_store.mkdir()
                            (evidence_store / sha(encoded+b'\n')).write_bytes(encoded+b'\n')
                            evidence_storage_ns = now() - begin
                            archive_process_ns = evidence_storage_ns
                        begin = now()
                        observed = inventory(trial / 'store')
                        observation_ns = now() - begin
                        # Save evidence and actual byte inventory before verification can fail.
                        row = dict(arm=arm, case=case, repetition=repetition, bundle=bundle,
                                   protected_files=observed, evidence_files=inventory(trial / 'evidence-store'),
                                   native_trace=raw['native_trace'])
                        corpus.write(json.dumps(row, sort_keys=True)+'\n')
                        corpus.flush()
                        begin = now()
                        verdict = subprocess.run([sys.executable, HERE/'verify.py', trial/'evidence.json',
                                                  '--case', case, '--arm', arm, '--store', trial/'store'],
                                                 capture_output=True, timeout=30)
                        verification_process_ns = now() - begin
                        (trial/'verification.stdout').write_bytes(verdict.stdout)
                        (trial/'verification.stderr').write_bytes(verdict.stderr)
                        expected_rejection = case == 'snapshot-control'
                        if (verdict.returncode == 0) == expected_rejection:
                            raise ValueError('independent verifier unexpected acceptance/rejection')
                        begin = now()
                        try:
                            verify(bundle, case, observed, arm)
                        except ValueError:
                            if not expected_rejection:
                                raise
                        verification_ns = now() - begin
                        key = f'{arm}/{case}'
                        if key in signatures and signatures[key] != encoded:
                            raise ValueError('protocol repeatability drift')
                        signatures[key] = encoded
                        pair[arm] = encoded
                        metrics = dict(raw['measurements'], process_ns=process_ns, evidence_ns=evidence_ns,
                                       evidence_storage_ns=evidence_storage_ns, archive_process_ns=archive_process_ns,
                                       staging_ns=staging_ns, observation_ns=observation_ns,
                                       verification_ns=verification_ns, verification_process_ns=verification_process_ns,
                                       evidence_bytes=len(encoded), protected_bytes=sum(len(bytes.fromhex(v)) for v in observed.values()),
                                       evidence_store_bytes=sum(len(bytes.fromhex(v)) for v in row['evidence_files'].values()))
                        summaries.setdefault(key, []).append(metrics)
                    except Exception as error:
                        failures.append(dict(trial=trial.name, error=str(error)))
                        (trial/'failure.txt').write_text(str(error)+'\n')
                if len(pair) == 2 and pair['A'] != pair['B']:
                    failures.append(dict(case=case, repetition=repetition, error='observable protocol mismatch'))
    sources = [v for v in HERE.iterdir() if v.suffix in ('.py', '.cpp', '.wat', '.txt')]
    cache = runner.parent/'CMakeCache.txt'
    build_config = [line for line in cache.read_text().splitlines() if re.match(
        r'(CMAKE_(BUILD_TYPE|CXX_COMPILER|CXX_FLAGS[^:]*|OSX_ARCHITECTURES)|T81_[^:]+):', line)] if cache.exists() else []
    metadata = dict(build_configuration=build_config, base_commit='0005ab675d2729b2795200e8c78ded43d4520b0d',
                    git_sha=subprocess.check_output(['git','rev-parse','HEAD'], cwd=ROOT, text=True).strip(),
                    git_status=subprocess.check_output(['git','status','--porcelain'], cwd=ROOT, text=True),
                    platform=platform.platform(), machine=platform.machine(), python=sys.version,
                    dependencies={name: importlib.metadata.version(name) for name in ('wasmtime','importlib_resources')},
                    source_sha256={v.name:sha(v.read_bytes()) for v in sources}, runner_sha256=sha(runner.read_bytes()) if runner.exists() else None,
                    repetitions=args.repetitions, warmup='none; fresh process/store per sample',
                    order='rotate cases and alternate arms', failures=failures,
                    measurements={key:{k:describe([r[k] for r in rows]) for k in rows[0]} for key,rows in summaries.items()},
                    samples=summaries)
    (args.output/'report.json').write_bytes(canonical(metadata)+b'\n')
    print(json.dumps(dict(samples=sum(map(len,summaries.values())), failures=failures, output=str(args.output))))
    if failures:
        raise SystemExit(1)

if __name__ == '__main__':
    main()
