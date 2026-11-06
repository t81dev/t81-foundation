"""Protocol wire constants and canonical artifacts. No mathematical decoding."""
import hashlib
import json
import re
from pathlib import Path, PurePosixPath
import struct

HEADER = struct.Struct('<4sBBBBQ')
EXPECTED = struct.Struct('<QBBBBQQQQQBBBB32s')
OBSERVED = struct.Struct('<QBBBBQQI')
RECORDS = (50,332145,4630,8960,20,8960,2560,256,49012,580,778,83)
CALLS = (60,398574,5556,10752,24,10752,3072,256,49048,696,4662,101)
D0 = 0xcbf29ce484222325
TAGS = (1,0,4,1,2,3)
STATUS = {64:'PASS_CORRECTNESS_CORE',65:'FAIL_CORRECTNESS',66:'FAIL_GUARD',67:'FAIL_EVIDENCE',68:'FAIL_ORACLE',69:'BLOCKED_TOOLCHAIN',70:'BLOCKED_RESOURCE_ACCOUNTING',71:'INCONCLUSIVE_RESOURCE',72:'BLOCKED_AUTHORIZATION'}


def _json_domain(obj):
    if obj is None or type(obj) in (bool, str):
        return
    if type(obj) is int:
        if not 0 <= obj <= 2**53-1:
            raise ValueError('JSON integer outside exact domain')
        return
    if isinstance(obj, list):
        for value in obj:
            _json_domain(value)
        return
    if isinstance(obj, dict):
        if any(type(k) is not str or not k.isascii() for k in obj):
            raise ValueError('property names must be ASCII')
        for value in obj.values():
            _json_domain(value)
        return
    raise ValueError('noncanonical JSON type')


def canonical(obj):
    _json_domain(obj)
    return (json.dumps(obj, sort_keys=True, separators=(',', ':'), ensure_ascii=True, allow_nan=False)+'\n').encode('ascii')


def load_json(path):
    raw = Path(path).read_bytes()
    obj = json.loads(raw)
    if raw != canonical(obj):
        raise ValueError('noncanonical JSON')
    return obj


def write_json(path, obj):
    Path(path).write_bytes(canonical(obj))


def sha(path):
    h = hashlib.sha256()
    with open(path, 'rb') as stream:
        for chunk in iter(lambda: stream.read(65536), b''):
            h.update(chunk)
    return h.hexdigest()


def safe_path(value):
    if not isinstance(value, str) or not value.isascii() or '\\' in value:
        raise ValueError('unsafe artifact path')
    p = PurePosixPath(value)
    if not value or p.is_absolute() or any(x in ('', '.', '..') for x in value.split('/')):
        raise ValueError('unsafe artifact path')
    return value


def fold(digest, status, count):
    for byte in struct.pack('<BQ', status, count):
        digest = ((digest ^ byte)*0x100000001b3) & (2**64-1)
    return digest


def seal(directory):
    root = Path(directory)
    index = []
    for path in sorted(root.rglob('*')):
        if path.is_symlink():
            raise ValueError('symlink artifact')
        if path.is_file() and path.relative_to(root).as_posix() not in ('artifact-index.json', 'evidence.sha256'):
            name = safe_path(path.relative_to(root).as_posix())
            index.append({'path':name, 'bytes':path.stat().st_size, 'sha256':sha(path)})
    index.sort(key=lambda x:x['path'])
    write_json(root/'artifact-index.json', index)
    (root/'evidence.sha256').write_text(sha(root/'artifact-index.json')+'\n', encoding='ascii')


def verify_index(directory):
    root = Path(directory)
    index = load_json(root/'artifact-index.json')
    if not isinstance(index, list):
        raise ValueError('index array required')
    names = []
    for row in index:
        if set(row) != {'path','bytes','sha256'}:
            raise ValueError('index keys')
        name = safe_path(row['path'])
        path = root/name
        if type(row['bytes']) is not int or row['bytes']<0 or not isinstance(row['sha256'],str) or not re.fullmatch('[0-9a-f]{64}',row['sha256']):
            raise ValueError('index scalar types')
        if path.is_symlink() or not path.is_file() or path.stat().st_size != row['bytes'] or sha(path) != row['sha256']:
            raise ValueError('artifact identity')
        names.append(name)
    if names != sorted(set(names)):
        raise ValueError('index order/duplicates')
    actual = sorted(p.relative_to(root).as_posix() for p in root.rglob('*') if p.is_file() and p.relative_to(root).as_posix() not in ('artifact-index.json','evidence.sha256'))
    if actual != names or (root/'evidence.sha256').read_bytes() != (sha(root/'artifact-index.json')+'\n').encode():
        raise ValueError('index completeness/root')
    return set(names)


def manifest_template(protocol_sha, proposal_sha, implementation):
    return dict(schema='t81.arch002.correctness-evidence.v1', protocol_revision='T81-ARCH-002-R1',
                protocol_source_revision='1454bd63426d21686cfb5ef22dbaebc3c9cae94b', implementation_source_revision=implementation,
                protocol_sha256=protocol_sha, proposal_sha256=proposal_sha,
                authorization_receipt=None, host_receipt=None, binary_receipt=None, failure_receipt=None,
                build_receipts=[None]*3, status_id=72,status=STATUS[72],complete=False,expectations_sealed=False,
                expected_records=408034,calls_per_profile=483553,expected_file=None,guard_expected_file=None,
                actual_expected_records=0,observed_files=[None]*3,actual_calls=[0]*3,
                final_digests=[[f'{D0:016x}']*6 for _ in range(3)], oracle_receipts=[None]*6,
                guard_receipts=[None]*6,sanitizer_probe_receipts=[None]*2,
                resources=dict(rss_peak_bound_bytes=None,elapsed_ns=None,retained_bytes=None,accounting_complete=False))


def validate_manifest(obj, directory, complete=False):
    template = manifest_template('', '', '')
    if set(obj) != set(template) or obj['schema'] != template['schema'] or obj['protocol_revision'] != template['protocol_revision']:
        raise ValueError('campaign schema')
    for key in ('protocol_source_revision','implementation_source_revision'):
        if not isinstance(obj[key],str) or not re.fullmatch('[0-9a-f]{40}',obj[key]):
            raise ValueError('revision')
    for key in ('protocol_sha256','proposal_sha256'):
        if not isinstance(obj[key],str) or not re.fullmatch('[0-9a-f]{64}',obj[key]):
            raise ValueError('source hash')
    if type(obj['status_id']) is not int or obj['status'] != STATUS.get(obj['status_id']):
        raise ValueError('campaign status')
    if type(obj['complete']) is not bool or obj['complete'] != (obj['status_id']==64) or type(obj['expectations_sealed']) is not bool:
        raise ValueError('campaign completeness')
    if obj['expected_records'] != 408034 or obj['calls_per_profile'] != 483553:
        raise ValueError('planned cardinalities')
    for name,size in [('build_receipts',3),('observed_files',3),('actual_calls',3),('final_digests',3),('oracle_receipts',6),('guard_receipts',6),('sanitizer_probe_receipts',2)]:
        if not isinstance(obj[name],list) or len(obj[name])!=size:
            raise ValueError('campaign slots')
    for row in obj['final_digests']:
        if not isinstance(row,list) or len(row)!=6 or any(not isinstance(v,str) or not re.fullmatch('[0-9a-f]{16}',v) for v in row):
            raise ValueError('digest schema')
    for value in [obj['actual_expected_records'],*obj['actual_calls']]:
        if type(value) is not int or not 0<=value<=2**32-1:
            raise ValueError('actual count')
    resources=obj['resources']
    if set(resources)!=set(template['resources']) or type(resources['accounting_complete']) is not bool:
        raise ValueError('resources schema')
    for key in ('rss_peak_bound_bytes','elapsed_ns','retained_bytes'):
        if resources[key] is not None and (type(resources[key]) is not int or not 0<=resources[key]<=2**53-1):
            raise ValueError('resource integer')
    scalar_paths=('authorization_receipt','host_receipt','binary_receipt','failure_receipt','expected_file','guard_expected_file')
    paths=[obj[k] for k in scalar_paths]
    paths += [p for k in ('build_receipts','observed_files','oracle_receipts','guard_receipts','sanitizer_probe_receipts') for p in obj[k]]
    for p in paths:
        if p is not None:
            safe_path(p)
            if not (Path(directory)/p).is_file():
                raise ValueError('missing receipt')
    if obj['expected_file'] not in (None,'expected.bin') or obj['guard_expected_file'] not in (None,'guard-expected.bin'):
        raise ValueError('fixed filenames')
    if complete or obj['complete']:
        if not obj['complete'] or not obj['expectations_sealed'] or obj['actual_expected_records']!=408034 or obj['actual_calls']!=[483553]*3:
            raise ValueError('incomplete matrix')
        required=[obj[k] for k in scalar_paths if k!='failure_receipt'] + [p for k in ('build_receipts','observed_files','oracle_receipts','guard_receipts','sanitizer_probe_receipts') for p in obj[k]]
        if any(p is None for p in required):
            raise ValueError('incomplete receipts')
        if any(resources[k] is None for k in resources) or not resources['accounting_complete']:
            raise ValueError('incomplete accounting')
        if resources['rss_peak_bound_bytes']>512*2**20 or resources['elapsed_ns']>1800*10**9 or resources['retained_bytes']>100*2**20:
            raise ValueError('resource cap')
    return obj
