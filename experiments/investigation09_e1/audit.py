#!/usr/bin/env python3
"""Independent post-gate artifact decoder, VM replay, and cost ledger.

Does not use the runner's expected size or ratio calculations. No timing.
"""
import argparse
from collections import Counter
import hashlib
import importlib.metadata
import json
import os
from pathlib import Path
import struct
import subprocess
import tempfile

from oracle import decode, expected


def digest(p):
    return hashlib.sha256(p.read_bytes()).hexdigest()


class Reader:
    def __init__(self, b): self.b, self.i = b, 0
    def u8(self):
        x = self.b[self.i]; self.i += 1; return x
    def leb(self):
        value, shift = 0, 0
        while True:
            x = self.u8(); value |= (x & 127) << shift
            if x < 128: return value
            shift += 7
    def vector(self):
        n = self.leb(); b = self.b[self.i:self.i+n]; self.i += n; return b


def wasm_ledger(path):
    data = path.read_bytes()
    if data[:8] != b'\0asm\1\0\0\0': raise ValueError("not core Wasm v1")
    r = Reader(data); r.i = 8
    sections, ops, locals_payload, memory_pages = [], Counter(), [], []
    while r.i < len(data):
        start = r.i; kind = r.u8(); body = Reader(r.vector())
        sections.append({"section_id": kind, "bytes_including_header": r.i-start})
        if kind == 5:
            for _ in range(body.leb()):
                flags = body.leb(); minimum = body.leb()
                maximum = body.leb() if flags & 1 else None
                if flags & ~1: raise ValueError("unexpected shared/memory64")
                memory_pages.append({"minimum": minimum, "maximum": maximum})
        if kind != 10: continue
        for _ in range(body.leb()):
            fn = Reader(body.vector())
            local_bytes = 0
            for _ in range(fn.leb()):
                count, ty = fn.leb(), fn.u8()
                local_bytes += count * {0x7f:4, 0x7e:8}[ty]
            locals_payload.append(local_bytes)
            while fn.i < len(fn.b):
                op = fn.u8(); ops[f"0x{op:02x}"] += 1
                if op in (2,3,4):
                    fn.leb()  # block type (including signed type-index encoding)
                elif op in (12,13,16,32,33,34,35,36,37,38,63,64): fn.leb()
                elif op == 14:
                    for _ in range(fn.leb()+1): fn.leb()
                elif op == 17: fn.leb(); fn.leb()
                elif 0x28 <= op <= 0x3e: fn.leb(); fn.leb()
                elif op in (0x41,0x42): fn.leb()  # signed LEB has same byte extent
                elif op == 0x43: fn.i += 4
                elif op == 0x44: fn.i += 8
                elif op in (0,1,5,11,15,26,27) or 0x45 <= op <= 0xc4: pass
                else: raise ValueError(f"unsupported Wasm decoder opcode {op:#x} in {path}")
            if fn.i != len(fn.b): raise ValueError("instruction decoder overrun")
    if 8 + sum(x['bytes_including_header'] for x in sections) != len(data):
        raise ValueError("section ledger does not reconcile")
    return {"sections": sections, "decoded_instruction_count_including_end_else": sum(ops.values()),
            "opcode_histogram": dict(ops), "nonparameter_local_payload_bytes_per_function": locals_payload,
            "memory_limits_pages": memory_pages}


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument('evidence', type=Path)
    ap.add_argument('--driver', type=Path, required=True)
    ap.add_argument('--wasm-dis', type=Path, required=True)
    args = ap.parse_args()
    evidence = args.evidence.resolve()
    report = json.loads((evidence/'comparison.json').read_text())
    rows = json.loads((evidence/'raw-parity.json').read_text())
    ledgers, replay, commands = [], [], []
    # Reject unexpected fault types, not merely the existence of some fault.
    for row in rows:
        if not row['oracle']['fault']: continue
        obs = row['observed']; fault = obs['fault']
        if fault == 6: pass  # driver convention int(Trap::DivisionFault)+1
        elif fault == 'trap' and 'unreachable' in obs['message']: pass
        elif fault == 'native_trap' and obs['signal'] in (4,5): pass
        else: raise ValueError(f"unexplained expected-fault classification: {row}")
    disdir = evidence/'disassembly'
    disdir.mkdir(exist_ok=False)
    import wasmtime
    engine = wasmtime.Engine()
    for entry in report['inventory']:
        p = evidence/entry['path']
        record = {"path":entry['path'],"kind":entry['kind'],"module":entry['module'],
                  "bytes":len(p.read_bytes()),"sha256":digest(p)}
        if entry['kind'].endswith('tisc'):
            n = struct.unpack_from('<Q',p.read_bytes())[0]
            record.update(instruction_count=n, saved_instruction_record_bytes=18,
                          saved_instruction_bytes=18*n, other_saved_bytes=p.stat().st_size-18*n,
                          encoded_stream_bytes=13*n)
            if n != entry['instructions'] or entry['encoded_stream_bytes'] != 13*n:
                raise ValueError("TISC instruction ledger mismatch")
            label = 'compiler-tisc' if entry['kind']=='tisc' else p.name
            original = [r for r in rows if r['module']==entry['module'] and r['implementation']==label]
            observed = []
            for old in original:
                case = old['inputs']
                with tempfile.TemporaryDirectory(prefix='t81-i09-audit-') as store:
                    if list(Path(store).iterdir()): raise ValueError('nonempty store')
                    argv = [str(args.driver.resolve()),'run',str(p)]
                    inp = ' '.join(map(str,case))+'\n'
                    cp = subprocess.run(argv,input=inp,text=True,capture_output=True,
                                        env=dict(os.environ,T81_CANONFS_ROOT=store))
                    commands.append({"argv":argv,"stdin":inp,"returncode":cp.returncode,
                                     "stderr":cp.stderr,"stdout":cp.stdout,"store_initially_empty":True})
                    if cp.returncode: raise ValueError(cp.stderr)
                    obs = json.loads(cp.stdout)
                oracle = expected(entry['module'],case)
                if obs['fault'] != old['observed']['fault'] or obs['result'] != old['observed']['result']:
                    raise ValueError('independent artifact replay changed result/fault')
                if obs['dynamic_instructions'] != len(obs['steps'])+4:
                    raise ValueError('adapter instruction count does not reconcile')
                if not oracle['fault'] and decode(entry['module'],case,obs['result']) != oracle['values']:
                    raise ValueError('independent oracle mismatch')
                for step in obs['steps']:
                    if step['op'] in (12,18,19,20,59):
                        v=step['value']
                        if step['flags'] != [int(v==0),int(v<0),int(v>0)]:
                            raise ValueError('consumed arithmetic/logic flags differ')
                # Division must be skipped precisely when short-circuiting.
                if entry['module']=='boolean':
                    divided = any(s['op']==8 for s in obs['steps'])
                    if divided != oracle['rhs_evaluated']:
                        raise ValueError('boolean RHS evaluation-order mismatch')
                observed.append(obs)
                replay.append({"path":entry['path'],"inputs":case,"observed":obs})
            record['dynamic_instruction_range']=[min(x['dynamic_instructions'] for x in observed),max(x['dynamic_instructions'] for x in observed)]
            record['runtime_state_ranges']={key:[min(x[key] for x in observed),max(x[key] for x in observed)] for key in observed[0] if key.endswith('_bytes') or key in ('trace_entries','policy_events')}
        else:
            record.update(wasm_ledger(p))
            # Actual fresh instance memory, in addition to binary memory limits.
            store=wasmtime.Store(engine)
            mod=wasmtime.Module.from_file(engine,str(p))
            inst=wasmtime.Instance(store,mod,[])
            memory=inst.exports(store).get('memory')
            record['initial_exported_memory_bytes']=memory.data_len(store) if memory else None
            record['memory_growth_instructions']=record['opcode_histogram'].get('0x40',0)
            target=disdir/(p.stem+'.wat')
            argv=[str(args.wasm_dis.resolve()),str(p),'-o',str(target)]
            cp=subprocess.run(argv,text=True,capture_output=True)
            commands.append({"argv":argv,"returncode":cp.returncode,"stdout":cp.stdout,"stderr":cp.stderr})
            if cp.returncode: raise ValueError(cp.stderr)
        ledgers.append(record)
    # Byte arithmetic is independent of the original runner's ratio result.
    tisc=next(x for x in ledgers if x['kind']=='tisc' and x['module']=='combined')
    best=min((x for x in ledgers if x['kind']=='wasm' and x['module']=='combined'),key=lambda x:x['bytes'])
    # All common entry contracts have four signed 64-bit words (32 bytes).
    # The abstract graph never requires more simultaneous payload: after selector
    # dispatch, decision needs at most meet, negation and final (24 bytes), then
    # accumulates the same three values. This is logical ABI storage, not RSS.
    logical={m:{"entry_payload_bytes":32,"abstract_peak_live_payload_bytes":32,
                "decision_intermediate_payload_bytes":24 if m in ('combined','decision') else None,
                "scope":"common 64-bit ABI semantic graph; excludes physical copies, tags and runtime metadata"}
             for m in {x['module'] for x in ledgers}}
    support=[]
    distribution=importlib.metadata.distribution('wasmtime')
    runtime=[distribution.locate_file(x) for x in distribution.files if str(x).endswith(('.dylib','.so','.dll'))]
    for name,p in [('tisc_adapter_and_statically_linked_runtime',args.driver.resolve()),('python_executable',Path(os.sys.executable).resolve()),('binaryen_wasm_dis_tool',args.wasm_dis.resolve())]+[('wasmtime_shared_library',Path(x)) for x in runtime]:
        support.append({"component":name,"path":str(p),"bytes":p.stat().st_size,"sha256":digest(p)})
    value={"artifact_ledgers":ledgers,"logical_state":logical,"support_binaries":support,
           "replay_observations":len(replay),"raw_parity_rows":len(rows),
           "expected_fault_rows":sum(bool(r['oracle']['fault']) for r in rows),
           "combined":{"tisc_bytes":tisc['bytes'],"best_wasm_bytes":best['bytes'],"ratio":tisc['bytes']/best['bytes'],
                       "best_wasm_path":best['path'],"logical_state_increases":False,
                       "threshold_pass":20*tisc['bytes']<=19*best['bytes'],"disposition":"NOT SUPPORTED" if 20*tisc['bytes']>19*best['bytes'] else 'SUPPORTED_FOR_THIS_CONTRACT'}}
    def save(name,value): (evidence/name).write_text(json.dumps(value,indent=2,sort_keys=True)+'\n')
    save('ledger.json',value); save('independent-replay.json',replay); save('audit-commands.json',commands)
    print(json.dumps(value['combined'],indent=2))


if __name__=='__main__': main()
