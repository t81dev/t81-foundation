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

        # WITHOUT DB: Offline verification fails valid claim and reports DETACHED_OFFLINE_SIGNATURE_ONLY
        offline_ver = verify_evidence(evidence_file, db_path=None)
        self.assertFalse(offline_ver["valid"])
        self.assertEqual(offline_ver["reason"], "DETACHED_OFFLINE_SIGNATURE_ONLY")
        self.assertTrue(offline_ver["crypto_authentic"])
        self.assertFalse(offline_ver["witness_committed"])

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
        self.assertTrue(ver["crypto_authentic"])
        self.assertTrue(ver["authorization_valid"])
        self.assertTrue(ver["witness_committed"])
        self.assertFalse(ver["effect_observed"])

    # --- NEW REGRESSION TESTS ---

    def test_stored_limits_lower_than_caller_limits(self):
        """Stored policy limits (e.g., max_unit_price=1000) lower than caller limits (e.g. 5000) are enforced."""
        # Unit price 2000 is within caller limit (5000) but exceeds stored policy limit (1000)
        po_data = {"item": "Widget", "quantity": 1, "unit_price": 2000, "currency": "USD"}
        caller_limits = {"max_unit_price": 5000, "max_total_amount": 100000}
        success, res, status = submit_transaction(
            self.db_path, "tx-stored-lower", "v2", po_data, contract_limits=caller_limits
        )
        self.assertFalse(success)
        self.assertEqual(status, "POLICY_LIMIT_EXCEEDED")

    def test_caller_limits_lower_than_stored_limits(self):
        """Caller limits (e.g., max_unit_price=500) lower than stored limits (1000) further restrict execution."""
        po_data = {"item": "Widget", "quantity": 1, "unit_price": 800, "currency": "USD"}
        caller_limits = {"max_unit_price": 500, "max_total_amount": 50000}
        success, res, status = submit_transaction(
            self.db_path, "tx-caller-lower", "v2", po_data, contract_limits=caller_limits
        )
        self.assertFalse(success)
        self.assertEqual(status, "POLICY_LIMIT_EXCEEDED")

    def test_missing_or_inaccessible_witness_database(self):
        """Passing an explicitly provided non-existent database path to verify_evidence fails."""
        ev = execute_runner("A", "tx-missing-db", "v2", 5, 100, self.db_path, self.out_dir)
        self.assertTrue(ev["success"])
        evidence_file = self.out_dir / "evidence_tx-missing-db.json"

        non_existent_db = self.test_dir / "does_not_exist.db"
        ver = verify_evidence(evidence_file, db_path=non_existent_db)
        self.assertFalse(ver["valid"])
        self.assertEqual(ver["reason"], "MISSING_WITNESS_DATABASE")

    def test_detached_offline_signature_verification(self):
        """Detached verification without DB returns crypto_authentic=True, valid=False, reason=DETACHED_OFFLINE_SIGNATURE_ONLY."""
        ev = execute_runner("A", "tx-offline-sig", "v2", 5, 100, self.db_path, self.out_dir)
        self.assertTrue(ev["success"])
        evidence_file = self.out_dir / "evidence_tx-offline-sig.json"

        ver = verify_evidence(evidence_file, db_path=None)
        self.assertFalse(ver["valid"])
        self.assertEqual(ver["reason"], "DETACHED_OFFLINE_SIGNATURE_ONLY")
        self.assertTrue(ver["crypto_authentic"])
        self.assertFalse(ver["witness_committed"])

    def test_duplicate_transaction_ids(self):
        """Re-submitting identical transaction ID is rejected with REPLAY_DETECTED."""
        ev1 = execute_runner("A", "tx-dup-id", "v2", 5, 100, self.db_path, self.out_dir)
        self.assertTrue(ev1["success"])

        ev2 = execute_runner("A", "tx-dup-id", "v2", 5, 100, self.db_path, self.out_dir)
        self.assertFalse(ev2["success"])
        self.assertEqual(ev2["status_code"], "REPLAY_DETECTED")

    def test_revoked_authority(self):
        """Submitting transaction under a revoked authority version fails."""
        revoke_authority(self.db_path, "v2")
        ev = execute_runner("A", "tx-rev-test", "v2", 5, 100, self.db_path, self.out_dir)
        self.assertFalse(ev["success"])
        self.assertEqual(ev["status_code"], "REVOCATION_DETECTED")

    def test_invalid_monetary_inputs(self):
        """Monetary/quantity inputs that are negative, zero, boolean, or float are rejected."""
        po_negative = {"item": "Widget", "quantity": -5, "unit_price": 100, "currency": "USD"}
        succ1, res1, stat1 = submit_transaction(self.db_path, "tx-invalid-1", "v2", po_negative)
        self.assertFalse(succ1)
        self.assertEqual(stat1, "INVALID_TRANSACTION_INPUT")

        po_bool = {"item": "Widget", "quantity": True, "unit_price": 100, "currency": "USD"}
        succ2, res2, stat2 = submit_transaction(self.db_path, "tx-invalid-2", "v2", po_bool)
        self.assertFalse(succ2)
        self.assertEqual(stat2, "INVALID_TRANSACTION_INPUT")

        po_float = {"item": "Widget", "quantity": 10, "unit_price": 99.9, "currency": "USD"}
        succ3, res3, stat3 = submit_transaction(self.db_path, "tx-invalid-3", "v2", po_float)
        self.assertFalse(succ3)
        self.assertEqual(stat3, "INVALID_TRANSACTION_INPUT")

    def test_receipt_and_authority_version_mismatches(self):
        """Detect receipt hash and authority version discrepancies between evidence and store."""
        ev = execute_runner("A", "tx-mismatch", "v2", 5, 100, self.db_path, self.out_dir)
        self.assertTrue(ev["success"])
        evidence_file = self.out_dir / "evidence_tx-mismatch.json"

        # Tamper receipt authority version in evidence file
        with open(evidence_file, "r") as f:
            data = json.load(f)
        data["receipt"]["authority_version"] = "v1"
        # Re-compute receipt signature to make cryptographic HMAC valid for v1
        import hashlib
        from witness import compute_receipt_signature, DEFAULT_SECRET_KEY
        data["receipt"]["receipt_id"] = compute_receipt_signature(
            secret_key=DEFAULT_SECRET_KEY,
            tx_id=data["receipt"]["tx_id"],
            authority_version="v1",
            effect_id=data["receipt"]["effect_id"],
            payload_hash=data["receipt"]["payload_hash"],
            total_amount=data["receipt"]["total_amount"],
            timestamp=data["receipt"]["timestamp"],
            witness_id=data["receipt"]["witness_id"]
        )
        with open(evidence_file, "w") as f:
            json.dump(data, f)

        # Database contains authority_version 'v2', evidence contains 'v1'
        ver = verify_evidence(evidence_file, self.db_path)
        self.assertFalse(ver["valid"])
        self.assertEqual(ver["reason"], "AUTHORITY_VERSION_MISMATCH_IN_WITNESS_STORE")

    def test_historical_transaction_verified_post_revocation(self):
        """Transaction legitimately authorized and committed before revocation remains historically valid."""
        ev = execute_runner("A", "tx-hist-01", "v2", 5, 100, self.db_path, self.out_dir)
        self.assertTrue(ev["success"])

        # Revoke authority version v2 AFTER tx-hist-01 was committed
        revoke_authority(self.db_path, "v2")

        evidence_file = self.out_dir / "evidence_tx-hist-01.json"
        ver = verify_evidence(evidence_file, self.db_path)

        # Historical transaction evidence remains valid; current status is disaggregated
        self.assertTrue(ver["valid"])
        self.assertTrue(ver["witness_committed"])
        self.assertTrue(ver["authorization_valid"])
        self.assertFalse(ver["current_authority_active"])

    def test_guest_execution_failure_prevents_submission(self):
        """Failed guest execution results in GUEST_EXECUTION_FAILED and does NOT submit a valid receipt."""
        bad_wat = self.test_dir / "non_existent.wat"
        ev = execute_runner(
            arm="B",
            tx_id="tx-failed-guest",
            authority_version="v2",
            quantity=5,
            unit_price=100,
            db_path=self.db_path,
            output_dir=self.out_dir,
            wat_file=bad_wat
        )
        self.assertFalse(ev["success"])
        self.assertEqual(ev["status_code"], "GUEST_EXECUTION_FAILED")
        self.assertIsNone(ev["receipt"])

        evidence_file = self.out_dir / "evidence_tx-failed-guest.json"
        ver = verify_evidence(evidence_file, self.db_path)
        self.assertFalse(ver["valid"])

    def test_malformed_caller_contract_limits_rejected(self):
        """Malformed or non-positive caller limits are rejected with INVALID_TRANSACTION_INPUT."""
        po = {"item": "Widget", "quantity": 1, "unit_price": 100, "currency": "USD"}

        # Non-dict contract_limits
        s1, r1, st1 = submit_transaction(self.db_path, "tx-mal-1", "v2", po, contract_limits="500")
        self.assertFalse(s1)
        self.assertEqual(st1, "INVALID_TRANSACTION_INPUT")

        # String max_unit_price
        s2, r2, st2 = submit_transaction(self.db_path, "tx-mal-2", "v2", po, contract_limits={"max_unit_price": "500"})
        self.assertFalse(s2)
        self.assertEqual(st2, "INVALID_TRANSACTION_INPUT")

        # Negative max_unit_price
        s3, r3, st3 = submit_transaction(self.db_path, "tx-mal-3", "v2", po, contract_limits={"max_unit_price": -500})
        self.assertFalse(s3)
        self.assertEqual(st3, "INVALID_TRANSACTION_INPUT")

        # Boolean max_unit_price
        s4, r4, st4 = submit_transaction(self.db_path, "tx-mal-4", "v2", po, contract_limits={"max_unit_price": True})
        self.assertFalse(s4)
        self.assertEqual(st4, "INVALID_TRANSACTION_INPUT")

    def test_database_rollback_documented_limitation(self):
        """Database snapshot rollback allows reusing transaction IDs; documented as an unresolved limitation."""
        snapshot_path = self.test_dir / "witness_snapshot.db"
        shutil.copyfile(self.db_path, snapshot_path)

        ev1 = execute_runner("A", "tx-rollback-limit", "v2", 5, 100, self.db_path, self.out_dir)
        self.assertTrue(ev1["success"])

        # Remove active DB file and WAL/SHM sidecars to simulate restoring older snapshot
        for p in self.test_dir.glob("witness.db*"):
            p.unlink()
        shutil.copyfile(snapshot_path, self.db_path)

        # Re-submission succeeds because database state was rolled back (unresolved local limitation)
        ev2 = execute_runner("A", "tx-rollback-limit", "v2", 5, 100, self.db_path, self.out_dir)
        self.assertTrue(ev2["success"], "Confirmed limitation: local SQLite DB alone cannot detect external snapshot rollback")

if __name__ == "__main__":
    unittest.main()
