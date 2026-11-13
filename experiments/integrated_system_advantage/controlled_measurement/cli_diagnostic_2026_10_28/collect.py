"""EXPLORATORY — NONCONFIRMATORY. Ten diagnostic pairs plus ten version controls.

Never imports the confirmatory analyzer or schedule. Existing validation workers
are reused unchanged as correctness executors; their diagnostic timestamps are
not measurements. Only this wrapper's explicitly labeled clock intervals are.
"""
import argparse
import base64
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import signal
import statistics
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
CONTROL = HERE.parent
REPO = CONTROL.parents[2]
sys.path.insert(0, str(CONTROL))
from apparatus import AblationHarness, Journal, ROLES

LABEL = "EXPLORATORY — NONCONFIRMATORY"
SEED = 6281029


def utc():
    return datetime.now(timezone.utc).isoformat()


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def write_json(path, value):
    with Path(path).open("x") as stream:
        json.dump(value, stream, indent=2, ensure_ascii=False, allow_nan=False)
        stream.write("\n")
        stream.flush()
        os.fsync(stream.fileno())


def probe(command):
    result = subprocess.run(command, capture_output=True, timeout=30)
    return dict(command=command, returncode=result.returncode,
                stdout=result.stdout.decode(errors="replace"),
                stderr=result.stderr.decode(errors="replace"))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", required=True)
    parser.add_argument("--out", required=True, help="Must not exist; all attempts/stores retained")
    args = parser.parse_args()
    out = Path(args.out).resolve()
    out.mkdir(parents=True, exist_ok=False)
    events = out / "attempts.jsonl"
    events.touch(exist_ok=False)

    def event(kind, **fields):
        record = dict(label=LABEL, purpose="exploratory_nonconfirmatory",
                      confirmatory=False, eligible_for_preregistered_dataset=False,
                      kind=kind, utc=utc(), monotonic_ns=time.monotonic_ns())
        record.update(fields)
        with events.open("a") as stream:
            stream.write(json.dumps(record, ensure_ascii=False, allow_nan=False) + "\n")
            stream.flush()
            os.fsync(stream.fileno())

    event("preparation_start")
    try:
        build = Path(args.build_dir).resolve()
        binaries = [build / name for name in
                    ("t81", "t81_make_assess_fixed_demo", "t81_investigation08_probe")]
        prior = json.loads((CONTROL / "validation/report.json").read_text())
        for binary in binaries:
            if digest(binary) != prior["binary_sha256"][binary.name]:
                raise ValueError("Binary differs from validated host build: " + str(binary))
        harness = AblationHarness(REPO, out / "fixtures", build)
        fixtures = harness.prepare_fixtures()
        pinned = list(CONTROL.glob("*.py")) + list(CONTROL.glob("*.json"))
        pinned += list(CONTROL.glob("*.md")) + list(CONTROL.parent.glob("*.py"))
        pinned += binaries + [Path(sys.executable).resolve(), Path(__file__).resolve(), HERE / "worker.py"]
        expected_roles = json.loads((CONTROL / "exploratory_2026_10_28/artifact-identities.json").read_text())["roles_stable_in_all_ten_pairs"]
        pinned += [CONTROL / "exploratory_2026_10_28/artifact-identities.json"]
        pinned += [Path(fixtures[k]) for k in ("model_path", "apl_policy_path", "tokenizer_path")]
        # Pin loaded Python dependencies; no ambient environment or secrets collected.
        pinned += [Path(m.__file__).resolve() for m in list(sys.modules.values())
                   if getattr(m, "__file__", None) and Path(m.__file__).is_file()]
        hashes = {str(p): digest(p) for p in sorted(set(pinned))}
        orders = [["A", "B"] for _ in range(5)] + [["B", "A"] for _ in range(5)]
        random.Random(SEED).shuffle(orders)
        write_json(out / "plan.json", dict(label=LABEL, confirmatory=False,
                   eligible_for_preregistered_dataset=False, candidate="05639a677b11ea1f4ef0328b3f67140784a4ead1",
                   seed=SEED, orders=orders, input="greet hello", condition="empty_store",
                   max_seconds=1800, pairs=10, version_controls=10,
                   timing="Outer: immediately before Popen through communicate return. Nested: invoking parent immediately before Popen through communicate return; diagnostic and original journal fsync excluded from nested intervals, included in outer time. Verify sequential containment before summing.",
                   fixtures=fixtures, pinned_sha256=hashes))
        commands = [["uname", "-a"], ["sw_vers"],
                    ["sysctl", "-n", "hw.model", "hw.memsize", "machdep.cpu.brand_string"],
                    ["pmset", "-g", "batt"], ["pmset", "-g", "therm"],
                    ["df", "-h", str(out)], ["mount"],
                    ["/usr/bin/clang++", "--version"], ["otool", "-L", str(binaries[0])]]
        write_json(out / "environment-before.json", dict(label=LABEL, utc=utc(),
                   platform=platform.platform(), python=sys.version, executable=sys.executable,
                   logical_cpus=os.cpu_count(), loadavg=os.getloadavg(),
                   perf_counter=vars(time.get_clock_info("perf_counter")),
                   environment={k: os.environ.get(k) for k in
                                ("LANG", "LC_ALL", "TZ", "PYTHONHASHSEED", "T81_CANONFS_READ_VERIFY", "T81_TRITWISE_PROFILE")},
                   commands=[probe(c) for c in commands]))
        for name in ("CMakeCache.txt", "build.ninja"):
            (out / name).write_bytes((build / name).read_bytes())

        def check_pins():
            for path, expected in hashes.items():
                if digest(path) != expected:
                    raise ValueError("Pinned identity changed: " + path)

        rows = []
        start = time.monotonic()
        deadline = start + 1800
        event("collection_start", budget_seconds=1800)
        for index, order in enumerate(orders):
            check_pins()
            pair = out / f"pair-{index:02d}"
            pair.mkdir()
            roots = {a: pair / a for a in ("A", "B")}
            for root in roots.values():
                root.mkdir()
            if any(list(r.iterdir()) for r in roots.values()):
                raise ValueError("Store not empty")
            journal = Journal(pair / "worker-diagnostics.jsonl", create=True)
            row = dict(label=LABEL, confirmatory=False, eligible_for_preregistered_dataset=False,
                       index=index, order=order, condition="empty_store", input="greet hello", arms={})
            event("pair_start", index=index, order=order, roots={a: str(r) for a,r in roots.items()}, initial_entries={a:0 for a in roots})
            for arm in order:
                remaining = deadline - time.monotonic()
                if remaining <= 0:
                    raise TimeoutError("30 minute collection cap")
                check_pins()
                timings_path = pair / f"nested-{arm}.jsonl"
                timings_path.touch(exist_ok=False)
                command = [sys.executable, "-B", str(HERE / "worker.py"), "worker",
                           "--arm", arm, "--operation", "run", "--store", str(roots[arm]),
                           "--cli", str(binaries[0]), "--journal", str(journal.path),
                           "--scratch", str(pair), "--model", fixtures["model_path"],
                           "--policy", fixtures["apl_policy_path"], "--tokenizer", fixtures["tokenizer_path"],
                           "--input", "greet hello"]
                event("arm_start", index=index, arm=arm, command=command)
                process = None
                clock_utc = utc()
                clock_start = time.perf_counter_ns()
                try:
                    process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                               env=dict(os.environ, T81_CANONFS_READ_VERIFY="1",
                                                        T81_I08_DIAGNOSTIC_TIMINGS=str(timings_path)), start_new_session=True)
                    stdout, stderr = process.communicate(timeout=min(180, remaining))
                    clock_end = time.perf_counter_ns()
                except BaseException as error:
                    if process is not None:
                        try:
                            os.killpg(process.pid, signal.SIGKILL)
                        except ProcessLookupError:
                            pass
                        stdout, stderr = process.communicate()
                    else:
                        stdout = stderr = b""
                    event("arm_failure", index=index, arm=arm, error=repr(error),
                          stdout_base64=base64.b64encode(stdout).decode(), stderr_base64=base64.b64encode(stderr).decode(),
                          returncode=process.returncode if process else None)
                    raise
                record = dict(elapsed_ns=clock_end-clock_start, clock_start_ns=clock_start,
                              clock_end_ns=clock_end, start_utc=clock_utc, end_utc=utc(),
                              returncode=process.returncode, stdout_base64=base64.b64encode(stdout).decode(),
                              stderr_base64=base64.b64encode(stderr).decode())
                event("arm_finish", index=index, arm=arm, **record)
                if process.returncode or record["elapsed_ns"] <= 0:
                    raise RuntimeError("Worker failed or invalid duration")
                result = json.loads(stdout)
                if result.get("verified") is not True or set(result.get("artifact_roles", {})) != set(ROLES):
                    raise ValueError("Unverified seven-object workflow")
                if result["artifact_roles"] != expected_roles[arm]:
                    raise ValueError("Diagnostic artifacts differ from prior exploratory packet")
                nested = [json.loads(line) for line in timings_path.read_text().splitlines()]
                expected_operations = ["execute"] + ["put"]*5 + ["get"]*7 if arm == "A" else []
                if [n["context"]["operation"] for n in nested] != expected_operations:
                    raise ValueError("Unexpected nested invocation sequence")
                previous_end = clock_start
                for sequence, invocation in enumerate(nested, 1):
                    if (invocation["sequence"] != sequence or invocation["error"] is not None or
                        not previous_end <= invocation["start_ns"] < invocation["end_ns"] <= clock_end or
                        invocation["elapsed_ns"] != invocation["end_ns"]-invocation["start_ns"]):
                        raise ValueError("Nested intervals overlap, escape outer interval or failed")
                    previous_end = invocation["end_ns"]
                groups = {k:sum(n["elapsed_ns"] for n in nested if n["context"]["operation"]==k)
                          for k in ("execute", "put", "get")}
                total = sum(groups.values())
                record["nested"] = nested
                record["accounting"] = dict(execution_ns=groups["execute"], persistence_ns=groups["put"],
                                             readback_ns=groups["get"], nested_total_ns=total,
                                             residual_ns=record["elapsed_ns"]-total,
                                             nested_share=total/record["elapsed_ns"],
                                             sequential_nonoverlapping=True)
                # Keep correctness payload labels intact; new clock records are exploratory.
                record["correctness_response"] = result
                row["arms"][arm] = record
                check_pins()
            a, b = [row["arms"][arm]["correctness_response"] for arm in ("A", "B")]
            for key in ("summary", "common_evidence", "input_identities"):
                if a[key] != b[key]:
                    raise ValueError("Matched workflow mismatch: " + key)
            for role in ROLES[:5]:
                if a["artifact_roles"][role] != b["artifact_roles"][role]:
                    raise ValueError("Common artifact identity mismatch: " + role)
            if rows:
                for arm in ("A", "B"):
                    if row["arms"][arm]["correctness_response"]["artifact_roles"] != rows[0]["arms"][arm]["correctness_response"]["artifact_roles"]:
                        raise ValueError("Artifact identities changed between pairs")
            row["ratio_A_over_B"] = row["arms"]["A"]["elapsed_ns"] / row["arms"]["B"]["elapsed_ns"]
            event("pair_complete", equivalent=True, **row)
            rows.append(row)
        # Separate non-task CLI control. Never subtracted to claim exact startup.
        controls = []
        for index in range(10):
            check_pins()
            remaining = deadline-time.monotonic()
            if remaining <= 0:
                raise TimeoutError("30 minute diagnostic cap")
            command = [str(binaries[0]), "--version"]
            event("version_control_start", index=index, command=command)
            process = None
            clock_utc = utc()
            clock_start = time.perf_counter_ns()
            try:
                process = subprocess.Popen(command, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                           env=dict(os.environ, T81_CANONFS_READ_VERIFY="1"), start_new_session=True)
                stdout, stderr = process.communicate(timeout=min(180,remaining))
                clock_end = time.perf_counter_ns()
            except BaseException as error:
                if process is not None:
                    try:
                        os.killpg(process.pid, signal.SIGKILL)
                    except ProcessLookupError:
                        pass
                    stdout, stderr = process.communicate()
                else:
                    stdout = stderr = b""
                event("version_control_failure", index=index, error=repr(error),
                      stdout_base64=base64.b64encode(stdout).decode(), stderr_base64=base64.b64encode(stderr).decode())
                raise
            control = dict(index=index, start_utc=clock_utc, start_ns=clock_start, end_ns=clock_end,
                           elapsed_ns=clock_end-clock_start, returncode=process.returncode,
                           stdout_base64=base64.b64encode(stdout).decode(), stderr_base64=base64.b64encode(stderr).decode())
            event("version_control_finish", **control)
            if process.returncode or control["elapsed_ns"] <= 0:
                raise RuntimeError("Version control failed")
            controls.append(control)
        check_pins()
        elapsed = time.monotonic() - start
        if elapsed > 1800:
            raise TimeoutError("30 minute collection cap")
        def describe(values):
            median = statistics.median(values)
            q1, _, q3 = statistics.quantiles(values, n=4, method="inclusive")
            return dict(n=len(values), median=median, min=min(values), max=max(values),
                        sample_sd=statistics.stdev(values), iqr=q3-q1,
                        mad=statistics.median(abs(v-median) for v in values),
                        cv=statistics.stdev(values)/statistics.mean(values))
        write_json(out / "observations.json", dict(label=LABEL, confirmatory=False,
                   eligible_for_preregistered_dataset=False, collection_status="complete",
                   collection_seconds=elapsed, pairs=rows, version_controls=controls,
                   decomposition={k:describe([r["arms"]["A"]["accounting"][k] for r in rows])
                                  for k in ("execution_ns", "persistence_ns", "readback_ns", "nested_total_ns", "residual_ns", "nested_share")},
                   version_control_summary=describe([r["elapsed_ns"] for r in controls]),
                   summary={**{a: describe([r["arms"][a]["elapsed_ns"] for r in rows]) for a in ("A", "B")},
                            "paired_ratio_A_over_B": describe([r["ratio_A_over_B"] for r in rows])}))
        write_json(out / "environment-after.json", dict(label=LABEL, utc=utc(), loadavg=os.getloadavg(),
                   commands=[probe(["pmset", "-g", "batt"]), probe(["pmset", "-g", "therm"])]))
        event("collection_complete", pairs=10, collection_seconds=elapsed)
    except BaseException as error:
        event("stopped", error_type=type(error).__name__, error=str(error))
        raise


if __name__ == "__main__":
    main()
