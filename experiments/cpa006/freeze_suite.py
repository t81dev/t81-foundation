#!/usr/bin/env python3
"""Specify and freeze the CPA006 finite suite without executing either VM.

Independent Python integer reference; never consumes hosted or guest output.
Refuses to overwrite frozen artifacts. Use --check to regenerate in scratch.
"""
import argparse,hashlib,json,struct,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent
OPS={1:'Halt',2:'LoadImm',3:'Load',4:'Store',5:'Add',7:'Mul',8:'Div',9:'Mod',11:'JumpIfZero',15:'Cmp',18:'TNot',19:'TAnd',20:'TOr',21:'TXor',25:'JumpIfNotZero',28:'Trap',60:'JumpIfNegative'}
def sha(x):return hashlib.sha256(x).hexdigest()
def ins(op,a=0,b=0,c=0):return (op,a,b,c)
def specifications():
 s=[]
 def add(id,title,rows,limit=64,classes='',excluded='Non-integer tags, registers 81..242, arbitrary programs, int64 overflow and full ISA'):
  s.append(dict(id=id,title=title,records=rows,limit=limit,operand_classes=classes,excluded=excluded))
 add('C01','Signed/zero addition and multiplication',[(2,40,-9,0),(2,41,7,0),(2,42,0,0),(5,43,40,41),(5,44,43,41),(5,45,42,42),(7,46,40,41),(7,47,40,40),(7,48,41,42),(1,0,0,0)],classes='positive, negative, zero; mixed-sign, same-sign, cancellation')
 rows=[]
 for lhs,rhs in [(-7,2),(7,-2),(-7,-2),(0,-1),(-2147483648,-1),(-2147483648,1),(2147483647,-2147483648)]:
  rows += [(2,40,lhs,0),(2,41,rhs,0),(8,42,40,41),(9,43,40,41)]
 add('C02','Signed division/remainder truncating toward zero',rows+[ins(1)],classes='all sign combinations; zero dividend; int32 min/max; min32/-1 fits int64')
 rows=[]
 for lhs in (-1,0,1):
  for rhs in (-1,0,1):
   rows += [(2,40,lhs,0),(2,41,rhs,0),(18,42,40,0),(19,43,40,41),(20,44,40,41),(21,45,40,41)]
 add('C03','Complete scalar ternary truth tables',rows+[ins(1)],classes='all 3 unary values and all 9 binary trit pairs; difference XOR')
 add('C04','Taken/untaken conditional branches with flag/register discriminator',[(2,40,0,0),(2,41,1,0),(15,40,40,0),(25,6,41,0),ins(28),ins(28),(11,8,41,0),(11,9,40,0),ins(28),(25,11,40,0),(15,40,41,0),(60,13,0,0),ins(28),(15,41,40,0),(60,16,0,0),ins(1),ins(28)],classes='JZ, JNZ and JN both taken and not taken; JNZ true while zero flag true')
 add('C05','Comparison flags and persistence through stores/branches/Halt',[(2,40,-1,0),(2,41,1,0),(15,40,40,0),(4,100,40,0),(11,5,41,0),(15,40,41,0),(4,101,41,0),(60,9,0,0),ins(28),(15,41,40,0),(4,102,40,0),ins(1)],classes='equal, less, greater; Store/branch/Halt preserve flags')
 rows=[(2,30,400,0),(2,40,7,0),(4,30,40,0),(3,41,30,0),(3,42,400,0),(4,0,40,0),(3,43,0,0),(4,0,40,0),(3,44,0,0),ins(1)]
 n=len(rows);rows[5]=(4,n,40,0);rows[6]=(3,43,n,0);rows[7]=(4,n+1535,40,0);rows[8]=(3,44,n+1535,0)
 add('C06','Direct memory and exact data boundaries',rows,classes='first data slot, last logical word, zero read; R30=400 differs from direct address 30')
 add('C07-negative','Negative memory read traps',[(3,40,-1,0),ins(1)],classes='address -1')
 add('C07-upper','One-past-memory read traps',[(3,40,1538,0),ins(1)],classes='address n+1536')
 add('C07-code','Write into logical code region traps',[(2,40,7,0),(4,0,40,0),ins(1)],classes='address 0 is protected code')
 for name,op in [('div',8),('mod',9)]:add('C08-'+name,'Division/remainder by zero runtime trap',[(2,40,-7,0),(2,41,0,0),(op,42,40,41),ins(1)],classes='negative dividend and zero divisor')
 add('C09','Explicit Trap terminates at executed boundary',[(2,40,3,0),ins(28),ins(1)],classes='TrapInstruction; no synthetic post-trap continuation')
 add('C10-undefined','Undefined opcode rejected before execution',[(255,0,0,0)],classes='undefined byte 255')
 add('C10-truncated','Truncated raw13 record rejected before execution',[],classes='12 bytes: last operand incomplete')
 add('C10-excluded','Defined out-of-subset opcode rejected by experimental admission',[(36,0,0,0)],classes='FAdd 36 is valid production opcode, outside suite subset')
 add('C11','Instruction budget exhaustion in a bounded loop',[(11,0,0,0)],limit=8,classes='R0=0; eight taken JZ iterations; no Halt; below JIT hotspot threshold')
 add('C12-valid','Admitted register boundaries and reserved-register reads',[(2,0,9,0),(2,74,-1,0),(5,40,0,74),(15,80,0,0),ins(1)],classes='R0 write then reset; R74 highest ordinary; R80 seal read')
 add('C12-destination','Invalid destination register traps',[(2,243,1,0),ins(1)],classes='243 is one past native register file; extended 81..242 excluded')
 add('C12-source','Negative source register traps',[(5,40,-1,0),ins(1)],classes='source -1')
 return s

def signature(words):
 h=1469598103934665603
 for x in words:
  for b in (x%(1<<64)).to_bytes(8,'little'):h=((h^b)*1099511628211)%(1<<64)
 return (h% (1<<63)) or 1

def oracle(blob,limit):
 digest=sha(blob)
 if not blob or len(blob)%13:return dict(classification='admission_rejection',reason='malformed',steps=0),[]
 rows=list(struct.iter_unpack('<Biii',blob));n=len(rows)
 if any(op not in OPS for op,a,b,c in rows):
  reason='undefined_opcode' if any(op==255 for op,a,b,c in rows) else 'unsupported_profile'
  return dict(classification='admission_rejection',reason=reason,steps=0),[]
 assert 1<=n<=64 and 0<=limit<=64
 regs=[0]*81;memory=[0]*(n+1536);flags=dict(zero=False,negative=False,positive=False)
 pc=tick=0;halted=False;trap=None;trace=[]
 lineage=signature([n]+[x for row in rows for x in (*row,0)])
 def frame(before,instruction):
  regs[0]=0;regs[75:81]=[tick,lineage,signature([tick,0,0,0,pc,0]),0,0,int(not halted)]
  return dict(schema='t81.cpa006.observation.v1',program_sha256=digest,sequence=tick,
    pc_before=before,instruction=instruction,status='trap' if trap else ('halted' if halted else 'running'),trap=trap,
    state=dict(pc=pc,sp=n+256,halted=halted,flags=flags.copy(),register_values=list(map(str,regs)),register_tags=[0]*81,
      memory=dict(word_count=len(memory),default_value='0',default_tag=0,
       overrides=[dict(address=i,value=str(x),tag=0) for i,x in enumerate(memory) if x])))
 def valid(r):return 0<=r<=80
 trace.append(frame(None,None))
 while tick<limit and not halted and not trap:
  assert 0<=pc<n
  before=pc;op,a,b,c=rows[pc];pc+=1;tick+=1;value=None
  needs={2:(a,),3:(a,),4:(b,),5:(a,b,c),7:(a,b,c),8:(a,b,c),9:(a,b,c),15:(a,b),18:(a,b),19:(a,b,c),20:(a,b,c),21:(a,b,c),11:(b,),25:(b,)}
  if any(not valid(r) for r in needs.get(op,())):trap='DecodeFault'
  elif op in (3,4) and not n<=(b if op==3 else a)<len(memory):trap='BoundsFault'
  elif op==1:halted=True
  elif op==2:value=b
  elif op==3:value=memory[b]
  elif op==4:memory[a]=regs[b]
  elif op==5:value=regs[b]+regs[c]
  elif op==7:value=regs[b]*regs[c]
  elif op in (8,9):
   lhs,rhs=regs[b],regs[c]
   if rhs==0:trap='DivisionFault'
   else:
    q=abs(lhs)//abs(rhs)*(-1 if (lhs<0)!=(rhs<0) else 1)
    value=q if op==8 else lhs-q*rhs
  elif op==15:flags=dict(zero=regs[a]==regs[b],negative=regs[a]<regs[b],positive=regs[a]>regs[b])
  elif op==18:value=-regs[b]
  elif op==19:value=min(regs[b],regs[c])
  elif op==20:value=max(regs[b],regs[c])
  elif op==21:value=((regs[b]-regs[c]+1)%3)-1
  elif op in (11,25,60):
   take=regs[b]==0 if op==11 else (regs[b]!=0 if op==25 else flags['negative'])
   if take:
    if not 0<=a<=n:trap='DecodeFault'
    else:pc=a
  elif op==28:trap='TrapInstruction'
  else:raise ValueError(op)
  if value is not None:
   assert -(1<<63)<=value<(1<<63)
   if op==2 and (a==0 or 75<=a<=80): value=regs[a]
   else: regs[a]=value
   flags=dict(zero=value==0,negative=value<0,positive=value>0)
  trace.append(frame(before,dict(opcode=op,name=OPS[op],a=a,b=b,c=c)))
 terminal=dict(classification='runtime_trap' if trap else ('halted' if halted else 'instruction_limit'),reason=trap,steps=tick)
 return terminal,trace

def generate(out):
 out.mkdir();suite=[]
 for spec in specifications():
  rows=spec.pop('records');blob=b''.join(struct.pack('<Biii',*r) for r in rows)
  if spec['id']=='C10-truncated':blob=struct.pack('<Biii',2,40,1,0)[:-1]
  terminal,trace=oracle(blob,spec['limit']);id=spec['id']
  (out/(id+'.raw13')).write_bytes(blob)
  expected=dict(terminal=terminal,observations=trace)
  data=(json.dumps(expected,indent=2)+'\n').encode();(out/(id+'.expected.json')).write_bytes(data)
  spec.update(program_sha256=sha(blob),bytes=len(blob),records=len(rows),expected_sha256=sha(data),terminal=terminal,
   sequence=[f['pc_before'] for f in trace[1:]],initial_state=trace[0]['state'] if trace else None,
   permitted_observations='Initial plus every actually attempted instruction through Halt/trap; no artificial state after rejection or budget exhaustion. Common R0..R80 + all logical memory/tags, flags, PC/SP, halt, instruction, tick/status/trap.',
   canonicality='Canonical raw13 integer records' if not id.startswith('C10') else 'Deliberately invalid or outside-profile input; not a valid canonical ISA program')
  suite.append(spec)
 (out/'suite.json').write_text(json.dumps(dict(schema='t81.cpa006.suite.v1',cases=suite),indent=2)+'\n')
 schema=json.loads((ROOT.parent/'cpa004/observation.schema.json').read_text())
 # Reuse the field shapes, never historical fixture identities or expected data.
 def walk(x):
  if isinstance(x,dict):
   if x.get('const')=='t81.cpa003.observation.v1':x['const']='t81.cpa006.observation.v1'
   if x.get('const')=='b0649777cfa1c9e0fa1febb8bdfb1191c5cd3deea0d313791a4a32d5f207c80b':x.pop('const');x['pattern']='^[0-9a-f]{64}$'
   if x.get('const')==283:x.pop('const');x.update(type='integer',minimum=257,maximum=320)
   if x.get('maxItems')==1563:x['maxItems']=1600
   if x.get('const')==1563:x.pop('const');x.update(minimum=1537,maximum=1600)
   # Existing schema had profile-specific instruction and address limits.
   if x.get('maximum')==27:x['maximum']=64
   if x.get('maximum')==26:x['maximum']=63
   if x.get('maximum')==1562:x['maximum']=1599
   for v in list(x.values()):walk(v)
  elif isinstance(x,list):
   for v in x:walk(v)
 walk(schema);schema['$id']='urn:t81:cpa006:observation:v1'
 (out/'observation.schema.json').write_text(json.dumps(schema,indent=2)+'\n')
 manifest={p.name:dict(bytes=p.stat().st_size,sha256=sha(p.read_bytes())) for p in out.iterdir()}
 (out/'manifest.json').write_text(json.dumps(dict(files=manifest),indent=2)+'\n')

def main():
 parser=argparse.ArgumentParser();parser.add_argument('--check',action='store_true');args=parser.parse_args()
 if args.check:
  with tempfile.TemporaryDirectory() as temp:
   out=Path(temp)/'frozen';generate(out)
   for p in out.iterdir():
    if p.read_bytes()!=(ROOT/'frozen'/p.name).read_bytes():raise RuntimeError('Frozen mismatch '+p.name)
  print('PASS frozen regeneration; no evaluator output used')
 else:generate(ROOT/'frozen')
if __name__=='__main__':main()
