"""Verify that the rejection driver cannot accept the wrong failure path."""

import subprocess
import unittest
from unittest.mock import patch

import test_result_rejection as driver


class RejectionDriverTests(unittest.TestCase):
    def test_all_cases_use_argument_vectors_and_their_own_diagnostic(self):
        executable = 'path with spaces/quoted "name" & shell.exe'

        def child(argv, **kwargs):
            self.assertEqual(argv[0:2], [executable, "--negative"])
            self.assertIs(kwargs["shell"], False)
            self.assertEqual(kwargs["timeout"], 10)
            self.assertTrue(kwargs["capture_output"])
            diagnostic = (driver.BUDGET_DIAGNOSTIC if argv[2] == "budget"
                          else driver.ORACLE_DIAGNOSTIC)
            return subprocess.CompletedProcess(argv, 1, "", diagnostic)

        with patch.object(driver.subprocess, "run", side_effect=child) as run:
            driver.verify_rejections(executable)
        self.assertEqual([call.args[0][2] for call in run.call_args_list], list(driver.CASES))

    def test_wrong_exit_or_missing_stderr_diagnostic_is_rejected(self):
        for status, stdout, stderr in (
            (0, "", driver.ORACLE_DIAGNOSTIC),
            (2, "", driver.ORACLE_DIAGNOSTIC),
            (-11, "", driver.ORACLE_DIAGNOSTIC),
            (1, "", "unrelated assertion failure"),
            (1, driver.ORACLE_DIAGNOSTIC, ""),
            (1, "", driver.BUDGET_DIAGNOSTIC),
        ):
            with self.subTest(status=status, stdout=stdout, stderr=stderr):
                result = subprocess.CompletedProcess([], status, stdout, stderr)
                with patch.object(driver.subprocess, "run", return_value=result):
                    with self.assertRaises(SystemExit):
                        driver.verify_rejections("unused")

    def test_launch_failure_and_timeout_are_rejected(self):
        for error in (OSError("missing executable"), subprocess.TimeoutExpired([], 10)):
            with self.subTest(error=error):
                with patch.object(driver.subprocess, "run", side_effect=error):
                    with self.assertRaises(SystemExit):
                        driver.verify_rejections("unused")


if __name__ == "__main__":
    unittest.main()
