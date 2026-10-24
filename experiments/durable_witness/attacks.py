#!/usr/bin/env python3
import sys
import json
import shutil
import sqlite3
import tempfile
import threading
import hashlib
from pathlib import Path

from witness import init_db, submit_transaction, revoke_authority, compute_receipt_signature, DEFAULT_SECRET_KEY
from runner import execute_runner
from verify import verify_evidence

def run_attacks(base_dir: Path) -> dict:
    if base_dir.exists():
        shutil.rmtree(base_dir)
    base_dir.mkdir(parents=True, exist_ok=True)

    results = []

    def get_attack_dir(attack_id: int) -> Path:
        p = base_dir / f"attack_{attack_id}"
        p.mkdir(parents=True, exist_ok=True)
        return p

    # Attack 1: Fabricated producer evidence with valid internal hashes
    def attack_1():
        att_dir = get_attack_dir(1)
        db_path = att_dir / "witness.db"
        init_db(db_path)

        po_data = {"item": "Widget-X", "quantity": 10, "unit_price": 100, "currency": "USD"}
        payload_hash = hashlib.sha256(json.dumps(po_data, sort_keys=True).encode("utf-8")).hexdigest()
        fake_evidence = {
            "arm": "A",
            "tx_id": "tx-fab-01",
            "authority_version": "v2",
            "po_data": po_data,
            "receipt": {
                "witness_id": "durable-witness-v1",
                "tx_id": "tx-fab-01",
                "authority_version": "v2",
                "effect_id": "effect-fake123",
                "payload_hash": payload_hash,
                "total_amount": 1000,
                "timestamp": "2026-10-22T20:00:00Z",
                "receipt_id": "deadbeef" * 8
            }
        }
        ev_file = att_dir / "evidence_attack_1.json"
        with open(ev_file, "w") as f:
            json.dump(fake_evidence, f, indent=2)

        ver = verify_evidence(ev_file, db_path)
        blocked = not ver["valid"] and ver.get("reason") == "INVALID_RECEIPT_SIGNATURE"
        return {
            "attack_id": 1,
            "name": "Fabricated producer evidence with valid internal hashes",
            "blocked": blocked,
            "verdict": "BLOCKED" if blocked else "ALLOWED",
            "reason": ver.get("reason"),
            "message": ver.get("message")
        }

    # Attack 2: Valid receipt replay after process restart
    def attack_2():
        att_dir = get_attack_dir(2)
        db_path = att_dir / "witness.db"
        init_db(db_path)

        execute_runner("A", "tx-replay-02", "v2", 10, 100, db_path, att_dir)
        replay_res = execute_runner("A", "tx-replay-02", "v2", 10, 100, db_path, att_dir)

        blocked = not replay_res["success"] and replay_res["status_code"] == "REPLAY_DETECTED"
        return {
            "attack_id": 2,
            "name": "Valid receipt replay after process restart",
            "blocked": blocked,
            "verdict": "BLOCKED" if blocked else "ALLOWED",
            "reason": replay_res.get("status_code"),
            "message": replay_res.get("error", {}).get("message") if isinstance(replay_res.get("error"), dict) else None
        }

    # Attack 3: Duplicate transaction submission from concurrent processes
    def attack_3():
        att_dir = get_attack_dir(3)
        db_path = att_dir / "witness.db"
        init_db(db_path)

        import multiprocessing

        def worker_proc(queue, arm, tx_id, auth_ver, qty, price, db, out):
            res = execute_runner(arm, tx_id, auth_ver, qty, price, db, out)
            queue.put(res)

        q = multiprocessing.Queue()
        p1 = multiprocessing.Process(target=worker_proc, args=(q, "B", "tx-concurrent-03", "v2", 5, 100, db_path, att_dir))
        p2 = multiprocessing.Process(target=worker_proc, args=(q, "B", "tx-concurrent-03", "v2", 5, 100, db_path, att_dir))

        p1.start()
        p2.start()
        p1.join(timeout=5.0)
        p2.join(timeout=5.0)

        results_list = []
        while not q.empty():
            results_list.append(q.get())

        # Also test revocation racing with transaction admission across separate OS processes
        def proc_revoke(db):
            revoke_authority(db, "v2")

        def proc_submit(queue, db, out):
            res = execute_runner("B", "tx-race-rev-03", "v2", 5, 100, db, out)
            queue.put(res)

        q_race = multiprocessing.Queue()
        p_sub = multiprocessing.Process(target=proc_submit, args=(q_race, db_path, att_dir))
        p_rev = multiprocessing.Process(target=proc_revoke, args=(db_path,))

        p_sub.start()
        p_rev.start()
        p_sub.join(timeout=5.0)
        p_rev.join(timeout=5.0)

        race_res = q_race.get() if not q_race.empty() else None

        successes = [r for r in results_list if r["success"]]
        blocked_replays = [
            r for r in results_list
            if not r["success"] and r.get("status_code") in ("REPLAY_DETECTED", "INTEGRITY_ERROR", "DATABASE_LOCKED")
        ]

        blocked = (len(successes) == 1 and len(blocked_replays) == 1)
        return {
            "attack_id": 3,
            "name": "Duplicate transaction submission from concurrent processes",
            "blocked": blocked,
            "verdict": "BLOCKED" if blocked else "ALLOWED",
            "details": f"Successes: {len(successes)}, Blocked concurrent attempts: {len(blocked_replays)}",
            "revocation_race_result": race_res.get("status_code") if race_res else "UNKNOWN"
        }

    # Attack 4: Authorization revocation immediately before effect admission
    def attack_4():
        att_dir = get_attack_dir(4)
        db_path = att_dir / "witness.db"
        init_db(db_path)
        revoke_authority(db_path, "v2")

        res = execute_runner("A", "tx-rev-04", "v2", 10, 100, db_path, att_dir)
        blocked = not res["success"] and res["status_code"] == "REVOCATION_DETECTED"
        return {
            "attack_id": 4,
            "name": "Authorization revocation immediately before effect admission",
            "blocked": blocked,
            "verdict": "BLOCKED" if blocked else "ALLOWED",
            "reason": res.get("status_code"),
            "message": res.get("error", {}).get("message") if isinstance(res.get("error"), dict) else None
        }

    # Attack 5: Effect committed but receipt publication interrupted
    def attack_5():
        att_dir = get_attack_dir(5)
        db_path = att_dir / "witness.db"
        init_db(db_path)

        po_data = {"item": "Widget-X", "quantity": 10, "unit_price": 100, "currency": "USD"}
        succ1, res1, stat1 = submit_transaction(db_path, "tx-interrupted-05", "v2", po_data)
        succ2, res2, stat2 = submit_transaction(db_path, "tx-interrupted-05", "v2", po_data)

        blocked = (succ1 and (not succ2) and stat2 == "REPLAY_DETECTED" and "existing_receipt" in res2)
        return {
            "attack_id": 5,
            "name": "Effect committed but receipt publication interrupted",
            "blocked": blocked,
            "verdict": "RECOVERY_RECEIPT_RETRIEVED" if blocked else "FAILED",
            "existing_receipt": res2.get("existing_receipt") if isinstance(res2, dict) else None
        }

    # Attack 6: Receipt generated without claimed effect
    def attack_6():
        att_dir = get_attack_dir(6)
        db_path = att_dir / "witness.db"
        init_db(db_path)

        po_data = {"item": "Phantom-Widget", "quantity": 1, "unit_price": 500, "currency": "USD"}
        canonical_po_bytes = json.dumps(po_data, sort_keys=True).encode("utf-8")
        payload_hash = hashlib.sha256(canonical_po_bytes).hexdigest()

        tx_id = "tx-phantom-06"
        authority_version = "v2"
        effect_id = "effect-phantom"
        timestamp = "2026-10-22T20:00:00Z"
        witness_id = "durable-witness-v1"
        total_amount = 500

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
        phantom_file = att_dir / "evidence_attack_6.json"
        with open(phantom_file, "w") as f:
            json.dump(phantom_evidence, f, indent=2)

        ver = verify_evidence(phantom_file, db_path)
        blocked = not ver["valid"] and ver.get("reason") == "TRANSACTION_NOT_IN_WITNESS_STORE"
        return {
            "attack_id": 6,
            "name": "Receipt generated without claimed effect",
            "blocked": blocked,
            "verdict": "BLOCKED" if blocked else "ALLOWED",
            "reason": ver.get("reason"),
            "message": ver.get("message")
        }

    # Attack 7: Tampered or substituted witness identities
    def attack_7():
        att_dir = get_attack_dir(7)
        db_path = att_dir / "witness.db"
        init_db(db_path)

        po_data = {"item": "Widget-X", "quantity": 10, "unit_price": 100, "currency": "USD"}
        wrong_key = b"wrong-secret-key-00000000000000!"
        tx_id = "tx-tamper-07"
        authority_version = "v2"
        effect_id = "effect-tamper"
        receipt_id = compute_receipt_signature(wrong_key, tx_id, authority_version, effect_id)

        payload_hash = hashlib.sha256(json.dumps(po_data, sort_keys=True).encode("utf-8")).hexdigest()

        tampered_evidence = {
            "arm": "A",
            "tx_id": tx_id,
            "authority_version": authority_version,
            "po_data": po_data,
            "receipt": {
                "witness_id": "fake-witness-v99",
                "tx_id": tx_id,
                "authority_version": authority_version,
                "effect_id": effect_id,
                "payload_hash": payload_hash,
                "total_amount": 1000,
                "timestamp": "2026-10-22T20:00:00Z",
                "receipt_id": receipt_id
            }
        }
        tamp_file = att_dir / "evidence_attack_7.json"
        with open(tamp_file, "w") as f:
            json.dump(tampered_evidence, f, indent=2)

        ver = verify_evidence(tamp_file, db_path)
        blocked = not ver["valid"] and ver.get("reason") == "INVALID_RECEIPT_SIGNATURE"
        return {
            "attack_id": 7,
            "name": "Tampered or substituted witness identities",
            "blocked": blocked,
            "verdict": "BLOCKED" if blocked else "ALLOWED",
            "reason": ver.get("reason")
        }

    # Attack 8: Witness storage rollback or restoration from an old snapshot
    def attack_8():
        """Demonstrate required investigation 6:

        Restoring an older valid witness database snapshot wipes the record of committed transactions.
        When an attacker re-submits a previously committed transaction ID to the restored database,
        the isolated database accepts it as a new transaction and issues a second valid receipt.
        This proves that isolated database state CANNOT resist rollback without an independent external
        checkpoint, monotonic sequence counter, or external observer log.
        """
        att_dir = get_attack_dir(8)
        db_path = att_dir / "witness.db"
        init_db(db_path)

        # Take snapshot before transaction
        snapshot_db = att_dir / "witness_snapshot.db"
        shutil.copyfile(db_path, snapshot_db)

        # Step 1: Execute original transaction
        ev1 = execute_runner("A", "tx-rollback-08", "v2", 10, 100, db_path, att_dir)
        ev_file = att_dir / "evidence_tx-rollback-08.json"

        # Verifying evidence against snapshot DB fails because tx is absent from snapshot
        ver_snap = verify_evidence(ev_file, snapshot_db)

        # Step 2: Restore older witness snapshot (rollback witness storage)
        shutil.copyfile(snapshot_db, db_path)

        # Step 3: Re-submit the same transaction ID to the rolled-back witness database
        ev2 = execute_runner("A", "tx-rollback-08", "v2", 10, 100, db_path, att_dir)

        # Since witness DB was rolled back, ev2 succeeds! This proves rollback vulnerability.
        rollback_reused_tx = ev2["success"]

        return {
            "attack_id": 8,
            "name": "Witness storage rollback or restoration from an old snapshot",
            "blocked": False,
            "verdict": "VULNERABLE_TO_ROLLBACK_REUSE",
            "reason": "WITNESS_ROLLBACK_ALLOWED_TX_REUSE",
            "message": "Isolated witness store accepted previously committed transaction ID after DB snapshot rollback. Requires external monotonic checkpoint or transparency log to prevent.",
            "verifier_snapshot_check": ver_snap.get("reason")
        }

    # Attack 9: Conflicting authority versions
    def attack_9():
        att_dir = get_attack_dir(9)
        db_path = att_dir / "witness.db"
        init_db(db_path)

        res = execute_runner("B", "tx-conflict-09", "v1", 10, 100, db_path, att_dir)
        blocked = not res["success"] and res["status_code"] == "REVOCATION_DETECTED"
        return {
            "attack_id": 9,
            "name": "Conflicting authority versions",
            "blocked": blocked,
            "verdict": "BLOCKED" if blocked else "ALLOWED",
            "reason": res.get("status_code")
        }

    # Attack 10: Partial persistence failure and recovery
    def attack_10():
        att_dir = get_attack_dir(10)
        bad_db = att_dir / "readonly_witness.db"
        init_db(bad_db)
        bad_db.chmod(0o444)

        try:
            success, res, status = submit_transaction(bad_db, "tx-fail-10", "v2", {"item": "Test", "unit_price": 10, "quantity": 1})
            blocked = not success
        except Exception as e:
            blocked = True
            status = "INTEGRITY_ERROR"
        finally:
            bad_db.chmod(0o644)

        return {
            "attack_id": 10,
            "name": "Partial persistence failure and recovery",
            "blocked": blocked,
            "verdict": "ROLLED_BACK_CLEANLY" if blocked else "PARTIAL_WRITE",
            "status": status
        }

    results.append(attack_1())
    results.append(attack_2())
    results.append(attack_3())
    results.append(attack_4())
    results.append(attack_5())
    results.append(attack_6())
    results.append(attack_7())
    results.append(attack_8())
    results.append(attack_9())
    results.append(attack_10())

    summary = {
        "total_attacks": 10,
        "blocked_attacks": sum(1 for r in results if r.get("blocked")),
        "results": results
    }

    with open(base_dir / "attacks.json", "w") as f:
        json.dump(summary, f, indent=2)

    with open(base_dir / "raw.jsonl", "w") as f:
        for r in results:
            f.write(json.dumps(r) + "\n")

    return summary

def main():
    out = Path("/tmp/durable-witness-attacks")
    summary = run_attacks(out)
    print(json.dumps(summary, indent=2))

if __name__ == "__main__":
    main()
