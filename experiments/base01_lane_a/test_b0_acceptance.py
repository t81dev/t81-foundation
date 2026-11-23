"""Deterministic acceptance checks. No interval, warm-up or trial is executed.

Native rejection tests require BASE01_ACCEPTANCE_NATIVE to identify an already
built Lane A executable; otherwise they report an explicit skip.
"""
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

spec = importlib.util.spec_from_file_location("acceptance_lane_a", Path(__file__).with_name("lane_a.py"))
lane = importlib.util.module_from_spec(spec)
spec.loader.exec_module(lane)


class B0Acceptance(unittest.TestCase):
    def test_full_manifest_identity_is_required(self):
        original = (lane.ROOT / "phase-a-manifest.json").read_bytes()
        manifest = lane.verify_manifest(original)
        self.assertEqual(manifest["protocol"], "BASE-01")
        self.assertEqual(manifest["version"], "0.1.0-draft")
        mutations = []
        for key, value in [("protocol", "other"), ("version", "other"),
                           ("measurement_authorized", True)]:
            changed = json.loads(original)
            changed[key] = value
            mutations.append(changed)
        changed = json.loads(original)
        changed["revisions"]["inspected_head"] = "0" * 40
        mutations.append(changed)
        changed = json.loads(original)
        changed["workloads"][0]["eligibility"] = "EXCLUDED"
        mutations.append(changed)
        changed = json.loads(original)
        changed["research_boundaries"]["h04_r03_phase_c"]["measurement_authorized"] = True
        mutations.append(changed)
        for changed in mutations:
            # Keep all five minimal IDs intact: the former ID-only check missed this.
            self.assertEqual(changed["recommended_minimal_phase_b_workload_set"], lane.IDS)
            with self.assertRaisesRegex(ValueError, "manifest identity mismatch"):
                lane.verify_manifest(json.dumps(changed).encode())
        with self.assertRaises(ValueError):
            lane.verify_manifest(original + b"\n")

    def test_retained_evidence_and_historical_implementation_identity(self):
        final = lane.ROOT / "evidence/final"
        receipt = json.loads((final / "receipt.json").read_text())
        self.assertEqual(receipt["source_sha"], lane.SOURCE_SHA)
        self.assertEqual(receipt["runner_sha"], "54c1a92ffe5c19871c3b1a68c71083ac87fc7f0c")
        self.assertEqual(receipt["manifest_commit"], lane.MANIFEST_SHA)
        self.assertEqual(receipt["manifest_sha256"], lane.MANIFEST_SHA256)
        self.assertIs(receipt["measurement_authorized"], False)
        self.assertEqual(receipt["performance_samples"], [])
        for name, expected in json.loads((final / "sha256.json").read_text()).items():
            self.assertEqual(lane.digest((final / name).read_bytes()), expected, name)
        repository = lane.ROOT.parents[1]
        for name, expected in receipt["runner_files"].items():
            path = str((lane.ROOT / name).relative_to(repository))
            historical = subprocess.check_output(["git", "-C", str(repository),
                                                   "show", receipt["runner_sha"] + ":" + path])
            self.assertEqual(hashlib.sha256(historical).hexdigest(), expected, name)
        pinned = subprocess.check_output(["git", "-C", str(repository), "show",
            lane.MANIFEST_SHA + ":docs/research/base01/BASE01_MANIFEST.json"])
        self.assertEqual(pinned, (lane.ROOT / "phase-a-manifest.json").read_bytes())
        correctness = json.loads((final / "correctness.json").read_text())
        self.assertEqual(len(correctness), 4)
        self.assertTrue(all(record["returncode"] == 0 for record in correctness))
        native = json.loads((final / "native-validation.json").read_text())
        self.assertEqual(lane.verify_codec(native["codec"]),
                         receipt["input_identities"][lane.IDS[0]])
        self.assertEqual(len(native["vm"]), 6)
        self.assertEqual([case["bytes"] for case in native["canonfs"]],
                         [4096, 65536, 1048576])
        self.assertEqual(receipt["resource_observer"]["scope"],
                         "orchestrator RUSAGE_SELF smoke check only")

    @unittest.skipUnless(lane.platform.system() in ("Darwin", "Linux"), "POSIX resource check only")
    def test_actual_resource_smoke_and_negative_invariants(self):
        result = lane.resource_validation()
        self.assertEqual(result, {"status": "validated",
            "scope": "orchestrator RUSAGE_SELF smoke check only",
            "rss_units": "KiB", "cpu_time_samples_retained": False})
        # Fixture counters test rejection; they are not collected performance data.
        before = SimpleNamespace(ru_maxrss=10, ru_utime=2, ru_stime=1)
        for after in [SimpleNamespace(ru_maxrss=9, ru_utime=2, ru_stime=1),
                      SimpleNamespace(ru_maxrss=10, ru_utime=1, ru_stime=1),
                      SimpleNamespace(ru_maxrss=10, ru_utime=2, ru_stime=0)]:
            with patch("resource.getrusage", side_effect=[before, after]):
                with self.assertRaisesRegex(ValueError, "getrusage monotonic invariant"):
                    lane.resource_validation()

    @unittest.skipUnless(os.environ.get("BASE01_ACCEPTANCE_NATIVE"), "set BASE01_ACCEPTANCE_NATIVE for native gate checks")
    def test_native_gate_rejects_alternate_arguments_and_environment(self):
        native = str(Path(os.environ["BASE01_ACCEPTANCE_NATIVE"]).resolve())
        env = dict(os.environ, BASE01_MEASUREMENT_AUTHORIZED="1",
                   BASE01_PHASE_B1_APPROVED="1", BASE01_AUTHORIZE_MEASUREMENT="1")
        with tempfile.TemporaryDirectory() as scratch:
            for args in [[], ["--measure"], ["--measure=true"], ["--trial"],
                         ["--warmup"], ["--cmp3"], ["--h04-r03-phase-c"],
                         ["--validate-only", "--measure"],
                         ["--validate-only", "--authorize-base01-measurement"]]:
                with self.subTest(args=args):
                    proc = subprocess.run([native, *args], env=env, cwd=scratch,
                                          text=True, capture_output=True)
                    self.assertEqual(proc.returncode, 2)
                    self.assertEqual(proc.stdout, "")
                    self.assertIn("only --validate-only is available", proc.stderr)
                    self.assertEqual(list(Path(scratch).iterdir()), [])


if __name__ == "__main__":
    unittest.main()
