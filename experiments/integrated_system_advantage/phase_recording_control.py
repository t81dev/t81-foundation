#!/usr/bin/env python3
"""Same timing-enabled binary, idle versus active recording; exploratory observer control."""
import argparse
import hashlib
import json
from pathlib import Path
import random
import tempfile

from ablation_runner import AblationHarness
from phase_experiment import EXT, ROOT, run
from summarize_phase_costs import stats


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--gate-packet',required=True,type=Path)
    parser.add_argument('--on-build',required=True,type=Path)
    parser.add_argument('--out',required=True,type=Path)
    parser.add_argument('--work-dir',required=True,type=Path)
    parser.add_argument('--pairs',type=int,default=5);parser.add_argument('--seed',type=int,default=609)
    args=parser.parse_args()
    if args.out.exists() or args.pairs<2:parser.error('new output and at least two pairs required')
    gate=json.loads(args.gate_packet.read_text())
    if not gate['status'].startswith('BOUNDED NONINTERFERENCE PASSED'):
        parser.error('successful semantic gate required')
    cli=args.on_build/('t81'+EXT)
    digest=hashlib.sha256(cli.read_bytes()).hexdigest()
    if digest!=gate['binaries']['ON']['t81']:parser.error('binary differs from gated binary')
    args.work_dir.mkdir(parents=True,exist_ok=True)
    work=Path(tempfile.mkdtemp(prefix='recording-control-',dir=args.work_dir))
    fixtures=AblationHarness(ROOT,work,args.on_build).prepare_fixtures()
    rng=random.Random(args.seed);observations=[]
    for index in range(args.pairs):
        order=['idle','active'];rng.shuffle(order);arms={}
        for arm in order:
            directory=work/(str(index)+'-'+arm);directory.mkdir()
            store=directory/'store';store.mkdir()
            if list(store.iterdir()):raise ValueError('nonempty control store')
            command=[cli,'ai','task','assess-fixed','--model','investigation08',
                     '--model-file',fixtures['model_path'],'--policy',fixtures['apl_policy_path'],
                     '--canonfs-root',store,'--mode','strict_deterministic','--input','greet hello']
            proc,measurement=run(command,directory/'timing.json' if arm=='active' else None,arm=='active')
            output=json.loads(proc.stdout)
            arms[arm]=dict(measurement=measurement,output=output,store=str(store),initial_entries=0)
        if arms['idle']['output']!=arms['active']['output']:raise ValueError('idle/active semantic output mismatch')
        observations.append(dict(index=index,order=order,arms=arms))
        print(f'recording pair {index+1}/{args.pairs} complete',flush=True)
    differences=[row['arms']['active']['measurement']['wall_ns']-row['arms']['idle']['measurement']['wall_ns'] for row in observations]
    data=dict(binary_sha256=digest,gate_packet_sha256=hashlib.sha256(args.gate_packet.read_bytes()).hexdigest(),
              seed=args.seed,observations=observations,active_minus_idle_ns=stats(differences),
              idle_wall_ns=stats([r['arms']['idle']['measurement']['wall_ns'] for r in observations]),
              active_wall_ns=stats([r['arms']['active']['measurement']['wall_ns'] for r in observations]),
              limitation='Same executable reduces build-codegen confounding; allocation, scheduling, thermal and cache effects remain. Net observer perturbation, not a pure clock-call microbenchmark.')
    with args.out.open('x') as stream:json.dump(data,stream,indent=2,ensure_ascii=False);stream.write('\n')


if __name__=='__main__':main()
