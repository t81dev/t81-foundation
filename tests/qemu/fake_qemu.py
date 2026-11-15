#!/usr/bin/env python3
"""Protocol fixture only; never evidence of guest execution."""
import json
import os
from pathlib import Path
import signal
import socket
import sys
import time

mode = os.environ.get('FAKE_MODE', 'pass')
args = sys.argv[1:]
qmp_path = args[args.index('-qmp') + 1].split(',')[0][5:]
chardev = dict(item.split('=', 1) for item in args[args.index('-chardev') + 1].split(',')[1:])
serial_path, log = chardev['path'], Path(chardev['logfile'])
if os.environ.get('FAKE_PID_FILE'):
    Path(os.environ['FAKE_PID_FILE']).write_text(str(os.getpid()))
if mode == 'exit':
    print('fixture child failure', flush=True)
    sys.exit(37)
if mode == 'signal':
    os.kill(os.getpid(), signal.SIGTERM)
if mode == 'ignore_term':
    signal.signal(signal.SIGTERM, signal.SIG_IGN)
    time.sleep(30)
if mode == 'hang':
    time.sleep(30)
if mode != 'missing':
    log.write_text('' if mode == 'empty' else
                   'fatal boot failure\n' if mode == 'failure' else
                   'ordinary boot output\n' if mode == 'incomplete' else
                   '[axion] t81sh: ready (principal=axion, tier=1)\n')
if mode == 'early_help':
    with log.open('a') as output:
        output.write('show RFC-00B9 shell commands\n')
with socket.socket(socket.AF_UNIX) as qmp, socket.socket(socket.AF_UNIX) as serial:
    qmp.bind(qmp_path)
    qmp.listen(1)
    serial.bind(serial_path)
    serial.listen(1)
    with qmp.accept()[0] as connection, connection.makefile('rb') as stream:
        if mode == 'dribble':
            while True:
                connection.sendall(b' ')
                time.sleep(.1)
        connection.sendall(b'{"QMP": {"version": {}}}\n')
        while True:
            line = stream.readline()
            if not line:
                break
            request = json.loads(line)
            cmd = request['execute']
            result = {}
            if mode == 'qmp_error' and cmd == 'query-status':
                connection.sendall(b'{"error":{"desc":"fixture failure"}}\n')
                continue
            if cmd == 'query-status':
                result = {'running': mode != 'stopped'}
            elif cmd == 'pmemsave':
                dump = Path(request['arguments']['filename'])
                if mode != 'missing_dump':
                    dump.write_bytes(b'\0' * (0 if mode == 'empty_dump' else 4096))
            elif cmd == 'query-cpus-fast':
                result = [] if mode == 'bad_cpu' else [{'cpu-index': 0}]
            elif cmd == 'query-block':
                result = [] if mode == 'bad_block' else [{'device': 'hd0', 'inserted': {'file': 'guest.img'}}]
                connection.sendall((json.dumps({'return': result}) + '\n').encode())
                # The next harness operation sends a serial help command.
                if mode != 'bad_block':
                    with serial.accept()[0] as console:
                        data = console.recv(100)
                        if data != b'help\r':
                            sys.exit(9)
                        with log.open('a') as output:
                            if mode not in ('missing_help', 'early_help'):
                                output.write('show RFC-00B9 shell commands\n')
                            if mode == 'late_failure':
                                output.write('ERROR: shutdown failed\n')
                continue
            elif cmd == 'quit':
                connection.sendall(b'{"return": {}}\n')
                if mode == 'hang_shutdown':
                    time.sleep(30)
                sys.exit(23 if mode == 'exit_shutdown' else 0)
            connection.sendall((json.dumps({'return': result}) + '\n').encode())
