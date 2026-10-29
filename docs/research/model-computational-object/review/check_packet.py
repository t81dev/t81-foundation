#!/usr/bin/env python3
"""Audit frozen research data and JSON schemas; does NOT implement authorization.

No candidate modules, model invocation, policy engine or effect sink are loaded.
Semantic decision annotations require the independent procedure in ORACLE_SPEC.md.
"""
import base64
from collections import Counter
from decimal import Decimal
import hashlib
from importlib.metadata import version
import itertools
import json
from pathlib import Path
import platform
import re
import sys
from jsonschema import Draft202012Validator, validators

ROOT=Path(__file__).resolve().parents[1]
def wire(v):return json.dumps(v,sort_keys=True,ensure_ascii=True,separators=(',',':')).encode()
def sha(v):return hashlib.sha256(v).hexdigest()
def ref(kind,raw):return 'sha256:'+sha(kind.encode()+b'\0'+raw)
def require(value,message):
    if not value:raise RuntimeError(message)
def decode(s):return None if s is None else base64.b64decode(s,validate=True)
def rows(name):return [json.loads(line) for line in (ROOT/'corpus'/name).read_bytes().splitlines()]

# JSON Schema's integer is mathematical, not a Python type or numeric spelling.
# Exact Decimal parsing avoids e.g. 1e-4000 underflowing to an integer zero.
def mathematical_integer(checker,value):
    return ((isinstance(value,int) and not isinstance(value,bool)) or
            (isinstance(value,Decimal) and value.is_finite() and value==value.to_integral_value()))
ExactValidator=validators.extend(Draft202012Validator,
    type_checker=Draft202012Validator.TYPE_CHECKER.redefine('integer',mathematical_integer))

def profile(raw,limit):
    if raw is None:return 'not_observed',None
    if len(raw)>limit:return 'byte_limit',None
    try:text=raw.decode('utf-8',errors='strict')
    except UnicodeDecodeError:return 'utf8',None
    if text.startswith('\ufeff'):return 'bom',None
    def pairs(items):
        keys=[k for k,v in items]
        if len(set(keys))!=len(keys):raise ValueError('duplicate_key')
        return dict(items)
    def constant(text):raise ValueError('json_syntax')
    try:value=json.loads(text,object_pairs_hook=pairs,parse_float=Decimal,parse_int=Decimal,parse_constant=constant)
    except (ValueError,ArithmeticError) as error:
        return ('duplicate_key' if str(error)=='duplicate_key' else 'json_syntax'),None
    def scalar(v):
        if isinstance(v,str):return not any(0xd800<=ord(c)<=0xdfff for c in v)
        if isinstance(v,list):return all(scalar(x) for x in v)
        if isinstance(v,dict):return all(scalar(k) and scalar(x) for k,x in v.items())
        return True
    if not scalar(value):return 'unicode_scalar',None
    def depth(v):
        children=list(v.values()) if isinstance(v,dict) else v if isinstance(v,list) else None
        return 0 if children is None else 1+max((depth(x) for x in children),default=0)
    if depth(value)>4:return 'depth_limit',None
    return 'ok',value


def main():
    schemas={k:json.loads((ROOT/'schemas'/f'{k}.json').read_text(encoding='utf-8')) for k in ['request','response']}
    originals={k:json.loads((ROOT/'schemas'/f'{k}.cd7d4620.json').read_text(encoding='utf-8')) for k in schemas}
    embedded=[json.loads(x) for x in re.findall(r'```json\n(.*?)\n```',(ROOT/'FORMAL_MODEL.md').read_text(encoding='utf-8'),re.S)]
    require(embedded==list(schemas.values()),'embedded/extracted schema drift')
    for schema in [*schemas.values(),*originals.values()]:Draft202012Validator.check_schema(schema)
    objects=json.loads((ROOT/'corpus/objects.json').read_text(encoding='utf-8'))
    for name,o in objects.items():
        raw=decode(o['bytes_base64'])
        require(sha(raw)==o['raw_sha256'] and ref(o['kind'],raw)==o['ref'],f'object identity: {name}')
    cases=rows('cases.jsonl'); require(len(cases)==60,'case count')
    require(Counter(c['family'] for c in cases)=={n:5 for n in range(1,13)},'family balance')
    require(len({c['case_id'] for c in cases})==60,'duplicate case labels')
    profiles=Counter();schema_results=Counter()
    for c in cases:
        data=dict(c);declared=data.pop('fixture_sha256')
        require(sha(wire(data))==declared,c['case_id']+' fixture hash')
        require(sha(wire(c['context']))==c['context_sha256'],c['case_id']+' context hash')
        for kind,limit in [('request',8192),('response',4096)]:
            raw=decode(c[kind+'_bytes_base64']); status,value=profile(raw,limit)
            schema_status=('valid' if ExactValidator(schemas[kind]).is_valid(value) else 'invalid') if status=='ok' else 'not_run'
            expected=c['expected']['wire_review'][kind]
            require({'profile':status,'schema':schema_status}=={k:expected[k] for k in ['profile','schema']},c['case_id']+' '+kind+' profile/schema')
            actual_errors=([dict(keyword=x.validator,path=''.join('/'+str(y) for y in x.absolute_path)) for x in ExactValidator(schemas[kind]).iter_errors(value)] if status=='ok' else [])
            require(actual_errors==expected['schema_errors'],c['case_id']+' '+kind+' schema diagnostics')
            profiles[kind+':'+status]+=1;schema_results[kind+':'+schema_status]+=1
            if raw is not None:
                rawhash=c['request_raw_sha256' if kind=='request' else 'response_raw_sha256']
                identity=c['request_ref' if kind=='request' else 'output_ref']
                require(sha(raw)==rawhash and ref('request' if kind=='request' else 'output',raw)==identity,c['case_id']+' wire identity')
        for entry in c['context']['registry'].values():require(entry in objects,c['case_id']+' missing catalog bytes')
        # Arithmetic consistency of declared sink expectations, not execution.
        expected=c['expected'];admissions=expected['admissions']
        require(sum(a['new_rows'] for a in admissions)==expected['new_rows'],c['case_id']+' row annotation')
        require(sum(a['effect_dispatches'] for a in admissions)==expected['effect_dispatches'],c['case_id']+' dispatch annotation')
        require(all(a['effect_dispatches']==0 for a in admissions if a['decision']!='P'),c['case_id']+' unsafe declared dispatch')
        journal=expected['transition_expectations']
        state=dict(policy_state=c['context']['policy_state'],replay_state=c['context']['replay_state'],
                   remaining_units=c['context']['remaining_units'],rows=len(c['context']['initial_rows']),
                   owner_sequence=len(c['context']['initial_rows']))
        for index,record in enumerate(journal):
            require(record['sequence']==index and record['before']==state,c['case_id']+' discontinuous transition record')
            after=dict(state)
            if record['event']=='V':after['policy_state']='revoked'
            if record['new_rows']:
                require(record['new_rows']==1 and record['decision']=='P' and record['effect_dispatches']==1,c['case_id']+' invalid declared commit')
                after.update(rows=state['rows']+1,owner_sequence=state['owner_sequence']+1,
                             replay_state='spent',remaining_units=state['remaining_units']-1)
            require(after==record['after'],c['case_id']+' transition arithmetic')
            state=after
        require({k:state[k] for k in expected['final_state']}==expected['final_state'],c['case_id']+' final state annotation')
        if expected['outcome']=='EffectFailed':require(expected['outcome_reasons']==['SINK_FAILURE'],c['case_id']+' missing sink diagnostic')
    # Twenty-two schema-instance boundary probes, separate from the 60-case corpus.
    probes=[]
    def probe(label,kind,instance,want):
        observed=ExactValidator(schemas[kind]).is_valid(instance)
        require(observed==want,'schema probe '+label);probes.append(dict(name=label,valid=observed))
    request=json.loads(decode(cases[0]['request_bytes_base64']))
    response=json.loads(decode(cases[0]['response_bytes_base64']))
    for kind,base in [('request',request),('response',response)]:
        for field in ['request_id','model_ref']:
            bad=dict(base);bad[field]+='\n';probe(kind+' '+field+' newline',kind,bad,False)
        bad=dict(base);bad['extra']=0;probe(kind+' additional field',kind,bad,False)
        bad=dict(base);bad.pop('schema');probe(kind+' missing required',kind,bad,False)
        bad=dict(base);bad['evidence_refs']=[base['evidence_refs'][0]]*2;probe(kind+' duplicate refs',kind,bad,False)
        bad=dict(base);bad['evidence_refs']=[];probe(kind+' empty refs',kind,bad,False)
    for value,label,want in [(Decimal('1.0'),'1.0',True),(True,'Boolean',False),
                             (Decimal('1e-4000'),'tiny positive',False),(9,'out of range',False)]:
        bad=dict(request);bad['budget_units']=value;probe('budget '+label,'request',bad,want)
    for value,label,want in [('x'*512,'512 code points',True),('x'*513,'513 code points',False),
                             ('\U0001f50d'*512,'512 non-BMP code points',True)]:
        bad=dict(response);bad['rationale']=value;probe('rationale '+label,'response',bad,want)
    for value,label,want in [(Decimal('1e-4000'),'underflow fractional deadline',False),
                             (Decimal('20.0'),'integral decimal deadline',True)]:
        bad=dict(request);bad['deadline_tick']=value;probe(label,'request',bad,want)
    # Cross-message mismatch is intentionally structurally valid.
    bad=dict(response);bad['request_ref']='sha256:'+'f'*64;probe('wrong binding is schema-valid','response',bad,True)
    assignments=rows('guard-assignments.jsonl')
    require(len(assignments)==729,'guard table count')
    require([r['guards'] for r in assignments]==[''.join(v) for v in itertools.product('DUP',repeat=6)],'guard enumeration')
    for index,r in enumerate(assignments):
        require(r['index']==index,'guard index')
        g=r['guards']
        domain=('completed' if g[0]=='P' and g[5]!='U' else 'binding-prefix' if g=='DUUUUU' else 'abstract-only')
        require(r['domain']==domain,'guard domain classification')
        truth='P' if all(v=='P' for v in g) else 'D' if any(v=='D' for v in g) else 'U'
        require(truth==r['summary'],'guard table expected summary')
    require(Counter(r['summary'] for r in assignments)=={'D':665,'U':63,'P':1},'guard summary partition')
    require(Counter(r['domain'] for r in assignments)=={'completed':162,'binding-prefix':1,'abstract-only':566},'guard domain partition')
    plans=rows('event-permutations.jsonl')
    require({r['order'] for r in plans}=={''.join(v) for v in itertools.permutations('RVAB')},'event plan enumeration')
    for plan in plans:
        order=plan['order'];reachable=order.index('R')<order.index('A')<order.index('B')
        require(plan['reachable']==reachable,'event reachability')
        if not reachable:require(plan['executed_events']==[] and plan['new_rows'] is None,'invalid plan counted as executed')
        else:require(plan['new_rows']==int(order.index('A')<order.index('V')),'declared temporal row count')
    report=dict(scope='fixture integrity, raw-byte profiles, Draft 2020-12 structural validation and finite table consistency; NOT candidate enforcement or mutation execution',
                python=platform.python_version(),platform=platform.platform(),jsonschema=version('jsonschema'),
                meta_schemas_validated=4,schema_boundary_probes=len(probes),schema_probe_results=probes,
                corpus_cases=60,profiles=dict(sorted(profiles.items())),schema_results=dict(sorted(schema_results.items())),
                guard_assignments=729,guard_summary_counts={'D':665,'U':63,'P':1},
                guard_domains={'completed':162,'binding-prefix':1,'abstract-only':566},
                event_plans=24,reachable_event_plans=4,invalid_event_plans=20,
                binary_float_underflow_demo=dict(default_parser_schema_valid=Draft202012Validator(schemas['request']).is_valid(dict(request,deadline_tick=json.loads('1e-4000'))),exact_parser_schema_valid=ExactValidator(schemas['request']).is_valid(dict(request,deadline_tick=Decimal('1e-4000')))),
                authorization_annotations='design expectations; not executable semantic verification',
                candidate_runs=0,mutation_runs=0)
    if '--manifest' in sys.argv[1:]:
        manifest=json.loads((ROOT/'review/manifest.json').read_text(encoding='utf-8'))
        for name,entry in manifest['files'].items():
            raw=(ROOT/name).read_bytes()
            require(len(raw)==entry['bytes'] and sha(raw)==entry['sha256'],'manifest mismatch: '+name)
        report['manifest_files_verified']=len(manifest['files'])
    print(json.dumps(report,indent=2))

if __name__=='__main__':main()
