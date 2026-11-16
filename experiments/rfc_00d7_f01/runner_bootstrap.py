"""Stdlib-only exec stub. No experimental imports before the verified GO packet."""

import argparse
import ctypes
import fcntl
import hashlib
import json
import os
import resource
import signal
import stat
import sys
import sysconfig
from pathlib import Path


def identity(pid):
    fields = Path('/proc/' + str(pid) + '/stat').read_text().rsplit(')', 1)[1].split()
    return {'pid': pid, 'start': int(fields[19]), 'ppid': int(fields[1]),
            'pgid': int(fields[2]), 'sid': int(fields[3])}


def parse_authorization(raw):
    if not 0 < len(raw) <= 8192:
        raise ValueError('authorization byte bound')
    def unique(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise ValueError('duplicate authorization key')
            result[key] = value
        return result
    return json.loads(raw, object_pairs_hook=unique)


def authorization_snapshot(fd, expected=None):
    required = fcntl.F_SEAL_WRITE | fcntl.F_SEAL_GROW | fcntl.F_SEAL_SHRINK | fcntl.F_SEAL_SEAL
    seals = fcntl.fcntl(fd, fcntl.F_GET_SEALS)
    info = os.fstat(fd)
    if seals & required != required or not stat.S_ISREG(info.st_mode) or not 0 < info.st_size <= 8192:
        raise ValueError('unsealed or invalid authorization snapshot')
    raw = os.pread(fd, 8193, 0)
    if len(raw) != info.st_size:
        raise ValueError('truncated authorization snapshot')
    binding = {'authorization_sha256': hashlib.sha256(raw).hexdigest(),
               'authorization_size': info.st_size, 'authorization_inode': info.st_ino,
               'authorization_device': info.st_dev}
    if expected is not None and (set(expected) != set(binding)
            or any(type(expected[key]) is not int for key in ('authorization_size', 'authorization_inode', 'authorization_device'))
            or type(expected['authorization_sha256']) is not str or binding != expected):
        raise ValueError('authorization snapshot identity/integrity mismatch')
    return raw, binding


def interpreter_boundary(root, runner=False):
    """Strict Linux CPython venv profile, before any dependency/model import.

    -S prevents site/.pth/customization execution; -I (gate/stub) or -P with
    the exact minimal environment (runner) excludes ambient/script import paths.
    This is a reviewed profile, not a general-purpose Python path resolver.
    """
    if sys.platform != 'linux' or sys.implementation.name != 'cpython':
        raise ValueError('unsupported interpreter')
    if not sys.flags.no_site or not sys.flags.safe_path or not __debug__:
        raise ValueError('site initialization or unsafe search path enabled')
    if runner:
        required_env = {'PATH': os.defpath, 'PYTHONHASHSEED': '0',
                        'PYTHONDONTWRITEBYTECODE': '1', 'PYTHONNOUSERSITE': '1',
                        'F01_QUALIFIED_CHILD': '1'}
        if (sys.flags.ignore_environment or any(os.environ.get(k) != v for k, v in required_env.items())
                or set(os.environ) - set(required_env) - {'LC_CTYPE'}):
            raise ValueError('unreviewed final interpreter environment')
    elif not sys.flags.isolated:
        raise ValueError('isolated bootstrap/gate required')
    prefix = Path(sys.prefix).resolve()
    base = Path(sys.base_prefix).resolve()
    executable = Path(sys.executable).absolute()
    base_executable = Path(sys._base_executable).resolve()
    if (prefix == base or Path(sys.exec_prefix).resolve() != prefix
            or Path(sys.base_exec_prefix).resolve() != base
            or executable.parent.parent.resolve() != prefix
            or executable.resolve() != base_executable):
        raise ValueError('unreviewed venv/interpreter mapping')
    # CPython searches beside the lexical executable, then its parent. Reject
    # ambiguous/alternative configs instead of trusting sys.prefix alone.
    locations = [executable.parent / 'pyvenv.cfg', executable.parent.parent / 'pyvenv.cfg']
    present = [path for path in locations if path.exists() or path.is_symlink()]
    configuration = prefix / 'pyvenv.cfg'
    if len(present) != 1 or present[0].is_symlink() or not os.path.samefile(present[0], configuration):
        raise ValueError('ambiguous venv configuration')
    with configuration.open('rb') as stream:
        raw = stream.read(8193)
    if len(raw) > 8192:
        raise ValueError('venv configuration bound')
    entries = {}
    for row in raw.decode('utf-8').splitlines():
        if not row.strip():
            continue
        key, value = row.split('=', 1)
        key, value = key.strip().lower(), value.strip()
        if key in entries:
            raise ValueError('duplicate venv configuration key')
        entries[key] = value
    if entries.get('include-system-site-packages', '').lower() != 'false':
        raise ValueError('system site packages are not explicitly disabled')
    if Path(entries['home']).resolve() != base_executable.parent:
        raise ValueError('venv home/base executable mismatch')
    for directory in {executable.parent, base_executable.parent, Path(sysconfig.get_config_var('LIBDIR'))}:
        if any(directory.glob('*._pth')):
            raise ValueError('unreviewed interpreter path override')
    library = Path(sysconfig.get_path('purelib')).resolve()
    if (library != Path(sysconfig.get_path('platlib')).resolve()
            or prefix not in library.parents or not library.is_dir()):
        raise ValueError('venv package library outside reviewed prefix')
    stdlib = Path(sysconfig.get_path('stdlib')).resolve()
    dynamic = Path(sysconfig.get_config_var('DESTSHARED')).resolve()
    archive = stdlib.parent / ('python' + str(sys.version_info.major) + str(sys.version_info.minor) + '.zip')
    expected = [str(archive), str(stdlib), str(dynamic)]
    if (base not in stdlib.parents or base not in dynamic.parents or archive.exists()
            or [str(Path(path).resolve()) for path in sys.path] != expected):
        raise ValueError('unreviewed effective interpreter search path')
    root = root.resolve()
    for directory in (root, library, stdlib, dynamic):
        if (not directory.is_dir() or any(directory.glob('*.pth'))
                or (directory / 'sitecustomize.py').exists()
                or (directory / 'usercustomize.py').exists()):
            raise ValueError('startup hook or unsupported import directory')
    return {'executable': str(executable), 'base_executable': str(base_executable),
            'prefix': str(prefix), 'base_prefix': str(base),
            'configuration': str(configuration), 'configuration_sha256': hashlib.sha256(raw).hexdigest(),
            'include_system_site_packages': False, 'initial_search_path': expected,
            'library': str(library), 'root': str(root), 'site_initialization': False}


def main():
    parser = argparse.ArgumentParser()
    for name in ('go_fd', 'ready_fd', 'runner_fd', 'parent_pid', 'parent_start', 'authorization_fd'):
        parser.add_argument('--' + name.replace('_', '-'), type=int, required=True)
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
    if set(go) != {'go', 'authorization_sha256', 'authorization_size',
                   'authorization_inode', 'authorization_device'} or go['go'] is not True:
        raise RuntimeError('GO not granted')
    binding = {key: value for key, value in go.items() if key != 'go'}
    authorization_snapshot(args.authorization_fd, binding)
    interpreter_boundary(Path(__file__).parent)
    if os.getppid() != args.parent_pid or identity(args.parent_pid)['start'] != args.parent_start:
        raise RuntimeError('parent changed before entry')
    os.close(args.go_fd)
    os.close(args.runner_fd)
    env = {'PATH': os.defpath, 'PYTHONHASHSEED': '0', 'PYTHONDONTWRITEBYTECODE': '1',
           'PYTHONNOUSERSITE': '1', 'F01_QUALIFIED_CHILD': '1'}
    os.set_inheritable(args.authorization_fd, True)
    # -S disables startup hooks; -P excludes the script path; the minimal env
    # retains PYTHONHASHSEED=0 (which -I would ignore).
    argv = [sys.executable, '-P', '-S', '-B', str(Path(__file__).with_name('run.py')),
            '--authorization-fd', str(args.authorization_fd), '--output', args.output]
    for key, value in binding.items():
        argv.extend(['--' + key.replace('_', '-'), str(value)])
    os.execve(sys.executable, argv, env)


if __name__ == '__main__':
    main()
