#!/usr/bin/env python3
"""Offline, read-only Research Commons v1. No experiments or network operations."""
import sys

# Prevent local imports from creating repository bytecode caches.
sys.dont_write_bytecode = True

import argparse
import copy
import hashlib
import json
import os
from pathlib import Path, PurePosixPath
import re
import subprocess

from jsonschema import Draft202012Validator, ValidationError

PROTOCOL = 't81.research-commons.v1'
ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / 'docs/research/commons'
CANONICAL_REGISTER_SHA256 = 'af8ca65dcef136f294eb33010b5a6621875a52db4f3e60b94a4e28db5318a1c1'
CANONICAL_CHARTER_SHA256 = 'bb073bafcd00a81d6790558d05ab60b2e53eee9a8724e19563228983bc575ca7'
HYPOTHESES = tuple('H%02d' % n for n in range(1, 9))


class CommonsError(ValueError):
    """Invalid request, unavailable evidence, or an inconsistent index."""


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(',', ':'), ensure_ascii=True)


def load_json(raw):
    def unique_pairs(pairs):
        result = {}
        for key, value in pairs:
            if key in result:
                raise CommonsError('duplicate JSON key: ' + key)
            result[key] = value
        return result

    def invalid_number(value):
        raise CommonsError('non-finite JSON number: ' + value)

    return json.loads(raw, object_pairs_hook=unique_pairs, parse_constant=invalid_number)


def validate_schema(value, filename):
    definition = load_json((DATA / filename).read_bytes())
    Draft202012Validator.check_schema(definition)
    Draft202012Validator(definition).validate(value)


class Commons:
    """Loads a curated snapshot; only allowlisted Git reads reach subprocess."""

    def __init__(self, root=ROOT, manifest=None):
        self.root = Path(root).resolve()
        raw = (self.root / 'docs/research/commons/manifest.v1.json').read_bytes()
        self.manifest = copy.deepcopy(manifest) if manifest is not None else load_json(raw)
        validate_schema(self.manifest, 'manifest.schema.json')
        self.digest = hashlib.sha256(canonical(self.manifest).encode('ascii')).hexdigest()
        self.sources = self._index(self.manifest['sources'])
        self.hypotheses = self._index(self.manifest['hypotheses'])
        self._index(self.manifest['reproductions'])
        self._index(self.manifest['open_questions'])
        self._index(self.manifest['discrepancy_history'])
        tracks = self._index(self.manifest['tracks'])
        if set(tracks) != {'R', 'F', 'M'} or set(self.hypotheses) != set(HYPOTHESES):
            raise CommonsError('canonical track/hypothesis identities required')
        version = re.search(rb'git version (\d+)\.(\d+)', self._git('--version'))
        if version is None or tuple(map(int, version.groups())) < (2, 46):
            raise CommonsError('Git 2.46+ required for offline lazy-fetch suppression')
        self.content = {}
        revisions = {self.manifest['baseline_revision']} | {s['revision'] for s in self.sources.values()}
        for revision in sorted(revisions):
            if self._git('cat-file', '-t', revision).strip() != b'commit':
                raise CommonsError('source revision must identify a commit')
        for source in self.sources.values():
            self.content[source['id']] = self._read_source(source)
        for id, expected in [('register', CANONICAL_REGISTER_SHA256),
                             ('charter', CANONICAL_CHARTER_SHA256)]:
            if self.sources.get(id, {}).get('sha256') != expected:
                raise CommonsError('canonical authority identity changed: ' + id)
        self.register = load_json(self.content['register'])
        self.entries = self._index(self.register['entries'])
        if tuple(self.entries) != HYPOTHESES:
            raise CommonsError('historical register identities/order changed')
        for hypothesis in self.hypotheses.values():
            index = int(hypothesis['pointer'].rsplit('/', 1)[1])
            if (hypothesis['source_ref'] != 'register'
                    or self.register['entries'][index]['id'] != hypothesis['id']):
                raise CommonsError('hypothesis pointer reassigns canonical identity')
            self._refs(hypothesis['evidence_refs'])
        groups = (self.manifest['tracks'] + self.manifest['reproductions']
                  + self.manifest['open_questions'] + self.manifest['discrepancy_history']
                  + [self.manifest['governance'], self.manifest['integration']])
        for record in groups:
            self._refs(record['source_refs'])
            self._refs(record.get('superseded_by_refs', []))
            for id in record.get('hypothesis_ids', []):
                if id not in self.hypotheses:
                    raise CommonsError('unknown hypothesis: ' + id)
        integration = self.manifest['integration']
        if integration['main_revision'] != self.manifest['baseline_revision']:
            raise CommonsError('integration main revision differs from baseline')
        for ref in ('initiative-610-proposed', 'reconciliation-610-proposed'):
            source = self.sources.get(ref, {})
            if (source.get('role') != 'proposal'
                    or source.get('revision') != integration['pending_revision']):
                raise CommonsError('unmerged correction must remain proposed evidence')
        assigned = [id for track in tracks.values() for id in track['hypothesis_ids']]
        if sorted(assigned) != list(HYPOTHESES):
            raise CommonsError('each canonical hypothesis must have one track')
        # No derived validated-finding field exists: dispositions come only from
        # the historical authority, and evidence retains source-specific limits.

    @staticmethod
    def _index(records):
        result = {}
        for record in records:
            if record['id'] in result:
                raise CommonsError('duplicate ID: ' + record['id'])
            result[record['id']] = record
        return result

    def _refs(self, refs):
        for ref in refs:
            if ref not in self.sources:
                raise CommonsError('unknown evidence reference: ' + ref)

    def _git(self, *arguments):
        environment = dict(os.environ, GIT_NO_LAZY_FETCH='1',
                           GIT_TERMINAL_PROMPT='0', GIT_OPTIONAL_LOCKS='0')
        try:
            return subprocess.check_output(
                ['git', '--no-replace-objects', '-C', str(self.root), *arguments],
                stderr=subprocess.PIPE, env=environment)
        except (subprocess.CalledProcessError, OSError) as error:
            raise CommonsError('pinned Git evidence unavailable; explicit checkout/fetch '
                               'setup required, never performed by Commons') from error

    def _read_source(self, source):
        revision, path = source['revision'], source['path']
        parts = PurePosixPath(path).parts
        if (not re.fullmatch('[0-9a-f]{40}', revision) or not parts
                or path.startswith('/') or '..' in parts or '\\' in path
                or str(PurePosixPath(path)) != path):
            raise CommonsError('unsafe source identity')
        expected_url = self.manifest['repository'] + '/blob/' + revision + '/' + path
        if source['url'] != expected_url:
            raise CommonsError('source URL does not match identity')
        # Exact-path match also rejects directories, symlinks and submodules.
        tree = self._git('ls-tree', '-z', revision, '--', path).split(b'\0')
        if len(tree) != 2 or not tree[0]:
            raise CommonsError('source must be a single file: ' + path)
        metadata, actual_path = tree[0].split(b'\t', 1)
        mode, kind, blob = metadata.split()
        if mode not in (b'100644', b'100755') or kind != b'blob' or actual_path.decode() != path:
            raise CommonsError('source must be a regular file: ' + path)
        raw = self._git('cat-file', 'blob', blob.decode('ascii'))
        if hashlib.sha256(raw).hexdigest() != source['sha256']:
            raise CommonsError('evidence SHA-256 mismatch: ' + source['id'])
        return raw.decode('utf-8')

    def evidence(self, id):
        if id not in self.sources:
            raise CommonsError('unknown evidence ID: ' + id)
        return {**self.sources[id], 'content': self.content[id]}

    def hypothesis(self, id):
        if id not in self.hypotheses:
            raise CommonsError('unknown hypothesis: ' + id)
        return {'id': id, 'authority': self.sources['register'],
                'historical_disposition_as_of': self.register['as_of'],
                'canonical_record': self.entries[id],
                'integration': self.manifest['integration'],
                'discrepancy_history': [q for q in self.manifest['discrepancy_history']
                                        if id in q['hypothesis_ids']],
                'evidence': [self.sources[ref] for ref in self.hypotheses[id]['evidence_refs']],
                'open_questions': [q for q in self.manifest['open_questions']
                                   if id in q['hypothesis_ids']]}

    def dispatch(self, request):
        validate_schema(request, 'request.schema.json')
        operation, args = request['operation'], request['arguments']
        if operation == 'list_research_tracks':
            result = self.manifest['tracks']
        elif operation == 'list_hypotheses':
            result = [self.hypothesis(id) for id in HYPOTHESES]
        elif operation == 'get_hypothesis':
            result = self.hypothesis(args['hypothesis_id'])
        elif operation == 'get_evidence':
            result = self.evidence(args['evidence_id'])
        elif operation == 'get_reproduction_instructions':
            id = args['hypothesis_id']
            result = {'hypothesis_id': id,
                      'instructions': [r for r in self.manifest['reproductions']
                                       if id in r['hypothesis_ids']],
                      'limitation': 'Curated instructions only; an empty list means no '
                                    'indexed reproduction recipe, not evidence of absence.',
                      'execution': 'unavailable'}
        else:  # request schema allows only list_open_research_questions here
            result = self.manifest['open_questions']
        return copy.deepcopy({'protocol': PROTOCOL, 'operation': operation,
                              'baseline_revision': self.manifest['baseline_revision'],
                              'manifest_sha256': self.digest,
                              'scope': self.manifest['scope'], 'result': result})

    def validate_contribution(self, value):
        prefix = 't81.research-commons.'
        kinds = ('experiment-proposal', 'independent-review',
                 'reproducibility-report', 'evidence-discrepancy')
        kind = next((k for k in kinds if value.get('schema') == prefix + k + '.v1'), None)
        if kind is None:
            raise CommonsError('unknown contribution schema')
        validate_schema(value, kind + '.schema.json')
        self._refs([e['source_ref'] for e in value['evidence']])
        if self._git('cat-file', '-t', value['repository_revision']).strip() != b'commit':
            raise CommonsError('contribution revision must identify a commit')
        if kind == 'evidence-discrepancy':
            self._refs(value['affected_sources'])
        auth = value['human_authorization']
        if auth['status'] == 'authorized' and (not auth['human'] or not auth['record']):
            raise CommonsError('authorization requires named human and evidence record')
        return {'status': 'valid_proposal_shape', 'execution': 'unavailable',
                'limitation': 'Shape/reference checks establish neither authorization '
                              'authenticity nor scientific acceptance.'}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('--request', help='one JSON request, or - to read stdin')
    group.add_argument('--validate', action='store_true', help='validate pinned index')
    group.add_argument('--validate-contribution', type=Path)
    args = parser.parse_args()
    try:
        commons = Commons()
        if args.validate:
            result = {'protocol': PROTOCOL, 'status': 'valid_index',
                      'manifest_sha256': commons.digest, 'sources': len(commons.sources)}
        elif args.validate_contribution:
            result = commons.validate_contribution(load_json(args.validate_contribution.read_bytes()))
        else:
            raw = sys.stdin.read() if args.request == '-' else args.request
            result = commons.dispatch(load_json(raw))
        print(canonical(result))
        return 0
    except (CommonsError, ValidationError, OSError, ValueError, KeyError, TypeError) as error:
        # Do not echo source content, arbitrary requests or host-specific Git errors.
        message = error.message if isinstance(error, ValidationError) else str(error)
        print(canonical({'protocol': PROTOCOL, 'error': 'validation_failed',
                         'message': message}), file=sys.stderr)
        return 2


if __name__ == '__main__':
    raise SystemExit(main())
