"""Independent schedule/state checker; does not import or execute the producer.

Trace statements and volatile-memory observations assume a trusted test harness.
Filesystem bytes can additionally be checked from the saved fresh-store artifacts.
"""
import argparse
import hashlib
import json
from pathlib import Path

BACKENDS = ("conventional-file", "t81-file", "conventional-memory", "t81-memory",
            "conventional-block", "t81-block")
MODES = ("snapshot", "serialized-owner")
SCENARIOS = ("no_revocation", "before_authorization", "after_authorization",
             "during_revocation", "after_commit", "at_mutation")
PAYLOAD = b"25000\n"


def check(condition, message):
    if not condition:
        raise ValueError(message)


def content_address(data):
    alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-−×÷=<>≤≥≠≈∞λμπστωΓ"
    value, name = int.from_bytes(hashlib.sha3_512(data).digest()[:32], "big"), ""
    while value:
        value, digit = divmod(value, 81)
        name = alphabet[digit] + name
    return name


def verify(path, artifacts=None):
    evidence = json.loads(Path(path).read_text())
    check(evidence["version"] == 1 and evidence["payload_hex"] == PAYLOAD.hex(), "wrong evidence profile")
    expected = {(b, m, s) for b in BACKENDS for m in MODES for s in SCENARIOS
                if s != "at_mutation" or b.endswith("block")}
    seen = set()
    snapshot_counterexamples = 0
    serialized_prohibited = 0
    total = 0
    for row in evidence["rows"]:
        key = row["backend"], row["mode"], row["scenario"]
        check(key in expected and key not in seen, "unknown/duplicate case")
        seen.add(key)
        backend, mode, scenario = key
        n = row["repetitions"]
        check(type(n) is int and n > 0 and len(row["writer_us"]) == n, "missing repetitions")
        check(all(type(v) in (float, int) and v >= 0 for v in row["writer_us"]), "bad timing sample")
        total += n
        trace = row["trace"]
        check(len(trace) == len(set(trace)), "ambiguous repeated trace event")
        def before(a, b):
            check(a in trace and b in trace and trace.index(a) < trace.index(b), f"order: {a} before {b}")
        committed = scenario not in ("before_authorization", "during_revocation")
        check(row["committed"] is committed and row["payload_present"] is committed, "wrong storage outcome")
        check(row["observed_hex"] == (PAYLOAD.hex() if committed else ""), "wrong observed bytes")
        before("pause.before_authorization", "release.before_authorization")
        verdict = "authorization.allow" if committed else "authorization.deny"
        finish = "write.commit" if committed else "write.denied"
        before("release.before_authorization", verdict)
        before(verdict, finish)
        check(row["followup_denied"] is (scenario != "no_revocation"), "post-revoke request not denied")
        check(row["policy_calls"] == (1 if scenario == "no_revocation" else 2), "wrong callback count")
        if scenario == "no_revocation":
            check(not any(e.startswith("revoke.") for e in trace), "unexpected revocation")
        else:
            before("revoke.begin", "revoke.linearized")
            before("revoke.linearized", "revoke.complete")
            before("revoke.complete", "followup.denied")
        if scenario == "before_authorization":
            before("revoke.complete", "release.before_authorization")
        if scenario == "during_revocation":
            before("revoke.linearized", "pause.revocation")
            before("pause.revocation", "release.before_authorization")
            before("release.revocation", "revoke.complete")
            before("revoke.complete", "release.authorization")
            before("release.authorization", finish)
            if mode == "serialized-owner":
                before("writer.lock_busy", "release.revocation")
                before("revoke.complete", verdict)
            else:
                before(verdict, "release.revocation")
        if scenario in ("after_authorization", "at_mutation", "after_commit"):
            point = {"after_authorization": "authorization", "at_mutation": "mutation",
                     "after_commit": "commit"}[scenario]
            before("pause." + point, "release." + point)
            if mode == "serialized-owner":
                before("pause." + point, "revoker.lock_busy")
                before("revoker.lock_busy", "release." + point)
                before("write.commit", "revoke.linearized")
            else:
                before("revoke.complete", "release." + point)
            if scenario == "after_commit":
                before("write.commit", "pause.commit")
            elif scenario == "after_authorization":
                before("authorization.allow", "pause.authorization")
                before("release.authorization", "write.commit")
            else:
                before("storage.before_first_mutation", "pause.mutation")
                before("release.mutation", "storage.first_mutation")
        if backend.endswith("block"):
            raw = bytes.fromhex(row["device_data_hex"])
            check(len(raw) == 729, "wrong block image size")
            if committed:
                check(row["device_writes"] > 0 and row["device_flushes"] > 0, "no observable device commit")
                before("authorization.allow", "storage.before_first_mutation")
                before("storage.before_first_mutation", "storage.first_mutation")
                before("storage.first_mutation", "write.commit")
                if backend == "t81-block":
                    check(int.from_bytes(raw[:4], "little") == len(PAYLOAD) and raw[4:10] == PAYLOAD,
                          "wrong CanonFS packed block")
                else:
                    check(raw[:len(PAYLOAD)] == PAYLOAD, "wrong conventional block")
            else:
                check(row["device_writes"] == row["device_flushes"] == 0 and not any(raw),
                      "denied request modified device")
        else:
            check(row["device_writes"] == row["device_flushes"] == 0 and row["device_data_hex"] == "",
                  "unexpected device effect")
        expected_files = {}
        if committed and backend.endswith("file"):
            name = "result" if backend == "conventional-file" else f"objects/{content_address(PAYLOAD)}.blk"
            expected_files = {name: PAYLOAD.hex()}
        check(row["files"] == expected_files, "filesystem effect mismatch")
        if artifacts is not None:
            for repetition in range(n):
                directory = Path(artifacts) / ("-".join(key) + f"-{repetition}")
                check(directory.is_dir(), "missing fresh-store artifact")
                files = {p.relative_to(directory).as_posix(): p.read_bytes().hex()
                         for p in directory.rglob("*") if p.is_file()}
                check(files == expected_files, "actual filesystem differs from receipt")
        violation = committed and scenario != "no_revocation" and trace.index("revoke.complete") < trace.index("write.commit")
        expected_violation = mode == "snapshot" and scenario in ("after_authorization", "at_mutation")
        check(violation == expected_violation, "unexpected stronger-contract result")
        if violation:
            snapshot_counterexamples += n
        if mode == "serialized-owner" and violation:
            serialized_prohibited += n
    check(seen == expected, "missing scheduled case")
    return dict(cases=len(seen), operations=total,
                stronger_goal_snapshot_counterexamples=snapshot_counterexamples,
                serialized_owner_prohibited_commits=serialized_prohibited,
                existing_serialized_contract_violation_demonstrated=False)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("evidence", type=Path)
    parser.add_argument("--artifacts", type=Path)
    args = parser.parse_args()
    try:
        print(json.dumps(verify(args.evidence, args.artifacts), indent=2))
    except (ValueError, KeyError, TypeError, OSError) as error:
        parser.exit(1, f"REJECT: {error}\n")
