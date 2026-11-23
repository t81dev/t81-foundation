"""Check the boundary between documentation presence and baseline execution."""

import contextlib
import importlib.util
import io
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch


SCRIPT = Path(__file__).resolve().parents[2] / "scripts/ci/check_stable_baseline_compliance.py"
SPEC = importlib.util.spec_from_file_location("stable_baseline_checker", SCRIPT)
CHECKER = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CHECKER)


class StableBaselineCheckerTest(unittest.TestCase):
    def setUp(self):
        self.scratch = tempfile.TemporaryDirectory(prefix="t81-baseline-check-")
        self.addCleanup(self.scratch.cleanup)
        self.root = Path(self.scratch.name).resolve()
        for name in CHECKER.REQUIRED_DOCS:
            path = self.root / name
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("presence fixture\n", encoding="utf-8")

    def invoke(self, *args, responses=()):
        stdout, stderr = io.StringIO(), io.StringIO()
        argv = [str(SCRIPT), "--repo-root", str(self.root), *args]
        with patch.object(sys, "argv", argv), \
                patch.object(CHECKER.subprocess, "run", side_effect=responses) as ctest, \
                contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            try:
                code = CHECKER.main()
            except SystemExit as error:
                code = error.code
        return code, stdout.getvalue(), stderr.getvalue(), ctest.call_args_list

    def listing(self, names=None):
        names = CHECKER.REQUIRED_CTEST_RUNNERS if names is None else names
        return subprocess.CompletedProcess(["ctest"], 0, "\n".join(names), "")

    def test_docs_only_does_not_claim_compliance_or_invoke_ctest(self):
        code, out, err, calls = self.invoke("--docs-only")
        self.assertEqual(code, 0, err)
        self.assertIn("documentation only", out)
        self.assertIn("baseline execution were not checked", out)
        self.assertNotIn("Stable baseline compliance check passed", out)
        self.assertEqual(calls, [])

    def test_docs_only_still_rejects_missing_required_document(self):
        (self.root / CHECKER.REQUIRED_DOCS[-1]).unlink()
        code, _, err, calls = self.invoke("--docs-only")
        self.assertEqual(code, 1)
        self.assertIn(CHECKER.REQUIRED_DOCS[-1], err)
        self.assertEqual(calls, [])

    def test_docs_only_cannot_be_combined_with_execution(self):
        code, _, err, calls = self.invoke("--docs-only", "--run-tests")
        self.assertEqual(code, 2)
        self.assertIn("not allowed", err)
        self.assertEqual(calls, [])

    def test_default_mode_still_requires_build_directory(self):
        code, _, err, calls = self.invoke()
        self.assertEqual(code, 1)
        self.assertIn("build directory not found", err)
        self.assertEqual(calls, [])

    def test_default_mode_still_requires_every_runner(self):
        (self.root / "build").mkdir()
        code, _, err, calls = self.invoke(
            responses=[self.listing(CHECKER.REQUIRED_CTEST_RUNNERS[:-1])])
        self.assertEqual(code, 1)
        self.assertIn(CHECKER.REQUIRED_CTEST_RUNNERS[-1], err)
        self.assertEqual(len(calls), 1)

    def test_full_presence_mode_checks_configured_ctest_listing(self):
        (self.root / "build").mkdir()
        code, out, err, calls = self.invoke(responses=[self.listing()])
        self.assertEqual(code, 0, err)
        self.assertIn("Stable baseline compliance check passed", out)
        self.assertEqual(calls[0].args[0],
                         ["ctest", "--test-dir", str(self.root / "build"), "-N"])

    def test_enumeration_failure_is_not_accepted(self):
        (self.root / "build").mkdir()
        failure = subprocess.CompletedProcess(["ctest"], 1, "", "enumeration failed")
        code, _, err, calls = self.invoke(responses=[failure])
        self.assertEqual(code, 1)
        self.assertIn("unable to enumerate", err)
        self.assertEqual(len(calls), 1)

    def test_execution_failure_is_propagated(self):
        (self.root / "build").mkdir()
        failed_test = subprocess.CompletedProcess(["ctest"], 13)
        code, _, err, calls = self.invoke("--run-tests", responses=[self.listing(), failed_test])
        self.assertEqual(code, 13)
        self.assertIn("test slice did not pass", err)
        self.assertEqual(calls[1].args[0][-2:], ["-R", CHECKER.BASELINE_CTEST_REGEX])


if __name__ == "__main__":
    unittest.main()
