#!/usr/bin/env python3
"""Correctness-only source ledger. No timer, batching or performance collection."""
import argparse
import collections
import gzip
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

SOURCE = "cda9135d36144d4de2771af174e52ac7b30a83c9"
REFERENCE = "64c3f71ff100ea16270f1a70497ac39860c1b772"
NAMES = ["workload", "micro", "meso", "mixed", "policy-heavy", "tensor-access"]


def require(value, message):
    if not value:
        raise ValueError(message)


def sha(data):
    return hashlib.sha256(data).hexdigest()


def git(source, *args):
    return subprocess.check_output(["git", "-C", str(source), *args])


def source_identity(source):
    require(git(source, "rev-parse", "HEAD").decode().strip() == SOURCE, "wrong source revision")
    entries = []
    for entry in git(source, "ls-tree", "-rz", "HEAD").split(b"\0"):
        if not entry:
            continue
        header, name = entry.split(b"\t", 1)
        mode, kind, oid = header.split()
        path = os.fsdecode(name)
        if kind == b"commit" and path == "third_party/llama.cpp":
            entries.append([path, "gitlink:" + oid.decode()])
            continue
        require(kind == b"blob", "unexpected source entry")
        data = os.readlink(source/path).encode() if mode == b"120000" else (source/path).read_bytes()
        identity = hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()
        require(identity == oid.decode(), f"source byte mismatch: {path}")
        entries.append([path, sha(data)])
    return sha(json.dumps(entries, sort_keys=True, separators=(",", ":")).encode())


def expected_paths(name, length):
    # Derived from source: threshold 50; recording occurs before interpreter dispatch.
    # The original R0 workload's 50 interpreted increments are reset at each sync;
    # only its subsequent 81 trace iterations retain R0 across the loop body.
    if name in ("workload", "meso"):
        traces = 81 if name == "workload" else 31
        return ([[i] for i in range(3)] + [[pc] for _ in range(50) for pc in (3, 4, 5)] +
                [[3, 4, 5] for _ in range(traces)] + [[i] for i in range(6, length)])
    return [[i] for i in range(length)]


def verify_case(name, program, steps, final):
    instructions = program["instructions"]
    expected = expected_paths(name, len(instructions))
    require(program["initial"]["r75"] == 0, name+": not fresh R75")
    require(len(steps) == len(expected) == final["step_calls"], name+": step count differs from ledger")
    attempts = interpreted = traced = trace_entries = 0
    visible = 0
    per_pc = collections.Counter()
    for index, (step, pcs) in enumerate(zip(steps, expected)):
        before, after = step["before"], step["after"]
        require(step["index"] == index and step["success"], name+": failed dispatch")
        require(before["pc"] == pcs[0] and before["r75"] == visible, name+": before state differs")
        hooks = [e for e in step["events"] if e[0] == "step"]
        if program["observer_active"]:
            require([e[1] for e in hooks] == pcs, name+": semantic PC sequence differs")
            require([e[2] for e in hooks] == [instructions[pc][0] for pc in pcs], name+": opcode differs")
            require([e[3] for e in hooks] == list(range(attempts+1, attempts+len(pcs)+1)), name+": count projection differs")
        else:
            require(name in ("mixed","policy-heavy") and not step["events"], "unexpected unobserved program")
        require(all(e[4] == visible for e in step["events"]), name+": unexpected register sync within dispatch")
        per_pc.update(pcs)
        jit = [e for e in step["events"] if e[0].startswith("jit trace")]
        if len(pcs) == 3:
            require(len(jit) == 2 and jit[0][0] == "jit trace enter" and jit[1][0] == "jit trace exit", name+": unexpected side exit")
            require(jit[0][3] == attempts and jit[1][3] == attempts+3, name+": trace counter delta differs")
            require(not step["new_trace"], name+": trace unexpectedly logged interpreter entries")
            require(after["r75"] == visible == 153, name+": traced return synchronized R75")
            traced += 3
            # The last loop branch falls through, earlier ones branch to 3.
            done = after["registers"][3 if name == "workload" else 43] == 0
            require(after["pc"] == (6 if done else 3), name+": branch effect differs")
        else:
            require(not jit and len(step["new_trace"]) == 1, name+": interpreter path differs")
            pc = pcs[0]
            require(step["new_trace"][0] == [pc, instructions[pc][0], -1], name+": trap/trace mismatch")
            visible = attempts+1
            require(after["r75"] == visible, name+": interpreter failed to sync count")
            interpreted += 1
            trace_entries += 1
        attempts += len(pcs)
    state = final["state"]
    halt_pc = len(instructions)-1
    require(state["halted"] and state["pc"] == len(instructions), name+": explicit completion failed")
    require(steps[-1]["new_trace"] == [[halt_pc, instructions[-1][0], -1]], name+": no successful final Halt trace")
    require(state["r75"] == attempts and final["trace_entries"] == trace_entries, name+": final ledger mismatch")
    # Semantic checks independent of signature repetition.
    registers = state["registers"]
    if name in ("workload", "meso", "mixed"):
        reg, value = {"workload":(9,81),"meso":(44,81),"mixed":(46,42)}[name]
        require(registers[reg] == value and state["printed"] == [str(value)], name+": wrong semantic result")
    elif name == "micro":
        require(registers[43] == 84, "micro result")
    elif name == "policy-heavy":
        require(registers[43] == 17, "policy result")
    else:
        handle = registers[44]
        require(state["tags"][44] == 3 and handle > 0 and state["floats"][handle-1] == 9, "tensor float handle")
    return {"tier":name,"interpreter_attempts":interpreted,"trace_attempts":traced,
            "successful_primary_dispatches":attempts,"failed_primary_dispatches":0,
            "guard_retries":0,"axion_side_exits":0,"final_r75":state["r75"],
            "step_calls":len(steps),"trace_entries":trace_entries,"halt_included":1,
            "final_pc":state["pc"],"per_pc_successful_dispatches":dict(sorted(per_pc.items()))}


def verify_records(records):
    programs = {r["name"]:r for r in records if r["type"] == "program"}
    finals = {r["name"]:r for r in records if r["type"] == "final"}
    steps = {name:[r for r in records if r["type"] == "step" and r["name"] == name] for name in programs}
    identities=[{"tier":name,"inputs":programs[name]["instructions"],"policy":programs[name]["policy"]} for name in NAMES]
    require(sha(json.dumps(identities,sort_keys=True,separators=(",", ":")).encode()) ==
            "86787727b57b249fb6be8285043ffd957d3efba8a2728db12f70397d4142d529", "historical builders changed")
    ledger = [verify_case(name, programs[name], steps[name], finals[name]) for name in NAMES]
    guard_steps = steps["synthetic-guard-retry"]
    expected_guard = ([[i] for i in range(3)] +
                      [[pc] for _ in range(50) for pc in (3,4,5,6)] +
                      [[3,4,5,6] for _ in range(5)] + [[3,4,4]])
    require(len(guard_steps)==len(expected_guard)==209,"guard prefix step count")
    attempted=visible=0
    for step, pcs in zip(guard_steps,expected_guard):
        hooks=[e for e in step["events"] if e[0]=="step"]
        require([e[1] for e in hooks]==pcs,"guard full-path PCs")
        require([e[3] for e in hooks]==list(range(attempted+1,attempted+len(pcs)+1)),"guard full-path attempts")
        require(step["before"]["r75"]==visible,"guard visible prefix")
        attempted+=len(pcs)
        if step["new_trace"]:
            visible=attempted
        require(step["after"]["r75"]==visible,"guard publication point")
    require(attempted==226,"guard final attempts")
    guard = guard_steps[-1]
    require([e[0] for e in guard["events"]] == ["jit trace enter","step","step","jit trace deopt","step"], "guard event sequence")
    require([e[3] for e in guard["events"]] == [223,224,225,225,226], "guard attempt count")
    require([e[1] for e in guard["events"]] == [3,3,4,4,4], "guard retry PC")
    require(not guard["success"] and guard["after"]["r75"] == 226 and not guard["after"]["halted"], "guard fault state")
    denied = steps["synthetic-policy-denial"][-1]
    require(not denied["success"] and denied["events"][0][3] == 3 and denied["after"]["r75"] == 2, "interpreter policy asymmetry")
    denied_trace = steps["synthetic-trace-policy-denial"][-1]
    require(not denied_trace["success"] and denied_trace["after"]["r75"] == 153, "trace policy denial")
    require([e[3] for e in denied_trace["events"]] == [153,154,153], "trace prospective count not committed")
    probes = {r["name"]:r for r in records if r["type"] == "counterexample"}
    require(probes["budget-and-reload"]["reloaded_initial_r75"] == 5, "reload stale counter")
    require(probes["reused-trace"]["state"]["r75"] == 3 and probes["reused-trace"]["state"]["pc"] == 6, "reused trace")
    require(probes["side-exit-and-implicit-halt"]["axion_boundaries_not_recorded"] == 5, "recorder boundary scope")
    return {"disposition":"R75 SEMANTICS ESTABLISHED", "scope":"fresh single-context VM; exact six builders; factory default/builder policies; successful explicit Halt; no external register writes/overflow",
            "ledger":ledger, "negative_probes":list(probes),
            "historical_inputs_sha256":"86787727b57b249fb6be8285043ffd957d3efba8a2728db12f70397d4142d529",
            "guard_counterexample":{"attempts":226,"successful_primary_dispatches":224,"failed_div_attempts":2},
            "performance_collection":False}


def cache(path):
    return dict((line.split("=",1)[0].split(":",1)[0],line.split("=",1)[1])
                for line in path.read_text().splitlines() if line and not line.startswith(("#","//")) and ":" in line.split("=",1)[0] and "=" in line)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--validate-only", action="store_true", required=True)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--source-build", type=Path, required=True)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args=parser.parse_args()
    root=Path(__file__).resolve().parent
    identity=source_identity(args.source)
    project=cache(args.source_build/"CMakeCache.txt"); native=cache(args.build/"CMakeCache.txt")
    require(Path(project["t81_SOURCE_DIR"]).resolve()==args.source.resolve(),"archive build source path")
    for key,value in {"T81_BUILD_BENCHMARKS":"OFF","T81_USE_CXX23":"ON","T81_BUILD_TUI":"OFF","T81_ENABLE_DPE":"OFF","T81_ENABLE_LLAMA_CPP":"OFF","CMAKE_BUILD_TYPE":"Release"}.items():
        require(project[key]==value,"source build configuration: "+key)
    require(native["CMAKE_CXX_COMPILER"]==project["CMAKE_CXX_COMPILER"],"compiler mismatch")
    require(Path(native["AUDITED_SOURCE"]).resolve()==args.source.resolve() and
            Path(native["AUDITED_BUILD"]).resolve()==args.source_build.resolve() and
            Path(native["CMAKE_HOME_DIRECTORY"]).resolve()==root,"probe build paths mismatch")
    executable=args.build/"r75_correctness"
    args.output.mkdir(parents=True,exist_ok=False)
    environment=dict(os.environ); environment.pop("T81_CANONFS_ROOT",None)
    command=[str(executable),"--validate-only"]
    proc=subprocess.run(command,capture_output=True,env=environment)
    (args.output/"raw.jsonl.gz").write_bytes(gzip.compress(proc.stdout,mtime=0))
    (args.output/"stderr.txt").write_bytes(proc.stderr)
    require(proc.returncode==0,f"native correctness failed: {proc.returncode}")
    records=[json.loads(line) for line in proc.stdout.splitlines()]
    summary=verify_records(records)
    # Negative verifier checks: stable-but-wrong counters must not pass the ledger.
    mutations=0
    for field in ("r75","pc","halted"):
        bad=json.loads(json.dumps(records))
        target=next(x for x in bad if x["type"]=="final" and x["name"]=="meso")["state"]
        target[field]=False if field=="halted" else target[field]+1
        try:
            verify_records(bad)
        except ValueError:
            mutations+=1
        else:
            raise ValueError("corrupted ledger accepted: "+field)
    summary["verifier_negative_checks_passed"]=mutations
    summary["source_sha"]=SOURCE; summary["design_reference"]=REFERENCE
    summary["source_tracked_bytes_sha256"]=identity
    summary["command"]=command; summary["native_exit"]=proc.returncode
    denied=subprocess.run([str(executable),"--measure"],capture_output=True,env=environment)
    require(denied.returncode==2 and not denied.stdout,"non-correctness argument accepted")
    summary["native_measurement_rejection"]={"returncode":denied.returncode,"stderr":denied.stderr.decode()}
    summary["raw_stdout_sha256"]=sha(proc.stdout)
    paths=[executable]+sorted(args.source_build.glob("libt81_*.a"))
    summary["binary_artifacts"]=[{"path":str(p),"sha256":sha(p.read_bytes())} for p in paths]
    summary["compiler_version"]=subprocess.check_output([project["CMAKE_CXX_COMPILER"],"--version"],text=True)
    summary["runner_sha"]=git(root,"rev-parse","HEAD").decode().strip()
    summary["runner_files"]={p.name:sha(p.read_bytes()) for p in sorted(root.iterdir()) if p.is_file()}
    summary["environment"]={"T81_CANONFS_ROOT":"unset by verifier","platform":sys.platform,"machine":os.uname().machine}
    for label,path in [("source-cache",args.source_build/"CMakeCache.txt"),("probe-cache",args.build/"CMakeCache.txt"),("probe-compile-commands",args.build/"compile_commands.json")]:
        (args.output/(label+path.suffix)).write_bytes(path.read_bytes())
    source_commands=(args.source_build/"compile_commands.json").read_bytes()
    summary["source_compile_commands_sha256"]=sha(source_commands)
    (args.output/"source-compile-commands.json.gz").write_bytes(gzip.compress(source_commands,mtime=0))
    require(source_identity(args.source)==identity,"source changed during checks")
    (args.output/"summary.json").write_text(json.dumps(summary,indent=2)+"\n")
    (args.output/"sha256.json").write_text(json.dumps({p.name:sha(p.read_bytes()) for p in sorted(args.output.iterdir()) if p.is_file()},indent=2)+"\n")
    print(json.dumps({"disposition":summary["disposition"],"tiers":len(summary["ledger"]),"negative_checks":mutations,"performance_collection":False}))


if __name__=="__main__":
    try:
        main()
    except (ValueError,OSError,subprocess.SubprocessError) as error:
        print(str(error),file=sys.stderr)
        sys.exit(1)
