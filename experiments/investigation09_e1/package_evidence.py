#!/usr/bin/env python3
"""Losslessly archive raw E1 evidence and verify every stored byte before release."""
import argparse
import gzip
import hashlib
import io
import json
from pathlib import Path
import shutil
import tarfile


def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()


def main():
    ap=argparse.ArgumentParser()
    ap.add_argument('raw',type=Path)
    ap.add_argument('destination',type=Path)
    args=ap.parse_args()
    raw=args.raw.resolve(); dest=args.destination.resolve()
    dest.mkdir(parents=True,exist_ok=False)
    for name in ('manifest.json','audit-manifest.json','comparison.json','ledger.json'):
        shutil.copyfile(raw/name,dest/name)
    selected=dest/'selected-artifacts'; selected.mkdir()
    info=json.loads((raw/'comparison.json').read_text())['inventory']
    keep=[e for e in info if e['kind'].endswith('tisc')]
    for module in sorted({e['module'] for e in info}):
        keep.append(min((e for e in info if e['module']==module and e['kind']=='wasm'),key=lambda e:e['bytes']))
    for e in keep:
        shutil.copyfile(raw/e['path'],selected/Path(e['path']).name)
    paths=sorted(p for p in raw.rglob('*') if p.is_file())
    expected={str(p.relative_to(raw)):sha(p) for p in paths}
    archive=dest/'raw-evidence.tar.gz'
    with archive.open('wb') as stream:
        with gzip.GzipFile(fileobj=stream,mode='wb',mtime=0,filename='') as compressed:
            with tarfile.open(fileobj=compressed,mode='w') as tar:
                for p in paths:
                    b=p.read_bytes(); record=tarfile.TarInfo('data/'+str(p.relative_to(raw)))
                    record.size=len(b); record.mode=0o644; record.mtime=0
                    tar.addfile(record,io.BytesIO(b))
    actual={}
    with tarfile.open(archive,'r:gz') as tar:
        for member in tar:
            if not member.isfile(): raise ValueError('unexpected archive entry')
            actual[member.name.removeprefix('data/')]=hashlib.sha256(tar.extractfile(member).read()).hexdigest()
    if actual!=expected: raise ValueError('archive lost or changed evidence bytes')
    (dest/'archive-contents-sha256.json').write_text(json.dumps(expected,sort_keys=True,indent=2)+'\n')
    manifest={str(p.relative_to(dest)):sha(p) for p in sorted(dest.rglob('*')) if p.is_file()}
    (dest/'SHA256SUMS.json').write_text(json.dumps(manifest,sort_keys=True,indent=2)+'\n')
    print(json.dumps({'archived_files':len(paths),'archive_bytes':archive.stat().st_size,'sha256':sha(archive)}))


if __name__=='__main__': main()
