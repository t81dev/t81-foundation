"""Documentation-vector and preservation checks, not a runtime admission test.
Run: python3 docs/records/decisions/2026-11-14-artifact-admission/evidence/validate-package.py
"""
from pathlib import Path
import hashlib
import json
import re
import struct
import subprocess

p = Path('docs/records/decisions/2026-11-14-artifact-admission')
prior = '07968a9a307415bf8a7aa6f04056caa22db01c78'
history = Path('docs/releases/drafts/2026-11-14-opcode-boundary')
canonical = lambda d: json.dumps(d,sort_keys=True,separators=(',',':'),ensure_ascii=True).encode('ascii')
ids = json.loads((p/'identities.json').read_bytes())
b = (p/'execution-descriptor.json').read_bytes()
assert b == canonical(json.loads(b))
assert hashlib.sha256(b).hexdigest() == ids['descriptor_sha256']
assert ids['execution_profile'] == 'hosted-sha256-'+ids['descriptor_sha256']
assert hashlib.sha256(canonical(json.loads((p/'caps.json').read_bytes()))).hexdigest() == ids['resource_policy_sha256']
e = bytes.fromhex((p/'evidence/envelope-vector.hex').read_text())
h = bytearray(e[:64])
assert h[:8] == b'T81PROG\0'
version, header_size, flags, m, n = struct.unpack_from('<HHIIQ',h,8)
assert (version,header_size,flags)==(1,64,0) and 0<m<=1024 and 0<n<=8388608
assert h[60:64] == b'\0'*4 and len(e)==64+m+n
manifest=e[64:64+m]
assert manifest==(p/'evidence/manifest-vector.json').read_bytes()==canonical(json.loads(manifest))
d = json.loads(manifest)
assert list(d)==['execution_profile','execution_profile_digest','payload_codec','producer_revision']
assert d['execution_profile']==ids['execution_profile'] and d['execution_profile_digest']==ids['descriptor_sha256']
assert d['payload_codec']=='native-program-v0-le64-i32-f64'
assert re.fullmatch('[0-9a-f]{40}',d['producer_revision'])
digest=bytes(h[28:60]);h[28:60]=bytes(32)
assert hashlib.sha256(h+e[64:]).digest()==digest
assert e[64+m:] == (history/'evidence/main.tisc').read_bytes()
expected=json.loads((p/'evidence/vector-expectations.json').read_bytes())
assert hashlib.sha256(e).hexdigest()==expected['whole_envelope_sha256']
for manifest_path, relative in [(history/'SHA256SUMS',True),(history/'revision-2/SHA256SUMS',False)]:
    for line in manifest_path.read_text().splitlines():
        sha,name=line.split('  ',1);f=history/name if relative else Path(name)
        assert hashlib.sha256(f.read_bytes()).hexdigest()==sha, str(f)
old_paths=subprocess.check_output(['git','ls-tree','-r','--name-only',prior,str(history),'docs/architecture/adr/ADR-PROPOSED-artifact-admission-boundary.md'],text=True).splitlines()
for name in old_paths:
    assert Path(name).read_bytes()==subprocess.check_output(['git','show',prior+':'+name]),name
rfc=Path('spec/rfcs/RFC-00D6-versioned-tisc-state-transition-contract.md')
old=subprocess.check_output(['git','show',prior+':'+str(rfc)],text=True)
new=rfc.read_text()
for i in range(1,9):
    prefix=f'| **DM-{i:02}**'
    assert [x for x in old.splitlines() if x.startswith(prefix)]==[x for x in new.splitlines() if x.startswith(prefix)]
assert old.split('MAINTAINER AUTHORIZATION RECORD')[1].split('```')[0] == new.split('MAINTAINER AUTHORIZATION RECORD')[1].split('```')[0]
assert subprocess.run(['git','diff','--quiet',prior,'--','include','isa','vm','tools','tests','cmake','CMakeLists.txt','.github'],check=False).returncode==0
for f in p.rglob('*.json'):json.loads(f.read_bytes())
for f in [p/'README.md',Path('docs/architecture/adr/ADR-0007-saved-program-admission-boundary.md')]:
    for target in re.findall(r'\]\(([^)]+)\)',f.read_text()):
        if '://' not in target and not target.startswith('#'):
            assert (f.parent/target.split('#')[0]).exists(),(str(f),target)
print(json.dumps({'result':'PASS: documentation vectors and preservation only','historical_files_unchanged':len(old_paths),'DM01_DM08_and_signature_unchanged':True,'runtime_workflow_golden_changes':False,'new_runtime_or_VM_verification':False},indent=2))
