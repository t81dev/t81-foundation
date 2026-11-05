#!/usr/bin/env python3
"""Show valid byte mutation is caught by evidence identity, not native ISA validity."""
import json,os,subprocess
from pathlib import Path
from observe import parse,compare
ROOT=Path(__file__).resolve().parent;OUT=ROOT/'evidence/negative'
original=(ROOT/'frozen/C01.raw13').read_bytes();changed=bytearray(original);changed[5]^=1
path=OUT/'mutated-C01.raw13';path.write_bytes(changed)
env=dict(os.environ);env.pop('T81_CANONFS_ROOT',None)
command=['/private/tmp/cpa006-build/cpa006_hosted',str(path),'64']
p=subprocess.run(command,env=env,capture_output=True,timeout=30)
(OUT/'mutated-hosted.stdout').write_bytes(p.stdout);(OUT/'mutated-hosted.stderr').write_bytes(p.stderr)
if p.returncode:raise RuntimeError('unexpected native infrastructure failure')
try:parse(p.stdout,original,hosted=True)
except RuntimeError as exc:identity_error=str(exc)
else:raise RuntimeError('mutated actual bytes accepted as original')
actual=parse(p.stdout,bytes(changed),hosted=True)
try:compare(actual,json.loads((ROOT/'frozen/C01.expected.json').read_text()))
except RuntimeError as exc:state_error=str(exc)
else:raise RuntimeError('mutated instruction falsely equivalent')
(OUT/'mutated-hosted-result.json').write_text(json.dumps(dict(command=command,exit_code=p.returncode,native_accepts_valid_modified_program=True,original_identity_gate_rejects=True,identity_error=identity_error,oracle_comparison_rejects=True,state_error=state_error,scope='Guest sealed-input byte check rejects the same modified input; native production validity is a separate boundary.'),indent=2)+'\n')
print('PASS: valid native mutation executes; original identity/equivalence gates reject it')
