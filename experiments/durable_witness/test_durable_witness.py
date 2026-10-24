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

    def test_offline_verification_accepts_forged_receipt_without_witness_db(self):
        """Demonstrate required investigation 3:

        An offline verifier without a database connection cannot verify whether a
        transaction actually exists in the witness database or whether it was ever admitted.
        If the producer knows the secret key, offline HMAC verification succeeds even if the
        transaction was NEVER committed to the witness store (phantom evidence).
        """
        import hashlib, json
        from witness import compute_receipt_signature, DEFAULT_SECRET_KEY

        po_data = {"item": "Phantom-Widget", "quantity": 1, "unit_price": 500, "currency": "USD"}
        canonical_po_bytes = json.dumps(po_data, sort_keys=True).encode("utf-8")
        payload_hash = hashlib.sha256(canonical_po_bytes).hexdigest()

        tx_id = "tx-phantom-offline"
        authority_version = "v2"
        effect_id = "effect-phantom-1"
        timestamp = "2026-10-24T12:00:00Z"
        witness_id = "durable-witness-v1"
        total_amount = 500

        # Construct receipt with known secret key (producer forged or offline generator)
        receipt_id = compute_receipt_signature(
            secret_key=DEFAULT_SECRET_KEY,
            tx_id=tx_id,
            authority_version=authority_version,
            effect_id=effect_id,
            payload_hash=payload_hash,
            total_amount=total_amount,
            timestamp=timestamp,
            witness_id=witness_id
        )

        phantom_evidence = {
            "arm": "B",
            "tx_id": tx_id,
            "authority_version": authority_version,
            "po_data": po_data,
            "receipt": {
                "witness_id": witness_id,
                "tx_id": tx_id,
                "authority_version": authority_version,
                "effect_id": effect_id,
                "payload_hash": payload_hash,
                "total_amount": total_amount,
                "timestamp": timestamp,
                "receipt_id": receipt_id
            }
        }
        self.out_dir.mkdir(parents=True, exist_ok=True)
        evidence_file = self.out_dir / "evidence_phantom.json"
        with open(evidence_file, "w") as f:
            json.dump(phantom_evidence, f, indent=2)

        # WITHOUT DB: Offline verification claims valid!
        offline_ver = verify_evidence(evidence_file, db_path=None)
        self.assertTrue(offline_ver["valid"])
        self.assertFalse(offline_ver["witness_verified"])

        # WITH DB: Witness store verification detects phantom receipt!
        online_ver = verify_evidence(evidence_file, db_path=self.db_path)
        self.assertFalse(online_ver["valid"])
        self.assertEqual(online_ver["reason"], "TRANSACTION_NOT_IN_WITNESS_STORE")

    def test_database_record_vs_authenticated_effect_observation(self):
        """Demonstrate required investigation 5:

        Separate a database record stating that an effect occurred from an
        authenticated observation that the actual physical/external effect occurred.
        A database record alone only proves state transition inside the witness DB,
        not physical external world execution (e.g. shipping or payment disbursement).
        """
        # Execute transaction in DB
        ev = execute_runner("A", "tx-effect-obs", "v2", 10, 100, self.db_path, self.out_dir)
        self.assertTrue(ev["success"])

        # Check DB record
        ver = verify_evidence(self.out_dir / "evidence_tx-effect-obs.json", self.db_path)
        self.assertTrue(ver["valid"])
        self.assertTrue(ver["witness_verified"])

        # Define external observation check function
        def observe_external_world_effect(tx_id: str, external_audit_log: dict) -> bool:
            """An authenticated external observation requires a separate observation proof
            (e.g., physical hardware sensor attestation or external bank transfer receipt),
            not merely an internal SQLite table row.
            """
            return tx_id in external_audit_log.get("confirmed_external_disbursements", [])

        # Simulated external reality log
        external_world_log = {"confirmed_external_disbursements": []}

        # DB record exists, but external world effect observation is FALSE
        db_record_exists = ver["valid"]
        external_effect_observed = observe_external_world_effect("tx-effect-obs", external_world_log)

        self.assertTrue(db_record_exists, "DB record confirms state transition inside witness DB")
        self.assertFalse(external_effect_observed, "External world observation remains unverified without independent sensor/disbursement proof")

if __name__ == "__main__":
    unittest.main()
