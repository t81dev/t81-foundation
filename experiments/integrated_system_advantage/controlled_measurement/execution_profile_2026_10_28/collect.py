"""EXPLORATORY — NONCONFIRMATORY: external sample, at most 10 P + 10 C.

No code injection, production instrumentation or rebuild. Stop on profiler or
symbol failure. This isolates the previously measured execution command, not
the whole seven-object workflow; verify its two emitted artifacts unchanged.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import random
import re
import signal
import subprocess
import sys
import time

HERE = Path(__file__).resolve().parent
CONTROL = HERE.parent
REPO = CONTROL.parents[2]
ALPHABET = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz-−×÷=<>≤≥≠≈∞λμπστωΓ"
LABEL = "EXPLORATORY — NONCONFIRMATORY"


def sha(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def identity(raw):
    number = int.from_bytes(hashlib.sha3_512(raw).digest()[:32], "big")
    digits = ""
    while number:
        number, r = divmod(number, 81)
        digits = ALPHABET[r]+digits
    return "sha3-256:"+(digits or "0")


def utc():
    return datetime.now(timezone.utc).isoformat()


def save(path, value):
    with Path(path).open("x") as stream:
        json.dump(value, stream, indent=2, ensure_ascii=False)
        stream.write("\n")
        stream.flush()
        os.fsync(stream.fileno())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", required=True)
    parser.add_argument("--out", required=True)
    args = parser.parse_args()
    out = Path(args.out).resolve()
    out.mkdir(parents=True, exist_ok=False)
    attempts = out/"attempts.jsonl"
    attempts.touch(exist_ok=False)

    def event(kind, **fields):
        with attempts.open("a") as stream:
            stream.write(json.dumps(dict(label=LABEL, confirmatory=False,
                         eligible_for_preregistered_dataset=False, kind=kind,
                         utc=utc(), monotonic_ns=time.monotonic_ns(), **fields), ensure_ascii=False)+"\n")
            stream.flush()
            os.fsync(stream.fileno())

    def environment(name):
        probes = []
        for command in (["uname","-a"],["sw_vers"],["sysctl","-n","hw.model","hw.memsize","machdep.cpu.brand_string"],
                        ["pmset","-g","batt"],["pmset","-g","therm"]):
            r = subprocess.run(command, capture_output=True, timeout=30)
            probes.append(dict(command=command,returncode=r.returncode,
                               stdout=r.stdout.decode(errors="replace"),stderr=r.stderr.decode(errors="replace")))
        save(out/name, dict(label=LABEL,utc=utc(),platform=platform.platform(),python=sys.version,
                           loadavg=os.getloadavg(),clock=vars(time.get_clock_info("perf_counter")),probes=probes))

    target = profiler = None
    try:
        cli = Path(args.build_dir).resolve()/"t81"
        if sha(cli) != json.loads((CONTROL/"validation/report.json").read_text())["binary_sha256"]["t81"]:
            raise ValueError("Accepted binary identity differs")
        old = CONTROL/"cli_diagnostic_2026_10_28/run"
        oldplan = json.loads((old/"plan.json").read_text())
        fixtures = oldplan["fixtures"]
        expected = json.loads((CONTROL/"exploratory_2026_10_28/artifact-identities.json").read_text())
        refs = expected["roles_stable_in_all_ten_pairs"]["A"]
        pins = {str(p):sha(p) for p in
                [cli,Path("/usr/bin/sample"),Path(sys.executable).resolve(),Path(__file__).resolve(),
                 Path(fixtures["model_path"]),Path(fixtures["apl_policy_path"]),Path(fixtures["tokenizer_path"]),
                 REPO/"tools/cli/core/main.cpp",REPO/"tools/cli/ai/ai_cli_shared.cpp",
                 REPO/"vm/decoder.cpp",REPO/"vm/vm.cpp",REPO/"tools/cli/core/driver.cpp",
                 CONTROL/"schedule.json",CONTROL/"PREREGISTRATION.md"]}
        orders = [["P","C"] for _ in range(5)]+[["C","P"] for _ in range(5)]
        random.Random(6281030).shuffle(orders)
        save(out/"plan.json",dict(label=LABEL,confirmatory=False,eligible_for_preregistered_dataset=False,
             evidence_commit="2d51b7019c4dfd65562d74a69dcb4d47344466bb",seed=6281030,orders=orders,
             input="greet hello",condition="empty_store",max_profiled=10,max_controls=10,max_seconds=1800,
             profiler="/usr/bin/sample PID 5 1 -mayDie -file REPORT",nominal_interval_ms=1,
             requested_duration_seconds=5,stop_on_insufficient_symbols=True,pins_sha256=pins,
             fixtures=fixtures,expected_artifact_roles=refs,
             timing="Parent perf_counter_ns immediately before target Popen through target communicate return. Profiler spawn is inside profiled interval; subsequent profiler finalization/verification excluded."))
        environment("environment-before.json")
        start = time.monotonic()
        deadline = start+1800
        event("collection_start", budget_seconds=1800)
        rows = []
        for pair, order in enumerate(orders):
            for treatment in order:
                for path, expected_sha in pins.items():
                    if sha(path) != expected_sha:
                        raise ValueError("Pinned identity changed: "+path)
                root = out/f"pair-{pair:02d}-{treatment}"
                root.mkdir()
                store = root/"store"
                store.mkdir()
                (root/"model.t81w").write_bytes(Path(fixtures["model_path"]).read_bytes())
                (root/"policy.apl").write_bytes(Path(fixtures["apl_policy_path"]).read_bytes())
                (root/"tokenizer.json").write_bytes(Path(fixtures["tokenizer_path"]).read_bytes())
                command = [str(cli),"ai","task","assess-fixed","--model","investigation08",
                           "--model-file",str(root/"model.t81w"),"--policy",str(root/"policy.apl"),
                           "--input","greet hello","--mode","strict_deterministic","--canonfs-root",str(store)]
                event("execution_start",pair=pair,treatment=treatment,order=order,command=command,initial_entries=0)
                target = profiler = None
                remaining = deadline-time.monotonic()
                if remaining <= 0:
                    raise TimeoutError("30 minute cap")
                clock_utc = utc()
                clock_start = time.perf_counter_ns()
                target = subprocess.Popen(command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,
                                          env=dict(os.environ,T81_CANONFS_READ_VERIFY="1"),start_new_session=True)
                profiler_command = None
                if treatment == "P":
                    profiler_command = ["/usr/bin/sample",str(target.pid),"5","1","-mayDie","-file",str(root/"sample.txt")]
                    event("profiler_start",pair=pair,pid=target.pid,command=profiler_command)
                    profiler = subprocess.Popen(profiler_command,stdout=subprocess.PIPE,stderr=subprocess.PIPE,start_new_session=True)
                stdout, stderr = target.communicate(timeout=min(180,remaining))
                clock_end = time.perf_counter_ns()
                (root/"stdout.bin").write_bytes(stdout)
                (root/"stderr.bin").write_bytes(stderr)
                record = dict(pair=pair,treatment=treatment,order=order,start_utc=clock_utc,end_utc=utc(),
                              start_ns=clock_start,end_ns=clock_end,elapsed_ns=clock_end-clock_start,
                              pid=target.pid,returncode=target.returncode,command=command,profiler_command=profiler_command)
                event("execution_finish",**record)
                if profiler is not None:
                    pout,perr = profiler.communicate(timeout=min(90,max(1,deadline-time.monotonic())))
                    (root/"profiler-stdout.bin").write_bytes(pout)
                    (root/"profiler-stderr.bin").write_bytes(perr)
                    record["profiler_returncode"] = profiler.returncode
                    event("profiler_finish",pair=pair,returncode=profiler.returncode)
                if target.returncode or record["elapsed_ns"] <= 0:
                    raise ValueError("Execution failed or invalid duration")
                response = json.loads(stdout)
                verified = {}
                for role,key in (("result","result_ref"),("provenance","provenance_ref")):
                    ref = response[key]
                    raw = (store/"objects"/(ref.split(":",1)[1]+".blk")).read_bytes()
                    if ref != refs[role] or identity(raw) != ref:
                        raise ValueError("Artifact identity mismatch: "+role)
                    verified[role] = dict(ref=ref,sha256=hashlib.sha256(raw).hexdigest(),bytes=len(raw))
                if response["decision"] != "ALLOW" or response["decision_token_ids"] != [42] or response["policy_result"] != "allowed":
                    raise ValueError("Correctness mismatch")
                record["verified_artifacts"] = verified
                event("correctness_verified",pair=pair,treatment=treatment,artifacts=verified)
                if treatment == "P":
                    if profiler.returncode != 0 or not (root/"sample.txt").is_file():
                        raise RuntimeError("PROFILING_UNAVAILABLE: profiler failed; preserve raw logs")
                    sample = (root/"sample.txt").read_text(errors="replace")
                    if "Call graph:" not in sample or not re.search(r"t81::|run_fixed_label_task",sample):
                        raise RuntimeError("INSUFFICIENT_SYMBOLS: stop without another instrumentation framework")
                rows.append(record)
                event("observation_complete",**record)
        elapsed = time.monotonic()-start
        if elapsed > 1800:
            raise TimeoutError("30 minute cap")
        save(out/"observations.json",dict(label=LABEL,confirmatory=False,
             eligible_for_preregistered_dataset=False,collection_seconds=elapsed,observations=rows))
        environment("environment-after.json")
        event("collection_complete",profiled=10,controls=10,collection_seconds=elapsed)
    except BaseException as error:
        for process in (profiler,target):
            if process is not None and process.poll() is None:
                os.killpg(process.pid,signal.SIGKILL)
                stdout,stderr = process.communicate()
                name = "profiler" if process is profiler else "target"
                (out/("interrupted-"+name+"-stdout.bin")).write_bytes(stdout)
                (out/("interrupted-"+name+"-stderr.bin")).write_bytes(stderr)
        event("stopped",error_type=type(error).__name__,error=str(error))
        raise


if __name__ == "__main__":
    main()
