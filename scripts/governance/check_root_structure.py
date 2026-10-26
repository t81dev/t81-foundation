#!/usr/bin/env python3
"""Preparation-only typed validation; no arguments preserves the required CI gate.

--structural is explicitly NOT policy authorization. --authorization needs an
operator-supplied approval file outside the candidate checkout and a trusted
copy of this evaluator. No auto-discovered candidate-local trust is accepted.
"""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path, PurePosixPath
import re
import subprocess
import sys

CONTRACT = 'docs/governance/root-layout-contract.json'
SCHEMA = 'docs/governance/root-layout-contract.schema.json'
CHECKER = 'scripts/governance/check_root_structure.py'
LEGACY = 'scripts/governance/check_root_structure_legacy.py'
OID = re.compile(r'[0-9a-f]{40}')
LITERAL = re.compile(r'(?P<comment>\#\[(?P<ceq>=*)\[.*?\](?P=ceq)\]|\#[^\n]*)|(?P<bracket>\[(?P<beq>=*)\[.*?\](?P=beq)\])|(?P<quoted>"(?:\\.|[^"\\])*")', re.S)
COMMAND = re.compile(r'\b([A-Za-z_][A-Za-z0-9_]*)\s*\(')


class Invalid(ValueError):
    pass


def unique_object(pairs):
    value = {}
    for key, item in pairs:
        if key in value:
            raise Invalid(f'duplicate JSON key: {key}')
        value[key] = item
    return value


def json_read(raw):
    return json.loads(raw, object_pairs_hook=unique_object)


def git(repo, *args):
    result = subprocess.run(['git', *args], cwd=repo, capture_output=True)
    if result.returncode:
        raise Invalid('Git operation failed: ' + result.stderr.decode('utf-8', errors='replace').strip())
    return result.stdout


def resolve(repo, revision):
    # Resolve once; later reads use this immutable commit/tree, never HEAD again.
    commit = git(repo, 'rev-parse', '--verify', '--end-of-options', revision + '^{commit}').decode().strip()
    if not OID.fullmatch(commit):
        raise Invalid('unsupported Git commit identity')
    return commit, git(repo, 'rev-parse', commit + '^{tree}').decode().strip()


def parse_tree(raw):
    if raw and not raw.endswith(b'\0'):
        raise Invalid('malformed Git tree: missing NUL terminator')
    entries = {}
    for record in raw.split(b'\0'):
        if not record:
            continue
        try:
            meta, name = record.split(b'\t', 1)
            mode, kind, oid = meta.decode('ascii').split(' ')
            path = name.decode('utf-8')
        except (ValueError, UnicodeError) as error:
            raise Invalid('malformed Git tree record') from error
        if (not OID.fullmatch(oid) or not path or path in entries
                or mode not in {'040000', '100644', '100755', '120000', '160000'}
                or kind != {'040000': 'tree', '160000': 'commit'}.get(mode, 'blob')):
            raise Invalid('malformed Git tree metadata')
        entries[path] = (mode, kind, oid)
    return entries


def blob(repo, tree, path):
    data = parse_tree(git(repo, 'ls-tree', '-z', tree, '--', path))
    if path not in data or data[path][0] not in {'100644', '100755'}:
        raise Invalid(f'missing regular committed input: {path}')
    return git(repo, 'cat-file', 'blob', data[path][2])


def schema_check(value, schema, path='$'):
    """Small strict interpreter for this schema's supported JSON Schema subset.

    No optional third-party dependency in the CI-facing script. Unsupported
    schema vocabulary fails rather than silently skipping constraints.
    """
    supported = {'$schema', 'title', 'type', 'additionalProperties', 'required',
                 'properties', 'enum', 'const', 'pattern', 'items', 'minItems'}
    if not isinstance(schema, dict) or set(schema) - supported:
        raise Invalid(f'unsupported schema vocabulary: {path}')
    if 'const' in schema and (type(value) is not type(schema['const']) or value != schema['const']):
        raise Invalid(f'invalid contract constant: {path}')
    if 'enum' in schema and value not in schema['enum']:
        raise Invalid(f'invalid contract enum: {path}')
    if 'pattern' in schema:
        if not isinstance(value, str) or re.search(schema['pattern'], value) is None:
            raise Invalid(f'invalid contract pattern: {path}')
    expected = schema.get('type')
    kinds = {'object': dict, 'array': list, 'string': str}
    if expected is not None and (expected not in kinds or type(value) is not kinds[expected]):
        raise Invalid(f'invalid contract type: {path}')
    if expected == 'object':
        properties = schema.get('properties', {})
        if schema.get('additionalProperties') is not False:
            raise Invalid(f'non-strict schema object: {path}')
        if set(value) - set(properties) or set(schema.get('required', [])) - set(value):
            raise Invalid(f'invalid contract fields: {path}')
        for key in sorted(value):
            schema_check(value[key], properties[key], path + '.' + key)
    elif expected == 'array':
        if len(value) < schema.get('minItems', 0):
            raise Invalid(f'empty required contract array: {path}')
        for index, item in enumerate(value):
            schema_check(item, schema['items'], f'{path}[{index}]')


def load_contract(repo, tree):
    contract = json_read(blob(repo, tree, CONTRACT))
    schema = json_read(blob(repo, tree, SCHEMA))
    if not isinstance(contract, dict) or type(contract.get('version')) is not int or contract['version'] != 1:
        raise Invalid('unsupported contract version')
    if not isinstance(schema, dict) or schema.get('$schema') != 'https://json-schema.org/draft/2020-12/schema':
        raise Invalid('unsupported schema revision')
    schema_check(contract, schema)
    # Semantic invariants also hold if a candidate weakens its schema. Approval
    # of schema bytes is independently checked by authorization, not inferred here.
    if contract.get('status') != 'preparation-only':
        raise Invalid('unsupported contract activation status')
    entries = contract.get('entries')
    if not isinstance(entries, list) or not entries:
        raise Invalid('missing contract entries')
    names = []
    for entry in entries:
        path = entry.get('path', '')
        if not re.fullmatch(r'(?!\.{1,2}$)[A-Za-z0-9_.-]+', path) or path == '.git':
            raise Invalid(f'invalid root contract path: {path}')
        names.append(path)
        expected_mode = {'directory': {'040000'}, 'file': {'100644', '100755'}}.get(entry.get('kind'), set())
        if entry.get('mode') not in expected_mode:
            raise Invalid(f'invalid contract kind/mode: {path}')
        fields = {'path', 'kind', 'mode'}
        if 'object' in entry:
            fields |= {'object', 'owner', 'status', 'review_trigger'}
            if (not isinstance(entry['object'], str) or not OID.fullmatch(entry['object'])
                    or entry.get('owner') != 't81dev' or entry.get('status') != 'preservation-only'
                    or entry.get('review_trigger') != 'before any proposed change to this entry or root contract'):
                raise Invalid(f'invalid preservation exception: {path}')
        if set(entry) != fields:
            raise Invalid(f'invalid entry fields: {path}')
    if len(names) != len(set(names)) or names != sorted(names):
        raise Invalid('duplicate or unsorted contract entries')
    denied = contract.get('prohibited_generated')
    if not isinstance(denied, list) or any(not isinstance(p, str) or not re.fullmatch(r'(?!\.{1,2}$)[A-Za-z0-9_.-]+', p) for p in denied) or len(denied) != len(set(denied)):
        raise Invalid('invalid generated-artifact list')
    if set(denied) & set(names):
        raise Invalid('generated-artifact prohibition overlaps permitted entry')
    return contract


def cmake_commands(text):
    """Literal inventory, including optional branches, never evaluate CMake."""
    code = LITERAL.sub(lambda m: re.sub(r'[^\n]', ' ', m[0]), text)
    position = definitions = 0
    while match := COMMAND.search(code, position):
        begin = position = match.end()
        depth = 1
        while position < len(code) and depth:
            depth += (code[position] == '(') - (code[position] == ')')
            position += 1
        if depth:
            raise Invalid('unclosed CMake command')
        command = match[1].lower()
        if command in {'function', 'macro'}:
            definitions += 1
            continue
        if command in {'endfunction', 'endmacro'}:
            definitions -= 1
            if definitions < 0:
                raise Invalid('unmatched CMake definition')
            continue
        if definitions:
            continue
        arguments = LITERAL.sub(lambda m: ' ' if m['comment'] is not None else m[0], text[begin:position - 1])
        tokens = []
        cursor = 0
        while cursor < len(arguments):
            if arguments[cursor].isspace():
                cursor += 1
                continue
            literal = LITERAL.match(arguments, cursor)
            if literal:
                token = literal[0]
                if literal['quoted'] is not None:
                    token = re.sub(r'\\(.)', r'\1', token[1:-1])
                elif literal['bracket'] is not None:
                    n = len(literal['beq']) + 2
                    token = token[n:-n].lstrip('\n')
                cursor = literal.end()
            else:
                token = re.match(r'\S+', arguments[cursor:])[0]
                cursor += len(token)
            tokens.append(token)
        yield command, tokens
    if definitions:
        raise Invalid('unclosed CMake definition')


def cmake_dependencies(repo, tree, contract):
    required = {e['path'] for e in contract['entries'] if e['kind'] == 'directory' and 'object' not in e}
    pending, visited, errors = ['CMakeLists.txt'], set(), set()
    while pending:
        path = pending.pop()
        if path in visited:
            continue
        visited.add(path)
        commands = cmake_commands(blob(repo, tree, path).decode('utf-8'))
        for command, args in commands:
            references = []
            if command == 'add_subdirectory' and args:
                first = args[0]
                if '$' in first:
                    raise Invalid(f'unsupported computed CMake subdirectory: {path}: {first}')
                references.append((str(PurePosixPath(path).parent / first), True))
            if command in {'add_library', 'add_executable', 'target_sources'}:
                for arg in args[1:]:
                    if arg.startswith('${PROJECT_SOURCE_DIR}/'):
                        references.append((arg[len('${PROJECT_SOURCE_DIR}/'):], False))
                    elif '$' not in arg and '/' in arg and re.search(r'\.(cpp|cc|c|hpp|h)$', arg):
                        references.append((str(PurePosixPath(path).parent / arg), False))
            for reference, directory in references:
                normalized = []
                if reference.startswith('/') or '\\' in reference:
                    raise Invalid(f'CMake path escapes repository: {reference}')
                for part in PurePosixPath(reference).parts:
                    if part == '..':
                        if not normalized:
                            raise Invalid(f'CMake path escapes repository: {reference}')
                        normalized.pop()
                    elif part != '.':
                        normalized.append(part)
                if not normalized:
                    raise Invalid('empty CMake structural dependency')
                if normalized[0] not in required:
                    errors.add(f'CMake input absent from required contract: {normalized[0]}')
                if directory:
                    pending.append('/'.join(normalized) + '/CMakeLists.txt')
    return sorted(errors)


def structural(repo, tree):
    contract = load_contract(repo, tree)
    actual = parse_tree(git(repo, 'ls-tree', '-z', tree))
    expected = {e['path']: e for e in contract['entries']}
    errors = []
    for path in sorted(set(actual) | set(expected)):
        if path not in expected:
            label = 'prohibited generated artifact' if path in contract['prohibited_generated'] else 'unknown root entry'
            errors.append(f'{label}: {path}')
        elif path not in actual:
            label = 'missing preservation entry' if 'object' in expected[path] else 'missing required ' + expected[path]['kind']
            errors.append(f'{label}: {path}')
        else:
            mode, kind, oid = actual[path]
            if mode in {'120000', '160000'}:
                errors.append(f'{"symlink" if mode == "120000" else "gitlink"} forbidden: {path}')
            elif mode != expected[path]['mode']:
                errors.append(f'entry type/mode mismatch: {path}: expected {expected[path]["mode"]}, found {mode}')
            elif 'object' in expected[path] and oid != expected[path]['object']:
                errors.append(f'preservation identity mismatch: {path}')
    # Avoid cascading source-read failures after an already diagnosed missing tree.
    if not errors:
        errors.extend(cmake_dependencies(repo, tree, contract))
    return sorted(errors), sum('object' in e for e in contract['entries'])


def authorization(repo, commit, tree, approval_file):
    """Prototype trust boundary: file is supplied by a trusted external operator.

    Location outside checkout prevents accidental candidate-local discovery; it
    does not authenticate a file or a human. The caller must authenticate and
    protect the manifest, this evaluator, invocation and status publication.
    """
    if not approval_file:
        raise Invalid('absent trusted approval: external operator input required')
    source = Path(approval_file).resolve(strict=True)
    if source.is_relative_to(repo.resolve()):
        raise Invalid('candidate-local approval forbidden')
    data = json_read(source.read_bytes())
    if (not isinstance(data, dict) or set(data) != {'version', 'purpose', 'approved_commit', 'approver', 'artifacts'}
            or type(data['version']) is not int or data['version'] != 1
            or data['purpose'] != 'scope-a-preparation-review' or data['approver'] != 't81dev'
            or data['approved_commit'] != commit or not isinstance(data['artifacts'], dict)):
        raise Invalid('invalid trusted approval or unapproved commit')
    tracked = parse_tree(git(repo, 'ls-tree', '-r', '-z', tree))
    protected = {CONTRACT, SCHEMA, CHECKER, LEGACY}
    protected |= {p for p in tracked if p.startswith(('.github/workflows/', '.github/actions/'))}
    if set(data['artifacts']) != protected:
        raise Invalid('trusted approval artifact coverage mismatch')
    for path in sorted(protected):
        expected = data['artifacts'][path]
        if not isinstance(expected, str) or not re.fullmatch(r'[0-9a-f]{64}', expected):
            raise Invalid(f'invalid trusted artifact digest: {path}')
        if hashlib.sha256(blob(repo, tree, path)).hexdigest() != expected:
            raise Invalid(f'unapproved artifact revision: {path}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    modes = parser.add_mutually_exclusive_group()
    modes.add_argument('--structural', action='store_true')
    modes.add_argument('--authorization', action='store_true')
    parser.add_argument('--repo-root', type=Path, default=Path(__file__).resolve().parents[2])
    parser.add_argument('--revision', default='HEAD')
    parser.add_argument('--trusted-approval', type=Path)
    args = parser.parse_args()
    if not args.structural and not args.authorization:
        if args.revision != 'HEAD' or args.trusted_approval or args.repo_root.resolve() != Path(__file__).resolve().parents[2]:
            parser.error('explicit preparation mode required for additional arguments')
        # Preserve the pre-activation required command exactly. No workflow edits.
        import check_root_structure_legacy
        return check_root_structure_legacy.main()
    try:
        commit, tree = resolve(args.repo_root, args.revision)
        if args.authorization:
            authorization(args.repo_root, commit, tree, args.trusted_approval)
            print(f'authorization prototype PASSED: {commit}; operator-authenticated external input assumed; NOT ACTIVATION')
            return 0
        errors, preserved = structural(args.repo_root, tree)
        if errors:
            print('committed root structure FAILED (preparation only)')
            for error in errors:
                print('- ' + error)
            return 1
        print(f'committed root structure PASSED: {commit}; preservation-only exceptions: {preserved}; NOT POLICY AUTHORIZATION')
        return 0
    except (Invalid, OSError, ValueError, TypeError, KeyError, UnicodeError, RecursionError) as error:
        print(f'root validation FAILED: {error}')
        return 1


if __name__ == '__main__':
    sys.exit(main())
