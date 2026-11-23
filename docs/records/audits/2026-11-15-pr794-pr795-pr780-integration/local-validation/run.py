import datetime,hashlib,json,os,platform,shutil,subprocess,sys
from pathlib import Path
source=Path('/private/tmp/t81-pr794-pr795-pr780-integration')
out=Path('/private/tmp/t81-combined-local-validation')
out.mkdir(parents=True,exist_ok=True)
build=Path('/private/tmp/t81-combined-release')
python='/private/tmp/t81-pr795-parity-venv/bin/python'
record={'source_sha':subprocess.check_output(['git','rev-parse','HEAD'],cwd=source,text=True).strip(),'platform':platform.platform(),'machine':platform.machine(),'scope':'Correctness/contract checks only; no benchmark or measurement execution.','commands':[]}
def run(label,argv,timeout=600):
 entry={'label':label,'argv':list(map(str,argv)),'cwd':str(source),'started_utc':datetime.datetime.now(datetime.timezone.utc).isoformat()}
 r=subprocess.run(entry['argv'],cwd=source,capture_output=True,text=True,timeout=timeout)
 entry.update(exit_code=r.returncode,stdout=r.stdout,stderr=r.stderr)
 for name,content in [('stdout',r.stdout),('stderr',r.stderr)]: (out/(label+'.'+name+'.log')).write_text(content)
 record['commands'].append(entry)
 (out/'summary.json').write_text(json.dumps(record,indent=2)+'\n')
 print(label,r.returncode,flush=True)
 if r.returncode:
  print(r.stdout[-3000:]+r.stderr[-3000:],flush=True);raise SystemExit(1)
 return r.stdout
run('compiler',['/usr/bin/c++','--version'])
run('python-environment',[python,'-c','import sys,importlib.metadata; print(sys.version); print("jsonschema="+importlib.metadata.version("jsonschema"))'])
run('python-packages',[python,'-m','pip','freeze','--all'])
run('configure',['cmake','-S','.', '-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','-DT81_BUILD_TESTS=ON','-DT81_BUILD_EXPERIMENTS=ON','-DT81_BUILD_TUI=OFF','-DT81_BUILD_EXAMPLES=OFF','-DT81_BUILD_BENCHMARKS=ON','-DT81_ENABLE_TERNARYOS=OFF','-DFETCHCONTENT_SOURCE_DIR_ASIO=/Users/t81dev/Code/t81-foundation/build/_deps/asio-src','-DFETCHCONTENT_SOURCE_DIR_GOOGLEBENCHMARK=/Users/t81dev/Code/t81-foundation/build/_deps/googlebenchmark-src','-DPython3_EXECUTABLE='+python])
run('build',['cmake','--build',build,'--target','t81','t81_process_capture_test','cli_repro_hash_test','t81_determinism_primitives_test','benchmark_runner','-j','4'])
script='scripts/ci/validate_scg08_gate1_evidence.py'
for flags,name in (([],'normal'),(['-O'],'optimized')):
 for option,label in (([],'positive'),(['--test-negative'],'negative'),(['--test-json-files'],'json-files')):
  output=run('validator-'+label+'-'+name,[python,*flags,script,*option])
  if label=='negative' and '[SCG-08 SCHEMA PARITY] 61 rejection cases and 2 positive records passed.' not in output: raise RuntimeError('Parity skipped')
run('wrapper-receipts',[python,'tests/python/test_skill_policy_boundary.py','--bash','/bin/bash','--t81-bin',build/'t81','--receipt',out/'wrapper-receipts.json'])
run('policy-receipts',[python,'tests/python/test_cli_execution_policy.py','--t81-bin',build/'t81','--receipt',out/'policy-receipts.json'])
pattern='^(t81_skill_policy_boundary_test|t81_cli_execution_policy_test|t81_cli_repro_hash_test|t81_process_capture_test|t81_determinism_primitives_test|t81_exp_scg08_01_schema_.*)$'
run('ctest-inventory',['ctest','--test-dir',build,'--show-only=json-v1','-R',pattern])
output=run('ctest',['ctest','--test-dir',build,'--output-on-failure','--no-tests=error','-V','--output-junit',out/'ctest-junit.xml','-R',pattern])
if output.count('[SCG-08 SCHEMA PARITY] 61 rejection cases and 2 positive records passed.')!=2: raise RuntimeError('CTest parity skipped')
run('bash-syntax',['/bin/bash','-n','skills/policy-checker/validate-policy.sh'])
run('workflow-actions',['python3','scripts/ci/audit_workflow_actions.py','--max-tagged','0','--max-unknown','0'])
run('workflow-permissions',['python3','scripts/ci/audit_workflow_permissions.py','--max-missing','0'])
for p in ['CMakeCache.txt','compile_commands.json','Testing/Temporary/LastTest.log']:
 dest=out/Path(p).name;shutil.copyfile(build/p,dest)
record['source_hashes']={p:hashlib.sha256((source/p).read_bytes()).hexdigest() for p in ['CMakeLists.txt','scripts/ci/validate_scg08_gate1_evidence.py','tests/python/test_skill_policy_boundary.py','tests/determinism/test_primitives.cpp','include/t81/types/cell.hpp']}
(out/'summary.json').write_text(json.dumps(record,indent=2)+'\n')
print('All local integration checks passed.',flush=True)
