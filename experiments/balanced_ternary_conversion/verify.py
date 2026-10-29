"""Offline packet verification: no native execution, project runtime or network."""
import hashlib
import json
from pathlib import Path
import statistics
import tarfile
from oracle import convert

HERE=Path(__file__).resolve().parent


def require(ok,why):
    if not ok:raise ValueError(why)


def main():
    manifest=json.loads((HERE/'manifest.json').read_text())
    for name,expected in manifest['files_sha256'].items():
        require(hashlib.sha256((HERE/name).read_bytes()).hexdigest()==expected,'Packet hash '+name)
    # Recreate the extraction, independently of arithmetic output agreement.
    original=(HERE/'source/T81Int.hpp').read_text()
    expected=original.replace('#include "t81/axion/api.hpp"\n','').replace('#include "t81/determinism/canon_hash81.hpp"\n','')
    expected=expected[:expected.index('\nnamespace std {')].replace('axion::trap_overflow(','throw std::overflow_error(')
    require((HERE/'ternary_int_extracted.hpp').read_text()=='// Research extraction: no runtime/governance/hash dependency. See SPEC.md.\n'+expected,'Integer extraction')
    expected=(HERE/'source/dmath_types.hpp').read_text().replace('#include "t81/types/T81Int.hpp"','#include "ternary_int_extracted.hpp"')
    require((HERE/'fixed_extracted.hpp').read_text()=='// Research extraction: include-path change only.\n'+expected,'Fixed extraction')
    hanoi=(HERE/'hanoi-source.cweb').read_bytes()[:-1]
    require(hashlib.sha1(b'blob '+str(len(hanoi)).encode()+b'\0'+hanoi).hexdigest()=='7132efaed9346526d1b4ad3bc1c593e0a71ee0d3','Hanoi source identity after recorded trailing-LF normalization')
    with tarfile.open(HERE/'raw-evidence.tar.gz') as t:
        def raw(name):return t.extractfile(name).read()
        require({m.name for m in t.getmembers() if m.isfile()}==set(manifest['archive_members_sha256']),'Archive inventory')
        for name,expected in manifest['archive_members_sha256'].items():
            require(hashlib.sha256(raw(name)).hexdigest()==expected,'Archive hash '+name)
        corpus=json.loads(raw('data/corpus.json'))['cases']
        plan=json.loads(raw('data/plan.json'))
        for name,expected in plan['sources_sha256'].items():
            require(hashlib.sha256((HERE/name).read_bytes()).hexdigest()==expected,'Measured source identity '+name)
        oracle=json.loads(raw('data/oracle.json'))
        native=raw('data/correctness-stdout.txt').decode().splitlines()
        require(len(native)==len(corpus)==len(oracle),'Case count')
        errors=[]
        for case,stored,line in zip(corpus,oracle,native):
            expected=convert(int(case['bits'],16))
            require(expected==stored['expected'],'Exact oracle regeneration')
            bits,b,c=line.split()
            want=expected['packed'] or 'nonfinite'
            if bits!=case['bits'] or b!=want or c!=want:errors.append(case['bits'])
        require(not errors and json.loads(raw('data/mismatches.json'))==[],'No equivalent-performance claim with mismatches')
        counts=json.loads(raw('data/operation-counts.json'))
        for case,row in zip(corpus,counts):
            bits=int(case['bits'],16);e=(bits>>23)&255;f=bits&0x7fffff
            active=e!=255 and (e!=0 or f!=0);k=max(0,150-e) if e else 149
            require(row==dict(bits=case['bits'],B_wide_divisions=k if active else 0,
                             C_wide_divisions=int(active and k>0),C_denominator_doublings=k if active and k>0 else 0),'Operation counts')
        performance=json.loads((HERE/'performance-summary.json').read_text())
        rows=performance['rows'];order=raw('data/schedule.txt').decode().splitlines()
        planned=[(i,m) for i,o in enumerate(order) for m in o]
        require([(r['pair'],r['method']) for r in rows]==planned[:len(rows)],'Retained timing chronology must be a prefix, never filtered')
        native_rows=[]
        lines=raw('data/benchmark-stdout.txt').decode().splitlines()
        require(lines[0].startswith('warmup '),'Warmup retention')
        for line in lines[1:]:
            i,m,ns,n,digest=line.split()
            native_rows.append(dict(pair=int(i),method=m,elapsed_ns=int(ns),inputs=int(n),digest=digest))
        require(rows==native_rows,'All completed native rows retained')
        require(len({r['digest'] for r in rows})==1,'Output checksum consistency')
        require(performance['planned_batches']==len(planned) and performance['complete']==(len(rows)==len(planned)),'Completion must not be fabricated')
        if not performance['complete']:
            events=[json.loads(line) for line in raw('data/attempts.jsonl').splitlines()]
            require(any(e['kind']=='launch_failure' and e['name']=='benchmark' and 'TimeoutExpired' in e['error'] for e in events),'Incomplete timing must retain timeout evidence')
        for method in 'BC':
            values=[r['elapsed_ns']/len(corpus) for r in rows if r['method']==method]
            require(all(v>0 for v in values),'Clock positivity')
            require(statistics.median(values)==performance['all_completed_summary'][method]['median'],'Latency median')
        print(json.dumps(dict(hypothesis='H-CONV-01',oracle_cases=len(corpus),mismatches=0,
                              planned_native_batches=len(planned),native_timed_batches=len(rows),
                              complete_native_plan=performance['complete'],summary_ns_per_input=performance['all_completed_summary'],
                              wide_divisions_B=sum(x['B_wide_divisions'] for x in counts),
                              wide_divisions_C=sum(x['C_wide_divisions'] for x in counts)),indent=2))


if __name__=='__main__':main()
