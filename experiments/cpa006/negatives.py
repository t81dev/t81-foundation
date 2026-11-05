import pathlib,json,subprocess,sys,copy
r=pathlib.Path('/private/tmp/t81-cpa006/experiments/cpa006');o=r/'evidence/negative';o.mkdir(exist_ok=True)
case=json.loads((r/'frozen/suite.json').read_text())['cases'][0];blob=(r/'frozen/C01.raw13').read_bytes()
(o/'cpa006_program.hpp').write_text('#pragma once\nstatic constexpr unsigned char kCpa006Program[] = {'+','.join(map(str,blob))+'};\nstatic constexpr const char* kCpa006Digest="'+case['program_sha256']+'";\nstatic constexpr unsigned kCpa006Limit=64;\n')
commands=[]
def run(cmd,name,expected=0):
 p=subprocess.run(list(map(str,cmd)),capture_output=True)
 (o/(name+'.stdout')).write_bytes(p.stdout);(o/(name+'.stderr')).write_bytes(p.stderr)
 commands.append(dict(command=list(map(str,cmd)),exit_code=p.returncode,expected=expected,stdout=name+'.stdout',stderr=name+'.stderr'))
 (o/'commands.json').write_text(json.dumps(commands,indent=2)+'\n');assert p.returncode==expected,name
 return p.stdout
base=['/opt/homebrew/opt/llvm/bin/clang++','-std=c++17','-Wall','-Wextra','-Wpedantic','-Werror','-I'+str(o),'-I'+str(r),r/'admission_test.cpp',r/'cpa006_evaluator.cpp']
for mode,flags in [('debug',['-O0','-g','-UNDEBUG']),('release',['-O3','-DNDEBUG']),('sanitized',['-O1','-DNDEBUG','-fsanitize=address,undefined'])]:
 run(base+flags+['-o',o/mode],mode+'-compile');run([o/mode],mode+'-run')
run([o/'release','wrong-expectation'],'intentional-wrong-expectation',1)
sys.path.insert(0,str(r));from observe import compare
expected=json.loads((r/'frozen/C01.expected.json').read_text());corrupt=copy.deepcopy(expected);corrupt['observations'][-1]['state']['register_values'][40]='999'
try:compare(corrupt,expected)
except RuntimeError as exc:detected=str(exc)
else:raise RuntimeError('Corrupted expected state was accepted')
(o/'corrupted-expected.json').write_text(json.dumps(corrupt,indent=2)+'\n')
(o/'corrupted-expectation-result.json').write_text(json.dumps(dict(detected=True,error=detected,original_unchanged=True),indent=2)+'\n')
print('PASS negative controls; intentional release failure exits 1')
