import argparse, functools, gzip, hashlib, json, operator, os, struct, subprocess
from fractions import Fraction
from pathlib import Path

parser=argparse.ArgumentParser()
parser.add_argument('--hosted-observer',type=Path,required=True)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args()
p=Path(__file__).resolve().parents[2]; out=args.output
out.mkdir(parents=True,exist_ok=True)
def check(ok, message):
    if not ok: raise RuntimeError(message)
blob=(p/'program.raw13').read_bytes()
digest=hashlib.sha256(blob).hexdigest()
check(len(blob)==351 and digest=='b0649777cfa1c9e0fa1febb8bdfb1191c5cd3deea0d313791a4a32d5f207c80b','artifact identity')
listing=json.loads((p/'instruction-listing.json').read_text())
records=list(struct.iter_unpack('<Biii',blob))
check(records==[tuple(r[k] for k in ('opcode','a','b','c')) for r in listing],'all 27 tuples')
mask=(1<<64)-1
def sig(words):
    data=b''.join(struct.pack('<Q',w&mask) for w in words)
    value=functools.reduce(lambda h,b: ((h^b)*1099511628211)&mask,data,1469598103934665603)&((1<<63)-1)
    return value or 1
lineage=sig([27]+[w for op,a,b,c in records for w in (op,a,b,c,0)])
regs=dict.fromkeys(range(81),0); memory={}; flags=[False]*3; pc=0; tick=0; halted=False
def sync():
    regs.update({0:0,75:tick,76:lineage,77:sig([tick,0,0,0,pc,0]),78:0,79:0,80:int(not halted)})
def state():
    return dict(pc=pc,sp=283,halted=halted,flags=dict(zip(('zero','negative','positive'),flags)),
                register_values=[str(regs[i]) for i in range(81)],register_tags=[0]*81,
                memory=dict(word_count=1563,default_value='0',default_tag=0,
                    overrides=[dict(address=i,value=str(v),tag=0) for i,v in sorted(memory.items()) if v]))
def frame(before,insn):
    return dict(schema='t81.cpa003.observation.v1',program_sha256=digest,sequence=tick,
        pc_before=before,instruction=insn,status='halted' if halted else 'running',trap=None,state=state())
sync(); derived=[frame(None,None)]; visited=[]
while not halted:
    check(tick<64 and 0<=pc<27,'bounded execution')
    start=pc; op,a,b,c=records[pc]; visited.append(pc); pc+=1; tick+=1
    result=None
    if op==2: result=b
    elif op==18: result=-regs[b]
    elif op==19: result=min(regs[b],regs[c])
    elif op==20: result=max(regs[b],regs[c])
    elif op==21:
        result=regs[b]-regs[c]
        if result>1:result-=3
        elif result<-1:result+=3
    elif op in (5,7):result={5:operator.add,7:operator.mul}[op](regs[b],regs[c])
    elif op in (8,9):
        q=Fraction(regs[b],regs[c]).__trunc__()
        result=q if op==8 else regs[b]-q*regs[c]
    elif op==4:memory[a]=regs[b]
    elif op==3:result=memory.get(b,0)
    elif op==15:flags=[regs[a]==regs[b],regs[a]<regs[b],regs[a]>regs[b]]
    elif op in (11,25,60):
        take=(regs[b]==0) if op==11 else ((regs[b]!=0) if op==25 else flags[1])
        if take:pc=a
    elif op==1:halted=True
    else:raise RuntimeError('unexpected opcode/sentinel execution')
    if result is not None:
        regs[a]=result;flags=[result==0,result<0,result>0]
    sync()
    derived.append(frame(start,dict(opcode=op,name=listing[start]['name'],a=a,b=b,c=c)))
expected=json.loads((p/'expected.json').read_text())
check(derived==expected,'separate rational/integer derivation vs all 25 frozen states')
check(tick==24 and pc==27 and not {18,20,23}.intersection(visited),'final count/PC/sentinels')
check(derived[-1]['state']['sp']==283,'SP')
def normalize(raw):
    s=raw['state'];check(len(s['memory_values'])==len(s['memory_tags'])==1563,'raw memory shape')
    check(all(t==0 for t in s['memory_tags']+s['register_tags']),'Int tags')
    return dict(schema='t81.cpa003.observation.v1',program_sha256=digest,
        **{k:raw[k] for k in ('sequence','pc_before','instruction','status','trap')},
        state=dict(pc=s['pc'],sp=s['sp'],halted=s['halted'],flags=s['flags'],
            register_values=list(map(str,s['register_values'])),register_tags=s['register_tags'],
            memory=dict(word_count=1563,default_value='0',default_tag=0,overrides=[dict(address=i,value=str(v),tag=t)
                for i,(v,t) in enumerate(zip(s['memory_values'],s['memory_tags'])) if v or t])))
environment=dict(os.environ);environment.pop('T81_CANONFS_ROOT',None)
fresh=[]
for run in range(1,4):
    old=[json.loads(line) for line in gzip.decompress((p/f'evidence/acceptance/hosted-{run}.jsonl.gz').read_bytes()).decode().splitlines()]
    check([normalize(x) for x in old]==derived,'retained hosted raw vs independent derivation')
    cmd=[str(args.hosted_observer),str(p/'program.raw13')]
    observed=subprocess.run(cmd,env=environment,capture_output=True,timeout=30)
    (out/f'hosted-{run}.jsonl').write_bytes(observed.stdout)
    (out/f'hosted-{run}.stderr').write_bytes(observed.stderr)
    check(observed.returncode==0,'fresh hosted exit')
    raw=[json.loads(line) for line in observed.stdout.splitlines()]
    check([normalize(x) for x in raw]==derived,'fresh hosted vs independent derivation')
    fresh.append(dict(command=cmd,exit_code=observed.returncode,raw_sha256=hashlib.sha256(observed.stdout).hexdigest()))
result=dict(phase1='PASS',program_sha256=digest,records=27,executed=tick,final_pc=pc,sp=283,
    sentinel_pcs_executed=[],all_initial_and_post_states_equal=True,system_formulas_verified=True,fresh_hosted=fresh,
    admitted_profile='Pinned hosted raw13 implementation semantics; direct Load/Store addresses, immediate branch targets, register-tested JZ/JNZ and wrapped-difference TXor.',
    discrepancy_boundary='User CPA-004 instruction explicitly tests implemented hosted semantics while preserving normative conflicts. This is a scoped experimental-profile decision, not normative ISA acceptance or specification repair.',
    author_relationship='Same Codex agent and prior context; separate derivation is not independent-person scientific review.')
(out/'gate.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
