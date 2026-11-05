#!/usr/bin/env python3
"""Build and check the sealed guest evaluator natively under NDEBUG + sanitizers."""
from pathlib import Path
import json,subprocess,tempfile,hashlib
from observe import parse,compare
ROOT=Path(__file__).resolve().parent
OUT=ROOT/'evidence/native';OUT.mkdir(parents=True,exist_ok=True)
CC='/opt/homebrew/opt/llvm/bin/clang++'
commands=[]
def run(cmd,name,expected=0):
 p=subprocess.run(list(map(str,cmd)),capture_output=True)
 (OUT/(name+'.stdout')).write_bytes(p.stdout);(OUT/(name+'.stderr')).write_bytes(p.stderr)
 commands.append(dict(command=list(map(str,cmd)),exit_code=p.returncode,expected=expected,stdout=name+'.stdout',stderr=name+'.stderr'))
 (OUT/'commands.json').write_text(json.dumps(commands,indent=2)+'\n')
 if p.returncode!=expected:raise RuntimeError(name)
 return p.stdout
suite=json.loads((ROOT/'frozen/suite.json').read_text())['cases']
results=[]
for case in suite:
 id=case['id'];blob=(ROOT/f'frozen/{id}.raw13').read_bytes()
 with tempfile.TemporaryDirectory(prefix='cpa006-native-') as temp:
  path=Path(temp)
  header='#pragma once\nstatic constexpr unsigned char kCpa006Program[] = {'+','.join(str(x) for x in blob)+'};\nstatic constexpr const char* kCpa006Digest = "'+case['program_sha256']+'";\nstatic constexpr unsigned kCpa006Limit = '+str(case['limit'])+';\n'
  (path/'cpa006_program.hpp').write_text(header)
  cmd=[CC,'-std=c++17','-O1','-g','-DNDEBUG','-Wall','-Wextra','-Wpedantic','-Werror','-fsanitize=address,undefined','-fno-omit-frame-pointer','-I'+str(path),ROOT/'native_guest.cpp',ROOT/'cpa006_evaluator.cpp','-o',path/'guest']
  run(cmd,id+'-compile');raw=run([path/'guest'],id+'-run')
  actual=parse(raw,blob);expected=json.loads((ROOT/f'frozen/{id}.expected.json').read_text());compare(actual,expected)
  results.append(dict(id=id,match=True,sanitizers=True,NDEBUG=True))
(OUT/'result.json').write_text(json.dumps(results,indent=2)+'\n')
print('PASS',len(results),'native sealed cases with ASan/UBSan and NDEBUG')
