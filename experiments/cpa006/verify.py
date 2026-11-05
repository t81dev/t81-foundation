#!/usr/bin/env python3
"""Replay all raw records and frozen identities; never trust PASS summaries alone."""
import argparse,copy,gzip,hashlib,json,subprocess
from pathlib import Path
from jsonschema import Draft202012Validator
from observe import parse,compare
ROOT=Path(__file__).resolve().parent

def sha(x):return hashlib.sha256(x).hexdigest()
def require(ok,why):
 if not ok:raise RuntimeError(why)
def main():
 parser=argparse.ArgumentParser();parser.add_argument('--corrupt-expected',action='store_true');args=parser.parse_args()
 receipt=json.loads((ROOT/'evidence/freeze-receipt.json').read_text())
 require(sha((ROOT/'frozen/manifest.json').read_bytes())==receipt['manifest_sha256'],'preexecution manifest receipt')
 require(sha((ROOT/'freeze_suite.py').read_bytes())==receipt['oracle_sha256'],'preexecution oracle receipt')
 for name,pin in json.loads((ROOT/'frozen/manifest.json').read_text())['files'].items():
  data=(ROOT/'frozen'/name).read_bytes();require(len(data)==pin['bytes'] and sha(data)==pin['sha256'],'frozen '+name)
 subprocess.run(['python3','-B',str(ROOT/'freeze_suite.py'),'--check'],check=True)
 schema=json.loads((ROOT/'frozen/observation.schema.json').read_text());Draft202012Validator.check_schema(schema);validator=Draft202012Validator(schema)
 suite=json.loads((ROOT/'frozen/suite.json').read_text())['cases'];rows=[];schema_count=0
 for case in suite:
  id=case['id'];blob=(ROOT/f'frozen/{id}.raw13').read_bytes();expected=json.loads((ROOT/f'frozen/{id}.expected.json').read_text())
  if args.corrupt_expected and id=='C01':expected['observations'][-1]['state']['register_values'][40]='999'
  for frame in expected['observations']:validator.validate(frame);schema_count+=1
  hosted=[];guest=[];d=ROOT/'evidence/suite'/id
  for run in range(1,4):
   for kind in ('hosted','guest'):
    path=d/f'hosted-{run}.raw.gz' if kind=='hosted' else d/f'boot-{run}/serial.log.gz'
    record_path=d/f'hosted-{run}.result.json' if kind=='hosted' else d/f'boot-{run}/result.json'
    record=json.loads(record_path.read_text());raw=gzip.decompress(path.read_bytes())
    require(sha(raw)==record['raw_sha256'] and not record['timeout'],id+' raw identity or timeout')
    if kind=='hosted':require(record['exit_code']==0,id+' hosted process failure')
    else:
     markers=[b'[axion] bare-metal EL1 kernel entry',b'[axion] ExitBootServices complete;',b'[cpa006] live post-ExitBootServices evaluator',b'CPA006_ARTIFACT_HEX=']
     offsets=[raw.find(x) for x in markers]
     require(all(x>=0 for x in offsets) and offsets==sorted(offsets),id+' actual guest call path')
    actual=parse(raw,blob,hosted=(kind=='hosted'));compare(actual,expected)
    for frame in actual['observations']:validator.validate(frame);schema_count+=1
    (hosted if kind=='hosted' else guest).append(actual)
  require(hosted==guest,id+' hosted vs guest')
  # Extract evidence-backed finite coverage without claiming unexecuted variants.
  instructions=[f['instruction'] for f in expected['observations'][1:]]
  branches=[]
  for position,frame in enumerate(expected['observations'][1:],1):
   i=frame['instruction']
   if i['opcode'] in (11,25,60):
    before=expected['observations'][position-1]['state']
    taken=(int(before['register_values'][i['b']])==0) if i['opcode']==11 else ((int(before['register_values'][i['b']])!=0) if i['opcode']==25 else before['flags']['negative'])
    branches.append(dict(pc=frame['pc_before'],opcode=i['name'],taken=taken,predicate_basis='Prior observed state plus selected frozen semantics',outcome_distinguished_by_pc=i['a']!=frame['pc_before']+1))
  rows.append(dict(id=id,status='SUPPORTED',title=case['title'],instruction_semantics=sorted({i['name'] for i in instructions}),operand_classes=case['operand_classes'],branches=branches,terminal=expected['terminal'],boundaries=len(expected['observations']),hosted=dict(runs=3,result='PASS'),freestanding=dict(boots=3,result='PASS'),oracle_agreement=True,program_identity=True,evidence_complete=True,excluded=case['excluded'],memory_behavior=case['title'] if id.startswith(('C06','C07')) else 'No dedicated memory boundary claim',metadata_limit='C01 label includes cancellation, but nonzero cancellation is not actually exercised.' if id=='C01' else None))
 matrix=dict(schema='t81.cpa006.conformance.v1',disposition='PASS — BOUNDED CONFORMANCE',cases=rows,hosted_runs=57,fresh_guests=57,schema_records=schema_count,finite_profile=True,unsupported=['all other opcodes','registers 81..242','non-Int tags and arbitrary precision','int64 overflow and INT64_MIN/-1','arbitrary programs and concurrency','untested malformed metadata or branch targets','Linux/Windows hosts and physical hardware'])
 (ROOT/'conformance.json').write_text(json.dumps(matrix,indent=2)+'\n');print(json.dumps({k:v for k,v in matrix.items() if k!='cases'},indent=2))
if __name__=='__main__':main()
