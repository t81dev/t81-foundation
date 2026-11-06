"""In-process Linux ptrace receipt collector (little-endian AArch64/x86-64).

Birth/exec/exit stops close the polling race. Before every exec syscall and at
exit-stop, read the process's own VmHWM before the address space disappears.
wait4 receipts are also retained, but their descendant maximum is NOT presented
as the driver's own peak. No preload, shell wrapper, monitor child or helper.
"""
import ctypes
import errno
import os
from pathlib import Path
import signal
import struct
import time

from accounting import AccountingUnavailable

TRACEME, SYSCALL, SETOPTIONS, GETEVENTMSG, GET_SYSCALL_INFO = 0, 24, 0x4200, 0x4201, 0x420e
OPTIONS = 1 | 2 | 4 | 8 | 16 | 64 | (1 << 20)  # syscall, fork/vfork/clone/exec/exit, EXITKILL
WALL = 0x40000000
EXEC_SYSCALLS = {0xc000003e: (59, 322), 0xc00000b7: (221, 281)}


def read_process(pid, proc=Path('/proc')):
    """Identity plus raw KiB values; unavailable data is an error, never zero."""
    try:
        status = (proc/str(pid)/'status').read_text()
        fields = dict(line.split(':', 1) for line in status.splitlines() if ':' in line)
        stat = (proc/str(pid)/'stat').read_text()
        tail = stat[stat.rindex(')')+2:].split()
        values = {}
        for name in ('VmRSS', 'VmHWM'):
            number, unit = fields[name].split()
            if unit != 'kB' or int(number) < 0:
                raise ValueError('RSS unit/value')
            values[name] = int(number)
        children = set()
        for task in (proc/str(pid)/'task').iterdir():
            children.update(int(p) for p in (task/'children').read_text().split())
        return dict(pid=int(fields['Pid']), tgid=int(fields['Tgid']), ppid=int(fields['PPid']),
                    start_ticks=int(tail[19]), rss_bytes=values['VmRSS']*1024,
                    peak_bytes=values['VmHWM']*1024, vmhwm_raw_kib=values['VmHWM'],
                    children=sorted(children), threads=int(fields['Threads']))
    except (OSError, ValueError, KeyError, IndexError) as exc:
        raise AccountingUnavailable(f'/proc identity/RSS unavailable for PID {pid}: {exc}') from exc


class LinuxTrace:
    def __init__(self, accountant, role):
        self.acc = accountant
        if role not in ('compiler','runtime','metadata','worker','verifier'):
            raise AccountingUnavailable('unknown accounting process role')
        self.role = role
        self.libc = ctypes.CDLL(None, use_errno=True)
        self.libc.ptrace.restype = ctypes.c_long
        self.libc.ptrace.argtypes = [ctypes.c_uint, ctypes.c_int, ctypes.c_void_p, ctypes.c_void_p]
        self.tasks = {}
        self.processes = {}
        self.events = []
        self.ordinal = 0
        self.root = None
        self.root_exit = None
        self.pending_stops = {}
        self.stopped_tasks = set()
        self.terminating = False
        self.syscall_stops = 0
        self.failure = None
        self.reader = accountant.process

    def ptrace(self, request, pid, address=0, data=0):
        ctypes.set_errno(0)
        result = self.libc.ptrace(request, pid, ctypes.c_void_p(address), ctypes.c_void_p(data))
        if result == -1:
            code = ctypes.get_errno()
            raise AccountingUnavailable(f'ptrace request {request} PID {pid}: {os.strerror(code)}')
        return result

    def event(self, kind, pid, **fields):
        self.ordinal += 1
        self.events.append(dict(ordinal=self.ordinal, kind=kind, pid=pid, **fields))
        if len(self.events) > 16384:
            raise AccountingUnavailable('bounded process-event ledger exhausted')

    def register(self, pid, parent, kind):
        # Record kernel-reported birth before /proc can fail. Identity/peak stay
        # explicitly unavailable until read, while cleanup can still kill/reap.
        self.tasks.setdefault(pid,dict(tgid=None,start_ticks=None,reaped=False,exit_stop=False,initial_stop=True,wait4=None))
        row = self.reader(pid)
        if row['pid'] != pid:
            raise AccountingUnavailable('PID identity mismatch')
        tgid = row['tgid']
        self.tasks[pid] = dict(tgid=tgid, start_ticks=row['start_ticks'], reaped=False,
                               exit_stop=False, initial_stop=True, wait4=None)
        self.event(kind, pid, parent=parent, tgid=tgid, start_ticks=row['start_ticks'])
        if tgid not in self.processes:
            # A process must be registered before any of its threads.
            if tgid != pid:
                raise AccountingUnavailable('thread group leader not registered')
            self.processes[tgid] = dict(pid=pid, parent=parent, start_ticks=row['start_ticks'],
                born=self.ordinal, ended=None, peak_rss_bytes=row['peak_bytes'],
                last_rss_bytes=row['rss_bytes'], peak_observations=[], threads_seen=[pid],
                exit_code=None, complete=False)
        else:
            self.processes[tgid]['threads_seen'].append(pid)
        self.observe(pid, kind)
        live = [p for p in self.processes.values() if p['ended'] is None]
        limit = 2 if self.role in ('compiler', 'runtime') else 1
        if len(live) > limit:
            raise AccountingUnavailable('unexpected descendant: process roster exceeded')
        if pid != self.root and tgid == pid:
            if self.tasks.get(parent,{}).get('tgid') != self.root or self.role not in ('compiler', 'runtime'):
                raise AccountingUnavailable('unexpected descendant: nested or forbidden worker/tool')
        return row

    def observe(self, pid, reason=None):
        row = self.reader(pid)
        task = self.tasks[pid]
        if (row['pid'], row['tgid'], row['start_ticks']) != (pid, task['tgid'], task['start_ticks']):
            raise AccountingUnavailable(f'PID reuse/identity race for {pid}')
        record = self.processes[task['tgid']]
        record['peak_rss_bytes'] = max(record['peak_rss_bytes'], row['peak_bytes'])
        record['last_rss_bytes'] = row['rss_bytes']
        if reason:
            record['peak_observations'].append(dict(tid=pid, at=reason, vmhwm_raw_kib=row['vmhwm_raw_kib'],
                                                   rss_bytes=row['rss_bytes']))
        return row

    def snapshot(self):
        rows = []
        for group, record in self.processes.items():
            if record['ended'] is not None:
                continue
            active = [pid for pid,t in self.tasks.items() if t['tgid']==group and not t['reaped'] and not t['exit_stop']]
            if active:
                rows.append(self.observe(active[0]))
            else:
                # Exit-stop already supplied an OS receipt. Charge the last live
                # RSS conservatively until all corresponding reaps are observed.
                rows.append(dict(pid=group, rss_bytes=record['last_rss_bytes'],
                                 peak_bytes=record['peak_rss_bytes'], exiting=True))
        return rows

    def resume(self, pid, sig=0):
        self.ptrace(SYSCALL, pid, data=sig)
        self.stopped_tasks.discard(pid)

    def stopped(self, pid, status):
        if pid not in self.tasks:
            if self.terminating:
                self.tasks[pid]=dict(tgid=None,start_ticks=None,reaped=False,exit_stop=False,initial_stop=True,wait4=None)
            else:
                self.pending_stops[pid] = status
                return
        task = self.tasks[pid]
        if task['tgid'] is None:
            if not self.terminating:raise AccountingUnavailable('unavailable process birth identity')
            self.ptrace(7,pid)
            self.stopped_tasks.discard(pid)
            return
        event = status >> 16
        sig = os.WSTOPSIG(status)
        if event == 6:
            # SIGKILL can reach a freshly traced child before its initial stop.
            self.observe(pid, 'exit_stop')
            task['exit_stop'] = True
            self.event('exit_stop', pid)
            self.resume(pid)
            return
        if task['initial_stop']:
            if sig != signal.SIGSTOP or event:
                raise AccountingUnavailable('missing initial trace stop')
            task['initial_stop'] = False
            self.ptrace(SETOPTIONS, pid, data=OPTIONS)
            self.resume(pid)
        elif event in (1,2,3):
            child = ctypes.c_ulong()
            self.ptrace(GETEVENTMSG, pid, data=ctypes.addressof(child))
            self.register(child.value, pid, {1:'fork',2:'vfork',3:'clone'}[event])
            if child.value in self.pending_stops:
                self.stopped(child.value, self.pending_stops.pop(child.value))
            self.resume(pid)
        elif event == 4:
            # Nonleader exec changes TIDs and needs a separately reviewed mapping.
            old = ctypes.c_ulong()
            self.ptrace(GETEVENTMSG, pid, data=ctypes.addressof(old))
            if old.value != pid:
                raise AccountingUnavailable('nonleader exec identity transition unsupported')
            self.observe(pid, 'after_exec')
            self.event('exec', pid)
            self.resume(pid)
        elif sig == signal.SIGTRAP | 0x80:
            info = ctypes.create_string_buffer(88)
            n = self.ptrace(GET_SYSCALL_INFO, pid, address=len(info), data=ctypes.addressof(info))
            if n < 24 or info.raw[0] not in (1,2):
                raise AccountingUnavailable('syscall stop information unavailable')
            self.syscall_stops += 1
            if info.raw[0] == 1:
                if n < 80:
                    raise AccountingUnavailable('truncated syscall entry information')
                arch = struct.unpack_from('<I', info.raw, 4)[0]
                if arch not in EXEC_SYSCALLS:
                    raise AccountingUnavailable('unsupported syscall audit architecture')
                number = struct.unpack_from('<Q', info.raw, 24)[0]
                if number in EXEC_SYSCALLS[arch]:
                    if task['tgid']!=pid:
                        raise AccountingUnavailable('nonleader exec unsupported before identity transition')
                    self.observe(pid, 'before_exec')
            self.resume(pid)
        elif event or sig == signal.SIGSTOP:
            raise AccountingUnavailable('unsupported trace event/group stop')
        else:
            self.resume(pid, sig)  # preserve actual signal delivery

    def reaped(self, pid, status, usage):
        if pid not in self.tasks:
            raise AccountingUnavailable(f'unregistered reaped task {pid}')
        task = self.tasks[pid]
        self.pending_stops.pop(pid,None)
        self.stopped_tasks.discard(pid)
        task['reaped'] = True
        code=os.waitstatus_to_exitcode(status)
        task['wait4'] = dict(status_raw=status, exit_code=code if code>=0 else 128-code, termination_signal=-code if code<0 else 0,
                             ru_maxrss_raw_kib=int(usage.ru_maxrss))
        self.event('reap', pid, **task['wait4'])
        if task['tgid'] is None:
            if pid==self.root:self.root_exit=task['wait4']['exit_code']
            raise AccountingUnavailable('reaped process with unavailable birth identity/peak')
        group = self.processes[task['tgid']]
        group['wait4_peak_bound_bytes']=max(group.get('wait4_peak_bound_bytes',0),int(usage.ru_maxrss)*1024)
        if pid == group['pid']:
            group['exit_code'] = task['wait4']['exit_code']
        if all(t['reaped'] for t in self.tasks.values() if t['tgid']==task['tgid']):
            group['ended'] = self.ordinal
            group['complete'] = all(t['exit_stop'] for t in self.tasks.values() if t['tgid']==task['tgid'])
        if pid == self.root:
            self.root_exit = task['wait4']['exit_code']
        if not task['exit_stop']:
            raise AccountingUnavailable(f'PID {pid} reaped without per-process peak exit-stop')

    def drain_one(self):
        try:
            pid, status, usage = os.wait4(-1, os.WNOHANG | WALL)
        except ChildProcessError:
            return False
        if not pid:
            return False
        if os.WIFSTOPPED(status):
            self.stopped_tasks.add(pid)
            self.stopped(pid,status)
        elif os.WIFEXITED(status) or os.WIFSIGNALED(status):
            self.reaped(pid,status,usage)
        else:
            raise AccountingUnavailable('unrecognized wait4 event')
        return True

    def terminate(self):
        # Kill every traced TID, including descendants that changed sessions.
        # Continue draining kernel trace events and reaping; never start a helper.
        self.terminating=True
        end=time.monotonic()+2
        while (any(not t['reaped'] for t in self.tasks.values()) or self.pending_stops) and time.monotonic()<end:
            for pid,t in list(self.tasks.items()):
                if not t['reaped']:
                    try:os.kill(pid,signal.SIGKILL)
                    except ProcessLookupError:pass
            for pid in list(self.stopped_tasks):
                try:self.ptrace(7,pid)
                except AccountingUnavailable:pass
                self.stopped_tasks.discard(pid)
            try:
                if not self.drain_one():time.sleep(0.001)
            except AccountingUnavailable:
                self.acc.complete=False
                # An exit-stop with unavailable /proc still must be resumed/reaped.
                for pid,t in list(self.tasks.items()):
                    if not t['reaped']:
                        try:self.ptrace(7,pid)  # PTRACE_CONT for termination only
                        except AccountingUnavailable:pass
        if self.root is not None and self.root not in self.tasks:
            try:os.kill(self.root,signal.SIGKILL)
            except ProcessLookupError:pass
            while time.monotonic()<end:
                try:
                    pid,status,_=os.wait4(self.root,os.WNOHANG|WALL)
                except ChildProcessError:break
                if not pid:
                    time.sleep(0.001);continue
                if os.WIFEXITED(status) or os.WIFSIGNALED(status):break
                try:self.ptrace(7,self.root)
                except AccountingUnavailable:pass
            else:
                self.acc.complete=False
                raise AccountingUnavailable('unregistered launch PID could not be reaped')
        if any(not t['reaped'] for t in self.tasks.values()) or self.pending_stops:
            self.acc.complete=False
            raise AccountingUnavailable('trace tree could not be completely reaped')

    def execute(self, command, out, err, env):
        self.root=os.fork()
        if self.root==0:
            try:
                os.setsid()
                os.dup2(out.fileno(),1);os.dup2(err.fileno(),2)
                self.ptrace(TRACEME,0)
                os.kill(os.getpid(),signal.SIGSTOP)
                os.execvpe(command[0],command,env if env is not None else os.environ)
            except BaseException as exc:
                os.write(2,('trace launch failed: '+str(exc)+'\n').encode())
                os._exit(127)
        try:
            self.register(self.root,os.getpid(),'root')
            while any(not t['reaped'] for t in self.tasks.values()):
                self.acc.checkpoint()
                if not self.drain_one():time.sleep(0.0002)
            if self.pending_stops or self.root_exit is None:
                raise AccountingUnavailable('unmatched process birth/exit events')
        except BaseException as exc:
            self.failure=exc
            self.acc.complete=False
            self.terminate()
            raise
        return self.root_exit

    def peak_sum(self):
        # Peak assigned over each process lifetime; maximum over sequential jobs
        # and tool replacements, not a sum of all historical tool peaks.
        records=list(self.processes.values())
        return max((sum(max(p['peak_rss_bytes'],p.get('wait4_peak_bound_bytes',0)) for p in records if p['born']<=point and (p['ended'] is None or point<=p['ended']))
                    for point in {p['born'] for p in records}),default=0)

    def receipt(self):
        coverage=bool(self.processes) and all(p['complete'] for p in self.processes.values()) and all(t['reaped'] and t['exit_stop'] and t['tgid'] is not None for t in self.tasks.values())
        return dict(role=self.role, root_pid=self.root, exit_code=self.root_exit,
                    processes=list(self.processes.values()), tasks={str(pid):row for pid,row in self.tasks.items()},
                    events=self.events, syscall_stops=self.syscall_stops,
                    per_process_receipts_complete=coverage,
                    descendants_peak_bound_bytes=self.peak_sum() if coverage else None,
                    observed_descendants_peak_bound_bytes=self.peak_sum(),
                    complete=self.failure is None and self.root_exit is not None and coverage)
