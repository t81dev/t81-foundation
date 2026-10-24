#!/usr/bin/env python3
import unittest
import tempfile
import shutil
import json
from pathlib import Path

from witness import init_db, submit_transaction, revoke_authority
from runner import execute_runner
from verify import verify_evidence

class TestDurableWitness(unittest.TestCase):
    def setUp(self):
        self.test_dir = Path(tempfile.mkdtemp())
        self.db_path = self.test_dir / "witness.db"
        self.out_dir = self.test_dir / "out"
        init_db(self.db_path)

    def tearDown(self):
        shutil.rmtree(self.test_dir)

    def test_successful_transaction_arm_a(self):
        evidence = execute_runner(
            arm="A",
            tx_id="tx-001",
            authority_version="v2",
            quantity=5,
            unit_price=100,
            db_path=self.db_path,
            output_dir=self.out_dir
        )
        self.assertTrue(evidence["success"])
        self.assertEqual(evidence["status_code"], "SUCCESS")

        evidence_file = self.out_dir / "evidence_tx-001.json"
        ver = verify_evidence(evidence_file, self.db_path)
        self.assertTrue(ver["valid"])

    def test_successful_transaction_arm_b(self):
        evidence = execute_runner(
            arm="B",
            tx_id="tx-002",
            authority_version="v2",
            quantity=5,
            unit_price=100,
            db_path=self.db_path,
            output_dir=self.out_dir
        )
        self.assertTrue(evidence["success"])
        self.assertEqual(evidence["status_code"], "SUCCESS")

        evidence_file = self.out_dir / "evidence_tx-002.json"
        ver = verify_evidence(evidence_file, self.db_path)
        self.assertTrue(ver["valid"])

    def test_replay_rejected_across_restart(self):
        # First execution
        ev1 = execute_runner("A", "tx-replay", "v2", 5, 100, self.db_path, self.out_dir)
        self.assertTrue(ev1["success"])

        # Second execution with same tx_id (simulating process restart)
        ev2 = execute_runner("A", "tx-replay", "v2", 5, 100, self.db_path, self.out_dir)
        self.assertFalse(ev2["success"])
        self.assertEqual(ev2["status_code"], "REPLAY_DETECTED")

    def test_revocation_at_boundary(self):
        # Revoke v2
        revoke_authority(self.db_path, "v2")

        # Submission under revoked version v2
        ev = execute_runner("A", "tx-revoked", "v2", 5, 100, self.db_path, self.out_dir)
        self.assertFalse(ev["success"])
        self.assertEqual(ev["status_code"], "REVOCATION_DETECTED")

if __name__ == "__main__":
    unittest.main()
