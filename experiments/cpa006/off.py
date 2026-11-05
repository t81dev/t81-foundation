from pathlib import Path
import subprocess,json,hashlib
r=Path('/private/tmp/t81-cpa006');o=r/'experiments/cpa006/evidence/isolation';o.mkdir(exist_ok=True);build=Path('/private/tmp/cpa006-build-off');commands=[]
def run(cmd,name):
 p=subprocess.run(list(map(str,cmd)),cwd=r,capture_output=True);(o/(name+'.stdout')).write_bytes(p.stdout);(o/(name+'.stderr')).write_bytes(p.stderr)
 commands.append(dict(command=list(map(str,cmd)),exit_code=p.returncode,stdout=name+'.stdout',stderr=name+'.stderr'));(o/'commands.json').write_text(json.dumps(commands,indent=2)+'\n');assert p.returncode==0,name
 return p.stdout
run(['cmake','-S',r,'-B',build,'-G','Ninja','-DCMAKE_BUILD_TYPE=Release','-DCMAKE_C_COMPILER=/opt/homebrew/opt/llvm/bin/clang','-DCMAKE_CXX_COMPILER=/opt/homebrew/opt/llvm/bin/clang++','-DT81_BUILD_TESTS=OFF','-DT81_BUILD_TUI=OFF','-DT81_BUILD_EXAMPLES=OFF','-DT81_BUILD_BENCHMARKS=OFF','-DT81_BUILD_FUZZ_TESTS=OFF','-DT81_ENABLE_TERNARYOS=ON','-DT81_ENABLE_DPE=OFF'],'configure')
run(['cmake','--build',build,'--target','t81_ternaryos_qemu_slice6_efi','-j','4'],'build')
link=run(['ninja','-C',build,'-t','commands','t81_ternaryos_qemu_slice6_efi'],'link-commands')
assert b'cpa006_evaluator.cpp' not in link and b'cpa004_evaluator.cpp' not in link
cache=(build/'CMakeCache.txt').read_text();assert 'T81_BUILD_CPA006_EXPERIMENT:BOOL=OFF' in cache and 'T81_BUILD_CPA004_EXPERIMENT:BOOL=OFF' in cache
assert not (build/'ternaryos/generated/cpa006_program.hpp').exists()
obj=build/'ternaryos/qemu_slice6/BOOTAA64_bare.obj';dis=run(['/opt/homebrew/opt/llvm/bin/llvm-objdump','-d',obj],'bare-disassembly').splitlines()[3:]
old=Path('/Users/t81dev/Code/t81-foundation/experiments/cpa005/evidence/isolation')
assert dis==(old/'main-bare-disassembly.stdout').read_bytes().splitlines()[3:]
efi=build/'ternaryos/qemu_slice6/BOOTAA64.EFI'
for section in ('text','rdata'):
 run(['/opt/homebrew/opt/llvm/bin/llvm-objcopy','--dump-section','.'+section+'='+str(o/('off-efi.'+section)),efi],'extract-'+section)
 assert (o/('off-efi.'+section)).read_bytes()==(old/('main-efi.'+section)).read_bytes(),section
run(['/opt/homebrew/opt/llvm/bin/llvm-readobj','--coff-imports',efi],'imports')
(o/'result.json').write_text(json.dumps(dict(default_off=True,neither_evaluator_linked=True,no_generated_cpa006_program=True,bare_disassembly_identical_to_main=True,efi_text_identical_to_main=True,efi_rdata_identical_to_main=True,main='ea6d031a8117341fe03013cc9d5473de1b07727d',efi_sha256=hashlib.sha256(efi.read_bytes()).hexdigest()),indent=2)+'\n')
print('PASS default off')
