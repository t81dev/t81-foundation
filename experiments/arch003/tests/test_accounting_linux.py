"""Opt-in actual Linux kernel lifecycle fixtures; never a correctness campaign.

Set ARCH003_LINUX_FIXTURES=1 and ARCH003_ACCOUNTING_FIXTURE to the small compiled
accounting_fixture.cpp program. Each case has a fresh component accountant,
explicitly separate from the single non-resettable future campaign envelope.
"""
import hashlib
import os
from pathlib import Path
import signal
import sys
import tempfile
import time
import unittest
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]))
from accounting import Accountant, AccountingUnavailable, ResourceExceeded, DEADLINE_NS
from linux_trace import LinuxTrace
from evidence import write_json, HEADER, EXPECTED, OBSERVED, D0, fold, manifest_template, verify_index
from finalization import finalize, retain_interruption, recover_prefix, preserve_failure_status

SOURCE=Path(__file__).resolve().parents[1]
ENABLED=sys.platform=='linux' and os.environ.get('ARCH003_LINUX_FIXTURES')=='1'


@unittest.skipUnless(ENABLED,'explicit Linux infrastructure fixture opt-in required')
class LinuxLifecycle(unittest.TestCase):
    def setUp(self):
        self.temp=tempfile.TemporaryDirectory()
        self.root=Path(self.temp.name)
        self.acc=Accountant(self.root)
        self.fixture=os.environ['ARCH003_ACCOUNTING_FIXTURE']
        self.notes={}

    def tearDown(self):
        evidence=os.environ.get('ARCH003_LINUX_EVIDENCE_DIR')
        if evidence:
            directory=Path(evidence);directory.mkdir(parents=True,exist_ok=True)
            write_json(directory/(self._testMethodName+'.json'),dict(jobs=self.acc.jobs,
                accounting_complete=self.acc.complete,notes=self.notes,
                source_sha256={str(p.relative_to(SOURCE)):hashlib.sha256(p.read_bytes()).hexdigest()
                    for p in [SOURCE/'accounting.py',SOURCE/'linux_trace.py',SOURCE/'finalization.py',Path(__file__)]}))
        self.temp.cleanup()

    def run_job(self,mode,**kwargs):
        return self.acc.run([self.fixture,mode],self.root/'out.stdout',self.root/'out.stderr',**kwargs)

    def check_receipts(self,receipt,count):
        self.assertTrue(receipt['complete'])
        self.assertEqual(len(receipt['processes']),count)
        for row in receipt['processes']:
            self.assertTrue(row['complete'])
            self.assertGreater(row['peak_rss_bytes'],0)
            self.assertTrue(any(p['at']=='exit_stop' for p in row['peak_observations']))
        for task in receipt['tasks'].values():
            self.assertTrue(task['reaped']);self.assertIsNotNone(task['wait4'])
        self.assertLessEqual(receipt['aggregate_bound_bytes'],512*2**20)
        self.assertLessEqual(receipt['observed_max_sample_gap_ns'],100_000_000)
        self.assertGreaterEqual(receipt['sample_count'],1)

    def test_short_tool_between_rss_samples(self):
        receipt=self.run_job('sequential',compiler=True)
        self.check_receipts(receipt,4)
        self.assertEqual(len([e for e in receipt['events'] if e['kind']=='fork']),3)
        # Three sequential tools are charged by simultaneous lifetimes, not summed.
        total=sum(max(p['peak_rss_bytes'],p['wait4_peak_bound_bytes']) for p in receipt['processes'])
        self.assertLess(receipt['descendants_peak_bound_bytes'],total)

    def test_actual_driver_and_linker_receipts(self):
        source=self.root/'tiny.cpp';source.write_text('int main(){return 0;}\n')
        compiler=os.environ.get('ARCH003_LINUX_CLANG','/usr/bin/clang++')
        for args in ([compiler,'-std=c++20','-Wall','-Wextra','-Werror','-fno-lto','-c',str(source),'-o',str(self.root/'tiny.o')],
                     [compiler,'-fno-lto',str(self.root/'tiny.o'),'-o',str(self.root/'tiny')]):
            receipt=self.acc.run(args,self.root/'compiler.stdout',self.root/'compiler.stderr',compiler=True)
            self.assertEqual(receipt['exit_code'],0,(self.root/'compiler.stderr').read_text())
            self.assertTrue(receipt['complete'])
        self.assertEqual(len(self.acc.jobs[-1]['processes']),2)
        self.check_receipts(self.acc.jobs[-1],2)

    def test_peak_survives_free_then_exec(self):
        receipt=self.run_job('preexec')
        self.check_receipts(receipt,1)
        row=receipt['processes'][0]
        self.assertGreaterEqual(row['peak_rss_bytes'],24*2**20)
        self.assertGreater(row['peak_rss_bytes'],row['last_rss_bytes'])
        self.assertGreaterEqual(len([p for p in row['peak_observations'] if p['at']=='before_exec']),2)

    def test_unexpected_nested_descendant(self):
        with self.assertRaisesRegex(AccountingUnavailable,'unexpected descendant'):
            self.run_job('nested',role='runtime')
        self.assertFalse(self.acc.complete)
        self.assertTrue(all(t['reaped'] for t in self.acc.jobs[-1]['tasks'].values()))

    def test_metadata_cannot_spawn_worker(self):
        with self.assertRaisesRegex(AccountingUnavailable,'unexpected descendant'):
            self.run_job('driver')
        self.assertFalse(self.acc.complete)

    def test_abnormal_signal_reaped_with_peak(self):
        receipt=self.run_job('signal')
        self.check_receipts(receipt,1)
        self.assertEqual(receipt['exit_code'],128+signal.SIGTERM)
        task=next(iter(receipt['tasks'].values()))
        self.assertEqual(task['wait4']['termination_signal'],signal.SIGTERM)

    def test_runtime_maintenance_thread_accounted_once(self):
        receipt=self.run_job('threads',role='worker')
        self.check_receipts(receipt,1)
        self.assertEqual(len(receipt['tasks']),2)
        self.assertEqual(len(receipt['processes'][0]['threads_seen']),2)

    def test_nonleader_exec_blocks_before_pid_transition(self):
        with self.assertRaisesRegex(AccountingUnavailable,'nonleader exec'):
            self.run_job('thread-exec',role='worker')
        self.assertFalse(self.acc.complete)
        self.assertTrue(all(t['reaped'] for t in self.acc.jobs[-1]['tasks'].values()))
        self.notes['limitation']='nonleader exec deliberately blocked before kernel TID remapping'

    def test_unavailable_exit_peak_is_null_not_zero(self):
        original=LinuxTrace.observe
        failed=False
        def missing(trace,pid,reason=None):
            nonlocal failed
            if reason=='exit_stop' and not failed:
                failed=True
                raise AccountingUnavailable('fixture exit peak unavailable')
            return original(trace,pid,reason)
        with patch.object(LinuxTrace,'observe',missing):
            with self.assertRaises(AccountingUnavailable):self.run_job('short')
        self.assertFalse(self.acc.complete)
        receipt=self.acc.jobs[-1]
        self.assertTrue(all(t['reaped'] for t in receipt['tasks'].values()))
        self.assertIsNone(receipt['descendants_peak_bound_bytes'])
        self.assertIsNone(receipt['aggregate_bound_bytes'])
        self.notes['fault_injection']='exit-stop VmHWM unavailable; no zero substituted'

    def test_unavailable_proc_at_launch_is_reaped(self):
        original=self.acc.process
        def missing(pid):
            if pid!=os.getpid():raise AccountingUnavailable('fixture initial /proc unavailable')
            return original(pid)
        self.acc.process=missing
        with self.assertRaises(AccountingUnavailable):self.run_job('short')
        receipt=self.acc.jobs[-1]
        self.assertTrue(all(t['reaped'] for t in receipt['tasks'].values()))
        self.assertIsNone(receipt['aggregate_bound_bytes'])
        self.notes['fault_injection']='initial process identity unavailable; kernel birth PID retained for cleanup'

    def test_ptrace_options_unavailable_is_reaped(self):
        original=LinuxTrace.ptrace
        def denied(trace,request,pid,address=0,data=0):
            if request==0x4200:raise AccountingUnavailable('fixture ptrace options unavailable')
            return original(trace,request,pid,address,data)
        with patch.object(LinuxTrace,'ptrace',denied):
            with self.assertRaises(AccountingUnavailable):self.run_job('short')
        self.assertFalse(self.acc.complete)
        self.assertTrue(all(t['reaped'] for t in self.acc.jobs[-1]['tasks'].values()))
        self.notes['fault_injection']='ptrace options refused before execution'

    def test_unavailable_proc_mid_job(self):
        original=self.acc.process
        reads=0
        def missing(pid):
            nonlocal reads
            if pid!=os.getpid():
                reads+=1
                if reads==5:raise AccountingUnavailable('fixture /proc unavailable')
            return original(pid)
        self.acc.process=missing
        with self.assertRaisesRegex(AccountingUnavailable,'fixture /proc unavailable'):self.run_job('sleep')
        self.assertFalse(self.acc.complete)
        self.assertTrue(all(t['reaped'] for t in self.acc.jobs[-1]['tasks'].values()))
        self.notes['fault_injection']='single /proc read unavailable during live trace'

    def test_pid_identity_race_mid_job(self):
        original=self.acc.process
        reads=0
        def changed(pid):
            nonlocal reads
            row=original(pid)
            if pid!=os.getpid():
                reads+=1
                if reads==5:row['start_ticks']+=1
            return row
        self.acc.process=changed
        with self.assertRaisesRegex(AccountingUnavailable,'PID reuse/identity race'):self.run_job('sleep')
        self.assertFalse(self.acc.complete)
        self.notes['fault_injection']='one observed process start_ticks changed; no real PID reuse claimed'

    def test_sampling_gap_while_live(self):
        real=time.monotonic_ns
        def clock():
            trace=getattr(self.acc,'_trace',None)
            return real()+(101_000_000 if trace and trace.syscall_stops>8 else 0)
        self.acc.clock=clock
        self.acc.sample(2)
        with self.assertRaisesRegex(AccountingUnavailable,'sample interval'):self.run_job('sleep')
        self.notes['fault_injection']='clock advances by 101 ms during live syscalls'

    def test_actual_L_V_W_fixed_frame(self):
        expected=self.root/'expected.bin'
        expected.write_bytes(HEADER.pack(b'A2R1',0,255,0,0,1)+EXPECTED.pack(0,0,0,0,0,0,0,0,0,0,0,0,0,2,bytes(32)))
        worker=self.root/'fixed_worker.py'
        worker.write_text('import os\nos.write(1,'+repr(OBSERVED.pack(0,1,0,1,0,0,fold(D0,0,0),0))+')\n')
        env=dict(os.environ,ARCH003_AUTHORIZED_VERIFIER='bounded infrastructure fixture only')
        command=[sys.executable,'-B',str(SOURCE/'verifier.py'),'--component-prefix','--expected',str(expected),
                 '--output',str(self.root/'observed.bin'),'--receipt',str(self.root/'verifier.json'),
                 '--diagnostic',str(self.root/'worker.stderr'),'--',sys.executable,'-B',str(worker)]
        receipt=self.acc.run(command,self.root/'v.stdout',self.root/'v.stderr',env=env,role='runtime')
        self.assertEqual(receipt['exit_code'],0,(self.root/'v.stderr').read_text())
        self.check_receipts(receipt,2)
        self.assertEqual((self.root/'observed.bin').stat().st_size,48)

    def _failure_and_prefix_survive(self,kind):
        # Surrogate V/W infrastructure hierarchy: fixed byte prefix plus a
        # completed failure journal, then wait. No candidate or G1/G2 invocation.
        script=self.root/'prefix_verifier.py'
        prefix=HEADER.pack(b'A2R1',1,0,0,0,0)+OBSERVED.pack(0,1,0,1,0,1,fold(D0,0,1),0)+b'partial'
        script.write_text('import os,time,sys\nfrom pathlib import Path\n'
            'sys.path.insert(0,'+repr(str(SOURCE))+')\nfrom evidence import write_json\n'
            'child=os.fork()\nif child==0:\n time.sleep(2)\n os._exit(0)\n'
            'Path('+repr(str(self.root/'observed.bin'))+').write_bytes('+repr(prefix)+')\n'
            'write_json('+repr(str(self.root/'normal.failure.json'))+',{\'status_id\':65,\'probe\':0,\'message\':\'fixed infrastructure mismatch\'})\n'
            'os.waitpid(child,0)\n')
        real=time.monotonic_ns
        if kind=='deadline':
            self.acc.clock=lambda:real()+(DEADLINE_NS+1 if (self.root/'normal.failure.json').exists() else 0)
        else:
            original=self.acc.process
            failed=False
            def missing(pid):
                nonlocal failed
                if (self.root/'normal.failure.json').exists() and pid!=os.getpid() and not failed:
                    failed=True
                    raise AccountingUnavailable('fixture /proc lost after completed failure')
                return original(pid)
            self.acc.process=missing
        with self.assertRaises(ResourceExceeded if kind=='deadline' else AccountingUnavailable):
            self.acc.run([sys.executable,'-B',str(script)],self.root/'v.stdout',self.root/'v.stderr',role='runtime')
        self.assertEqual(preserve_failure_status(self.root,71),65)
        result=recover_prefix(self.root/'observed.bin')
        self.assertEqual(result['records'],1);self.assertEqual(result['partial_bytes'],7)
        self.assertEqual((self.root/'observed.bin.partial.stderr').read_bytes(),b'partial')
        self.assertEqual(HEADER.unpack((self.root/'observed.bin').read_bytes()[:16])[-1],1)
        self.assertTrue(all(t['reaped'] for t in self.acc.jobs[-1]['tasks'].values()))
        self.notes.update(fault_injection=kind+' stop after durable fixed failure journal',prefix_records=1,partial_bytes=7,preserved_status_id=65)

    def test_failure_and_prefix_survive_deadline_tree_termination(self):
        self._failure_and_prefix_survive('deadline')

    def test_failure_and_prefix_survive_accounting_tree_termination(self):
        self._failure_and_prefix_survive('accounting')

    def test_finalization_and_seal_inside_limits(self):
        manifest=manifest_template('a'*64,'b'*64,'c'*40)
        (self.root/'fixed.txt').write_text('bounded component artifact\n')
        finalize(self.acc,self.root,manifest)
        verify_index(self.root)
        self.assertEqual(manifest['resources']['retained_bytes'],self.acc.check_output())
        self.notes['root_verified']=True

    def test_output_cap_during_finalization_preserves_failure(self):
        manifest=manifest_template('a'*64,'b'*64,'c'*40)
        write_json(self.root/'normal.failure.json',{'status_id':65,'probe':0,'message':'fixed completed infrastructure mismatch'})
        with (self.root/'large.stderr').open('wb') as stream:stream.truncate(8*2**20+1)
        with self.assertRaises(ResourceExceeded) as caught:finalize(self.acc,self.root,manifest)
        self.assertEqual(retain_interruption(self.acc,self.root,manifest,caught.exception),65)
        self.assertFalse((self.root/'evidence.sha256').exists())
        self.assertTrue((self.root/'normal.failure.json').exists())
        self.notes['fault_injection']='diagnostic sub-budget exceeded by sparse fixed file'

    def test_deadline_during_seal_removes_success_root(self):
        manifest=manifest_template('a'*64,'b'*64,'c'*40)
        (self.root/'fixed.txt').write_bytes(bytes(128*1024))
        finalize(self.acc,self.root,manifest)
        self.assertTrue((self.root/'evidence.sha256').exists())
        original=self.acc.checkpoint
        checkpoints=0
        def expired():
            nonlocal checkpoints
            checkpoints+=1
            if checkpoints==8:raise ResourceExceeded('fixture deadline during sealing')
            return original()
        self.acc.checkpoint=expired
        with self.assertRaises(ResourceExceeded) as caught:finalize(self.acc,self.root,manifest)
        status=retain_interruption(self.acc,self.root,manifest,caught.exception)
        self.assertEqual(status,71)
        self.assertFalse((self.root/'evidence.sha256').exists())
        self.assertEqual((self.root/'fixed.txt').stat().st_size,128*1024)
        self.notes['fault_injection']='deadline checkpoint during hash/index sealing'


if __name__=='__main__':unittest.main()
