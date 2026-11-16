"""Linux source candidate only. Host qualification and execution remain gated."""

import argparse
import ctypes
import hashlib
import importlib.util
import json
import os
import resource
import selectors
import signal
import subprocess
import sys
import sysconfig
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent
MOUNT = Path('/sys/fs/cgroup')
MIB = 1024**2


def require(condition, reason):
    if not condition:
        raise ValueError('BLOCKED: ' + reason)


def digest(path, guard=None):
    result = hashlib.sha256()
    with path.open('rb') as stream:
        while chunk := stream.read(65536):
            if guard is not None:
                guard()
            result.update(chunk)
    return result.hexdigest()


def bounded_json(path, max_bytes=32768):
    def unique(pairs):
        result = {}
        for key, value in pairs:
            require(key not in result, 'duplicate metadata key')
            result[key] = value
        return result
    with path.open('rb') as stream:
        raw = stream.read(max_bytes + 1)
    require(len(raw) <= max_bytes, 'oversized gate input')
    return json.loads(raw, object_pairs_hook=unique)


def identity(pid):
    fields = Path('/proc/' + str(pid) + '/stat').read_text().rsplit(')', 1)[1].split()
    return {'pid': pid, 'start': int(fields[19]), 'ppid': int(fields[1]),
            'pgid': int(fields[2]), 'sid': int(fields[3])}


def membership(pid):
    rows = Path('/proc/' + str(pid) + '/cgroup').read_text().splitlines()
    require(len(rows) == 1 and rows[0].startswith('0::/'), 'unknown cgroup mapping')
    return MOUNT / rows[0][3:].lstrip('/')


class Group:
    """Directory-FD anchoring; never repair topology or controller configuration."""

    def __init__(self, path):
        require(path.resolve() == path, 'noncanonical cgroup path')
        self.path = path
        self.fd = os.open(path, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
        self.inode = os.fstat(self.fd).st_ino
        self.device = os.fstat(self.fd).st_dev
        if os.fstat(self.fd).st_uid != os.geteuid():
            os.close(self.fd)
            raise ValueError('BLOCKED: cgroup delegation owner')

    def unchanged(self):
        value = self.path.stat()
        require((value.st_dev, value.st_ino) == (self.device, self.inode), 'cgroup replaced')

    def read(self, name):
        fd = os.open(name, os.O_RDONLY | os.O_NOFOLLOW, dir_fd=self.fd)
        try:
            raw = os.read(fd, 8193)
            require(len(raw) <= 8192, 'cgroup inventory overflow')
            return raw.decode('ascii').strip()
        finally:
            os.close(fd)

    def write(self, name, raw):
        fd = os.open(name, os.O_WRONLY | os.O_NOFOLLOW, dir_fd=self.fd)
        try:
            require(os.write(fd, raw) == len(raw), 'short cgroup write')
        finally:
            os.close(fd)

    def procs(self):
        values = self.read('cgroup.procs').split()
        require(len(values) <= 8, 'unexpected process population')
        return {int(value) for value in values}

    def children(self):
        children = set()
        with os.scandir(self.fd) as entries:
            for entry in entries:
                require(not entry.is_symlink(), 'cgroup symlink')
                if entry.is_dir(follow_symlinks=False):
                    children.add(entry.name)
                    require(len(children) <= 2, 'unexpected nested cgroups')
        return children

    def cpu(self):
        fields = self.read('cpu.stat').split()
        return int(dict(zip(fields[::2], fields[1::2]))['usage_usec'])

    def populated(self):
        fields = self.read('cgroup.events').split()
        return int(dict(zip(fields[::2], fields[1::2]))['populated'])

    def close(self):
        os.close(self.fd)


def topology(groups, pending=None):
    aggregate, supervisor, runner = groups
    for group in groups:
        group.unchanged()
        require(group.read('cgroup.type') == 'domain', 'non-domain cgroup')
    require(aggregate.children() == {'supervisor', 'runner'}, 'unknown aggregate children')
    require(not supervisor.children() and not runner.children(), 'non-leaf execution groups')
    require(not aggregate.procs() and supervisor.procs() == {os.getpid()}, 'internal/unowned PID')
    require(runner.procs() == (set() if pending is None else {pending['pid']}), 'G0/G1 runner population')
    require(membership(os.getpid()) == supervisor.path, 'supervisor membership')
    parent = aggregate.path
    ancestors = []
    for _ in range(32):
        enabled = set((parent / 'cgroup.subtree_control').read_text().split())
        require({'cpu', 'memory', 'cpuset'} <= enabled, 'controllers not enabled top-down')
        if parent != MOUNT:
            observation = {'path': str(parent)}
            for name in ('memory.max', 'memory.swap.max', 'cpu.max', 'cpuset.cpus.effective'):
                value = (parent / name).read_text().strip()
                require(0 < len(value) <= 1024, 'ancestor limit inventory')
                observation[name] = value
            ancestors.append(observation)
        if parent == MOUNT:
            break
        parent = parent.parent
    else:
        raise ValueError('BLOCKED: ancestor depth')
    cpus = os.sched_getaffinity(0)
    require(len(cpus) == 1, 'single CPU affinity required')
    cpu = str(next(iter(cpus)))
    for group, limit in zip(groups, (256, 64, 128)):
        require(group.read('memory.max') == str(limit * MIB), 'memory ceiling')
        require(group.read('memory.swap.max') == '0' and group.read('memory.swap.current') == '0', 'swap')
        require(group.read('cpuset.cpus.effective') == cpu, 'singleton effective cpuset')
        require(bool(group.read('cpuset.mems.effective')), 'empty memory node set')
    quota, period = aggregate.read('cpu.max').split()
    require(quota != 'max' and 0 < int(quota) <= int(period), 'one CPU quota')
    if (aggregate.path / 'cpu.max.burst').exists():
        require(aggregate.read('cpu.max.burst') == '0', 'CPU burst')
    with os.scandir('/proc/self/task') as entries:
        tids = []
        for entry in entries:
            tids.append(int(entry.name))
            require(len(tids) <= 8, 'thread inventory cap')
    for tid in tids:
        require(os.sched_getaffinity(tid) == cpus, 'thread affinity mismatch')
        require(os.sched_getscheduler(tid) in (os.SCHED_OTHER, os.SCHED_BATCH, os.SCHED_IDLE), 'scheduler')
    require(os.access('cgroup.kill', os.W_OK, dir_fd=runner.fd), 'cgroup.kill unavailable')
    if pending is not None:
        require(identity(pending['pid']) == pending, 'pending process identity changed')
        require(membership(pending['pid']) == runner.path, 'pending process escaped')
        require(resource.prlimit(pending['pid'], resource.RLIMIT_CPU) == (44, 45), 'runner CPU kernel readback')
        require(os.sched_getaffinity(pending['pid']) == cpus, 'runner affinity')
        require(os.sched_getscheduler(pending['pid']) in (os.SCHED_OTHER, os.SCHED_BATCH, os.SCHED_IDLE), 'runner scheduler')
    return ancestors


def checkpoint(launch, aggregate):
    require(time.monotonic_ns() - launch['started_monotonic_ns'] < 58_000_000_000, 'suite wall guard')
    require(aggregate.cpu() < 48_000_000, 'whole-invocation CPU guard')


def g0(qualification_path, launch_path):
    require(sys.platform == 'linux' and sys.flags.isolated and sys.flags.no_site and __debug__, 'Linux -I -S without -O required')
    require(os.geteuid() != 0, 'unprivileged delegated operator required')
    capability_fields = dict(row.split(':', 1) for row in Path('/proc/self/status').read_text().splitlines() if ':' in row)
    require(all(int(capability_fields[name].strip(), 16) == 0 for name in ('CapEff', 'CapPrm', 'CapAmb')), 'capability-bearing launcher')
    for name in ('cgroup', 'mnt'):
        require(os.readlink('/proc/self/ns/' + name) == os.readlink('/proc/1/ns/' + name), 'unsupported namespace mapping')
    require(os.readlink('/proc/self/ns/time') == os.readlink('/proc/1/ns/time'), 'time namespace')
    mounts = Path('/proc/self/mountinfo').read_text().splitlines()
    require(any(row.split()[3:5] == ['/', str(MOUNT)] and row.split(' - ')[1].split()[0] == 'cgroup2'
                for row in mounts), 'unsupported cgroup mount namespace')
    launch = bounded_json(launch_path, 8192)
    require(set(launch) == {'aggregate_path', 'aggregate_inode', 'cpu_origin_usec',
                           'started_monotonic_ns', 'boot_id', 'invocation_id', 'manager_unit',
                           'manager_scope'}, 'launch record fields')
    require(launch['manager_scope'] in ('system', 'user'), 'manager scope')
    require(launch['invocation_id'] == os.environ.get('INVOCATION_ID')
            and len(launch['invocation_id']) == 32, 'invocation identity')
    require(launch['boot_id'] == Path('/proc/sys/kernel/random/boot_id').read_text().strip(), 'launch boot')
    require(launch['cpu_origin_usec'] == 0, 'fresh aggregate zero origin required')
    require(0 <= time.monotonic_ns() - launch['started_monotonic_ns'] < 58_000_000_000, 'launch time')
    qualification = bounded_json(qualification_path, 8192)
    path = Path(qualification['aggregate_cgroup'])
    require(str(path) == launch['aggregate_path'] and MOUNT in path.parents, 'aggregate launch mapping')
    groups = []
    try:
        for item in (path, path / 'supervisor', path / 'runner'):
            groups.append(Group(item))
        require(groups[0].inode == launch['aggregate_inode'], 'aggregate launch inode')
        topology(groups)
        checkpoint(launch, groups[0])
        hard = resource.getrlimit(resource.RLIMIT_CPU)[1]
        require(hard == resource.RLIM_INFINITY or hard >= 45, 'inherited hard CPU below 45')
        usage = resource.getrusage(resource.RUSAGE_SELF)
        require(usage.ru_utime + usage.ru_stime < 1, 'startup CPU guard')
        libc = ctypes.CDLL(None, use_errno=True)
        require(libc.prctl(38, 1, 0, 0, 0) == 0 and libc.prctl(39, 0, 0, 0, 0) == 1, 'no-new-privileges')
        require(libc.prctl(36, 1, 0, 0, 0) == 0, 'subreaper unavailable')
        value = ctypes.c_int()
        require(libc.prctl(37, ctypes.byref(value), 0, 0, 0) == 0 and value.value == 1, 'subreaper readback')
        return launch, groups
    except BaseException:
        for group in groups:
            group.close()
        raise


def command(argv, launch, aggregate):
    """Bound metadata helpers in S; they are charged to D, never escape it."""
    process = subprocess.Popen(argv, cwd=ROOT, env={'PATH': os.defpath, 'XDG_RUNTIME_DIR': '/run/user/' + str(os.geteuid())},
                               stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)
    result = bytearray()
    deadline = time.monotonic() + 2
    selector = selectors.DefaultSelector()
    selector.register(process.stdout, selectors.EVENT_READ)
    try:
        while selector.get_map():
            checkpoint(launch, aggregate)
            require(time.monotonic() < deadline, 'metadata helper deadline')
            for key, _ in selector.select(0.01):
                raw = os.read(key.fileobj.fileno(), 4096)
                if not raw:
                    selector.unregister(key.fileobj)
                result.extend(raw)
                require(len(result) <= 65536, 'metadata helper output cap')
        require(process.wait(timeout=0.2) == 0, 'metadata helper failed')
        return result.decode('utf-8')
    finally:
        if process.poll() is None:
            process.kill()
        process.wait(timeout=0.2)
        process.stdout.close()
        selector.close()


def manager_gate(launch, aggregate):
    unit = launch['manager_unit']
    require(unit.endswith('.service') and '/' not in unit and len(unit) <= 128, 'manager unit')
    properties = ('MainPID InvocationID ControlGroup Type ActiveState Restart KillMode SendSIGKILL '
                  'RemainAfterExit NotifyAccess RuntimeMaxUSec RuntimeRandomizedExtraUSec '
                  'TimeoutStartUSec TimeoutStopUSec ExecStartPre ExecStartPost ExecStop ExecStopPost ExecReload').split()
    argv = ['/usr/bin/systemctl'] + (['--user'] if launch['manager_scope'] == 'user' else [])
    raw = command(argv + ['show', unit, '--property=' + ','.join(properties)], launch, aggregate)
    observed = dict(row.split('=', 1) for row in raw.splitlines())
    expected = {'MainPID': str(os.getpid()), 'InvocationID': launch['invocation_id'],
                'ControlGroup': '/' + str(aggregate.path.relative_to(MOUNT)),
                'Type': 'exec', 'ActiveState': 'active', 'Restart': 'no',
                'KillMode': 'control-group', 'SendSIGKILL': 'yes',
                'RemainAfterExit': 'no', 'NotifyAccess': 'none'}
    require(all(observed.get(key) == value for key, value in expected.items()), 'manager lifecycle profile')
    require(all(observed.get(key) == '' for key in ('ExecStartPre', 'ExecStartPost', 'ExecStop', 'ExecStopPost', 'ExecReload')),
            'manager hooks outside minimal profile')
    def seconds(value):
        if value == '0':
            return 0.0
        for suffix, scale in (('ms', .001), ('us', .000001), ('s', 1)):
            if value.endswith(suffix):
                return float(value[:-len(suffix)]) * scale
        raise ValueError('BLOCKED: unsupported manager duration')
    require(0 < seconds(observed['RuntimeMaxUSec']) <= 55, 'outer runtime deadline')
    require(seconds(observed['RuntimeRandomizedExtraUSec']) == 0, 'randomized deadline')
    require(all(0 < seconds(observed[key]) <= 1 for key in ('TimeoutStartUSec', 'TimeoutStopUSec')), 'outer activation/stop bound')
    return observed


def local_module(name):
    specification = importlib.util.spec_from_file_location(name, ROOT / (name + '.py'))
    module = importlib.util.module_from_spec(specification)
    specification.loader.exec_module(module)
    return module


def check_gate(authorization_path, qualification_path, launch, groups, pending):
    topology(groups, pending)
    require(resource.getrlimit(resource.RLIMIT_CPU) == (2, 3), 'supervisor CPU readback')
    manager = manager_gate(launch, groups[0])
    require(not bounded_json(ROOT / 'dependency-lock.json', MIB)['linux_relock_required'], 'reviewed Linux re-lock required')
    library = Path(sysconfig.get_path('purelib'))
    require(str(library) == sysconfig.get_path('platlib') and library.is_dir(), 'reviewed import profile')
    for directory in set(sys.path) | {str(library)}:
        path = Path(directory)
        if path.is_dir():
            require(not any(path.glob('*.pth')) and not (path / 'sitecustomize.py').exists()
                    and not (path / 'usercustomize.py').exists(), 'site startup hooks')
    sys.path.append(str(library))  # No site.main(), .pth evaluation or user-site imports.
    from jsonschema import Draft202012Validator
    authorization = bounded_json(authorization_path, 8192)
    qualification = bounded_json(qualification_path, 8192)
    schemas = bounded_json(ROOT / 'evidence.schema.json', 65536)
    for name, record in (('authorization', authorization), ('qualification', qualification)):
        Draft202012Validator(schemas['$defs'][name]).validate(record)
        plan = bounded_json(ROOT / 'fa01-plan.json')
        for key in ('corpus_sha256', 'amendment_sha256', 'protocol_sha256'):
            require(record[key] == plan[key], 'stale FA01 ' + key)
        require(record['area_f_plan_id'] == plan['id'], 'F plan')
    require(authorization['scope'] == 'one-bounded-run' and authorization['human_record'], 'human execution record')
    local_module('interpretation_policy').validate(authorization['interpretations'], bounded_json(ROOT / 'interpretations.json')['choices'])
    head = command(['git', 'rev-parse', 'HEAD'], launch, groups[0]).strip()
    require(not command(['git', 'status', '--porcelain'], launch, groups[0]), 'dirty source')
    require(head == authorization['harness_sha'] == qualification['harness_sha'], 'source identity')
    protocol = ROOT.parents[1] / 'docs/research/proposals/RFC-00D7-F01/PROTOCOL.md'
    require(digest(protocol) == authorization['protocol_sha256'], 'protocol identity')
    require(qualification['approved_by'] and qualification['boot_id'] == launch['boot_id'], 'qualification boot/approval')
    require(qualification['kernel_release'] == os.uname().release, 'kernel changed')
    require(qualification['dependency_lock_sha256'] == digest(ROOT / 'dependency-lock.json'), 'dependency identity')
    command([sys.executable, '-B', str(ROOT / 'static_check.py')], launch, groups[0])
    checkpoint(launch, groups[0])
    manager['ancestor_limits'] = topology(groups, pending)
    return authorization, qualification, manager


def wait_ready(process, fd, launch, groups):
    selector = selectors.DefaultSelector()
    selector.register(fd, selectors.EVENT_READ)
    deadline = time.monotonic() + 5
    raw = bytearray()
    try:
        while b'\n' not in raw:
            checkpoint(launch, groups[0])
            require(process.poll() is None and time.monotonic() < deadline, 'bootstrap exit/timeout')
            for _, _ in selector.select(0.01):
                chunk = os.read(fd, 1025 - len(raw))
                require(bool(chunk), 'bootstrap EOF')
                raw.extend(chunk)
                require(len(raw) <= 1024, 'READY cap')
        ready = json.loads(raw)
        pending = identity(process.pid)
        require(pending['ppid'] == os.getpid() and pending['pgid'] == pending['sid'] == process.pid, 'runner process identity')
        require(ready == dict(pending, cpu=[44, 45], runner_inode=groups[2].inode), 'READY readback')
        return pending
    finally:
        selector.close()


def cleanup(process, pending, groups, deadline):
    runner = groups[2]
    runner.unchanged()
    identity_failure = False
    if pending is not None and process.poll() is None:
        try:
            safe = identity(process.pid) == pending and membership(process.pid) == runner.path
            if safe:
                os.killpg(process.pid, signal.SIGKILL)
            else:
                identity_failure = True
        except ProcessLookupError:
            pass
        except FileNotFoundError:
            pass  # Exit between poll and /proc read; R kill/reaping still required.
        except Exception:
            identity_failure = True
    # Also catches descendants which changed process groups/sessions. Never kill D
    # from S. Before READY a stub can still be in S; terminate that owned child too.
    if process.poll() is None:
        process.kill()
    runner.write('cgroup.kill', b'1')
    while time.monotonic() < deadline:
        for _ in range(16):
            try:
                pid, wait_status = os.waitpid(-1, os.WNOHANG)
            except ChildProcessError:
                break
            if pid == 0:
                break
            if pid == process.pid:
                # Do not let Popen mistake an already reaped nonzero child for0.
                process.returncode = os.waitstatus_to_exitcode(wait_status)
        direct_done = process.poll() is not None
        children = Path('/proc/self/task/' + str(os.getpid()) + '/children').read_text().split()
        require(len(children) <= 16, 'orphan inventory overflow')
        if direct_done and not children and runner.populated() == 0 and not runner.procs():
            return {'complete': not identity_failure, 'runner_populated': 0,
                    'adopted_children': 0, 'identity_failure': identity_failure}
        time.sleep(0.005)
    return {'complete': False, 'reason': 'cleanup_deadline', 'identity_failure': identity_failure}


def persist(output, record, authorization, qualification, launch, aggregate):
    def final_guard():
        require(time.monotonic_ns() - launch['started_monotonic_ns'] < 59_000_000_000, 'finalization wall guard')
        require(aggregate.cpu() < 49_000_000, 'finalization CPU guard')
    final_guard()
    # Reserve supporting bytes before writing; no schema/result payload changes.
    total, count = 0, 0
    with os.scandir(output) as entries:
        for entry in entries:
            final_guard()
            require(entry.is_file(follow_symlinks=False), 'unsafe existing artifact')
            total += entry.stat(follow_symlinks=False).st_size
            count += 1
            require(count <= 251 and total + 131072 <= 25 * MIB, 'supporting reservation unavailable')
    # Exclusive bounded writes retain successful earlier artifacts on failure.
    for name, value in (('supervisor.json', record), ('authorization-record.json', authorization),
                        ('qualification-record.json', qualification), ('launch-record.json', launch)):
        final_guard()
        raw = (json.dumps(value, sort_keys=True) + '\n').encode()
        require(len(raw) <= 16384, 'supporting evidence cap')
        with (output / name).open('xb') as stream:
            stream.write(raw)
    paths = []
    with os.scandir(output) as entries:
        for entry in entries:
            final_guard()
            require(entry.is_file(follow_symlinks=False) and entry.name.isascii()
                    and all(char.isalnum() or char in '._-' for char in entry.name), 'unsafe artifact')
            paths.append(Path(entry.path))
            require(len(paths) <= 256, 'artifact count cap')
    total = sum(path.stat().st_size for path in paths)
    require(total + 65536 <= 25 * MIB, 'artifact reservation unavailable')
    manifest = ''.join(digest(path, final_guard) + '  ' + path.name + '\n' for path in sorted(paths)).encode()
    require(len(manifest) <= 65536, 'manifest cap')
    final_guard()
    with (output / 'SHA256SUMS').open('xb') as stream:
        stream.write(manifest)
    final_guard()


def main():
    parser = argparse.ArgumentParser()
    for name in ('authorization', 'qualification', 'launch-record', 'output'):
        parser.add_argument('--' + name, type=Path, required=True)
    args = parser.parse_args()
    groups, launch, process, pending = [], None, None, None
    authorization, qualification, manager = None, None, None
    go_read = go_write = ready_read = ready_write = None
    selector = None
    cleanup_record = {'complete': False, 'reason': 'not_started'}
    status, reason, current_area = 'BLOCKED', 'startup_not_completed', None
    stderr, status_buffer = bytearray(), bytearray()
    output = args.output.resolve()
    try:
        launch, groups = g0(args.qualification, args.launch_record)
        def alarm_handler(signum, frame):
            raise TimeoutError('whole-invocation wall guard')
        signal.signal(signal.SIGALRM, alarm_handler)
        signal.setitimer(signal.ITIMER_REAL, max(.001, 58 - (time.monotonic_ns() - launch['started_monotonic_ns']) / 10**9))
        resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
        resource.setrlimit(resource.RLIMIT_FSIZE, (25 * MIB, 25 * MIB))
        go_read, go_write = os.pipe()
        ready_read, ready_write = os.pipe()
        parent = identity(os.getpid())
        argv = [sys.executable, '-I', '-S', '-B', str(ROOT / 'runner_bootstrap.py'),
                '--go-fd', str(go_read), '--ready-fd', str(ready_write), '--runner-fd', str(groups[2].fd),
                '--parent-pid', str(os.getpid()), '--parent-start', str(parent['start']),
                '--authorization', str(args.authorization.resolve()), '--output', str(output)]
        process = subprocess.Popen(argv, cwd=ROOT, env={'PATH': os.defpath},
                                   stdin=subprocess.DEVNULL, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                   pass_fds=(go_read, ready_write, groups[2].fd), start_new_session=True)
        os.close(go_read)
        go_read = None
        os.close(ready_write)
        ready_write = None
        resource.setrlimit(resource.RLIMIT_CPU, (2, 3))
        pending = wait_ready(process, ready_read, launch, groups)
        authorization, qualification, manager = check_gate(args.authorization, args.qualification, launch, groups, pending)
        require(str(output) == authorization['artifact_directory'], 'artifact directory authorization')
        output.mkdir(mode=0o700, parents=False, exist_ok=False)
        checkpoint(launch, groups[0])
        topology(groups, pending)
        packet = (json.dumps({'go': True, 'authorization_sha256': digest(args.authorization)}) + '\n').encode()
        require(len(packet) <= 256 and os.write(go_write, packet) == len(packet), 'GO write')
        os.close(go_write)
        go_write = None
        status, reason = 'INCONCLUSIVE_LIMIT', 'runner_not_completed'
        selector = selectors.DefaultSelector()
        selector.register(process.stdout, selectors.EVENT_READ, 'status')
        selector.register(process.stderr, selectors.EVENT_READ, 'stderr')
        area_start = time.monotonic_ns()
        while process.poll() is None:
            checkpoint(launch, groups[0])
            if current_area:
                require(time.monotonic_ns() - area_start < (5 if current_area in 'AB' else 10) * 10**9, 'area wall guard')
            for key, _ in selector.select(0.01):
                raw = os.read(key.fileobj.fileno(), 1024)
                if not raw:
                    selector.unregister(key.fileobj)
                    continue
                if key.data == 'stderr':
                    require(len(stderr) + len(raw) <= 8192, 'stderr cap')
                    stderr.extend(raw)
                else:
                    status_buffer.extend(raw)
                    require(len(status_buffer) <= 32, 'status overflow')
                    while b'\n' in status_buffer:
                        row, _, tail = status_buffer.partition(b'\n')
                        status_buffer[:] = tail
                        area = row.decode('ascii')
                        require(area in 'ABCDEF' and area == ('A' if current_area is None else chr(ord(current_area) + 1)), 'nonsequential area')
                        current_area, area_start = area, time.monotonic_ns()
        reason = 'runner_exit'
    except Exception as error:
        reason = type(error).__name__ + ': ' + str(error)[:256]
        if process is not None:
            status = 'INCONCLUSIVE_LIMIT'
    finally:
        for fd in (go_read, go_write, ready_read, ready_write):
            if fd is not None:
                os.close(fd)
        signal.setitimer(signal.ITIMER_REAL, 0)
        if selector is not None:
            selector.close()
        if process is not None:
            try:
                end = min(time.monotonic() + 1, launch['started_monotonic_ns'] / 10**9 + 59)
                cleanup_record = cleanup(process, pending, groups, end)
            except Exception as error:
                cleanup_record = {'complete': False, 'reason': type(error).__name__ + ': ' + str(error)[:256]}
            if not cleanup_record['complete']:
                status = 'INCONCLUSIVE_LIMIT'
    # No record is fabricated when setup, capacity, CPU or manager termination
    # prevents persistence. External qualification evidence must retain that gap.
    if authorization is not None and output.is_dir():
        try:
            elapsed = time.monotonic_ns() - launch['started_monotonic_ns']
            cpu = groups[0].cpu()
            peak = int(groups[0].read('memory.peak'))
            require(elapsed < 60_000_000_000 and cpu < 50_000_000 and peak <= 256 * MIB, 'observed ceiling violation')
            if reason == 'runner_exit' and process.returncode == 0 and cleanup_record['complete']:
                from jsonschema import Draft202012Validator
                summary = bounded_json(output / 'summary.json')
                Draft202012Validator(bounded_json(ROOT / 'evidence.schema.json', 65536)['$defs']['summary']).validate(summary)
                if current_area == 'F' and set(summary['areas']) == set('ABCDEF') and all(
                        area['verdict'] not in ('INCONCLUSIVE_LIMIT', 'BLOCKED', 'INVALID_RUN')
                        for area in summary['areas'].values()):
                    status = 'COMPLETED_UNVERIFIED'
            record = {'status': status, 'stop': reason, 'returncode': process.returncode,
                      'incomplete': status != 'COMPLETED_UNVERIFIED', 'last_area': current_area,
                      'evidence_finalization': 'unverified_until_external_inventory',
                      'elapsed_ns': elapsed, 'aggregate_cpu_usec': cpu, 'aggregate_memory_peak': peak,
                      'cleanup': cleanup_record, 'stderr': stderr.decode('utf-8', errors='replace'),
                      'manager': manager, 'authorization_authenticity': 'operator responsibility'}
            persist(output, record, authorization, qualification, launch, groups[0])
        except Exception as error:
            status = 'INCONCLUSIVE_LIMIT'
            print('INCOMPLETE_EVIDENCE: ' + type(error).__name__ + ': ' + str(error)[:256], file=sys.stderr)
    else:
        print(status + ': ' + reason, file=sys.stderr)
    for group in groups:
        group.close()
    return 0 if status == 'COMPLETED_UNVERIFIED' else 2


if __name__ == '__main__':
    raise SystemExit(main())
