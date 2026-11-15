"""Shared candidate wire/schema layer only; the Ruby oracle imports none of it."""
from decimal import Decimal
import base64
import hashlib
import json
from pathlib import Path
from jsonschema import Draft202012Validator, validators

PACKET=Path(__file__).resolve().parents[2]/'docs/research/model-computational-object'
SCHEMAS={k:json.loads((PACKET/'schemas'/f'{k}.json').read_text()) for k in ('request','response')}

def identity(kind,raw):return 'sha256:'+hashlib.sha256(kind.encode()+b'\0'+raw).hexdigest()
def canonical(value):return json.dumps(value,sort_keys=True,ensure_ascii=True,separators=(',',':')).encode()
def exact_integer(checker,value):
    return (type(value) is int or isinstance(value,Decimal) and value.is_finite() and value==value.to_integral_value())
Validator=validators.extend(Draft202012Validator,type_checker=Draft202012Validator.TYPE_CHECKER.redefine('integer',exact_integer))

class WireError(ValueError):pass

def inspect(raw,kind):
    if raw is None:return None,dict(profile='not_observed',schema='not_run',schema_errors=[])
    def fail(reason):raise WireError(reason)
    def pairs(items):
        result={}
        for key,value in items:
            if key in result:fail('duplicate_key')
            result[key]=value
        return result
    def visit(value):
        if isinstance(value,str):
            if any(0xd800<=ord(c)<=0xdfff for c in value):fail('unicode_scalar')
            return 0
        if isinstance(value,dict):
            for key in value:visit(key)
            return 1+max((visit(v) for v in value.values()),default=0)
        if isinstance(value,list):return 1+max((visit(v) for v in value),default=0)
        return 0
    try:
        if len(raw)>(8192 if kind=='request' else 4096):fail('byte_limit')
        try:text=raw.decode('utf-8',errors='strict')
        except UnicodeDecodeError:fail('utf8')
        if text.startswith('\ufeff'):fail('bom')
        try:value=json.loads(text,parse_int=Decimal,parse_float=Decimal,parse_constant=lambda x:fail('json_syntax'),object_pairs_hook=pairs)
        except WireError:raise
        except (ValueError,ArithmeticError):fail('json_syntax')
        if visit(value)>4:fail('depth_limit')
        errors=[dict(keyword=e.validator,path=''.join('/'+str(p) for p in e.absolute_path)) for e in Validator(SCHEMAS[kind]).iter_errors(value)]
        return value,dict(profile='ok',schema='invalid' if errors else 'valid',schema_errors=errors)
    except WireError as e:return None,dict(profile=str(e),schema='not_run',schema_errors=[])

def observations(case):
    request=base64.b64decode(case['request_bytes_base64'],validate=True)
    output=None if case['response_bytes_base64'] is None else base64.b64decode(case['response_bytes_base64'],validate=True)
    req,rw=inspect(request,'request');res,ow=inspect(output,'response')
    stored=res['recommendation'] if ow['profile']=='ok' and ow['schema']=='valid' else None
    return request,output,req,res,dict(wire_review=dict(request=rw,response=ow),stored_interpretation=stored)

def formatted(meta,stage,decision,reasons,subject,interpretation=None,guards=None,diagnostics=None):
    return dict(**meta,pipeline_stage=stage,interpretation=interpretation,guards=guards,
                decision=decision,reasons=reasons,evidence_observations=[],diagnostics=diagnostics if diagnostics is not None else [dict(code=x,subject=subject) for x in reasons])
