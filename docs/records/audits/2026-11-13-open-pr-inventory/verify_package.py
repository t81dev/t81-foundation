"""Read-only stdlib integrity checks; not scientific or execution authentication."""
import hashlib
import json
from pathlib import Path
import zipfile

root = Path(__file__).resolve().parent
for name, expected in json.loads((root / 'SHA256SUMS.json').read_text()).items():
    assert hashlib.sha256((root / name).read_bytes()).hexdigest() == expected, name
identity = json.loads((root / 'original-snapshot/IDENTITY.json').read_text())
original = root / 'original-snapshot/complete-original-inventory.zip'
assert hashlib.sha256(original.read_bytes()).hexdigest() == identity['sha256']
with zipfile.ZipFile(original) as archive:
    for name, expected in json.loads(archive.read('SHA256SUMS.json')).items():
        assert hashlib.sha256(archive.read(name)).hexdigest() == expected, name
    old = json.loads(archive.read('inventory.json'))
current = json.loads((root / 'inventory.json').read_text())
assert len(old['records']) == len(current['records']) == 80
old_pairs = {r['number']: (r['head'], r['classification']) for r in old['records']}
assert len(old_pairs) == 80
for record in current['records']:
    assert (record['head'], record['classification']) == old_pairs[record['number']]
    assert record['classification_status'] == 'PROVISIONAL'
manifest = json.loads((root / 'closure-source-manifest.json').read_text())
with zipfile.ZipFile(root / 'closure-source-snapshots.zip') as archive:
    for entry in manifest:
        raw = archive.read(entry['archive_path'])
        assert len(raw) == entry['bytes']
        assert hashlib.sha256(raw).hexdigest() == entry['sha256']
        git_blob = b'blob ' + str(len(raw)).encode() + b'\0' + raw
        assert hashlib.sha1(git_blob).hexdigest() == entry['git_blob_id']
receipts = json.loads((root / 'comparison-receipts.json').read_text())
with zipfile.ZipFile(root / 'comparison-output.zip') as archive:
    for receipt in receipts:
        assert hashlib.sha256(archive.read(receipt['stdout_path'])).hexdigest() == receipt['stdout_sha256']
        archive.read(receipt['stderr_path'])
recommendations = json.loads((root / 'closure-recommendations.json').read_text())
assert {r['pr'] for r in recommendations} == {645, 747, 760, 766, 769}
for recommendation in recommendations:
    assert recommendation['status'] == 'PROVISIONAL'
    assert recommendation['posted'] is False and recommendation['executed'] is False
    assert recommendation['head_sha'] == next(r['head'] for r in current['records'] if r['number'] == recommendation['pr'])
print(f'PASS: 80 provisional records, original package identity, {len(manifest)} source blobs, 5 unposted recommendations and all file checksums')
