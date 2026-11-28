#!/usr/bin/env python3
"""Automated tests for the RFC-00D4 two-agent offline receipt handoff."""

from __future__ import annotations

import json
from pathlib import Path
import shutil
import sys
import tempfile
import unittest

HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))

import handoff_demo  # noqa: E402


class AgentReceiptHandoffTest(unittest.TestCase):
    def test_valid_handoff_and_fail_closed_rejections(self) -> None:
        with tempfile.TemporaryDirectory(prefix="t81-handoff-test-") as td:
            root = Path(td)
            good_dir = root / "good"
            env = handoff_demo.agent_a_produce_handoff(good_dir)

            # 1. Valid signed handoff -> PASS
            ok = handoff_demo.agent_b_verify_handoff(
                good_dir, expected_policy_sha3_512=env["expected_policy_sha3_512"]
            )
            self.assertTrue(ok["ok"], ok)
            self.assertEqual(ok["code"], "HANDOFF_VERIFIED")

            # 2. Missing signature -> FAIL-CLOSED
            missing_sig_dir = root / "missing_sig"
            shutil.copytree(good_dir, missing_sig_dir)
            raw_path = missing_sig_dir / "forensic_bundle" / "receipt_raw.json"
            raw_obj = json.loads(raw_path.read_text(encoding="utf-8"))
            raw_obj["ed25519_signature_b64"] = ""
            raw_path.write_text(json.dumps(raw_obj, separators=(",", ":")), encoding="utf-8")
            r_miss = handoff_demo.agent_b_verify_handoff(
                missing_sig_dir, expected_policy_sha3_512=env["expected_policy_sha3_512"]
            )
            self.assertFalse(r_miss["ok"], r_miss)
            self.assertEqual(r_miss["code"], "ERR_MISSING_SIGNATURE")

            # 3. Tampered payload (manifest re-hashed to isolate Ed25519 signature check) -> FAIL-CLOSED
            tamper_sig_dir = root / "tamper_sig"
            shutil.copytree(good_dir, tamper_sig_dir)
            t_raw = tamper_sig_dir / "forensic_bundle" / "receipt_raw.json"
            t_text = t_raw.read_text(encoding="utf-8").replace('"agent-a"', '"agent-X"', 1)
            t_raw.write_text(t_text, encoding="utf-8")
            # First verify manifest check catches raw tamper:
            r_man = handoff_demo.agent_b_verify_handoff(
                tamper_sig_dir, expected_policy_sha3_512=env["expected_policy_sha3_512"]
            )
            self.assertFalse(r_man["ok"], r_man)
            self.assertEqual(r_man["code"], "ERR_MANIFEST_DIGEST_MISMATCH")
            # Now rewrite manifest.sha3-512 with attacker's tampered hash to prove Ed25519 still fails closed:
            b_bytes = (tamper_sig_dir / "forensic_bundle" / "bundle.json").read_bytes()
            r_bytes = t_raw.read_bytes()
            (tamper_sig_dir / "forensic_bundle" / "manifest.sha3-512").write_text(
                f"{handoff_demo._sha3_512(b_bytes)}  bundle.json\n"
                f"{handoff_demo._sha3_512(r_bytes)}  receipt_raw.json\n",
                encoding="utf-8",
            )
            r_ed = handoff_demo.agent_b_verify_handoff(
                tamper_sig_dir, expected_policy_sha3_512=env["expected_policy_sha3_512"]
            )
            self.assertFalse(r_ed["ok"], r_ed)
            self.assertEqual(r_ed["code"], "ERR_RECEIPT_VERIFY_FAILED")

            # 4. Mismatched policy digest -> FAIL-CLOSED
            r_pol = handoff_demo.agent_b_verify_handoff(
                good_dir, expected_policy_sha3_512="sha3-512:" + ("f" * 128)
            )
            self.assertFalse(r_pol["ok"], r_pol)
            self.assertEqual(r_pol["code"], "ERR_POLICY_DIGEST_MISMATCH")


if __name__ == "__main__":
    unittest.main()
