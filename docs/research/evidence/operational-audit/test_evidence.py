#!/usr/bin/env python3
"""Check the retained audit evidence; never execute supplied capability commands."""
import hashlib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import unittest

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[4]
DATA = ROOT / 'docs/research/evidence'
try:
    from jsonschema.validators import validator_for
except ImportError:
    validator_for = None


class AuditEvidence(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.inventory = json.loads((DATA / 'operational_capabilities.json').read_text())
        cls.cli = json.loads((DATA / 'operational-audit/cli-observations.json').read_text())
        cls.wrappers = json.loads((DATA / 'operational-audit/wrapper-observations.json').read_text())
        cls.observations = {o['id']: o for o in cls.cli['observations']}

    def test_actual_source_identities_and_line_locators(self):
        for id, source in self.inventory['sources'].items():
            raw = subprocess.check_output(['git', '--no-replace-objects', '-C', str(ROOT),
                                          'show', source['revision'] + ':' + source['path']])
            self.assertEqual(hashlib.sha256(raw).hexdigest(), source['sha256'], id)
        for path in self.inventory['raw_receipts']:
            self.assertTrue((DATA / path).is_file(), path)
        for capability in self.inventory['capabilities']:
            for ref in capability['evidence_references']:
                self.assertIn(ref['source_id'], self.inventory['sources'])
                self.assertGreaterEqual(ref['line'], 1)

    def test_every_advertised_command_and_script_is_accounted_for(self):
        spec = importlib.util.spec_from_file_location('probe', DATA / 'operational-audit/probe_cli.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        rows = module.catalog()
        self.assertEqual(len(rows), 69)
        self.assertEqual(sum(len(r['advertised_syntax']) for r in rows), 71)
        recorded = {c['id'] for c in self.inventory['capabilities'] if 'skill' in c}
        self.assertEqual(recorded, {r['id'] for r in rows})
        self.assertEqual(len(self.wrappers['script_sha256']), 66)
        for path, sha in self.wrappers['script_sha256'].items():
            self.assertEqual(hashlib.sha256((ROOT / path).read_bytes()).hexdigest(), sha)
        self.assertEqual(sum(not r['wrapper_exists'] for r in rows), 3)

    def test_success_counts_exclude_fake_backend_and_security_negative_cases(self):
        positive = [o for o in self.cli['observations']
                    if o['id'].startswith('direct.') and o['exit_code'] == 0]
        self.assertEqual(len(positive), 11)
        minimal = [o for o in self.cli['observations']
                   if o['id'].startswith('wrapper-minimal.') and o['exit_code'] == 0]
        self.assertEqual(len(minimal), 2)
        self.assertFalse(self.inventory['summary']['fake_cli_success_is_real_operation'])
        capabilities = [c for c in self.inventory['capabilities'] if 'skill' in c]
        self.assertEqual(sum(c['dispatch']['intended_handler_registered'] for c in capabilities), 8)

    def test_benign_eval_and_policy_negatives_remain_discoverable(self):
        eval_probe = next(o for o in self.wrappers['observations'] if o['id']=='eval-ordinary-execution')
        self.assertIn('$(printf AUDIT_CANARY)', eval_probe['argv'][-1])
        self.assertIn('inputAUDIT_CANARY', eval_probe['fake_cli_arguments'][2])
        for id in ('negative.vm-invalid-policy', 'negative.missing-policy', 'negative.code-run-missing-policy'):
            o = self.observations[id]
            self.assertEqual(o['exit_code'], 0)
            self.assertIn('47\n', o['stdout'])
        self.assertIn('Could not open policy file',
                      self.observations['negative.code-run-missing-policy']['stderr'])

    def test_denial_receipts_and_prepolicy_mutation_are_not_called_read_only(self):
        for id in ('negative.import-deny-all', 'negative.export-deny-all'):
            o = self.observations[id]
            self.assertEqual(o['exit_code'], 1)
            self.assertEqual(o['json_payload']['policy_result'], 'denied')
            self.assertTrue(o['json_payload']['provenance_ref'])
            self.assertTrue(any('objects/' in path for path in o['changed_paths']))
        o = next(o for o in self.wrappers['observations'] if o['id']=='wrapper-prepolicy-mkdir')
        self.assertEqual(o['exit_code'], 1)
        self.assertIn('prepolicy-store', o['created_paths'])
        self.assertFalse(o['fake_cli_arguments'])

    def test_labels_are_bounded_and_canonical_hypotheses_preserved(self):
        allowed = set(self.inventory['evidence_labels'])
        for c in self.inventory['capabilities']:
            self.assertTrue(set(c['labels']) <= allowed)
            self.assertNotIn('MEASURED', c['labels'])
            self.assertNotIn('COMPARATIVELY_VALIDATED', c['labels'])
        path = 'docs/research/continuity/HYPOTHESIS_REGISTER_v1.json'
        raw = (ROOT / path).read_bytes()
        self.assertEqual(hashlib.sha256(raw).hexdigest(),
                         self.inventory['scientific_authority']['sha256'])
        register = json.loads(raw)
        self.assertEqual([e['id'] for e in register['entries']], ['H%02d'%i for i in range(1,9)])
        self.assertEqual(register['entries'][6]['evidence_tiers'], ['B','C'])
        self.assertEqual(register['entries'][7]['evidence_tiers'], ['C','E'])
        self.assertIn('comparative benefit untested', register['entries'][7]['confidence'])

    @unittest.skipIf(validator_for is None, 'install jsonschema==4.23.0 to check native output schemas')
    def test_external_schema_failures_preserved_and_reproducible(self):
        spec = importlib.util.spec_from_file_location('schema_check',
            DATA / 'operational-audit/check_output_schemas.py')
        module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        recorded = json.loads((DATA / 'operational-audit/schema-validation.json').read_text())['results']
        self.assertEqual(module.observations(), recorded)
        self.assertEqual(len(recorded), 6)
        self.assertEqual(sum(not r['valid'] for r in recorded), 5)
        self.assertTrue(next(r for r in recorded if r['observation_id']=='direct.model-import')['valid'])



if __name__ == '__main__':
    unittest.main()
