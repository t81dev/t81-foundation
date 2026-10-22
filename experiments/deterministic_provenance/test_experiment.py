"""Positive, boundary and adversarial tests for the admitted evidence protocol."""
import argparse
import copy
from pathlib import Path
import subprocess
import tempfile
import unittest

from run import CASES, execute
from verify import canonical, digest, load, verify

parser = argparse.ArgumentParser()
parser.add_argument("--runner", type=Path, required=True)
ARGS, remaining = parser.parse_known_args()


class Experiment(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.counter = 0

    def produce(self, x=40, divisor=2, profile="allow"):
        self.counter += 1
        store = self.root / f"store-{self.counter}"
        envelope, raw = execute(ARGS.runner.resolve(), store, x, divisor, profile)
        return envelope, store

    def test_repeated_execution_and_input_sensitivity(self):
        for x, divisor, profile in CASES.values():
            with self.subTest(x=x, divisor=divisor, profile=profile):
                first, _ = self.produce(x, divisor, profile)
                for _ in range(2):
                    repeat, _ = self.produce(x, divisor, profile)
                    self.assertEqual(canonical(first), canonical(repeat))
        a, _ = self.produce()
        b, _ = self.produce(41)
        self.assertNotEqual(a["record_hash"], b["record_hash"])
        self.assertNotEqual(a["record"]["output"], b["record"]["output"])
        # Same branch sequence legitimately has the same trace hash.
        self.assertEqual(a["record"]["trace_hash"], b["record"]["trace_hash"])

    def test_denials_before_side_effect(self):
        denied, store = self.produce(profile="deny-vm")
        self.assertEqual(denied["record"]["output"], "0\n")
        self.assertNotIn("10 Store", denied["record"]["trace"])
        self.assertFalse(store.exists())
        denied, store = self.produce(profile="deny-store")
        self.assertEqual(denied["record"]["output"], "63\n")
        self.assertEqual(list(store.rglob("*.blk")), [])
        self.assertEqual(denied["record"]["reference"], "")

    def test_rehashed_tampering(self):
        envelope, _ = self.produce()
        mutations = {
            "output": "64\n", "trace": envelope["record"]["trace"].replace("0 LoadImm", "1 LoadImm", 1),
            "reference": "invalid", "vm_policy": "(policy)\n", "storage_policy": "(policy)\n",
            "decisions": "Store 0 allowed\n", "program": envelope["record"]["program"] + "Nop 0 0 0 0\n",
            "input": "41 2\n", "trap": "SecurityFault", "float_conversions": "NaE\n",
            "integer_boundaries": "0\n", "profile": "baseline", "version": True,
        }
        for field, value in mutations.items():
            with self.subTest(field=field):
                attacked = copy.deepcopy(envelope)
                attacked["record"][field] = value
                if field + "_hash" in attacked["record"]:
                    attacked["record"][field + "_hash"] = digest(value)
                attacked["record_hash"] = digest(canonical(attacked["record"]))
                with self.assertRaises(ValueError):
                    verify(attacked, 40, 2, "allow")
        attacked = copy.deepcopy(envelope)
        attacked["record"]["output"] = "64\n"
        with self.assertRaises(ValueError):
            verify(attacked, 40, 2, "allow")

    def test_trace_order_and_policy_pin(self):
        envelope, _ = self.produce()
        attacked = copy.deepcopy(envelope)
        lines = attacked["record"]["trace"].splitlines(keepends=True)
        lines[0], lines[1] = lines[1], lines[0]
        attacked["record"]["trace"] = "".join(lines)
        attacked["record"]["trace_hash"] = digest(attacked["record"]["trace"])
        attacked["record_hash"] = digest(canonical(attacked["record"]))
        with self.assertRaises(ValueError):
            verify(attacked, 40, 2, "allow")
        for x, d, profile in ((41, 2, "allow"), (40, 3, "allow"), (40, 2, "deny-store")):
            with self.assertRaises(ValueError):
                verify(envelope, x, d, profile)

    def test_actual_storage_tampering(self):
        envelope, store = self.produce()
        payload = next(store.rglob("*.blk"))
        payload.write_bytes(b"64\n")
        with self.assertRaises(ValueError):
            verify(envelope, 40, 2, "allow", store)
        payload.unlink()
        with self.assertRaises(ValueError):
            verify(envelope, 40, 2, "allow", store)
        denied, store = self.produce(profile="deny-store")
        (store / "unauthorized.blk").write_bytes(b"63\n")
        with self.assertRaises(ValueError):
            verify(denied, 40, 2, "deny-store", store)

    def test_malformed_input_and_evidence(self):
        for x, d, profile in (("-1", "2", "allow"), ("101", "2", "allow"), ("040", "2", "allow"),
                              ("40", "4", "allow"), ("40", "2", "unknown")):
            run = subprocess.run([str(ARGS.runner.resolve()), x, d, profile, str(self.root / "invalid")],
                                 capture_output=True, timeout=30)
            self.assertEqual(run.returncode, 2)
            self.assertFalse((self.root / "invalid").exists())
        path = self.root / "duplicate.json"
        path.write_text('{"record":{},"record":{}}')
        with self.assertRaises(ValueError):
            load(path)
        envelope, _ = self.produce()
        for key in envelope["record"]:
            attacked = copy.deepcopy(envelope)
            del attacked["record"][key]
            attacked["record_hash"] = digest(canonical(attacked["record"]))
            with self.assertRaises(ValueError):
                verify(attacked, 40, 2, "allow")

    def test_unsupported_conversion_is_explicit(self):
        self.counter += 1
        envelope, raw = execute(ARGS.runner.resolve(), self.root / f"diagnostic-{self.counter}", 40, 2, "allow")
        self.assertEqual(envelope["record"]["version"], 2)
        self.assertNotIn("float_conversions", envelope["record"])
        self.assertEqual(raw["diagnostics"]["status"], "UNSUPPORTED_NONCANONICAL")
        self.assertEqual(len(raw["diagnostics"]["float_conversions"].splitlines()), 9)
        historical = load(Path(__file__).parent / "fixture.json")
        with self.assertRaisesRegex(ValueError, "unsupported evidence version"):
            verify(historical, 40, 2, "allow")

    def test_checked_in_fixture(self):
        fixture = Path(__file__).parent / "fixture-v2.json"
        envelope = load(fixture)
        verify(envelope, 40, 2, "allow")
        actual, _ = self.produce()
        self.assertEqual(canonical(actual), canonical(envelope))


if __name__ == "__main__":
    unittest.main(argv=[__file__] + remaining)
