"""L: separately authorized, exact-commit correctness campaign owner.

No default execution and no performance mode. Development component tests do
not enter this owner. Authorization is a human decision, never inferred from a
hash or from the presence of this implementation.
"""
import argparse
import hashlib
import os
from pathlib import Path
import shutil
import socket
import re
import sys
import tempfile
import time
from accounting import Accountant, AccountingUnavailable, ResourceExceeded
from finalization import finalize, retain_interruption, preserve_failure_status
from evidence import (HEADER,EXPECTED,OBSERVED,D0,STATUS,sha,load_json,write_json,
                      manifest_template,seal,fold)

PROTOCOL='1454bd63426d21686cfb5ef22dbaebc3c9cae94b'
COMMON=['-std=c++20','-Wall','-Wextra','-Werror','-fno-lto']
PROFILES=[['-O0','-g','-UNDEBUG'],['-O3','-DNDEBUG'],['-O1','-g','-DNDEBUG','-fno-omit-frame-pointer','-fsanitize=address,undefined','-fno-sanitize-recover=all']]
SOURCE=Path(__file__).resolve().parent
SUPPORT=['schedule','sha256','wire']
CANDIDATES=['adapters','admission','count_tc2','count_byte','count_planes','count_bc2','count_pt5','count_native']


class Stop(RuntimeError):
    def __init__(self,status,message):
        self.status=status
        super().__init__(message)


def require(condition,status,message):
    if not condition:raise Stop(status,message)


def authorize(path):
    auth=load_json(path)
    required={'protocol_revision','implementation_revision','host','compiler','compiler_sha256',
              'output','human_authorization','oracle_review','verifier_review','accounting_review'}
    require(set(auth)==required,72,'authorization fields')
    require(auth['protocol_revision']==PROTOCOL and auth['host']==socket.gethostname(),72,'authorization protocol/host mismatch')
    for field in ('human_authorization','oracle_review','verifier_review','accounting_review'):
        require(isinstance(auth[field],str) and bool(auth[field].strip()),72,'missing human authorization/review reference')
    require(isinstance(auth['implementation_revision'],str) and re.fullmatch('[0-9a-f]{40}',auth['implementation_revision']),72,'implementation revision syntax')
    compiler=Path(auth['compiler'])
    require(compiler.is_absolute() and compiler.is_file() and sha(compiler)==auth['compiler_sha256'],72,'compiler identity mismatch')
    return auth


def main():
    started=time.monotonic_ns()
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--authorization',required=True,help='canonical exact-commit human authorization JSON')
    args=parser.parse_args()
    # Fail before launching any subprocess if authorization is absent/malformed.
    try:auth=authorize(args.authorization)
    except (OSError,ValueError,Stop) as exc:
        print('BLOCKED_AUTHORIZATION: '+str(exc),file=sys.stderr);return 2
    root=Path(auth['output']).resolve()
    if root.exists():
        print('BLOCKED_AUTHORIZATION: output must be a new directory; no overwrite/retry',file=sys.stderr);return 2
    root.mkdir(parents=True)
    # Owner lifetime includes all metadata, builds and sealing; never restart clock.
    protocol_bytes=(SOURCE.parents[1]/'docs/research/arch002/PROTOCOL.md').read_bytes()
    proposal_bytes=(SOURCE.parents[1]/'docs/research/arch002/proposal.json').read_bytes()
    manifest=manifest_template(hashlib.sha256(protocol_bytes).hexdigest(),hashlib.sha256(proposal_bytes).hexdigest(),auth['implementation_revision'])
    manifest['authorization_receipt']='authorization.json'
    shutil.copyfile(args.authorization,root/'authorization.json')
    acc=None
    counter=0
    work=None
    sealed_expected_sha=None
    env=os.environ.copy()
    env.update(ASAN_OPTIONS='halt_on_error=1:detect_leaks=0:symbolize=0',UBSAN_OPTIONS='halt_on_error=1:print_stacktrace=0:symbolize=0',PYTHONDONTWRITEBYTECODE='1',ARCH003_AUTHORIZED_VERIFIER='exact commit checked by owner')
    try:
        acc=Accountant(root,started_ns=started)
        def run(command,label,compiler=False,role=None):
            nonlocal counter
            counter+=1
            prefix=f'{counter:04d}-{label}'
            receipt=acc.run([str(c) for c in command],root/(prefix+'.stdout'),root/(prefix+'.stderr'),env=env,compiler=compiler,role=role)
            write_json(root/(prefix+'.json'),receipt)
            return receipt,prefix
        def metadata(command,label):
            r,p=run(command,label)
            require(r['exit_code']==0,69,'metadata command failed: '+label)
            return (root/(p+'.stdout')).read_text().strip()
        revision=metadata(['git','-C',SOURCE,'rev-parse','HEAD'],'source-revision')
        require(revision==auth['implementation_revision'],72,'implementation commit mismatch')
        dirty=metadata(['git','-C',SOURCE,'status','--porcelain','--untracked-files=normal'],'source-cleanliness')
        require(not dirty,72,'source checkout must be clean')
        pinned=metadata(['git','-C',SOURCE,'show',PROTOCOL+':docs/research/arch002/PROTOCOL.md'],'pinned-protocol')
        require(pinned==protocol_bytes.decode().strip(),72,'accepted protocol bytes changed')
        version=metadata([auth['compiler'],'--version'],'compiler-version')
        require('clang' in version.lower(),69,'Clang-family compiler required')
        write_json(root/'host.json',{'host':socket.gethostname(),'platform':sys.platform,'machine':os.uname().machine,
                                     'python':sys.version,'compiler_version':version,'environment':{k:env[k] for k in ('ASAN_OPTIONS','UBSAN_OPTIONS')},
                                     'accounting':'Linux ptrace birth/exec/exit stops, per-process VmHWM and wait4 receipts',
                                     'sanitizer_runtime_defaults':'toolchain defaults; UAR/scope submodes not qualified'})
        manifest['host_receipt']='host.json'
        # Direct compiler-driver invocations preserve L + driver + at most one tool.
        # CMake is the development build interface, not an extra live supervisor.
        work=Path(tempfile.mkdtemp(prefix='.arch003-build-',dir=root))
        binaries={}
        for profile,name in enumerate(('D','R','S')):
            directory=work/name;directory.mkdir()
            flags=COMMON+PROFILES[profile]+['-I',str(SOURCE/'include')]
            sources=SUPPORT+CANDIDATES+['sink','worker','oracle_main','oracle_scalar','oracle_histogram']
            if profile==2:sources+=['sanitizer_probe']
            commands=[]
            for unit in sources:
                source=SOURCE/('tests' if unit=='sanitizer_probe' else 'src')/(unit+'.cpp')
                command=[auth['compiler'],*flags,'-c',source,'-o',directory/(unit+'.o')]
                r,_=run(command,'compile-'+name+'-'+unit,True)
                require(r['exit_code']==0,69,'compilation failed')
                commands.append([str(c) for c in command])
            groups={'scalar_oracle':SUPPORT+['oracle_main','oracle_scalar'],
                    'histogram_oracle':SUPPORT+['oracle_main','oracle_histogram'],
                    'worker':SUPPORT+CANDIDATES+['sink','worker']}
            if profile==2:groups['sanitizer_probe']=['sanitizer_probe']
            for binary,units in groups.items():
                target=directory/('arch003_'+binary)
                command=[auth['compiler'],*flags,*[directory/(u+'.o') for u in units],'-o',target]
                r,_=run(command,'link-'+name+'-'+binary,True)
                require(r['exit_code']==0,69,'link failed')
                commands.append([str(c) for c in command])
                binaries[(profile,binary)]=target
            for obj in directory.glob('*.o'):obj.unlink()
            receipt=f'build-{name}.json'
            write_json(root/receipt,{'profile':profile,'commands':commands,'source_sha256':{str(p.relative_to(SOURCE)):sha(p) for p in sorted(SOURCE.rglob('*')) if p.suffix in ('.cpp','.hpp','.py')},'flags':flags})
            manifest['build_receipts'][profile]=receipt
        write_json(root/'binaries.json',{f'{p}-{kind}':sha(path) for (p,kind),path in binaries.items()})
        manifest['binary_receipt']='binaries.json'
        # Required S probes are the first post-build worker activity.
        for i,(kind,mode,markers) in enumerate((('asan_heap_end','heap',[b'AddressSanitizer: heap-buffer-overflow']),('ubsan_shift','shift',[b'runtime error:',b'shift exponent']))):
            r,p=run([binaries[(2,'sanitizer_probe')],mode],'probe-'+mode,role='worker')
            detected=r['exit_code']!=0 and all(m in (root/(p+'.stderr')).read_bytes() for m in markers)
            receipt='probe-'+mode+'.json'
            write_json(root/receipt,dict(kind=kind,binary_sha256=sha(binaries[(2,'sanitizer_probe')]),exit_code=r['exit_code'],diagnostic_file=p+'.stderr',detected=detected))
            manifest['sanitizer_probe_receipts'][i]=receipt
            require(detected,69,'required sanitizer diagnostic absent')
        # Guard expectation, independently spelled out rather than candidate-produced.
        pair=hashlib.sha256(b'ARCH002-R1-PAIR\0'+__import__('struct').pack('<BQQ',1,1,1)+b'\0'+__import__('struct').pack('<BQQ',1,1,1)+b'\0').digest()
        (root/'guard-expected.bin').write_bytes(HEADER.pack(b'A2R1',0,255,255,0,1)+EXPECTED.pack(2**64-1,255,1,1,0,1,1,1,1,1,0,0,0,1,pair))
        manifest['guard_expected_file']='guard-expected.bin'
        def supervised(profile,kind,output,expected=None,probe=0):
            if sealed_expected_sha is not None:
                require(sha(root/'expected.bin')==sealed_expected_sha,67,'sealed expected file replacement')
            label=f'{profile}-{kind}-{probe}';receipt=label+'-supervisor.json'
            command=[sys.executable,'-B',SOURCE/'verifier.py','--profile',str(profile),'--probe',str(probe),
                     '--output',output,'--receipt',root/receipt,'--diagnostic',root/(label+'-worker.stderr')]
            oracle=kind!='worker'
            if oracle:command+=['--oracle']
            if expected:command+=['--expected',expected]
            command+=['--',binaries[(profile,kind)],'--campaign' if not probe else '--G'+str(probe)]
            if not oracle:command+=[expected]
            run_receipt,_=run(command,'supervisor-'+label,role='runtime')
            require((root/receipt).exists(),67,'missing verifier receipt')
            record=load_json(root/receipt)
            return run_receipt['exit_code'],record
        for profile,name in enumerate(('D','R','S')):
            for k,kind in enumerate(('scalar_oracle','histogram_oracle')):
                first=profile==0 and k==0
                code,r=supervised(profile,kind,root/'expected.bin' if first else os.devnull,None if first else root/'expected.bin')
                if first:
                    manifest['expected_file']='expected.bin';manifest['actual_expected_records']=r['records']
                receipt=f'oracle-{profile}-{k}.json'
                write_json(root/receipt,dict(profile=profile,kind='scalar' if k==0 else 'histogram',binary_sha256=sha(binaries[(profile,kind)]),raw_stream_sha256=r['raw_stream_sha256'],records=r['records'],exit_code=r['worker_exit'],agreed=code==0))
                if r['records']==408034:
                    manifest['oracle_receipts'][profile*2+k]=receipt
                require(code==0,68,'oracle stream disagreement or failure')
            if profile==0:
                sealed_expected_sha=sha(root/'expected.bin')
                write_json(root/'expected-seal.json',{'file_sha256':sealed_expected_sha,'raw_stream_sha256':r['raw_stream_sha256']})
                manifest['expectations_sealed']=True
            for probe in (1,2):
                output=f'guard-{name}-G{probe}.bin'
                code,r=supervised(profile,'worker',root/output,root/'guard-expected.bin',probe)
                wrong=OBSERVED.pack(2**64-1,0,0,1,0,2,fold(D0,0,2),0)
                correct_output=(root/output).read_bytes()==HEADER.pack(b'A2R1',1,profile,probe,0,1)+wrong
                rejected=code==1 and r['status_id']==65 and r['worker_exit']==(1 if probe==1 else 0) and correct_output
                receipt=f'guard-{name}-G{probe}.json'
                write_json(root/receipt,dict(profile=profile,kind='G'+str(probe),worker_exit=r['worker_exit'],verifier_exit=code,expected_count='0000000000000001',observed_count='0000000000000002',rejected=rejected,observed_file=output))
                manifest['guard_receipts'][profile*2+probe-1]=receipt
                require(rejected,66,'FAIL_GUARD: wrong result/exits not observed')
            output=f'observed-{name}.bin'
            code,r=supervised(profile,'worker',root/output,root/'expected.bin')
            manifest['observed_files'][profile]=output;manifest['actual_calls'][profile]=r['valid_observations'];manifest['final_digests'][profile]=r['final_digests']
            require(code==0,r['status_id'] if r['status_id'] in (65,67) else 67,'normal candidate/verifier failure')
        require(sha(root/'expected.bin')==sealed_expected_sha,67,'sealed expected file replacement')
        # All binaries have frozen receipts. Do not retain duplicate build products.
        shutil.rmtree(work);work=None
        manifest.update(status_id=64,status=STATUS[64],complete=True)
    except (Stop,AccountingUnavailable,ResourceExceeded,OSError,ValueError) as exc:
        status=exc.status if isinstance(exc,Stop) else 70 if isinstance(exc,AccountingUnavailable) else 71 if isinstance(exc,ResourceExceeded) else 67
        status=preserve_failure_status(root,status)
        # Preserve a previously confirmed mismatch over a later accounting failure.
        if manifest['status_id'] not in (65,66,67,68):manifest['status_id']=status
        manifest['status']=STATUS[manifest['status_id']];manifest['complete']=False
        write_json(root/'failure.json',{'status_id':manifest['status_id'],'message':str(exc)})
        manifest['failure_receipt']='failure.json'
    finally:
        if work is not None:shutil.rmtree(work)
    try:
        if acc is None:
            raise AccountingUnavailable('collector unavailable')
        if manifest['complete']:
            (root/'final-verifier.stdout').touch()
            (root/'final-verifier.stderr').touch()
        finalize(acc,root,manifest)
        if manifest['complete']:
            command=[sys.executable,'-B',SOURCE/'verifier.py','--bundle',root]
            r=acc.run([str(c) for c in command],root/'final-verifier.stdout',root/'final-verifier.stderr',env=env,role='verifier')
            require(r['exit_code']==0,67,'final bundle verification failed')
            finalize(acc,root,manifest)
    except (OSError,ValueError,Stop,ResourceExceeded,AccountingUnavailable) as exc:
        retain_interruption(acc,root,manifest,exc)
    print(manifest['status'],file=sys.stderr)
    return 0 if manifest['complete'] else 1 if manifest['status_id'] in (65,66,67,68) else 3 if manifest['status_id']==71 else 2


if __name__=='__main__':sys.exit(main())
