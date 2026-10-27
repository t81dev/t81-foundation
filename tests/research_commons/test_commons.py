"""Contract checks for discovery infrastructure, not scientific replication."""
import sys
sys.dont_write_bytecode = True

import copy
import hashlib
import importlib.util
import json
from pathlib import Path
import socket
import subprocess
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('commons', ROOT / 'tools/research_commons/commons.py')
commons = importlib.util.module_from_spec(spec)
spec.loader.exec_module(commons)


def request(operation, **arguments):
    return {'protocol': commons.PROTOCOL, 'operation': operation, 'arguments': arguments}


class DiscoveryContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.index = commons.Commons()

    def modified(self, change):
        manifest = copy.deepcopy(self.index.manifest)
        change(manifest)
        with self.assertRaises((commons.CommonsError, commons.ValidationError)):
            commons.Commons(manifest=manifest)

    def test_all_paths_and_immutable_hashes_resolve(self):
        for source in self.index.sources.values():
            with self.subTest(id=source['id']):
                raw = self.index.content[source['id']].encode('utf-8')
                self.assertEqual(hashlib.sha256(raw).hexdigest(), source['sha256'])
                self.assertIn(source['revision'] + '/' + source['path'], source['url'])

    def test_historical_hypotheses_are_verbatim_and_separate_from_hs1(self):
        original = json.loads(self.index.content['register'])
        result = self.index.dispatch(request('list_hypotheses'))['result']
        self.assertEqual([r['id'] for r in result], list(commons.HYPOTHESES))
        self.assertEqual([r['canonical_record'] for r in result], original['entries'])
        self.assertIn('AI-assisted', result[7]['canonical_record']['formal_claim'])
        self.assertNotIn('H-S1', self.index.hypotheses)

    def test_pinned_evidence_ignores_working_tree_substitution(self):
        git_dir = subprocess.check_output(
            ['git', '-C', str(ROOT), 'rev-parse', '--absolute-git-dir'], text=True).strip()
        with tempfile.TemporaryDirectory() as temporary:
            alternate = Path(temporary)
            (alternate / '.git').write_text('gitdir: ' + git_dir + '\n')
            target = alternate / 'docs/research/commons'
            target.mkdir(parents=True)
            (target / 'manifest.v1.json').write_bytes(
                (commons.DATA / 'manifest.v1.json').read_bytes())
            evidence = alternate / self.index.sources['hanoi-negative']['path']
            evidence.parent.mkdir(parents=True)
            evidence.write_text('FAKE WORKING TREE SUCCESS')
            pinned = commons.Commons(root=alternate)
            self.assertEqual(pinned.evidence('hanoi-negative'),
                             self.index.evidence('hanoi-negative'))

    def test_negative_results_remain_discoverable(self):
        h02 = self.index.dispatch(request('get_hypothesis', hypothesis_id='H02'))['result']
        self.assertIn('hanoi-negative', [e['id'] for e in h02['evidence']])
        raw = json.loads(self.index.evidence('hanoi-negative')['content'])
        self.assertFalse(raw['candidate_passes'])
        self.assertEqual(raw['addition_failures'], 422)
        e1 = self.index.evidence('e1-readme')['content']
        self.assertIn('NOT SUPPORTED', e1)
        self.assertIn('1,240 bytes', e1)
        self.assertIn('157 bytes', e1)
        self.assertEqual(self.index.sources['e1-readme']['revision'],
                         '827d2accbf8ccb56afe3fe8d96dfb7251f249d7d')

    def test_unverified_claims_not_promoted(self):
        self.assertEqual(self.index.evidence('initiative')['role'], 'disputed')
        self.assertEqual(self.index.evidence('audit-matrix')['role'], 'disputed')
        h07 = self.index.hypothesis('H07')
        self.assertEqual(h07['canonical_record'], self.index.entries['H07'])
        self.assertIn('D-ASSURANCE', [q['id'] for q in h07['open_questions']])
        self.modified(lambda m: m['hypotheses'][6].update(disposition='validated'))
        self.modified(lambda m: m['sources'][0].update(role='comparatively_validated'))

    def test_missing_evidence_reference_rejected(self):
        self.modified(lambda m: m['hypotheses'][0]['evidence_refs'].append('missing'))
        self.modified(lambda m: m['sources'].pop())
        value = json.loads((commons.DATA / 'examples/h04-proposal.json').read_text())
        value['evidence'][0]['source_ref'] = 'missing'
        with self.assertRaises(commons.CommonsError):
            self.index.validate_contribution(value)

    def test_missing_file_rejected(self):
        def change(m):
            s = m['sources'][0]
            s['path'] = 'docs/research/absent.md'
            s['url'] = m['repository'] + '/blob/' + s['revision'] + '/' + s['path']
        self.modified(change)

    def test_hash_commit_url_and_path_tampering_rejected(self):
        self.modified(lambda m: m['sources'][0].update(sha256='0' * 64))
        self.modified(lambda m: m['sources'][0].update(revision='0' * 40))
        self.modified(lambda m: m['sources'][0].update(url='https://example.invalid'))
        for path in ['../AGENTS.md', '/etc/passwd', 'docs/../AGENTS.md', 'docs\\AGENTS.md']:
            self.modified(lambda m, path=path: m['sources'][0].update(path=path))

    def test_canonical_reassignment_and_duplicates_rejected(self):
        self.modified(lambda m: m['hypotheses'][0].update(pointer='/entries/7'))
        self.modified(lambda m: m['hypotheses'][0].update(id='H08'))
        self.modified(lambda m: m['sources'].append(m['sources'][0]))
        self.modified(lambda m: m['tracks'][0]['hypothesis_ids'].remove('H01'))

    def test_protocol_strict_and_execution_unavailable(self):
        for value in [request('execute_experiment'), request('fetch'),
                      request('get_evidence', evidence_id='../../etc/passwd'),
                      request('list_hypotheses', output='AGENTS.md'),
                      {**request('list_hypotheses'), 'protocol': 'v2'}]:
            with self.assertRaises((commons.CommonsError, commons.ValidationError)):
                self.index.dispatch(value)
        with self.assertRaises(commons.CommonsError):
            commons.load_json('{"operation":1,"operation":2}')
        with self.assertRaises(commons.CommonsError):
            commons.load_json('{"value":NaN}')

    def all_requests(self):
        return [request('list_research_tracks'), request('list_hypotheses'),
                request('get_hypothesis', hypothesis_id='H08'),
                request('get_evidence', evidence_id='hanoi-negative'),
                request('get_reproduction_instructions', hypothesis_id='H02'),
                request('list_open_research_questions')]

    def test_all_six_operations_deterministic(self):
        another = commons.Commons()
        for value in self.all_requests():
            first = commons.canonical(self.index.dispatch(value))
            self.assertEqual(first, commons.canonical(another.dispatch(value)))
            self.assertNotIn(str(ROOT), first)
            commons.validate_schema(json.loads(first), 'response.schema.json')
        returned = self.index.dispatch(request('list_research_tracks'))
        returned['result'].clear()
        self.assertEqual(len(self.index.manifest['tracks']), 3)

    def test_all_operations_preserve_files_and_git_state_offline(self):
        def snapshot():
            files = {p.relative_to(ROOT).as_posix(): hashlib.sha256(p.read_bytes()).hexdigest()
                     for p in ROOT.rglob('*') if p.is_file()}
            state = [subprocess.check_output(['git', '--no-optional-locks', '-C', str(ROOT), *args])
                     for args in [('rev-parse', 'HEAD'), ('show-ref',),
                                  ('status', '--porcelain', '--untracked-files=all')]]
            index_path = subprocess.check_output(
                ['git', '-C', str(ROOT), 'rev-parse', '--git-path', 'index'], text=True).strip()
            index_hash = hashlib.sha256((ROOT / index_path).read_bytes()).hexdigest()
            return files, state, index_hash
        before = snapshot()
        actual = subprocess.check_output
        calls = []
        def guarded(command, **kwargs):
            calls.append(command)
            self.assertEqual(command[:4], ['git', '--no-replace-objects', '-C', str(ROOT)])
            self.assertIn(command[4], ('ls-tree', 'cat-file', '--version'))
            self.assertEqual(kwargs['env']['GIT_NO_LAZY_FETCH'], '1')
            self.assertEqual(kwargs['env']['GIT_OPTIONAL_LOCKS'], '0')
            return actual(command, **kwargs)
        with patch.object(socket, 'socket', side_effect=AssertionError('network forbidden')):
            with patch.object(subprocess, 'check_output', side_effect=guarded):
                fresh = commons.Commons()
                for value in self.all_requests():
                    fresh.dispatch(value)
        self.assertTrue(calls)
        self.assertEqual(before, snapshot())

    def test_schema_definitions_and_example_contributions(self):
        for path in commons.DATA.glob('*.schema.json'):
            commons.Draft202012Validator.check_schema(json.loads(path.read_text()))
        for path in (commons.DATA / 'examples').glob('*.json'):
            value = json.loads(path.read_text())
            self.assertEqual(self.index.validate_contribution(value)['execution'], 'unavailable')

    def test_proposal_requires_scientific_fields_and_authorization(self):
        value = json.loads((commons.DATA / 'examples/h04-proposal.json').read_text())
        for name in ['falsification_criterion', 'matched_control', 'measurement_plan',
                     'repository_revision', 'evidence', 'human_authorization']:
            invalid = copy.deepcopy(value)
            del invalid[name]
            with self.assertRaises(commons.ValidationError):
                self.index.validate_contribution(invalid)
        value['human_authorization']['status'] = 'authorized'
        with self.assertRaises(commons.CommonsError):
            self.index.validate_contribution(value)
        value['human_authorization'] = {'status': 'not_requested', 'human': None, 'record': None}
        value['status'] = 'accepted'
        with self.assertRaises(commons.ValidationError):
            self.index.validate_contribution(value)

    def assert_historical_receipt(self, checkout_path, committed, original):
        # Git objects preserve exact evidence bytes; checkout text may use CRLF.
        self.assertEqual(committed, original)
        self.assertEqual(checkout_path.read_text(encoding='utf-8'), original.decode('utf-8'))

    def test_historical_receipt_crlf_checkout_retains_byte_identity(self):
        original = (commons.DATA / 'demo-observation.initial.json').read_text(
            encoding='utf-8').encode('utf-8')
        with tempfile.TemporaryDirectory() as temporary:
            checkout = Path(temporary) / 'receipt.json'
            checkout.write_bytes(original.replace(b'\n', b'\r\n'))
            self.assertNotEqual(checkout.read_bytes(), original)
            self.assert_historical_receipt(checkout, original, original)
            with self.assertRaises(AssertionError):
                self.assert_historical_receipt(checkout, original + b' ', original)
            checkout.write_bytes(original + b' ')
            with self.assertRaises(AssertionError):
                self.assert_historical_receipt(checkout, original, original)

    def test_recorded_demonstration_matches_pinned_responses(self):
        receipt = json.loads((commons.DATA / 'demo-observation.json').read_text())
        initial = subprocess.check_output(['git', '-C', str(ROOT), 'show',
            '7183ae0de2a6d069fa1c422af335294909736eff:docs/research/commons/demo-observation.json'])
        preserved = subprocess.check_output(['git', '-C', str(ROOT), 'show',
            'HEAD:docs/research/commons/demo-observation.initial.json'])
        self.assert_historical_receipt(commons.DATA / 'demo-observation.initial.json',
                                       preserved, initial)
        self.assertEqual(receipt['manifest_sha256'], self.index.digest)
        self.assertEqual(len(receipt['observations']), 6)
        for observation in receipt['observations']:
            output = commons.canonical(self.index.dispatch(observation['request']))
            self.assertEqual(hashlib.sha256(output.encode()).hexdigest(),
                             observation['response_sha256'])

    def test_correction_dependency_and_historical_discrepancies(self):
        integration = self.index.manifest['integration']
        self.assertEqual(integration['status'], 'awaiting_pr610_merge')
        self.assertEqual(integration['main_revision'], self.index.manifest['baseline_revision'])
        self.assertNotEqual(integration['pending_revision'], integration['main_revision'])
        history = {q['id']: q for q in self.index.manifest['discrepancy_history']}
        self.assertEqual(history['D-607-IDS']['status'], 'superseded')
        self.assertIn('initiative-608', history['D-607-IDS']['superseded_by_refs'])
        self.assertEqual(history['D-608-H07-H08']['status'], 'unresolved')
        self.assertEqual(self.index.sources['initiative']['revision'],
                         '274421fd8e85c5b9bb4f165fce190715530f4e85')
        for ref in ('initiative-610-proposed', 'reconciliation-610-proposed'):
            self.assertEqual(self.index.sources[ref]['role'], 'proposal')
        self.modified(lambda m: next(s for s in m['sources']
                                    if s['id'] == 'initiative-610-proposed').update(role='authority'))
        self.modified(lambda m: m['discrepancy_history'][0]['superseded_by_refs'].append('missing'))

    def test_h07_h08_exact_dated_dispositions_and_limits(self):
        for id, tiers in [('H07', ['B', 'C']), ('H08', ['C', 'E'])]:
            h = self.index.hypothesis(id)
            self.assertEqual(h['canonical_record'], self.index.entries[id])
            self.assertEqual(h['historical_disposition_as_of'], '2026-10-25')
            self.assertEqual(h['canonical_record']['evidence_tiers'], tiers)
        h08 = self.index.hypothesis('H08')['canonical_record']
        self.assertIn('comparative benefit untested', h08['confidence'])
        self.assertIn('earlier prompt provenance not available',
                      h08['earliest_identified_source']['source'])
        self.assertIn('Repeated model summaries are not independent verification', h08['assumptions'])

    def test_fresh_process_h08_discovery_and_unapproved_proposal(self):
        command = [sys.executable, '-B', str(ROOT / 'tools/research_commons/commons.py')]
        def call(operation, **arguments):
            run = subprocess.run(command + ['--request', commons.canonical(request(operation, **arguments))],
                                 text=True, capture_output=True, check=True)
            self.assertEqual(run.stderr, '')
            return json.loads(run.stdout)
        tracks = call('list_research_tracks')['result']
        self.assertIn('H08', next(t for t in tracks if t['id'] == 'M')['hypothesis_ids'])
        h = call('get_hypothesis', hypothesis_id='H08')['result']
        self.assertIn('comparative benefit untested', h['canonical_record']['confidence'])
        self.assertEqual(h['integration']['status'], 'awaiting_pr610_merge')
        self.assertTrue(any(q['status'] == 'superseded' for q in h['discrepancy_history']))
        authority = call('get_evidence', evidence_id=h['authority']['id'])['result']
        canonical_h08 = next(e for e in json.loads(authority['content'])['entries'] if e['id'] == 'H08')
        self.assertEqual(h['canonical_record'], canonical_h08)
        proposed = call('get_evidence', evidence_id='initiative-610-proposed')['result']
        self.assertEqual(proposed['role'], 'proposal')
        self.assertIn('UNMERGED', proposed['uncertainty'])
        proposal = json.loads((commons.DATA / 'examples/h08-proposal.json').read_text())
        self.assertEqual(proposal['hypothesis_id'], 'H08')
        self.assertEqual(proposal['repository_revision'], h['integration']['main_revision'])
        self.assertEqual(proposal['human_authorization']['status'], 'not_requested')
        self.assertIn('human-only', proposal['matched_control'])
        self.assertIn('Blind adjudicators independent of generators', proposal['matched_control'])
        # The client prepares a proposal in scratch; the CLI only reads/validates it.
        with tempfile.TemporaryDirectory() as temporary:
            path = Path(temporary) / 'proposal.json'
            path.write_text(commons.canonical(proposal))
            run = subprocess.run(command + ['--validate-contribution', str(path)],
                                 text=True, capture_output=True, check=True)
            self.assertEqual(json.loads(run.stdout)['execution'], 'unavailable')

    def test_cli_success_and_fail_closed(self):
        command = [sys.executable, '-B', str(ROOT / 'tools/research_commons/commons.py')]
        value = commons.canonical(request('get_hypothesis', hypothesis_id='H08'))
        result = subprocess.run(command + ['--request', '-'], input=value,
                                text=True, capture_output=True, check=True)
        self.assertEqual(json.loads(result.stdout)['result']['id'], 'H08')
        self.assertEqual(result.stderr, '')
        for invalid in ['{', commons.canonical(request('execute_experiment'))]:
            result = subprocess.run(command + ['--request', invalid], text=True, capture_output=True)
            self.assertEqual(result.returncode, 2)
            self.assertEqual(result.stdout, '')
            self.assertEqual(json.loads(result.stderr)['error'], 'validation_failed')


if __name__ == '__main__':
    unittest.main()
