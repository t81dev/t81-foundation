#!/usr/bin/env python3
import sys
import json
import sqlite3
import argparse
from pathlib import Path

from witness import compute_receipt_signature, DEFAULT_SECRET_KEY

def verify_evidence(evidence_path: Path, db_path: Path = None, secret_key: bytes = DEFAULT_SECRET_KEY) -> dict:
    if not evidence_path.exists():
        return {
            "valid": False,
            "reason": "MISSING_EVIDENCE_FILE",
            "message": f"Evidence file {evidence_path} does not exist",
            "crypto_authentic": False,
            "authorization_valid": False,
            "witness_committed": False,
            "effect_observed": False
        }

    with open(evidence_path, "r", encoding="utf-8") as f:
        evidence = json.load(f)

    # If db_path was explicitly provided (i.e. not None), verify that it exists and is readable
    if db_path is not None and not db_path.exists():
        return {
            "valid": False,
            "reason": "MISSING_WITNESS_DATABASE",
            "message": f"Witness database path {db_path} was explicitly supplied but does not exist or cannot be inspected",
            "crypto_authentic": False,
            "authorization_valid": False,
            "witness_committed": False,
            "effect_observed": False
        }

    # 1. Inspect required fields
    receipt = evidence.get("receipt")
    po_data = evidence.get("po_data")

    if not receipt or not po_data:
        return {
            "valid": False,
            "reason": "MISSING_RECEIPT_OR_PODATA",
            "message": "Missing receipt or po_data in evidence",
            "crypto_authentic": False,
            "authorization_valid": False,
            "witness_committed": False,
            "effect_observed": False
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
    if receipt_id != expected_receipt_id:
        return {
            "valid": False,
            "reason": "INVALID_RECEIPT_SIGNATURE",
            "message": f"Receipt ID {receipt_id} does not match expected cryptographic HMAC {expected_receipt_id}",
            "crypto_authentic": False,
            "authorization_valid": False,
            "witness_committed": False,
            "effect_observed": False
        }

    # 3. Check payload hash matches PO data
    import hashlib
    canonical_po_bytes = json.dumps(po_data, sort_keys=True).encode("utf-8")
    expected_payload_hash = hashlib.sha256(canonical_po_bytes).hexdigest()
    if payload_hash != expected_payload_hash:
        return {
            "valid": False,
            "reason": "PAYLOAD_HASH_MISMATCH",
            "message": f"Evidence payload hash {payload_hash} does not match computed PO hash {expected_payload_hash}",
            "crypto_authentic": False,
            "authorization_valid": False,
            "witness_committed": False,
            "effect_observed": False
        }

    crypto_authentic = True

    # 4. If witness database path is not provided, perform detached/offline signature verification only.
    if db_path is None:
        return {
            "valid": False,
            "reason": "DETACHED_OFFLINE_SIGNATURE_ONLY",
            "message": "Detached/offline signature verification succeeded, but witness database presence is unverified. Offline signatures alone do not constitute a witness-backed success claim.",
            "tx_id": tx_id,
            "authority_version": authority_version,
            "effect_id": effect_id,
            "receipt_id": receipt_id,
            "crypto_authentic": crypto_authentic,
            "authorization_valid": False,
            "witness_committed": False,
            "effect_observed": False,
            "witness_verified": False
        }

    # 5. Inspect witness database for authorization, store commitment, and state consistency
    try:
        conn = sqlite3.connect(db_path)
    except sqlite3.Error as e:
        return {
            "valid": False,
            "reason": "MISSING_WITNESS_DATABASE",
            "message": f"Cannot connect to or inspect witness database {db_path}: {e}",
            "crypto_authentic": crypto_authentic,
            "authorization_valid": False,
            "witness_committed": False,
            "effect_observed": False
        }

    try:
        cur = conn.execute("SELECT authority_version, receipt_id FROM transactions WHERE tx_id = ?", (tx_id,))
        tx_row = cur.fetchone()

        cur = conn.execute(
            "SELECT status FROM authority_policies WHERE authority_version = ?",
            (authority_version,)
        )
        auth_row = cur.fetchone()
        conn.close()
    except sqlite3.OperationalError as e:
        return {
            "valid": False,
            "reason": "WITNESS_STORE_INSPECTION_FAILED",
            "message": f"Error querying witness store tables: {e}",
            "crypto_authentic": crypto_authentic,
            "authorization_valid": False,
            "witness_committed": False,
            "effect_observed": False
        }

    current_authority_active = (auth_row is not None and auth_row[0] == "ACTIVE")

    if not tx_row:
        if not auth_row or auth_row[0] != "ACTIVE":
            return {
                "valid": False,
                "reason": "REVOKED_OR_INVALID_AUTHORITY_VERSION",
                "message": f"Authority version {authority_version} is missing or not ACTIVE in witness store",
                "crypto_authentic": crypto_authentic,
                "authorization_valid": False,
                "witness_committed": False,
                "current_authority_active": current_authority_active,
                "effect_observed": False
            }

        return {
            "valid": False,
            "reason": "TRANSACTION_NOT_IN_WITNESS_STORE",
            "message": f"Transaction {tx_id} is absent from trusted witness database",
            "crypto_authentic": crypto_authentic,
            "authorization_valid": False,
            "witness_committed": False,
            "current_authority_active": current_authority_active,
            "effect_observed": False
        }

    db_auth_ver, db_receipt_id = tx_row

    if db_auth_ver != authority_version:
        return {
            "valid": False,
            "reason": "AUTHORITY_VERSION_MISMATCH_IN_WITNESS_STORE",
            "message": f"Recorded authority version {db_auth_ver} differs from evidence authority version {authority_version}",
            "crypto_authentic": crypto_authentic,
            "authorization_valid": False,
            "witness_committed": False,
            "current_authority_active": current_authority_active,
            "effect_observed": False
        }

    if db_receipt_id != receipt_id:
        return {
            "valid": False,
            "reason": "RECEIPT_MISMATCH_IN_WITNESS_STORE",
            "message": f"Recorded receipt {db_receipt_id} differs from evidence receipt {receipt_id}",
            "crypto_authentic": crypto_authentic,
            "authorization_valid": False,
            "witness_committed": False,
            "current_authority_active": current_authority_active,
            "effect_observed": False
        }

    witness_committed = True
    authorization_valid = True

    return {
        "valid": True,
        "tx_id": tx_id,
        "authority_version": authority_version,
        "effect_id": effect_id,
        "receipt_id": receipt_id,
        "crypto_authentic": crypto_authentic,
        "authorization_valid": authorization_valid,
        "witness_committed": witness_committed,
        "current_authority_active": current_authority_active,
        "effect_observed": False,  # Physical external world execution requires separate external attestation
        "witness_verified": True
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
