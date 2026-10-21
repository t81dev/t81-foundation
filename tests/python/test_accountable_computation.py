"""Exercise real VM/CanonFS/Axion paths and detached-verifier attacks."""
import argparse
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
EXAMPLE = ROOT / "examples/governance/accountable-computation"


def load(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


producer = load("producer", EXAMPLE / "run.py")
verifier = load("verifier", EXAMPLE / "verify.py")


class Experiment(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temp = tempfile.TemporaryDirectory()
        cls.output = Path(cls.temp.name) / "results"
        cls.summary = producer.run(RUNNER, cls.output, repetitions=2,
                                   arms=("A", "D") if PRODUCTION_ONLY else ("A", "B", "C"))
        # A second key lets negative tests re-sign logically invalid evidence.
        cls.key = Path(cls.temp.name) / "test-key.pem"
        cls.public = Path(cls.temp.name) / "test-public.pem"
        subprocess.run(["openssl", "genpkey", "-algorithm", "ED25519", "-out", str(cls.key)], check=True)
        subprocess.run(["openssl", "pkey", "-in", str(cls.key), "-pubout", "-out", str(cls.public)],
                       check=True, capture_output=True)

    @classmethod
    def tearDownClass(cls):
        cls.temp.cleanup()

    def test_actual_boundaries_and_replay(self):
        for row in self.summary["rows"]:
            with self.subTest(arm=row["arm"], case=row["case"]):
                # A failing research gate is an expected, recorded finding.
                known_failure = row["arm"] == "B" and row["case"] in ("invalid_policy", "denied", "revoked")
                self.assertEqual(row["result"], "fail" if known_failure else "pass")
                if row["arm"] != "A" and row["case"] in ("authorized", "denied", "revoked"):
                    self.assertGreater(row["observation"]["vm_policy_calls"], 0)
                    self.assertEqual(row["observation"]["policy_calls"], 1)
                if not known_failure and row["case"] in ("changed_bytes", "wrong_hash", "denied", "revoked"):
                    self.assertEqual(row["object_count"], 0)

    def test_detached_verifier_attacks(self):
        folder = self.output / ("D-authorized-0" if PRODUCTION_ONLY else "C-authorized-0")
        original = json.loads((folder / "receipt.json").read_text())
        def verify(envelope, public=None):
            receipt = Path(self.temp.name) / "attack.json"
            receipt.write_text(json.dumps(envelope))
            return verifier.verify(receipt, public or self.public, folder / "input", folder / "policy")
        record = original["record"]
        valid = producer.sign(record, self.key)
        self.assertEqual(verify(valid), record)
        for field in record:
            with self.subTest(missing=field):
                altered = copy.deepcopy(record)
                del altered[field]
                with self.assertRaises(ValueError):
                    verify(producer.sign(altered, self.key))
        for field, value in (("amount", 24999), ("output_digest", "0" * 64),
                             ("policy_digest", "0" * 64), ("status", "policy_denied")):
            with self.subTest(tampered=field):
                altered = copy.deepcopy(record)
                altered["observation"][field] = value
                with self.assertRaises(ValueError):
                    verify(producer.sign(altered, self.key))
        altered = copy.deepcopy(valid)
        altered["record"]["method"] = "tampered"
        with self.assertRaises(ValueError):
            verify(altered)
        altered = copy.deepcopy(valid)
        altered["record"]["case"] = "tampered"
        with self.assertRaises(ValueError):
            verify(altered)
        with self.assertRaises(ValueError):
            verify(valid, self.output / "public.pem")
        for mutation in ("objects", "input_hex", "policy_hex"):
            altered = copy.deepcopy(record)
            altered[mutation] = [] if mutation == "objects" else b"tampered".hex()
            with self.assertRaises(ValueError):
                verify(producer.sign(altered, self.key))
        for field, value in (("hex", b"24999\n".hex()), ("name", "incorrect.blk")):
            altered = copy.deepcopy(record)
            altered["objects"][0][field] = value
            with self.assertRaises(ValueError):
                verify(producer.sign(altered, self.key))
        altered = copy.deepcopy(record)
        del altered["observation"]["policy_digest"]
        with self.assertRaises(ValueError):
            verify(producer.sign(altered, self.key))
        altered = copy.deepcopy(record)
        altered["version"] = 1.0
        with self.assertRaises(ValueError):
            verify(producer.sign(altered, self.key))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--runner", required=True, type=Path)
    parser.add_argument("--production-only", action="store_true")
    args, rest = parser.parse_known_args()
    PRODUCTION_ONLY = args.production_only
    RUNNER = args.runner.resolve()
    unittest.main(argv=[sys.argv[0], *rest])
