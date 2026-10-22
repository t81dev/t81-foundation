"""Record native production failures against an exact independent reference.

A completed audit is not a numerical conformance pass. Unsupported conversion
cases stay outside canonical provenance. No platform-specific expected outputs.
"""
import argparse
import hashlib
import json
from pathlib import Path
import platform
import subprocess

from oracle import binary64, decode, reference

HERE = Path(__file__).resolve().parent


def observe(probe, corpus):
    run = subprocess.run([str(probe)], input="\n".join(corpus.values()) + "\n",
                         text=True, capture_output=True, check=True, timeout=30)
    rows = {}
    lines = run.stdout.splitlines()
    if len(lines) != len(corpus):
        raise ValueError("incomplete production probe")
    for (name, bits), line in zip(corpus.items(), lines):
        columns = line.split("\t")
        if len(columns) != 8 or columns[0] != bits:
            raise ValueError("malformed production probe")
        expectation = reference(bits)
        kind, exact = binary64(bits)
        try:
            representation = decode(columns[1])
        except ValueError:
            representation = None
        rows[name] = {
            "bits": bits, "production_canonical": columns[1], "reference": expectation,
            "represents_exact_input": representation is not None and representation == exact,
            "matches_nearest_candidate": columns[1] == expectation["candidate"],
            "lossless_contract_satisfied": expectation["status"] == "exact" and columns[1] == expectation["candidate"],
            "input_class": kind,
            "intermediates": dict(zip(("host_exp", "unsigned_scale", "signed_scale", "power_finite",
                                        "signed_power_finite", "signed_scaled_finite"), columns[2:])),
        }
    return {"compiler": run.stderr.strip(), "cases": rows}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--probe", required=True, type=Path)
    parser.add_argument("--unoptimized-probe", type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    corpus = json.loads((HERE / "corpus.json").read_text())
    report = observe(args.probe.resolve(), corpus)
    if args.unoptimized_probe:
        other = observe(args.unoptimized_probe.resolve(), corpus)
        report["unoptimized"] = other
        report["optimization_changes_canonical_outputs"] = any(
            report["cases"][k]["production_canonical"] != other["cases"][k]["production_canonical"] for k in corpus)
    header = HERE.parents[1] / "include/t81/types/T81Float.hpp"
    historical = json.loads((HERE.parent / "deterministic_provenance/fixture.json").read_text())
    sequence = ("positive-zero", "negative-zero", "quiet-nan", "positive-infinity", "negative-infinity",
                "denorm-min", "min-normal", "max-finite", "negative-max-finite")
    observed_legacy = "".join(report["cases"][name]["production_canonical"] + "\n" for name in sequence)
    report["v1_macos_fixture_would_reject_this_platform"] = observed_legacy != historical["record"]["float_conversions"]
    report["source_sha256"] = {
        "production_header": hashlib.sha256(header.read_bytes()).hexdigest(),
        "probe": hashlib.sha256((HERE / "probe.cpp").read_bytes()).hexdigest(),
        "probe_binary": hashlib.sha256(args.probe.read_bytes()).hexdigest(),
        "oracle": hashlib.sha256((HERE / "oracle.py").read_bytes()).hexdigest(),
        "corpus": hashlib.sha256((HERE / "corpus.json").read_bytes()).hexdigest(),
    }
    report.update({"platform": {"system": platform.system(), "machine": platform.machine(),
                                "release": platform.release(), "python": platform.python_version()},
                   "git_sha": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=HERE, text=True).strip(),
                   "production_status": "UNRESOLVED: excluded from cross-platform canonical provenance",
                   "production_before_after": "UNCHANGED: no numerical production correction in this PR",
                   "oracle_profile": "checked exact binary64 -> balanced ternary; nearest-even candidate diagnostic only",
                   "excluded_from_cross_platform_provenance": True})
    if report["cases"]["max-finite"]["matches_nearest_candidate"]:
        raise ValueError("known production failure changed; review exclusion and audit")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n")
    print(json.dumps(report, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
