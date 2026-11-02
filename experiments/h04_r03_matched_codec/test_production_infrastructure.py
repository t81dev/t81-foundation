"""Synthetic correctness/fault tests. No experiment timing or codec benchmark."""
import copy
import hashlib
import io
import json
import os
from pathlib import Path
import platform
import subprocess
import tempfile
import unittest
from unittest.mock import patch
import uuid

from experiments.h04_r03_matched_codec import checksum, checksum_witness as witness
from experiments.h04_r03_matched_codec import enforcement, process_control as pc, retention
from experiments.h04_r03_matched_codec import measurement_engine as engine
from experiments.h04_r03_matched_codec.workload import generate_dataset_symbols, CANONICAL_DATASET_SHA256
from experiments.h04_r03_matched_codec.test_measurement_engine import fixture, instrument, blocks_fixture, host_fixture


BASE = "d8e1376a7a77902cc26269c6e8d4991bcde117a7"


def cgroup_files(path):
    raw = {"memory.max": str(engine.RSS_LIMIT), "memory.swap.max": "0",
           "memory.current": "0", "memory.peak": "0", "memory.oom.group": "1",
           "pids.max": "1", "cgroup.procs": "", "cgroup.events": "populated 0\nfrozen 0\n",
           "memory.events.local": "max 0\noom 0\noom_kill 0\noom_group_kill 0\n"}
    for name, value in raw.items():
        (path / name).write_text(value)


class FakeSpawner:
    def __init__(self, fail_first=False):
        self.calls, self.fail_first = 0, fail_first

    def __call__(self, argv, **options):
        self.calls += 1
        if self.fail_first and self.calls == 1:
            raise OSError("synthetic spawn failure")
        pid = 10000 + self.calls
        class Process:
            def __init__(self):
                self.pid, self.returncode, self.stdin = pid, None, io.BytesIO()
            def communicate(self, request, timeout):
                message = json.loads(request)
                if message["action"] != "ok":
                    raise AssertionError("fake process accepts only synthetic success")
                result = {"evidence": "SYNTHETIC", "nonce": message["nonce"],
                          "pid": self.pid, "status": "synthetic-only-complete"}
                options["stdout"].write((json.dumps(result, sort_keys=True) + "\n").encode())
                self.returncode = 0
            def poll(self):
                return self.returncode
            def wait(self):
                return self.returncode
        return Process()


def fake_identity(pid):
    return {"pid": pid, "boot_id": "00000000-0000-0000-0000-000000000001", "start_ticks": pid + 10}


def fake_plan(directory, fail_first=False):
    spawner = FakeSpawner(fail_first)
    with patch.object(pc.subprocess, "Popen", side_effect=spawner), patch.object(pc, "observe_identity", side_effect=fake_identity), patch.object(pc.uuid, "uuid4", side_effect=[uuid.UUID(int=i + 1) for i in range(120)]):
        result = pc.execute_synthetic_plan(pc.make_plan(73, BASE), directory)
    return result, spawner.calls


class ChecksumInfrastructureTests(unittest.TestCase):
    def test_known_answers_and_source_separation(self):
        backend = checksum.checked_backend()
        for data, seed, expected in checksum.KNOWN_ANSWERS:
            self.assertEqual(witness.reference_xxh64(data, seed), expected)
            self.assertEqual(backend.xxh64(data, seed=seed).intdigest(), expected)
        source = Path(witness.__file__).read_text()
        self.assertNotIn("from .workload", source)
        self.assertNotIn("import xxhash", source)

    def test_stripe_tail_and_streaming_equivalence(self):
        backend = checksum.checked_backend()
        for size in list(range(100)) + [127, 128, 129, 1023, 1024, 1025]:
            data = bytes((i * 19 + 7) % 256 for i in range(size))
            for seed in (0, 1, (1 << 64) - 1):
                expected = witness.reference_xxh64(data, seed)
                obj = backend.xxh64(seed=seed)
                for offset in range(0, len(data), 7):
                    obj.update(data[offset:offset + 7])
                self.assertEqual(obj.intdigest(), expected, (size, seed))
                self.assertEqual(backend.xxh64(data, seed=seed).intdigest(), expected)

    def test_full_witness_reproduction_no_codec_execution(self):
        # Untimed correctness preparation, not an experiment trial.
        record = checksum.witness_record()
        derived = witness.derive_witness()
        for key, value in derived.items():
            self.assertEqual(record[key], value, key)
        self.assertEqual(derived["dataset_sha256"], CANONICAL_DATASET_SHA256)
        obj = checksum.checksum_factory()()
        import struct
        for value in generate_dataset_symbols():
            obj.update(struct.pack("<q", value))
        self.assertEqual(obj.hexdigest(), record["xxh64"])
        self.assertEqual(record["xxh64"], "cb66754878debc38")

    def test_pins_reject_mismatch(self):
        with patch.object(checksum, "version", return_value="wrong"):
            with self.assertRaisesRegex(RuntimeError, "identity"):
                checksum.checked_backend()
        with patch.object(checksum, "KNOWN_ANSWERS", ((b"a", 0, 0),)):
            with self.assertRaisesRegex(RuntimeError, "known-answer"):
                checksum.checked_backend()
        info = checksum.backend_identity()
        self.assertEqual(info["seed"], 0)
        self.assertEqual(len(info["extension_sha256"]), 64)

    def test_lifecycle_uses_actual_xxh64_with_mock_clock(self):
        instance, values, _ = fixture(n=65)
        import struct
        expected = f"{witness.reference_xxh64(struct.pack('<65q', *values)):016x}"
        instance.checksum_factory = checksum.checksum_factory()
        instance.expected_checksum = expected
        result = instrument(instance)
        self.assertTrue(result["valid"], result)
        self.assertEqual(result["checksum"], expected)
        self.assertEqual(len(result["checksum"]), 16)


class EnforcementInfrastructureTests(unittest.TestCase):
    def test_kernel_snapshots_preserve_transient_and_oom_evidence(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            cgroup_files(root)
            before = enforcement.cgroup_snapshot(root)
            enforcement.validate_cgroup_controls(before, empty=True)
            (root / "memory.peak").write_text(str(engine.RSS_LIMIT + 4096))
            (root / "memory.events.local").write_text("max 1\noom 1\noom_kill 1\noom_group_kill 1\n")
            after = enforcement.cgroup_snapshot(root)
            self.assertEqual(after["accounted_current_bytes"], 0)
            self.assertGreater(after["accounted_peak_bytes"], engine.RSS_LIMIT)
            with self.assertRaises(engine.ProtocolError):
                enforcement.validate_cgroup_controls(after, empty=True)
            journal = pc.Journal(root / "evidence.jsonl")
            journal.append({"kind": "kernel_evidence", "before": before, "after": after,
                            "source": "synthetic-files-not-kernel"})
            journal.close()
            recovered = pc.recover_journal(root / "evidence.jsonl")
            self.assertTrue(recovered["integrity"])
            self.assertEqual(recovered["records"][0]["event"]["after"]["events"]["oom_kill"], 1)

    def test_invalid_kernel_controls(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            for field, value in (("memory.max", "max"), ("memory.swap.max", "1"),
                                 ("memory.oom.group", "0"), ("pids.max", "2"),
                                 ("cgroup.procs", "123"), ("memory.events.local", "max 0\n")):
                cgroup_files(root)
                (root / field).write_text(value)
                with self.subTest(field=field), self.assertRaises(engine.ProtocolError):
                    enforcement.validate_cgroup_controls(enforcement.cgroup_snapshot(root), empty=True)
            with patch.object(enforcement.platform, "system", return_value="Darwin"):
                with self.assertRaises(enforcement.UnresolvedEnforcement):
                    enforcement.KernelMemoryGuard(root)

    def test_scratch_transient_peak_is_observed_but_not_full_enforcement(self):
        monitor = enforcement.TracedScratch()
        monitor.begin()
        transient = bytearray(engine.SCRATCH_LIMIT + 4096)
        del transient
        result = monitor.finish()
        self.assertTrue(result["limit_exceeded"])
        self.assertGreater(result["traced_peak_bytes"], result["traced_current_bytes"])
        self.assertFalse(result["continuous_enforcement"])
        self.assertFalse(result["native_coverage_complete"])

    def test_reservation_refuses_before_allocation_and_retains_refusal(self):
        with tempfile.TemporaryDirectory() as name:
            path = Path(name) / "reservations.jsonl"
            journal = pc.Journal(path)
            budget = enforcement.ScratchReservations(journal)
            budget.reserve("a", engine.SCRATCH_LIMIT)
            with self.assertRaises(engine.ProtocolError):
                budget.reserve("transient", 1)
            self.assertEqual(budget.evidence()["reserved_peak_bytes"], engine.SCRATCH_LIMIT)
            budget.release("a")
            self.assertEqual(budget.evidence()["outstanding"], {})
            self.assertFalse(budget.evidence()["continuous_enforcement"])
            with self.assertRaises(engine.ProtocolError):
                budget.release("a")
            journal.close()
            events = [r["event"] for r in pc.recover_journal(path)["records"]]
            self.assertTrue(events[1]["refused"])
            self.assertEqual(events[1]["requested_live_bytes"], engine.SCRATCH_LIMIT + 1)


class ProcessInfrastructureTests(unittest.TestCase):
    def test_30_paired_blocks_exact_order_and_reproducible_receipts(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            first, calls = fake_plan(root / "first")
            second, _ = fake_plan(root / "second")
            self.assertEqual(calls, 120)
            self.assertTrue(pc.validate_process_recovery(first))
            self.assertTrue(pc.verify_attempt_files(root / "first", first))
            self.assertEqual((root / "first/attempts.jsonl").read_bytes(), (root / "second/attempts.jsonl").read_bytes())
            ends = [r["event"] for r in first["records"] if r["event"]["kind"] == "attempt_finished"]
            self.assertEqual(len(ends), 120)
            self.assertTrue(all(not e["measurement_eligible"] for e in ends))
            for i in range(30):
                self.assertEqual([e["slot"]["arm"] for e in ends[4*i:4*i+4]], engine.paired_orders(73)[i])

    def test_spawn_failure_is_retained_without_replacement_runs(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name) / "run"
            recovery, calls = fake_plan(root, fail_first=True)
            self.assertEqual(calls, 120)
            self.assertTrue(pc.validate_process_recovery(recovery))
            ends = [r["event"] for r in recovery["records"] if r["event"]["kind"] == "attempt_finished"]
            self.assertIn("spawn failure", ends[0]["failure"])
            self.assertIsNone(ends[0]["returncode"])
            self.assertFalse(ends[0]["reclamation"]["address_space_released"])
            self.assertEqual(recovery["records"][-1]["event"]["failures"], 1)

    @unittest.skipUnless(os.name == "posix", "POSIX process group tests")
    def test_actual_diagnostic_workers_and_killed_output_survive(self):
        # These workers never import codecs or call a clock/timing collector.
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            journal = pc.Journal(root / "events.jsonl")
            for position, action in enumerate(("ok", "nonzero", "kill", "malformed", "wait")):
                slot = {"block": position, "position": 0, "arm": "T243"}
                result = pc._synthetic_attempt(slot, root, journal, action, .2 if action == "wait" else 5)
                self.assertEqual(result["valid_synthetic_result"], action == "ok", result)
                self.assertFalse(result["measurement_eligible"])
                self.assertTrue(result["reclamation"]["process_reaped"])
                self.assertFalse(result["reclamation"]["physical_reclamation_verified"])
                if action == "kill":
                    self.assertLess(result["returncode"], 0)
                    self.assertGreater(result["files"]["stdout"]["size"], 0)
                if action == "wait":
                    self.assertIn("TimeoutExpired", result["failure"])
            journal.close()
            recovery = pc.recover_journal(root / "events.jsonl")
            self.assertTrue(recovery["integrity"])
            self.assertEqual(recovery["incomplete_attempts"], [])

    def test_incomplete_truncated_and_tampered_evidence(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            path = root / "events.jsonl"
            journal = pc.Journal(path)
            journal.append({"kind": "attempt_started", "attempt": "synthetic"})
            journal.close()
            original = path.read_bytes()
            recovery = pc.recover_journal(path)
            self.assertEqual(recovery["incomplete_attempts"], ["synthetic"])
            with path.open("ab") as stream:
                stream.write(b'{"partial":')
            recovery = pc.recover_journal(path)
            self.assertFalse(recovery["integrity"])
            self.assertTrue(recovery["unparsed_tail_base64"])
            self.assertEqual(len(recovery["records"]), 1)
            path.write_bytes(original.replace(b"synthetic", b"tampered"))
            self.assertFalse(pc.recover_journal(path)["integrity"])
            with self.assertRaises(FileExistsError):
                pc.Journal(path)

    def test_durability_failure_prevents_spawning_and_bad_json_is_retained(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name)
            with patch.object(pc.os, "fsync", side_effect=OSError("synthetic disk failure")), patch.object(pc.subprocess, "Popen") as spawn:
                with self.assertRaisesRegex(OSError, "disk failure"):
                    pc.execute_synthetic_plan(pc.make_plan(73, BASE), root / "run")
                spawn.assert_not_called()
            path = root / "bad.jsonl"
            path.write_text('{"event":[],"previous":"' + '0'*64 + '","sequence":0,"sha256":"bad"}\n')
            recovered = pc.recover_journal(path)
            self.assertFalse(recovered["integrity"])
            self.assertTrue(recovered["unparsed_tail_base64"])

    def test_artifact_and_semantic_tampering(self):
        with tempfile.TemporaryDirectory() as name:
            root = Path(name) / "run"
            recovery, _ = fake_plan(root)
            first = next(r["event"] for r in recovery["records"] if r["event"]["kind"] == "attempt_finished")
            (root / first["files"]["stdout"]["path"]).write_bytes(b"tampered")
            with self.assertRaises(engine.ProtocolError):
                pc.verify_attempt_files(root, recovery)
            altered = copy.deepcopy(recovery)
            # Semantics are rechecked even when caller claims hash integrity.
            altered["records"][-1]["event"]["attempts"] = 119
            with self.assertRaises(engine.ProtocolError):
                pc.validate_process_recovery(altered)
            altered = copy.deepcopy(recovery)
            event = next(r["event"] for r in altered["records"] if r["event"]["kind"] == "attempt_finished")
            event["files"] = {}
            with self.assertRaises(engine.ProtocolError):
                pc.validate_process_recovery(altered)

    def test_pid_reuse_and_parenthesized_comm(self):
        boot = "00000000-0000-0000-0000-000000000001"
        def stat(ticks):
            return "23 (worker ) (name) " + " ".join(["S"] + ["0"] * 18 + [str(ticks)] + ["0"] * 30)
        a = pc.linux_identity(23, stat(900), boot)
        b = pc.linux_identity(23, stat(901), boot)
        self.assertNotEqual(a, b)
        self.assertEqual(a["start_ticks"], 900)
        for text in ("broken", "23 (partial) S 0"):
            with self.assertRaises(engine.ProtocolError):
                pc.linux_identity(23, text, boot)

    def test_no_commands_or_freeze_bypass(self):
        plan = pc.make_plan(73, BASE)
        plan["configuration"]["q"] = 1
        with self.assertRaises(engine.ProtocolError):
            pc.validate_plan(plan)
        for entry in (pc.run_process_plan, engine.run_measurements):
            with self.assertRaisesRegex(engine.ProtocolError, "FROZEN"):
                entry(authorized=True, qualified=True, synthetic=True)
        instance, _, _ = fixture()
        with self.assertRaisesRegex(engine.ProtocolError, "FROZEN"):
            engine._instrument(instance)
        with tempfile.TemporaryDirectory() as name:
            root = Path(name) / "never-created"
            with self.assertRaises(engine.ProtocolError):
                pc.execute_synthetic_plan(pc.make_plan(73, BASE), root, actions={"00-0-T243": "benchmark"})
            self.assertFalse(root.exists())
        snapshot, pins = host_fixture()
        text, _ = engine.make_artifact(BASE, {}, engine.qualify_host(snapshot, pins), blocks_fixture(), 73)
        self.assertEqual(json.loads(text)["statistics"]["decision"]["classification"], "INVALID")
        with patch.object(engine, "summarize", side_effect=AssertionError("invalid evidence must not reach statistics")):
            engine.make_artifact(BASE, {}, {"qualified": True}, blocks_fixture(), 73)
            engine.make_artifact(BASE, {}, {"qualified": False}, blocks_fixture(), 73, synthetic=True)


class RetentionInfrastructureTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if platform.system() not in ("Linux", "Darwin"):
            raise unittest.SkipTest("native fence build tested only with POSIX C11 toolchains")
        cls.tmp = tempfile.TemporaryDirectory()
        cls.binary = Path(cls.tmp.name) / ("fence.dylib" if platform.system() == "Darwin" else "fence.so")
        source = Path(retention.__file__).with_name("retention_fence.c")
        flag = "-dynamiclib" if platform.system() == "Darwin" else "-shared"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-fPIC", flag,
                        str(source), "-o", str(cls.binary)], check=True, capture_output=True)
        cls.sha = hashlib.sha256(cls.binary.read_bytes()).hexdigest()

    @classmethod
    def tearDownClass(cls):
        if hasattr(cls, "tmp"):
            cls.tmp.cleanup()

    def test_alias_detection_and_ordering_limitations(self):
        raw = retention.OwnedBuffer(b"synthetic input")
        alias = raw
        barrier = retention.RetentionBarrier([raw], self.binary, self.sha)
        del raw
        with self.assertRaisesRegex(engine.ProtocolError, "aliases"):
            barrier.verify_release()
        del alias
        result = barrier.verify_release()
        self.assertTrue(result["ordering_fence_called"])
        self.assertEqual(result["live_owned_buffers"], 0)
        self.assertIsNone(result["allocator_pages_reclaimed"])
        self.assertFalse(result["physical_bytes_erased"])
        self.assertFalse(result["cache_flushed"])

    def test_lifecycle_barrier_and_binary_pin(self):
        instance, _, _ = fixture()
        instance.raw = retention.OwnedBuffer(instance.raw)
        instance.retention_factory = lambda owners: retention.RetentionBarrier(owners, self.binary, self.sha)
        result = instrument(instance)
        self.assertTrue(result["valid"], result)
        self.assertEqual(result["retention_evidence"]["live_owned_buffers"], 0)
        with self.assertRaises(engine.ProtocolError):
            retention.RetentionBarrier([], self.binary, "0" * 64)

    def test_reaping_does_not_claim_global_reclamation(self):
        result = retention.process_reclamation(-9, True, {"populated": 0, "pids": [], "accounted_current_bytes": 4096})
        self.assertTrue(result["address_space_released"])
        self.assertTrue(result["cgroup_empty"])
        self.assertFalse(result["physical_reclamation_verified"])
        self.assertEqual(result["accounted_current_bytes"], 4096)


if __name__ == "__main__":
    unittest.main()
