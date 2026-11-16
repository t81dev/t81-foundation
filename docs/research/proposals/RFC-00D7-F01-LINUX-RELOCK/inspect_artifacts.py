"""Static artifact inspection only; no installation, imports from artifacts or ELF loading.

Explicit --fetch reads bounded official artifacts and writes inert inspection JSON.
This is not a qualification probe, interpreter builder, installer or A-F entry.
"""
import argparse
import base64
import csv
import email.parser
import hashlib
import io
import json
import struct
import tarfile
import time
import urllib.request
import zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SOURCE = 'https://www.python.org/ftp/python/3.14.3/Python-3.14.3.tar.xz'
SOURCE_SHA = 'a97d5549e9ad81fe17159ed02c68774ad5d266c72f8d9a0b5a9c371fe85d902b'


def sha(raw):
    return hashlib.sha256(raw).hexdigest()


def save(name, value):
    raw = (json.dumps(value, indent=2, sort_keys=True) + '\n').encode()
    assert len(raw) < 4 * 1024**2
    (ROOT / name).write_bytes(raw)


def retrieve(url, cap):
    assert url.startswith(('https://pypi.org/', 'https://files.pythonhosted.org/', 'https://www.python.org/'))
    started = time.monotonic()
    with urllib.request.urlopen(url, timeout=15) as stream:
        assert stream.geturl().startswith(('https://pypi.org/', 'https://files.pythonhosted.org/', 'https://www.python.org/'))
        raw = bytearray()
        while True:
            assert time.monotonic() - started < 60
            chunk = stream.read(min(65536, cap + 1 - len(raw)))
            if not chunk:
                return bytes(raw)
            raw.extend(chunk)
            assert len(raw) <= cap


def elf(raw):
    assert raw[:6] == b'\x7fELF\x02\x01' and len(raw) <= 8 * 1024**2
    header = struct.unpack_from('<HHIQQQIHHHHHH', raw, 16)
    assert header[0] == 3 and header[1] in (62, 183) and header[10] == 64
    assert 0 < header[11] <= 128 and header[5] + 64 * header[11] <= len(raw)
    sections = [struct.unpack_from('<IIQQQQIIQQ', raw, header[5] + 64 * i) for i in range(header[11])]
    def contents(section):
        offset, size = section[4:6]
        assert offset + size <= len(raw)
        return raw[offset:offset + size]
    def string(table, offset):
        assert 0 <= offset < len(table)
        end = table.index(b'\0', offset)
        assert end - offset < 4096
        return table[offset:end].decode('ascii')
    names = contents(sections[header[12]])
    named = {string(names, section[0]): section for section in sections}
    dynamic = named['.dynamic']
    strings = contents(sections[dynamic[6]])
    data = contents(dynamic)
    dependencies, paths = [], []
    for offset in range(0, len(data), 16):
        tag, value = struct.unpack_from('<qQ', data, offset)
        if tag == 1:
            dependencies.append(string(strings, value))
        if tag in (15, 29):
            paths.append({'tag': tag, 'path': string(strings, value)})
    versions = []
    version = contents(named['.gnu.version_r'])
    offset = 0
    while True:
        _, count, library, auxiliary, next_record = struct.unpack_from('<HHIII', version, offset)
        assert count < 128
        cursor = offset + auxiliary
        for _ in range(count):
            _, _, _, name, next_aux = struct.unpack_from('<IHHII', version, cursor)
            versions.append({'library': string(strings, library), 'version': string(strings, name)})
            if not next_aux:
                break
            cursor += next_aux
        if not next_record:
            break
        assert next_record > 0
        offset += next_record
        assert offset < len(version)
    symbols = named['.dynsym']
    symbol_names = contents(sections[symbols[6]])
    undefined, initializers = [], []
    data = contents(symbols)
    for offset in range(0, len(data), 24):
        name, _, _, section, _, _ = struct.unpack_from('<IBBHQQ', data, offset)
        if name:
            value = string(symbol_names, name)
            if section == 0:
                undefined.append(value)
            if value.startswith('PyInit_'):
                initializers.append(value)
    return {'elf_class': 64, 'endianness': 'little', 'machine': header[1],
            'needed': dependencies, 'rpath_runpath': paths, 'version_requirements': versions,
            'undefined_symbols': sorted(undefined), 'python_initializers': initializers,
            'runtime_loaded': False, 'instruction_set_requirements_verified': False}


def inspect_wheel(raw, selected):
    assert sha(raw) == selected['sha256'] and len(raw) == selected['size']
    with zipfile.ZipFile(io.BytesIO(raw)) as archive:
        entries = archive.infolist()
        assert len(entries) <= 512 and len({entry.filename for entry in entries}) == len(entries)
        assert sum(entry.file_size for entry in entries) <= 16 * 1024**2
        members = {}
        for entry in entries:
            name = entry.filename
            assert not name.startswith('/') and '..' not in Path(name).parts
            assert not name.endswith(('.pth', 'sitecustomize.py', 'usercustomize.py'))
            assert entry.file_size <= 8 * 1024**2
            if not entry.is_dir():
                data = archive.read(name)
                members[name] = {'sha256': sha(data), 'size': len(data)}
        record_name = next(name for name in members if name.endswith('.dist-info/RECORD'))
        rows = list(csv.reader(io.StringIO(archive.read(record_name).decode())))
        assert {row[0] for row in rows} == set(members) and len(rows) == len(members)
        for name, value, size in rows:
            if name == record_name:
                assert not value and not size
            else:
                algorithm, encoded = value.split('=', 1)
                assert algorithm == 'sha256'
                assert base64.urlsafe_b64decode(encoded + '=' * (-len(encoded) % 4)).hex() == members[name]['sha256']
                assert int(size) == members[name]['size']
        wheel = next(name for name in members if name.endswith('.dist-info/WHEEL'))
        metadata = next(name for name in members if name.endswith('.dist-info/METADATA'))
        parse = email.parser.BytesParser()
        information = parse.parsebytes(archive.read(metadata))
        tags = parse.parsebytes(archive.read(wheel)).get_all('Tag')
        native = []
        for name in members:
            if name.endswith('.so'):
                native.append(dict(path=name, sha256=members[name]['sha256'], inspection=elf(archive.read(name))))
        return dict(selected, observed_sha256=sha(raw), record_members_checked=len(rows),
                    name=information['Name'], version=information['Version'],
                    requires_python=information['Requires-Python'],
                    requires_dist=information.get_all('Requires-Dist') or [], wheel_tags=tags,
                    metadata_sha256=members[metadata]['sha256'], wheel_metadata_sha256=members[wheel]['sha256'],
                    native=native, members=members)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--fetch', action='store_true', required=True)
    parser.add_argument('--prior-proposals', type=Path)
    args = parser.parse_args()
    if not __debug__:
        raise ValueError('assertions required')
    if args.prior_proposals is not None:
        profiles = [json.loads((args.prior_proposals / ('dependency-profile-' + arch + '.json')).read_text())
                    for arch in ('x86_64', 'aarch64')]
    else:
        # Self-contained independent recheck uses the exact retained selections.
        profiles = [{'packages': json.loads((ROOT / ('profile-' + arch + '.proposed.json')).read_text())['wheels']}
                    for arch in ('x86_64', 'aarch64')]
    selected = {row['filename']: row for profile in profiles for row in profile['packages']}
    evidence = []
    metadata_dir = ROOT / 'registry-metadata'
    metadata_dir.mkdir(exist_ok=True)
    releases = {}
    for profile in profiles:
        for package in profile['packages']:
            key = (package['name'], package['version'])
            if key not in releases:
                raw = retrieve('https://pypi.org/pypi/' + '/'.join(key) + '/json', 1024**2)
                (metadata_dir / ('-'.join(key) + '.json')).write_bytes(raw)
                releases[key] = json.loads(raw)
    for filename, prior in sorted(selected.items()):
        release = releases[(prior['name'], prior['version'])]
        row = next(item for item in release['urls'] if item['filename'] == filename)
        assert not row['yanked'] and row['digests']['sha256'] == prior['sha256']
        assert row['url'] == prior['url'] and row['size'] == prior['size']
        choice = {key: prior[key] for key in ('name', 'version', 'filename', 'sha256', 'size', 'url')}
        evidence.append(inspect_wheel(retrieve(choice['url'], 2 * 1024**2), choice))
    save('wheel-inspection.json', {'kind': 'static_artifact_inspection', 'artifacts': evidence,
         'unique_archive_bytes': sum(row['size'] for row in evidence), 'packages_installed': 0,
         'artifact_code_executed': False, 'host_qualified': False})
    raw = retrieve(SOURCE, 32 * 1024**2)
    assert sha(raw) == SOURCE_SHA
    library, critical = [], []
    desired = {'Modules/getpath.py', 'Python/pathconfig.c', 'Python/initconfig.c',
               'Include/patchlevel.h', 'configure', 'configure.ac', 'Makefile.pre.in'}
    # Streaming TAR: no extract()/extractall(), no package/source code execution.
    with tarfile.open(fileobj=io.BytesIO(raw), mode='r|xz') as archive:
        for entry in archive:
            name = entry.name.removeprefix('Python-3.14.3/')
            if entry.isfile() and (name.startswith('Lib/') or name in desired):
                assert entry.size <= 16 * 1024**2
                stream = archive.extractfile(entry)
                result = hashlib.sha256()
                size = 0
                while chunk := stream.read(65536):
                    result.update(chunk)
                    size += len(chunk)
                assert size == entry.size
                item = {'path': name, 'sha256': result.hexdigest(), 'size': size}
                (library if name.startswith('Lib/') else critical).append(item)
            assert len(library) < 5000
    save('stdlib-source-inventory.json', {'source_sha256': SOURCE_SHA, 'scope': 'Source Lib/ regular members; not an installed Linux stdlib',
                                        'files': sorted(library, key=lambda row: row['path'])})
    save('cpython-source-inspection.json', {'url': SOURCE, 'version': '3.14.3', 'observed_sha256': sha(raw),
         'size': len(raw), 'critical_members': critical, 'stdlib_regular_files': len(library),
         'stdlib_source_inventory_sha256': sha((ROOT / 'stdlib-source-inventory.json').read_bytes()),
         'built': False, 'interpreter_executed': False, 'target_binary_sha256': None,
         'source_authenticity_signature_verified': False})
    print(json.dumps({'wheel_archives': len(evidence), 'wheel_archive_bytes': sum(row['size'] for row in evidence),
                      'source_archive_bytes': len(raw), 'source_lib_files': len(library),
                      'packages_installed': 0, 'artifact_code_executed': False, 'host_qualified': False}, sort_keys=True))


if __name__ == '__main__':
    main()
