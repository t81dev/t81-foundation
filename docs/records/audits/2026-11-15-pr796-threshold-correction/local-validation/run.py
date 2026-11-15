import datetime,hashlib,json,os,subprocess,sys,zipfile
from pathlib import Path
source=Path('/private/tmp/t81-pr794-pr795-pr780-integration')
out=Path('/private/tmp/t81-pr796-correction-validation')
record={'base_head':'b48e1a4da29784860a528e5397f896c73bc48450','scope':'Correction tests and archived input replay; no new benchmark measurement.','commands':[]}
def run(label,argv,env=None):
 r=subprocess.run(argv,cwd=source,env=env,capture_output=True,text=True,timeout=180)
 entry={'label':label,'argv':argv,'cwd':str(source),'recorded_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'exit_code':r.returncode,'stdout':r.stdout,'stderr':r.stderr}
 record['commands'].append(entry)
 (out/'results.json').write_text(json.dumps(record,indent=2)+'\n')
 (out/(label+'.stdout.log')).write_text(r.stdout);(out/(label+'.stderr.log')).write_text(r.stderr)
 print(label,r.returncode,flush=True)
 if r.returncode: print(r.stdout+r.stderr);raise SystemExit(1)
 return r.stdout
run('normal-tests',['python3','tests/python/test_simd_regression.py'])
run('optimized-tests',['python3','-O','tests/python/test_simd_regression.py'])
run('tabnanny',['python3','-m','tabnanny','scripts/ci/check_simd_regression.py','tests/python/test_simd_regression.py'])
run('workflow-actions',['python3','scripts/ci/audit_workflow_actions.py','--max-tagged','0','--max-unknown','0'])
run('workflow-permissions',['python3','scripts/ci/audit_workflow_permissions.py','--max-missing','0'])
run('diff-check',['git','diff','--check'])
# Enumeration invokes no timed benchmarks. These source bytes are unchanged
# from the old combined-head build; record its actual native NEON registrations.
text=run('native-compiled-registry',['/private/tmp/t81-combined-release/benchmarks/benchmark_runner','--benchmark_list_tests=true','--benchmark_filter=^BM_Kernel_TAnd_(SWAR|AVX2|NEON)/'],env={**os.environ,'T81_BENCHMARK_WRITE_REPORTS':'0'})
(out/'native-compiled-inventory.txt').write_text('\n'.join(line for line in text.splitlines() if line.startswith('BM_Kernel_TAnd_'))+'\n')
# Replay preserved old Linux SIMD results, explicitly not new measurements.
with zipfile.ZipFile('/private/tmp/t81-combined-hosted-qualification/simd-regression-artifacts.zip') as z:
 payload=z.read('bench.json')
(out/'old-simd-bench.json').write_bytes(payload)
names=[f'BM_Kernel_TAnd_{impl}/{size}' for impl in ('SWAR','AVX2') for size in (16,64,256,1024,4096,16384,65536)]
(out/'old-linux-expected-inventory.txt').write_text('\n'.join(names)+'\n')
run('old-artifact-replay',['python3','scripts/ci/check_simd_regression.py',str(out/'old-simd-bench.json'),'--inventory',str(out/'old-linux-expected-inventory.txt')])
record['source_hashes']={p:hashlib.sha256((source/p).read_bytes()).hexdigest() for p in ['scripts/ci/check_simd_regression.py','tests/python/test_simd_regression.py','.github/workflows/ci.yml']}
record['replay_scope']='Old b48e1a4da main-CI SIMD artifact only; supplied inventory represents its existing Linux AVX2 compile profile, not an independently regenerated old binary inventory.'
(out/'results.json').write_text(json.dumps(record,indent=2)+'\n')
