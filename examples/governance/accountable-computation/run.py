"""Run the three-arm precursor experiment; no third-party Python packages."""
import argparse
import hashlib
import json
import platform
from pathlib import Path
import statistics
import subprocess
import sys
import tempfile
import time

ALLOW = b"(policy (tier 1))\n"
DENY = b'(policy (tier 1) (require-axion-event (reason "commit-authorized")))\n'
INVALID = b"invalid-policy\n"


def digest(data):
    return hashlib.sha3_512(data).digest()[:32].hex()


def canonical(record):
    return json.dumps(record, sort_keys=True, separators=(",", ":"), ensure_ascii=True).encode()


def baseline(payload, expected, initial_policy, store, revoked=None):
    # Conventional SHA3, exact policy allowlist, integer arithmetic, exclusive write.
    policy = initial_policy
    status, amount, calls, objects = "identity_denied", 0, 0, []
    if digest(payload) == expected:
        units, price = map(int, payload.split())
        if not 1 <= units <= 1000 or not 1 <= price <= 1000:
            raise ValueError("input range")
        amount = units * price
        policy = revoked if revoked is not None else policy
        calls = 1
        status = "policy_denied"
        if policy == ALLOW:
            store.mkdir()
            output = f"{amount}\n".encode()
            with (store / (digest(output) + ".blk")).open("xb") as stream:
                stream.write(output)
            objects = [output]
            status = "committed"
    return dict(status=status, amount=amount, input_digest=digest(payload),
                policy_digest=digest(policy), output_digest=digest(objects[0]) if objects else "",
                vm_policy_calls=0, policy_calls=calls)


def sign(record, key):
    with tempfile.TemporaryDirectory() as temp:
        path = Path(temp) / "record"
        path.write_bytes(canonical(record))
        sig = subprocess.check_output(["openssl", "pkeyutl", "-sign", "-inkey", str(key),
                                       "-rawin", "-in", str(path)])
    return {"record": record, "signature": sig.hex()}


def run(runner, destination, repetitions=30):
    destination.mkdir(parents=True, exist_ok=False)
    key = destination / "ephemeral-test-key.pem"
    public = destination / "public.pem"
    subprocess.run(["openssl", "genpkey", "-algorithm", "ED25519", "-out", str(key)], check=True)
    subprocess.run(["openssl", "pkey", "-in", str(key), "-pubout", "-out", str(public)],
                   check=True, capture_output=True)
    payload = b"25 1000\n"
    scenarios = {
        "authorized": (payload, digest(payload), ALLOW, None),
        "changed_bytes": (b"26 1000\n", digest(payload), ALLOW, None),
        "wrong_hash": (payload, "0" * 64, ALLOW, None),
        "denied": (payload, digest(payload), DENY, None),
        "revoked": (payload, digest(payload), ALLOW, DENY),
        "invalid_policy": (payload, digest(payload), INVALID, None),
    }
    rows = []
    for case, (data, expected, policy, revoke) in scenarios.items():
        for arm in ("A", "B", "C"):
            timings, verify_times = [], []
            for repetition in range(repetitions):
                folder = destination / f"{arm}-{case}-{repetition}"
                folder.mkdir()
                source, active, revoked = folder / "input", folder / "policy", folder / "revoked"
                source.write_bytes(data)
                active.write_bytes(policy)
                if revoke is not None:
                    revoked.write_bytes(revoke)
                store = folder / "store"
                begin = time.perf_counter_ns()
                if arm == "A":
                    observation = baseline(data, expected, policy, store, revoke)
                else:
                    result = subprocess.run([str(runner), arm, str(source), expected, str(active),
                                             str(store), str(revoked) if revoke is not None else "-"],
                                            check=True, capture_output=True, text=True)
                    observation = json.loads(result.stdout)
                timings.append((time.perf_counter_ns() - begin) / 1e6)
                objects = [{"name": p.name, "hex": p.read_bytes().hex()}
                           for p in sorted(store.rglob("*.blk"))] if store.exists() else []
                should_commit = case == "authorized"
                boundary_pass = (len(objects) == int(should_commit) and
                                 (observation["status"] == "committed") == should_commit)
                record = dict(version=1, arm=arm, case=case, input_hex=data.hex(),
                              expected_digest=expected, policy_hex=(revoke if revoke is not None else policy).hex(),
                              observation=observation, objects=objects, boundary="result-object creation",
                              method="bounded-integer-product-v1")
                receipt = folder / "receipt.json"
                receipt.write_text(json.dumps(sign(record, key), indent=2) + "\n")
                begin = time.perf_counter_ns()
                verified = subprocess.run([sys.executable, str(Path(__file__).with_name("verify.py")),
                                           str(receipt), "--public-key", str(public), "--input", str(source),
                                           "--policy", str(revoked if revoke is not None else active)],
                                          capture_output=True, text=True)
                verify_times.append((time.perf_counter_ns() - begin) / 1e6)
                if repetition == 0:
                    first = record
                    verification = verified.returncode == 0
                elif record != first or (verified.returncode == 0) != verification:
                    raise RuntimeError("replay mismatch across repetitions")
                if boundary_pass != verification:
                    raise RuntimeError("boundary/verifier disagreement")
            rows.append(dict(arm=arm, case=case, repetitions=repetitions,
                             result="pass" if verification else "fail", object_count=len(first["objects"]),
                             observation=first["observation"],
                             runner_ms=dict(median=statistics.median(timings), minimum=min(timings),
                                            maximum=max(timings), stdev=statistics.stdev(timings) if repetitions > 1 else 0,
                                            samples=timings),
                             verifier_ms=dict(median=statistics.median(verify_times), maximum=max(verify_times),
                                              samples=verify_times)))
    key.unlink()  # Ephemeral harness attestation, never a repository trust root.
    summary = dict(base_commit="d1c24670470e74412ab96182a177a47e7127c68a", environment=platform.platform(),
                   machine=platform.machine(), python=sys.version, openssl=subprocess.check_output(
                       ["openssl", "version"], text=True).strip(), input_hex=payload.hex(),
                   input_digest=digest(payload), memory="not measured", performance_comparison="inconclusive: Python in-process vs C++ subprocess+VM",
                   runner_sha256=hashlib.sha256(runner.read_bytes()).hexdigest(),
                   runner_bytes=runner.stat().st_size,
                   implementation_lines={name: len(Path(__file__).with_name(name).read_text().splitlines())
                                         for name in ("runner.cpp", "run.py", "verify.py")},
                   source_sha256={name: hashlib.sha256(Path(__file__).with_name(name).read_bytes()).hexdigest()
                                  for name in ("runner.cpp", "run.py", "verify.py")},
                   recommendation="Revise", rows=rows)
    (destination / "results.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))
    return summary


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--repetitions", type=int, default=30)
    args = parser.parse_args()
    if args.repetitions < 1:
        parser.error("repetitions must be positive")
    run(args.runner.resolve(), args.output.resolve(), args.repetitions)
