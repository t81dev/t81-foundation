"""Prepared Linux-only run gate and supervisor. Not invoked in preparation."""

import argparse
import hashlib
import json
import os
import resource
import selectors
import signal
import subprocess
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parent


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def bounded_json(path, max_bytes=32768):
    if path.stat().st_size > max_bytes:
        raise ValueError('oversized gate input')
    return json.loads(path.read_text())


def check_gate(authorization_path, qualification_path):
    from jsonschema import Draft202012Validator
    authorization = bounded_json(authorization_path, 8192)
    qualification = bounded_json(qualification_path, 8192)
    schemas = bounded_json(ROOT / 'evidence.schema.json')
    for name, value in [('authorization', authorization), ('qualification', qualification)]:
        Draft202012Validator(schemas['$defs'][name]).validate(value)
    plan = bounded_json(ROOT / 'fa01-plan.json')
    for record in (authorization, qualification):
        for key in ('corpus_sha256', 'amendment_sha256', 'protocol_sha256'):
            if record[key] != plan[key]:
                raise ValueError('BLOCKED: stale FA01 identity: ' + key)
        if record['area_f_plan_id'] != plan['id']:
            raise ValueError('BLOCKED: unknown F plan')
    if sys.platform != 'linux':
        raise ValueError('BLOCKED: only qualified Linux supported')
    if authorization['scope'] != 'one-bounded-run' or not authorization['human_record']:
        raise ValueError('BLOCKED: separate human execution record required')
    from interpretation_policy import validate
    validate(authorization['interpretations'], bounded_json(ROOT / 'interpretations.json')['choices'])
    # A record is checked for scope/identity, NOT authenticated as a human act.
    # The operator must independently establish authorization authenticity.
    head = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    dirty = subprocess.check_output(['git', 'status', '--porcelain'], cwd=ROOT)
    if dirty or head != authorization['harness_sha']:
        raise ValueError('BLOCKED: dirty or unreviewed harness revision')
    protocol = ROOT.parents[1] / 'docs/research/proposals/RFC-00D7-F01/PROTOCOL.md'
    if digest(protocol) != authorization['protocol_sha256']:
        raise ValueError('BLOCKED: protocol changed')
    if qualification['harness_sha'] != head or not qualification['approved_by']:
        raise ValueError('BLOCKED: unreviewed qualification')
    if qualification['dependency_lock_sha256'] != digest(ROOT / 'dependency-lock.json'):
        raise ValueError('BLOCKED: dependency lock changed')
    from static_check import verify
    verify(ROOT)  # No imports or invocations of candidate/oracle/domains.
    if bounded_json(ROOT / 'dependency-lock.json', 1024 * 1024)['linux_relock_required']:
        raise ValueError('BLOCKED: reviewed Linux dependency re-lock required')
    cgroup = Path(qualification['aggregate_cgroup']).resolve()
    actual_group = Path('/sys/fs/cgroup') / Path('/proc/self/cgroup').read_text().strip().split('0::', 1)[1].lstrip('/')
    if actual_group.resolve() != cgroup:
        raise ValueError('BLOCKED: supervisor not in declared aggregate cgroup')
    child_group = cgroup / 'runner'
    for group, memory in ((cgroup, 256 * 1024**2), (child_group, 128 * 1024**2)):
        if int((group / 'memory.max').read_text()) != memory:
            raise ValueError('BLOCKED: memory controller mismatch')
        if (group / 'memory.swap.max').read_text().strip() != '0':
            raise ValueError('BLOCKED: swap not disabled')
    quota, period = (cgroup / 'cpu.max').read_text().split()
    if quota == 'max' or int(quota) > int(period):
        raise ValueError('BLOCKED: aggregate CPU quota > one CPU')
    procs = set((cgroup / 'cgroup.procs').read_text().split())
    if procs != {str(os.getpid())} or (child_group / 'cgroup.procs').read_text().strip():
        raise ValueError('BLOCKED: aggregate group contains unrelated work')
    if len(os.sched_getaffinity(0)) != 1:
        raise ValueError('BLOCKED: supervisor/child require inherited single CPU affinity')
    # Receipt must pin independently reviewed current-machine qualification.
    if qualification['boot_id'] != Path('/proc/sys/kernel/random/boot_id').read_text().strip():
        raise ValueError('BLOCKED: stale qualification boot')
    if qualification['kernel_release'] != os.uname().release:
        raise ValueError('BLOCKED: kernel changed')
    return authorization, qualification, cgroup, child_group


def cpu_usec(cgroup):
    fields = (cgroup / 'cpu.stat').read_text().split()
    return int(dict(zip(fields[::2], fields[1::2]))['usage_usec'])


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--authorization', type=Path, required=True)
    parser.add_argument('--qualification', type=Path, required=True)
    parser.add_argument('--output', type=Path, required=True)
    args = parser.parse_args()
    started = time.monotonic_ns()
    authorization, qualification, group, child_group = check_gate(args.authorization, args.qualification)
    output = args.output.resolve()
    if str(output) != authorization['artifact_directory']:
        raise SystemExit('BLOCKED: artifact directory outside authorization')
    output.mkdir(mode=0o700, parents=False, exist_ok=False)
    # Entire invocation, including gate/setup/finalization, is supervised.
    # Qualification must bound kernel delivery/cleanup overshoot within margins.
    resource.setrlimit(resource.RLIMIT_CPU, (2, 3))
    resource.setrlimit(resource.RLIMIT_FSIZE, (25 * 1024**2, 25 * 1024**2))
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    baseline_cpu = cpu_usec(group)

    def child_limits():
        (child_group / 'cgroup.procs').write_text(str(os.getpid()))
        resource.setrlimit(resource.RLIMIT_CPU, (44, 45))
        resource.setrlimit(resource.RLIMIT_CORE, (0, 0))

    env = {'PATH': os.defpath, 'PYTHONHASHSEED': '0', 'PYTHONDONTWRITEBYTECODE': '1',
           'F01_QUALIFIED_CHILD': '1'}
    process = subprocess.Popen([sys.executable, '-B', str(ROOT / 'run.py'),
                                '--authorization', str(args.authorization.resolve()),
                                '--output', str(output)], env=env, cwd=ROOT,
                               stdin=subprocess.DEVNULL, stdout=subprocess.PIPE,
                               stderr=subprocess.PIPE, preexec_fn=child_limits,
                               start_new_session=True)
    selector = selectors.DefaultSelector()
    selector.register(process.stdout, selectors.EVENT_READ, 'status')
    selector.register(process.stderr, selectors.EVENT_READ, 'stderr')
    current_area = None
    area_start = time.monotonic_ns()
    stderr = bytearray()
    status_buffer = bytearray()
    reason = 'normal_exit'
    try:
        while process.poll() is None:
            now = time.monotonic_ns()
            if now - started >= 58_000_000_000:
                reason = 'suite_deadline_guard'
                break
            if cpu_usec(group) - baseline_cpu >= 48_000_000:
                reason = 'aggregate_cpu_guard'
                break
            if current_area and now - area_start >= (5 if current_area in 'AB' else 10) * 10**9:
                reason = 'area_wall_deadline'
                break
            for key, _ in selector.select(timeout=0.01):
                chunk = os.read(key.fileobj.fileno(), 1024)
                if not chunk:
                    selector.unregister(key.fileobj)
                    continue
                if key.data == 'stderr':
                    stderr.extend(chunk[:max(0, 8192 - len(stderr))])
                    if len(stderr) >= 8192:
                        reason = 'stderr_cap'
                        break
                else:
                    status_buffer.extend(chunk)
                    if len(status_buffer) > 32:
                        raise ValueError('status channel overflow')
                    while b'\n' in status_buffer:
                        row, _, tail = status_buffer.partition(b'\n')
                        status_buffer[:] = tail
                        area = row.decode('ascii')
                        expected = 'A' if current_area is None else chr(ord(current_area) + 1)
                        if area != expected or area not in 'ABCDEF':
                            raise ValueError('nonsequential area')
                        current_area, area_start = area, time.monotonic_ns()
            if reason != 'normal_exit':
                break
    finally:
        if process.poll() is None:
            os.killpg(process.pid, signal.SIGKILL)  # Budget cleanup only, no fault experiment.
        process.wait(timeout=1)
        selector.close()
    summary_path = output / 'summary.json'
    incomplete = not summary_path.exists()
    record = {'stop': reason, 'returncode': process.returncode,
              'incomplete': incomplete, 'last_area': current_area,
              'elapsed_ns': time.monotonic_ns() - started,
              'aggregate_cpu_usec': cpu_usec(group) - baseline_cpu,
              'aggregate_memory_peak': int((group / 'memory.peak').read_text()),
              'stderr': stderr.decode('utf-8', errors='replace'),
              'limits_qualified': qualification['approved_by'],
              'authorization_authenticity': 'operator responsibility'}
    raw = (json.dumps(record, sort_keys=True) + '\n').encode()
    if len(raw) > 16384:
        raise ValueError('supervisor record cap')
    (output / 'supervisor.json').write_bytes(raw)
    report = ('# Bounded synthetic run\n\nSee summary.json and supervisor.json. '
              'Missing summary or forced termination means INCONCLUSIVE_LIMIT. '
              'No production or security qualification follows.\n')
    (output / 'REPORT.md').write_text(report)
    # Copy only bounded authorization/qualification metadata, never credentials.
    (output / 'authorization-record.json').write_text(json.dumps(authorization))
    (output / 'qualification-record.json').write_text(json.dumps(qualification))
    hashes = []
    total = 0
    for path in sorted(output.iterdir()):
        total += path.stat().st_size
        hashes.append(digest(path) + '  ' + path.name + '\n')
    manifest = ''.join(hashes).encode()
    if total + len(manifest) > 25 * 1024**2:
        raise ValueError('artifact accounting violation; invalidate run')
    (output / 'SHA256SUMS').write_bytes(manifest)


if __name__ == '__main__':
    main()
