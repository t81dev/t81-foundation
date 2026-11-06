"""L's in-process Linux /proc collector and wait4 accounting.

Unsupported hosts fail closed. This is sampled monitoring plus conservative
peak rejection, not instantaneous containment. No helper monitoring processes.
"""
import os
from pathlib import Path
import resource
import signal
import sys
import time

RSS_CAP=512*2**20
OUTPUT_CAP=100*2**20
DEADLINE_NS=1800*10**9


class AccountingUnavailable(RuntimeError):
    pass


class ResourceExceeded(RuntimeError):
    pass


class Accountant:
    def __init__(self, directory, *, clock=time.monotonic_ns, started_ns=None):
        self.clock=clock
        self._start=clock() if started_ns is None else started_ns
        self.directory=Path(directory)
        self.peak_bound=0
        self.samples=[]
        self.jobs=[]
        self.complete=True
        self.last_sample=None
        self.max_sample_gap_ns=0
        self.sample_count=0
        if sys.platform!='linux' or not Path('/proc/self/status').is_file() or not hasattr(os,'wait4'):
            self.complete=False
            raise AccountingUnavailable('BLOCKED_RESOURCE_ACCOUNTING: Linux /proc + wait4 required; Darwin/Windows collectors not qualified')

    @property
    def start(self):
        return self._start

    @staticmethod
    def process(pid):
        from linux_trace import read_process
        return read_process(pid)

    def checkpoint(self):
        now=self.clock()
        if now-self.start>DEADLINE_NS:
            raise ResourceExceeded('inclusive deadline exceeded')
        if self.last_sample is None or now-self.last_sample>=25_000_000:
            self.sample(2)
        elif now-self.start>DEADLINE_NS:
            raise ResourceExceeded('inclusive deadline exceeded')

    def sample(self, max_descendants):
        try:
            return self._sample(max_descendants)
        except AccountingUnavailable:
            self.complete=False
            raise

    def _sample(self, max_descendants):
        now=self.clock()
        gap=0 if self.last_sample is None else now-self.last_sample
        self.max_sample_gap_ns=max(getattr(self,'max_sample_gap_ns',0),gap)
        self.sample_count=getattr(self,'sample_count',0)+1
        if gap>100_000_000:
            self.complete=False
            raise AccountingUnavailable('RSS sample interval exceeded 100 ms')
        self.last_sample=now
        owner=self.process(os.getpid())
        trace=getattr(self,'_trace',None)
        if trace is not None:
            rows=[owner,*trace.snapshot()]
        else:
            rows=[owner]
            pending=list(owner['children'])
            seen={owner['pid']}
            while pending:
                pid=pending.pop()
                if pid in seen:continue
                seen.add(pid)
                row=self.process(pid);rows.append(row);pending.extend(row['children'])
        if len(rows)-1>max_descendants:
            self.complete=False
            raise AccountingUnavailable('unexpected descendant roster')
        live=sum(r['rss_bytes'] for r in rows)
        self.peak_bound=max(self.peak_bound,live)
        self.samples=rows
        if live>RSS_CAP:raise ResourceExceeded('aggregate absolute RSS exceeded')
        if now-self.start>DEADLINE_NS:raise ResourceExceeded('inclusive deadline exceeded')
        self.check_output()
        return rows

    def check_output(self):
        total=0
        buckets={'numeric':0,'commands':0,'diagnostics':0,'manifests':0}
        for p in self.directory.rglob('*'):
            if not p.is_file():continue
            size=p.stat().st_size;total+=size
            relative=p.relative_to(self.directory)
            if relative.parts[0].startswith('.arch003-build-'):continue
            if p.suffix=='.bin':bucket='numeric'
            elif p.suffix=='.stderr' or 'assembly' in p.name:bucket='diagnostics'
            elif p.suffix=='.stdout' or p.name[:1].isdigit() or p.name.startswith('build-') or p.name in ('host.json','authorization.json','binaries.json'):bucket='commands'
            else:bucket='manifests'
            buckets[bucket]+=size
        for bucket,cap in {'numeric':84,'commands':4,'diagnostics':8,'manifests':2}.items():
            if buckets[bucket]>cap*2**20:raise ResourceExceeded(bucket+' output sub-budget exceeded')
        if total>OUTPUT_CAP:raise ResourceExceeded('retained output exceeded')
        return total

    def run(self, command, stdout_path, stderr_path, *, env=None, compiler=False, role=None):
        from linux_trace import LinuxTrace
        if not self.complete:raise AccountingUnavailable('incomplete accountant cannot launch replacement work')
        self.sample(0)  # No unobserved child may predate trace installation.
        trace=LinuxTrace(self,role or ('compiler' if compiler else 'metadata'))
        self._trace=trace
        caught=None
        # Do not reset last_sample or start between jobs.
        try:
            self.checkpoint()
            with open(stdout_path,'wb') as out,open(stderr_path,'wb') as err:
                trace.execute([str(c) for c in command],out,err,env)
        except BaseException as exc:
            caught=exc
            if isinstance(exc,AccountingUnavailable):self.complete=False
        finally:
            self._trace=None
        owner_peak=int(resource.getrusage(resource.RUSAGE_SELF).ru_maxrss)*1024
        receipt=trace.receipt()
        descendant_bound=receipt['descendants_peak_bound_bytes']
        bound=owner_peak+descendant_bound if descendant_bound is not None else None
        if bound is not None:self.peak_bound=max(self.peak_bound,bound)
        receipt.update(command=list(command),aggregate_bound_bytes=bound,
                       owner_peak_rss_raw_kib=owner_peak//1024,
                       collector='Linux ptrace birth/exec/exit stops + /proc VmHWM; wait4 at each reap',
                       raw_peak_units='KiB',conversion_to_bytes=1024,sample_interval_ns=25_000_000,
                       compiler=compiler,observed_max_sample_gap_ns=self.max_sample_gap_ns,sample_count=self.sample_count)
        # Persist an incomplete or failed job receipt before propagating any cap.
        self.jobs.append(receipt)
        if caught:raise caught
        if not receipt['complete']:
            self.complete=False
            raise AccountingUnavailable('incomplete per-process peak receipts')
        if bound>RSS_CAP:raise ResourceExceeded('retrospective peak bound exceeded')
        self.checkpoint()
        return receipt

    def resources(self):
        owner_peak=int(resource.getrusage(resource.RUSAGE_SELF).ru_maxrss)*1024
        self.peak_bound=max(self.peak_bound,owner_peak+max((j['descendants_peak_bound_bytes'] for j in self.jobs if j['descendants_peak_bound_bytes'] is not None),default=0))
        if self.peak_bound>RSS_CAP:raise ResourceExceeded('retrospective owner peak bound exceeded')
        self.checkpoint()
        return dict(rss_peak_bound_bytes=self.peak_bound if self.complete else None,elapsed_ns=self.clock()-self.start,
                    retained_bytes=self.check_output(),accounting_complete=self.complete)
