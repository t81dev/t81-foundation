"""Offline native evidence auditor: stdlib only, no driver/harness imports."""
import argparse
import hashlib
import json
from pathlib import Path
import tarfile

ALPHABET='0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-−×÷=<>≤≥≠≈∞λμπστωΓ'
MARKER=b't81.canonfs.storage-map.v2\n'


def identity(payload):
    raw=hashlib.sha3_512(payload).digest()[:32]
    n=int.from_bytes(raw,'big');text=''
    while n:
        n,i=divmod(n,81);text=ALPHABET[i]+text
    return raw.hex(),text or '0'


def unique(pairs):
    out={}
    for key,value in pairs:
        if key in out:raise ValueError('duplicate JSON key')
        out[key]=value
    return out


def parse(data):return json.loads(data,object_pairs_hook=unique)


def audit(original,path):
    with tarfile.open(original,'r:gz') as source:
        original_rows=[parse(line) for line in source.extractfile('challenge-results/corpus.jsonl').read().splitlines()]
        originals={f"{r['case']}-A-{r['repetition']}":r for r in original_rows if r['arm']=='B'}
        original_failures={r['trial'] for r in parse(source.extractfile('challenge-results/report.json').read())['failures']}
    if len(originals)!=110 or len(original_failures)!=100:raise ValueError('wrong historical corpus')
    with tarfile.open(path,'r:gz') as a:
        names=[m.name for m in a.getmembers() if m.isfile()]
        if len(names)!=len(set(names)):raise ValueError('duplicate tar member')
        report=parse(a.extractfile('replay-output/report.json').read())
        if hashlib.sha256(Path(original).read_bytes()).hexdigest()!=report['historical_archive_sha256']:
            raise ValueError('historical archive digest mismatch')
        if report['failures']:raise ValueError('native run has unexpected failures')
        raw=[parse(line) for line in a.extractfile('replay-output/raw.jsonl').read().splitlines()]
        observed={}
        signatures={}
        for row in raw:
            trial=row['trial'];layout=row['layout'];payload=bytes.fromhex(row['payload_hex'])
            if trial in observed or layout not in ('legacy','portable'):raise ValueError('duplicate/unknown trial')
            key,display=identity(payload)
            if row['returncode']!=0:raise ValueError('probe failed')
            native=parse(bytes.fromhex(row['stdout_hex']))
            if native['key']!=key or native['display']!=display:raise ValueError('identity/display mismatch')
            if type(native['written']) is not bool:raise ValueError('invalid write result')
            prefix='replay-output/'+trial+'/'
            if a.extractfile(prefix+'payload').read()!=payload:raise ValueError('native input bytes changed')
            inventory={name[len(prefix+'store/'):]:a.extractfile(name).read().hex()
                       for name in names if name.startswith(prefix+'store/')}
            if inventory!=row['files']:raise ValueError('tar bytes differ from recorded inventory')
            if native['written']:
                if native['cold_read'] is not True or native['duplicate_ok'] is not True or native['error']!=0:
                    raise ValueError('read/duplicate check failed')
                if layout=='portable':
                    expected={'canonfs-layout':MARKER.hex(),'v2/objects/'+key+'.blk':payload.hex()}
                else:
                    codepage=report['native_ansi_codepage']
                    name=display.encode('utf-8').decode(f'cp{codepage}') if codepage else display
                    expected={'objects/'+name+'.blk':payload.hex()}
                if inventory!=expected:raise ValueError('exact path/payload mismatch')
            else:
                if layout=='portable' or native['cold_read'] is not False or native['duplicate_ok'] is not False or inventory:
                    raise ValueError('failed write published/was accepted')
            observed[trial]=dict(written=native['written'],payload=payload,key=key,display=display)
            if layout=='portable':signatures[trial]=dict(key=key,display=display,payload_hex=payload.hex())
        results={(r['layout'],r['original_trial']):r for r in report['results']}
        if len(results)!=220:raise ValueError('missing result rows')
        used=set()
        for layout in ('legacy','portable'):
            for trial,record in originals.items():
                phases=[]
                if record['bundle']['record']['payload_sha256']:
                    phases.append(('po',bytes.fromhex(next(iter(record['protected_files'].values())))))
                phases.append(('evidence',bytes.fromhex(next(iter(record['evidence_files'].values())))))
                complete=True
                for phase,payload in phases:
                    label=f'{layout}-{trial}-{phase}'
                    if label not in observed or observed[label]['payload']!=payload:raise ValueError('corpus omission/substitution')
                    used.add(label)
                    if not observed[label]['written']:complete=False;break
                result=results[(layout,trial)]
                if result['complete']!=complete or result['originally_failed']!=(trial in original_failures):
                    raise ValueError('summary disagrees with native data')
                if layout=='portable' and not complete:raise ValueError('portable corpus failure')
                if layout=='legacy' and report['native_ansi_codepage'] and complete==(trial in original_failures):
                    raise ValueError('original Windows failure set changed')
        for i,payload in enumerate((b'',b'\0\xff\x80\n',bytes(range(256)),b'x'*65536)):
            label=f'edge-{i}'
            if label not in observed or observed[label]['payload']!=payload or not observed[label]['written']:
                raise ValueError('edge corpus omission')
            used.add(label)
        if used!=set(observed):raise ValueError('unexpected raw trials')
        for log in ('contract.log','contract-diagnostics.log'):
            lines=a.extractfile(log).read().splitlines()
            checks=[parse(line) for line in lines if line.startswith(b'{')]
            if not checks or checks[-1].get('status')!='pass' or checks[-1].get('mapping_byte_position_cases')!=8192:
                raise ValueError('native contract suite did not complete')
        return dict(platform=report['platform'],machine=report['machine'],git_sha=report['git_sha'],
                    native_ansi_codepage=report['native_ansi_codepage'],legacy_failures=report['legacy_failures'],
                    legacy_noncanonical_paths=report['legacy_completed_noncanonical_paths'],portable_complete=110,
                    actual_files_verified=True,raw_probe_count=len(raw),archive_sha256=hashlib.sha256(Path(path).read_bytes()).hexdigest()),signatures


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--original',type=Path,required=True)
    p.add_argument('archives',type=Path,nargs='+')
    args=p.parse_args();reports={};previous=None
    for path in args.archives:
        result,signature=audit(args.original,path)
        if previous is not None and signature!=previous:raise ValueError('portable identities/payloads differ across native hosts')
        previous=signature;reports[str(path)]=result
    print(json.dumps(dict(native_archives=reports,portable_identities_and_bytes_equal=True),indent=2,sort_keys=True))

if __name__=='__main__':main()
