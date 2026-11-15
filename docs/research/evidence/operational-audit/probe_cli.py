#!/usr/bin/env python3
"""Pinned CLI audit in scratch: benign fixtures only; no downloads or host data."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[4]
ROUTES = {
'ai-tasks':('ai task',dict(assess='assess',route='route',classify='classify',chain='chain',status='status',history='history')),
'bundle-creator':('bundle',dict(zip(['create-bundle','bundle-from-execution','bundle-info','sign-bundle','verify-bundle','merge-bundles','extract-bundle'],['create','from-execution','info','sign','verify','merge','extract']))),
'bundle-validator':('bundle',{}),'canonfs-export':('canonfs',{}),'canonfs-import':('canonfs',{}),
'governance-demo':('demo',dict(zip(['demo-governance','demo-policy','demo-determinism','tutorial-policy','interactive-policy','compliance-demo'],['governance','policy','determinism','tutorial-policy','interactive-policy','compliance']))),
'model-loader':('model',{'probe-model':'probe','validate-model':'validate','convert-model':'convert'}),
'policy-checker':('axion',{}),'t81-compiler':('code',{}),
't81-debugger':('debug',{'debug':'start'}),'t81vm-runner':('vm',{}),
'tensor-ops':('tensor',dict(zip(['tensor-math','tensor-reshape','tensor-validate','tensor-convert','tensor-stats','tensor-batch'],['math','reshape','validate','convert','stats','batch'])))}


def catalog():
    rows=[]
    for path in sorted((ROOT/'skills').glob('*/SKILL.md')):
        section=path.read_text().split('### Commands',1)[1].split('### Examples',1)[0]
        syntax=[b.strip().splitlines()[0] for b in re.findall(r'```[^\n]*\n(.*?)```',section,re.S)]
        groups={}
        for s in syntax: groups.setdefault(s.split()[0],[]).append(s)
        prefix,mapping=ROUTES[path.parent.name]
        for command,forms in groups.items():
            rows.append(dict(id=path.parent.name+'.'+command,skill=path.parent.name,
                command=command,advertised_syntax=forms,
                intended_cli=['t81',*prefix.split(),mapping.get(command,command)],
                wrapper=str(path.parent.relative_to(ROOT)/(command+'.sh')),
                wrapper_exists=(path.parent/(command+'.sh')).is_file()))
    return rows


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--t81-bin',required=True,type=Path)
    p.add_argument('--output',required=True,type=Path)
    args=p.parse_args()
    if args.output.exists():p.error('output must not exist')
    binary=args.t81_bin.resolve();observations=[]
    with tempfile.TemporaryDirectory(prefix='t81-native-audit-') as temporary:
        scratch=Path(temporary);home=scratch/'home';home.mkdir()
        env=dict(os.environ,HOME=str(home),T81_CANONFS_ROOT=str(scratch/'store'),TMPDIR=str(scratch),PATH=str(binary.parent)+os.pathsep+os.environ['PATH'])
        def run(id,values,entry=None):
            before={str(p.relative_to(scratch)):hashlib.sha256(p.read_bytes()).hexdigest() for p in scratch.rglob('*') if p.is_file()}
            command=[str(entry or binary),*values]
            result=subprocess.run(command,cwd=scratch,env=env,
                                  capture_output=True,text=True,timeout=30,input='quit\n')
            after={str(p.relative_to(scratch)):hashlib.sha256(p.read_bytes()).hexdigest() for p in scratch.rglob('*') if p.is_file()}
            try:payload=json.loads(result.stdout)
            except ValueError:payload=None
            record=dict(id=id,argv=command,exit_code=result.returncode,
                        stdout=result.stdout,stderr=result.stderr,json_payload=payload,
                        changed_paths=sorted(k for k in set(before)|set(after) if before.get(k)!=after.get(k)))
            observations.append(record);return record
        for row in catalog():
            run('advertised-route:'+row['id'],row['intended_cli'][1:]+['--json'])
        source=scratch/'safe.t81';source.write_text('fn main() -> i32 { print(47); return 0; }\n')
        policy=scratch/'allow.apl';policy.write_text('(policy (tier 1))\n')
        tisc=scratch/'safe.tisc'
        run('direct.code-build',['code','build',str(source),'--output',str(tisc),'--json'])
        if tisc.exists():
            run('direct.code-disasm',['code','disasm',str(tisc)])
            run('direct.vm-run',['vm','run',str(tisc),'--policy',str(policy)])
            run('direct.vm-trace',['vm','trace',str(tisc),'--policy',str(policy),'--out',str(scratch/'trace.txt')])
            run('direct.vm-state',['vm','state',str(tisc),'--json'])
            invalid=scratch/'invalid.apl';invalid.write_text('not a policy')
            run('negative.vm-invalid-policy',['vm','run',str(tisc),'--policy',str(invalid)])
            run('negative.code-run-missing-policy',['code','run',str(tisc),'--policy',str(scratch/'missing.apl')])
            run('negative.missing-policy',['vm','run',str(tisc),'--policy',str(scratch/'missing.apl')])
        run('wrapper-minimal.canonfs-import',[str(ROOT/'skills/canonfs-import/import.sh'),str(source)],entry='/bin/bash')
        run('wrapper-minimal.compiler-build',[str(ROOT/'skills/t81-compiler/build.sh'),str(source)],entry='/bin/bash')
        run('direct.policy-validate',['policy','validate',str(policy),'--json'])
        imported=run('direct.canonfs-import',['canonfs','import',str(source),'--canonfs-root',str(scratch/'store'),'--json'])
        payload=imported['json_payload'] or {};objects=payload.get('imported_objects',[])
        if objects and isinstance(objects[0],str):
            ref=objects[0]
            run('direct.canonfs-stat',['canonfs','stat',ref,'--canonfs-root',str(scratch/'store'),'--json'])
            run('direct.canonfs-verify',['canonfs','verify',ref,'--canonfs-root',str(scratch/'store'),'--json'])
            run('direct.canonfs-export',['canonfs','export',ref,'--out',str(scratch/'exported.t81'),'--canonfs-root',str(scratch/'store'),'--json'])
            run('negative.export-deny-all',['canonfs','export',ref,'--out',str(scratch/'denied-export'),'--canonfs-root',str(scratch/'store'),'--policy-profile','deny-all','--json'])
        run('negative.import-deny-all',['canonfs','import',str(source),'--canonfs-root',str(scratch/'deny-store'),'--policy-profile','deny-all','--json'])
        run('negative.import-missing-policy',['canonfs','import',str(source),'--canonfs-root',str(scratch/'missing-policy-store'),'--policy',str(scratch/'missing.apl'),'--json'])
        header=json.dumps({'w':{'dtype':'F32','shape':[1],'data_offsets':[0,4]}},separators=(',',':')).encode()
        model=scratch/'tiny.safetensors';model.write_bytes(struct.pack('<Q',len(header))+header+struct.pack('<f',1.0))
        run('direct.model-import',['model','import',str(model),'--json'])
        run('negative.wrapper-build-flags',['code','build',str(source),'--optimize','2','--json'])
        run('negative.wrapper-disasm-flags',['code','disasm',str(tisc),'--format','binary','--json'])
        run('negative.wrapper-export-verify',['canonfs','export','sha3-256:'+'0'*64,'--verify','--json'])
    receipt=dict(scope='Pinned real CLI, self-authored benign fixtures and scratch filesystem; no external model/API or destructive host operations',
       repository_revision=subprocess.check_output(['git','-C',str(ROOT),'rev-parse','HEAD'],text=True).strip(),
       binary=str(binary),binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),
       environment=dict(platform=platform.platform(),python=platform.python_version()),observations=observations)
    args.output.write_text(json.dumps(receipt,indent=2)+'\n')
    print(json.dumps(dict(route_probes=len(catalog()),direct_successes=[o['id'] for o in observations if o['id'].startswith('direct.') and o['exit_code']==0],output=str(args.output))))


if __name__=='__main__':main()
