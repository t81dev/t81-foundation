#!/usr/bin/env python3
"""Honest single-owner harness; the sole permitted effect is list.append.

The two decision modules run in separate invocations. This owner and the wire
reader are shared controls/TCB, not evidence of independently implemented sinks.
"""
import argparse
import base64
from copy import deepcopy
import hashlib
import importlib
import json
from pathlib import Path
from wire import PACKET,identity,canonical

ROOT=Path(__file__).resolve().parent

def verifier_ref(arm):
    files={name:hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in ['wire.py',f'arm_{arm.lower()}.py','driver.py']}
    return identity('artifact',canonical(files))

def valid_order(events,full=False):
    if any(e not in 'RVAB' for e in events) or len(set(events))!=len(events):return False
    if full and set(events)!=set('RVAB'):return False
    if 'A' in events and ('R' not in events or events.index('R')>events.index('A')):return False
    if 'B' in events and ('A' not in events or events.index('A')>events.index('B')):return False
    return True

class Owner:
    def __init__(self,case,arm,objects,mutant=None):
        self.case=case;self.arm=arm;self.objects=objects;self.mutant=mutant
        self.impl=importlib.import_module('arm_'+arm.lower())
        self.context=deepcopy(case['context'])
        self.rows=deepcopy(self.context['initial_rows'])
        self.sequence=len(self.rows)
        self.journal=[];self.receipts=[];self.admissions=[]
        self.verifier=verifier_ref(arm)
    def snapshot(self):
        return dict(policy_state=self.context['policy_state'],replay_state=self.context['replay_state'],
                    remaining_units=self.context['remaining_units'],rows=len(self.rows),owner_sequence=self.sequence)
    def evaluate(self,final=False):
        context=deepcopy(self.context)
        if final and self.mutant=='skip_revocation':context['policy_state']=self.cached_policy
        if final and self.mutant=='skip_replay':context['replay_state']='fresh'
        return self.impl.evaluate(self.case,context,self.objects,self.mutant)
    def run(self):
        initial=self.evaluate();before=self.snapshot()
        self.cached_policy=self.context['policy_state']
        self.journal.append(dict(sequence=0,event='Decision',decision=initial['decision'],reasons=initial['reasons'],before=before,after=before,effect_dispatches=0,new_rows=0))
        for event in self.case['events']:
            before=self.snapshot();result=None;dispatches=0;added=0
            if event=='V':self.context['policy_state']='revoked'
            else:
                result=self.evaluate(final=event in ('A','B'))
                if event=='R':self.cached_policy=self.context['policy_state']
                elif event in ('A','B'):
                    if result['decision']=='P':
                        dispatches=1
                        if not self.context['sink_fault']:
                            added=1
                            after=dict(before,rows=len(self.rows)+1,owner_sequence=self.sequence+1,
                                       remaining_units=self.context['remaining_units']-1,replay_state='spent')
                            record=dict(sequence=len(self.journal),event=event,decision=result['decision'],reasons=result['reasons'],before=before,after=after,effect_dispatches=1,new_rows=1)
                            prefix=dict(case_id=self.case['case_id'],fixture_sha256=self.case['fixture_sha256'],arm=self.arm,verifier_ref=self.verifier,journal=self.journal+[record])
                            request=json.loads(base64.b64decode(self.case['request_bytes_base64']))
                            row=dict(request_ref=identity('request',base64.b64decode(self.case['request_bytes_base64'])),
                                     artifact_ref=request['artifact_ref'],model_ref=request['model_ref'],policy_ref=request['policy_ref'],
                                     output_ref=identity('output',base64.b64decode(self.case['response_bytes_base64'])),
                                     verifier_ref=self.verifier,execution_ref=identity('execution',canonical(prefix)),
                                     decision='P',outcome='Committed',owner_sequence=self.sequence+1)
                            # This list belongs to the honest owner, not the model.
                            self.rows.append(row)
                            self.sequence+=1;self.context['remaining_units']-=1;self.context['replay_state']='spent'
                            self.receipts.append(dict(row=row,receipt_ref=identity('receipt',canonical(row))))
                    self.admissions.append(dict(event=event,decision=result['decision'],reasons=result['reasons'],effect_dispatches=dispatches,new_rows=added))
            self.journal.append(dict(sequence=len(self.journal),event=event,decision=None if result is None else result['decision'],
                                     reasons=[] if result is None else result['reasons'],before=before,after=self.snapshot(),effect_dispatches=dispatches,new_rows=added))
        primary=next((a for a in self.admissions if a['event']=='A'),None)
        final=primary['decision'] if primary else initial['decision']
        reasons=primary['reasons'] if primary else initial['reasons']
        outcome=('Committed' if primary and primary['new_rows'] else 'EffectFailed' if primary and primary['effect_dispatches'] else 'Deferred' if final=='U' else 'Rejected')
        summary=dict(**initial,final_decision=final,final_reasons=reasons,admissions=self.admissions,
                     effect_dispatches=sum(x['effect_dispatches'] for x in self.admissions),new_rows=len(self.rows)-len(self.case['context']['initial_rows']),
                     final_state={k:v for k,v in self.snapshot().items() if k!='owner_sequence'},outcome=outcome,
                     outcome_reasons=['SINK_FAILURE'] if outcome=='EffectFailed' else reasons if outcome in ('Rejected','Deferred') else [],
                     failure_subtag=self.case['response_event'] if self.case['response_event']!='Response' else None,
                     transition_expectations=self.journal)
        return dict(case_id=self.case['case_id'],arm=self.arm,mutant=self.mutant,summary=summary,
                    journal=self.journal,ledger_rows=self.rows,receipts=self.receipts,verifier_ref=self.verifier,
                    input_context=deepcopy(self.case['context']),request_ref=identity('request',base64.b64decode(self.case['request_bytes_base64'])),
                    output_ref=None if self.case['response_bytes_base64'] is None else identity('output',base64.b64decode(self.case['response_bytes_base64'])))

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--arm',choices=['A','B'],required=True)
    parser.add_argument('--mutant',choices=['permit_unknown','skip_revocation','skip_replay','omit_model_binding'])
    parser.add_argument('--proposal-trace',type=Path)
    args=parser.parse_args()
    module=importlib.import_module('arm_'+args.arm.lower())
    objects=json.loads((PACKET/'corpus/objects.json').read_text())
    cases=[json.loads(x) for x in (PACKET/'corpus/cases.jsonl').read_text().splitlines()]
    observed=[]
    for original in cases:
        case={k:v for k,v in original.items() if k!='expected'}
        if not valid_order(case['events']):raise ValueError('invalid preregistered case event order')
        observed.append(Owner(case,args.arm,objects,args.mutant).run())
    base={k:v for k,v in cases[0].items() if k!='expected'}
    plans=[]
    for order in __import__('itertools').permutations('RVAB'):
        if not valid_order(order,full=True):
            plans.append(dict(order=''.join(order),reachable=False,classification='INVALID_ORDER',executed_events=[],new_rows=None))
            continue
        c=dict(base,events=list(order))
        record=Owner(c,args.arm,objects,args.mutant).run()
        plans.append(dict(order=''.join(order),reachable=True,classification='EXPECTED_TRANSITIONS',
                          executed_events=[dict(row,sequence=i) for i,row in enumerate(record['journal'][1:])],new_rows=record['summary']['new_rows'],
                          final_state=record['summary']['final_state'],observation=record))
    states=__import__('itertools')
    guards=[dict(guards=''.join(v),summary=module.compose(v,args.mutant)) for v in states.product('DUP',repeat=6)]
    pairs=[dict(values=''.join(v),summary=module.compose(v,args.mutant)) for v in states.product('DUP',repeat=2)]
    triples=[dict(values=''.join(v),left=module.compose([module.compose(v[:2],args.mutant),v[2]],args.mutant),
                  right=module.compose([v[0],module.compose(v[1:],args.mutant)],args.mutant)) for v in states.product('DUP',repeat=3)]
    choices=[]
    if args.proposal_trace:
        trace=json.loads(args.proposal_trace.read_text())
        catalog={c['case_id']:c for c in cases}
        for case_id in trace['choices']:
            if case_id not in ('F01-C01','F01-C02','F01-C03'):raise ValueError('unexpected proposal fixture')
            choices.append(Owner({k:v for k,v in catalog[case_id].items() if k!='expected'},args.arm,objects,args.mutant).run())
    print(json.dumps(dict(arm=args.arm,mutant=args.mutant,cases=observed,plans=plans,guards=guards,pairs=pairs,triples=triples,proposal_choices=choices),sort_keys=True,indent=2))

if __name__=='__main__':main()
