#!/usr/bin/env python3
"""Collect external JSON Schema outcomes; zero means collection, not conformity."""
import argparse
import hashlib
import json
from pathlib import Path
from jsonschema.validators import validator_for

ROOT = Path(__file__).resolve().parents[4]
DATA = Path(__file__).resolve().parent
SCHEMAS = {'t81.canonfs-import.v1':'spec/rfcs/RFC-00D1-canonfs-import-result-schema.json',
           't81.canonfs-export.v1':'spec/rfcs/RFC-00D1-canonfs-export-result-schema.json',
           't81.model-import.v1':'spec/rfcs/RFC-00D3-model-import-result-schema.json'}


def observations():
    receipt = json.loads((DATA/'cli-observations.json').read_text())
    results=[]
    for o in receipt['observations']:
        value=o['json_payload']
        if not isinstance(value,dict) or value.get('schema') not in SCHEMAS:continue
        path=SCHEMAS[value['schema']];raw=(ROOT/path).read_bytes();schema=json.loads(raw)
        validator=validator_for(schema);validator.check_schema(schema)
        errors=sorted(validator(schema).iter_errors(value),key=lambda e:(str(list(e.path)),e.message))
        results.append(dict(observation_id=o['id'],schema_path=path,
                            schema_sha256=hashlib.sha256(raw).hexdigest(),valid=not errors,
                            errors=[dict(path=list(e.path),validator=e.validator,message=e.message) for e in errors]))
    return results


def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--output',type=Path,required=True)
    args=p.parse_args()
    if args.output.exists():p.error('preserve prior outcomes: output must not exist')
    results=observations()
    args.output.write_text(json.dumps(dict(scope='External schema check of retained native documents; failures are preserved audit evidence',results=results),indent=2)+'\n')
    print(json.dumps(dict(checked=len(results),valid=sum(r['valid'] for r in results),invalid=sum(not r['valid'] for r in results))))


if __name__=='__main__':main()
