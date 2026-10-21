"""Execute real storage schedules and challenge their detached evidence checker."""
import argparse
import copy
import importlib.util
import json
from pathlib import Path
import subprocess
import tempfile
import unittest

EXAMPLE = Path(__file__).resolve().parents[2] / "examples/governance/accountable-computation"
spec = importlib.util.spec_from_file_location("detached_concurrency", EXAMPLE / "verify_concurrent.py")
verifier = importlib.util.module_from_spec(spec)
spec.loader.exec_module(verifier)


class ConcurrentRevocation(unittest.TestCase):
    def test_schedules_and_storage(self):
        with tempfile.TemporaryDirectory() as temp:
            output = Path(temp) / "run"
            subprocess.run([str(RUNNER), str(output), "3"], check=True, timeout=90)
            summary = verifier.verify(output / "results.json", output)
            self.assertEqual(summary["cases"], 64)
            self.assertEqual(summary["operations"], 192)
            self.assertEqual(summary["stronger_goal_snapshot_counterexamples"], 24)
            self.assertEqual(summary["serialized_owner_prohibited_commits"], 0)
            self.assertFalse(summary["existing_serialized_contract_violation_demonstrated"])
            data = json.loads((output / "results.json").read_text())
            for change in ("trace", "effect", "case", "content"):
                bad = copy.deepcopy(data)
                row = next(r for r in bad["rows"] if r["backend"] == "t81-file" and
                           r["mode"] == "serialized-owner" and r["scenario"] == "after_authorization")
                if change == "trace":
                    a, b = row["trace"].index("write.commit"), row["trace"].index("revoke.complete")
                    row["trace"][a], row["trace"][b] = row["trace"][b], row["trace"][a]
                elif change == "effect":
                    row["files"] = {}
                elif change == "case":
                    bad["rows"].pop()
                else:
                    row["observed_hex"] = b"wrong".hex()
                attacked = Path(temp) / "attacked.json"
                attacked.write_text(json.dumps(bad))
                with self.subTest(change=change), self.assertRaises(ValueError):
                    verifier.verify(attacked)
            target = next(output.glob("t81-file-serialized-owner-no_revocation-0/objects/*.blk"))
            target.write_bytes(b"tampered")
            with self.assertRaises(ValueError):
                verifier.verify(output / "results.json", output)
            # Reusing a store must reject rather than hide effects in EEXIST paths.
            result = subprocess.run([str(RUNNER), str(output), "1"], capture_output=True)
            self.assertNotEqual(result.returncode, 0)


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--runner", required=True, type=Path)
    args, remaining = parser.parse_known_args()
    RUNNER = args.runner.resolve()
    unittest.main(argv=[__file__, *remaining])
