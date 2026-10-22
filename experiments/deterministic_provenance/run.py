"""Reproduce the bounded experiment; keep timing/platform outside canonical evidence."""
import argparse
import json
import os
from pathlib import Path
import platform
import statistics
import subprocess
import time

from verify import canonical, digest, verify

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
CASES = {
    "allow": (40, 2, "allow"), "changed-input": (41, 2, "allow"),
    "deny-vm": (40, 2, "deny-vm"), "deny-store": (40, 2, "deny-store"),
    "division-zero": (40, 0, "allow"), "lower-bound": (0, 1, "allow"),
    "upper-bound": (100, 3, "allow"), "baseline": (40, 2, "baseline"),
}


def execute(runner, root, x, divisor, profile):
    env = {k: v for k, v in os.environ.items() if not k.startswith("T81_")}
    # No ambient JIT/storage/fault settings enter the admitted experiment.
    start = time.perf_counter_ns()
    run = subprocess.run([str(runner), str(x), str(divisor), profile, str(root)],
                         capture_output=True, check=True, timeout=30, env=env)
    process_ns = time.perf_counter_ns() - start
    raw = json.loads(run.stdout)
    envelope = {"record": raw["record"], "record_hash": digest(canonical(raw["record"]))}
    start = time.perf_counter_ns()
    verify(envelope, x, divisor, profile, root)
    raw["measurements"]["verification_ns"] = time.perf_counter_ns() - start
    raw["measurements"]["process_ns"] = process_ns
    raw["measurements"]["artifact_bytes"] = len(canonical(envelope))
    raw["measurements"]["stored_payload_bytes"] = sum(p.stat().st_size for p in root.rglob("*") if p.is_file())
    return envelope, raw


def describe(values):
    return {"n": len(values), "mean": statistics.mean(values), "median": statistics.median(values),
            "stdev": statistics.stdev(values) if len(values) > 1 else 0,
            "min": min(values), "max": max(values)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    parser.add_argument("--repetitions", type=int, default=30)
    args = parser.parse_args()
    if args.repetitions < 2:
        parser.error("at least two repetitions required")
    args.output.mkdir(parents=True, exist_ok=False)
    runner = args.runner.resolve()
    observations = {case: [] for case in CASES}
    signatures = {}
    unsupported = {case: [] for case in CASES}
    compiler = None
    # Rotate order each round to reduce fixed-order filesystem/thermal bias.
    cases = list(CASES)
    for round_id in range(args.repetitions):
        order = cases[round_id % len(cases):] + cases[:round_id % len(cases)]
        for case in order:
            x, divisor, profile = CASES[case]
            trial = args.output / f"{case}-{round_id}"
            trial.mkdir()
            evidence, raw = execute(runner, trial / "store", x, divisor, profile)
            data = canonical(evidence)
            (trial / "evidence.json").write_bytes(data + b"\n")
            if case in signatures and signatures[case] != data:
                raise ValueError(f"canonical evidence drift: {case}")
            signatures[case] = data
            observations[case].append(raw["measurements"])
            unsupported[case].append(raw["diagnostics"])
            compiler = raw["compiler"]
    if signatures["allow"] == signatures["changed-input"]:
        raise ValueError("input sensitivity failed")
    summary = {case: {key: describe([r[key] for r in rows]) for key in rows[0]}
               for case, rows in observations.items()}
    sha = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip()
    dirty = bool(subprocess.check_output(["git", "status", "--porcelain", "--untracked-files=no"], cwd=ROOT))
    sources = [HERE / name for name in ("runner.cpp", "verify.py", "run.py", "test_experiment.py")]
    baseline = summary["baseline"]["execution_ns"]["mean"]
    report = {
        "canonical_evidence_version": 2,
        "conversion_status": "UNRESOLVED: excluded from canonical evidence; see float_conversion_audit",
        "unsupported_conversion_observations": unsupported,
        "git_sha": sha, "tracked_tree_dirty": dirty,
        "source_hashes": {str(p.relative_to(ROOT)): digest(p.read_bytes()) for p in sources},
        "runner_hash": digest(runner.read_bytes()),
        "platform": {"system": platform.system(), "release": platform.release(), "machine": platform.machine(),
                     "compiler": compiler, "python": platform.python_version()},
        "configuration": {"strict_float": True, "jit": False, "max_steps": 64,
                          "store": "fresh persistent CanonFS per sample", "ambient_T81_variables": "removed"},
        "methodology": {"repetitions": args.repetitions, "warmup": "five VM/hash runs inside every process",
                        "order": "rotate cases each round", "workload": "12 instructions; taken branch skips one",
                        "timing": "steady_clock nanoseconds; process_ns includes startup/warmup; verification reported separately",
                        "execution": "run_to_halt only; includes built-in trace generation and Axion audit",
                        "serialization_hash": "actual trace/audit/output serialization plus trace hash",
                        "total": "program setup + VM execution + serialization/hash + storage write/readback + numeric probes + record JSON/digests; excludes stdout and Python envelope hashing",
                        "memory": "whole-process peak RSS from getrusage on macOS/Linux (-1 if unavailable); retained vector capacities exclude string allocations",
                        "storage": "directory creation + write authorization + payload write + allowed readback; no fsync guarantee"},
        "verifier": "PASS for every run, including actual store inspection",
        "canonical_repeatability": "PASS: exact bytes equal per case",
        "evidence_hashes": {case: digest(data) for case, data in signatures.items()},
        "measurements": summary, "samples": observations,
        "policy_increment_percent": (summary["allow"]["execution_ns"]["mean"] / baseline - 1) * 100,
        "known_exclusions": [
            "No cross-platform identity established; this report is one native host only",
            "WASM NOT TESTED; other native architectures/OSes NOT TESTED in this report",
            "Built-in trace generation cannot be timed separately through the stable VM API",
            "Peak RSS includes startup/warmup; no marginal allocator measurement",
            "No host libm comparison, transcendental accuracy claim, AI inference, signatures or remote attestation",
            "INT64_MIN / -1, helper signed overflow and negative BigInt cancellation excluded pending PR #536 audit/fixes",
            "All T81Float from_double probes are unsupported noncanonical diagnostics; no conversion correctness or identity claim",
            "Unsigned evidence needs caller-pinned workload/profile and trusted verifier; unreported external side effects cannot be proven absent",
            "Less-than-5-percent overhead remains an untested inference/libm hypothesis",
        ],
    }
    (args.output / "report.json").write_bytes(canonical(report) + b"\n")
    print(json.dumps({"report": str(args.output / "report.json"), "verifier": report["verifier"],
                      "policy_increment_percent": report["policy_increment_percent"]}, indent=2))


if __name__ == "__main__":
    main()
