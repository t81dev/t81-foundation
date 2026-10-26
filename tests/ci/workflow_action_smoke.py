#!/usr/bin/env python3
"""Disposable runner-local endpoints and artifacts; never contacts a registry."""
import argparse
import http.server
import json
from pathlib import Path
import zipfile

ROOT = Path('.github/logs/supply-smoke')


def prepare():
    ROOT.mkdir(parents=True, exist_ok=True)
    (ROOT / 'dist').mkdir(exist_ok=True)
    (ROOT / 'payload').write_text('T81 immutable action smoke fixture\n')
    (ROOT / 'Dockerfile').write_text('FROM scratch\nCOPY payload /payload\n')
    prefix = 't81_ci_pin_smoke-0.0.0.dist-info/'
    files = {
        't81_ci_pin_smoke/__init__.py': '',
        prefix + 'METADATA': 'Metadata-Version: 2.1\nName: t81-ci-pin-smoke\nVersion: 0.0.0\nSummary: Disposable CI fixture\n\nCI only.\n',
        prefix + 'WHEEL': 'Wheel-Version: 1.0\nGenerator: T81 CI\nRoot-Is-Purelib: true\nTag: py3-none-any\n',
    }
    files[prefix + 'RECORD'] = ''.join(name + ',,\n' for name in [*files, prefix + 'RECORD'])
    with zipfile.ZipFile(ROOT / 'dist/t81_ci_pin_smoke-0.0.0-py3-none-any.whl', 'w') as wheel:
        for name, data in files.items():
            wheel.writestr(name, data)


class Handler(http.server.BaseHTTPRequestHandler):
    def record(self, method, size=0):
        with (ROOT / 'requests.jsonl').open('a') as stream:
            stream.write(json.dumps({'method': method, 'path': self.path, 'bytes': size}) + '\n')

    def do_GET(self):
        if self.path not in ('/v2/', '/health'):
            self.send_error(404)
            return
        self.record('GET')
        self.send_response(200)
        self.send_header('Docker-Distribution-Api-Version', 'registry/2.0')
        self.end_headers()
        self.wfile.write(b'{}')

    def do_POST(self):
        size = int(self.headers.get('Content-Length', '0'))
        if self.path != '/legacy/' or not 0 < size < 1024 * 1024:
            self.send_error(400)
            return
        body = self.rfile.read(size)
        if b't81_ci_pin_smoke-0.0.0-py3-none-any.whl' not in body:
            self.send_error(400)
            return
        self.record('POST', size)
        self.send_response(200)
        self.end_headers()


def check():
    rows = [json.loads(line) for line in (ROOT / 'requests.jsonl').read_text().splitlines()]
    assert any(r['method'] == 'GET' and r['path'] == '/v2/' for r in rows), rows
    assert any(r['method'] == 'POST' and r['path'] == '/legacy/' for r in rows), rows
    print('Local Docker login and PyPI upload endpoints received expected requests.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('mode', choices=['prepare', 'serve', 'check'])
    mode = parser.parse_args().mode
    if mode == 'prepare':
        prepare()
    elif mode == 'serve':
        # Only dummy credentials/artifacts. Docker action container reaches
        # the host's bridge address; no public package/registry is used.
        http.server.ThreadingHTTPServer(('0.0.0.0', 8787), Handler).serve_forever()
    else:
        check()
