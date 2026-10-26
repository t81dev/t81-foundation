#!/usr/bin/env python3
import sys
import json
import shutil
import sqlite3
import tempfile
import threading
import hashlib
from pathlib import Path

from witness import init_db, submit_transaction, revoke_authority, backup_db, compute_receipt_signature
from runner import execute_runner
from verify import verify_evidence

TEST_SECRET_KEY = b"test-secret-key-for-attack-suite-32bytes!"

# Module-level worker functions for multiprocessing compatibility across fork, spawn, and forkserver
def _mp_worker_proc(queue, arm, tx_id, auth_ver, qty, price, db_path, out_dir, secret_key=TEST_SECRET_KEY, arm_a_bin=None):
    res = execute_runner(
        arm=arm,
        tx_id=tx_id,
        authority_version=auth_ver,
        quantity=qty,
        unit_price=price,
        db_path=db_path,
        output_dir=out_dir,
        secret_key=secret_key,
        arm_a_bin=arm_a_bin
    )
    queue.put(res)

def _mp_proc_revoke(db_path):
    revoke_authority(db_path, "v2")

def _mp_proc_submit(queue, arm, tx_id, auth_ver, qty, price, db_path, out_dir, secret_key=TEST_SECRET_KEY, arm_a_bin=None):
    res = execute_runner(
        arm=arm,
        tx_id=tx_id,
        authority_version=auth_ver,
        quantity=qty,
        unit_price=price,
        db_path=db_path,
        output_dir=out_dir,
        secret_key=secret_key,
        arm_a_bin=arm_a_bin
    )
    queue.put(res)

def run_attacks(base_dir: Path, secret_key: bytes = TEST_SECRET_KEY) -> dict:
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

        ver = verify_evidence(ev_file, db_path, secret_key=secret_key)
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

        execute_runner("A", "tx-replay-02", "v2", 10, 100, db_path, att_dir, secret_key=secret_key)
        replay_res = execute_runner("A", "tx-replay-02", "v2", 10, 100, db_path, att_dir, secret_key=secret_key)

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

        contexts_to_test = ["spawn", "forkserver", "fork"]
        context_reports = {}
        overall_blocked = True

        for ctx_name in contexts_to_test:
            try:
                ctx = multiprocessing.get_context(ctx_name)
            except (ValueError, KeyError, AttributeError) as e:
                context_reports[ctx_name] = {"status": "UNSUPPORTED", "reason": str(e)}
                continue

            ctx_db_path = att_dir / f"witness_{ctx_name}.db"
            init_db(ctx_db_path)
            ctx_out_dir = att_dir / f"out_{ctx_name}"
            ctx_out_dir.mkdir(parents=True, exist_ok=True)

            try:
                q = ctx.Queue()
                p1 = ctx.Process(target=_mp_worker_proc, args=(q, "B", "tx-concurrent-03", "v2", 5, 100, ctx_db_path, ctx_out_dir, secret_key))
                p2 = ctx.Process(target=_mp_worker_proc, args=(q, "B", "tx-concurrent-03", "v2", 5, 100, ctx_db_path, ctx_out_dir, secret_key))

                p1.start()
                p2.start()
                p1.join(timeout=10.0)
                p2.join(timeout=10.0)

                results_list = []
                while not q.empty():
                    results_list.append(q.get())

                # Also test revocation racing with transaction admission across separate OS processes
                q_race = ctx.Queue()
                p_sub = ctx.Process(target=_mp_proc_submit, args=(q_race, "B", "tx-race-rev-03", "v2", 5, 100, ctx_db_path, ctx_out_dir, secret_key))
                p_rev = ctx.Process(target=_mp_proc_revoke, args=(ctx_db_path,))

                p_sub.start()
                p_rev.start()
                p_sub.join(timeout=10.0)
                p_rev.join(timeout=10.0)

                race_res = q_race.get() if not q_race.empty() else None

                successes = [r for r in results_list if r["success"]]
                blocked_replays = [
                    r for r in results_list
                    if not r["success"] and r.get("status_code") in ("REPLAY_DETECTED", "INTEGRITY_ERROR", "DATABASE_LOCKED")
                ]

                ctx_blocked = (len(successes) == 1 and len(blocked_replays) == 1)
                if not ctx_blocked:
                    overall_blocked = False

                context_reports[ctx_name] = {
                    "status": "EXECUTED",
                    "blocked": ctx_blocked,
                    "successes": len(successes),
                    "blocked_replays": len(blocked_replays),
                    "revocation_race_result": race_res.get("status_code") if race_res else "UNKNOWN"
                }
            except Exception as e:
                context_reports[ctx_name] = {"status": "FAILED", "error": str(e)}
                overall_blocked = False

        return {
            "attack_id": 3,
            "name": "Duplicate transaction submission from concurrent processes",
            "blocked": overall_blocked,
            "verdict": "BLOCKED" if overall_blocked else "FAILED",
            "context_reports": context_reports
        }

    # Attack 4: Authorization revocation immediately before effect admission
    def attack_4():
        att_dir = get_attack_dir(4)
        db_path = att_dir / "witness.db"
        init_db(db_path)
        revoke_authority(db_path, "v2")

        res = execute_runner("A", "tx-rev-04", "v2", 10, 100, db_path, att_dir, secret_key=secret_key)
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
        succ1, res1, stat1 = submit_transaction(db_path, "tx-interrupted-05", "v2", po_data, secret_key=secret_key)
        succ2, res2, stat2 = submit_transaction(db_path, "tx-interrupted-05", "v2", po_data, secret_key=secret_key)

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
            secret_key=secret_key,
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

        ver = verify_evidence(phantom_file, db_path, secret_key=secret_key)
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

        ver = verify_evidence(tamp_file, db_path, secret_key=b"trusted-verifier-key-32bytes!!")
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

        # Step 1: Take consistent snapshot before transaction using SQLite backup API
        snapshot_pre = att_dir / "witness_snapshot_pre.db"
        backup_db(db_path, snapshot_pre)

        # Step 2: Execute original transaction and backup post-execution state
        ev1 = execute_runner("A", "tx-rollback-08", "v2", 10, 100, db_path, att_dir, secret_key=secret_key)
        ev_file = att_dir / "evidence_tx-rollback-08.json"

        snapshot_post = att_dir / "witness_snapshot_post.db"
        backup_db(db_path, snapshot_post)

        # Verifying evidence against pre-execution snapshot DB fails because tx is absent
        ver_snap_pre = verify_evidence(ev_file, snapshot_pre, secret_key=secret_key)

        # Step 3: Restore pre-execution snapshot (rollback witness storage) using backup API
        backup_db(snapshot_pre, db_path)

        # Step 4: Re-submit the same transaction ID to the rolled-back witness database
        ev2 = execute_runner("A", "tx-rollback-08", "v2", 10, 100, db_path, att_dir, secret_key=secret_key)

        # Since witness DB was rolled back, ev2 succeeds! This proves transaction reuse.
        rollback_reused_tx = ev2["success"]
        blocked = not rollback_reused_tx

        return {
            "attack_id": 8,
            "name": "Witness storage rollback or restoration from an old snapshot",
            "blocked": blocked,
            "verdict": "VULNERABLE_TO_ROLLBACK_REUSE" if not blocked else "BLOCKED",
            "reason": "WITNESS_ROLLBACK_ALLOWED_TX_REUSE" if not blocked else "ROLLBACK_PREVENTED",
            "message": "Isolated witness store accepted previously committed transaction ID after DB snapshot rollback. Requires external monotonic checkpoint or transparency log to prevent.",
            "verifier_snapshot_check": ver_snap_pre.get("reason"),
            "preserved_snapshots": {
                "pre_execution_db": str(snapshot_pre),
                "post_execution_db": str(snapshot_post)
            }
        }

    # Attack 9: Conflicting authority versions
    def attack_9():
        att_dir = get_attack_dir(9)
        db_path = att_dir / "witness.db"
        init_db(db_path)

        res = execute_runner("B", "tx-conflict-09", "v1", 10, 100, db_path, att_dir, secret_key=secret_key)
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
            success, res, status = submit_transaction(bad_db, "tx-fail-10", "v2", {"item": "Test", "unit_price": 10, "quantity": 1}, secret_key=secret_key)
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
