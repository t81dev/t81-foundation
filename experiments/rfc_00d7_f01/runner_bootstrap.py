"""Stdlib-only exec stub. No experimental imports before the verified GO packet."""

import argparse
import ctypes
import hashlib
import json
import os
import resource
import signal
import sys
from pathlib import Path


def identity(pid):
    fields = Path('/proc/' + str(pid) + '/stat').read_text().rsplit(')', 1)[1].split()
    return {'pid': pid, 'start': int(fields[19]), 'ppid': int(fields[1]),
            'pgid': int(fields[2]), 'sid': int(fields[3])}


def main():
    parser = argparse.ArgumentParser()
    for name in ('go_fd', 'ready_fd', 'runner_fd', 'parent_pid', 'parent_start'):
        parser.add_argument('--' + name.replace('_', '-'), type=int, required=True)
    parser.add_argument('--authorization', required=True)
    parser.add_argument('--output', required=True)
    args = parser.parse_args()
    if sys.platform != 'linux' or not sys.flags.isolated or not sys.flags.no_site:
        raise RuntimeError('unsupported bootstrap interpreter')
    parent = identity(args.parent_pid)
    if os.getppid() != args.parent_pid or parent['start'] != args.parent_start:
        raise RuntimeError('parent identity changed')
    libc = ctypes.CDLL(None, use_errno=True)
    if libc.prctl(1, signal.SIGKILL, 0, 0, 0) != 0:  # PR_SET_PDEATHSIG
        raise OSError(ctypes.get_errno(), 'PDEATHSIG')
    if os.getppid() != args.parent_pid:
        raise RuntimeError('parent exited during bootstrap')
    hard = resource.getrlimit(resource.RLIMIT_CPU)[1]
    if hard != resource.RLIM_INFINITY and hard < 45:
        raise RuntimeError('inherited CPU hard limit below 45')
    resource.setrlimit(resource.RLIMIT_CPU, (44, 45))
    resource.setrlimit(resource.RLIMIT_CORE, (0, 0))
    resource.setrlimit(resource.RLIMIT_FSIZE, (25 * 1024**2, 25 * 1024**2))
    fd = os.open('cgroup.procs', os.O_WRONLY | os.O_NOFOLLOW, dir_fd=args.runner_fd)
    try:
        os.write(fd, str(os.getpid()).encode())
    finally:
        os.close(fd)
    ready = identity(os.getpid())
    ready.update(cpu=list(resource.getrlimit(resource.RLIMIT_CPU)),
                 runner_inode=os.fstat(args.runner_fd).st_ino)
    raw = (json.dumps(ready, sort_keys=True) + '\n').encode()
    if len(raw) > 1024 or os.write(args.ready_fd, raw) != len(raw):
        raise RuntimeError('READY write incomplete')
    os.close(args.ready_fd)
    # The supervisor closes its writer after one bounded packet. EOF before GO
    # or any excess/trailing bytes is a blocked startup, never model entry.
    packet = bytearray()
    while True:
        chunk = os.read(args.go_fd, 257 - len(packet))
        if not chunk:
            break
        packet.extend(chunk)
        if len(packet) > 256:
            raise RuntimeError('GO overflow')
    go = json.loads(packet)
    if set(go) != {'go', 'authorization_sha256'} or go['go'] is not True:
        raise RuntimeError('GO not granted')
    auth = Path(args.authorization)
    with auth.open('rb') as stream:
        raw = stream.read(8193)
    if len(raw) > 8192 or hashlib.sha256(raw).hexdigest() != go['authorization_sha256']:
        raise RuntimeError('authorization changed after GO')
    if os.getppid() != args.parent_pid or identity(args.parent_pid)['start'] != args.parent_start:
        raise RuntimeError('parent changed before entry')
    os.close(args.go_fd)
    os.close(args.runner_fd)
    env = {'PATH': os.defpath, 'PYTHONHASHSEED': '0', 'PYTHONDONTWRITEBYTECODE': '1',
           'PYTHONNOUSERSITE': '1', 'F01_QUALIFIED_CHILD': '1'}
    # Existing run.py's import policy is unchanged; G1 must have approved the
    # interpreter/file lock and rejected site startup hooks before this exec.
    os.execve(sys.executable, [sys.executable, '-B', str(Path(__file__).with_name('run.py')),
                              '--authorization', str(auth), '--output', args.output], env)


if __name__ == '__main__':
    main()
