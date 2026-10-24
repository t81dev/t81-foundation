#!/usr/bin/env python3
import sys
import json
import sqlite3
import argparse
from pathlib import Path

from witness import compute_receipt_signature, DEFAULT_SECRET_KEY

def verify_evidence(evidence_path: Path, db_path: Path = None, secret_key: bytes = DEFAULT_SECRET_KEY) -> dict:
    if not evidence_path.exists():
        return {"valid": False, "reason": f"Evidence file {evidence_path} does not exist"}

    with open(evidence_path, "r", encoding="utf-8") as f:
        evidence = json.load(f)

    # 1. Inspect required fields
    receipt = evidence.get("receipt")
    po_data = evidence.get("po_data")

    if not receipt or not po_data:
        return {"valid": False, "reason": "Missing receipt or po_data in evidence"}

    tx_id = receipt.get("tx_id")
    authority_version = receipt.get("authority_version")
    effect_id = receipt.get("effect_id")
    receipt_id = receipt.get("receipt_id")
    payload_hash = receipt.get("payload_hash")

    # 2. Check receipt signature
    expected_receipt_id = compute_receipt_signature(secret_key, tx_id, authority_version, effect_id)
    if receipt_id != expected_receipt_id:
        return {
            "valid": False,
            "reason": "INVALID_RECEIPT_SIGNATURE",
            "message": f"Receipt ID {receipt_id} does not match expected cryptographic HMAC {expected_receipt_id}"
        }

    # 3. Check payload hash matches PO data
    import hashlib
    canonical_po_bytes = json.dumps(po_data, sort_keys=True).encode("utf-8")
    expected_payload_hash = hashlib.sha256(canonical_po_bytes).hexdigest()
    if payload_hash != expected_payload_hash:
        return {
            "valid": False,
            "reason": "PAYLOAD_HASH_MISMATCH",
            "message": f"Evidence payload hash {payload_hash} does not match computed PO hash {expected_payload_hash}"
        }

    # 4. If witness database path provided, inspect durable state
    if db_path and db_path.exists():
        conn = sqlite3.connect(db_path)
        try:
            cur = conn.execute("SELECT authority_version, receipt_id FROM transactions WHERE tx_id = ?", (tx_id,))
            row = cur.fetchone()
        except sqlite3.OperationalError:
            row = None
        finally:
            conn.close()

        if not row:
            return {
                "valid": False,
                "reason": "TRANSACTION_NOT_IN_WITNESS_STORE",
                "message": f"Transaction {tx_id} is absent from trusted witness database"
            }

        db_auth_ver, db_receipt_id = row
        if db_receipt_id != receipt_id:
            return {
                "valid": False,
                "reason": "RECEIPT_MISMATCH_IN_WITNESS_STORE",
                "message": f"Recorded receipt {db_receipt_id} differs from evidence receipt {receipt_id}"
            }

    return {
        "valid": True,
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
    args = parser.parse_args()

    result = verify_evidence(args.evidence, args.db)
    print(json.dumps(result, indent=2))
    if not result.get("valid"):
        sys.exit(1)

if __name__ == "__main__":
    main()
