"""Exercise maturity validation through the real CLI and isolated document fixtures."""
from pathlib import Path
import json
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
CHECK = 'scripts/governance/check_status_label_coherence.py'
CANONICAL = 'docs/governance/MATURITY_CANONICAL_STATUS.json'
FILES = (CHECK, 'README.md', 'docs/status/IMPLEMENTATION_MATRIX.md',
         'docs/status/SYSTEM_STATUS.md', 'docs/status/T81LANG_PROMOTION_GATE.md',
         CANONICAL, 'spec/t81lang-spec.md', 'spec/t81vm-spec.md', 'spec/axion-kernel.md')


class MaturityMetadata(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        for name in FILES:
            target = self.root / name
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / name, target)

    def check(self, expected, diagnostic=None):
        result = subprocess.run([sys.executable, CHECK], cwd=self.root,
                                capture_output=True, text=True, timeout=10)
        self.assertEqual(result.returncode, expected, result.stdout + result.stderr)
        if diagnostic:
            self.assertIn(diagnostic, result.stdout + result.stderr)
        return result

    def write_canonical(self, payload):
        (self.root / CANONICAL).write_text(json.dumps(payload))

    def test_current_documents_pass(self):
        self.check(0)

    def test_header_order_and_decorated_labels(self):
        (self.root / 'README.md').write_text(
            '| Maturity | Component | Role |\n| --- | --- | --- |\n'
            '| **Stable** | T81VM | Interpreter |\n'
            '| ✅ Stable | T81Lang | Compiler |\n'
            '| `Stable` | Axion | Authorization |\n')
        self.check(0)

    def test_empty_canonical_components_fail(self):
        self.write_canonical({'components': {}})
        self.check(1, "nonempty 'components' object")

    def test_malformed_canonical_metadata_fails(self):
        for payload in ({}, {'components': []}, [], {'components': {'t81vm': None}},
                        {'components': {'t81vm': {'maturity': 1}}}):
            with self.subTest(payload=payload):
                self.write_canonical(payload)
                self.check(1)
        (self.root / CANONICAL).write_text('{')
        self.check(1)

    def test_missing_canonical_fields_fail(self):
        original = json.loads((self.root / CANONICAL).read_text())
        for field in ('maturity', 'spec_authority', 'spec_path'):
            with self.subTest(field=field):
                payload = json.loads(json.dumps(original))
                del payload['components']['t81vm'][field]
                self.write_canonical(payload)
                self.check(1, field)

    def test_invalid_labels_fail_in_every_governed_document(self):
        surfaces = ('README.md', 'docs/status/IMPLEMENTATION_MATRIX.md',
                    'docs/status/SYSTEM_STATUS.md')
        for name in surfaces:
            path = self.root / name
            original = path.read_text()
            for label in ('Unstable', 'Not Stable', 'Stable-ish', 'BetaStable', ''):
                with self.subTest(surface=name, label=label):
                    lines = original.splitlines(True)
                    for i, line in enumerate(lines):
                        if line.startswith('| **T81VM**'):
                            lines[i] = line.replace('Stable', label)
                    path.write_text(''.join(lines))
                    self.check(1)
            path.write_text(original)

    def test_invalid_canonical_labels_fail(self):
        original = json.loads((self.root / CANONICAL).read_text())
        for field in ('maturity', 'spec_authority'):
            for label in ('Unstable', 'Not Stable', 'Stable-ish'):
                with self.subTest(field=field, label=label):
                    payload = json.loads(json.dumps(original))
                    payload['components']['t81vm'][field] = label
                    self.write_canonical(payload)
                    self.check(1, 'invalid ' + field)

    def test_missing_maturity_header_fails(self):
        path = self.root / 'README.md'
        path.write_text(path.read_text().replace('| Maturity |', '| Role detail |'))
        self.check(1, 'README missing component status row')

    def test_duplicate_maturity_header_fails(self):
        path = self.root / 'README.md'
        path.write_text(path.read_text().replace('| Maturity |', '| Maturity | Maturity |'))
        self.check(1, 'duplicate Component/Maturity columns')

    def test_duplicate_component_rows_fail(self):
        path = self.root / 'README.md'
        line = next(line for line in path.read_text().splitlines(True)
                    if line.startswith('| **T81VM**'))
        path.write_text(path.read_text().replace(line, line + line))
        self.check(1, 'duplicate component status rows')

    def test_missing_required_document_fails(self):
        (self.root / 'docs/status/SYSTEM_STATUS.md').unlink()
        self.check(1, 'missing required file: docs/status/SYSTEM_STATUS.md')


if __name__ == '__main__':
    unittest.main(verbosity=2)
