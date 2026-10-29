"""One bounded H-CONV-01 corpus, correctness gate, then six native paired batches."""
from datetime import datetime, timezone
from fractions import Fraction
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import statistics
import subprocess
import sys
import time
from oracle import convert, rational, SCALE

HERE = Path(__file__).resolve().parent


def save(path, value):
    with path.open('x') as f:
        json.dump(value,f,indent=2,ensure_ascii=False)
        f.write('\n'); f.flush(); os.fsync(f.fileno())


def corpus():
    cases = {}
    def add(bits,tag):
        if 0 <= bits <= 0xffffffff:
            cases.setdefault(bits,[]).append(tag)
    def both(bits,tag):
        for sign in (0,1):add(bits | (sign<<31),tag)
    for exponent in range(255):
        # Previous representable, exact exponent boundary and following value.
        for offset in (-1,0,1):
            b=(exponent<<23)+offset
            if b>=0:both(b,'binary32_exponent_boundary')
    for b in (0x00000002,0x00400000,0x007ffffe,0x3eaaaaab,0x3f000000,0x3e800000,
              0x3f400000,0x3f800000,0x40000000,0x40400000,0x41800000,0x7f7fffff,
              0x7f800000,0x7fc00000,0x7f800001,0x7fffffff):both(b,'explicit_special_fraction_integer')
    # Find binary32 neighbors using exact rational comparison, not float rounding.
    for q in (1,2,3,7,19,3**40,SCALE-1,SCALE,SCALE+1,2**100):
        for threshold in (Fraction(q,SCALE),Fraction(2*q+1,2*SCALE)):
            lo,hi=0,0x7f7fffff
            while lo<hi:
                mid=(lo+hi+1)//2
                if rational(mid)<=threshold:lo=mid
                else:hi=mid-1
            for offset in (-1,0,1,2):both(lo+offset,'near_grid_or_half_grid_boundary')
    rng=random.Random(20261028)
    for _ in range(128):
        while True:
            bits=rng.getrandbits(32)
            if ((bits>>23)&255)!=255:break
        add(bits,'seeded_random_finite')
    return [dict(bits=f'{b:08x}',tags=sorted(set(tags))) for b,tags in sorted(cases.items())]


def main():
    binary=Path(sys.argv[1]).resolve()
    out=Path(sys.argv[2]).resolve();out.mkdir(parents=True,exist_ok=False)
    started=time.monotonic()
    def event(kind,**kw):
        with (out/'attempts.jsonl').open('a') as f:
            f.write(json.dumps(dict(hypothesis='H-CONV-01',purpose='standalone_exploratory',kind=kind,
                                   utc=datetime.now(timezone.utc).isoformat(),**kw))+'\n')
            f.flush();os.fsync(f.fileno())
    def launch(command,name,timeout):
        event('launch_start',name=name,command=[str(x) for x in command])
        with (out/(name+'-stdout.txt')).open('xb') as stdout,(out/(name+'-stderr.txt')).open('xb') as stderr:
            try:
                result=subprocess.run([str(x) for x in command],stdout=stdout,stderr=stderr,timeout=timeout)
            except BaseException as e:
                event('launch_failure',name=name,error=repr(e));raise
        event('launch_finish',name=name,returncode=result.returncode)
        if result.returncode:raise RuntimeError(name+' failed')
    entries=corpus()
    save(out/'corpus.json',dict(seed=20261028,cases=entries))
    (out/'corpus.hex').write_text(''.join(e['bits']+'\n' for e in entries))
    # Sanity anchors for the single independent oracle.
    anchors={0x00000000:0,0x80000000:0,0x3f800000:SCALE,0xbf800000:-SCALE,
             0x3f000000:SCALE//2,0xbf000000:-(SCALE//2),0x00000001:0,0x007fffff:1}
    for b,q in anchors.items():
        if int(convert(b)['raw_integer'])!=q:raise ValueError('Oracle anchor')
    oracle=[dict(input=e,expected=convert(int(e['bits'],16))) for e in entries]
    save(out/'oracle.json',oracle)
    order=['BC']*3+['CB']*3;random.Random(20261029).shuffle(order)
    (out/'schedule.txt').write_text('\n'.join(order)+'\n')
    sourcehash={str(f.relative_to(HERE)):hashlib.sha256(f.read_bytes()).hexdigest()
                for f in HERE.rglob('*') if f.is_file() and f.suffix in ('.py','.hpp','.cpp','.cweb')}
    save(out/'plan.json',dict(hypothesis='H-CONV-01',purpose='standalone_exploratory',cases=len(entries),
         order=order,warmup_full_corpus_per_candidate=1,paired_batches=6,max_active_seconds=3600,
         native_binary_sha256=hashlib.sha256(binary.read_bytes()).hexdigest(),sources_sha256=sourcehash,
         timing='steady_clock inside one native process; entire identical corpus, output checksum consumption included; I/O outside intervals'))
    probes=[]
    for command in (['/usr/bin/clang++','--version'],['uname','-a'],['sw_vers'],['pmset','-g','batt'],['pmset','-g','therm']):
        r=subprocess.run(command,capture_output=True,timeout=30)
        probes.append(dict(command=command,returncode=r.returncode,stdout=r.stdout.decode(errors='replace'),stderr=r.stderr.decode(errors='replace')))
    save(out/'environment.json',dict(python=sys.version,platform=platform.platform(),loadavg=os.getloadavg(),probes=probes))
    launch([binary,'check',out/'corpus.hex'],'correctness',900)
    actual=(out/'correctness-stdout.txt').read_text().splitlines()
    if len(actual)!=len(entries):raise ValueError('Incomplete correctness output')
    mismatches=[];counts=[]
    for row,expected in zip(actual,oracle):
        b,old,new=row.split()
        want=expected['expected']['packed'] or 'nonfinite'
        if b!=expected['input']['bits'] or old!=want or new!=want:
            mismatches.append(dict(input_bits=expected['input']['bits'],expected=expected['expected'],actual_B=old,actual_C=new,
                                   semantics='q=trunc_toward_zero(binary32_exact*3**80); reject nonfinite; collapse signed zero'))
        bits=int(b,16);exp=(bits>>23)&255;fraction=bits&0x7fffff
        active=exp!=255 and (exp!=0 or fraction!=0)
        k=max(0,150-exp) if exp else 149
        counts.append(dict(bits=b,B_wide_divisions=k if active else 0,C_wide_divisions=int(active and k>0),
                           C_denominator_doublings=k if active and k>0 else 0))
    save(out/'mismatches.json',mismatches);save(out/'operation-counts.json',counts)
    event('correctness_complete',cases=len(entries),mismatches=len(mismatches))
    if mismatches:
        event('stopped',reason='non-equivalent; no performance collection')
        return
    if time.monotonic()-started>1800:raise TimeoutError('Budget reserved for preservation/report')
    launch([binary,'bench',out/'corpus.hex',out/'schedule.txt'],'benchmark',1500)
    lines=(out/'benchmark-stdout.txt').read_text().splitlines()
    if not lines or not lines[0].startswith('warmup '):raise ValueError('Warmup not retained')
    rows=[]
    for line in lines[1:]:
        pair,method,ns,n,digest=line.split()
        rows.append(dict(pair=int(pair),method=method,elapsed_ns=int(ns),inputs=int(n),digest=digest))
    if [(r['pair'],r['method']) for r in rows]!=[(i,m) for i,o in enumerate(order) for m in o]:raise ValueError('Incomplete chronology')
    def summary(values):
        return dict(median=statistics.median(values),min=min(values),max=max(values),sd=statistics.stdev(values),
                    mad=statistics.median(abs(x-statistics.median(values)) for x in values))
    save(out/'performance.json',dict(rows=rows,summary={m:summary([r['elapsed_ns']/len(entries) for r in rows if r['method']==m]) for m in 'BC'},
         paired_C_over_B=[next(r['elapsed_ns'] for r in rows if r['pair']==i and r['method']=='C')/next(r['elapsed_ns'] for r in rows if r['pair']==i and r['method']=='B') for i in range(6)]))
    event('complete',elapsed_seconds=time.monotonic()-started,loadavg=os.getloadavg())


if __name__=='__main__':main()
