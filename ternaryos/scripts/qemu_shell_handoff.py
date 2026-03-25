#!/usr/bin/env python3
import argparse
import os
import pty
import select
import signal
import subprocess
import sys
import termios
import tty


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--trigger", required=True)
    parser.add_argument("--launch", required=True)
    parser.add_argument("cmd", nargs=argparse.REMAINDER)
    args = parser.parse_args()
    if not args.cmd or args.cmd[0] != "--" or len(args.cmd) == 1:
        parser.error("expected command after --")
    args.cmd = args.cmd[1:]
    return args


def restore_tty(fd: int, old_attrs):
    if old_attrs is not None:
        termios.tcsetattr(fd, termios.TCSADRAIN, old_attrs)


def main() -> int:
    args = parse_args()

    master_fd, slave_fd = pty.openpty()
    proc = subprocess.Popen(
        args.cmd,
        stdin=slave_fd,
        stdout=slave_fd,
        stderr=slave_fd,
        start_new_session=True,
        close_fds=True,
    )
    os.close(slave_fd)

    stdin_fd = sys.stdin.fileno()
    stdout_fd = sys.stdout.fileno()
    old_tty = None
    if os.isatty(stdin_fd):
        old_tty = termios.tcgetattr(stdin_fd)
        tty.setraw(stdin_fd)

    buffer = ""
    handoff = False

    stdin_open = True

    try:
        while True:
            read_fds = [master_fd]
            if not handoff and stdin_open:
                read_fds.append(stdin_fd)
            ready, _, _ = select.select(read_fds, [], [], 0.1)

            if master_fd in ready:
                try:
                    data = os.read(master_fd, 4096)
                except OSError:
                    data = b""
                if data:
                    os.write(stdout_fd, data)
                    text = data.decode("utf-8", errors="ignore")
                    buffer = (buffer + text)[-8192:]
                    if not handoff and args.trigger in buffer:
                        handoff = True
                        try:
                            os.killpg(proc.pid, signal.SIGTERM)
                        except ProcessLookupError:
                            pass
                elif proc.poll() is not None:
                    break

            if stdin_fd in ready and not handoff and stdin_open:
                try:
                    data = os.read(stdin_fd, 1024)
                except OSError:
                    data = b""
                if data:
                    os.write(master_fd, data)
                else:
                    stdin_open = False

            if proc.poll() is not None and not ready:
                break
    finally:
        restore_tty(stdin_fd, old_tty)
        try:
            os.close(master_fd)
        except OSError:
            pass

    if handoff:
        if proc.poll() is None:
            try:
                os.killpg(proc.pid, signal.SIGTERM)
            except ProcessLookupError:
                pass
            try:
                proc.wait(timeout=2)
            except subprocess.TimeoutExpired:
                os.killpg(proc.pid, signal.SIGKILL)
                proc.wait()
        print("\n[axion] launching hosted TUI frontend...\n")
        return subprocess.call([args.launch])

    return proc.returncode or 0


if __name__ == "__main__":
    raise SystemExit(main())
