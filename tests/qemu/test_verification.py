#!/usr/bin/env python3
"""Isolated production-entrypoint regressions using a fake QMP/serial peer."""
import os
import json
from pathlib import Path
import shutil
import signal
import subprocess
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[2]
ENTRY = ROOT / 'drivers/qemu/scripts/automated_verification.sh'


class VerificationTests(unittest.TestCase):
    receipts = []

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix='t81-verification-test-')
        self.addCleanup(self.temp.cleanup)
        self.path = Path(self.temp.name).resolve()
        self.fake = self.path / 'fake-qemu'
        shutil.copyfile(ROOT / 'tests/qemu/fake_qemu.py', self.fake)
        self.fake.chmod(0o755)
        self.image, self.bios = self.path / 'image', self.path / 'bios'
        self.image.write_bytes(b'fixture image')
        self.bios.write_bytes(b'fixture firmware')
        self.parent = self.path / 'results with spaces'
        self.env = dict(os.environ, QEMU_BIN=str(self.fake), QEMU_IMG=str(self.image),
                        BIOS=str(self.bios), OUTPUT_DIR=str(self.parent),
                        VERIFICATION_TIMEOUT='2', FAKE_MODE='pass')

    def run_case(self, mode='pass', entry=ENTRY, expect=0, cwd=None, env=None):
        config = dict(self.env, FAKE_MODE=mode)
        if env is not None:
            config = env
        result = subprocess.run(['bash', str(entry)], env=config, cwd=cwd or self.path,
                                text=True, capture_output=True, timeout=8)
        self.receipts.append(dict(mode=mode, entry=str(entry.relative_to(ROOT)) if entry.is_relative_to(ROOT) else str(entry), exit=result.returncode, stdout=result.stdout, stderr=result.stderr))
        if expect is None:
            self.assertNotEqual(result.returncode, 0, result.stdout + result.stderr)
        else:
            self.assertEqual(result.returncode, expect, result.stdout + result.stderr)
        self.assertEqual('Status: VERIFICATION SUCCESSFUL' in result.stdout,
                         result.returncode == 0, result.stdout)
        if result.returncode:
            self.assertNotIn('VERIFICATION SUCCESSFUL', result.stdout + result.stderr)
        return result

    def test_positive_repeat_counts_and_paths(self):
        first = self.run_case()
        second = self.run_case()
        self.assertIn('Tests Completed: 5/5', first.stdout)
        self.assertEqual(first.stdout.splitlines()[1:], second.stdout.splitlines()[1:])
        attempts = list(self.parent.iterdir())
        self.assertEqual(len(attempts), 2)
        for attempt in attempts:
            self.assertIn('VERIFICATION SUCCESSFUL', (attempt / 'verification_summary.txt').read_text())
            self.assertEqual((attempt / 'verification_memory.dump').stat().st_size, 4096)
        self.assertEqual(self.image.read_bytes(), b'fixture image')

    def test_negative_matrix(self):
        for mode in ('missing', 'empty', 'exit', 'signal', 'hang', 'failure',
                     'incomplete', 'qmp_error', 'stopped', 'missing_dump',
                     'empty_dump', 'bad_cpu', 'bad_block', 'missing_help',
                     'late_failure', 'hang_shutdown', 'exit_shutdown', 'dribble', 'ignore_term', 'early_help'):
            with self.subTest(mode=mode):
                self.run_case(mode, expect={'exit': 37, 'signal': 143, 'exit_shutdown': 23}.get(mode))
        for summary in self.parent.glob('*/verification_summary.txt'):
            self.assertNotIn('VERIFICATION SUCCESSFUL', summary.read_text())

    def test_stale_success_cannot_satisfy_new_attempt(self):
        self.run_case()
        (self.parent / 'verification_summary.txt').write_text('Status: VERIFICATION SUCCESSFUL\n')
        self.run_case('incomplete', expect=124)
        self.assertEqual(len(list(self.parent.glob('*/verification_summary.txt'))), 2)

    def test_missing_executable_image_firmware(self):
        for variable in ('QEMU_BIN', 'QEMU_IMG', 'BIOS'):
            with self.subTest(variable=variable):
                self.run_case(expect=None, env=dict(self.env, **{variable: str(self.path / 'absent')}))
        self.fake.chmod(0o644)
        self.run_case(expect=None)

    def test_alternate_entrypoints_and_working_directory(self):
        for entry in (ROOT / 'ternaryos/automated_verification.sh',
                      ROOT / 'drivers/qemu/scripts/verify_production.sh'):
            with self.subTest(entry=str(entry)):
                self.run_case(entry=entry, cwd=Path('/'))
                self.run_case('exit', entry=entry, expect=None)

    def test_output_directory_unset_empty_relative_and_invalid(self):
        for value in (None, '', 'relative-results'):
            config = dict(self.env, TMPDIR=str(self.path))
            if value is None:
                config.pop('OUTPUT_DIR')
            else:
                config['OUTPUT_DIR'] = value
            result = self.run_case(env=config)
            out = Path(result.stdout.splitlines()[0].removeprefix('Results: '))
            self.assertTrue(out.is_absolute())
            self.assertTrue(out.is_relative_to(self.path))
        self.run_case(expect=None, env=dict(self.env, OUTPUT_DIR=str(self.image)))

    def test_timeout_exit_status(self):
        self.run_case('hang', expect=124)
        self.run_case('hang_shutdown', expect=124)

    def test_interrupted_harness(self):
        pid_file = self.path / 'child.pid'
        config = dict(self.env, FAKE_MODE='hang', VERIFICATION_TIMEOUT='10',
                      FAKE_PID_FILE=str(pid_file))
        process = subprocess.Popen(['bash', str(ENTRY)], env=config, stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE, text=True)
        try:
            self.assertTrue(process.stdout.readline().startswith('Results: '))
            deadline = time.monotonic() + 2
            while not pid_file.exists() and time.monotonic() < deadline:
                time.sleep(.01)
            self.assertTrue(pid_file.exists(), 'mock child did not start')
            child_pid = int(pid_file.read_text())
            process.send_signal(signal.SIGTERM)
            stdout, stderr = process.communicate(timeout=5)
            self.assertEqual(process.returncode, 143, stdout + stderr)
            with self.assertRaises(ProcessLookupError):
                os.kill(child_pid, 0)
            self.assertNotIn('VERIFICATION SUCCESSFUL', stdout + stderr)
            summary = next(self.parent.glob('*/verification_summary.txt'))
            self.assertIn('VERIFICATION FAILED', summary.read_text())
        finally:
            if process.poll() is None:
                process.kill()
                process.wait()

    def test_generated_production_verifier(self):
        source = (ROOT / 'ternaryos/production_qemu_setup.sh').read_text()
        block = source.split('cat > "$SCRIPTS_DIR/verify_production.sh" << \'EOF\'\n', 1)[1].split('\nEOF', 1)[0]
        deployment = self.path / 'deployment'
        (deployment / 'scripts').mkdir(parents=True)
        (deployment / 't81-foundation').symlink_to(ROOT, target_is_directory=True)
        entry = deployment / 'scripts/verify_production.sh'
        entry.write_text(block)
        self.run_case(entry=entry)
        self.run_case('exit', entry=entry, expect=None)

    def test_missing_script_and_summary_ignored(self):
        self.run_case(entry=self.path / 'nonexistent.sh', expect=None)
        self.parent.mkdir()
        (self.parent / 'verification_summary.txt').write_text('VERIFICATION SUCCESSFUL\n')
        self.run_case('exit', expect=None)

    def test_baseline_arithmetic_and_safe_increment(self):
        # Bash 3.2 does not apply errexit to this arithmetic command. Retain the
        # actual behavior rather than incorrectly claiming local reproduction.
        unsafe = subprocess.run(['bash', '-c', 'set -e; n=0; ((n++)); echo REACHED'],
                                capture_output=True, text=True)
        self.assertIn(unsafe.returncode, (0, 1))
        safe = subprocess.run(['bash', '-c', 'set -e; n=0; n=$((n + 1)); echo "$n"'],
                              capture_output=True, text=True)
        self.assertEqual(safe.returncode, 0)
        self.assertEqual(safe.stdout, '1\n')
        print(f'baseline arithmetic: exit={unsafe.returncode}, stdout={unsafe.stdout.strip()!r}')


if __name__ == '__main__':
    program = unittest.main(verbosity=2, exit=False)
    print(f'entrypoint process receipts: {len(VerificationTests.receipts)}')
    if os.environ.get('T81_VERIFICATION_RECEIPTS'):
        Path(os.environ['T81_VERIFICATION_RECEIPTS']).write_text(json.dumps(VerificationTests.receipts, indent=2) + '\n')
    raise SystemExit(not program.result.wasSuccessful())
