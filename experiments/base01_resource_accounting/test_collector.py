"""Real OS tests plus explicitly mocked rejection cases; no elapsed timing."""
import json
import os
from pathlib import Path
import signal
import subprocess
import sys
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import collector as c

BUILD = None
OBSERVATIONS = []


class RealAccounting(unittest.TestCase):
    def setUp(self):
        self.collector = c.Collector(BUILD, OBSERVATIONS)

    def test_success(self):
        child = self.collector.launch()
        r = self.collector.reap(child)
        self.assertEqual(r["lifecycle"], ["spawned", "reaped"])
        self.assertEqual(r["exit_code"], 0)
        self.assertIsNone(r["signal"])
        self.assertEqual(r["wait4_child"]["subject_pid"], child.pid)
        self.assertEqual(r["child_self_before_exit"]["ppid"], os.getpid())
        self.assertEqual(r["performance_samples"], [])

    def test_nonzero_exit_is_rejected_and_retained(self):
        child = self.collector.launch("exit7")
        with self.assertRaises(c.AccountingError) as caught:
            self.collector.reap(child)
        r = caught.exception.record
        self.assertEqual(r["exit_code"], 7)
        self.assertEqual(r["validation"], "rejected")
        self.assertIn("wait4_child", r)
        self.assertIn("child_self_before_exit", r)

    def test_signal_is_rejected_and_retained(self):
        child = self.collector.launch("signal")
        with self.assertRaises(c.AccountingError) as caught:
            self.collector.reap(child)
        self.assertEqual(caught.exception.record["signal"], signal.SIGTERM)
        self.assertIsNone(caught.exception.record["exit_code"])
        self.assertIn("wait4_child", caught.exception.record)

    def test_sequential_distinct_children(self):
        first = self.collector.reap(self.collector.launch())
        second = self.collector.reap(self.collector.launch())
        self.assertNotEqual(first["pid"], second["pid"])
        self.assertEqual(first["expected_parent_pid"], second["expected_parent_pid"])

    def test_duplicate_reap_rejected_before_syscall(self):
        child = self.collector.launch()
        self.collector.reap(child)
        with patch.object(c.os, "wait4") as wait:
            with self.assertRaises(c.AccountingError):
                self.collector.reap(child)
            wait.assert_not_called()

    def test_external_reap_fails_closed(self):
        child = self.collector.launch()
        self.assertEqual(os.waitpid(child.pid, 0)[0], child.pid)
        with self.assertRaises(c.AccountingError) as caught:
            self.collector.reap(child)
        self.assertEqual(caught.exception.record["errno"], 10)  # ECHILD on supported systems.
        with patch.object(c.os, "wait4") as wait:
            with self.assertRaises(c.AccountingError):
                self.collector.reap(child)
            wait.assert_not_called()

    def test_foreign_collector_rejected_before_syscall(self):
        child = self.collector.launch()
        other = c.Collector(BUILD, OBSERVATIONS)
        with patch.object(c.os, "wait4") as wait:
            with self.assertRaises(c.AccountingError):
                other.reap(child)
            wait.assert_not_called()
        self.collector.reap(child)

    def test_wrong_parent_rejected_before_syscall(self):
        child = self.collector.launch()
        original = child.owner
        child.owner = original + 1
        try:
            with patch.object(c.os, "wait4") as wait:
                with self.assertRaises(c.AccountingError):
                    self.collector.reap(child)
                wait.assert_not_called()
        finally:
            child.owner = original
            self.collector.reap(child)

    def test_parent_child_attribution(self):
        r = self.collector.reap(self.collector.launch())
        for key in ("parent_self_before", "parent_self_after"):
            self.assertEqual(r[key]["subject_pid"], os.getpid())
            self.assertEqual(r[key]["scope"], "parent-process-all-threads")
        for key in ("aggregate_children_before", "aggregate_children_after"):
            self.assertIsNone(r[key]["subject_pid"])
            self.assertEqual(r[key]["scope"], "cumulative-waited-children-not-per-pid")
        self.assertNotEqual(r["pid"], os.getpid())
        for key in ("wait4_child", "parent_self_before", "parent_self_after",
                    "aggregate_children_before", "aggregate_children_after"):
            for field in ("user_cpu_seconds", "system_cpu_seconds", "peak_rss_kib"):
                self.assertGreaterEqual(r[key][field], 0)

    def test_exact_wait4_arguments(self):
        child = self.collector.launch()
        with patch.object(c.os, "wait4", wraps=os.wait4) as wait:
            self.collector.reap(child)
            wait.assert_called_once_with(child.pid, 0)

    def test_no_arbitrary_fixture_mode(self):
        with patch.object(c.os, "posix_spawn") as spawn:
            with self.assertRaises(c.AccountingError):
                self.collector.launch("benchmark")
            spawn.assert_not_called()

    def test_invalid_parent_sample_prevents_spawn(self):
        with patch.object(c.resource, "getrusage", return_value=None), patch.object(c.os, "posix_spawn") as spawn:
            with self.assertRaises(c.AccountingError):
                self.collector.launch()
            spawn.assert_not_called()

    def test_malformed_wait4_result_rejected(self):
        child = self.collector.launch()
        real = os.wait4
        def malformed(pid, flags):
            real(pid, flags)
            return None
        with patch.object(c.os, "wait4", side_effect=malformed):
            with self.assertRaises(c.AccountingError) as caught:
                self.collector.reap(child)
        caught.exception.record["injection"] = "synthetic malformed result after real reap"

    def test_missing_wait4_sample_rejected(self):
        child = self.collector.launch()
        real = os.wait4
        def missing(pid, flags):
            result_pid, status, _ = real(pid, flags)
            return result_pid, status, None
        with patch.object(c.os, "wait4", side_effect=missing):
            with self.assertRaises(c.AccountingError) as caught:
                self.collector.reap(child)
        self.assertEqual(caught.exception.record["validation"], "rejected")
        caught.exception.record["injection"] = "synthetic missing sample after real reap"

    def test_wrong_returned_pid_rejected(self):
        child = self.collector.launch()
        real = os.wait4
        def wrong(pid, flags):
            result_pid, status, usage = real(pid, flags)
            return result_pid + 1, status, usage
        with patch.object(c.os, "wait4", side_effect=wrong):
            with self.assertRaises(c.AccountingError) as caught:
                self.collector.reap(child)
        caught.exception.record["injection"] = "synthetic wrong PID after real reap"

    def test_missing_or_malformed_stdout_rejected(self):
        for data in (b"", b"not-json", b"{}", b"[]", b"x" * 4097):
            with self.subTest(data_length=len(data)):
                child = self.collector.launch()
                real = os.wait4
                def corrupt(pid, flags):
                    result = real(pid, flags)
                    child.output.seek(0)
                    child.output.truncate()
                    child.output.write(data)
                    child.output.flush()
                    return result
                with patch.object(c.os, "wait4", side_effect=corrupt):
                    with self.assertRaises(c.AccountingError) as caught:
                        self.collector.reap(child)
                caught.exception.record["injection"] = "synthetic stdout corruption after real reap"

    def test_binary_and_source_identity(self):
        with tempfile.TemporaryDirectory() as tmp:
            target = Path(tmp)
            manifest = dict(self.collector.manifest)
            (target / "fixture").write_bytes(self.collector.binary.read_bytes() + b"changed")
            (target / "build.json").write_text(json.dumps(manifest))
            with self.assertRaises(c.AccountingError):
                c.Collector(target, OBSERVATIONS)
            manifest["fixture_source_sha256"] = "0" * 64
            (target / "build.json").write_text(json.dumps(manifest))
            with self.assertRaises(c.AccountingError):
                c.Collector(target, OBSERVATIONS)


class Contracts(unittest.TestCase):
    def test_native_entry_rejects_unrecognized_modes(self):
        binary = str(Path(BUILD).resolve() / "fixture")
        for arguments in (("--measure",), ("--warmup",), ("--campaign",), (), ("success", "--measure")):
            with self.subTest(arguments=arguments):
                result = subprocess.run([binary, *arguments], capture_output=True, text=True)
                self.assertEqual(result.returncode, 64)
                self.assertEqual(result.stdout, "")
                self.assertEqual(result.stderr, "")

    def test_units(self):
        self.assertEqual(c.normalize_rss(2048, "Linux"), (2048, "KiB"))
        self.assertEqual(c.normalize_rss(2048, "Darwin"), (2, "bytes"))
        self.assertEqual(c.normalize_rss(0, "Linux"), (0, "KiB"))

    def test_invalid_values(self):
        for value in (None, -1, True, "12", float("nan"), float("inf"), 10 ** 1000):
            with self.subTest(value=repr(value)), self.assertRaises(c.AccountingError):
                c.normalize_rss(value, "Linux")

    def test_missing_and_malformed_samples(self):
        for sample in (None, SimpleNamespace(), SimpleNamespace(ru_utime=-1, ru_stime=0, ru_maxrss=0)):
            with self.subTest(sample=str(sample)), self.assertRaises(c.AccountingError):
                c.usage_record(sample, "Linux", "synthetic", 1)

    def test_invalid_selector_before_getrusage(self):
        with patch.object(c.resource, "getrusage") as get:
            for selector in ("WAITED_CHILD", "RUSAGE_THREAD", "", -1, 0, None):
                with self.subTest(selector=selector), self.assertRaises(c.AccountingError):
                    c.parent_snapshot(selector)
            get.assert_not_called()

    def test_unsupported_platform(self):
        for system in ("Windows", "FreeBSD", "unknown"):
            with self.subTest(system=system), self.assertRaises(c.AccountingError):
                c.supported(system)
            with self.assertRaises(c.AccountingError):
                c.normalize_rss(1, system)

    def test_unavailable_api(self):
        with patch.object(c.os, "wait4", create=True) as wait:
            # Test absence separately from a mock function's behavior.
            delattr(c.os, "wait4")
            with self.assertRaises(c.AccountingError):
                c.supported()
            c.os.wait4 = wait

    def test_platform_cannot_be_overridden_for_real_samples(self):
        wrong = "Linux" if c.platform.system() == "Darwin" else "Darwin"
        with patch.object(c.resource, "getrusage") as get:
            with self.assertRaises(c.AccountingError):
                c.parent_snapshot("RUSAGE_SELF", wrong)
            get.assert_not_called()

    def test_child_report_contract(self):
        valid = dict(pid=12, ppid=11, bytes=65536, checksum=8355840,
                     user_seconds=0, user_microseconds=0, system_seconds=0,
                     system_microseconds=0, maxrss_raw=0)
        self.assertEqual(c.validate_child(valid, 12, 11, "Linux"), valid)
        for key, value in (("pid", 13), ("ppid", 10), ("checksum", 0),
                           ("user_microseconds", 1000000), ("system_seconds", -1),
                           ("maxrss_raw", None), ("bytes", True)):
            bad = dict(valid, **{key: value})
            with self.subTest(key=key), self.assertRaises(c.AccountingError):
                c.validate_child(bad, 12, 11, "Linux")
        for bad in (None, [], {}, dict(valid, user_seconds=None)):
            with self.assertRaises(c.AccountingError):
                c.validate_child(bad, 12, 11, "Linux")

    def test_cli_rejects_measurement_before_build_or_execution(self):
        with tempfile.TemporaryDirectory() as tmp:
            for flag in ("--measure", "--timing", "--warmup", "--pilot", "--trial",
                         "--campaign", "--permit", "--benchmark", "--measure=true"):
                build, output = Path(tmp) / "absent-build", Path(tmp) / "absent-output"
                result = subprocess.run([sys.executable, str(Path(__file__).with_name("validate.py")),
                    "--validate-only", "--build-dir", str(build), "--output", str(output), flag],
                    env=dict(os.environ, BASE01_MEASUREMENT_AUTHORIZED="true", BASE01_MEASURE="1"),
                    capture_output=True, text=True)
                self.assertEqual(result.returncode, 2, result.stderr)
                self.assertIn("unrecognized arguments", result.stderr)
                self.assertFalse(build.exists())
                self.assertFalse(output.exists())


if __name__ == "__main__":
    raise SystemExit("Use validate.py --validate-only to build the fixed fixture and retain evidence")
