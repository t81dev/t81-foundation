#!/usr/bin/env python3
"""Committed Git mutation tests for Scope A preparation; no policy activation."""
import hashlib
import contextlib
import io
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
CHECKER = ROOT / 'scripts/governance/check_root_structure.py'
BASELINE = '71bb2c0e937dbcad73c9421e5465651b6e0d3ec3'
CONTRACT = 'docs/governance/root-layout-contract.json'
SCHEMA = 'docs/governance/root-layout-contract.schema.json'
spec = importlib.util.spec_from_file_location('root_checker', CHECKER)
module = importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)


class RootLayout(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.template = tempfile.TemporaryDirectory()
        root = Path(cls.template.name)
        cls.original = json.loads((ROOT / CONTRACT).read_text())
        preserved_modes = {}
        for entry in cls.original['entries']:
            path = root / entry['path']
            if 'object' in entry:
                raw = subprocess.check_output(['git', 'ls-tree', '-r', '-z', BASELINE, '--', entry['path']], cwd=ROOT)
                for row in raw.split(b'\0'):
                    if not row:
                        continue
                    meta, name = row.split(b'\t', 1)
                    mode, kind, oid = meta.decode().split()
                    assert kind == 'blob'
                    preserved_modes[name.decode()] = mode
                    target = root / name.decode()
                    target.parent.mkdir(parents=True, exist_ok=True)
                    target.write_bytes(subprocess.check_output(['git', 'cat-file', 'blob', oid], cwd=ROOT))
                    target.chmod(0o755 if mode == '100755' else 0o644)
            elif entry['kind'] == 'directory':
                path.mkdir(parents=True, exist_ok=True)
                (path / 'fixture.txt').write_text('required directory fixture\n')
            else:
                path.write_text('required file fixture\n')
                path.chmod(0o755 if entry['mode'] == '100755' else 0o644)
        for name in [CONTRACT, SCHEMA, module.CHECKER, module.LEGACY]:
            target = root / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / name, target)
        (root / 'CMakeLists.txt').write_text('add_subdirectory(src)\nadd_executable(research experiments/runner.cpp)\n')
        (root / 'src/CMakeLists.txt').write_text('add_executable(direct ${PROJECT_SOURCE_DIR}/development_tests/test.cpp)\n')
        (root / 'development_tests/test.cpp').write_text('// fixture\n')
        (root / 'experiments/runner.cpp').write_text('// fixture\n')
        cls.cmd(root, 'init', '-q')
        cls.cmd(root, 'config', 'user.name', 'Root Validator Fixture')
        cls.cmd(root, 'config', 'user.email', 'fixture@example.invalid')
        cls.cmd(root, 'config', 'commit.gpgsign', 'false')
        cls.cmd(root, 'config', 'core.autocrlf', 'false')
        # Preserve explicit index modes across platforms; mode-mutation tests use
        # update-index, rather than relying on filesystem executable support.
        cls.cmd(root, 'config', 'core.filemode', 'false')
        cls.cmd(root, 'add', '-f', '.')
        # Explicit modes also work when filesystem executable bits are unavailable.
        for entry in cls.original['entries']:
            if entry['kind'] == 'file':
                cls.cmd(root, 'update-index', '--chmod=' + ('+x' if entry['mode'] == '100755' else '-x'), entry['path'])
        for path, mode in preserved_modes.items():
            cls.cmd(root, 'update-index', '--chmod=' + ('+x' if mode == '100755' else '-x'), path)
        cls.cmd(root, 'commit', '-qm', 'valid fixture')

    @classmethod
    def tearDownClass(cls):
        cls.template.cleanup()

    @staticmethod
    def cmd(repo, *args):
        return subprocess.check_output(['git', *args], cwd=repo, stderr=subprocess.STDOUT).decode().strip()

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.repo = Path(self.temp.name) / 'candidate'
        shutil.copytree(self.template.name, self.repo)
        self.trust = Path(self.temp.name) / 'operator-approval.json'

    def commit(self):
        self.cmd(self.repo, 'add', '-f', '.')
        self.cmd(self.repo, 'commit', '-qm', 'mutation', '--allow-empty')

    def run_check(self, *args):
        return subprocess.run([sys.executable, str(CHECKER), '--repo-root', str(self.repo), *args], capture_output=True, text=True, timeout=30)

    def reject(self, diagnostic, *args):
        result = self.run_check(*(args or ('--structural',)))
        self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn(diagnostic, result.stdout + result.stderr)
        return result

    def contract_edit(self, edit):
        path = self.repo / CONTRACT
        data = json.loads(path.read_text())
        edit(data)
        path.write_text(json.dumps(data))

    def approve_fixture(self):
        # This file is test-operator input, never production approval evidence.
        commit, tree = module.resolve(self.repo, 'HEAD')
        tracked = module.parse_tree(module.git(self.repo, 'ls-tree', '-r', '-z', tree))
        paths = {CONTRACT, SCHEMA, module.CHECKER, module.LEGACY}
        paths |= {p for p in tracked if p.startswith(('.github/workflows/', '.github/actions/'))}
        self.trust.write_text(json.dumps({'version': 1, 'purpose': 'scope-a-preparation-review', 'approver': 't81dev', 'approved_commit': commit, 'artifacts': {p: hashlib.sha256(module.blob(self.repo, tree, p)).hexdigest() for p in sorted(paths)}}))

    def test_valid_approved_layout(self):
        result = self.run_check('--structural')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn('preservation-only exceptions: 17', result.stdout)

    def test_contract_matches_immutable_approved_proposal(self):
        proposal = json.loads(self.cmd(ROOT, 'show', BASELINE + ':docs/records/audits/2026-10-root-layout-contract/proposed-contract.json'))
        expected = {}
        for key in ['required_directories', 'required_files', 'unresolved_entries']:
            for entry in proposal[key]:
                expected[entry['path']] = (entry['type'], entry['baseline_git']['mode'],
                                          entry['baseline_git']['oid'] if key == 'unresolved_entries' else None)
        observed = {e['path']: (e['kind'], e['mode'], e.get('object')) for e in self.original['entries']}
        self.assertEqual(observed, expected)
        self.assertEqual(sum(e['kind'] == 'directory' and 'object' not in e for e in self.original['entries']), 31)
        self.assertEqual(sum(e['kind'] == 'file' and 'object' not in e for e in self.original['entries']), 29)

    def test_missing_required_file(self):
        (self.repo / 'LICENSE').unlink(); self.commit()
        self.reject('missing required file: LICENSE')

    def test_missing_required_directory(self):
        shutil.rmtree(self.repo / 'include'); self.commit()
        self.reject('missing required directory: include')

    def test_unknown_file(self):
        (self.repo / 'unauthorized.txt').write_text('x'); self.commit()
        self.reject('unknown root entry: unauthorized.txt')

    def test_unknown_directory(self):
        (self.repo / 'unauthorized').mkdir(); (self.repo / 'unauthorized/x').write_text('x'); self.commit()
        self.reject('unknown root entry: unauthorized')

    def test_file_replaces_directory(self):
        shutil.rmtree(self.repo / 'src'); (self.repo / 'src').write_text('x'); self.commit()
        self.reject('entry type/mode mismatch: src')

    def test_directory_replaces_file(self):
        (self.repo / 'LICENSE').unlink(); (self.repo / 'LICENSE').mkdir(); (self.repo / 'LICENSE/x').write_text('x'); self.commit()
        self.reject('entry type/mode mismatch: LICENSE')

    def test_symlink_substitution(self):
        # Plumbing avoids symlink privileges/developer-mode requirements on Windows.
        oid = subprocess.check_output(['git', 'hash-object', '-w', '--stdin'], cwd=self.repo, input=b'README.md').decode().strip()
        self.cmd(self.repo, 'update-index', '--cacheinfo', '120000,' + oid + ',LICENSE')
        self.cmd(self.repo, 'commit', '-qm', 'symlink mutation')
        self.reject('symlink forbidden: LICENSE')

    def test_gitlink_substitution(self):
        self.cmd(self.repo, 'rm', '-r', '-f', 'src')
        self.cmd(self.repo, 'update-index', '--add', '--cacheinfo', '160000,' + self.cmd(self.repo, 'rev-parse', 'HEAD') + ',src')
        self.cmd(self.repo, 'commit', '-qm', 'gitlink mutation')
        self.reject('gitlink forbidden: src')

    def test_generated_artifact(self):
        (self.repo / 'CMakeCache.txt').write_text('generated'); self.commit()
        self.reject('prohibited generated artifact: CMakeCache.txt')

    def test_missing_cmake_contract_dependency(self):
        self.contract_edit(lambda x: x.update(entries=[e for e in x['entries'] if e['path'] != 'development_tests']))
        # Physical presence cannot satisfy required classification: preservation is insufficient.
        self.contract_edit(lambda x: x['entries'].append({'path': 'development_tests', 'kind': 'directory', 'mode': '040000', 'object': self.cmd(self.repo, 'rev-parse', 'HEAD:development_tests'), 'owner': 't81dev', 'status': 'preservation-only', 'review_trigger': 'before any proposed change to this entry or root contract'}))
        self.contract_edit(lambda x: x['entries'].sort(key=lambda e: e['path']))
        self.commit(); self.reject('CMake input absent from required contract: development_tests')

    def test_head_worktree_divergence(self):
        (self.repo / 'LICENSE').unlink()
        (self.repo / CONTRACT).write_text('{"version":999}')
        self.assertEqual(self.run_check('--structural').returncode, 0)
        self.commit(); self.reject('unsupported contract version')
        shutil.copyfile(ROOT / CONTRACT, self.repo / CONTRACT)
        (self.repo / 'LICENSE').write_text('dirty repair')
        self.reject('unsupported contract version')

    def test_preservation_file_modified(self):
        (self.repo / 'record_hash.txt').write_text('changed'); self.commit()
        self.reject('preservation identity mismatch: record_hash.txt')

    def test_preservation_subtree_extended(self):
        (self.repo / 'skills/new-file').write_text('new'); self.commit()
        self.reject('preservation identity mismatch: skills')

    def test_preservation_directory_content_modified(self):
        target = next(p for p in (self.repo / 'skills').rglob('*') if p.is_file())
        target.write_bytes(target.read_bytes() + b'\nmutation')
        self.commit(); self.reject('preservation identity mismatch: skills')

    def test_preservation_missing(self):
        (self.repo / 'symbolic_algebraic_exported.tisc').unlink(); self.commit()
        self.reject('missing preservation entry: symbolic_algebraic_exported.tisc')

    def test_preservation_renamed(self):
        (self.repo / 'test_input.txt').rename(self.repo / 'renamed.txt'); self.commit()
        self.reject('missing preservation entry: test_input.txt')

    def test_wrong_executable_mode(self):
        self.cmd(self.repo, 'update-index', '--chmod=+x', 'LICENSE')
        self.cmd(self.repo, 'commit', '-qm', 'mode mutation')
        self.reject('entry type/mode mismatch: LICENSE')

    def test_authorization_valid_external_operator_fixture(self):
        self.approve_fixture()
        result = self.run_check('--authorization', '--trusted-approval', str(self.trust))
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        self.assertIn('NOT ACTIVATION', result.stdout)

    def test_authorization_absent(self):
        self.reject('absent trusted approval', '--authorization')

    def test_authorization_invalid(self):
        self.trust.write_text('{"approved":true}')
        self.reject('invalid trusted approval', '--authorization', '--trusted-approval', str(self.trust))

    def test_authorization_candidate_local(self):
        self.approve_fixture()
        local = self.repo / 'docs/approval.json'; shutil.copyfile(self.trust, local)
        self.reject('candidate-local approval forbidden', '--authorization', '--trusted-approval', str(local))

    def test_unapproved_commit(self):
        self.approve_fixture(); self.commit()
        self.reject('unapproved commit', '--authorization', '--trusted-approval', str(self.trust))

    def test_each_security_artifact_mutation(self):
        # Rebind fixture commit only, retaining original independently provided digests.
        for path in [CONTRACT, SCHEMA, module.CHECKER, module.LEGACY, '.github/workflows/fixture.txt']:
            with self.subTest(path=path):
                target = self.repo / path; target.parent.mkdir(parents=True, exist_ok=True)
                if not target.exists(): target.write_text('original')
                self.commit(); self.approve_fixture()
                original = target.read_bytes(); target.write_bytes(original + b'\nmutation'); self.commit()
                approval = json.loads(self.trust.read_text()); approval['approved_commit'] = self.cmd(self.repo, 'rev-parse', 'HEAD')
                self.trust.write_text(json.dumps(approval))
                self.reject('unapproved artifact revision: ' + path, '--authorization', '--trusted-approval', str(self.trust))
                target.write_bytes(original); self.commit()

    def test_workflow_addition_requires_coverage(self):
        self.approve_fixture(); (self.repo / '.github/workflows').mkdir(exist_ok=True)
        (self.repo / '.github/workflows/new.yml').write_text('new workflow'); self.commit()
        approval = json.loads(self.trust.read_text()); approval['approved_commit'] = self.cmd(self.repo, 'rev-parse', 'HEAD'); self.trust.write_text(json.dumps(approval))
        self.reject('artifact coverage mismatch', '--authorization', '--trusted-approval', str(self.trust))

    def test_malformed_git_data(self):
        for raw in [b'bad', b'bad\tname\0', b'100644 tree ' + b'a' * 40 + b'\tx\0', b'100644 blob ' + b'a' * 40 + b'\tx\0' + b'100644 blob ' + b'a' * 40 + b'\tx\0']:
            with self.subTest(raw=raw):
                with self.assertRaises(module.Invalid): module.parse_tree(raw)
                with (patch.object(module, 'resolve', return_value=('a' * 40, 'b' * 40)),
                      patch.object(module, 'git', return_value=raw),
                      patch.object(sys, 'argv', [str(CHECKER), '--structural', '--repo-root', str(self.repo)]),
                      contextlib.redirect_stdout(io.StringIO())):
                    self.assertNotEqual(module.main(), 0)

    def test_malformed_contract_entry(self):
        self.contract_edit(lambda x: x['entries'].__setitem__(0, 42)); self.commit()
        self.reject('invalid contract type')

    def test_workflow_deletion_requires_coverage(self):
        target = self.repo / '.github/workflows/trusted.yml'
        target.parent.mkdir(parents=True, exist_ok=True); target.write_text('fixture')
        self.commit(); self.approve_fixture(); target.unlink(); self.commit()
        approval = json.loads(self.trust.read_text()); approval['approved_commit'] = self.cmd(self.repo, 'rev-parse', 'HEAD')
        self.trust.write_text(json.dumps(approval))
        self.reject('artifact coverage mismatch', '--authorization', '--trusted-approval', str(self.trust))

    def test_unsupported_schema_revision(self):
        path = self.repo / SCHEMA; data = json.loads(path.read_text()); data['$schema'] = 'unsupported'; path.write_text(json.dumps(data)); self.commit()
        self.reject('unsupported schema revision')

    def test_unsupported_schema_keyword(self):
        path = self.repo / SCHEMA; data = json.loads(path.read_text()); data['unimplementedKeyword'] = True; path.write_text(json.dumps(data)); self.commit()
        self.reject('unsupported schema vocabulary')

    def test_duplicate_contract_key(self):
        path = self.repo / CONTRACT; path.write_text(path.read_text().replace('"version": 1', '"version": 1, "version": 1')); self.commit()
        self.reject('duplicate JSON key')

    def test_invalid_contract_path(self):
        self.contract_edit(lambda x: x['entries'][0].update(path='../escape')); self.commit()
        self.reject('invalid contract pattern')

    def test_invalid_revision(self):
        self.reject('Git operation failed', '--structural', '--revision', 'not-a-commit')

    def test_cmake_comments_do_not_create_dependencies(self):
        with (self.repo / 'CMakeLists.txt').open('a') as stream:
            stream.write('\n# add_subdirectory(fake)\nmessage("add_subdirectory(fake)")\n')
        self.commit(); result = self.run_check('--structural')
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_computed_cmake_dependency_rejected(self):
        with (self.repo / 'CMakeLists.txt').open('a') as stream: stream.write('\nadd_subdirectory(${unknown})\n')
        self.commit(); self.reject('unsupported computed CMake subdirectory')

    def test_git_replace_cannot_hide_unknown_root(self):
        original_tree = self.cmd(self.repo, 'rev-parse', 'HEAD^{tree}')
        (self.repo / 'unauthorized.txt').write_text('committed violation')
        self.commit()
        bad_tree = self.cmd(self.repo, 'rev-parse', 'HEAD^{tree}')
        self.cmd(self.repo, 'replace', bad_tree, original_tree)
        self.reject('unknown root entry: unauthorized.txt')

    def test_nested_repo_root_cannot_hide_outer_violation(self):
        good_tree = self.cmd(self.repo, 'rev-parse', 'HEAD^{tree}')
        (self.repo / 'unauthorized.txt').write_text('outer violation')
        self.cmd(self.repo, 'add', '-f', 'unauthorized.txt')
        self.cmd(self.repo, 'read-tree', '--prefix=docs/shadow/', good_tree)
        self.cmd(self.repo, 'commit', '-qm', 'nested clean shadow')
        nested = self.repo / 'docs/shadow'; nested.mkdir()
        self.reject('repository root must be Git working-tree top level',
                    '--structural', '--repo-root', str(nested))

    def test_git_replace_cannot_forge_authorization_digest(self):
        old_oid = self.cmd(self.repo, 'rev-parse', 'HEAD:' + module.CHECKER)
        self.approve_fixture()
        path = self.repo / module.CHECKER
        path.write_bytes(path.read_bytes() + b'\n# unapproved revision\n'); self.commit()
        new_oid = self.cmd(self.repo, 'rev-parse', 'HEAD:' + module.CHECKER)
        approval = json.loads(self.trust.read_text())
        approval['approved_commit'] = self.cmd(self.repo, 'rev-parse', 'HEAD')
        self.trust.write_text(json.dumps(approval))
        self.cmd(self.repo, 'replace', new_oid, old_oid)
        self.reject('unapproved artifact revision: ' + module.CHECKER,
                    '--authorization', '--trusted-approval', str(self.trust))

    def test_nonfinite_schema_json_is_rejected(self):
        path = self.repo / SCHEMA
        data = json.loads(path.read_text()); data['title'] = float('nan')
        path.write_text(json.dumps(data)); self.commit()
        self.reject('non-finite JSON constant')

    def test_cmake_literal_source_list_cannot_hide_preserved_root(self):
        with (self.repo / 'CMakeLists.txt').open('a') as stream:
            stream.write('\ntarget_sources(direct PRIVATE development_tests/test.cpp;market-demos/customer_demo_system.cpp)\n')
        self.commit()
        self.reject('CMake input absent from required contract: market-demos')

    def test_cmake_current_source_dir_cannot_hide_preserved_root(self):
        with (self.repo / 'CMakeLists.txt').open('a') as stream:
            stream.write('\ntarget_sources(direct PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/market-demos/customer_demo_system.cpp)\n')
        self.commit()
        self.reject('CMake input absent from required contract: market-demos')

    def commit_raw_root_entry(self, name, mode, oid):
        tree = self.cmd(self.repo, 'rev-parse', 'HEAD^{tree}')
        entries = module.parse_tree(subprocess.check_output(['git', 'ls-tree', '-z', tree], cwd=self.repo))
        old_mode, _, old_oid = entries[name]
        old_mode = '40000' if old_mode == '040000' else old_mode
        raw = subprocess.check_output(['git', 'cat-file', 'tree', tree], cwd=self.repo)
        original = old_mode.encode() + b' ' + name.encode() + b'\0' + bytes.fromhex(old_oid)
        replacement = mode.encode() + b' ' + name.encode() + b'\0' + bytes.fromhex(oid)
        self.assertEqual(raw.count(original), 1)
        raw = raw.replace(original, replacement)
        new_tree = subprocess.check_output(['git', 'hash-object', '--literally', '-w', '-t', 'tree', '--stdin'], cwd=self.repo, input=raw).decode().strip()
        commit = self.cmd(self.repo, 'commit-tree', new_tree, '-p', 'HEAD', '-m', 'raw tree mutation')
        self.cmd(self.repo, 'update-ref', 'HEAD', commit)

    def test_noncanonical_raw_git_mode_rejected(self):
        self.commit_raw_root_entry('LICENSE', '100664', self.cmd(self.repo, 'rev-parse', 'HEAD:LICENSE'))
        self.reject('noncanonical Git root mode')

    def test_directory_mode_cannot_disguise_blob_object(self):
        self.commit_raw_root_entry('artifacts', '40000', self.cmd(self.repo, 'rev-parse', 'HEAD:LICENSE'))
        self.reject('Git root object type')

    def test_file_mode_cannot_disguise_tree_object(self):
        self.commit_raw_root_entry('LICENSE', '100644', self.cmd(self.repo, 'rev-parse', 'HEAD:artifacts'))
        self.reject('Git root object type')

    def test_missing_root_blob_object_rejected(self):
        self.commit_raw_root_entry('LICENSE', '100644', '1' * 40)
        self.reject('Git root object type')

    def test_unsorted_raw_root_tree_rejected(self):
        raw = subprocess.check_output(['git', 'cat-file', 'tree', 'HEAD^{tree}'], cwd=self.repo)
        first = raw.index(b'\0') + 21
        second = raw.index(b'\0', first) + 21
        raw = raw[first:second] + raw[:first] + raw[second:]
        tree = subprocess.check_output(['git', 'hash-object', '--literally', '-w', '-t', 'tree', '--stdin'], cwd=self.repo, input=raw).decode().strip()
        commit = self.cmd(self.repo, 'commit-tree', tree, '-p', 'HEAD', '-m', 'unsorted tree')
        self.cmd(self.repo, 'update-ref', 'HEAD', commit)
        self.reject('noncanonical Git root ordering')

    def test_unusual_unknown_root_names_rejected(self):
        for name in [' leading', 'trailing ', 'line\nbreak', 'tab\tname', 'ümlaut', '-dash', '[glob]*.log']:
            with self.subTest(name=name):
                path = self.repo / name; path.write_text('unknown entry'); self.commit()
                self.reject('unknown root entry')
                path.unlink(); self.commit()

    def test_non_utf8_git_name_rejected(self):
        oid = self.cmd(self.repo, 'rev-parse', 'HEAD:LICENSE')
        subprocess.run(['git', 'update-index', '-z', '--index-info'], cwd=self.repo,
                       input=b'100644 ' + oid.encode() + b'\tinvalid-\xff\0', check=True)
        self.cmd(self.repo, 'commit', '-qm', 'invalid UTF-8 index name')
        self.reject('malformed raw Git root entry')

    def test_nonfinite_constants_all_rejected(self):
        for constant in ['NaN', 'Infinity', '-Infinity']:
            with self.subTest(constant=constant), self.assertRaises(module.Invalid):
                module.json_read('{"unused":' + constant + '}')

    def test_local_action_content_cannot_self_authorize(self):
        target = self.repo / '.github/actions/local/action.yml'
        target.parent.mkdir(parents=True); target.write_text('original fixture action')
        self.commit(); self.approve_fixture()
        target.write_text('unapproved action'); self.commit()
        data = json.loads(self.trust.read_text()); data['approved_commit'] = self.cmd(self.repo, 'rev-parse', 'HEAD')
        self.trust.write_text(json.dumps(data))
        self.reject('unapproved artifact revision: .github/actions/local/action.yml',
                    '--authorization', '--trusted-approval', str(self.trust))

    def test_no_argument_default_is_original_gate(self):
        original = subprocess.check_output(['git', '--no-replace-objects', 'show', BASELINE + ':' + module.CHECKER], cwd=ROOT)
        self.assertEqual((ROOT / module.LEGACY).read_bytes(), original)
        expected = subprocess.run([sys.executable, str(ROOT / module.LEGACY)], capture_output=True, text=True, timeout=30)
        env = dict(os.environ, T81_ROOT_STRUCTURE_MODE='structural', T81_ROOT_POLICY_ACTIVATE='1')
        actual = subprocess.run([sys.executable, str(CHECKER)], env=env, capture_output=True, text=True, timeout=30)
        self.assertEqual((actual.returncode, actual.stdout, actual.stderr), (expected.returncode, expected.stdout, expected.stderr))
        self.assertNotEqual(actual.returncode, 0)
        self.assertNotIn('committed root structure PASSED', actual.stdout)

    def test_required_workflows_and_actions_are_unchanged(self):
        diff = subprocess.check_output(['git', '--no-replace-objects', 'diff', BASELINE, '--', '.github/workflows', '.github/actions'], cwd=ROOT)
        self.assertEqual(diff, b'')


if __name__ == '__main__':
    unittest.main(verbosity=2)
