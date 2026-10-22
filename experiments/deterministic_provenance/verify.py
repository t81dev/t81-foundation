"""Independent bounded replay: Python stdlib only; no producer/runtime import.

Caller pins input/profile. Hashes establish integrity, not producer identity or
absence of unreported writes. This is deliberately not a general TISC interpreter.
"""
import argparse
import hashlib
import json
from pathlib import Path

ALPHABET = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-−×÷=<>≤≥≠≈∞λμπστωΓ"


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":"), ensure_ascii=True, allow_nan=False).encode("ascii")


def digest(data):
    if isinstance(data, str):
        data = data.encode("ascii")
    return hashlib.sha3_512(data).digest()[:32].hex()


def address(data):
    n, result = int(digest(data), 16), ""
    while n:
        n, digit = divmod(n, 81)
        result = ALPHABET[digit] + result
    return result or "0"


def program(x, divisor):
    rows = [("LoadImm",40,x,0), ("LoadImm",41,2,0), ("Add",42,40,41),
            ("LoadImm",43,3,0), ("Mul",42,42,43), ("LoadImm",46,divisor,0),
            ("Div",44,42,46), ("Mod",45,42,43), ("JumpIfZero",10,45,0),
            ("LoadImm",44,999,0), ("Store",1024,44,0), ("Halt",0,0,0)]
    return "".join(f"{op} {a} {b} {c} 0\n" for op, a, b, c in rows)


def unique_object(pairs):
    obj = {}
    for key, value in pairs:
        if key in obj:
            raise ValueError("duplicate JSON key")
        obj[key] = value
    return obj


def load(path):
    return json.loads(Path(path).read_bytes(), object_pairs_hook=unique_object,
                      parse_constant=lambda _: (_ for _ in ()).throw(ValueError("nonfinite JSON")))


def verify(envelope, x, divisor, profile, store=None):
    if type(x) is not int or not 0 <= x <= 100 or type(divisor) is not int or not 0 <= divisor <= 3:
        raise ValueError("input outside bounded domain")
    if profile not in ("allow", "deny-vm", "deny-store", "baseline"):
        raise ValueError("unsupported policy identifier")
    if set(envelope) != {"record", "record_hash"}:
        raise ValueError("invalid envelope")
    r = envelope["record"]
    if envelope["record_hash"] != digest(canonical(r)):
        raise ValueError("evidence hash mismatch")
    # Every field is checked; attacker-rehashed records still require replay.
    expected = {
        "version": 1, "input": f"{x} {divisor}\n", "profile": profile,
        "program": program(x, divisor),
        "vm_policy": "" if profile == "baseline" else
            f"(policy (tier 1) (max-instructions {9 if profile == 'deny-vm' else 64}))\n",
        "storage_policy": "(policy (tier 1) (max-meta-writes 0))\n" if profile == "deny-store"
            else "" if profile == "baseline" else "(policy (tier 1))\n",
    }
    # Fixed, independently understood admitted path. Store is the tenth dispatch;
    # budget 9 denies it before dispatch, so it is absent from the VM trace.
    names = ["LoadImm", "LoadImm", "Add", "LoadImm", "Mul", "LoadImm", "Div", "Mod", "JumpIfZero", "LoadImm", "Store", "Halt"]
    pcs = list(range(7)) if divisor == 0 else list(range(9))
    if divisor:
        pcs += [] if profile == "deny-vm" else [10, 11]
    trap = "DivisionFault" if divisor == 0 else "SecurityFault" if profile == "deny-vm" else "None"
    expected["trap"] = trap
    expected["trace"] = "".join(f"{pc} {names[pc]} {'DivisionFault' if pc == 6 and divisor == 0 else 'None'}\n" for pc in pcs)
    expected["output"] = f"{(x + 2) * 3 // divisor}\n" if divisor and profile != "deny-vm" else "0\n"
    expected["reference"] = address(expected["output"]) if trap == "None" and profile != "deny-store" else ""
    # Exact policy audit and conversion fixture constants are pinned by this
    # verifier implementation, never selected by the evidence producer.
    expected["decisions"] = expected_decisions(trap)
    expected["float_conversions"] = FLOAT_CONVERSIONS
    expected["integer_boundaries"] = f"{2**63}\n{(2**63-1)*3}\n{2**63}\n"
    for field in ("input", "program", "vm_policy", "storage_policy", "trace", "output"):
        expected[field + "_hash"] = digest(expected[field])
    if canonical(r) != canonical(expected):
        differing = sorted(k for k in set(r) | set(expected) if r.get(k) != expected.get(k))
        raise ValueError("replay mismatch: " + ", ".join(differing))
    if store is not None:
        root = Path(store)
        files = {p.relative_to(root).as_posix(): p.read_bytes() for p in root.rglob("*") if p.is_file()}
        # Persistent driver stores raw payloads, with base81 filenames.
        expected_files = {"objects/" + r["reference"] + ".blk": r["output"].encode()} if r["reference"] else {}
        if files != expected_files:
            raise ValueError("actual storage differs from verified side effects")
    return r


# Filled from the documented conversion surface; golden values are regression
# observations, not a proof of correctly rounded math across the full domain.
FLOAT_CONVERSIONS = "+0E0\n+0E0\nNaE\n+Inf\n-Inf\n+0E0\n+0E0\n+0E0\n+0E0\n"


def expected_decisions(trap):
    if trap == "DivisionFault":
        return ""
    prefix = "Nop 0 meta slot axion event segment=meta addr=1292\n"
    return prefix + ("Store 1 Instruction count limit exceeded: count=10 limit=9\n"
                     if trap == "SecurityFault" else "Store 0 memory store heap addr=1024\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("evidence", type=Path)
    parser.add_argument("--input", required=True, type=int)
    parser.add_argument("--divisor", required=True, type=int)
    parser.add_argument("--profile", required=True)
    parser.add_argument("--store", type=Path)
    args = parser.parse_args()
    try:
        verify(load(args.evidence), args.input, args.divisor, args.profile, args.store)
    except (ValueError, KeyError, TypeError, OSError) as error:
        parser.exit(1, f"REJECT: {error}\n")
    print("PASS: pinned workload, policy, ordered trace, result, content reference" +
          (", actual storage" if args.store is not None else ""))
