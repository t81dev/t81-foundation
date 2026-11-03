"""Synthetic durations and mocked clocks only: NEVER run a performance trial."""
import copy
import hashlib
import json
from pathlib import Path
import struct
import tempfile
import unittest
from unittest.mock import patch

from experiments.h04_r03_matched_codec import measurement_engine as engine
from experiments.h04_r03_matched_codec.benchmark_harness import compute_bootstrap_ci, evaluate_decision_rules
from experiments.h04_r03_matched_codec.codec import encode


class FakeMemory:
    def __init__(self, rss=1, scratch=0, reclaimed=True):
        self.rss, self.scratch, self.reclaimed = rss, scratch, reclaimed
        self.entered = []
    def enter(self, phase):
        self.entered.append(phase)
    def leave(self, phase):
        return {"peak_rss_bytes": self.rss, "peak_scratch_bytes": self.scratch,
                "reclaimed": self.reclaimed, "continuous_enforcement": True}


def fixture(arm="T243", n=65, bad_checksum=False):
    values = [(i * 19) % 729 - 364 for i in range(n)]
    raw = struct.pack(f"<{n}h", *values)
    # SHA-256 is exclusively a test double, never presented as xxHash64.
    expected = hashlib.sha256(struct.pack(f"<{n}q", *values)).hexdigest()
    instance = engine._Lifecycle(raw, [0, n - 1, 0, n // 2], arm, hashlib.sha256,
                                 "bad" if bad_checksum else expected, n=n)
    return instance, values, expected


def instrument(instance, memory=None):
    ticks = iter(range(0, 10000, 10))
    with patch.object(engine.time, "perf_counter_ns", side_effect=AssertionError("real clock forbidden")):
        return engine._instrument(instance, clock=lambda: next(ticks), memory=memory or FakeMemory())


def blocks_fixture(seed=73):
    blocks = []
    for number, order in enumerate(engine.paired_orders(seed)):
        arms = {}
        for arm in engine.ARMS:
            duration = 100 if arm == "T243" else 120
            phases = {phase: duration for phase in engine.PHASES}
            arms[arm] = {
                "valid": True, "failure": None, "phase_ns": phases,
                "lifecycle_ns": 7 * duration, "query_ns": duration,
                "process_identity": f"synthetic-{number}-{arm}",
                "resources": [FakeMemory().leave(p) for p in engine.PHASES],
                "checksum": "synthetic-checksum", "witness_checksum": "synthetic-checksum",
            }
        blocks.append({"order": order, "arms": arms})
    return blocks


def host_fixture():
    caches = [{"level": level, "size": "32K", "line_bytes": 64,
               "shared_cpu_list": "0", "type": "Unified"} for level in (1, 2, 3)]
    cpus = {str(i): {"package": 0, "core": i, "governor": "performance",
                     "min_khz": 2000000, "max_khz": 2000000, "caches": caches}
            for i in range(4)}
    snapshot = {"system": "Linux", "machine": "x86_64", "kernel": "synthetic-lts",
                "cpus": cpus, "affinity": [0], "isolated": [0], "turbo": {"boost": 0},
                "swap_bytes": 0, "allocation_limit_bytes": 16 * 1024**3}
    pins = {"kernel": "synthetic-lts", "lts_evidence": "test fixture", "affinity": [0],
            "frequency_khz": 2000000, "caches": {"0": caches},
            "dedicated_host_evidence": "fixture", "cache_reset_procedure": "fixture",
            "memory_enforcement_evidence": "fixture"}
    return snapshot, pins


class MeasurementEngineTests(unittest.TestCase):
    def test_all_eight_phases_use_mock_clock(self):
        for arm in engine.ARMS:
            with self.subTest(arm=arm):
                instance, _, checksum = fixture(arm)
                memory = FakeMemory()
                result = instrument(instance, memory)
                self.assertTrue(result["valid"], result)
                self.assertEqual(result["phase_ns"], dict.fromkeys(engine.PHASES, 10))
                self.assertEqual(result["lifecycle_ns"], 70)
                self.assertEqual(result["query_ns"], 10)
                self.assertEqual(memory.entered, list(engine.PHASES))
                self.assertEqual(result["checksum"], checksum)
                self.assertIsNone(instance.frame)
                self.assertIsNone(instance.queries)

    def test_adapters_preserve_wire_and_retention(self):
        for arm in engine.ARMS:
            for n in (1, 5, 63, 64, 65, 128):
                with self.subTest(arm=arm, n=n):
                    instance, values, _ = fixture(arm, n)
                    instance.input_mapping()
                    self.assertEqual(list(instance.mapped), [x + 364 for x in values])
                    instance.frame_allocation()
                    allocation = instance.frame
                    instance.encoding()
                    self.assertIs(instance.frame, allocation)
                    self.assertEqual(instance.frame, encode(values, arm, 64, 1, "RA-FIXED"))
                    instance.retention_boundary()
                    self.assertIsNone(instance.raw)
                    self.assertIsNone(instance.mapped)
                    instance.frame_validation()
                    instance.cleanup()

    def test_checksum_failure_and_cleanup(self):
        instance, _, _ = fixture(bad_checksum=True)
        result = instrument(instance)
        self.assertFalse(result["valid"])
        self.assertIn("checksum", result["failure"])
        self.assertIsNone(result["lifecycle_ns"])
        self.assertIn("cleanup", result["phase_ns"])
        self.assertIsNone(instance.output)

    def test_memory_failures(self):
        for memory in (FakeMemory(rss=engine.RSS_LIMIT + 1),
                       FakeMemory(scratch=engine.SCRATCH_LIMIT + 1),
                       FakeMemory(reclaimed=False)):
            instance, _, _ = fixture()
            result = instrument(instance, memory)
            self.assertFalse(result["valid"])
            self.assertIsNone(instance.frame)
        instance, _, _ = fixture()
        with self.assertRaises(engine.ProtocolError):
            engine._instrument(instance, clock=lambda: self.fail("clock read before enforcement"))

    def test_clock_failure_is_invalid(self):
        instance, _, _ = fixture()
        result = engine._instrument(instance, clock=lambda: 1, memory=FakeMemory())
        self.assertFalse(result["valid"])
        self.assertIsNone(instance.raw)

    def test_corruption_in_validation(self):
        for arm in engine.ARMS:
            instance, _, _ = fixture(arm)
            instance.input_mapping()
            instance.frame_allocation()
            instance.encoding()
            instance.frame[0] ^= 1
            with self.assertRaises(engine.ProtocolError):
                instance.frame_validation()
            instance.cleanup()

    def test_gate_always_frozen(self):
        with patch.object(engine.time, "perf_counter_ns", side_effect=AssertionError("clock forbidden")):
            for kwargs in ({}, {"authorized": True, "qualified": True}):
                with self.assertRaisesRegex(engine.ProtocolError, "FROZEN"):
                    engine.run_measurements(**kwargs)

    def test_input_preflight_rejects_before_lifecycle(self):
        with self.assertRaisesRegex(engine.ProtocolError, "lengths"):
            engine.verify_inputs(b"", b"")
        with self.assertRaisesRegex(engine.ProtocolError, "dataset digest"):
            engine.verify_inputs(bytes(2 * engine.WORKLOAD_N), bytes(4 * engine.WORKLOAD_N))

    def test_host_qualification_fail_closed(self):
        snapshot, pins = host_fixture()
        self.assertTrue(engine.qualify_host(snapshot, pins)["qualified"])
        cases = {"system": "Darwin", "machine": "aarch64", "kernel": "unapproved",
                 "swap_bytes": 1, "allocation_limit_bytes": 17 * 1024**3,
                 "isolated": [], "turbo": {}, "affinity": [1], "cpus": {}}
        for key, value in cases.items():
            changed = copy.deepcopy(snapshot)
            changed[key] = value
            with self.subTest(key=key):
                self.assertFalse(engine.qualify_host(changed, pins)["qualified"])
        for key, value in (("governor", "powersave"), ("max_khz", 3000000), ("caches", [])):
            changed = copy.deepcopy(snapshot)
            changed["cpus"]["0"][key] = value
            self.assertFalse(engine.qualify_host(changed, pins)["qualified"])
        for key in pins:
            changed = copy.deepcopy(pins)
            del changed[key]
            self.assertFalse(engine.qualify_host(snapshot, changed)["qualified"], key)

    def test_probe_missing_files_does_not_qualify(self):
        with tempfile.TemporaryDirectory() as directory:
            snapshot = engine.collect_host(Path(directory), system="Linux", machine="x86_64", affinity=[0])
        _, pins = host_fixture()
        self.assertFalse(engine.qualify_host(snapshot, pins)["qualified"])

    def test_synthetic_statistics_and_artifact_reproducibility(self):
        blocks = blocks_fixture()
        with patch.object(engine.time, "perf_counter_ns", side_effect=AssertionError("clock forbidden")):
            result = engine.summarize(blocks)
            self.assertEqual(result["decision"]["classification"], "ACCEPT")
            self.assertEqual(result["ci_life"]["B729-u5"], (1.2, 1.2, 1.2))
            snapshot, pins = host_fixture()
            qualification = engine.qualify_host(snapshot, pins)
            first = engine.make_artifact(engine.INTEGRATION_SHA, {"test": True}, qualification, blocks, 73,
                                         synthetic=True)
            second = engine.make_artifact(engine.INTEGRATION_SHA, {"test": True}, qualification, blocks, 73,
                                          synthetic=True)
            self.assertEqual(first, second)
            self.assertEqual(first[1], hashlib.sha256(first[0].encode("ascii")).hexdigest())
            self.assertEqual(json.loads(first[0])["evidence"], "SYNTHETIC")
            self.assertTrue(json.loads(first[0])["execution_frozen"])

    def test_invalid_precedes_noise_and_noise_precedes_rejection(self):
        blocks = blocks_fixture()
        # A noisy arm and a clearly subthreshold control: noise wins.
        for i, block in enumerate(blocks):
            t = 50 if i % 2 else 150
            block["arms"]["T243"]["phase_ns"] = dict.fromkeys(engine.PHASES, t)
            block["arms"]["T243"]["lifecycle_ns"] = 7 * t
            block["arms"]["T243"]["query_ns"] = t
        self.assertEqual(engine.summarize(blocks)["decision"]["classification"], "INCONCLUSIVE")
        blocks[-1]["arms"]["B729"]["valid"] = False
        self.assertEqual(engine.summarize(blocks)["decision"]["classification"], "INVALID")

    def test_query_estimand_uses_medians_of_throughput(self):
        blocks = blocks_fixture()
        for i, block in enumerate(blocks):
            for arm, trial in block["arms"].items():
                duration = (100 if i < 15 else 102) if arm == "T243" else (120 if i < 15 else 124)
                trial["phase_ns"]["random_access"] = duration
                trial["query_ns"] = duration
                trial["lifecycle_ns"] = sum(trial["phase_ns"][p] for p in engine.PHASES[:7])
        result = engine.summarize(blocks)
        expected = ((engine.WORKLOAD_N / 100 + engine.WORKLOAD_N / 102) / 2
                    / ((engine.WORKLOAD_N / 120 + engine.WORKLOAD_N / 124) / 2))
        self.assertEqual(result["ci_query"]["B10"][0], expected)
        self.assertNotEqual(result["ci_query"]["B10"][0], 122 / 101)

    def test_incomplete_or_bad_evidence_never_rejects_or_accepts(self):
        blocks = blocks_fixture()
        self.assertEqual(engine.summarize(blocks[:29])["decision"]["classification"], "INVALID")
        for field, value in (("phase_ns", {}), ("lifecycle_ns", 1), ("resources", []),
                             ("checksum", "wrong"), ("query_ns", float("nan"))):
            changed = copy.deepcopy(blocks)
            changed[0]["arms"]["T243"][field] = value
            self.assertEqual(engine.summarize(changed)["decision"]["classification"], "INVALID", field)
        changed = copy.deepcopy(blocks)
        changed[1]["arms"]["T243"]["process_identity"] = changed[0]["arms"]["T243"]["process_identity"]
        self.assertEqual(engine.summarize(changed)["decision"]["classification"], "INVALID")

    def test_bootstrap_validation_and_decision_boundaries(self):
        for x, y in (([0], [1]), ([1], [float("nan")]), ([float("inf")], [1])):
            with self.assertRaises(ValueError):
                compute_bootstrap_ci(x, y)
        with self.assertRaises(ValueError):
            compute_bootstrap_ci([1], [1], n_boot=0)
        cvs = dict.fromkeys(engine.ARMS, .05)
        life = dict.fromkeys(engine.ARMS[1:], (1.1, 1.1, 1.1))
        query = dict.fromkeys(engine.ARMS[1:], (.95, .95, .95))
        self.assertEqual(evaluate_decision_rules(cvs, life, query)["classification"], "ACCEPT")
        for control in engine.ARMS[1:]:
            changed = dict(life)
            changed[control] = (1.04, 1.01, 1.07)
            self.assertEqual(evaluate_decision_rules(cvs, changed, query)["classification"], "REJECT")
            changed[control] = (1.1, 1.05, 1.15)
            self.assertEqual(evaluate_decision_rules(cvs, changed, query)["classification"], "INCONCLUSIVE")
        self.assertEqual(evaluate_decision_rules(cvs, {}, {})["classification"], "INVALID")


if __name__ == "__main__":
    unittest.main()
