#!/usr/bin/env python3
"""Build sealed EFI case images; capture three fresh hosted processes/guests each.

No oracle generation here. All failures, serial bytes and timeout flags retained.
"""
import concurrent.futures,gzip,hashlib,json,os,select,shutil,subprocess,time
from pathlib import Path
from jsonschema import Draft202012Validator
from observe import parse,compare
ROOT=Path(__file__).resolve().parent;SOURCE=ROOT.parents[1]
BUILD=Path('/private/tmp/cpa006-build');OUT=ROOT/'evidence/suite'
if OUT.exists() and any(OUT.iterdir()):
 raise FileExistsError('Preserve existing CPA006 evidence; choose a fresh output path before a live rerun')
OUT.mkdir(parents=True,exist_ok=True)
IMAGES=Path('/private/tmp/cpa006-images');IMAGES.mkdir(exist_ok=True)
QEMU='/opt/homebrew/bin/qemu-system-aarch64';CODE=Path('/opt/homebrew/share/qemu/edk2-aarch64-code.fd');VARS=Path('/opt/homebrew/share/qemu/edk2-arm-vars.fd')
SUITE=json.loads((ROOT/'frozen/suite.json').read_text())['cases']
SCHEMA=json.loads((ROOT/'frozen/observation.schema.json').read_text())
commands=[]
def sha(data):return hashlib.sha256(data).hexdigest()
def run(cmd,name,expected=0):
 p=subprocess.run(list(map(str,cmd)),cwd=SOURCE,capture_output=True)
 (OUT/(name+'.stdout')).write_bytes(p.stdout);(OUT/(name+'.stderr')).write_bytes(p.stderr)
 commands.append(dict(command=list(map(str,cmd)),cwd=str(SOURCE),exit_code=p.returncode,expected=expected,stdout=name+'.stdout',stderr=name+'.stderr'))
 (OUT/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
 if p.returncode!=expected:raise RuntimeError(name)
 return p.stdout
# Existing baseline binaries are not used by CPA006.
cc='/opt/homebrew/opt/llvm/bin/clang++'
run([cc,'-std=c++23','-O3','-DNDEBUG','-DT81_DETERMINISTIC','-I'+str(SOURCE/'include'),ROOT/'hosted_observer.cpp','-L'+str(BUILD),'-L/opt/homebrew/opt/llvm/lib/c++','-Wl,-rpath,/opt/homebrew/opt/llvm/lib/c++','-lt81_vm','-lt81_isa','-lt81_axion','-lt81_ffi','-lt81_experimental','-lt81_core','-o',BUILD/'cpa006_hosted'],'hosted-build')
results=[]
for case in SUITE:
 id=case['id'];blob=(ROOT/f'frozen/{id}.raw13').read_bytes();expected=json.loads((ROOT/f'frozen/{id}.expected.json').read_text())
 directory=OUT/id;directory.mkdir(exist_ok=True)
 for run_number in range(1,4):
  cmd=[str(BUILD/'cpa006_hosted'),str(ROOT/f'frozen/{id}.raw13'),str(case['limit'])]
  env=dict(os.environ);env.pop('T81_CANONFS_ROOT',None)
  start=time.monotonic();timeout=False
  try:
   p=subprocess.run(cmd,env=env,capture_output=True,timeout=30);raw=p.stdout;stderr=p.stderr;code=p.returncode
  except subprocess.TimeoutExpired as exc:
   raw=exc.stdout or b'';stderr=exc.stderr or b'';code=None;timeout=True
  (directory/f'hosted-{run_number}.raw.gz').write_bytes(gzip.compress(raw,mtime=0));(directory/f'hosted-{run_number}.stderr').write_bytes(stderr)
  record=dict(id=id,profile='hosted',run=run_number,command=cmd,exit_code=code,timeout=timeout,raw_sha256=sha(raw),disposition='INCONCLUSIVE' if timeout else 'FAIL')
  if not timeout and code==0:
   try:
    actual=parse(raw,blob,hosted=True);compare(actual,expected)
    for frame in actual['observations']:Draft202012Validator(SCHEMA).validate(frame)
    (directory/f'hosted-{run_number}.normalized.json').write_text(json.dumps(actual,indent=2)+'\n')
    record.update(disposition='PASS',terminal=actual['terminal'],observations=len(actual['observations']))
   except Exception as exc:record['error']=str(exc)
  (directory/f'hosted-{run_number}.result.json').write_text(json.dumps(record,indent=2)+'\n');results.append(record)
 print(id,'hosted',','.join(r['disposition'] for r in results[-3:]),flush=True)
 # Inputs are compile-selected, not answer-selected; preserve every exact build.
 run(['cmake','-S',SOURCE,'-B',BUILD,'-DT81_CPA006_CASE='+id],id+'-configure')
 run(['cmake','--build',BUILD,'--target','t81_ternaryos_qemu_slice6_efi','-j','4'],id+'-build')
 shutil.copyfile(BUILD/'ternaryos/qemu_slice6/BOOTAA64.EFI',IMAGES/(id+'.efi'))
 (directory/'efi.sha256').write_text(sha((IMAGES/(id+'.efi')).read_bytes())+'\n')
 run(['ninja','-C',BUILD,'-t','commands','t81_ternaryos_qemu_slice6_efi'],id+'-build-commands')

def boot(job):
 case,run_number=job;id=case['id'];directory=OUT/id/f'boot-{run_number}';directory.mkdir(exist_ok=False)
 stage=directory/'esp';(stage/'EFI/BOOT').mkdir(parents=True);(stage/'TERNOS').mkdir()
 shutil.copyfile(IMAGES/(id+'.efi'),stage/'EFI/BOOT/BOOTAA64.EFI');shutil.copyfile(VARS,directory/'vars.fd')
 command=[QEMU,'-nodefaults','-machine','virt,gic-version=3','-accel','tcg','-cpu','cortex-a57','-smp','1','-m','512','-nographic','-monitor','none','-serial','stdio','-global','virtio-mmio.force-legacy=off','-drive',f'if=pflash,format=raw,readonly=on,file={CODE}','-drive',f'if=pflash,format=raw,file={directory}/vars.fd','-drive',f'id=boot0,if=none,format=raw,file=fat:rw:{stage}','-device','virtio-blk-device,drive=boot0,bootindex=0']
 start=time.monotonic();deadline=start+30;process=subprocess.Popen(command,stdin=subprocess.DEVNULL,stdout=subprocess.PIPE,stderr=subprocess.STDOUT);raw=bytearray();complete=False
 try:
  while time.monotonic()<deadline:
   ready,_,_=select.select([process.stdout],[],[],.1)
   if ready:
    chunk=os.read(process.stdout.fileno(),65536)
    if not chunk:break
    raw.extend(chunk)
    if b'CPA006_DONE\r\n' in raw:complete=True;break
   if process.poll() is not None:break
 finally:
  timeout=not complete and time.monotonic()>=deadline
  if process.poll() is None:process.terminate()
  try:tail,_=process.communicate(timeout=5)
  except subprocess.TimeoutExpired:process.kill();tail,_=process.communicate()
  raw.extend(tail)
 data=bytes(raw);(directory/'serial.log.gz').write_bytes(gzip.compress(data,mtime=0))
 result=dict(id=id,profile='guest',run=run_number,command=command,supervisor_returncode=process.returncode,stopped_after_terminal=complete,timeout=timeout,raw_sha256=sha(data),disposition='INCONCLUSIVE' if timeout else 'FAIL')
 if complete and not timeout:
  try:
   markers=[b'[axion] bare-metal EL1 kernel entry',b'[axion] ExitBootServices complete;',b'[cpa006] live post-ExitBootServices evaluator',b'CPA006_ARTIFACT_HEX=']
   offsets=[data.find(x) for x in markers]
   if not all(x>=0 for x in offsets) or offsets!=sorted(offsets):raise RuntimeError('post-ExitBootServices path missing')
   blob=(ROOT/f'frozen/{id}.raw13').read_bytes();expected=json.loads((ROOT/f'frozen/{id}.expected.json').read_text());actual=parse(data,blob);compare(actual,expected)
   for frame in actual['observations']:Draft202012Validator(SCHEMA).validate(frame)
   (directory/'normalized.json').write_text(json.dumps(actual,indent=2)+'\n')
   result.update(disposition='PASS',terminal=actual['terminal'],observations=len(actual['observations']),post_exit_boot_services=True,input_identical=True)
  except Exception as exc:result['error']=str(exc)
 (directory/'result.json').write_text(json.dumps(result,indent=2)+'\n')
 # Retain raw evidence and generated input identities, not 64MiB firmware copies.
 (directory/'generated-input-hashes.json').write_text(json.dumps(dict(efi_sha256=sha((IMAGES/(id+'.efi')).read_bytes()),vars_after_run_sha256=sha((directory/'vars.fd').read_bytes())),indent=2)+'\n')
 (directory/'vars.fd').unlink();shutil.rmtree(stage)
 return result
with concurrent.futures.ThreadPoolExecutor(max_workers=3) as executor:
 for result in executor.map(boot,[(case,n) for case in SUITE for n in range(1,4)]):
  results.append(result);print(result['id'],'guest',result['run'],result['disposition'],flush=True)
summary=dict(firmware_code_sha256=sha(CODE.read_bytes()),firmware_vars_sha256=sha(VARS.read_bytes()),runs=results,disposition='PASS — BOUNDED CONFORMANCE' if all(x['disposition']=='PASS' for x in results) else ('PARTIAL' if any(x['disposition']=='INCONCLUSIVE' for x in results) else 'FAIL'))
(OUT/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
if not all(x['disposition']=='PASS' for x in results):raise SystemExit(1)
