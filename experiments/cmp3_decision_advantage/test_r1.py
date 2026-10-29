"""Non-performance r1 tests. Ephemeral keys are TEST ONLY."""
import copy
import datetime as dt
from fractions import Fraction
import hashlib
import itertools
import json
import os
from pathlib import Path
import shutil
import signal
import subprocess
import sys
import tempfile
import unittest
import base64
import r1_analysis as a
import runner_r1 as r
import inspect_assembly as assembly

ROOT=Path(__file__).resolve().parent

def csv_bytes():
    return (a.HEADER+'\n'+''.join(f'{p},{m},{i},{70 if m==0 else 100},{8 if m==0 else 10},{50 if m==0 else 55},42\n'
            for p in range(3) for i in range(10) for j in range(5) for m in [(i+j)%5])).encode()

class Statistics(unittest.TestCase):
    def test_pcg_golden(self):
        g=a.PCG64()
        self.assertEqual([g.next() for _ in range(4)], [0x854CAFBD0A540778,0x3D174C3776108E7D,0xA8398F10D0F2976C,0x722D0D72D6D3EA65])
    def test_all_100000_indices_independent_jump(self):
        mask=(1<<128)-1;mul=a.PCG64.multiplier;s0=((3+0x5438315F434D50335F53454544303131)*mul+3)&mask
        powers=[];mm,bb=mul,3
        for _ in range(32):powers.append((mm,bb));mm,bb=mm*mm&mask,bb*(mm+1)&mask
        def raw(index):
            m,c=1,0;bit=0
            while index:
                if index&1:pm,pc=powers[bit];m,c=pm*m&mask,(pm*c+pc)&mask
                index>>=1;bit+=1
            s=(m*s0+c)&mask;x=(s>>64)^(s&((1<<64)-1));rot=s>>122
            return x if rot==0 else ((x>>rot)|((x&((1<<rot)-1))<<(64-rot)))
        expected=[];j=0
        while len(expected)<100000:
            x=raw(j);j+=1
            if x>=6:expected.append(x%10)
        actual=bytes(i for row in a.bootstrap_indices() for i in row)
        self.assertEqual(actual,bytes(expected))
        self.assertEqual(hashlib.sha256(actual).hexdigest(),'8c5f235bc6d1df1c1fcdca99903abaaaa8201591d5521fe84ba7e72868770b64')
    def test_rejection_and_invalid_bound(self):
        class Forced(a.PCG64):
            def __init__(self):self.outputs=iter([0,5,6,7,9,10])
            def next(self):return next(self.outputs)
        g=Forced();self.assertEqual([g.bounded(10) for _ in range(4)],[6,7,9,0])
        for n in (0,-1,1<<64):
            with self.subTest(n=n),self.assertRaises(a.Invalid):g.bounded(n)
    def test_all_eleven_counterexamples(self):
        cases=json.loads((ROOT/'fixtures/statistical_cases.json').read_text())['cases']
        self.assertEqual(len(cases),11)
        for name,case in cases.items():
            modes=[{k:v[k] for k in ('instructions','branch_misses','elapsed_ns')} for v in case['exact_series']]
            with self.subTest(case=name):self.assertEqual(a.classify_pattern(modes)['outcome'],case['outcome'])
    def test_seven_exact_bootstrap_fixtures(self):
        fixtures=json.loads((ROOT/'fixtures/bootstrap_cases.json').read_text())['fixtures']
        self.assertEqual(len(fixtures),7)
        for name,f in fixtures.items():
            with self.subTest(name=name):
                low,high=a.reduction_bounds(tuple(f['candidate']),tuple(f['baseline1']))
                self.assertEqual(low,Fraction(f['lower']['numerator'],f['lower']['denominator']))
                self.assertEqual(high,Fraction(f['upper']['numerator'],f['upper']['denominator']))
                self.assertEqual(low>=Fraction(3,20),f['passes_L_15'])
                got=a.contrast(f['candidate'],f['baseline1'])
                self.assertEqual(got['sample_variance'],Fraction(f['variance_difference']['numerator'],f['variance_difference']['denominator']))
    def test_near_zero_not_erased(self):
        b=[10**17]*10;c=[85*10**15]*9+[85*10**15+1]
        low,_=a.reduction_bounds(tuple(c),tuple(b))
        self.assertLess(low,Fraction(3,20));self.assertEqual(float(low),float(Fraction(3,20)))
        self.assertEqual(a.contrast(c,b)['sample_variance'],Fraction(1,10))
        self.assertFalse(a.contrast(c,b)['degenerate'])
    def test_zero_variance_limits(self):
        equal=a.contrast([10]*10,[10]*10);nonzero=a.contrast([11]*10,[10]*10)
        self.assertEqual(equal['p_two_sided'],1);self.assertEqual(equal['ci99'],[0,0])
        self.assertEqual(nonzero['p_two_sided'],0);self.assertEqual(nonzero['family_interval'],[1,1])
    def test_quantile_landmarks(self):
        for t,p in [(0,1),(2.2621571628540993,.05),(3.2498355415921254,.01),(4.7809125859335495,.001)]:
            with self.subTest(t=t):self.assertAlmostEqual(a.student_pvalue(t),p,places=11)
    def test_family_and_64_aggregate_combinations(self):
        family=[(p,k,b) for p in range(3) for k,b in [('I',1),('B',1),('T',1),('T',2),('T',3),('T',4)]]
        self.assertEqual(len(set(family)),18)
        self.assertAlmostEqual(a.critical_value(a.REGRESSION_ALPHA),5.211371786975027,places=10)
        for values in itertools.product(a.OUTCOMES,repeat=3):
            self.assertEqual(a.aggregate(values),next(x for x in ('BLOCKED','NOT SUPPORTED','INCONCLUSIVE','SUPPORTED') if x in values))
        for v,g,q in itertools.product([False,True],repeat=3):
            self.assertEqual(sum([not v,v and g,v and not g and q,v and not g and not q]),1)
    def test_each_of_eighteen_regression_contrasts(self):
        for pattern in range(3):
            for name,field,baseline in [('instructions_vs_1','instructions',1),('branch_misses_vs_1','branch_misses',1)]+[(f'latency_vs_{i}','elapsed_ns',i) for i in range(1,5)]:
                modes=a.parse_csv(csv_bytes())[pattern]
                for mode in modes:
                    for key in mode:mode[key]=[100]*10
                modes[0][field]=[110]*10
                if field=='elapsed_ns':
                    for i in range(1,5):modes[i][field]=[110]*10
                    modes[baseline][field]=[100]*10
                with self.subTest(pattern=pattern,contrast=name):
                    result=a.classify_pattern(modes)
                    self.assertEqual(result['outcome'],'NOT SUPPORTED')
                    self.assertEqual(result['reasons'],[name])
    def test_contrast_rejects_invalid_observations(self):
        for x in (float('nan'),float('inf'),1.0,True,-1,1<<64):
            with self.subTest(value=x),self.assertRaises(a.Invalid):a.contrast([x]+[1]*9,[1]*10)
    def test_invalid_series_direct(self):
        for field in ('instructions','branch_misses','elapsed_ns'):
            for defect in ('missing','extra','fraction','nan','negative','overflow','bool'):
                modes=a.parse_csv(csv_bytes())[0];v=modes[0][field]
                if defect=='missing':v.pop()
                elif defect=='extra':v.append(1)
                else:v[0]={'fraction':1.5,'nan':float('nan'),'negative':-1,'overflow':1<<64,'bool':True}[defect]
                with self.subTest(field=field,defect=defect),self.assertRaises(a.Invalid):a.classify_pattern(modes)

class Parsing(unittest.TestCase):
    def test_exact_uint64_preservation(self):
        data=csv_bytes().replace(b',70,',b',18446744073709551615,')
        self.assertEqual(a.parse_csv(data)[0][0]['instructions'][0],(1<<64)-1)
    def test_invalid_scalar_spellings(self):
        for value in (b'1.0',b'1e2',b'-1',b'+1',b'NaN',b'Inf',b'18446744073709551616',b'01',b' 70',b'70 '):
            with self.subTest(value=value),self.assertRaises(a.Invalid):a.parse_csv(csv_bytes().replace(b',70,',b','+value+b','))
    def test_csv_missing_duplicate_extra_fields(self):
        rows=csv_bytes().splitlines(keepends=True)
        for data in (b''.join(rows[:-1]),b''.join(rows+rows[1:2]),csv_bytes().replace(b',42\n',b',42,0\n'),csv_bytes()[:-1],b'',csv_bytes().replace(b'pattern',b'patte\xffrn')):
            with self.subTest(length=len(data)),self.assertRaises(a.Invalid):a.parse_csv(data)
    def test_payload_mismatch_and_duration_domain(self):
        with self.assertRaisesRegex(a.Invalid,'PAYLOAD_PARITY'):a.parse_csv(csv_bytes().replace(b',42\n',b',43\n',1))
        for duration in (b'0',b'9223372036854775808'):
            with self.subTest(duration=duration),self.assertRaises(a.Invalid):a.parse_csv(csv_bytes().replace(b',50,',b','+duration+b','))
    def test_zero_miss_valid(self):
        self.assertEqual(a.parse_csv(b'\n'.join(b','.join(row.split(b',')[:4]+[b'0']+row.split(b',')[5:]) if n and row else row for n,row in enumerate(csv_bytes().split(b'\n'))))[0][0]['branch_misses'],[0]*10)
    def test_test_diagnostic_never_measurement(self):
        result=a.diagnose(csv_bytes())
        self.assertEqual(result['diagnostic_outcome'],'SUPPORTED');self.assertEqual(result['measurement_outcome'],'BLOCKED')

class Assembly(unittest.TestCase):
    def test_operandless_ret_and_basic_blocks(self):
        text='target:\n\t.cfi_startproc\n\teor x8,x0,x1\n\tadd x0,x8,#1\n\tret\n\t.cfi_endproc\n'
        parsed=assembly.inventory(text)
        self.assertEqual(parsed[0]['total'],3);self.assertEqual(parsed[0]['blocks'][0]['count'],3)
        parsed=assembly.inventory('f:\n\t.cfi_startproc\n\tcmp x0,x1\n\tb.eq L0\n\tnop\n\tret\nL0:\n\tret\n\t.cfi_endproc\n')
        self.assertEqual([b['count'] for b in parsed[0]['blocks']],[2,2,1])
    def test_linker_hint_labels_are_not_blocks(self):
        functions=assembly.inventory('f:\n\t.cfi_startproc\n\tcmp x0,x1\nLloh0:\n\tadrp x2, foo\nLloh1:\n\tldr x2,[x2]\n\tret\n\t.cfi_endproc\n')
        self.assertEqual([b['count'] for b in functions[0]['blocks']],[4])
    def test_x86_ret_nop_and_directives(self):
        text='f:\n\t.cfi_startproc\n\t.cfi_def_cfa_offset 16\n\tnop\n\tretq\n\t.cfi_endproc\n'
        self.assertEqual(assembly.inventory(text)[0]['total'],2)
    def test_unterminated_and_opcode_directive(self):
        for text in ('f:\n\t.cfi_startproc\n\tret\n','f:\n\t.cfi_startproc\n\t.inst 123\n'):
            with self.assertRaises(ValueError):assembly.inventory(text)

class Evidence(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if shutil.which('openssl') is None:raise unittest.SkipTest('OpenSSL Ed25519 verifier unavailable')
        cls.keys=tempfile.TemporaryDirectory(prefix='comp3-TEST-ONLY-keys-')
        cls.keydir=Path(cls.keys.name)
        for role in ('TEST-ONLY-root','TEST-ONLY-runner'):
            key=cls.keydir/(role+'.pem')
            subprocess.run(['openssl','genpkey','-algorithm','ED25519','-out',str(key)],check=True,capture_output=True)
            key.chmod(0o600)
        cls.root_key=cls.keydir/'TEST-ONLY-root.pem';cls.runner_key=cls.keydir/'TEST-ONLY-runner.pem'
        cls.root_pub=cls.public(cls.root_key);cls.runner_pub=cls.public(cls.runner_key)
    @classmethod
    def tearDownClass(cls):cls.keys.cleanup()
    @staticmethod
    def public(path):
        result=subprocess.run(['openssl','pkey','-in',str(path),'-pubout','-outform','DER'],check=True,capture_output=True)
        assert result.stdout[:12]==bytes.fromhex('302a300506032b6570032100')
        return result.stdout[-32:]
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory(prefix='comp3-TEST-ONLY-evidence-');self.p=Path(self.temp.name)
        self.trust=self.p/'TEST-ONLY-trust.json'
        self.trust.write_bytes(r.canonical({'kind':'test_only_trust','root_public_key_hex':self.root_pub.hex()}))
        self.journal=self.p/'TEST-ONLY-journal.json'
        run_id,attempt=r.reserve_test_run(self.journal,'TEST-ONLY-campaign',3)
        common={'artifact_kind':'synthetic','packet_digest':r.PACKET_DIGEST,'campaign_id':'TEST-ONLY-campaign'}
        self.delegation={**common,'record_type':'runner_delegation','runner_public_key_hex':self.runner_pub.hex(),'runner_role':'test','max_attempts':3,'not_before':'2020-01-01T00:00:00Z','not_after':'2100-01-01T00:00:00Z'}
        dh=self.write('delegation','DELEGATION',self.delegation,self.root_key)
        self.approval={**common,'record_type':'maintainer_approval','decision':'APPROVE','M1':True,'M2':True,'M3':True,'M4':True,
           'document_sha256':r.DOCUMENTS,'original_blob':r.ORIGINAL_BLOB,'protocol_revision':'EXP-T81-COMP3-01/r1','measurement_authorized':False,
           'delegation_sha256':dh,'approved_at':'2020-01-01T00:00:00Z'}
        ah=self.write('approval','APPROVAL',self.approval,self.root_key)
        artifacts={}
        for name in ('csv','stderr','environment','corpus_A','corpus_B','corpus_C','executable','source_manifest','assembly'):
            file=self.p/(name+'.fixture')
            file.write_bytes(csv_bytes() if name=='csv' else b'TEST ONLY synthetic placeholder, not hardware or production artifacts\n')
            artifacts[name]={'path':file.name,'sha256':r.digest(file.read_bytes())}
        self.eligibility={**common,'record_type':'eligibility','approval_sha256':ah,'delegation_sha256':dh,
            'source_commit':'988b18ca9ebdb4a6fb76fe0be30115470e5637f0','eligible_at':'2020-01-01T00:00:01Z',
            'artifacts':{k:v for k,v in artifacts.items() if k not in ('csv','stderr','environment')}}
        eh=self.write('eligibility','ELIGIBILITY',self.eligibility,self.root_key)
        self.run={**common,'record_type':'run_completion','run_id':run_id,'attempt':attempt,
            'approval_sha256':ah,'delegation_sha256':dh,'eligibility_sha256':eh,'source_commit':self.eligibility['source_commit'],
            'started_at':'2020-01-01T00:00:02Z','finished_at':'2020-01-01T00:00:03Z','n':10000000,'repetitions':10,
            'terminal':{'raw_wait_status':0,'exited':True,'exit_code':0,'signal':None,'user_us':1,'system_us':2,'peak_rss_bytes':4096,'accounting':'test_only_fixture'},
            'run_order':[[p,i,j,(j+i)%5] for p in range(3) for i in range(10) for j in range(5)],
            'controls':{'affinity_verified':True,'test_only':True},'failure_codes':[],
            'pmu':{'instruction_event':'PERF_COUNT_HW_INSTRUCTIONS','miss_event':'PERF_COUNT_HW_BRANCH_MISSES','scheduled':True,'test_only':True},'artifacts':artifacts}
        self.resign()
    def tearDown(self):self.temp.cleanup()
    def write(self,name,role,value,key):
        (self.p/(name+'.json')).write_bytes(r.canonical(value))
        message=self.p/'TEST-ONLY-signing-input';message.write_bytes(r.framed(role,value))
        signature=subprocess.run(['openssl','pkeyutl','-sign','-rawin','-inkey',str(key),'-in',str(message)],check=True,capture_output=True).stdout
        (self.p/(name+'.sig')).write_bytes(base64.b64encode(signature))
        return r.digest(r.canonical(value))
    def resign(self):self.write('completion','RUN',self.run,self.runner_key)
    def verified(self):return r.verify_test_bundle(self.p,self.trust)
    def test_authentic_test_chain_not_measurement(self):
        v=self.verified();self.assertFalse(v['measurement_eligible']);self.assertEqual(a.diagnose(v['csv'])['measurement_outcome'],'BLOCKED')
        with self.assertRaisesRegex(a.Invalid,'MEASUREMENT_NOT_AUTHORIZED'):r.verify_measurement_bundle(self.p,self.trust)
        with self.assertRaisesRegex(a.Invalid,'MEASUREMENT_NOT_AUTHORIZED'):r.run_production()
    def test_missing_malformed_or_untrusted_record(self):
        for defect in ('missing','malformed','signature'):
            with self.subTest(defect=defect):
                self.resign()
                if defect=='missing':(self.p/'completion.sig').unlink()
                elif defect=='malformed':(self.p/'completion.json').write_bytes(b'not json')
                else:(self.p/'completion.sig').write_bytes(base64.b64encode(b'\0'*64))
                with self.assertRaises(a.Invalid):self.verified()
    def test_modified_csv_and_environment(self):
        for role in ('csv','environment','executable','corpus_A','source_manifest','assembly'):
            path=self.p/self.run['artifacts'][role]['path'];old=path.read_bytes()
            with self.subTest(role=role):
                path.write_bytes(old+b'tampered\n')
                with self.assertRaisesRegex(a.Invalid,'MODIFIED_ARTIFACT'):self.verified()
                path.write_bytes(old)
    def test_complete_csv_failed_exit_or_signal(self):
        original=copy.deepcopy(self.run['terminal'])
        for status in (6<<8,int(signal.SIGKILL)):
            with self.subTest(status=status):
                self.run['terminal']={**original,'raw_wait_status':status,**r.decode_status(status)};self.resign()
                with self.assertRaisesRegex(a.Invalid,'COLLECTOR_FAILED'):self.verified()
                self.assertEqual(len(a.parse_csv(csv_bytes())),3)
    def test_budget_equal_and_exceeded(self):
        original=copy.deepcopy(self.run['terminal'])
        for field,limit in [('user_us',600000000),('peak_rss_bytes',500000000)]:
            with self.subTest(field=field):
                self.run['terminal']={**original,'system_us':0,field:limit};self.resign();self.verified()
                self.run['terminal'][field]=limit+1;self.resign()
                with self.assertRaisesRegex(a.Invalid,'BUDGET_EXCEEDED'):self.verified()
    def test_contradictory_and_incomplete_terminal(self):
        original=copy.deepcopy(self.run['terminal'])
        variants=[None,{**original,'raw_wait_status':9},{**original,'user_us':'1.5'},{**original,'peak_rss_bytes':-1}]
        for value in variants:
            with self.subTest(value=value):
                self.run['terminal']=value;self.resign()
                with self.assertRaises(a.Invalid):self.verified()
    def test_replay_and_idempotent_read(self):
        v=self.verified();self.verified();r.admit_test_diagnostic(v,self.journal)
        self.verified() # read-only re-audit is idempotent
        with self.assertRaisesRegex(a.Invalid,'REPLAY'):r.admit_test_diagnostic(v,self.journal)
    def test_reservation_attempt_limit_and_unreserved_id(self):
        r.reserve_test_run(self.journal,'TEST-ONLY-campaign',3);r.reserve_test_run(self.journal,'TEST-ONLY-campaign',3)
        with self.assertRaisesRegex(a.Invalid,'ATTEMPT_LIMIT'):r.reserve_test_run(self.journal,'TEST-ONLY-campaign',3)
        self.run['run_id']='0'*32;self.resign()
        with self.assertRaisesRegex(a.Invalid,'UNRESERVED'):r.admit_test_diagnostic(self.verified(),self.journal)
    def test_signed_protocol_scope_and_run_order_failures(self):
        original=copy.deepcopy(self.run)
        mutations=[('packet_digest','0'*64),('artifact_kind','measurement'),('eligibility_sha256','0'*64),('source_commit','0'*40),
                   ('run_order',[]),('n',10000),('repetitions',1),('failure_codes',['CORRECTNESS_FAILURE']),('attempt',4),
                   ('started_at','2019-01-01T00:00:00Z'),('finished_at','2200-01-01T00:00:00Z'),('controls',{}),('pmu',{})]
        for key,value in mutations:
            with self.subTest(key=key):
                self.run=copy.deepcopy(original);self.run[key]=value;self.resign()
                with self.assertRaises(a.Invalid):self.verified()
    def test_missing_approval_and_eligibility(self):
        for file in ('approval.sig','eligibility.sig','delegation.sig'):
            p=self.p/file;old=p.read_bytes();p.unlink()
            with self.subTest(file=file),self.assertRaises(a.Invalid):self.verified()
            p.write_bytes(old)
    def test_wrong_root_and_signature_role(self):
        self.trust.write_bytes(r.canonical({'kind':'test_only_trust','root_public_key_hex':self.runner_pub.hex()}))
        with self.assertRaisesRegex(a.Invalid,'SIGNATURE'):self.verified()
        self.trust.write_bytes(r.canonical({'kind':'test_only_trust','root_public_key_hex':self.root_pub.hex()}))
        self.write('completion','APPROVAL',self.run,self.runner_key)
        with self.assertRaisesRegex(a.Invalid,'SIGNATURE'):self.verified()
    def test_revoked_test_delegation(self):
        trust=r.strict_json(self.trust.read_bytes())
        trust['revoked_delegation_sha256']=[r.digest((self.p/'delegation.json').read_bytes())]
        self.trust.write_bytes(r.canonical(trust))
        with self.assertRaisesRegex(a.Invalid,'REVOKED_DELEGATION'):self.verified()
    def test_signed_csv_order_must_match_manifest(self):
        path=self.p/self.run['artifacts']['csv']['path']
        lines=path.read_bytes().splitlines(keepends=True)
        lines[1],lines[2]=lines[2],lines[1]
        path.write_bytes(b''.join(lines))
        self.run['artifacts']['csv']['sha256']=r.digest(path.read_bytes());self.resign()
        with self.assertRaisesRegex(a.Invalid,'OBSERVATION_ORDER_MISMATCH'):self.verified()
    def test_cli_test_only_and_failed_collector(self):
        command=[sys.executable,str(ROOT/'analyze_r1.py'),'--bundle',str(self.p),'--trust',str(self.trust),'--test-only']
        result=subprocess.run(command,capture_output=True,check=True)
        parsed=json.loads(result.stdout)
        self.assertEqual(parsed['measurement_outcome'],'BLOCKED')
        self.assertEqual(parsed['diagnostic_outcome'],'SUPPORTED')
        self.run['terminal'].update(raw_wait_status=6<<8,exit_code=6);self.resign()
        result=subprocess.run(command,capture_output=True)
        self.assertEqual(result.returncode,6)
        self.assertEqual(json.loads(result.stdout)['measurement_outcome'],'BLOCKED')
        self.assertNotIn(b'SUPPORTED',result.stdout)
    def test_test_trust_isolation(self):
        self.trust.write_bytes(r.canonical({'kind':'production_trust','root_public_key_hex':self.root_pub.hex()}))
        with self.assertRaisesRegex(a.Invalid,'TEST_TRUST_REQUIRED'):self.verified()
    def test_noncanonical_json_and_duplicate_keys(self):
        for raw in (b'{"a":1,"a":2}\n',b'{"a":1.0}\n',b'{"a":NaN}\n',b'{ "a":1}\n'):
            with self.subTest(raw=raw),self.assertRaises(a.Invalid):r.strict_json(raw)
    def test_path_traversal_and_symlink(self):
        for name in ('../escape','/tmp/escape','foo/../bar'):
            with self.subTest(name=name),self.assertRaises(a.Invalid):r.safe_path(self.p,name)
        (self.p/'linked').symlink_to(self.p/'csv.fixture')
        with self.assertRaises(a.Invalid):r.safe_path(self.p,'linked')
    @unittest.skipUnless(os.name=='posix','POSIX wait status requires POSIX')
    def test_real_exit_and_signal_status(self):
        for signaled in (False,True):
            pid=os.fork()
            if pid==0:
                if signaled:os.kill(os.getpid(),signal.SIGKILL)
                os._exit(6)
            _,status=os.waitpid(pid,0)
            decoded=r.decode_status(status)
            self.assertEqual(decoded['signal'],signal.SIGKILL if signaled else None)
            self.assertEqual(decoded['exit_code'],None if signaled else 6)
    @unittest.skipUnless(sys.platform=='linux','Linux native integer wait4/rusage ABI unavailable')
    def test_linux_native_integer_resources(self):
        pid=os.fork()
        if pid==0:os._exit(0)
        record=r.wait4_linux_native(pid)
        self.assertEqual(record['exit_code'],0)
        self.assertIs(type(record['user_us']),int);self.assertIs(type(record['peak_rss_bytes']),int)

if __name__=='__main__':unittest.main(verbosity=2)
