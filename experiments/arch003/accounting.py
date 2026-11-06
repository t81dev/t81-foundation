"""L's in-process Linux /proc collector and wait4 accounting.

Unsupported hosts fail closed. This is sampled monitoring plus conservative
peak rejection, not instantaneous containment. No helper monitoring processes.
"""
import os
from pathlib import Path
import resource
import signal
import subprocess
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
    def __init__(self, directory, *, clock=time.monotonic_ns):
        self.clock=clock
        self.start=clock()
        self.directory=Path(directory)
        self.peak_bound=0
        self.samples=[]
        self.jobs=[]
        self.complete=True
        self.last_sample=None
        if sys.platform!='linux' or not Path('/proc/self/status').is_file() or not hasattr(os,'wait4'):
            self.complete=False
            raise AccountingUnavailable('BLOCKED_RESOURCE_ACCOUNTING: Linux /proc + wait4 required; Darwin/Windows collectors not qualified')

    @staticmethod
    def process(pid):
        try:
            status=Path(f'/proc/{pid}/status').read_text()
            fields=dict(line.split(':',1) for line in status.splitlines() if ':' in line)
            rss=fields['VmRSS'].split();peak=fields['VmHWM'].split()
            if rss[1]!='kB' or peak[1]!='kB':raise ValueError('RSS units')
            # All tasks' children are included, covering sanitizer maintenance threads.
            children=set()
            for task in Path(f'/proc/{pid}/task').iterdir():
                children.update(int(p) for p in (task/'children').read_text().split())
            return dict(pid=pid,rss_bytes=int(rss[0])*1024,peak_bytes=int(peak[0])*1024,children=sorted(children),threads=int(fields['Threads']))
        except (OSError,ValueError,KeyError) as exc:
            # A racing process exit is not silently charged as zero.
            raise AccountingUnavailable(f'cannot account PID {pid}: {exc}') from exc

    def sample(self, max_descendants):
        now=self.clock()
        if self.last_sample is not None and now-self.last_sample>100_000_000:
            self.complete=False
            raise AccountingUnavailable('RSS sample interval exceeded 100 ms')
        self.last_sample=now
        pending=[os.getpid()];rows=[];seen=set()
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
        # Bounded summary, not one growing Python object per sample.
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

    def run(self, command, stdout_path, stderr_path, *, env=None, compiler=False):
        if compiler:
            # A driver-tree maximum is not the required per-process tool receipt.
            # Do not execute a campaign build with incomplete accounting.
            self.complete=False
            raise AccountingUnavailable('BLOCKED_RESOURCE_ACCOUNTING: compiler-tool per-process peak receipt hook is not implemented')
        self.last_sample=None
        with open(stdout_path,'wb') as out,open(stderr_path,'wb') as err:
            child=subprocess.Popen(command,stdout=out,stderr=err,env=env,start_new_session=True)
            caught=None
            try:
                while True:
                    self.sample(2)  # L + driver/tool OR L + V/W
                    pid,status,usage=os.wait4(child.pid,os.WNOHANG)
                    if pid:break
                    time.sleep(0.025)
            except (AccountingUnavailable,ResourceExceeded) as exc:
                caught=exc
                try:os.killpg(child.pid,signal.SIGKILL)
                except ProcessLookupError:pass
                pid,status,usage=os.wait4(child.pid,0)
            child.returncode=os.waitstatus_to_exitcode(status)
        owner_peak=int(resource.getrusage(resource.RUSAGE_SELF).ru_maxrss)*1024
        # Linux wait4 ru_maxrss bounds each member of the reaped child's tree,
        # including waited descendants. Charge it twice for the two-role roster.
        # This intentionally double-counts a sequential child/tool conservative bound.
        bound=owner_peak+2*int(usage.ru_maxrss)*1024
        self.peak_bound=max(self.peak_bound,bound)
        receipt=dict(command=list(command),exit_code=child.returncode,wait4_peak_raw=int(usage.ru_maxrss),
                     peak_units='KiB',descendant_role_bound=2,aggregate_bound_bytes=bound,
                     collector='Linux /proc status VmRSS/VmHWM, all task children; wait4 ru_maxrss',
                     sample_interval_ns=25_000_000,compiler=compiler)
        self.jobs.append(receipt)
        if caught:raise caught
        if bound>RSS_CAP:raise ResourceExceeded('retrospective peak bound exceeded')
        self.check_output()
        if self.clock()-self.start>DEADLINE_NS:raise ResourceExceeded('inclusive deadline exceeded')
        return receipt

    def resources(self):
        return dict(rss_peak_bound_bytes=self.peak_bound,elapsed_ns=self.clock()-self.start,
                    retained_bytes=self.check_output(),accounting_complete=self.complete)
