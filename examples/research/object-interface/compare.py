"""Bounded comparison and independent checks; Python standard library only."""
import argparse
import ast
import hashlib
import json
import platform
import struct
import subprocess
import tempfile
from pathlib import Path


ALLOW = b"(policy (tier 1))"
DENY = b'(policy (tier 1) (require-axion-event (reason "absent")))'
CASES = {
    "authorized": "ok", "wrong_hash": "identity_denied",
    "wrong_representation": "representation_rejected",
    "unsupported": "adapter_unsupported", "capability_denial": "capability_denied",
    "policy_denial": "policy_denied", "changed_after_verify": "ok",
    "invalid_provenance": "ok", "incomplete_provenance": "ok",
}


def digest(data):
    # CanonHash81 implementation: first 32 bytes of SHA3-512, not SHA3-256.
    return hashlib.sha3_512(data).digest()[:32].hex()


def safetensors():
    header = json.dumps({"weight": {"dtype": "I8", "shape": [2, 2],
                                    "data_offsets": [0, 4]}}, separators=(",", ":")).encode()
    header += b" " * (-len(header) % 8)
    return struct.pack("<Q", len(header)) + header + bytes([255, 0, 1, 255])


def inspect_tensor(data):
    # Fixture-scoped conventional reader; no claim of general SafeTensors support.
    size, = struct.unpack("<Q", data[:8])
    tensors = json.loads(data[8:8 + size])
    assert tensors == {"weight": {"dtype": "I8", "shape": [2, 2], "data_offsets": [0, 4]}}
    assert data[8 + size:] == bytes([255, 0, 1, 255])
    return b"1:4\n"


def conventional(kind, scenario, source, expected, work):
    """Ordinary immutable bytes, hash store, explicit access checks, two providers."""
    data = source.read_bytes()
    identity = digest(data)
    result = dict(status="identity_denied", input_ref=identity, output_ref="",
                  provenance_ref="", completed_operations=0, inspection=None)
    if identity != expected:
        return result
    store = work / "store" / "objects"
    store.mkdir(parents=True)

    def put(payload):
        ref = digest(payload)
        path = store / (ref + ".blk")
        if not path.exists():
            with path.open("xb") as stream:
                stream.write(payload)
        assert path.read_bytes() == payload
        return ref

    put(data)
    (work / "restored").write_bytes(data)
    permissions = {identity: {"read", "write"}}
    if scenario == "capability_denial":
        permissions[identity] = {"write"}
    if "read" not in permissions[identity]:
        result["status"] = "capability_denied"
        return result
    policy = DENY if scenario == "policy_denial" else ALLOW
    if scenario == "unsupported":
        result["status"] = "adapter_unsupported"
        return result
    # Deliberately bounded policy language: only this exact allow policy admits.
    if policy != ALLOW:
        result["status"] = "policy_denied"
        return result
    if scenario == "changed_after_verify":
        source.write_bytes(data + b"changed")
    provider = ("safetensors" if kind == "tisc" else "tisc") if scenario == "wrong_representation" else kind
    try:
        if provider == "tisc":
            # Python source is the conventional executable representation. Both
            # arms compute 6*7; their executable bytes and engines differ.
            tree = ast.parse(data)
            assert ast.dump(tree) == ast.dump(ast.parse("result = 6 * 7\n"))
            namespace = {}
            exec(compile(tree, "<fixture>", "exec"), {"__builtins__": {}}, namespace)
            output = f"{namespace['result']}\n".encode()
            result["inspection"] = dict(representation="python-source", statements=len(tree.body))
        else:
            output = inspect_tensor(data)
            result["inspection"] = dict(representation="safetensors-i8", tensor="weight", shape=[2, 2])
    except (ValueError, SyntaxError, AssertionError, UnicodeError, struct.error):
        if scenario != "wrong_representation":
            raise
        result["status"] = "representation_rejected"
        return result
    result.update(status="ok", completed_operations=1, output_ref=put(output))
    (work / "output").write_bytes(output)
    record = (f"object-interface-experiment-v1\n{identity}\n{kind}\n"
              f"{'run' if kind == 'tisc' else 'inspect'}\n{digest(ALLOW)}\n"
              f"{result['output_ref']}\n").encode()
    if scenario == "invalid_provenance":
        record = record[:-65] + b"0" * 64 + b"\n"
    if scenario == "incomplete_provenance":
        record = b"object-interface-experiment-v1\n"
    result["provenance_ref"] = put(record)
    (work / "provenance").write_bytes(record)
    return result


def verify_record(record, ref, identity, kind, output):
    expected = ["object-interface-experiment-v1", identity, kind,
                "run" if kind == "tisc" else "inspect", digest(ALLOW), digest(output)]
    return digest(record) == ref and record.decode().splitlines() == expected


def run(runner, destination):
    destination.mkdir(parents=True, exist_ok=False)
    program = destination / "program.b81"
    subprocess.run([str(runner), "construct", str(program)], check=True)
    fixtures = {"tisc": program.read_bytes(), "safetensors": safetensors()}
    rows = []
    for arm in ("t81", "conventional"):
        for kind, original in fixtures.items():
            payload = b"result = 6 * 7\n" if arm == "conventional" and kind == "tisc" else original
            for scenario, expected_status in CASES.items():
                work = destination / f"{arm}-{kind}-{scenario}"
                work.mkdir()
                source = work / "source"
                source.write_bytes(payload)
                expected = "0" * 64 if scenario == "wrong_hash" else digest(payload)
                if arm == "t81":
                    proc = subprocess.run([str(runner), kind, scenario, str(source), expected, str(work)],
                                          check=True, capture_output=True, text=True, timeout=20)
                    row = json.loads(proc.stdout)
                else:
                    row = conventional(kind, scenario, source, expected, work)
                assert row["status"] == expected_status, (arm, kind, scenario, row)
                assert row["input_ref"] == digest(payload)
                blocks = [p.read_bytes() for p in (work / "store").rglob("*.blk")]
                if expected_status == "ok":
                    output = (work / "output").read_bytes()
                    assert output == (b"42\n" if kind == "tisc" else b"1:4\n")
                    record = (work / "provenance").read_bytes()
                    assert row["output_ref"] == digest(output)
                    assert digest(record) == row["provenance_ref"]
                    invalid = scenario in ("invalid_provenance", "incomplete_provenance")
                    assert verify_record(record, row["provenance_ref"], digest(payload), kind, output) is not invalid
                    assert sorted(blocks) == sorted([payload, output, record])
                    assert row["completed_operations"] == 1
                    if kind == "safetensors":
                        assert row["inspection"] == dict(representation="safetensors-i8", tensor="weight", shape=[2, 2])
                    elif arm == "t81":
                        assert row["inspection"] == dict(representation="base81-symbolic-tisc", instructions=4)
                    else:
                        assert row["inspection"] == dict(representation="python-source", statements=1)
                    # Invalid record with old hash, and incomplete record with a
                    # valid fresh hash: integrity and completeness are distinct.
                    assert not verify_record(record + b"false", row["provenance_ref"], digest(payload), kind, output)
                    incomplete = b"object-interface-experiment-v1\n"
                    assert not verify_record(incomplete, digest(incomplete), digest(payload), kind, output)
                    assert not verify_record(record, row["provenance_ref"], digest(payload), kind, output + b"changed")
                else:
                    assert not (work / "output").exists() and not (work / "provenance").exists()
                    assert blocks == ([] if scenario == "wrong_hash" else [payload])
                    assert row["completed_operations"] == 0
                if scenario != "wrong_hash":
                    assert (work / "restored").read_bytes() == payload
                if scenario == "changed_after_verify":
                    assert source.read_bytes() != payload and digest(source.read_bytes()) != row["input_ref"]
                rows.append(dict(arm=arm, kind=kind, scenario=scenario,
                                 stored_payloads_hex=[b.hex() for b in sorted(blocks)], **row))
    return dict(classification="Partially supported", platform=platform.platform(),
                python=platform.python_version(), rows=rows, checks="36 cases plus provenance/output tampering",
                baseline="8a6dce4831bd84e23d499fa71900e959ba940875",
                runner_sha256=hashlib.sha256(runner.read_bytes()).hexdigest(),
                source_sha256={p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                               for p in (Path(__file__), Path(__file__).with_name('runner.cpp'))})


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner", type=Path, required=True)
    parser.add_argument("--out", type=Path, help="new directory retaining raw evidence")
    args = parser.parse_args()
    if args.out:
        result = run(args.runner.resolve(), args.out)
        (args.out / "results.json").write_text(json.dumps(result, indent=2) + "\n")
    else:
        with tempfile.TemporaryDirectory() as temporary:
            result = run(args.runner.resolve(), Path(temporary) / "evidence")
    print(json.dumps(result, indent=2))
