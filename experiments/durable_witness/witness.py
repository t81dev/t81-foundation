#!/usr/bin/env python3
import hmac
import hashlib
import json
import sqlite3
import time
from pathlib import Path
from typing import Dict, Any, Tuple

DEFAULT_SECRET_KEY = b"t81-durable-witness-secret-key-32bytes!!"

def init_db(db_path: Path, secret_key: bytes = DEFAULT_SECRET_KEY) -> sqlite3.Connection:
    """Initialize SQLite DB with WAL mode and atomic tables."""
    conn = sqlite3.connect(db_path, timeout=10.0)
    conn.execute("PRAGMA journal_mode=WAL;")
    conn.execute("PRAGMA synchronous=FULL;")
    with conn:
        conn.execute("""
            CREATE TABLE IF NOT EXISTS authority_policies (
                authority_version TEXT PRIMARY KEY,
                max_unit_price INTEGER NOT NULL,
                max_total_amount INTEGER NOT NULL,
                status TEXT NOT NULL CHECK(status IN ('ACTIVE', 'REVOKED'))
            );
        """)
        conn.execute("""
            CREATE TABLE IF NOT EXISTS transactions (
                tx_id TEXT PRIMARY KEY,
                authority_version TEXT NOT NULL,
                effect_id TEXT NOT NULL,
                amount INTEGER NOT NULL,
                timestamp TEXT NOT NULL,
                receipt_id TEXT NOT NULL,
                FOREIGN KEY(authority_version) REFERENCES authority_policies(authority_version)
            );
        """)
        conn.execute("""
            CREATE TABLE IF NOT EXISTS effects (
                effect_id TEXT PRIMARY KEY,
                tx_id TEXT NOT NULL,
                payload_hash TEXT NOT NULL,
                created_at TEXT NOT NULL
            );
        """)
        cur = conn.execute("SELECT COUNT(*) FROM authority_policies")
        if cur.fetchone()[0] == 0:
            conn.execute("INSERT INTO authority_policies VALUES ('v1', 1000, 50000, 'REVOKED');")
            conn.execute("INSERT INTO authority_policies VALUES ('v2', 1000, 50000, 'ACTIVE');")
    return conn

def compute_receipt_signature(
    secret_key: bytes,
    tx_id: str,
    authority_version: str,
    effect_id: str,
    payload_hash: str = "",
    total_amount: int = 0,
    timestamp: str = "",
    witness_id: str = "durable-witness-v1"
) -> str:
    message = f"{witness_id}:{tx_id}:{authority_version}:{effect_id}:{payload_hash}:{total_amount}:{timestamp}".encode("utf-8")
    return hmac.new(secret_key, message, hashlib.sha256).hexdigest()

def submit_transaction(
    db_path: Path,
    tx_id: str,
    authority_version: str,
    po_data: Dict[str, Any],
    secret_key: bytes = DEFAULT_SECRET_KEY,
    contract_limits: Dict[str, Any] = None
) -> Tuple[bool, Dict[str, Any], str]:
    # 0. Validate input values
    if not isinstance(po_data, dict):
        return False, {
            "error": "INVALID_TRANSACTION_INPUT",
            "message": "PO data must be a dictionary."
        }, "INVALID_TRANSACTION_INPUT"

    unit_price = po_data.get("unit_price")
    quantity = po_data.get("quantity")

    if (
        isinstance(unit_price, bool) or not isinstance(unit_price, int) or
        isinstance(quantity, bool) or not isinstance(quantity, int) or
        unit_price <= 0 or quantity <= 0
    ):
        return False, {
            "error": "INVALID_TRANSACTION_INPUT",
            "message": f"Invalid unit_price ({unit_price}) or quantity ({quantity}). Must be positive integers."
        }, "INVALID_TRANSACTION_INPUT"

    total_amount = unit_price * quantity

    conn = sqlite3.connect(db_path, timeout=10.0)
    conn.execute("PRAGMA journal_mode=WAL;")
    conn.execute("PRAGMA synchronous=FULL;")

    try:
        with conn:
            # 1. Check duplicate tx_id (Replay Protection)
            cur = conn.execute("SELECT tx_id, receipt_id FROM transactions WHERE tx_id = ?", (tx_id,))
            existing = cur.fetchone()
            if existing:
                return False, {
                    "error": "REPLAY_DETECTED",
                    "message": f"Transaction {tx_id} already executed and committed.",
                    "existing_receipt": existing[1]
                }, "REPLAY_DETECTED"

            # 2. Check authority version state (Revocation Check)
            cur = conn.execute(
                "SELECT status, max_unit_price, max_total_amount FROM authority_policies WHERE authority_version = ?",
                (authority_version,)
            )
            policy = cur.fetchone()
            if not policy:
                return False, {
                    "error": "INVALID_AUTHORITY_VERSION",
                    "message": f"Authority version {authority_version} not recognized."
                }, "INVALID_AUTHORITY_VERSION"

            status, db_max_unit_price, db_max_total_amount = policy
            if status != "ACTIVE":
                return False, {
                    "error": "REVOCATION_DETECTED",
                    "message": f"Authority version {authority_version} is REVOKED."
                }, "REVOCATION_DETECTED"

            # 3. Compute effective limits (caller limits can restrict but never relax stored authority limits)
            effective_max_unit_price = db_max_unit_price
            effective_max_total_amount = db_max_total_amount

            if contract_limits is not None:
                if not isinstance(contract_limits, dict):
                    return False, {
                        "error": "INVALID_TRANSACTION_INPUT",
                        "message": "contract_limits must be a dictionary if provided."
                    }, "INVALID_TRANSACTION_INPUT"

                for limit_key in ("max_unit_price", "max_total_amount"):
                    if limit_key in contract_limits:
                        val = contract_limits[limit_key]
                        if isinstance(val, bool) or not isinstance(val, int) or val <= 0:
                            return False, {
                                "error": "INVALID_TRANSACTION_INPUT",
                                "message": f"Invalid caller contract limit for {limit_key} ({val}). Must be a positive integer."
                            }, "INVALID_TRANSACTION_INPUT"

                if "max_unit_price" in contract_limits:
                    effective_max_unit_price = min(effective_max_unit_price, contract_limits["max_unit_price"])
                if "max_total_amount" in contract_limits:
                    effective_max_total_amount = min(effective_max_total_amount, contract_limits["max_total_amount"])

            if unit_price > effective_max_unit_price or total_amount > effective_max_total_amount:
                return False, {
                    "error": "POLICY_LIMIT_EXCEEDED",
                    "message": f"PO amount (unit: {unit_price}, total: {total_amount}) exceeds policy limits."
                }, "POLICY_LIMIT_EXCEEDED"

            # 4. Generate effect and receipt identity bound to tx_id
            canonical_po_bytes = json.dumps(po_data, sort_keys=True).encode("utf-8")
            payload_hash = hashlib.sha256(canonical_po_bytes).hexdigest()
            effect_id = f"effect-{tx_id}-{payload_hash[:16]}"
            timestamp = time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime())

            witness_id = "durable-witness-v1"
            receipt_id = compute_receipt_signature(
                secret_key=secret_key,
                tx_id=tx_id,
                authority_version=authority_version,
                effect_id=effect_id,
                payload_hash=payload_hash,
                total_amount=total_amount,
                timestamp=timestamp,
                witness_id=witness_id
            )

            # 5. Insert atomically into SQLite DB
            conn.execute(
                "INSERT INTO effects (effect_id, tx_id, payload_hash, created_at) VALUES (?, ?, ?, ?)",
                (effect_id, tx_id, payload_hash, timestamp)
            )
            conn.execute(
                "INSERT INTO transactions (tx_id, authority_version, effect_id, amount, timestamp, receipt_id) VALUES (?, ?, ?, ?, ?, ?)",
                (tx_id, authority_version, effect_id, total_amount, timestamp, receipt_id)
            )

            receipt = {
                "witness_id": "durable-witness-v1",
                "tx_id": tx_id,
                "authority_version": authority_version,
                "effect_id": effect_id,
                "payload_hash": payload_hash,
                "total_amount": total_amount,
                "timestamp": timestamp,
                "receipt_id": receipt_id
            }
            return True, receipt, "SUCCESS"

    except sqlite3.IntegrityError as e:
        return False, {"error": "INTEGRITY_ERROR", "message": str(e)}, "INTEGRITY_ERROR"
    finally:
        conn.close()

def revoke_authority(db_path: Path, authority_version: str):
    conn = sqlite3.connect(db_path, timeout=10.0)
    with conn:
        conn.execute("UPDATE authority_policies SET status = 'REVOKED' WHERE authority_version = ?", (authority_version,))
    conn.close()
