"""Preparation-only checks. Never imports or executes model/domain modules."""

import ast
import hashlib
import importlib.metadata
import json
import platform
import subprocess
from pathlib import Path

from jsonschema import Draft202012Validator


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def verify(root):
    if not __debug__:
        raise ValueError('source verification requires assertions enabled')
    repo = root.parents[1]
    proposal = repo / 'docs/research/proposals/RFC-00D7-F01'
    pins = json.loads((proposal / 'source-pins.json').read_text())
    assert pins['candidate_revision'] == 'ec22a7588282d37ad027cff61c807bcf159ac2e0'
    for source in pins['sources']:
        obj = source['revision'] + ':' + source['path']
        raw = subprocess.check_output(['git', 'show', obj], cwd=repo)
        assert hashlib.sha256(raw).hexdigest() == source['sha256'], obj
        blob = subprocess.check_output(['git', 'rev-parse', obj], cwd=repo, text=True).strip()
        assert blob == source['git_blob'], obj
    for line in (proposal / 'SHA256SUMS').read_text().splitlines():
        expected, name = line.split('  ', 1)
        assert sha(proposal / name) == expected, name
    identity = json.loads((root / 'identities.json').read_text())
    assert sha(proposal / 'PROTOCOL.md') == identity['protocol_sha256']
    assert identity['protocol_commit'] == 'c8ae2200bc37064282ffc4aaa683ed21d94d567c'
    for name, expected in identity['files'].items():
        assert sha(root / name) == expected, name
    actual_names = {p.name for p in root.iterdir() if p.is_file()}
    assert actual_names == set(identity['files']) | {'identities.json', 'STATIC_VALIDATION.md'}
    schema = json.loads((root / 'pinned-d3.schema.json').read_text())
    source_schema = next(s for s in pins['sources'] if s['path'].endswith('00D3-model-manifest-schema.json'))
    assert sha(root / 'pinned-d3.schema.json') == source_schema['sha256']
    Draft202012Validator.check_schema(schema)  # Schema structure only, no F instances.
    Draft202012Validator.check_schema(json.loads((root / 'evidence.schema.json').read_text()))
    parsed = {}
    for path in sorted(root.glob('*.py')):
        text = path.read_text()
        tree = ast.parse(text, filename=str(path))
        compile(tree, str(path), 'exec')  # No exec(), import or bytecode execution.
        parsed[path.name] = tree
        assert '\t' not in text
        assert all(line == line.rstrip() for line in text.splitlines()), path
    for name in ('candidate.py', 'oracle.py', 'domains.py'):
        tree = parsed[name]
        for node in tree.body:
            assert isinstance(node, (ast.Import, ast.ImportFrom, ast.FunctionDef,
                                     ast.Expr)), (name, type(node).__name__)
            if isinstance(node, ast.Expr):
                assert isinstance(node.value, ast.Constant), name
        imported = set()
        for node in ast.walk(tree):
            if isinstance(node, ast.Import):
                imported.update(alias.name for alias in node.names)
            if isinstance(node, ast.ImportFrom):
                imported.add(node.module)
        assert not imported & {'candidate', 'oracle', 'domains', 'evaluate'}, name
    # Bind cross-module call signatures from ASTs without importing either side.
    definitions = {name[:-3]: {node.name: node for node in tree.body
                              if isinstance(node, ast.FunctionDef)}
                   for name, tree in parsed.items()}
    calls_checked = 0
    for tree in parsed.values():
        for call in ast.walk(tree):
            if not isinstance(call, ast.Call) or not isinstance(call.func, ast.Attribute):
                continue
            if not isinstance(call.func.value, ast.Name):
                continue
            module = call.func.value.id
            if module not in ('candidate', 'oracle', 'domains', 'evaluate'):
                continue
            definition = definitions[module][call.func.attr]
            arguments = definition.args.posonlyargs + definition.args.args
            names = [argument.arg for argument in arguments]
            assert len(call.args) <= len(names), (module, call.func.attr)
            supplied = set(names[:len(call.args)])
            for keyword in call.keywords:
                assert keyword.arg in names and keyword.arg not in supplied, (module, call.func.attr, keyword.arg)
                supplied.add(keyword.arg)
            required = names[:len(names) - len(definition.args.defaults)]
            assert set(required) <= supplied, (module, call.func.attr, required, supplied)
            calls_checked += 1
    lock = json.loads((root / 'dependency-lock.json').read_text())
    assert platform.python_version() == lock['python']
    for package in lock['packages']:
        installed = importlib.metadata.distribution(package['distribution'])
        assert installed.version == package['version']
        for entry in package['files']:
            assert sha(Path(installed.locate_file(entry['path']))) == entry['sha256'], entry['path']
    return {'preparation_only': True, 'source_pins': len(pins['sources']),
            'artifact_hashes': 4, 'compiled_modules': len(parsed),
            'schemas_checked': 2, 'cross_module_calls_checked': calls_checked, 'locked_distributions': len(lock['packages']),
            'model_imports': 0, 'experimental_states_enumerated': 0,
            'candidate_oracle_invocations': 0, 'execution_qualified': False}


if __name__ == '__main__':
    print(json.dumps(verify(Path(__file__).resolve().parent), sort_keys=True))
