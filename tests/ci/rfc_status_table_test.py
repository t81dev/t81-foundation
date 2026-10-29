"""Existing RFC table metadata is valid; lifecycle mismatch rejection remains."""
import importlib.util
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
SCRIPT = ROOT / 'scripts/governance/check_rfc_lifecycle_hygiene.py'
spec = importlib.util.spec_from_file_location('lifecycle', SCRIPT)
lifecycle = importlib.util.module_from_spec(spec)
spec.loader.exec_module(lifecycle)

class TableStatus(unittest.TestCase):
    def test_real_accepted_table_documents(self):
        for name in ('RFC-0036-t81lang-ffi-grammar.md', 'RFC-0037-tnn-stdlib.md',
                     'RFC-0038-lattice-crypto.md', 'RFC-00C4-per-device-wake-filtering.md'):
            with self.subTest(name=name):
                self.assertEqual(lifecycle.read_status_from_rfc(
                    (ROOT / 'spec/rfcs' / name).read_text()), 'accepted')

    def test_table_status_is_not_implicitly_accepted(self):
        self.assertEqual(lifecycle.read_status_from_rfc('| Status | **Draft** |\n'), 'draft')
        self.assertIsNone(lifecycle.read_status_from_rfc('| Unrelated | accepted |\n'))

    def run_fixture(self, file_status):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            script = root / 'scripts/governance' / SCRIPT.name
            script.parent.mkdir(parents=True)
            shutil.copyfile(SCRIPT, script)
            rfcs = root / 'spec/rfcs'
            rfcs.mkdir(parents=True)
            (rfcs / 'index.md').write_text('| RFC-0001 | Fixture | accepted | |\n')
            (rfcs / 'RFC-0001-fixture.md').write_text('| Field | Value |\n| Status | ' + file_status + ' |\n')
            return subprocess.run([sys.executable, str(script)], cwd=root,
                                  capture_output=True, text=True, timeout=10)

    def test_matching_explicit_status_passes(self):
        self.assertEqual(self.run_fixture('**Accepted**').returncode, 0)

    def test_lifecycle_mismatch_still_fails(self):
        result = self.run_fixture('**Draft**')
        self.assertEqual(result.returncode, 1)
        self.assertIn('status mismatch index=accepted file=draft', result.stdout)

    def test_unknown_status_still_fails(self):
        result = self.run_fixture('invented')
        self.assertEqual(result.returncode, 1)
        self.assertIn("invalid parsed status 'invented'", result.stdout)

if __name__ == '__main__':
    unittest.main(verbosity=2)
