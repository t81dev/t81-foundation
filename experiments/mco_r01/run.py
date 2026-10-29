#!/usr/bin/env python3
"""Execute bounded research processes; never execute model-produced commands."""
import argparse
from collections import Counter
from copy import deepcopy
import gzip
import hashlib
import json
from pathlib import Path
import platform
import random
import shutil
import subprocess
import sys

ROOT=Path(__file__).resolve().parent
REPO=ROOT.parents[1]
PACKET=REPO/'docs/research/model-computational-object'

def canonical(v):return json.dumps(v,sort_keys=True,ensure_ascii=True,separators=(',',':')).encode()
def require(condition,description):
    if not condition:raise RuntimeError(description)

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--output-dir',required=True,type=Path)
    args=parser.parse_args()
    out=args.output_dir.resolve();out.mkdir(parents=True,exist_ok=False)
    ruby=shutil.which('ruby');require(ruby is not None,'Ruby JSON/BigDecimal oracle is required')
    commands=[]
    def execute(name,command,codes=(0,),parse=True):
        process=subprocess.run(command,cwd=REPO,capture_output=True)
        (out/(name+'.stdout.gz')).write_bytes(gzip.compress(process.stdout,mtime=0))
        (out/(name+'.stderr.gz')).write_bytes(gzip.compress(process.stderr,mtime=0))
        commands.append(dict(name=name,command=command,returncode=process.returncode,
                             stdout_sha256=hashlib.sha256(process.stdout).hexdigest()))
        (out/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
        require(process.returncode in codes,f'{name}: unexpected exit {process.returncode}; see preserved stderr')
        return (json.loads(process.stdout) if parse else process.stdout),process.returncode
    manifest=json.loads((PACKET/'review/manifest.json').read_text())
    for file,entry in manifest['files'].items():
        require(hashlib.sha256((PACKET/file).read_bytes()).hexdigest()==entry['sha256'],'frozen packet mismatch: '+file)
    frozen_hash=hashlib.sha256((PACKET/'review/manifest.json').read_bytes()).hexdigest()
    oracle,_=execute('oracle',[ruby,str(ROOT/'oracle.rb'),str(PACKET)])
    require(oracle['agreement']==60 and oracle['disagreements']==0,'STOP: independent oracle disagrees with frozen cases')
    require(all(not p['mismatched_fields'] for p in oracle['plans']),'STOP: oracle disagrees with temporal specification')
    # Only after that gate do candidate arms execute.
    chooser=random.SystemRandom()
    trace=dict(source='OS entropy chooses inert frozen proposals; fresh independently authorized owner per draw',
               choices=[chooser.choice(['F01-C01','F01-C02','F01-C03']) for _ in range(32)])
    trace_path=out/'proposal-trace.json';trace_path.write_text(json.dumps(trace,indent=2)+'\n')
    cases=[json.loads(x) for x in (PACKET/'corpus/cases.jsonl').read_text().splitlines()]
    frozen_plans={json.loads(x)['order']:json.loads(x) for x in (PACKET/'corpus/event-permutations.jsonl').read_text().splitlines()}
    observations={};checks={};arm_counts={}
    def save_observation(name,value):
        path=out/(name+'.json');path.write_text(json.dumps(value,sort_keys=True)+'\n');return path
    for arm in ['A','B']:
        command=[sys.executable,str(ROOT/'driver.py'),'--arm',arm,'--proposal-trace',str(trace_path)]
        observed,_=execute('arm-'+arm,command)
        replay,_=execute('replay-'+arm,command)
        require(observed==replay,'recorded proposal replay changed observed output')
        observations[arm]=observed
        mismatches=[]
        for case,actual in zip(cases,observed['cases']):
            mismatches.extend([case['case_id'],key] for key,value in case['expected'].items() if actual['summary'].get(key)!=value)
        require(len(observed['cases'])==60 and not mismatches,'arm '+arm+' frozen case mismatch: '+str(mismatches))
        for plan in observed['plans']:
            expected=frozen_plans[plan['order']]
            require(all(plan[k]==v for k,v in expected.items() if k!='reason'),'arm '+arm+' plan mismatch')
        path=save_observation('observed-'+arm,observed)
        reconstructed,_=execute('reconstruct-'+arm,[ruby,str(ROOT/'oracle.rb'),str(PACKET),'--reconstruct',str(path),str(trace_path)])
        require(reconstructed['valid'],'independent reconstruction rejected control')
        checks[arm]=reconstructed
        arm_counts[arm]=dict(cases=60,case_disagreements=0,abstract_guards=len(observed['guards']),meet_pairs=len(observed['pairs']),
                             associativity_triples=len(observed['triples']),reachable_plans=sum(p['reachable'] for p in observed['plans']),
                             invalid_plans=sum(not p['reachable'] for p in observed['plans']),
                             protected_dispatches=sum(c['summary']['effect_dispatches'] for c in observed['cases']),
                             new_rows=sum(c['summary']['new_rows'] for c in observed['cases']),
                             independent_reconstructions=len(reconstructed['checks'])+sum(p['reachable'] for p in observed['plans']),
                             recorded_proposal_draws=32,replay_identical=True)
    require([x['summary'] for x in observations['A']['cases']]==[x['summary'] for x in observations['B']['cases']],'A/B behavioral difference')
    require([x['summary'] for x in observations['A']['proposal_choices']]==[x['summary'] for x in observations['B']['proposal_choices']],'A/B proposal trace difference')
    mutations=[]
    witnesses={'permit_unknown':'F06-C01','skip_revocation':'F10-C02','skip_replay':'F08-C05','omit_model_binding':'F09-C02'}
    for arm in ['A','B']:
        for mutant,witness in witnesses.items():
            observed,rc=execute('mutant-'+arm+'-'+mutant,[sys.executable,str(ROOT/'driver.py'),'--arm',arm,'--mutant',mutant])
            row=next(c for c in observed['cases'] if c['case_id']==witness)
            summary=row['summary']
            if mutant=='permit_unknown':target=summary['decision']=='P' and summary['guards'][2]=='U'
            elif mutant=='skip_revocation':target=summary['new_rows']==1 and next(j for j in row['journal'] if j['event']=='A')['before']['policy_state']=='revoked'
            elif mutant=='skip_replay':target=summary['new_rows']==2 and summary['admissions'][1]['decision']=='P'
            else:target=summary['decision']=='P' and row['input_context']['observed_model_ref']!=row['input_context']['approved_model_ref']
            require(target,'mutant failed to reach intended semantic violation: '+mutant)
            path=save_observation('mutated-'+arm+'-'+mutant,observed)
            rejection,rrc=execute('reject-'+arm+'-'+mutant,[ruby,str(ROOT/'oracle.rb'),str(PACKET),'--reconstruct',str(path)],codes=(2,))
            check=next(c for c in rejection['checks'] if c['case_id']==witness)
            require(not check['valid'] and check['mismatches'],'semantic mutant was not independently detected')
            mutations.append(dict(arm=arm,mutation=mutant,witness=witness,process_exit=rc,reconstructor_exit=rrc,
                                  target_reached=True,killed=True,mismatches=check['mismatches'],
                                  actual_decision=summary['decision'],actual_new_rows=summary['new_rows']))
    # Independent reconstruction must reject internally plausible forged records.
    tampers=[]
    for attack in ['reported_permit','rehash_wrong_ledger','omit_revocation','erase_observation','omit_cases']:
        forged=deepcopy(observations['A'])
        index={c['case_id']:c for c in forged['cases']}
        if attack=='reported_permit':index['F06-C01']['summary']['decision']='P'
        elif attack=='rehash_wrong_ledger':
            row=index['F01-C01']['ledger_rows'][0];row['artifact_ref']='sha256:'+'f'*64
            receipt=index['F01-C01']['receipts'][0];receipt['row']=deepcopy(row)
            receipt['receipt_ref']='sha256:'+hashlib.sha256(b'receipt\0'+canonical(row)).hexdigest()
        elif attack=='omit_revocation':index['F10-C02']['journal']=[j for j in index['F10-C02']['journal'] if j['event']!='V']
        elif attack=='erase_observation':index['F07-C01']['summary']['evidence_observations']=[]
        else:forged['cases']=[]
        path=save_observation('tamper-'+attack,forged)
        result,rc=execute('detect-'+attack,[ruby,str(ROOT/'oracle.rb'),str(PACKET),'--reconstruct',str(path),str(trace_path)],codes=(2,))
        require(result['valid'] is False,'tampered evidence accepted')
        tampers.append(dict(attack=attack,rejected=True,returncode=rc))
    for file,entry in manifest['files'].items():
        require(hashlib.sha256((PACKET/file).read_bytes()).hexdigest()==entry['sha256'],'frozen evidence changed during experiment')
    result=dict(experiment='MCO-R01',base_commit='633645afad1ac6160f2056dcd825e2590898fca2',
                python=platform.python_version(),platform=platform.platform(),ruby=oracle['ruby'],ruby_json=oracle['json'],
                frozen_manifest_sha256=frozen_hash,oracle_cases=60,oracle_disagreements=0,arms=arm_counts,
                arm_behaviorally_equivalent=True,mutations=mutations,mutation_kills=len(mutations),mutation_runs=len(mutations),
                reconstruction_tampers=tampers,proposal_choice_counts=dict(Counter(trace['choices'])),
                failed_current_checks=0,scope='finite synthetic conformance under honest single-owner/observer assumptions; no opaque-model or host-compromise guarantee',
                recommendation='REVISE',os_primitive_justified=False,ternary_native_implementation_justified=False)
    (out/'results.json').write_text(json.dumps(result,sort_keys=True,indent=2)+'\n')
    print(json.dumps(result,indent=2))

if __name__=='__main__':main()
