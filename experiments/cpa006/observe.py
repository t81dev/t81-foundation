"""Raw evidence parsing and exact comparison; no evaluator/oracle logic."""
import copy,hashlib,json

def parse(data,blob,hosted=False):
 lines=data.splitlines();identity=b'CPA006_ARTIFACT_HEX='+blob.hex().encode()
 if [x for x in lines if x.startswith(b'CPA006_ARTIFACT_HEX=')]!=[identity]:raise RuntimeError('actual input identity')
 frames=[json.loads(x.split(b'=',1)[1]) for x in lines if x.startswith(b'CPA006_OBSERVATION=')]
 terminal=[json.loads(x.split(b'=',1)[1]) for x in lines if x.startswith(b'CPA006_RESULT=')]
 if len(terminal)!=1 or lines.count(b'CPA006_DONE')!=1:raise RuntimeError('missing/duplicate terminal record')
 if hosted:
  for f in frames:
   s=f['state'];v=s.pop('memory_values');t=s.pop('memory_tags')
   if len(v)!=len(t) or not (len(s['register_values'])==len(s['register_tags'])==81):raise RuntimeError('raw shape')
   s['register_values']=list(map(str,s['register_values']))
   s['memory']=dict(word_count=len(v),default_value='0',default_tag=0,overrides=[dict(address=i,value=str(x),tag=y) for i,(x,y) in enumerate(zip(v,t)) if x or y])
   f.update(schema='t81.cpa006.observation.v1',program_sha256=hashlib.sha256(blob).hexdigest())
 return dict(terminal=terminal[0],observations=frames)

def compare(actual,expected):
 if actual!=expected:
  errors=[]
  if actual['terminal']!=expected['terminal']:errors.append('terminal')
  for i,(a,b) in enumerate(zip(actual['observations'],expected['observations'])):
   if a!=b:errors.append('sequence '+str(i))
  if len(actual['observations'])!=len(expected['observations']):errors.append('observation count')
  raise RuntimeError('mismatch: '+', '.join(errors))
