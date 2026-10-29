"""Re-execute PR #554 storage payloads, preserving before/after and every failure."""
import argparse
import ctypes
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys
import tarfile

HERE = Path(__file__).resolve().parent
ALPHABET = '0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-−×÷=<>≤≥≠≈∞λμπστωΓ'
ORIGINAL_SHA256 = 'aa28275cff9f612fe69a255d7b3a9f616b7b7af278a5ce6bfbd5279105314d14'


def digest(data):
    return hashlib.sha3_512(data).digest()[:32]


def display(data):
    number, result = int.from_bytes(digest(data), 'big'), ''
    while number:
        number, i = divmod(number, 81)
        result = ALPHABET[i] + result
    return result or '0'


def snapshot(root):
    return {p.relative_to(root).as_posix():p.read_bytes().hex()
            for p in root.rglob('*') if p.is_file()}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--probe', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=False)
    probe = args.probe.resolve()
    if os.name == 'nt' and not probe.exists(): probe = probe.with_suffix('.exe')
    source = HERE/'original-554/windows-failure.tar.gz'
    if hashlib.sha256(source.read_bytes()).hexdigest() != ORIGINAL_SHA256:
        raise ValueError('historical archive changed')
    failures, results = [], []
    with tarfile.open(source,'r:gz') as archive:
        historical = json.loads(archive.extractfile('challenge-results/report.json').read())
        failed = {row['trial'] for row in historical['failures']}
        rows = [json.loads(line) for line in archive.extractfile('challenge-results/corpus.jsonl').read().splitlines()]
        rows = [row for row in rows if row['arm']=='B']
        if len(rows)!=110 or len(failed)!=100: raise ValueError('unexpected original corpus')
        with (args.output/'raw.jsonl').open('w',encoding='utf-8') as rawlog:
            def run_one(layout, payload, trial):
                trial.mkdir(parents=True)
                inp = trial/'payload'; inp.write_bytes(payload)
                store = trial/'store'
                run = subprocess.run([str(probe),layout,str(inp),str(store)],capture_output=True,timeout=30,
                                     env={k:v for k,v in os.environ.items() if k!='T81_CANONFS_READ_VERIFY'})
                observation = dict(trial=trial.name,layout=layout,payload_hex=payload.hex(),
                                   returncode=run.returncode,stdout_hex=run.stdout.hex(),stderr_hex=run.stderr.hex(),stdout=run.stdout.decode('utf-8','backslashreplace'),
                                   stderr=run.stderr.decode('utf-8','backslashreplace'),files=snapshot(store))
                rawlog.write(json.dumps(observation,sort_keys=True)+'\n');rawlog.flush()
                if run.returncode: raise ValueError('probe failure; raw bytes retained')
                value = json.loads(run.stdout)
                if value['key']!=digest(payload).hex() or value['display']!=display(payload):
                    raise ValueError('logical identity mismatch')
                if value['written'] and not (value['cold_read'] and value['duplicate_ok']):
                    raise ValueError('created object did not verify/duplicate correctly')
                if value['written']:
                    path = ('v2/objects/'+digest(payload).hex()+'.blk') if layout=='portable' else 'objects/'+display(payload)+'.blk'
                    value['native_path_exact'] = observation['files'].get(path)==payload.hex()
                    if layout=='legacy' and os.name=='nt':
                        codepage=ctypes.windll.kernel32.GetACP()
                        native_name=display(payload).encode('utf-8').decode(f'cp{codepage}')
                        native_path='objects/'+native_name+'.blk'
                        if observation['files']!={native_path:payload.hex()}:
                            raise ValueError('legacy native-codepage path/payload mismatch')
                        value['native_codepage']=codepage
                    elif not value['native_path_exact']:
                        raise ValueError('native path/payload mismatch')
                    if layout=='portable' and observation['files'].get('canonfs-layout')!=b't81.canonfs.storage-map.v2\n'.hex():
                        raise ValueError('portable format marker mismatch')
                    if layout=='portable' and set(observation['files'])!={'canonfs-layout',path}:
                        raise ValueError('unexpected portable file inventory')
                return value
            for layout in ('legacy','portable'):
                for row in rows:
                    trial_id = f"{row['case']}-A-{row['repetition']}"
                    try:
                        committed = bool(row['bundle']['record']['payload_sha256'])
                        po = dict(written=True,native_path_exact=True)
                        if committed:
                            payload = bytes.fromhex(next(iter(row['protected_files'].values())))
                            po = run_one(layout,payload,args.output/f'{layout}-{trial_id}-po')
                        evidence_result = dict(written=False,native_path_exact=False)
                        if po['written']:
                            evidence = bytes.fromhex(next(iter(row['evidence_files'].values())))
                            evidence_result = run_one(layout,evidence,args.output/f'{layout}-{trial_id}-evidence')
                        complete = po['written'] and evidence_result['written']
                        result = dict(layout=layout,original_trial=trial_id,complete=complete,
                                      originally_failed=trial_id in failed,
                                      canonical_path_exact=complete and po.get('native_path_exact',False) and evidence_result.get('native_path_exact',False))
                        results.append(result)
                        if layout=='portable' and not complete: raise ValueError('portable corpus failed')
                        if layout=='legacy' and platform.system()=='Windows' and complete==(trial_id in failed):
                            raise ValueError('original Windows failure not reproduced')
                        if layout=='legacy' and platform.system()!='Windows' and not complete:
                            raise ValueError('unexpected native legacy failure')
                    except Exception as error:
                        failures.append(dict(layout=layout,trial=trial_id,error=str(error)))
            for i,payload in enumerate((b'',b'\0\xff\x80\n',bytes(range(256)),b'x'*65536)):
                try:
                    if not run_one('portable',payload,args.output/f'edge-{i}')['written']:
                        raise ValueError('edge payload not stored')
                except Exception as error: failures.append(dict(trial=f'edge-{i}',error=str(error)))
    report=dict(platform=platform.platform(),machine=platform.machine(),python=sys.version,
                git_sha=subprocess.check_output(['git','rev-parse','HEAD'],cwd=HERE.parents[1],text=True).strip(),
                native_ansi_codepage=ctypes.windll.kernel32.GetACP() if os.name=='nt' else None,
                source_sha256={str(path.relative_to(HERE.parents[1])):hashlib.sha256(path.read_bytes()).hexdigest()
                               for path in (HERE/'probe.cpp',HERE/'contract_test.cpp',Path(__file__),
                                            HERE.parents[1]/'fs/persistent_driver.cpp',HERE.parents[1]/'include/t81/canonfs/storage_mapping.hpp')},
                historical_archive_sha256=ORIGINAL_SHA256,original_failure_count=100,
                results=results,failures=failures,
                legacy_failures=sum(not r['complete'] for r in results if r['layout']=='legacy'),
                portable_complete=sum(r['complete'] for r in results if r['layout']=='portable'),
                legacy_completed_noncanonical_paths=sum(r['complete'] and not r['canonical_path_exact'] for r in results if r['layout']=='legacy'),
                probe_sha256=hashlib.sha256(probe.read_bytes()).hexdigest())
    (args.output/'report.json').write_text(json.dumps(report,indent=2,sort_keys=True)+'\n',encoding='utf-8')
    print(json.dumps({k:v for k,v in report.items() if k not in ('results','python')}))
    if failures: raise SystemExit(1)

if __name__=='__main__': main()
