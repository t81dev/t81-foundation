#!/usr/bin/env python3
import sys
import json
import sqlite3
import argparse
from pathlib import Path

import hashlib
import hmac
from witness import compute_receipt_signature

def verify_evidence(
    evidence_path: Path,
    db_path: Path = None,
    secret_key: bytes = None,
    external_audit_log: dict = None
) -> dict:
    """Verifies evidence and produces distinct verdicts for cryptographic authenticity,

    database presence, and externally observed effect occurrence.
    """
    if secret_key is None:
        return {
            "valid": False,
            "crypto_authentic": False,
            "database_present": False,
            "effect_observed": False,
            "reason": "MISSING_SECRET_KEY",
            "message": "A secret key must be supplied to verify cryptographic authenticity."
        }

    if not evidence_path.exists():
        return {
            "valid": False,
            "crypto_authentic": False,
            "database_present": False,
            "effect_observed": False,
            "reason": f"Evidence file {evidence_path} does not exist"
        }

    with open(evidence_path, "r", encoding="utf-8") as f:
        evidence = json.load(f)

    # 1. Inspect required fields
    receipt = evidence.get("receipt")
    po_data = evidence.get("po_data")

    if not receipt or not po_data:
        return {
            "valid": False,
            "crypto_authentic": False,
            "database_present": False,
            "effect_observed": False,
            "reason": "Missing receipt or po_data in evidence"
        }

    tx_id = receipt.get("tx_id")
    authority_version = receipt.get("authority_version")
    effect_id = receipt.get("effect_id")
    receipt_id = receipt.get("receipt_id")
    payload_hash = receipt.get("payload_hash")
    total_amount = receipt.get("total_amount", 0)
    timestamp = receipt.get("timestamp", "")
    witness_id = receipt.get("witness_id", "durable-witness-v1")

    # 2. Check receipt signature across all receipt fields
    expected_receipt_id = compute_receipt_signature(
        secret_key=secret_key,
        tx_id=tx_id,
        authority_version=authority_version,
        effect_id=effect_id,
        payload_hash=payload_hash,
        total_amount=total_amount,
        timestamp=timestamp,
        witness_id=witness_id
    )
    crypto_authentic = isinstance(receipt_id, str) and hmac.compare_digest(receipt_id, expected_receipt_id)

    # 3. Check payload hash matches PO data
    canonical_po_bytes = json.dumps(po_data, sort_keys=True).encode("utf-8")
    expected_payload_hash = hashlib.sha256(canonical_po_bytes).hexdigest()
    if payload_hash != expected_payload_hash:
        crypto_authentic = False

    if not crypto_authentic:
        reason = "INVALID_RECEIPT_SIGNATURE" if receipt_id != expected_receipt_id else "PAYLOAD_HASH_MISMATCH"
        return {
            "valid": False,
            "crypto_authentic": False,
            "database_present": False,
            "effect_observed": False,
            "reason": reason,
            "message": f"Cryptographic authenticity check failed ({reason})"
        }

    # Check the evidence's claimed computation against the authenticated amount.
    # Legacy/offline evidence may omit total_amount, but may not contradict it.
    q, p = po_data.get("quantity"), po_data.get("unit_price")
    if (type(q) is not int or type(p) is not int or type(total_amount) is not int
            or q * p != total_amount
            or ("total_amount" in evidence and
                (type(evidence["total_amount"]) is not int or evidence["total_amount"] != total_amount))):
        return {"valid": False, "crypto_authentic": True, "database_present": False,
                "effect_observed": False, "reason": "AMOUNT_BINDING_MISMATCH"}

    # 4. Inspect witness database state if provided
    database_present = False
    if db_path and db_path.exists():
        conn = sqlite3.connect(db_path)
        try:
            cur = conn.execute("SELECT authority_version, receipt_id FROM transactions WHERE tx_id = ?", (tx_id,))
            row = cur.fetchone()
        except sqlite3.OperationalError:
            row = None
        finally:
            conn.close()

        if row:
            db_auth_ver, db_receipt_id = row
            if db_receipt_id == receipt_id:
                database_present = True

    # 5. Check external audit log for external effect occurrence
    effect_observed = False
    if external_audit_log and isinstance(external_audit_log, dict):
        confirmed = external_audit_log.get("confirmed_external_disbursements", [])
        if tx_id in confirmed:
            effect_observed = True

    # Primary overall validity requires both cryptographic authenticity and witness DB presence when db_path is supplied
    is_valid = crypto_authentic and (database_present if db_path else True)
    reason = "SUCCESS"
    if not is_valid:
        if db_path and not database_present:
            reason = "TRANSACTION_NOT_IN_WITNESS_STORE"

    return {
        "valid": is_valid,
        "crypto_authentic": crypto_authentic,
        "database_present": database_present,
        "effect_observed": effect_observed,
        "reason": reason,
        "tx_id": tx_id,
        "authority_version": authority_version,
        "effect_id": effect_id,
        "receipt_id": receipt_id,
        "witness_verified": bool(db_path and db_path.exists())
    }

def main():
    parser = argparse.ArgumentParser(description="Independently verify durable witness evidence")
    parser.add_argument("evidence", type=Path, help="Path to evidence JSON")
    parser.add_argument("--db", type=Path, default=None, help="Path to witness SQLite DB")
    parser.add_argument("--secret-key", type=str, required=True, help="Secret key for witness HMAC verification")
    args = parser.parse_args()

    secret_key_bytes = args.secret_key.encode("utf-8")
    result = verify_evidence(args.evidence, args.db, secret_key=secret_key_bytes)
    print(json.dumps(result, indent=2))
    if not result.get("valid"):
        sys.exit(1)

if __name__ == "__main__":
    main()
