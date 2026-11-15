#!/usr/bin/env python3
"""Bounded QEMU inspection + guest boot/help smoke checks, not OS qualification.

OUTPUT_DIR is a result parent, never a reusable attempt. Only this process writes
its summary; consumers must use its exit status, never search historical reports.
"""
import json
import os
from pathlib import Path
import re
import shutil
import signal
import socket
import subprocess
import sys
import tempfile
import time

ROOT = Path(__file__).resolve().parents[3]
READY = '[axion] t81sh: ready (principal=axion, tier=1)'
HELP = 'show RFC-00B9 shell commands'
FAILURE = re.compile(r'❌|\b(?:error|fail|failed|failure|fatal|panic|assertion)\b', re.I)
CHECKS = ('monitor_test', 'memory_analysis', 'boot_sequence_test',
          'hardware_test', 'integration_test')


class VerificationError(Exception):
    pass


class ChildExit(VerificationError):
    def __init__(self, code):
        super().__init__(f'QEMU exit status {code}')
        self.code = code if code > 0 else 128 - code


class Interrupted(VerificationError):
    def __init__(self, signum):
        super().__init__(f'interrupted by signal {signum}')
        self.code = 128 + signum


def require(condition, message):
    if not condition:
        raise VerificationError(message)


def evidence(path):
    require(path.is_file() and path.stat().st_size > 0,
            f'missing or empty evidence: {path.name}')
    text = path.read_text(errors='replace')
    require(not FAILURE.search(text), f'failure output in {path.name}')
    return text


class Session:
    def __init__(self, process, deadline, log):
        self.process, self.deadline, self.log = process, deadline, log
        self.stream = None

    def remaining(self):
        left = self.deadline - time.monotonic()
        if left <= 0:
            raise TimeoutError('verification deadline exceeded')
        return left

    def connect(self, path):
        while True:
            self.remaining()
            if self.process.poll() is not None:
                raise ChildExit(self.process.returncode or 1)
            connection = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
            connection.settimeout(self.remaining())
            try:
                connection.connect(str(path))
                return connection
            except (FileNotFoundError, ConnectionRefusedError):
                connection.close()
                time.sleep(min(.05, self.remaining()))

    def read(self):
        self.connection.settimeout(self.remaining())
        line = self.stream.readline(1024 * 1024)
        require(line.endswith(b'\n'), 'missing or oversized QMP reply')
        require(bool(line), 'QMP disconnected before reply')
        self.log.write(line)
        self.log.flush()
        response = json.loads(line)
        require(isinstance(response, dict), 'invalid QMP response')
        return response

    def command(self, command, arguments=None):
        request = {'execute': command}
        if arguments is not None:
            request['arguments'] = arguments
        self.connection.settimeout(self.remaining())
        self.connection.sendall((json.dumps(request) + '\n').encode())
        while True:
            response = self.read()
            require('error' not in response, f'QMP command failed: {response}')
            if 'return' in response:
                return response['return']

    def guest_marker(self, path, marker, start=0):
        while True:
            self.remaining()
            if self.process.poll() is not None:
                raise ChildExit(self.process.returncode or 1)
            if path.is_file() and path.stat().st_size:
                text = evidence(path)
                if marker in text[start:]:
                    return
            time.sleep(min(.05, self.remaining()))


def firmware():
    if os.environ.get('BIOS'):
        candidates = [Path(os.environ['BIOS']).resolve()]
    else:
        candidates = [Path(p) for p in (
            '/usr/share/AAVMF/AAVMF_CODE.fd',
            '/usr/share/qemu/edk2-aarch64-code.fd',
            '/opt/homebrew/share/qemu/edk2-aarch64-code.fd',
            '/usr/local/share/qemu/edk2-aarch64-code.fd')]
    for path in candidates:
        if path.is_file() and path.stat().st_size:
            return path
    raise VerificationError('EDK2 AArch64 firmware unavailable; set BIOS')


def execute(out, passed):
    binary = shutil.which(os.environ.get('QEMU_BIN', 'qemu-system-aarch64'))
    require(binary is not None, 'QEMU executable unavailable')
    image = Path(os.environ.get('QEMU_IMG', str(ROOT / 'build/qemu_test_debug/qemu_slice6_guest.img'))).resolve()
    require(image.is_file() and image.stat().st_size > 0, f'image missing or empty: {image}')
    bios = firmware()
    seconds = float(os.environ.get('VERIFICATION_TIMEOUT', '60'))
    require(0 < seconds <= 600, 'VERIFICATION_TIMEOUT must be in (0, 600] seconds')
    shutil.copyfile(image, out / 'guest.img')
    # Short socket paths avoid platform AF_UNIX path-length limits regardless of
    # the chosen result parent. These sockets are private to this invocation.
    with tempfile.TemporaryDirectory(prefix='t81-qmp-', dir='/tmp') as sockets, \
            (out / 'qemu.log').open('wb') as stderr, \
            (out / 'qmp.log').open('wb') as transcript:
        qmp, serial = Path(sockets) / 'qmp', Path(sockets) / 'serial'
        command = [binary, '-machine', 'virt,gic-version=3', '-cpu', 'cortex-a57',
                   '-m', '512', '-smp', '1', '-nographic', '-bios', str(bios),
                   '-drive', f'file={out / "guest.img"},format=raw,if=none,id=hd0',
                   '-device', 'virtio-blk-device,drive=hd0', '-monitor', 'none',
                   '-qmp', f'unix:{qmp},server=on,wait=off',
                   '-chardev', f'socket,id=serial0,path={serial},server=on,wait=off,logfile={out / "serial.log"}',
                   '-serial', 'chardev:serial0']
        (out / 'command.json').write_text(json.dumps(command, indent=2) + '\n')
        process = subprocess.Popen(command, stdin=subprocess.DEVNULL, stdout=stderr, stderr=stderr)
        session = Session(process, time.monotonic() + seconds, transcript)
        try:
            # Enforce a wall-clock deadline even if a peer dribbles an
            # unterminated reply and keeps individual socket reads active.
            signal.setitimer(signal.ITIMER_REAL, seconds)
            with session.connect(qmp) as connection:
                session.connection = connection
                with connection.makefile('rb') as session.stream:
                    require('QMP' in session.read(), 'missing QMP greeting')
                    session.command('qmp_capabilities')
                    status = session.command('query-status')
                    require(isinstance(status, dict) and status.get('running') is True, 'QEMU is not running')
                    passed.append(CHECKS[0])
                    dump = out / 'verification_memory.dump'
                    session.command('pmemsave', {'val': 0x40000000, 'size': 4096, 'filename': str(dump)})
                    require(dump.is_file() and dump.stat().st_size == 4096, 'missing, empty or incomplete RAM sample')
                    passed.append(CHECKS[1])
                    session.guest_marker(out / 'serial.log', READY)
                    passed.append(CHECKS[2])
                    cpus = session.command('query-cpus-fast')
                    blocks = session.command('query-block')
                    require(isinstance(cpus, list) and len(cpus) == 1 and
                            isinstance(cpus[0], dict) and cpus[0].get('cpu-index') == 0, 'unexpected CPU configuration')
                    require(isinstance(blocks, list) and any(
                        isinstance(b, dict) and b.get('device') == 'hd0' and
                        isinstance(b.get('inserted'), dict) and
                        (b['inserted'].get('node-name') or b['inserted'].get('file'))
                        for b in blocks), 'boot block device not inserted')
                    passed.append(CHECKS[3])
                    with session.connect(serial) as console:
                        console.settimeout(session.remaining())
                        start = len(evidence(out / 'serial.log'))
                        console.sendall(b'help\r')
                        session.guest_marker(out / 'serial.log', HELP, start)
                    passed.append(CHECKS[4])
                    session.command('quit')
                    code = process.wait(timeout=session.remaining())
                    if code != 0:
                        raise ChildExit(code)
            evidence(out / 'serial.log')
            evidence(out / 'qmp.log')
            # A silent QEMU diagnostic stream is valid; guest/QMP evidence is not.
            require(not FAILURE.search((out / 'qemu.log').read_text(errors='replace')),
                    'failure output in qemu.log')
        finally:
            signal.setitimer(signal.ITIMER_REAL, 0)
            if process.poll() is None:
                process.terminate()
                try:
                    process.wait(timeout=2)
                except subprocess.TimeoutExpired:
                    process.kill()
                    process.wait()


def main():
    def timeout(_signum, _frame):
        raise TimeoutError('verification wall-clock deadline exceeded')
    signal.signal(signal.SIGALRM, timeout)

    def interrupt(signum, _frame):
        raise Interrupted(signum)
    for signum in (signal.SIGINT, signal.SIGTERM, signal.SIGHUP):
        signal.signal(signum, interrupt)
    out, passed, code, detail = None, [], 1, ''
    try:
        require(len(sys.argv) == 1, 'usage: automated_verification.sh (configuration via environment)')
        parent = Path(os.environ.get('OUTPUT_DIR') or tempfile.gettempdir()).resolve()
        parent.mkdir(parents=True, exist_ok=True)
        out = Path(tempfile.mkdtemp(prefix='ternaryos-verification-', dir=parent))
        print(f'Results: {out}', flush=True)
        execute(out, passed)
        require(tuple(passed) == CHECKS, 'incomplete verification')
        code = 0
    except (Interrupted, ChildExit) as exc:
        code, detail = exc.code, str(exc)
    except (TimeoutError, subprocess.TimeoutExpired) as exc:
        code, detail = 124, f'timeout: {exc}'
    except (VerificationError, OSError, ValueError, TypeError, KeyError) as exc:
        detail = str(exc)
    success = code == 0
    report = '\n'.join([
        'TernaryOS bounded QEMU verification (not OS qualification)',
        *[f'{name}: CHECK COMPLETED' for name in passed],
        f'Tests Completed: {len(passed)}/{len(CHECKS)}',
        f'QEMU exit and final evidence: {"PASSED" if success else "NOT PASSED"}',
        f'Status: VERIFICATION {"SUCCESSFUL" if success else "FAILED"}',
        f'Detail: {detail}', ''])
    if out is not None:
        (out / 'verification_summary.txt').write_text(report)
    print(report, end='', flush=True)
    return code


if __name__ == '__main__':
    sys.exit(main())
