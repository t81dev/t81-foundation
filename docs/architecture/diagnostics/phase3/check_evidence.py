#!/usr/bin/env python3
"""Offline identity and scope checks for the Phase 3 retained observations.

This is an evidence consistency check authored with the investigation, not an
independent acceptance test, guest execution, or replacement for verification.py.
"""
import gzip
import hashlib
import json
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
EVIDENCE = HERE / 'evidence'
ROOT = HERE.parents[3]
BASE = 'cd490f5b5c0496350682542b56424dd77bea6963'
READY = '[axion] t81sh: ready (principal=axion, tier=1)'
HELP = 'show RFC-00B9 shell commands'
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


for name, expected in read('SHA256SUMS.json').items():
    require(digest((EVIDENCE / name).read_bytes()) == expected, f'hash mismatch: {name}')
media, fresh = read('media.json'), read('fresh-media.json')
for before, after in ((media, read('substituted-media.json')), (fresh, read('reverse-media.json'))):
    a, b = before['file_tree_sha256'], after['file_tree_sha256']
    changed = {name for name in set(a) | set(b) if a.get(name) != b.get(name)}
    require(changed == {'EFI/BOOT/BOOTAA64.EFI'}, 'substitution changed other files')
require(media['header_crc_valid'] and media['entries_crc_valid'], 'tracked GPT CRC')
require(fresh['header_crc_valid'] and fresh['entries_crc_valid'], 'fresh GPT CRC')
require(media['embedded_matches_staging'] and fresh['embedded_matches_staging'], 'EFI extraction mismatch')
require(not media['contains_ready_marker'] and media['contains_debug_wfi'], 'historical payload classification')
require(fresh['contains_ready_marker'] and not fresh['contains_debug_wfi'], 'fresh payload classification')
require(media['pe']['machine'] == fresh['pe']['machine'] == '0xaa64', 'architecture mismatch')
require(digest(gzip.decompress((EVIDENCE / 'fresh-image.img.gz').read_bytes())) == fresh['image_sha256'], 'fresh archive identity')
require(digest((ROOT / 'drivers/qemu/production_image/qemu_slice6_guest.img').read_bytes()) == media['image_sha256'], 'tracked image changed')
observed = {record['name']: record for record in read('observations.json')}
for name, record in observed.items():
    require(record['result']['observation_only'] is True, f'{name}: scope missing')
    require(record['result']['qemu_exit'] == 0, f'{name}: QMP shutdown not zero')
    require('"return": {}' in record['raw']['qmp.log'], f'{name}: missing QMP replies')
    require('ArmTrngLib could not be correctly initialized.' in record['raw']['serial.log'], f'{name}: lost firmware output')
    require('Error: Image' in record['raw']['serial.log'], f'{name}: lost firmware error')
for name in ('tracked-original', 'tracked-max', 'tracked-pflash', 'tracked-nodefaults',
             'tracked-pflash-only', 'fresh-historical-payload'):
    record = observed[name]
    require(not record['result']['ready'] and READY not in record['raw']['serial.log'], f'{name}: unexpected shell marker')
    require('DEBUG: WFI loop iteration' in record['raw']['serial.log'], f'{name}: historical loop missing')
for name in ('fresh-original', 'fresh-pflash', 'tracked-current-payload'):
    record = observed[name]
    require(record['result']['ready'] and READY in record['raw']['serial.log'], f'{name}: no real ready output')
    require(record['result']['fresh_help'] and HELP in record['raw']['serial.log'], f'{name}: no help response')
require(READY not in observed['no-guest-disk']['raw']['serial.log'], 'diskless shell claim')
require('start failed: 00000001' in observed['no-guest-disk']['raw']['serial.log'], 'diskless firmware control missing')
for record in read('verifier-runs.json'):
    require(record['exit'] == 1, 'unexpected full-verifier exit')
    require('Tests Completed: 2/5' in record['stdout'], 'unexpected full-verifier count')
    require('Status: VERIFICATION FAILED' in record['stdout'], 'unexpected full-verifier disposition')
    require('VERIFICATION SUCCESSFUL' not in record['stdout'], 'false acceptance claim')
for path in ('drivers/qemu/scripts/verification.py', 'drivers/qemu/scripts/VERIFICATION.md',
             'drivers/qemu/scripts/automated_verification.sh', 'drivers/qemu/scripts/verify_production.sh',
             'ternaryos/automated_verification.sh', 'ternaryos/production_qemu_setup.sh'):
    original = subprocess.check_output(['git', 'show', f'{BASE}:{path}'], cwd=ROOT)
    require((ROOT / path).read_bytes() == original, f'contract/source changed: {path}')
print(f'{count} evidence consistency checks passed; NOT a five-check native acceptance pass')
