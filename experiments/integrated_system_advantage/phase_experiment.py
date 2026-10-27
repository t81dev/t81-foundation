#!/usr/bin/env python3
"""Investigation 08: gate OFF/ON noninterference, then collect exploratory phase costs."""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import statistics
import subprocess
import sys
import tempfile
import time
import uuid

from ablation_runner import AblationHarness
from baseline_stack import canon_sha3_256_ref
from phase_records import aggregate, validate_record

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
EXT = '.exe' if os.name == 'nt' else ''


def run(command, sidecar=None, expect_timing=False, check=True):
    env = dict(os.environ, T81_CANONFS_READ_VERIFY='1')
    env.pop('T81_I08_TIMING_OUT', None)
    if sidecar:
        env['T81_I08_TIMING_OUT'] = str(sidecar)
    start = time.perf_counter_ns()
    proc = subprocess.run([str(v) for v in command],env=env,capture_output=True,text=True,timeout=240)
    wall = time.perf_counter_ns()-start
    if check and proc.returncode:
        raise RuntimeError(f'{command[0]} exit {proc.returncode}: {proc.stderr}')
    record = None
    if expect_timing:
        if not sidecar or not sidecar.is_file():
            raise ValueError('missing phase sidecar')
        record = json.loads(sidecar.read_text())
        application = validate_record(record)
        if application > wall:
            raise ValueError('application window exceeds parent wall time')
    elif sidecar and sidecar.exists():
        raise ValueError('OFF binary unexpectedly recorded timing')
    return proc, dict(wall_ns=wall,phase_record=record,
                      phase_aggregates=aggregate(record) if record else None,
                      boundary_residual_ns=wall-validate_record(record) if record else None)


def gated_parity(off, on, directory):
    reports = {}
    for label, build in [('OFF',off),('ON',on)]:
        output = directory/(label+'-parity.json')
        env = dict(os.environ,T81_BUILD_DIR=str(build),T81_PARITY_REPORT=str(output),
                   T81_I08_NONINTERFERENCE='1')
        env.pop('T81_I08_TIMING_OUT',None);env.pop('T81_I08_TRACE_DIR',None)
        if label=='ON':env['T81_I08_TRACE_DIR']=str(directory/'gate-sidecars')
        proc = subprocess.run([sys.executable,str(HERE/'test_differential_parity.py')],
                              env=env,capture_output=True,text=True)
        (directory/(label+'-parity.log')).write_text(proc.stdout+proc.stderr)
        if proc.returncode:
            raise RuntimeError(label+' noninterference corpus failed; no measurements permitted')
        reports[label]=json.loads(output.read_text())
        print(label+' corpus passed',flush=True)
    if len(reports['OFF']['valid_scenarios']) != 15 or reports['OFF'] != reports['ON']:
        raise ValueError('OFF/ON semantic fingerprints differ')
    # A timing-enabled build is also idle unless explicitly requested.
    off_version,_=run([off/('t81'+EXT),'--version'],directory/'disabled-version.json')
    on_idle,_=run([on/('t81'+EXT),'--version'])
    if (off_version.returncode,off_version.stdout,off_version.stderr)!=(on_idle.returncode,on_idle.stdout,on_idle.stderr):
        raise ValueError('enabled but idle build differs')
    # Failed timing output must not alter CLI results or produce deterministic metadata.
    failed,_=run([on/('t81'+EXT),'--version'],directory/'absent-parent/fail.json')
    if (failed.returncode,failed.stdout,failed.stderr)!=(on_idle.returncode,on_idle.stdout,on_idle.stderr):
        raise ValueError('sidecar I/O failure changed task result')
    sidecars=list((directory/'gate-sidecars').glob('*.json'))
    for path in sidecars:validate_record(json.loads(path.read_text()))
    return dict(corpus=reports['OFF'],validated_sidecars=len(sidecars),
                gate_logs=['OFF-parity.log','ON-parity.log'],same_semantics=True)


def task_trial(build, enabled, fixtures, store, directory):
    cli=build/('t81'+EXT)
    command=[cli,'ai','task','assess-fixed','--model','investigation08',
             '--model-file',fixtures['model_path'],'--policy',fixtures['apl_policy_path'],
             '--canonfs-root',store,'--mode','strict_deterministic','--input','greet hello']
    proc,execution=run(command,directory/'execution.json',enabled)
    output=json.loads(proc.stdout)
    artifacts={}; lookups={}
    for key in ('result_ref','provenance_ref'):
        target=directory/(key+'.json')
        _,lookups[key]=run([cli,'canonfs','get',output[key],'--out',target,'--canonfs-root',store],
                           directory/(key+'-timing.json'),enabled)
        raw=target.read_bytes()
        if canon_sha3_256_ref(raw)!=output[key]:raise ValueError('artifact hash mismatch')
        artifacts[key]=json.loads(raw)
    again,replay=run(command,directory/'replay.json',enabled)
    if json.loads(again.stdout)!=output:raise ValueError('CLI replay changed result')
    evidence=artifacts['provenance_ref']['execution_evidence']
    return dict(execution=execution,lookup=lookups,replay=replay,verified=True,
                identity={k:output[k] for k in ('model_hash','policy_hash','input_hash','result_ref','provenance_ref','policy_result','decision_token_ids')},
                candidate_scores=evidence['sampled_token_scores'],candidate_ids=evidence['sampled_token_ids'],
                replay_kind='fresh CLI re-execution from original fixtures, not standalone CAS model recovery')


def conventional_trial(fixtures, store):
    worker=HERE/'baseline_worker.py'
    proc,execution=run([sys.executable,worker,'run','--store',store,'--model',fixtures['model_path'],
                        '--policy',fixtures['apl_policy_path'],'--tokenizer',fixtures['tokenizer_path'],
                        '--input','greet hello'])
    output=json.loads(proc.stdout)
    recovered,replay=run([sys.executable,worker,'replay','--store',store,'--bundle',output['bundle_ref']])
    verification=json.loads(recovered.stdout)
    if not verification['valid']:raise ValueError('conventional replay failed')
    return dict(execution=execution,replay=replay,verified=True,
                identity={k:output[k] for k in ('model_hash','policy_hash','input_hash','result_ref','policy_result','decision_token_ids')},
                candidate_scores=[v for v,_ in output['candidate_scores']],candidate_ids=[v for _,v in output['candidate_scores']],
                phase_timings_ns=output['timings_ns'],module_initialization_ns=output['worker_module_initialization_ns'],
                boundary_residual_ns=execution['wall_ns']-output['worker_total_ns'],
                verification=verification,replay_kind='persisted-input CAS recovery and re-execution')


def collect(off,on,directory,count,warm_repeats,seed):
    harness=AblationHarness(ROOT,directory,off)
    fixtures=harness.prepare_fixtures()
    rows=[];rng=random.Random(seed)
    for index in range(count):
        pair=Path(tempfile.mkdtemp(prefix=f'observation-{index}-',dir=directory))
        roots={arm:pair/arm/'store' for arm in ('OFF','ON','conventional')}
        for store in roots.values():store.mkdir(parents=True,exist_ok=False)
        before={arm:len(list(store.rglob('*'))) for arm,store in roots.items()}
        if any(before.values()):raise ValueError('nonempty fresh trial')
        order=list(roots);rng.shuffle(order);arms={}
        for arm in order:
            arms[arm]=conventional_trial(fixtures,roots[arm]) if arm=='conventional' else task_trial(
                off if arm=='OFF' else on,arm=='ON',fixtures,roots[arm],pair/arm)
        if arms['OFF']['identity']!=arms['ON']['identity']:raise ValueError('OFF/ON trial identity mismatch')
        for key in arms['conventional']['identity']:
            if arms['conventional']['identity'][key]!=arms['ON']['identity'][key]:raise ValueError('conventional trial identity mismatch')
        if not arms['OFF']['candidate_scores']==arms['ON']['candidate_scores']==arms['conventional']['candidate_scores']:
            raise ValueError('candidate score mismatch')
        rows.append(dict(index=index,order=order,initial_entries=before,roots={k:str(v) for k,v in roots.items()},arms=arms))
        print(f'cold observation {index+1}/{count} complete',flush=True)
    warm=[]
    for arm,build in [('OFF',off),('ON',on)]:
        sidecar=directory/(arm+'-warm.json')
        proc,record=run([build/('t81_investigation08_probe'+EXT),fixtures['model_path'],fixtures['tokenizer_path'],
                         'greet hello',str(warm_repeats)],sidecar,arm=='ON')
        warm.append(dict(arm=arm,repetitions=warm_repeats,record=record,semantic_output=json.loads(proc.stdout)))
    if warm[0]['semantic_output']!=warm[1]['semantic_output']:raise ValueError('warm OFF/ON mismatch')
    def stats(values):
        return dict(n=len(values),mean_ns=statistics.mean(values),sample_stddev_ns=statistics.stdev(values) if len(values)>1 else None,
                    min_ns=min(values),max_ns=max(values),median_ns=statistics.median(values),raw_ns=values)
    summaries={arm:stats([r['arms'][arm]['execution']['wall_ns'] for r in rows]) for arm in roots}
    differences=[r['arms']['ON']['execution']['wall_ns']-r['arms']['OFF']['execution']['wall_ns'] for r in rows]
    return dict(fixtures={key:canon_sha3_256_ref(Path(fixtures[key]).read_bytes()) for key in ('model_path','apl_policy_path','tokenizer_path')},
                observations=rows,warm_native_api=warm,summary=summaries,paired_overhead=stats(differences),seed=seed,
                warm_limit='Loaded model reused within each native probe process; compilation and VM initialization repeat. Not a warmed full CLI.',
                process_boundary_limit='Parent wall minus application window combines startup, teardown, recorder output and pipe overhead. It is not isolated startup.',
                architectural_disposition='INCONCLUSIVE: bounded corpus, different optimizations/replay guarantees, no matched architectural ablation')


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--off-build',required=True,type=Path);parser.add_argument('--on-build',required=True,type=Path)
    parser.add_argument('--out',required=True,type=Path);parser.add_argument('--work-dir',required=True,type=Path)
    parser.add_argument('--observations',type=int,default=5);parser.add_argument('--warm-repeats',type=int,default=4)
    parser.add_argument('--seed',type=int,default=608)
    args=parser.parse_args()
    if args.out.exists():parser.error('refusing to overwrite evidence')
    if args.observations<2 or not 2<=args.warm_repeats<=20:parser.error('at least two observations/repetitions required')
    args.work_dir.mkdir(parents=True,exist_ok=True)
    work=Path(tempfile.mkdtemp(prefix='phase-',dir=args.work_dir))
    data=dict(timestamp=datetime.now(timezone.utc).isoformat(),source_commit=subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip(),
              source_diff_sha256=hashlib.sha256(subprocess.check_output(['git','diff','HEAD'],cwd=ROOT)).hexdigest(),
              environment=dict(platform=platform.platform(),machine=platform.machine(),python=sys.version),
              build_flags={arm:(build/'CMakeCache.txt').read_text() for arm,build in [('OFF',args.off_build),('ON',args.on_build)]},
              binaries={arm:{name:hashlib.sha256((build/(name+EXT)).read_bytes()).hexdigest() for name in ('t81','t81_investigation08_probe')}
                        for arm,build in [('OFF',args.off_build),('ON',args.on_build)]})
    try:
        data['noninterference']=gated_parity(args.off_build,args.on_build,work)
        data['measurement']=collect(args.off_build,args.on_build,work,args.observations,args.warm_repeats,args.seed)
        data['status']='BOUNDED NONINTERFERENCE PASSED; EXPLORATORY COST ATTRIBUTION; INCONCLUSIVE ARCHITECTURAL ADVANTAGE'
    except Exception as error:
        data['status']='FAILED; NO PERFORMANCE CONCLUSION';data['error']=str(error)
        raise
    finally:
        data['work_directory']=str(work)
        args.out.parent.mkdir(parents=True,exist_ok=True)
        with args.out.open('x') as stream:json.dump(data,stream,indent=2,ensure_ascii=False);stream.write('\n')
    print(data['status'])


if __name__=='__main__':main()
