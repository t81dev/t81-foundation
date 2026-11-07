"""Three-state all-required-guards meet; independent candidate fact evaluation."""
import base64
import json
from wire import observations,formatted,identity

TABLE={('D','D'):'D',('D','U'):'D',('D','P'):'D',
       ('U','D'):'D',('U','U'):'U',('U','P'):'U',
       ('P','D'):'D',('P','U'):'U',('P','P'):'P'}

def compose(values,mutant=None):
    if not values:raise ValueError("operational guard collection cannot be empty")
    result='P'
    for value in values:result=TABLE[result,value]
    return 'P' if mutant=='permit_unknown' and result=='U' else result

def evaluate(case,context,catalog,mutant=None):
    request_bytes,response_bytes,r,s,meta=observations(case)
    pin=identity('request',request_bytes)
    def stop(stage,code,interpretation=None):return formatted(meta,stage,'D',[code],pin,interpretation)
    for kind,prefix in [('request','Request')]:
        review=meta['wire_review'][kind]
        if review['profile']!='ok':return stop(prefix+'Profile','INVALID_REQUEST')
        if review['schema']!='valid':return stop(prefix+'Schema','INVALID_REQUEST')
    bindings=[(pin,context['expected_request_ref']),(r['model_ref'],context['approved_model_ref']),
              (r['artifact_ref'],context['expected_artifact_ref']),(r['policy_ref'],context['expected_policy_ref']),
              (r['deadline_tick'],context['deadline_tick']),
              (r['evidence_refs'],[x['ref'] for x in context['required_evidence']]),
              (r['evidence_refs'],sorted(r['evidence_refs']))]
    if any(left!=right for left,right in bindings):return stop('RequestBinding','BINDING')
    if case['response_event'] in ('Timeout','Cancelled','TransportFailure'):
        if response_bytes is not None or case['output_ref'] is not None:raise ValueError('failure has response')
        return stop('ModelFailure','MODEL_FAILURE')
    if response_bytes is None:raise ValueError('missing Response')
    review=meta['wire_review']['response']
    if review['profile']!='ok':return stop('ResponseProfile','INVALID_RESPONSE')
    if review['schema']!='valid':return stop('ResponseSchema','INVALID_RESPONSE')
    interpretation=s['recommendation']
    bind=[(s['request_id'],r['request_id']),(s['request_ref'],pin),(s['model_ref'],r['model_ref']),
          (s['evidence_refs'],r['evidence_refs']),(context['observed_provider'],context['approved_provider'])]
    if mutant!='omit_model_binding':bind.append((context['observed_model_ref'],context['approved_model_ref']))
    if any(a!=b for a,b in bind):return stop('ResponseBinding','BINDING',interpretation)
    states=['P']*6
    evidence_observations=[]
    diagnostics=[]
    def fact(index,state,code,subject=pin):
        states[index]=TABLE[states[index],state]
        diagnostics.append(dict(code=code,subject=subject))
    if interpretation=='reject':fact(1,'D','MODEL_REJECT')
    if interpretation=='review':fact(1,'U','MODEL_REVIEW')
    for expected in context['required_evidence']:
        target=expected['ref']
        if target not in context['registry']:
            evidence_observations.append(dict(ref=target,validity='missing',pair=None))
            fact(2,'U','EVIDENCE_MISSING',target);continue
        item=catalog[context['registry'][target]]
        content=base64.b64decode(item['bytes_base64'],validate=True)
        observed=json.loads(content)
        checks=[identity('evidence',content)==target]
        checks.extend(observed.get(k)==expected[k] for k in ['artifact_ref','predicate','source','epoch'])
        checks.extend(type(observed.get(k)) is bool for k in ['supports','refutes'])
        evidence_observations.append(dict(ref=target,validity=all(checks),pair=[observed['supports'],observed['refutes']],observed_ref=identity('evidence',content)))
        if not all(checks):fact(2,'D','EVIDENCE_INVALID',target);continue
        status={(False,False):('U','EVIDENCE_MISSING'),(False,True):('D','EVIDENCE_REFUTED'),(True,True):('D','EVIDENCE_CONFLICT')}.get((observed['supports'],observed['refutes']))
        if status:fact(2,*status,target)
    required=dict(zip(['caller','model_ref','operation','artifact_ref','policy_ref'],
                      [context['caller'],r['model_ref'],'append_report',r['artifact_ref'],r['policy_ref']]))
    if context['grant']!=required or not context['policy_present']:fact(3,'D','CAPABILITY')
    if context['policy_present']:
        policy=context['policy_state']
        if policy=='revoked':fact(3,'D','POLICY_REVOKED')
        if policy=='unavailable':fact(3,'U','POLICY_UNAVAILABLE')
    if context['replay_state']=='spent':fact(4,'D','REPLAY')
    if context['current_policy_ref']!=r['policy_ref']:fact(4,'D','STALE_POLICY')
    if context['replay_state']=='unavailable':fact(4,'U','REPLAY_UNAVAILABLE')
    if min(context['remaining_units'],r['budget_units'])<1:fact(5,'D','BUDGET')
    if not context['now_tick']<=r['deadline_tick']:fact(5,'D','DEADLINE')
    result=formatted(meta,'Guards',compose(states,mutant),[x['code'] for x in diagnostics],pin,
                     interpretation,states,diagnostics)

    result['evidence_observations']=evidence_observations
    return result
