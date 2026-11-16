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
    assert identity['protocol_commit'] == 'c5e0ca79c073a72bb41f03b76ddd68acbd7eab73'
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
    declarations = json.loads((root / 'interpretations.json').read_text())['choices']
    policy = parsed['interpretation_policy.py']
    supported = next(node for node in policy.body if isinstance(node, ast.Assign)
                     and any(isinstance(target, ast.Name) and target.id == 'SUPPORTED_KEYS'
                             for target in node.targets))
    keys = ast.literal_eval(supported.value)
    assert len(keys) == len(set(keys)) and set(keys) == set(declarations)
    definition = json.loads((root / 'evidence.schema.json').read_text())
    configuration = definition['$defs']['authorization']['properties']['interpretations']
    assert configuration['additionalProperties'] is False
    assert set(configuration['required']) == set(keys) == set(configuration['properties'])
    for key in keys:
        assert configuration['properties'][key]['enum'] == [None] + declarations[key]['alternatives']
    # Literal metadata lookups must use declared keys. No configuration .get()
    # or alias may silently supply a favorable default.
    for tree in parsed.values():
        for node in ast.walk(tree):
            if isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute):
                assert not (isinstance(node.func.value, ast.Name)
                            and node.func.value.id == 'interpretations'
                            and node.func.attr == 'get')
            if isinstance(node, ast.Subscript) and isinstance(node.value, ast.Name):
                if node.value.id == 'values' and isinstance(node.slice, ast.Constant):
                    assert node.slice.value in keys
    functions = {node.name: node for node in policy.body if isinstance(node, ast.FunctionDef)}
    assert "set(values) != set(SUPPORTED_KEYS)" in ast.unparse(functions['validate'])
    assert isinstance(functions['validate'].body[0], ast.If)
    assert isinstance(functions['validate'].body[0].body[0], ast.Raise)
    assert 'value is not None and value not in alternatives' in ast.unparse(functions['validate'])
    assert "key != 'outside-arena' and value is not None" in ast.unparse(functions['validate'])
    assert "values['outside-arena']" in ast.unparse(functions['outside_arena'])
    comparison = next(node for node in parsed['evaluate.py'].body
                      if isinstance(node, ast.FunctionDef) and node.name == 'compare')
    assert ast.unparse(comparison.body[0]) == 'validate(interpretations, declarations)'
    # Candidate memoization is valid only while these are ALL semantic inputs.
    publication = next(node for node in parsed['candidate.py'].body
                       if isinstance(node, ast.FunctionDef) and node.name == 'publication')
    consumed_keys = {node.slice.value for node in ast.walk(publication)
                     if isinstance(node, ast.Subscript) and isinstance(node.value, ast.Name)
                     and node.value.id == 'case' and isinstance(node.slice, ast.Constant)}
    assert consumed_keys == {'order', 'initial_revoked', 'applicable'}
    memo_key = next(node for node in ast.walk(comparison) if isinstance(node, ast.Assign)
                    and any(isinstance(target, ast.Name) and target.id == 'key'
                            for target in node.targets))
    assert {node.slice.value for node in memo_key.value.elts} == consumed_keys
    assert not any(isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute)
                   and node.func.attr == 'permutations'
                   for node in ast.walk(parsed['domains.py']))
    old_domain = ast.parse(subprocess.check_output([
        'git', 'show', '71997805e156e64f3cb4d3b58bd4493ca8b71a32:experiments/rfc_00d7_f01/domains.py'
    ], cwd=repo, text=True))
    def f_recipe(tree):
        function = next(node for node in tree.body
                        if isinstance(node, ast.FunctionDef) and node.name == 'cases')
        branch = next(node for node in ast.walk(function)
                      if isinstance(node, ast.If) and ast.unparse(node.test) == "area == 'F'")
        return [ast.dump(node) for node in branch.body]
    assert f_recipe(old_domain) == f_recipe(parsed['domains.py'])
    order_generator = next(node for node in parsed['domains.py'].body
                           if isinstance(node, ast.FunctionDef) and node.name == 'publication_orders')
    chain_assignments = {node.targets[0].id: ast.literal_eval(node.value)
                         for node in order_generator.body if isinstance(node, ast.Assign)}
    assert chain_assignments == {'revoke': ('invoke', 'commit', 'response'),
                                 'fence': ('acquire', 'check', 'publish', 'release')}
    # Closed-form arithmetic only: no domains, orders, cases or controls run.
    accounting = json.loads((root / 'CAP_ACCOUNTING.json').read_text())
    bounds = {
        'A': 3088 + 4 * 114 + 24 + 84 + 252 + 17422,
        'B': 88 + 125 + 560 + 2 * 560 + 140 + 7 * 140 + 560 + 210,
        'C': 80 + 1555 + 5910 + 6 + 3 * 341 + 2 * 1252 + 6 * 12,
        'D': 8168 + 8748 * 14 + 576 + 512 * 29 + 976 + 96 * 6,
        'E': 72 + 6156 * 6 + 1026 * 21 + 144 * 12 + 9 * 5,
        'F': 32 + 288 * 6,
    }
    assert 9 * (8 + 2 + 11 * 2) == accounting['areas']['F']['inputs']
    assert 35 * 16 == accounting['areas']['B']['inputs']
    assert sum(row['cap'] for row in accounting['areas'].values()) == 49408
    for area, bound in bounds.items():
        row = accounting['areas'][area]
        assert row['full_states_upper'] == bound
        assert row['artifact_bytes_upper'] == (row['ledger_rows_upper'] * 128
                                               + row['rotated_controls'] * 4096 + 512 * 1024)
        assert row['artifact_bytes_upper'] <= 4 * 1024**2
    assert bounds['B'] <= accounting['areas']['B']['cap']
    assert accounting['areas']['F']['inputs'] > accounting['areas']['F']['cap']
    assert sum(row['artifact_bytes_upper'] for row in accounting['areas'].values()) + 131072 < 25 * 1024**2
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
    # FA01 literal metadata/AST checks only; never validate fixtures against D3.
    plan = json.loads((root / 'fa01-plan.json').read_text())
    packet = json.loads((root / 'fa01-corpus.json').read_text())
    assert sha(root / 'fa01-corpus.json') == plan['corpus_sha256']
    assert sha(root / 'pinned-d7.rfc.md') == plan['rfc_sha256']
    assert sha(proposal / 'PROTOCOL.md') == plan['protocol_sha256']
    assert plan['protocol_sha256'] == 'd48aeedc924cf505cab8a7fed316fc84d8f2c259dd502fd1d0a7fe8432f05de3'
    assert sha(repo / 'docs/research/proposals/RFC-00D7-F01-FA01/AMENDMENT.md') == plan['amendment_sha256']
    assert [item['id'] for item in packet['fixtures']] == plan['fixture_ids']
    assert len(packet['fixtures']) == 8 and len(plan['proof_facts']) == 24
    assert plan['state_bounds'] == {'controls': 32, 'proof': 78, 'boundary': 144, 'total': 254}
    assert 2 * 4 * 4 + (1 + 2 + 1 + 24 * 3 + 2) + 6 * 4 * 6 == 254 <= 256
    fa_tree = parsed['fa01.py']
    loader = next(n for n in fa_tree.body if isinstance(n, ast.FunctionDef) and n.name == 'load_plan')
    model_fields = {n.value for n in ast.walk(loader) if isinstance(n, ast.Constant) and n.value in ('expected_schema_valid', 'expected_root_model_equality')}
    for call in ast.walk(fa_tree):
        if isinstance(call, ast.Call) and isinstance(call.func, ast.Attribute) and call.func.attr == 'proof_fact':
            assert isinstance(call.func.value, ast.Name)
            expected_state = 'node_candidate' if call.func.value.id == 'candidate' else 'node_reference'
            assert isinstance(call.args[-1], ast.Name) and call.args[-1].id == expected_state
    assert not model_fields  # Expectation labels never become model inputs.
    assert not any(isinstance(n, ast.Call) and isinstance(n.func, ast.Attribute) and isinstance(n.func.value, ast.Name) and n.func.value.id == 'domains' for n in ast.walk(fa_tree))
    evidence_v2 = json.loads((root / 'evidence.schema.json').read_text())
    Draft202012Validator.check_schema(json.loads((root / 'evidence-v1.schema.json').read_text()))
    assert evidence_v2['$id'].endswith(':v2')
    prior_evidence = subprocess.check_output(['git', 'show', '43b49541ddea76ba504031765229e455ef80c4ea:experiments/rfc_00d7_f01/evidence.schema.json'], cwd=repo)
    assert prior_evidence == (root / 'evidence-v1.schema.json').read_bytes()
    f_schema = evidence_v2['$defs']['summary']['properties']['areas']['properties']['F']
    assert f_schema['additionalProperties'] is False
    assert set(f_schema['properties']) == set(f_schema['required'])
    f_driver = next(n for n in fa_tree.body if isinstance(n, ast.FunctionDef) and n.name == 'run')
    returned = [n for n in ast.walk(f_driver) if isinstance(n, ast.Return) and isinstance(n.value, ast.Dict)][-1]
    assert {n.value for n in returned.value.keys} == set(f_schema['required'])
    for record in ('authorization', 'qualification'):
        assert {'area_f_plan_id', 'corpus_sha256', 'protocol_sha256', 'amendment_sha256'} <= set(evidence_v2['$defs'][record]['required'])
    return {'preparation_only': True, 'source_pins': len(pins['sources']),
            'artifact_hashes': len((proposal / 'SHA256SUMS').read_text().splitlines()), 'compiled_modules': len(parsed),
            'schemas_checked': 3, 'fa01_planned_states': 254, 'cross_module_calls_checked': calls_checked, 'locked_distributions': len(lock['packages']),
            'declared_interpretations_checked': len(keys),
            'closed_form_cap_bounds_checked': len(bounds),
            'model_imports': 0, 'experimental_states_enumerated': 0,
            'candidate_oracle_invocations': 0, 'execution_qualified': False}


if __name__ == '__main__':
    print(json.dumps(verify(Path(__file__).resolve().parent), sort_keys=True))
