"""Partial r1 experiment-local verification. Production execution is disabled.
Test fixtures/signatures never become eligible measurement evidence.
No key provisioning, daemon, global trust store, or production credentials.
"""
import base64
import contextlib
import ctypes
import datetime as dt
import hashlib
import json
import os
from pathlib import Path
import re
import secrets
import shutil
import stat
import subprocess
import sys
import tempfile
from r1_analysis import Invalid, parse_csv

ORIGINAL_BLOB = '0fbe0b4da1614867bf51b1c3eaf200f82d789a65'
DOCUMENTS = {
    'PROPOSED_PROTOCOL_AMENDMENT.md': 'f068bb0dab6425029436483f82487e0297cf86c92135935d62d00efbaf04f193',
    'PROTOCOL_DECISION_MATRIX.md': '26e683aa40c19a72359135696f608004dd45a8d6cf29f4f4a591bbe0ab9f19b3',
    'STATISTICAL_COUNTEREXAMPLES.md': '64c11fdf13f6daf88fbd660fa72b5a7ba06ab2347d8f553ed9a1cc07f70e496c',
    'IMPLEMENTATION_REPAIR_PLAN.md': '2674c55f0dc08d55a4561839478869f82389c4fd0524763d4238878448913757',
    'FINAL_APPROVAL_RECORD.md': '8a5bdd874114e0c990f912dfbb343d65b3bf13ec49d411224ddd2b2835a67124'}
ROLES = {'APPROVAL': 'maintainer_approval', 'DELEGATION': 'runner_delegation',
         'ELIGIBILITY': 'eligibility', 'RUN': 'run_completion'}

def canonical(value):
    def check(x):
        if type(x) is dict:
            if any(type(k) is not str or not k.isascii() for k in x):
                raise Invalid('NONCANONICAL_KEY')
            for v in x.values(): check(v)
        elif type(x) in (list, tuple):
            for v in x: check(v)
        elif x is not None and type(x) not in (str, int, bool):
            raise Invalid('NONCANONICAL_VALUE')
    check(value)
    return (json.dumps(value, sort_keys=True, ensure_ascii=True, separators=(',', ':'), allow_nan=False)+'\n').encode('ascii')

PACKET_DIGEST = hashlib.sha256(canonical(DOCUMENTS)).hexdigest()

def digest(data): return hashlib.sha256(data).hexdigest()

def strict_json(raw):
    if len(raw) > 1024 * 1024: raise Invalid('OVERSIZE_RECORD')
    def unique(pairs):
        result = {}
        for k, v in pairs:
            if k in result: raise Invalid('DUPLICATE_JSON_KEY')
            result[k] = v
        return result
    try:
        value = json.loads(raw, object_pairs_hook=unique, parse_constant=lambda x: (_ for _ in ()).throw(Invalid('NONFINITE_JSON')))
        if type(value) is not dict or canonical(value) != raw: raise Invalid('NONCANONICAL_RECORD')
        return value
    except (ValueError, TypeError, UnicodeError) as error:
        raise Invalid('MALFORMED_RECORD') from error

def framed(role, payload):
    if role not in ROLES: raise Invalid('UNKNOWN_SIGNATURE_ROLE')
    return f'T81-COMP3-r1/{role}/v1'.encode('ascii')+b'\0'+canonical(payload)

def verify_signature(public_key, role, payload, signature):
    if len(public_key) != 32 or len(signature) != 64: raise Invalid('INVALID_SIGNATURE_ENCODING')
    executable = shutil.which('openssl')
    if executable is None: raise Invalid('SIGNATURE_VERIFIER_UNAVAILABLE')
    with tempfile.TemporaryDirectory(prefix='comp3-public-verify-') as directory:
        p = Path(directory)
        (p/'public.der').write_bytes(bytes.fromhex('302a300506032b6570032100')+public_key)
        (p/'message').write_bytes(framed(role, payload))
        (p/'signature').write_bytes(signature)
        result = subprocess.run([executable, 'pkeyutl', '-verify', '-rawin', '-pubin', '-inkey', str(p/'public.der'),
                                 '-keyform', 'DER', '-in', str(p/'message'), '-sigfile', str(p/'signature')], capture_output=True)
    if result.returncode: raise Invalid('UNTRUSTED_SIGNATURE')

def signed_record(folder, basename, role, public_key):
    try:
        path = safe_path(folder, basename+'.json')
        value = strict_json(path.read_bytes())
        sig = base64.b64decode(safe_path(folder, basename+'.sig').read_bytes(), validate=True)
    except (OSError, ValueError) as error:
        raise Invalid('MISSING_OR_MALFORMED_COMPLETION') from error
    if value.get('record_type') != ROLES[role]: raise Invalid('SIGNATURE_ROLE_MISMATCH')
    verify_signature(public_key, role, value, sig)
    return value, digest(path.read_bytes())

def safe_path(folder, name):
    base = Path(folder).resolve()
    if type(name) is not str or not name or Path(name).is_absolute() or any(x in ('..', '.', '') for x in name.split('/')):
        raise Invalid('INVALID_ARTIFACT_PATH')
    p = base
    for part in name.split('/'):
        p /= part
        if p.is_symlink(): raise Invalid('SYMLINK_ARTIFACT')
    if not p.is_file() or not p.resolve().is_relative_to(base): raise Invalid('MISSING_ARTIFACT')
    return p

def instant(text):
    if type(text) is not str or re.fullmatch(r'\d{4}-\d\d-\d\dT\d\d:\d\d:\d\dZ', text) is None:
        raise Invalid('INVALID_TIMESTAMP')
    try: return dt.datetime.strptime(text, '%Y-%m-%dT%H:%M:%SZ').replace(tzinfo=dt.timezone.utc)
    except ValueError as error: raise Invalid('INVALID_TIMESTAMP') from error

def public_bytes(value):
    if type(value) is not str or re.fullmatch('[0-9a-f]{64}', value) is None: raise Invalid('INVALID_PUBLIC_KEY')
    return bytes.fromhex(value)

def unsigned_integer(x, maximum=(1<<64)-1):
    if type(x) is not int or not 0 <= x <= maximum: raise Invalid('INVALID_RESOURCE_STATUS')
    return x

def decode_status(status):
    unsigned_integer(status, 65535)
    if os.WIFEXITED(status): return {'exited': True, 'exit_code': os.WEXITSTATUS(status), 'signal': None}
    if os.WIFSIGNALED(status): return {'exited': False, 'exit_code': None, 'signal': os.WTERMSIG(status)}
    raise Invalid('INCOMPLETE_TERMINAL_STATUS')

def check_terminal(terminal):
    if type(terminal) is not dict: raise Invalid('MISSING_TERMINAL_STATUS')
    expected = decode_status(terminal.get('raw_wait_status'))
    if any(type(terminal.get(k)) is not type(v) or terminal.get(k) != v for k, v in expected.items()): raise Invalid('CONTRADICTORY_TERMINAL_STATUS')
    if not expected['exited'] or expected['exit_code'] != 0: raise Invalid('COLLECTOR_FAILED')
    cpu = unsigned_integer(terminal.get('user_us')) + unsigned_integer(terminal.get('system_us'))
    rss = unsigned_integer(terminal.get('peak_rss_bytes'))
    if cpu > 600000000 or rss > 500000000: raise Invalid('BUDGET_EXCEEDED')
    if terminal.get('accounting') != 'test_only_fixture': raise Invalid('TEST_ACCOUNTING_SCOPE_REQUIRED')

def verify_test_bundle(folder, trust_file, now=None):
    """Authenticate synthetic diagnostic evidence; NEVER returns measurement eligibility."""
    try:
        trust = strict_json(Path(trust_file).read_bytes())
        if trust.get('kind') != 'test_only_trust': raise Invalid('TEST_TRUST_REQUIRED')
        root = public_bytes(trust.get('root_public_key_hex'))
        approval, approval_hash = signed_record(folder, 'approval', 'APPROVAL', root)
        delegation, delegation_hash = signed_record(folder, 'delegation', 'DELEGATION', root)
        eligibility, eligibility_hash = signed_record(folder, 'eligibility', 'ELIGIBILITY', root)
        revoked=trust.get('revoked_delegation_sha256',[])
        if type(revoked) is not list or any(type(x) is not str or re.fullmatch('[0-9a-f]{64}',x) is None for x in revoked):
            raise Invalid('MALFORMED_TEST_REVOCATIONS')
        if delegation_hash in revoked: raise Invalid('REVOKED_DELEGATION')
        runner = public_bytes(delegation.get('runner_public_key_hex'))
        run, run_hash = signed_record(folder, 'completion', 'RUN', runner)
        for record in (approval, delegation, eligibility, run):
            if record.get('artifact_kind') != 'synthetic' or record.get('packet_digest') != PACKET_DIGEST:
                raise Invalid('TEST_KIND_OR_PROTOCOL_MISMATCH')
        if approval.get('decision') != 'APPROVE' or any(approval.get(m) is not True for m in ('M1','M2','M3','M4')):
            raise Invalid('M1_M4_NOT_APPROVED_IN_TEST_FIXTURE')
        if approval.get('document_sha256') != DOCUMENTS or approval.get('original_blob') != ORIGINAL_BLOB:
            raise Invalid('PROTOCOL_IDENTITY_MISMATCH')
        if approval.get('protocol_revision') != 'EXP-T81-COMP3-01/r1' or approval.get('measurement_authorized') is not False:
            raise Invalid('TEST_APPROVAL_SCOPE')
        if delegation.get('runner_role') != 'test': raise Invalid('TEST_RUNNER_ROLE_REQUIRED')
        if approval.get('delegation_sha256') != delegation_hash: raise Invalid('DELEGATION_MISMATCH')
        if eligibility.get('approval_sha256') != approval_hash or eligibility.get('delegation_sha256') != delegation_hash:
            raise Invalid('ELIGIBILITY_MISMATCH')
        if run.get('approval_sha256') != approval_hash or run.get('delegation_sha256') != delegation_hash or run.get('eligibility_sha256') != eligibility_hash:
            raise Invalid('COMPLETION_CHAIN_MISMATCH')
        campaign = approval.get('campaign_id')
        if type(campaign) is not str or not campaign or any(r.get('campaign_id') != campaign for r in (delegation, eligibility, run)):
            raise Invalid('CAMPAIGN_MISMATCH')
        if re.fullmatch('[0-9a-f]{32}', run.get('run_id', '')) is None: raise Invalid('INVALID_RUN_ID')
        attempt = unsigned_integer(run.get('attempt'))
        if not 1 <= attempt <= unsigned_integer(delegation.get('max_attempts')): raise Invalid('ATTEMPT_SCOPE')
        start, finish = instant(run.get('started_at')), instant(run.get('finished_at'))
        if not max(instant(approval.get('approved_at')), instant(eligibility.get('eligible_at'))) < start <= finish:
            raise Invalid('PREAPPROVAL_OR_INVALID_RUN_TIME')
        current = now or dt.datetime.now(dt.timezone.utc)
        if not instant(delegation.get('not_before')) <= start <= finish <= min(current,instant(delegation.get('not_after'))):
            raise Invalid('EXPIRED_OR_FUTURE_RUN')
        if run.get('n') != 10000000 or run.get('repetitions') != 10: raise Invalid('MEASUREMENT_BUDGET_MISMATCH')
        if run.get('source_commit') != eligibility.get('source_commit') or re.fullmatch('[0-9a-f]{40}',run.get('source_commit','')) is None:
            raise Invalid('SOURCE_IDENTITY_MISMATCH')
        check_terminal(run.get('terminal'))  # mandatory BEFORE CSV parsing/statistics
        if run.get('failure_codes') != []: raise Invalid('RECORDED_COLLECTOR_FAILURE')
        expected_order = [[p,r,j,(j+r)%5] for p in range(3) for r in range(10) for j in range(5)]
        if run.get('run_order') != expected_order: raise Invalid('INVALID_MATCHED_RUN_ORDER')
        if run.get('controls') != {'affinity_verified':True,'test_only':True}: raise Invalid('ENVIRONMENT_CONTROLS_UNVERIFIED')
        if run.get('pmu') != {'instruction_event':'PERF_COUNT_HW_INSTRUCTIONS','miss_event':'PERF_COUNT_HW_BRANCH_MISSES','scheduled':True,'test_only':True}:
            raise Invalid('PMU_CONFIGURATION_MISMATCH')
        artifacts = run.get('artifacts')
        required = {'csv','stderr','environment','corpus_A','corpus_B','corpus_C','executable','source_manifest','assembly'}
        if type(artifacts) is not dict or set(artifacts) != required: raise Invalid('INCOMPLETE_ARTIFACT_MANIFEST')
        for role, entry in artifacts.items():
            if type(entry) is not dict or set(entry) != {'path','sha256'}: raise Invalid('MALFORMED_ARTIFACT_BINDING')
            if digest(safe_path(folder,entry['path']).read_bytes()) != entry['sha256']: raise Invalid('MODIFIED_ARTIFACT')
        inputs = eligibility.get('artifacts')
        if type(inputs) is not dict or set(inputs) != required - {'csv','stderr','environment'}:
            raise Invalid('INCOMPLETE_ELIGIBILITY_INPUTS')
        if any(artifacts[k] != value for k,value in inputs.items()): raise Invalid('ELIGIBLE_ARTIFACT_MISMATCH')
        raw_csv=safe_path(folder,artifacts['csv']['path']).read_bytes()
        parse_csv(raw_csv)  # terminal and authentication precede domain/coverage checks
        actual=[tuple(map(int,line.split(',')[:3])) for line in raw_csv.decode('ascii').splitlines()[1:]]
        if actual != [(p,m,block) for p,block,position,m in expected_order]:
            raise Invalid('OBSERVATION_ORDER_MISMATCH')
        return {'kind':'synthetic_diagnostic','measurement_eligible':False,'run':run,
                'record_sha256':run_hash,'csv':raw_csv}
    except (OSError,KeyError,TypeError,ValueError) as error:
        if isinstance(error,Invalid): raise
        raise Invalid('MISSING_OR_MALFORMED_COMPLETION') from error

def verify_measurement_bundle(*args, **kwargs):
    # This is an explicit publication/activation barrier, not a test-key override.
    raise Invalid('MEASUREMENT_NOT_AUTHORIZED_RUNNER_INCOMPLETE')

@contextlib.contextmanager
def locked_journal(path):
    if os.name != 'posix': raise Invalid('JOURNAL_LOCKING_UNSUPPORTED')
    import fcntl
    path = Path(path)
    if path.is_symlink(): raise Invalid('SYMLINK_JOURNAL')
    flags = os.O_CREAT | os.O_RDWR | getattr(os,'O_NOFOLLOW',0)
    fd = os.open(path,flags,0o600)
    try:
        fcntl.flock(fd,fcntl.LOCK_EX)
        info = os.fstat(fd)
        if info.st_uid != os.getuid() or stat.S_IMODE(info.st_mode)&0o077: raise Invalid('UNPROTECTED_TEST_JOURNAL')
        raw = os.read(fd,1024*1024)
        journal = strict_json(raw) if raw else {'kind':'test_only_journal','reservations':{},'admissions':{}}
        if journal.get('kind') != 'test_only_journal': raise Invalid('TEST_JOURNAL_SCOPE')
        yield journal
        encoded = canonical(journal)
        os.lseek(fd,0,0);os.ftruncate(fd,0)
        offset=0
        while offset<len(encoded): offset+=os.write(fd,encoded[offset:])
        os.fsync(fd)
    finally: os.close(fd)

def reserve_test_run(path,campaign,max_attempts):
    with locked_journal(path) as j:
        count=sum(value['campaign_id']==campaign for value in j['reservations'].values())
        if count>=max_attempts: raise Invalid('ATTEMPT_LIMIT')
        run_id=secrets.token_hex(16)
        while run_id in j['reservations']: run_id=secrets.token_hex(16)
        j['reservations'][run_id]={'campaign_id':campaign,'attempt':count+1}
    return run_id,count+1

def admit_test_diagnostic(verified,path):
    if verified.get('kind')!='synthetic_diagnostic' or verified.get('measurement_eligible') is not False: raise Invalid('TEST_ADMISSION_SCOPE')
    run=verified['run']
    with locked_journal(path) as j:
        reservation=j['reservations'].get(run['run_id'])
        if reservation!={'campaign_id':run['campaign_id'],'attempt':run['attempt']}: raise Invalid('UNRESERVED_RUN_ID')
        if run['run_id'] in j['admissions']: raise Invalid('REPLAY_DETECTED')
        j['admissions'][run['run_id']]=verified['record_sha256']

def wait4_linux_native(pid):
    """Native integer kernel accounting; deliberately unavailable on unverified ABIs."""
    if sys.platform!='linux' or ctypes.sizeof(ctypes.c_long)!=8: raise Invalid('LINUX_NATIVE_ACCOUNTING_UNAVAILABLE')
    class Timeval(ctypes.Structure): _fields_=[('sec',ctypes.c_long),('usec',ctypes.c_long)]
    class Usage(ctypes.Structure):
        _fields_=[('user',Timeval),('system',Timeval)]+[(k,ctypes.c_long) for k in
          ('maxrss','ixrss','idrss','isrss','minflt','majflt','nswap','inblock','oublock','msgsnd','msgrcv','nsignals','nvcsw','nivcsw')]
    library=ctypes.CDLL(None,use_errno=True)
    wait=library.wait4;wait.argtypes=[ctypes.c_int,ctypes.POINTER(ctypes.c_int),ctypes.c_int,ctypes.POINTER(Usage)];wait.restype=ctypes.c_int
    status=ctypes.c_int();usage=Usage()
    while True:
        result=wait(pid,ctypes.byref(status),0,ctypes.byref(usage))
        if result==pid: break
        if result<0 and ctypes.get_errno()==4: continue
        raise Invalid('SUPERVISOR_WAIT_FAILURE')
    return {'raw_wait_status':status.value,**decode_status(status.value),
            'user_us':usage.user.sec*1000000+usage.user.usec,
            'system_us':usage.system.sec*1000000+usage.system.usec,
            'peak_rss_bytes':usage.maxrss*1024,'accounting':'linux_wait4_native_integer'}

def run_production(*args, **kwargs):
    raise Invalid('MEASUREMENT_NOT_AUTHORIZED_RUNNER_INCOMPLETE')
