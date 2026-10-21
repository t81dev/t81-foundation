"""Detached verifier. Uses only Python stdlib, OpenSSL, and caller-pinned trust.

It checks signed statements and recomputes this bounded transformation. A producer
signature authenticates a claim; it cannot prove the absence of unreported writes.
"""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile

ALLOW = b"(policy (tier 1))\n"
DENY = b'(policy (tier 1) (require-axion-event (reason "commit-authorized")))\n'


def check_profile(value):
    if type(value) in (str, int):
        return
    if type(value) is list:
        for item in value:
            check_profile(item)
        return
    if type(value) is dict and all(type(key) is str for key in value):
        for item in value.values():
            check_profile(item)
        return
    raise ValueError("outside restricted canonical JSON profile")


def verify(receipt, public_key, expected_input, expected_policy):
    envelope = json.loads(Path(receipt).read_text())
    if set(envelope) != {"record", "signature"}:
        raise ValueError("incomplete envelope")
    record = envelope["record"]
    check_profile(record)
    required = {"version", "arm", "case", "input_hex", "expected_digest", "policy_hex",
                "observation", "objects", "boundary", "method"}
    if set(record) != required or record["version"] != 1:
        raise ValueError("incomplete record")
    if record["arm"] not in ("A", "B", "C") or record["boundary"] != "result-object creation":
        raise ValueError("unsupported scope")
    if record["method"] != "bounded-integer-product-v1":
        raise ValueError("unsupported method")
    # Restricted canonical form: ASCII strings, integers, lists, objects; no floats.
    canonical = json.dumps(record, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode()
    with tempfile.TemporaryDirectory() as temp:
        data, sig = Path(temp) / "record", Path(temp) / "signature"
        data.write_bytes(canonical)
        sig.write_bytes(bytes.fromhex(envelope["signature"]))
        check = subprocess.run(["openssl", "pkeyutl", "-verify", "-pubin", "-inkey", str(public_key),
                                "-rawin", "-in", str(data), "-sigfile", str(sig)], capture_output=True)
        if check.returncode:
            raise ValueError("signature mismatch")
    payload = bytes.fromhex(record["input_hex"])
    policy = bytes.fromhex(record["policy_hex"])
    if payload != Path(expected_input).read_bytes() or policy != Path(expected_policy).read_bytes():
        raise ValueError("caller trust mismatch")
    h = lambda b: hashlib.sha3_512(b).digest()[:32].hex()
    obs = record["observation"]
    if set(obs) != {"status", "input_digest", "policy_digest", "output_digest", "amount", "vm_policy_calls", "policy_calls"}:
        raise ValueError("incomplete observation")
    if obs["input_digest"] != h(payload) or obs["policy_digest"] != h(policy):
        raise ValueError("digest mismatch")
    identity_ok = record["expected_digest"] == h(payload)
    allowed = identity_ok and policy == ALLOW
    expected_status = "committed" if allowed else "policy_denied" if identity_ok else "identity_denied"
    if obs["status"] != expected_status:
        raise ValueError("policy or identity violation")
    units, price = map(int, payload.split())
    if not 1 <= units <= 1000 or not 1 <= price <= 1000:
        raise ValueError("unsupported input range")
    amount = units * price if identity_ok else 0
    if obs["amount"] != amount:
        raise ValueError("replay mismatch")
    objects = record["objects"]
    if len(objects) != int(allowed):
        raise ValueError("side-effect count mismatch")
    if allowed:
        if set(objects[0]) != {"name", "hex"}:
            raise ValueError("incomplete object")
        expected_output = f"{amount}\n".encode()
        if bytes.fromhex(objects[0]["hex"]) != expected_output or obs["output_digest"] != h(expected_output):
            raise ValueError("output replay mismatch")
        # Independent CanonFS base81 name computation (big-endian integer).
        alphabet = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-−×÷=<>≤≥≠≈∞λμπστωΓ"
        value, name = int(h(expected_output), 16), ""
        while value:
            value, digit = divmod(value, 81)
            name = alphabet[digit] + name
        target = h(expected_output) + ".blk" if record["arm"] == "A" else name + ".blk"
        if objects[0]["name"] != target:
            raise ValueError("content address mismatch")
    elif obs["output_digest"]:
        raise ValueError("denial contains output")
    return record


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("receipt", type=Path)
    parser.add_argument("--public-key", required=True, type=Path)
    parser.add_argument("--input", required=True, type=Path)
    parser.add_argument("--policy", required=True, type=Path)
    args = parser.parse_args()
    try:
        verify(args.receipt, args.public_key, args.input, args.policy)
    except (ValueError, KeyError, TypeError, OSError) as error:
        parser.exit(1, f"REJECT: {error}\n")
    print("PASS: signature, pinned inputs/policy, bounded replay, reported side effects")
