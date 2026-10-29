"""CLI regressions for moved book and spec-evidence paths; contracts stay strict."""
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

REPO = Path(__file__).resolve().parents[2]
BOOK_CHECK = 'scripts/governance/check_docs_structure.py'
SPEC_CHECK = 'scripts/governance/check_spec_code_alignment_baseline.py'
EVIDENCE = (
    'spec/tisc-spec.md', 'spec/t81vm-spec.md', 'spec/axion-kernel.md',
    'spec/determinism-profile.md', 'isa/encoding.cpp', 'vm/vm.cpp',
    'tests/cpp/tisc_opcode_matrix_test.cpp', 'scripts/ci/check_tisc_freeze_integrity.py',
    'tests/cpp/vm_determinism_property_test.cpp', 'tests/cpp/jit_trace_equivalence_test.cpp',
    'kernel/axion/policy_engine.cpp', 'tests/cpp/vm_policy_parse_fail_closed_test.cpp',
    'tests/cpp/test_axion_opcodes.cpp', 'docs/governance/DETERMINISM_SURFACE_REGISTRY.md',
    'scripts/ci/t81lang_repro_gate.py', 'scripts/ci/t3k_repro_gate.py',
)

class LayoutPaths(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        for name in (BOOK_CHECK, SPEC_CHECK, *EVIDENCE):
            out = self.root / name
            out.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(REPO / name, out)
        for language in ('en', 'cn', 'es', 'pt', 'ru'):
            book = self.root / 'docs/book' / ('book-' + language)
            book.mkdir(parents=True)
            for chapter in range(1, 16):
                (book / f'{chapter:02d}_chapter.md').write_text('fixture\n')

    def run_check(self, name, root=None):
        return subprocess.run([sys.executable, str((root or self.root) / name)],
                              cwd=root or self.root, capture_output=True, text=True,
                              timeout=10)

    def test_actual_current_paths_pass(self):
        for check in (BOOK_CHECK, SPEC_CHECK):
            with self.subTest(check=check):
                result = self.run_check(check, REPO)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)

    def test_missing_chapter_is_still_rejected(self):
        (self.root / 'docs/book/book-en/01_chapter.md').unlink()
        result = self.run_check(BOOK_CHECK)
        self.assertEqual(result.returncode, 1)
        self.assertIn('book-en: missing chapter prefixes 01', result.stdout)

    def test_old_book_tree_cannot_replace_current_paths(self):
        (self.root / 'docs/book').rename(self.root / 'book')
        result = self.run_check(BOOK_CHECK)
        self.assertEqual(result.returncode, 1)
        self.assertIn('missing directory: docs/book/book-en', result.stdout)

    def test_removed_mapped_sources_are_still_rejected(self):
        for name in ('isa/encoding.cpp', 'vm/vm.cpp'):
            path = self.root / name
            raw = path.read_bytes()
            path.unlink()
            with self.subTest(name=name):
                result = self.run_check(SPEC_CHECK)
                self.assertEqual(result.returncode, 1)
                self.assertIn('missing mapped implementation/evidence file: ' + name,
                              result.stdout)
            path.write_bytes(raw)

    def test_obsolete_core_copies_do_not_mask_missing_sources(self):
        for name in ('isa/encoding.cpp', 'vm/vm.cpp'):
            obsolete = self.root / 'core' / name
            obsolete.parent.mkdir(parents=True, exist_ok=True)
            (self.root / name).rename(obsolete)
        result = self.run_check(SPEC_CHECK)
        self.assertEqual(result.returncode, 1)
        self.assertIn('isa/encoding.cpp', result.stdout)
        self.assertIn('vm/vm.cpp', result.stdout)

if __name__ == '__main__':
    unittest.main(verbosity=2)
