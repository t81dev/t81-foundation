"""Three-arm fixture experiment; standard library only; run without python -O."""
import argparse
import hashlib
import importlib.util
import json
import platform
import subprocess
import tempfile
from pathlib import Path

HERE = Path(__file__).resolve().parent
SPEC = importlib.util.spec_from_file_location("baseline", HERE.parent / "object-interface/compare.py")
BASE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(BASE)
SCHEMA = "t81.experiment-descriptor.v1"
CONSTRAINT = "object-interface-fixtures-v1"
META = {"tisc": ("base81-symbolic-tisc", "t81vm", "run"),
        "safetensors": ("safetensors-i8", "native-weights", "inspect")}
CASES = {
    "valid": "ok", "wrong_reference": "payload_denied",
    "wrong_representation": "representation_denied", "unsupported": "unsupported",
    "false_capability": "capability_denied", "payload_substitution": "payload_denied",
    "descriptor_substitution": "descriptor_denied", "version": "version_denied",
    "malformed": "malformed", "source_mutation": "ok",
    "operation_denial": "operation_denied", "storage_denial": "storage_denied",
    "wrong_provider": "provider_denied", "constraint": "constraint_denied",
    "missing_field": "malformed", "extra_field": "malformed",
    "invalid_reference": "malformed", "missing_newline": "malformed",
    "duplicate_operations": "capability_denied", "empty_field": "malformed",
    "crlf": "malformed", "non_ascii": "malformed", "oversize": "malformed",
    "corrupt_payload": "representation_denied",
}


def check(condition, context):
    if not condition:
        raise RuntimeError(f"check failed: {context}")


def digest(data):
    return hashlib.sha3_512(data).digest()[:32].hex()


def decode(raw):
    try:
        check(len(raw) <= 4096 and raw.endswith(b"\n"), "encoding")
        fields = raw.decode("ascii").split("\n")
        check(len(fields) == 7 and fields[-1] == "", "fields")
        fields.pop()
        check(all(f and all(33 <= ord(c) <= 126 for c in f) for f in fields), "printable")
        check(len(fields[1]) == 64 and all(c in "0123456789abcdef" for c in fields[1]), "ref")
        return fields
    except (UnicodeError, RuntimeError) as error:
        raise ValueError("malformed") from error


def conventional(raw, expected_descriptor, source, expected_payload, operation, authorization,
                 scenario, work, fixtures):
    """Immutable bytes + content store + explicit preflight + two fixed providers."""
    data = source.read_bytes()
    identity = digest(data)
    store = work / "store/objects"
    store.mkdir(parents=True)
    with (store / (identity + ".blk")).open("xb") as stream:
        stream.write(data)
    (work / "restored").write_bytes(data)
    row = dict(payload_ref=identity, descriptor_ref=digest(raw), supported="",
               provider_invocations=0, storage_succeeded=True)

    def run_tisc(snapshot):
        # Fixture-scoped conventional interpretation of the SAME TISC bytes.
        # No Python source execution, full TISC decoder, or production VM parity.
        check(snapshot == fixtures["tisc"], "program bytes")
        registers = {40: 6, 41: 7}
        registers[42] = registers[40] * registers[41]
        return f"{registers[42]}\n".encode()

    def inspect(snapshot):
        return BASE.inspect_tensor(snapshot)

    adapters = {
        ("base81-symbolic-tisc", "t81vm"): ("run", fixtures["tisc"], run_tisc),
        ("safetensors-i8", "native-weights"): ("inspect", fixtures["safetensors"], inspect),
    }
    try:
        if digest(raw) != expected_descriptor:
            raise ValueError("descriptor_denied")
        fields = decode(raw)
        if fields[0] != SCHEMA:
            raise ValueError("version_denied")
        if fields[1] != expected_payload or identity != expected_payload:
            raise ValueError("payload_denied")
        if fields[5] != CONSTRAINT:
            raise ValueError("constraint_denied")
        adapter = adapters.get((fields[2], fields[3]))
        if adapter is None:
            raise ValueError("provider_denied")
        supported, fixture, invoke = adapter
        if data != fixture:
            raise ValueError("representation_denied")
        if fields[4] != supported:
            raise ValueError("capability_denied")
        row["supported"] = supported
        if operation != supported:
            raise ValueError("unsupported")
        permissions = {identity: {"read", "write"}}
        if scenario == "storage_denial":
            permissions[identity] = {"write"}
        if "read" not in permissions[identity]:
            raise ValueError("storage_denied")
        # Bounded exact policy allowlist, not a replacement Axion implementation.
        if authorization != "allow":
            raise ValueError("operation_denied")
        if scenario == "source_mutation":
            source.write_bytes(data + b"changed")
        row["provider_invocations"] += 1
        (work / "output").write_bytes(invoke(data))
        row["status"] = "ok"
    except ValueError as error:
        row["status"] = str(error)
    return row


def challenge(kind, scenario, fixtures):
    payload = fixtures[kind]
    representation, provider, operation = META[kind]
    fields = [SCHEMA, digest(payload), representation, provider, operation, CONSTRAINT]
    expected_payload = digest(payload)
    expected_descriptor = None
    authorization = "deny" if scenario == "operation_denial" else "allow"
    if scenario == "wrong_reference":
        fields[1] = "0" * 64
    elif scenario == "wrong_representation":
        fields[2:4] = META["safetensors" if kind == "tisc" else "tisc"][:2]
    elif scenario == "unsupported":
        operation = "infer"
    elif scenario == "false_capability":
        fields[4] = operation = "infer"
    elif scenario == "payload_substitution":
        payload = fixtures["safetensors" if kind == "tisc" else "tisc"]
        # Even an internally consistent substituted descriptor cannot override
        # the independently pinned caller payload reference.
        fields[1] = digest(payload)
    elif scenario == "descriptor_substitution":
        expected_descriptor = digest(("\n".join(fields) + "\n").encode())
        fields[4] = "infer"
    elif scenario == "version":
        fields[0] = "t81.experiment-descriptor.v2"
    elif scenario == "wrong_provider":
        fields[3] = "unregistered"
    elif scenario == "constraint":
        fields[5] = "unbounded"
    elif scenario == "invalid_reference":
        fields[1] = "z" * 64
    elif scenario == "duplicate_operations":
        fields[4] += "," + fields[4]
    elif scenario == "empty_field":
        fields[3] = ""
    elif scenario == "missing_field":
        fields.pop()
    elif scenario == "extra_field":
        fields.append("extra")
    elif scenario == "corrupt_payload":
        payload += b"corruption"
        fields[1] = expected_payload = digest(payload)
    raw = ("\n".join(fields) + "\n").encode()
    if scenario == "malformed":
        raw = b"{broken"
    elif scenario == "missing_newline":
        raw = raw[:-1]
    elif scenario == "crlf":
        raw = raw.replace(b"\n", b"\r\n")
    elif scenario == "non_ascii":
        raw = raw.replace(b"t81vm", b"t81\xffvm").replace(b"native-weights", b"native-\xffweights")
    elif scenario == "oversize":
        raw = b"x" * 4097 + b"\n"
    return raw, expected_descriptor or digest(raw), payload, expected_payload, operation, authorization


def run(runner, baseline_runner, destination):
    check(__debug__, "baseline assertions require Python without -O")
    destination.mkdir(parents=True, exist_ok=False)
    # Run PR #540 without edits, including its conventional control and verifier.
    baseline = BASE.run(baseline_runner, destination / "baseline")
    check(len(baseline["rows"]) == 36, "baseline cases")
    fixtures = {"tisc": (destination / "baseline/program.b81").read_bytes(),
                "safetensors": BASE.safetensors()}
    rows = []
    for arm in ("B", "C"):
        for kind in fixtures:
            for scenario, expected_status in CASES.items():
                work = destination / f"{arm}-{kind}-{scenario}"
                work.mkdir()
                raw, expected_descriptor, payload, expected_payload, operation, authorization = challenge(
                    kind, scenario, fixtures)
                source = work / "source"
                source.write_bytes(payload)
                descriptor = work / "descriptor"
                descriptor.write_bytes(raw)
                if arm == "B":
                    proc = subprocess.run([str(runner), str(descriptor), expected_descriptor, str(source),
                                           expected_payload, operation, authorization, scenario, str(work)],
                                          capture_output=True, check=True, text=True, timeout=20)
                    row = json.loads(proc.stdout)
                else:
                    row = conventional(raw, expected_descriptor, source, expected_payload, operation,
                                       authorization, scenario, work, fixtures)
                check(row["status"] == expected_status, (arm, kind, scenario, row))
                check(row["payload_ref"] == digest(payload), "payload identity")
                check(row["descriptor_ref"] == digest(raw), "descriptor identity")
                check(digest(raw) != digest(payload), "identities separate")
                discovered = META[kind][2] if scenario in (
                    "valid", "unsupported", "operation_denial", "storage_denial", "source_mutation") else ""
                check(row["supported"] == discovered, "validated operation discovery")
                check(row["storage_succeeded"], "storage accepted bytes")
                blocks = [p.read_bytes() for p in (work / "store").rglob("*.blk")]
                check(blocks == [payload], "independent persistent store inspection")
                check((work / "restored").read_bytes() == payload, "verified intake")
                ok = expected_status == "ok"
                check(row["provider_invocations"] == int(ok), "attempted invocation boundary")
                check((work / "output").exists() == ok, "no prohibited output")
                if ok:
                    check((work / "output").read_bytes() == (b"42\n" if kind == "tisc" else b"1:4\n"),
                          "independent expected output")
                if scenario == "source_mutation":
                    check(source.read_bytes() == payload + b"changed", "source changed")
                rows.append(dict(arm=arm, kind=kind, scenario=scenario,
                                 descriptor_hex=raw.hex(), expected_descriptor=expected_descriptor,
                                 expected_payload=expected_payload, payload_hex=payload.hex(), **row))
    for b, c in zip(rows[:len(rows) // 2], rows[len(rows) // 2:]):
        check({k: v for k, v in b.items() if k != "arm"} ==
              {k: v for k, v in c.items() if k != "arm"}, "B/C differential agreement")
    sources = [HERE / "runner.cpp", HERE / "compare.py", HERE.parent / "object-interface/runner.cpp",
               HERE.parent / "object-interface/compare.py"]
    return dict(classification="Unsupported", recommendation="Abandon descriptor adoption; retain explicit providers",
                baseline="b4860a8a3065a679c2f5f12896cb1990d1633470", platform=platform.platform(),
                python=platform.python_version(), checks=f"36 baseline + {len(rows)} descriptor cases",
                baseline_results=baseline, rows=rows,
                source_sha256={str(p.relative_to(HERE.parent)): hashlib.sha256(p.read_bytes()).hexdigest()
                               for p in sources},
                runner_sha256=hashlib.sha256(runner.read_bytes()).hexdigest(),
                added_executable_lines=sum(len(p.read_text().splitlines()) for p in sources[:2]),
                structural_audit={
                    "definition": "Manual source audit: production provider-selection decisions, excluding attack generation",
                    "A": {"caller_dispatch_decisions": 1, "provider_adapter_paths": 2,
                          "descriptor_parser_paths": 0, "fixture_preflight_paths": 0},
                    "B": {"caller_dispatch_decisions": 0, "provider_adapter_paths": 2,
                          "descriptor_parser_paths": 1, "fixture_preflight_paths": 2},
                    "C": {"caller_dispatch_decisions": 0, "provider_adapter_paths": 2,
                          "descriptor_parser_paths": 1, "fixture_preflight_paths": 2},
                    "removed_provider_validators": 0, "new_external_dependencies": 0})


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner", type=Path, required=True)
    parser.add_argument("--baseline-runner", type=Path, required=True)
    parser.add_argument("--out", type=Path)
    args = parser.parse_args()
    if args.out:
        result = run(args.runner.resolve(), args.baseline_runner.resolve(), args.out)
        (args.out / "results.json").write_text(json.dumps(result, indent=2) + "\n")
    else:
        with tempfile.TemporaryDirectory() as temporary:
            result = run(args.runner.resolve(), args.baseline_runner.resolve(), Path(temporary) / "evidence")
    print(json.dumps(result, indent=2))
