#!/usr/bin/env python3
"""Offline checks of retained Phase 4 identities and bounded conclusions.

Same-author consistency checks, not native execution or independent validation.
"""
import gzip
import hashlib
import json
from pathlib import Path
import re
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
EVIDENCE = HERE / 'evidence'
BASE = '6c9eaf1a2fc8368cd06f6dddbbdcd78e6606c0ce'
count = 0


def require(condition, message):
    global count
    count += 1
    if not condition:
        raise SystemExit(message)


def read(name):
    return json.loads((EVIDENCE / name).read_text())


def digest(data):
    return hashlib.sha256(data).hexdigest()


for name, value in read('SHA256SUMS.json').items():
    require(digest((EVIDENCE / name).read_bytes()) == value, 'hash mismatch: ' + name)
provenance = read('provenance.json')
require(provenance['baseline'] == BASE, 'baseline mismatch')
image_hash = provenance['input_sha256']['/private/tmp/t81-phase4/guest-image/qemu_slice6_guest.img']
image = gzip.decompress((EVIDENCE / 'fresh-guest.img.gz').read_bytes())
require(digest(image) == image_hash, 'fresh image archive mismatch')
require(b'[axion] t81sh: ready (principal=axion, tier=1)' in image, 'shell-capable payload missing')
rows = read('matrix.json')
require({r['name'] for r in rows} == {'supplied-original', 'supplied-matched', 'diagnostic-original', 'diagnostic-matched'}, 'matrix bounds changed')
for row in rows:
    require(row['exit'] == 1, 'unexpected verifier exit')
    require('Tests Completed: 2/5' in row['stdout'], 'unexpected completion count')
    require('Status: VERIFICATION FAILED' in row['stdout'], 'missing negative disposition')
    require('VERIFICATION SUCCESSFUL' not in row['stdout'], 'false success claim')
    require(row['image_sha256'] == image_hash, 'guest input not controlled')
    require('Error: Image' in row['raw']['serial.log'], 'firmware diagnostic lost')
    require('"running": true' in row['raw']['qmp.log'], 'QMP execution missing')
    require('"return": {}' in row['raw']['qmp.log'], 'QMP replies missing')
    if row['name'].endswith('matched'):
        argv = row['executed_qemu_argv']
        require(argv[argv.index('-cpu') + 1] == 'max', 'matched CPU mismatch')
        require('ramfb' in argv and 'virtio-rng-pci' in argv, 'matched devices missing')
        require('-bios' not in argv and any('if=pflash' in arg for arg in argv), 'pflash mismatch')
        require(not any('DebugLevel' in arg or 'DebugPrint' in arg for arg in argv), 'runtime log override')
observed = {r['name']: r for r in read('observations.json')}
old, matched = observed['diagnostic-final-a57'], observed['diagnostic-final-matched']
expected = {'RngDxe': '0xA000000000000001', 'QemuRamfbDxe': '0x800000000000000E',
            'Tcg2Dxe': '0x8000000000000003', 'QemuKernelLoaderFsDxe': '0x800000000000000E',
            'TlsDxe': '0x8000000000000015'}
for module, status in expected.items():
    pattern = r'T81DIAG pdb=[^\r\n]*/' + module + r'\.dll status=(0x[0-9A-Fa-f]+)'
    require(re.findall(pattern, old['raw']['serial.log']) == [status], 'baseline attribution: ' + module)
    target = status if module in ('Tcg2Dxe', 'QemuKernelLoaderFsDxe') else '0x0'
    require(re.findall(pattern, matched['raw']['serial.log']) == [target], 'matched attribution: ' + module)
for record in (old, matched):
    serial = record['raw']['serial.log']
    require('T81TRNG stage=0 smccc=0xFFFFFFFFFFFFFFFF status=0x8000000000000003' in serial, 'TRNG stage missing')
    require('No TPM2 instance required!' in serial, 'TPM branch evidence missing')
    require(record['result']['observation_only'] is True, 'diagnostic scope missing')
    require(record['result']['qemu_exit'] == 0, 'diagnostic QMP shutdown missing')
    require(record['result']['ready'] and record['result']['fresh_help'], 'real guest observation missing')
for path in ('drivers/qemu/scripts/verification.py', 'drivers/qemu/scripts/VERIFICATION.md',
             'drivers/qemu/scripts/automated_verification.sh', 'drivers/qemu/scripts/verify_production.sh',
             'ternaryos/automated_verification.sh', 'ternaryos/production_qemu_setup.sh',
             'drivers/qemu/production_image/qemu_slice6_guest.img'):
    original = subprocess.check_output(['git', 'show', f'{BASE}:{path}'], cwd=ROOT)
    require((ROOT / path).read_bytes() == original, 'contract/artifact changed: ' + path)
patch = read('instrumentation.json')['patch']
removed = [line for line in patch.splitlines() if line.startswith('-') and not line.startswith('---')]
require(not removed, 'diagnostic instrumentation removed source')
require('Image->Status  = Image->EntryPoint' in patch, 'status instrumentation anchor missing')
print(f'{count} evidence consistency checks passed; native acceptance remains BLOCKED')
