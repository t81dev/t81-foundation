#!/usr/bin/env python3
"""Bounded implementation observations, not an architectural speed comparison."""
import hashlib
import json
import os
from pathlib import Path
import random
import statistics
import subprocess
import sys
import tempfile
import time

from baseline_stack import canon_sha3_256_ref

HERE = Path(__file__).resolve().parent


class AblationHarness:
    def __init__(self, repo_root, work_dir, build_dir=None):
        self.repo_root = Path(repo_root).resolve()
        self.work_dir = Path(work_dir).resolve()
        self.work_dir.mkdir(parents=True, exist_ok=True)
        self.build_dir = Path(build_dir or os.environ.get('T81_BUILD_DIR', self.repo_root/'build'))
        ext = '.exe' if os.name == 'nt' else ''
        self.t81_bin = str(self.build_dir/('t81'+ext))
        self.demo_builder = str(self.build_dir/('t81_make_assess_fixed_demo'+ext))
        self.worker = str(HERE/'baseline_worker.py')

    def prepare_fixtures(self):
        # Caller owns an invocation-specific directory; never reuse an old model silently.
        model_dir = Path(tempfile.mkdtemp(prefix='fixtures-', dir=self.work_dir))
        subprocess.run([self.demo_builder, str(model_dir)], check=True, capture_output=True)
        model = model_dir/'assess-fixed-demo.t81w'
        policy = model_dir/'allow.apl'
        fingerprint = 'sha3-512:'+hashlib.sha3_512(model.read_bytes()).hexdigest()
        policy.write_bytes((f'(policy (tier 1) (allowed-ternary-model-hashes ["{fingerprint}"]) '
                          '(require-axion-event (reason "task:assess_fixed.v1")))\n').encode('utf-8'))
        return dict(model_path=str(model), tokenizer_path=str(model_dir/'tokenizer.json'),
                    apl_policy_path=str(policy), model_hash=fingerprint)

    def _launch(self, command):
        start = time.perf_counter_ns()
        result = subprocess.run(command, capture_output=True, text=True, timeout=180)
        elapsed = time.perf_counter_ns()-start
        if result.returncode:
            raise RuntimeError(f'{command[0]} exit {result.returncode}: {result.stderr}')
        return json.loads(result.stdout), elapsed

    def run_config_a(self, fixtures, input_text, canon_root):
        command = [self.t81_bin, 'ai', 'task', 'assess-fixed', '--model-file', fixtures['model_path'],
                   '--model', 'investigation08', '--policy', fixtures['apl_policy_path'],
                   '--canonfs-root', str(canon_root), '--mode', 'strict_deterministic', '--input', input_text]
        result, wall = self._launch(command)
        # Separate fresh-process artifact retrieval, with byte-identity verification.
        lookup_start = time.perf_counter_ns()
        for key in ('result_ref', 'provenance_ref'):
            with tempfile.TemporaryDirectory(dir=self.work_dir) as tmp:
                target = Path(tmp)/'artifact.json'
                subprocess.run([self.t81_bin, 'canonfs', 'get', result[key], '--out', str(target),
                                '--canonfs-root', str(canon_root)], check=True, capture_output=True, timeout=30)
                if canon_sha3_256_ref(target.read_bytes()) != result[key]:
                    raise ValueError('T81 artifact identity mismatch')
        lookup_wall = time.perf_counter_ns()-lookup_start
        replayed, replay_wall = self._launch(command)
        if any(result[key] != replayed[key] for key in ('result_ref','provenance_ref','model_hash','policy_result')):
            raise ValueError('T81 rerun differs')
        return dict(arm='T81 C++ CLI', end_to_end_process_ns=wall,
                    phase_timings_ns=dict(process_initialization=None, model_loading=None,
                                          computational_execution=None, policy_evaluation=None, artifact_persistence=None),
                    phase_limit='CLI exposes no internal phase clocks; do not infer phase costs from wall time.',
                    artifact_lookup_process_ns=lookup_wall, replay_process_ns=replay_wall,
                    replay_kind='fresh CLI re-execution using original fixture files; not standalone CAS model recovery',
                    verified=True, result_ref=result['result_ref'], provenance_ref=result['provenance_ref'],
                    model_hash=result['model_hash'], decision=result['decision'], policy_result=result['policy_result'])

    def run_config_b(self, fixtures, input_text, cas_root):
        command = [sys.executable, self.worker, 'run', '--store', str(cas_root),
                   '--model', fixtures['model_path'], '--policy', fixtures['apl_policy_path'],
                   '--tokenizer', fixtures['tokenizer_path'], '--input', input_text]
        result, wall = self._launch(command)
        replayed, replay_wall = self._launch([sys.executable, self.worker, 'replay', '--store', str(cas_root),
                                             '--bundle', result['bundle_ref']])
        return dict(arm='pure Python conventional implementation', end_to_end_process_ns=wall,
                    phase_timings_ns=result['timings_ns'],
                    module_initialization_ns=result['worker_module_initialization_ns'],
                    process_boundary_residual_ns=wall-result['worker_total_ns'],
                    residual_limit='Includes interpreter startup, teardown, output serialization and pipe overhead; not isolated initialization.',
                    artifact_lookup_ns=replayed['lookup_latency_ns'], replay_execution_ns=replayed['replay_latency_ns'],
                    replay_process_ns=replay_wall, replay_kind='independent persisted-input CAS recovery and re-execution',
                    verified=replayed['valid'], result_ref=result['result_ref'], bundle_ref=result['bundle_ref'],
                    model_hash=result['model_hash'], decision=result['decision'], policy_result=result['policy_result'])

    def run_paired_benchmarks(self, fixtures, input_text, iterations=30, seed=602):
        if iterations < 1:
            raise ValueError('positive iteration count required')
        rng = random.Random(seed)
        observations = []
        # Unique directory per invocation AND per observation/arm, including populated trials.
        invocation = Path(tempfile.mkdtemp(prefix='trials-', dir=self.work_dir))
        for condition in ('empty_store', 'populated_store'):
            for index in range(iterations):
                pair = Path(tempfile.mkdtemp(prefix=f'{condition}-{index}-', dir=invocation))
                roots = {arm: pair/arm for arm in ('A','B')}
                for root in roots.values():
                    root.mkdir(exist_ok=False)
                initial_counts = {arm: len(list(root.rglob('*'))) for arm,root in roots.items()}
                if any(initial_counts.values()):
                    raise ValueError('trial roots are not empty')
                functions = dict(A=self.run_config_a, B=self.run_config_b)
                # Prepopulate both arms before every populated observation, outside timing records.
                if condition == 'populated_store':
                    for arm in ('A','B'):
                        functions[arm](fixtures, input_text, roots[arm])
                before = {arm: len(list(root.rglob('*'))) for arm,root in roots.items()}
                order = ['A','B']; rng.shuffle(order)
                results = {arm:functions[arm](fixtures,input_text,roots[arm]) for arm in order}
                if any(results['A'][key] != results['B'][key] for key in ('result_ref','model_hash','decision','policy_result')):
                    raise ValueError('observation parity failed; stop measurement')
                observations.append(dict(condition=condition,index=index,order=order,
                                         roots={k:str(v) for k,v in roots.items()},
                                         entries_before=before,initial_entries=initial_counts,arms=results))
        summaries = {}
        for condition in ('empty_store','populated_store'):
            summaries[condition] = {}
            for arm in ('A','B'):
                values = [row['arms'][arm]['end_to_end_process_ns'] for row in observations if row['condition']==condition]
                summaries[condition][arm] = dict(count=len(values),mean_ns=statistics.mean(values),
                                                sample_stddev_ns=statistics.stdev(values) if len(values)>1 else None,
                                                min_ns=min(values),max_ns=max(values))
        return dict(seed=seed, invocation=str(invocation), observations=observations,summaries=summaries,
                    performance_disposition='INCONCLUSIVE: pure Python versus optimized C++ CLI; different replay guarantees and uninstrumented T81 phases; no architectural advantage/disadvantage inference')
