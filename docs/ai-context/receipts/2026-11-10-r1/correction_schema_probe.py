import copy, importlib.util, json, sys
from pathlib import Path
from jsonschema import Draft202012Validator
root=Path(sys.argv[1]) if len(sys.argv)>1 else Path('/private/tmp/t81-scg08-correction')
spec=importlib.util.spec_from_file_location('validator', root/'scripts/ci/validate_scg08_gate1_evidence.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
e=json.loads((root/'experiments/scg08_arithmetic_work/gate1_evidence_record.json').read_text())
s=json.loads((root/'experiments/scg08_arithmetic_work/evidence_schema.json').read_text())
for label,change in [('null operand_trits',lambda x:x['tier2_work']['evaluated_bands'][0].__setitem__('operand_trits',None)),('string ternary_limbs_op',lambda x:x['tier2_work']['evaluated_bands'][0].__setitem__('ternary_limbs_op','two'))]:
 x=copy.deepcopy(e);change(x)
 try:m.validate_dict(x,s);result='ACCEPTED'
 except (SystemExit,Exception):result='REJECTED'
 errors=list(Draft202012Validator(s).iter_errors(x))
 print(label+': candidate validator '+result+'; Draft202012Validator errors='+str(len(errors)))
 assert result=='ACCEPTED' and errors
print('Evidence revision identity:',e['git_context'])
print('No arithmetic kernels, smoke suite, Markov simulation, benchmarks or Gate 2 executed.')
