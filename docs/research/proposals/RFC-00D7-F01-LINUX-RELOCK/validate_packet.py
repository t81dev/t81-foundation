"""Offline consistency/source/AST checks only. No network, artifacts or harness imports."""
import ast
import hashlib
import json
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parent
REPO = ROOT.parents[3]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def load(name):
    def unique(pairs):
        value = {}
        for key, item in pairs:
            assert key not in value, key
            value[key] = item
        return value
    return json.loads((ROOT / name).read_text(), object_pairs_hook=unique)


def main():
    if not __debug__:
        raise ValueError('assertions required')
    pins = load('source-pins.json')
    base = pins['accepted_source']
    assert base == '0afbc339288b44cc7536af6c227f957257bb8ff6'
    for row in pins['sources']:
        obj = row['revision'] + ':' + row['path']
        raw = subprocess.check_output(['git', 'show', obj], cwd=REPO)
        assert hashlib.sha256(raw).hexdigest() == row['sha256']
        assert subprocess.check_output(['git', 'rev-parse', obj], cwd=REPO, text=True).strip() == row['git_blob']
        assert (REPO / row['path']).read_bytes() == raw
    subprocess.check_call(['git', 'diff', '--quiet', base, '--', 'experiments/rfc_00d7_f01'], cwd=REPO)
    assert json.loads((REPO / 'experiments/rfc_00d7_f01/dependency-lock.json').read_text())['linux_relock_required'] is True
    inventory = load('stdlib-source-inventory.json')
    source = load('cpython-source-inspection.json')
    assert source['observed_sha256'] == 'a97d5549e9ad81fe17159ed02c68774ad5d266c72f8d9a0b5a9c371fe85d902b'
    assert sha(ROOT / 'stdlib-source-inventory.json') == source['stdlib_source_inventory_sha256']
    assert len(inventory['files']) == source['stdlib_regular_files'] == 2405
    assert len({row['path'] for row in inventory['files']}) == 2405
    interpreter = load('interpreter-profile.proposed.json')
    assert all(value is None for value in interpreter['required_target_evidence'].values())
    layout = interpreter['proposed_layout']
    assert layout['include_system_site_packages'] is False
    assert sha(ROOT / layout['pyvenv_cfg_proposed_bytes_file']) == layout['pyvenv_cfg_proposed_sha256']
    cfg = dict(row.split(' = ', 1) for row in (ROOT / 'pyvenv.cfg.proposed').read_text().splitlines())
    assert cfg['include-system-site-packages'] == 'false' and cfg['version'] == '3.14.3'
    assert cfg['home'] == str(Path(layout['resolved_base_executable']).parent)
    assert interpreter['version'] == '3.14.3' and interpreter['runtime_policy']['abi'] == 'cp314'
    observed = load('wheel-inspection.json')
    assert observed['unique_archive_bytes'] == 948967 and len(observed['artifacts']) == 6
    artifacts = {row['filename']: row for row in observed['artifacts']}
    checked_members = 0
    for row in artifacts.values():
        assert row['sha256'] == row['observed_sha256']
        assert row['record_members_checked'] == len(row['members'])
        checked_members += len(row['members'])
        release = load('registry-metadata/' + row['name'].replace('_', '-') + '-' + row['version'] + '.json')
        selected = next(item for item in release['urls'] if item['filename'] == row['filename'])
        assert not selected['yanked'] and selected['url'] == row['url'] and selected['size'] == row['size']
        assert selected['digests']['sha256'] == row['observed_sha256']
    versions = {'attrs':'26.1.0', 'jsonschema':'4.26.0', 'jsonschema-specifications':'2025.9.1',
                'referencing':'0.37.0', 'rpds-py':'2026.9.1'}
    native_machines = []
    for architecture, machine in (('x86_64', 62), ('aarch64', 183)):
        profile = load('profile-' + architecture + '.proposed.json')
        assert 'packages' not in profile  # Not an active installed-file lock.
        assert profile['elf_machine'] == machine
        assert profile['linux_relock_required'] is True and profile['execution_authorized'] is False
        assert profile['host_qualified'] is False
        assert profile['interpreter_profile_sha256'] == sha(ROOT / 'interpreter-profile.proposed.json')
        assert all(value is None for value in profile['target_runtime_identity'].values())
        assert {row['name'].replace('_', '-'):row['version'] for row in profile['wheels']} == versions
        requirements = (ROOT / ('requirements-' + architecture + '.proposed.lock')).read_text().splitlines()[1:]
        assert requirements == [row['name'] + '==' + row['version'] + ' --hash=sha256:' + row['sha256'] for row in profile['wheels']]
        for row in profile['wheels']:
            actual = artifacts[row['filename']]
            assert all(actual[key] == value for key, value in row.items())
        assert len(profile['native']) == 1
        native = profile['native'][0]['inspection']
        assert native['machine'] == machine and native['elf_class'] == 64
        assert native['python_initializers'] == ['PyInit_rpds']
        assert native['rpath_runpath'] == []
        assert native['runtime_loaded'] is False
        assert all(tuple(map(int, row['version'][6:].split('.'))) <= (2, 17)
                   for row in native['version_requirements'] if row['version'].startswith('GLIBC_'))
        native_machines.append(machine)
    compiled = 0
    for path in ROOT.glob('*.py'):
        tree = ast.parse(path.read_text(), filename=str(path))
        compile(tree, str(path), 'exec')  # No exec of the resulting code object.
        compiled += 1
        assert not any(isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute)
                       and node.func.attr in ('extract', 'extractall', 'CDLL', 'PyDLL') for node in ast.walk(tree))
    # Inspect accepted boundary source as AST; never import or call it.
    boundary_tree = ast.parse((REPO / 'experiments/rfc_00d7_f01/runner_bootstrap.py').read_text())
    boundary = next(node for node in boundary_tree.body if isinstance(node, ast.FunctionDef) and node.name == 'interpreter_boundary')
    assert "entries.get('include-system-site-packages', '').lower() != 'false'" in ast.unparse(boundary)
    manifest = {}
    for row in (ROOT / 'SHA256SUMS').read_text().splitlines():
        expected, name = row.split('  ', 1)
        assert name not in manifest and sha(ROOT / name) == expected, name
        manifest[name] = expected
    files = {str(path.relative_to(ROOT)) for path in ROOT.rglob('*') if path.is_file()}
    assert files == set(manifest) | {'SHA256SUMS'}
    total = sum(path.stat().st_size for path in ROOT.rglob('*') if path.is_file())
    assert total < 4 * 1024**2
    print(json.dumps({'source_pins':len(pins['sources']), 'profiles':2, 'wheel_records':len(artifacts),
                      'record_member_observations':checked_members, 'stdlib_source_files':2405,
                      'native_machines':native_machines, 'compiled_preparation_modules':compiled,
                      'packet_hashes':len(manifest), 'active_harness_unchanged':True,
                      'linux_relock_required':True, 'host_qualified':False,
                      'artifact_or_harness_execution':False}, sort_keys=True))


if __name__ == '__main__':
    main()
