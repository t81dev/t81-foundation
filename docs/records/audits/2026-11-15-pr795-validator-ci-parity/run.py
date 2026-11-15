import datetime,hashlib,json,os,platform,subprocess,sys
from pathlib import Path
source=Path('/private/tmp/t81-pr767-main-validator')
evidence=Path('/private/tmp/t81-pr795-validator-verification')
python='/private/tmp/t81-pr795-parity-venv/bin/python'
script='scripts/ci/validate_scg08_gate1_evidence.py'
record={'source_sha':subprocess.check_output(['git','rev-parse','HEAD'],cwd=source,text=True).strip(),'platform':platform.platform(),'machine':platform.machine(),'commands':[],'kind':'Local direct Python and CTest execution; not hosted CI.'}
def run(label,argv,expected=0):
    entry={'label':label,'argv':argv,'cwd':str(source),'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'expected_exit':expected}
    r=subprocess.run(argv,cwd=source,text=True,capture_output=True,timeout=300)
    entry.update(exit_code=r.returncode,stdout=r.stdout,stderr=r.stderr)
    (evidence/(label+'.stdout.log')).write_text(r.stdout)
    (evidence/(label+'.stderr.log')).write_text(r.stderr)
    record['commands'].append(entry)
    (evidence/'local-results.json').write_text(json.dumps(record,indent=2)+'\n')
    print(label,r.returncode,flush=True)
    if r.returncode!=expected:
        print(r.stdout[-4000:]+r.stderr[-4000:],flush=True)
        raise SystemExit(1)
    return r.stdout
run('environment',[python,'-c','import importlib.metadata,sys; print(sys.version); print(sys.executable); print("jsonschema="+importlib.metadata.version("jsonschema")); print("optimization="+str(sys.flags.optimize))'])
run('pip-freeze',[python,'-m','pip','freeze','--all'])
for flags,name in (([],'normal'),(['-O'],'optimized')):
    for option,label in (([],'positive'),(['--test-negative'],'negative'),(['--test-json-files'],'json-files')):
        output=run('direct-'+label+'-'+name,[python,*flags,script,*option])
        if label=='negative' and '[SCG-08 SCHEMA PARITY] 61 rejection cases and 2 positive records passed.' not in output:
            raise RuntimeError('Parity checks did not execute: '+output)
run('cmake-configure',['cmake','-S','.', '-B','/private/tmp/t81-pr795-parity-release','-G','Ninja','-DCMAKE_BUILD_TYPE=Release','-DT81_BUILD_TESTS=ON','-DT81_BUILD_EXPERIMENTS=ON','-DT81_BUILD_TUI=OFF','-DT81_BUILD_EXAMPLES=OFF','-DT81_BUILD_BENCHMARKS=OFF','-DT81_ENABLE_TERNARYOS=OFF','-DFETCHCONTENT_SOURCE_DIR_ASIO=/Users/t81dev/Code/t81-foundation/build/_deps/asio-src','-DPython3_EXECUTABLE='+python])
run('ctest-inventory',['ctest','--test-dir','/private/tmp/t81-pr795-parity-release','--show-only=json-v1','-R','^t81_exp_scg08_01_schema_'])
output=run('ctest-registered',['ctest','--test-dir','/private/tmp/t81-pr795-parity-release','--output-on-failure','--no-tests=error','-V','--output-junit',str(evidence/'ctest-junit.xml'),'-R','^t81_exp_scg08_01_schema_'])
if output.count('[SCG-08 SCHEMA PARITY] 61 rejection cases and 2 positive records passed.')!=2:
    raise RuntimeError('CTest parity did not run twice.')
record['source_hashes']={str(p):hashlib.sha256((source/p).read_bytes()).hexdigest() for p in map(Path,[script,'experiments/scg08_arithmetic_work/CMakeLists.txt','experiments/scg08_arithmetic_work/evidence_schema.json','experiments/scg08_arithmetic_work/gate1_evidence_record.json'])}
run('source-clean-after',['git','status','--porcelain'])
(evidence/'local-results.json').write_text(json.dumps(record,indent=2)+'\n')
print('All direct and registered CTest checks passed with parity.',flush=True)
