#!/usr/bin/env python3
"""Narrow documentation contract for PR608's H07/H08 and freeze-check corrections."""
import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[2]
INITIATIVE = ROOT / 'docs/research/HUMAN_AI_RESEARCH_INITIATIVE.md'
REGISTER = ROOT / 'docs/research/continuity/HYPOTHESIS_REGISTER_v1.json'
CHECKER = 'scripts/ci/check_tisc_freeze_integrity.py'


def issues(text, register):
    errors = []
    for entry in register['entries']:
        if entry['id'] not in ('H07', 'H08'):
            continue
        rows = [line for line in text.splitlines()
                if line.startswith('| **' + entry['id'] + '** |')]
        if len(rows) != 1:
            errors.append(entry['id'] + ': expected one canonical row')
            continue
        row = rows[0]
        cells = [cell.strip() for cell in row.split('|')]
        if cells[3] != entry['formal_claim']:
            errors.append(entry['id'] + ': formal claim differs from register')
        for field in ('result', 'confidence', 'next_action'):
            if entry[field] not in cells[4]:
                errors.append(entry['id'] + ': missing exact register ' + field)
        tiers = 'Register evidence tiers: ' + ', '.join(entry['evidence_tiers']) + '.'
        if tiers not in cells[4] or 'established' in cells[4].lower():
            errors.append(entry['id'] + ': evidence tiers/status promoted')
    for qualification in (register['as_of'], 'Earlier prompt provenance is not available.',
                          'repeated model summaries are not independent',
                          'simulated dispatch supplies no inference-security or malicious-host claim.'):
        if qualification not in ' '.join(text.split()):
            errors.append('missing dated evidence limitation: ' + qualification)
    lines = [line for line in text.splitlines()
             if line.startswith('- **TISC ISA / Core Encoding:**')]
    if len(lines) != 1:
        errors.append('expected one ISA verification entry')
    else:
        for required in ('python3 ' + CHECKER, 'registry integrity',
                         'not executable opcode conformance',
                         'separately built and run tests'):
            if required not in lines[0]:
                errors.append('missing verification scope: ' + required)
    if 't81_tisc_freeze_integrity_test' in text:
        errors.append('nonexistent freeze CTest cited')
    return errors


class InitiativeReconciliation(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.text = INITIATIVE.read_text(encoding='utf-8')
        cls.register = json.loads(REGISTER.read_text(encoding='utf-8'))

    def test_canonical_dispositions_and_actual_verification_reference(self):
        self.assertEqual(issues(self.text, self.register), [])
        self.assertTrue((ROOT / CHECKER).is_file())

    def test_rejects_reinstated_established_status(self):
        for entry in self.register['entries'][6:]:
            changed = self.text.replace(entry['result'], 'Established for artifact identity.')
            self.assertTrue(issues(changed, self.register))

    def test_rejects_tier_promotion_and_removed_provenance_limit(self):
        changed = self.text.replace('Register evidence tiers: C, E.', 'Register evidence tiers: A.')
        self.assertTrue(issues(changed, self.register))
        changed = self.text.replace('Earlier prompt provenance is not available.', '')
        self.assertTrue(issues(changed, self.register))

    def test_rejects_nonexistent_test_or_unqualified_conformance(self):
        changed = self.text.replace('python3 ' + CHECKER, 't81_tisc_freeze_integrity_test')
        self.assertTrue(issues(changed, self.register))
        changed = self.text.replace('not executable opcode conformance', 'executable opcode conformance')
        self.assertTrue(issues(changed, self.register))


if __name__ == '__main__':
    unittest.main()
