"""Conventional typed Boolean facts and explicit binary-encoded result enum."""
from dataclasses import dataclass
from enum import IntEnum
import base64
import json
from wire import observations,formatted,identity

class Decision(IntEnum):
    DENY=0
    DEFER=1
    PERMIT=2
LABEL={Decision.DENY:'D',Decision.DEFER:'U',Decision.PERMIT:'P'}
HARD={'MODEL_REJECT','EVIDENCE_REFUTED','EVIDENCE_CONFLICT','EVIDENCE_INVALID','CAPABILITY','POLICY_REVOKED','REPLAY','STALE_POLICY','BUDGET','DEADLINE'}

@dataclass(frozen=True)
class Check:
    positive:bool
    blocked:bool
    diagnostics:tuple
    @property
    def decision(self):return Decision.DENY if self.blocked else Decision.PERMIT if self.positive else Decision.DEFER

def compose(values,mutant=None):
    if not values:raise ValueError("operational guard collection cannot be empty")
    decision='D' if any(v=='D' for v in values) else 'P' if all(v=='P' for v in values) else 'U'
    return 'P' if mutant=='permit_unknown' and decision=='U' else decision

def evaluate(case,ctx,objects,mutant=None):
    raw,out,request,response,meta=observations(case)
    subject=identity('request',raw)
    def reject(stage,code,interpretation=None):return formatted(meta,stage,'D',[code],subject,interpretation)
    rw,ow=meta['wire_review']['request'],meta['wire_review']['response']
    if rw['profile']!='ok':return reject('RequestProfile','INVALID_REQUEST')
    if rw['schema']!='valid':return reject('RequestSchema','INVALID_REQUEST')
    intent=(subject==ctx['expected_request_ref'] and request['model_ref']==ctx['approved_model_ref']
            and request['artifact_ref']==ctx['expected_artifact_ref'] and request['policy_ref']==ctx['expected_policy_ref']
            and request['deadline_tick']==ctx['deadline_tick'] and request['evidence_refs']==sorted(request['evidence_refs'])
            and request['evidence_refs']==[v['ref'] for v in ctx['required_evidence']])
    if not intent:return reject('RequestBinding','BINDING')
    if case['response_event']!='Response':
        if out is not None or case['output_ref'] is not None:raise ValueError('inconsistent failure metadata')
        return reject('ModelFailure','MODEL_FAILURE')
    if out is None:raise ValueError('Response requires bytes')
    if ow['profile']!='ok':return reject('ResponseProfile','INVALID_RESPONSE')
    if ow['schema']!='valid':return reject('ResponseSchema','INVALID_RESPONSE')
    interpretation=response['recommendation']
    echo=all(response[k]==v for k,v in [('request_id',request['request_id']),('request_ref',subject),('model_ref',request['model_ref']),('evidence_refs',request['evidence_refs'])])
    provider=(ctx['observed_provider']==ctx['approved_provider'] and (mutant=='omit_model_binding' or ctx['observed_model_ref']==ctx['approved_model_ref']))
    if not echo or not provider:return reject('ResponseBinding','BINDING',interpretation)
    groups=[[],[],[],[],[]]
    observed=[]
    def add(group,code,ref=subject):groups[group].append(dict(code=code,subject=ref))
    if interpretation=='reject':add(0,'MODEL_REJECT')
    elif interpretation=='review':add(0,'MODEL_REVIEW')
    for need in ctx['required_evidence']:
        alias=ctx['registry'].get(need['ref'])
        if alias is None:
            observed.append(dict(ref=need['ref'],validity='missing',pair=None))
            add(1,'EVIDENCE_MISSING',need['ref']);continue
        blob=base64.b64decode(objects[alias]['bytes_base64'],validate=True)
        evidence=json.loads(blob)
        valid=(identity('evidence',blob)==need['ref'] and all(evidence[k]==need[k] for k in ('artifact_ref','predicate','source','epoch'))
               and type(evidence.get('supports')) is bool and type(evidence.get('refutes')) is bool)
        observed.append(dict(ref=need['ref'],validity=valid,pair=[evidence['supports'],evidence['refutes']],observed_ref=identity('evidence',blob)))
        if not valid:add(1,'EVIDENCE_INVALID',need['ref'])
        elif evidence['supports'] and evidence['refutes']:add(1,'EVIDENCE_CONFLICT',need['ref'])
        elif evidence['refutes']:add(1,'EVIDENCE_REFUTED',need['ref'])
        elif not evidence['supports']:add(1,'EVIDENCE_MISSING',need['ref'])
    expected_grant=dict(caller=ctx['caller'],model_ref=request['model_ref'],operation='append_report',artifact_ref=request['artifact_ref'],policy_ref=request['policy_ref'])
    if not ctx['policy_present'] or ctx['grant']!=expected_grant:add(2,'CAPABILITY')
    if ctx['policy_present'] and ctx['policy_state']=='revoked':add(2,'POLICY_REVOKED')
    if ctx['policy_present'] and ctx['policy_state']=='unavailable':add(2,'POLICY_UNAVAILABLE')
    if ctx['replay_state']=='spent':add(3,'REPLAY')
    if request['policy_ref']!=ctx['current_policy_ref']:add(3,'STALE_POLICY')
    if ctx['replay_state']=='unavailable':add(3,'REPLAY_UNAVAILABLE')
    if min(request['budget_units'],ctx['remaining_units'])<1:add(4,'BUDGET')
    if ctx['now_tick']>request['deadline_tick']:add(4,'DEADLINE')
    checks=[Check(not g,any(d['code'] in HARD for d in g),tuple(g)) for g in groups]
    # Boolean permissions, not a meet table, determine the conventional result.
    permit=all(c.positive for c in checks)
    decision=Decision.PERMIT if permit else Decision.DENY if any(c.blocked for c in checks) else Decision.DEFER
    label=LABEL[decision]
    if mutant=='permit_unknown' and label=='U':label='P'
    diagnostics=[d for check in checks for d in check.diagnostics]
    result=formatted(meta,'Guards',label,[d['code'] for d in diagnostics],subject,interpretation,
                     ['P']+[LABEL[c.decision] for c in checks],diagnostics)

    result['evidence_observations']=observed
    return result
