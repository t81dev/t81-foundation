"""Native Windows Debug execution of ISA IO independently of the blocked VM build."""
import hashlib,json,pathlib,platform,shutil,subprocess,sys
source=pathlib.Path('source').resolve(); out=pathlib.Path('evidence').resolve();out.mkdir()
report={'source_commit':subprocess.check_output(['git','rev-parse','HEAD'],cwd=source,text=True).strip(),'platform':platform.platform(),'python':sys.version,'commands':[],'performance_samples':[]}
assert platform.system()=='Windows' and report['source_commit']=='35f73ac9751423f9ac083c7f4b84def34d810443'
def run(label,argv):
 argv=list(map(str,argv));r=subprocess.run(argv,cwd=source,stdout=subprocess.PIPE,stderr=subprocess.PIPE,timeout=1200)
 rec={'label':label,'argv':argv,'cwd':str(source),'returncode':r.returncode};report['commands'].append(rec)
 (out/(label+'.stdout')).write_bytes(r.stdout);(out/(label+'.stderr')).write_bytes(r.stderr)
 (out/(label+'.command.json')).write_text(json.dumps(rec,indent=2)+'\n')
 print(label,r.returncode,flush=True)
 return r.returncode
try:
 assert run('compiler',['cl'])==0
 run('cmake-version',['cmake','--version']);run('ninja-version',['ninja','--version'])
 assert run('configure',['cmake','-S',source,'-B',out/'build','-G','Ninja','-DCMAKE_BUILD_TYPE=Debug','-DCMAKE_C_COMPILER=cl','-DCMAKE_CXX_COMPILER=cl','-DCMAKE_EXPORT_COMPILE_COMMANDS=ON','-DT81_BUILD_BENCHMARKS=OFF','-DT81_BUILD_TUI=OFF','-DT81_BUILD_EXAMPLES=OFF','-DT81_BUILD_FUZZ_TESTS=OFF','-DT81_ENABLE_TERNARYOS=OFF','-DT81_ENABLE_DPE=OFF','-DT81_ENABLE_LLAMA_CPP=OFF'])==0
 assert run('build-isa',['cmake','--build',out/'build','--verbose','--parallel','2','--target','t81_isa_binary_io_determinism_test'])==0
 assert run('test-isa',[out/'build/t81_isa_binary_io_determinism_test.exe'])==0
 report['status']='PASS'
except Exception as e:
 report['status']='FAIL_OR_BLOCKED';report['error']=str(e)
finally:
 for name in ['CMakeCache.txt','compile_commands.json']:
  p=out/'build'/name
  if p.exists():shutil.copyfile(p,out/name)
 report['tracked_source_status_after']=subprocess.check_output(['git','status','--porcelain','--untracked-files=no'],cwd=source,text=True).strip()
 (out/'result.json').write_text(json.dumps(report,indent=2)+'\n')
 index={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in out.iterdir() if p.is_file()}
 (out/'SHA256SUMS.json').write_text(json.dumps(index,indent=2)+'\n')
sys.exit(0 if report['status']=='PASS' else 1)
