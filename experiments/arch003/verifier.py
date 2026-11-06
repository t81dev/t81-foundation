"""V: independent fixed-frame verifier, never imports candidate/oracle code.

CLI stream supervision is separate from semantic validation. Worker exit zero
never makes a wrong observation acceptable. All checks survive Python -O.
"""
import argparse
import hashlib
import os
from pathlib import Path
import struct
import subprocess
import sys
from evidence import (HEADER, EXPECTED, OBSERVED, RECORDS, CALLS, D0, TAGS,
                      fold, sha, load_json, write_json, validate_manifest, verify_index)


class Failure(ValueError):
    def __init__(self, status, message):
        self.status = status
        super().__init__(message)


def need(condition, message, status=67):
    if not condition:
        raise Failure(status, message)


def exact(stream, size):
    raw = stream.read(size)
    need(len(raw)==size, 'truncated frame')
    return raw


def header(stream, kind, profile, probe):
    magic,k,p,g,res,count = HEADER.unpack(exact(stream,16))
    need((magic,k,p,g,res)==(b'A2R1',kind,profile,probe,0),'header')
    return count


def expectations(stream, count, guard=False, full=True):
    families=[0]*12
    call_counts=[0]*12
    for seq in range(count):
        raw=exact(stream,88)
        e=EXPECTED.unpack(raw)
        case,fam,fa,fb,status,na,nb,ba,bb,truth,oa,ob,mode,mask,_=e
        need(case==(2**64-1 if guard else seq),'expected sequence')
        need(status<=6 and mask and not mask&~63 and oa<32 and ob<32 and mode<=1,'expected domains')
        need((status==0 and truth<=na and na==nb and na<=2**24) or (status!=0 and truth==0),'expected count/null')
        if guard:
            need(e[:-1]==(2**64-1,255,1,1,0,1,1,1,1,1,0,0,0,1),'guard anchor')
            pair=hashlib.sha256(b'ARCH002-R1-PAIR\x00'+struct.pack('<BQQ',1,1,1)+b'\0'+struct.pack('<BQQ',1,1,1)+b'\0').digest()
            need(e[-1]==pair,'guard pair hash')
        else:
            need(fam<12,'family')
            need(fam==next((i for i,n in enumerate(RECORDS) if seq<sum(RECORDS[:i+1])),None),'family progress')
            if fam!=10:
                need(fa==fb and fa<=4 or (fam==11 and fa==255 and fb<=4),'format domain')
                f=fb
                need(mask==sum(1<<i for i,t in enumerate(TAGS) if t==f),'registered mask')
            else:
                need(na==nb==ba==bb==0 and status==1,'format family')
                want=63 if fa!=fb or fa>=5 else 63^sum(1<<i for i,t in enumerate(TAGS) if t==fa)
                need(mask==want,'misroute mask')
            if fam!=5:
                need((oa,ob,mode)==(0,0,0),'allocation metadata')
            else:
                need(ob==(7*oa+3)%32,'paired offsets')
            families[fam]+=1
            call_counts[fam]+=mask.bit_count()
        yield e, raw
    need(not stream.read(1),'expected trailing bytes')
    if full and not guard:
        need(count==408034 and tuple(families)==RECORDS and tuple(call_counts)==CALLS,'expected completeness')


def verify(expected_path, observed_stream, guard=False, full=True, progress=None):
    if progress is None:progress={'digests':[D0]*6,'calls':0}
    digests=progress['digests']
    actual=0
    with open(expected_path,'rb') as expected:
        count=header(expected,0,255,255 if guard else 0)
        need(count==(1 if guard else 408034) if full else count<=408034,'expected header count')
        for e,_ in expectations(expected,count,guard,full):
            case,_,_,_,truth_status,na,_,_,_,truth,_,_,_,mask,_=e
            for impl in range(6):
                if not mask&(1<<impl):
                    continue
                row=OBSERVED.unpack(exact(observed_stream,32))
                seq,k,status,valid,r8,value,digest,r32=row
                need(seq==case and k==impl,'observation order/eligibility')
                need(status<=6 and valid==int(status==0) and r8==r32==0,'observation fields')
                need(status==0 or value==0,'error count must be null')
                digests[k]=fold(digests[k],status,value)
                need(digest==digests[k],'rolling digest')
                progress['calls']+=1
                need(status==truth_status and value==truth and (status!=0 or value<=na),'result mismatch',65)
                actual+=1
        need(not observed_stream.read(1),'unexpected observation/trailing bytes')
    return actual,[f'{d:016x}' for d in digests]


def supervise(args):
    """One W at a time. V reaps W and retains the OS peak receipt."""
    output=Path(args.output)
    count=0
    error=None
    progress={'digests':[D0]*6,'calls':0}
    digest=hashlib.sha256()
    env=os.environ.copy()
    env['ARCH003_AUTHORIZED_WORKER']='owner authorization checked'
    with open(args.diagnostic,'wb') as diagnostic, open(output,'w+b') as retained:
        retained.write(HEADER.pack(b'A2R1',0 if args.oracle else 1,255 if args.oracle else args.profile,args.probe,0,0))
        worker=subprocess.Popen(args.command,stdout=subprocess.PIPE,stderr=diagnostic,env=env)
        # Read exactly one frame at a time; retain valid-size prefixes even on failure.
        size=88 if args.oracle else 32
        class Tee:
            def read(self,n):
                nonlocal count
                raw=worker.stdout.read(n)
                if n==size and len(raw)==size:
                    retained.write(raw);count+=1;digest.update(raw)
                elif raw:
                    # Partial bytes are diagnostics, never a fabricated full frame.
                    diagnostic.write(b'\npartial/trailing frame: '+raw.hex().encode()+b'\n')
                return raw
        try:
            if args.oracle:
                if args.expected:
                    with open(args.expected,'rb') as seal:
                        n=header(seal,0,255,0)
                        for _,raw in expectations(seal,n):
                            actual=exact(Tee(),88)
                            if actual!=raw:
                                diagnostic.write(b'first divergent expected/actual: '+raw.hex().encode()+b' '+actual.hex().encode()+b'\n')
                                raise Failure(68,'oracle replay divergence')
                        need(not Tee().read(1),'oracle trailing bytes',68)
                else:
                    for _ in expectations(Tee(),408034):
                        pass
            else:
                verify(args.expected,Tee(),guard=args.probe!=0,progress=progress)
        except (ValueError,OSError) as exc:
            error=exc
            # Guards must still observe their natural required exits.
            if not args.probe:
                worker.kill()
        worker.stdout.close()
        pid,status,usage=os.wait4(worker.pid,0)
        worker.returncode=os.waitstatus_to_exitcode(status)
        retained.seek(0)
        retained.write(HEADER.pack(b'A2R1',0 if args.oracle else 1,255 if args.oracle else args.profile,args.probe,0,count))
        retained.flush()
    receipt={'worker_exit':worker.returncode,'worker_peak_rss_raw':usage.ru_maxrss,
             'peak_units':'bytes' if sys.platform=='darwin' else 'KiB',
             'records':count,'raw_stream_sha256':digest.hexdigest(),'valid_observations':progress['calls'],'final_digests':[f'{d:016x}' for d in progress['digests']],
             'status_id':getattr(error,'status',67) if error else (68 if args.oracle and worker.returncode else 67 if worker.returncode else 64),
             'message':str(error) if error else None}
    write_json(args.receipt,receipt)
    return 1 if error or worker.returncode else 0


def bundle(directory):
    root=Path(directory)
    names=verify_index(root)
    obj=load_json(root/'campaign.json')
    validate_manifest(obj,root,complete=True)
    # The manifest cannot stand in for retained per-call evidence.
    for i,p in enumerate(obj['observed_files']):
        with open(root/p,'rb') as stream:
            need(header(stream,1,i,0)==483553,'observed count')
            count,digests=verify(root/'expected.bin',stream)
            need(count==obj['actual_calls'][i] and digests==obj['final_digests'][i],'manifest progress')
    expected_hash=None
    for i,p in enumerate(obj['oracle_receipts']):
        r=load_json(root/p)
        need(set(r)=={'profile','kind','binary_sha256','raw_stream_sha256','records','exit_code','agreed'},'oracle receipt schema')
        need(r['profile']==i//2 and r['kind']==('scalar' if i%2==0 else 'histogram') and r['exit_code']==0 and r['records']==408034 and r['agreed'] is True,'oracle receipt')
        for field in ('binary_sha256','raw_stream_sha256'):
            need(isinstance(r[field],str) and len(r[field])==64 and all(c in '0123456789abcdef' for c in r[field]),'receipt hash')
        if expected_hash is None:
            with open(root/'expected.bin','rb') as stream:
                stream.read(16);h=hashlib.sha256()
                for chunk in iter(lambda:stream.read(65536),b''):h.update(chunk)
                expected_hash=h.hexdigest()
        need(r['raw_stream_sha256']==expected_hash,'oracle seal identity')
    for i,p in enumerate(obj['guard_receipts']):
        r=load_json(root/p)
        need(set(r)=={'profile','kind','worker_exit','verifier_exit','expected_count','observed_count','rejected','observed_file'},'guard receipt schema')
        need(r['profile']==i//2 and r['kind']==('G1' if i%2==0 else 'G2') and r['worker_exit']==(1 if i%2==0 else 0) and r['verifier_exit']==1 and r['expected_count']=='0000000000000001' and r['observed_count']=='0000000000000002' and r['rejected'] is True,'guard receipt')
        need(r['observed_file'] in names,'guard artifact')
        with open(root/r['observed_file'],'rb') as stream:
            need(header(stream,1,i//2,1+i%2)==1,'guard header')
            raw=exact(stream,32)
            need(not stream.read(1),'guard trailing')
            need(OBSERVED.unpack(raw)==(2**64-1,0,0,1,0,2,fold(D0,0,2),0),'guard wrong record')
    for i,p in enumerate(obj['sanitizer_probe_receipts']):
        r=load_json(root/p)
        need(set(r)=={'kind','binary_sha256','exit_code','diagnostic_file','detected'},'probe receipt schema')
        need(r['kind']==('asan_heap_end' if i==0 else 'ubsan_shift') and type(r['exit_code']) is int and r['exit_code']!=0 and r['detected'] is True,'probe receipt')
        need(r['diagnostic_file'] in names,'probe diagnostic')
        raw=(root/r['diagnostic_file']).read_bytes()
        need(b'AddressSanitizer: heap-buffer-overflow' in raw if i==0 else b'runtime error:' in raw and b'shift exponent' in raw,'probe diagnostic marker')
    return True


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--bundle')
    parser.add_argument('--output');parser.add_argument('--expected');parser.add_argument('--receipt');parser.add_argument('--diagnostic')
    parser.add_argument('--profile',type=int,choices=range(3),default=0)
    parser.add_argument('--probe',type=int,choices=(0,1,2),default=0)
    parser.add_argument('--oracle',action='store_true')
    parser.add_argument('command',nargs=argparse.REMAINDER)
    args=parser.parse_args()
    try:
        if args.bundle:
            bundle(args.bundle);return 0
        need(os.environ.get('ARCH003_AUTHORIZED_VERIFIER') and args.output and args.receipt and args.diagnostic and args.command,'BLOCKED_AUTHORIZATION: owner required')
        if args.command[0]=='--':args.command=args.command[1:]
        return supervise(args)
    except (ValueError,OSError) as e:
        print(str(e),file=sys.stderr);return 1


if __name__=='__main__':
    sys.exit(main())
