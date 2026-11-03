#!/usr/bin/env python3
"""BASE-01 correctness foundation. No timer, warm-up, pilot, trial or campaign API."""
import argparse
import gzip
import hashlib
import json
import os
from pathlib import Path
import platform
import subprocess
import sys

from oracle import canonical, digest, verify_codec
from observers import artifact_metadata, MAX_ARTIFACT, selector_bindings, selector_scope, validate_lifecycle

ROOT = Path(__file__).resolve().parent
SOURCE = "cda9135d36144d4de2771af174e52ac7b30a83c9"
MANIFEST_COMMIT = "ecd66137bacbce50e605cfc1466ec7d36131ba9e"
MANIFEST_HASH = "1367b021f393594e4aab3bc4b52a7b76ca94ee70a49b8f0bda62aea96d46e4ea"
VM_REFERENCE = "7ef553d221ed668f333ff4429012ee0c9378cd13"
CODEC_INPUT = "0fefb0e6b968e8c11556699700530d133505a0bac2013674a42203b10fcb54ef"
VM_INPUT = "86787727b57b249fb6be8285043ffd957d3efba8a2728db12f70397d4142d529"
SIZES = (4096, 65536, 1048576)
IDS = ["WL-ARITH-CODEC-I64", "WL-VM-TISC-DETERMINISM-TIERS", "WL-CANONFS-MEM-THROUGHPUT",
       "WL-RES-RUSAGE-METRICS", "WL-RES-ARTIFACT-SIZE"]
MAX_NATIVE_OUTPUT = 8 * 1024 * 1024
COMPLETE = "ENGINEERING COMPLETE — AWAITING QUALIFICATION"
PARTIAL = "ENGINEERING PARTIAL"


def need(value, message):
    if not value:
        raise ValueError(message)


def verify_manifest(data):
    need(digest(data) == MANIFEST_HASH, "manifest byte identity mismatch")
    m = json.loads(data)
    need(m["measurement_authorized"] is False and m["recommended_minimal_phase_b_workload_set"] == IDS,
         "manifest scope/authorization mismatch")
    return m


def git(source, *args):
    return subprocess.check_output(["git", "-C", str(source), *args])


def verify_source(source):
    need(git(source,"rev-parse","HEAD").decode().strip() == SOURCE, "wrong audited revision")
    entries = []
    for entry in git(source,"ls-tree","-rz","HEAD").split(b"\0"):
        if not entry:
            continue
        header, name = entry.split(b"\t",1)
        mode, kind, oid = header.split()
        path = os.fsdecode(name)
        if kind == b"commit" and path == "third_party/llama.cpp":
            entries.append([path,"gitlink:"+oid.decode()])
            continue
        need(kind == b"blob", "unexpected source entry")
        data = os.readlink(source/path).encode() if mode == b"120000" else (source/path).read_bytes()
        actual = hashlib.sha1(b"blob "+str(len(data)).encode()+b"\0"+data).hexdigest()
        need(actual == oid.decode(), "changed audited bytes: "+path)
        entries.append([path,digest(data)])
    return digest(canonical(entries))


def cache(path):
    return {line.split("=",1)[0].split(":",1)[0]:line.split("=",1)[1]
            for line in path.read_text().splitlines()
            if line and not line.startswith(("#","//")) and "=" in line and ":" in line.split("=",1)[0]}


def verify_builds(source, source_build, native_build):
    project = cache(source_build/"CMakeCache.txt")
    native = cache(native_build/"CMakeCache.txt")
    for key, value in {"CMAKE_BUILD_TYPE":"Release", "T81_BUILD_BENCHMARKS":"OFF",
                      "T81_BUILD_TUI":"OFF", "T81_ENABLE_LLAMA_CPP":"OFF", "T81_USE_CXX23":"ON",
                      "T81_TRITWISE_PROFILE":"OFF", "T81_ENABLE_TERNARYOS":"OFF", "T81_ENABLE_DPE":"OFF"}.items():
        need(project.get(key) == value, "audited build configuration: "+key)
    for value, expected in [(project.get("t81_SOURCE_DIR"),source),
                            (native.get("AUDITED_SOURCE"),source), (native.get("AUDITED_BUILD"),source_build),
                            (native.get("CMAKE_HOME_DIRECTORY"),ROOT)]:
        need(value and Path(value).resolve() == expected.resolve(), "build path mismatch")
    need(native.get("CMAKE_CXX_COMPILER") == project.get("CMAKE_CXX_COMPILER") and
         native.get("CMAKE_BUILD_TYPE") == "Release", "compiler/profile mismatch")
    return project["CMAKE_CXX_COMPILER"]


def native_identity():
    return digest("".join(name+":"+digest((ROOT/name).read_bytes())+"\n"
                          for name in ("CMakeLists.txt","native.cpp","codec.cpp","codec.hpp")).encode())


def verify_codec_record(record):
    need(len(record["encoded"]) == len(record["decoded"]) == 1031, "codec retained count")
    rows = [[item[0], back, item[1]] for item, back in zip(record["encoded"],record["decoded"])]
    need(verify_codec(rows) == CODEC_INPUT, "codec input identity")
    r = record["retention"]
    need(r["inputs"] == r["encoded_outputs"] == r["decoded_outputs"] == 1031, "codec lifecycle count")
    need(0 < r["trit_capacity"] <= 1031*64 and r["trit_bytes"] == 1 and
         r["vector_object_bytes"] > 0, "codec retention capacity")
    need(r["accounted_container_bytes"] == r["trit_capacity"]+1031*(r["vector_object_bytes"]+16),
         "codec retention accounting")
    return {"input_sha256":CODEC_INPUT,"retention":r,"oracle":"verbatim B0 arbitrary-precision positional check"}


def verify_canonfs(records):
    need([x["bytes"] for x in records] == list(SIZES), "CanonFS exact size set")
    checked = []
    for r in records:
        data = bytes([66])*r["bytes"]
        expected = hashlib.sha3_512(data).digest()[:32].hex()
        need(r["fill"] == 66 and r["hashes"] == [expected,expected], "independent CanonHash mismatch")
        need(len(r["read_outputs_hex"]) == 2 and all(bytes.fromhex(x) == data for x in r["read_outputs_hex"]),
             "CanonFS exact read bytes mismatch")
        need(r["retained_refs"] == r["retained_reads"] == 2 and r["accounted_payload_bytes"] == 3*len(data),
             "CanonFS retained output bound")
        checked.append({"bytes":len(data),"payload_sha256":digest(data),"expected_canonhash":expected,
                        "retained_refs":2,"retained_reads":2,"accounted_payload_bytes":3*len(data)})
    return checked


def verify_vm(records):
    names = ["workload","micro","meso","mixed","policy-heavy","tensor-access"]
    ticks = [406,5,250,11,7,8]
    pcs = [16,5,10,11,7,8]
    need([x["tier"] for x in records] == names, "VM historical builder set")
    inputs = [{k:r[k] for k in ("tier","inputs","policy")} for r in records]
    need(digest(canonical(inputs)) == VM_INPUT, "VM builder identity mismatch")
    for n, r in enumerate(records):
        need(r["status"] == "PROVISIONAL" and r["contexts"] == 1 and r["initial_r75"] == 0 and
             r["halted"] is True and r["terminal_pc"] == pcs[n] and r["final_r75"] == ticks[n],
             "VM provisional completion/count condition")
        need(r["memory_words"] == 1536+pcs[n] and 0 < r["trace_entries"] <= 512 and
             0 <= r["axion_events"] <= 2048, "VM fixed-profile retention bound")
        expected = {0:(9,81),1:(43,84),2:(44,81),3:(46,42),4:(43,17)}
        if n in expected:
            reg, value = expected[n]
            need(r["registers"][reg] == value and r["tags"][reg] == 0, "VM semantic result/tag")
        else:
            handle = r["registers"][44]
            need(r["tags"][44] == 3 and 1 <= handle <= len(r["floats"]) and r["floats"][handle-1] == 9,
                 "VM float handle result/tag")
        printed = [["81"],[],["81"],["42"],[],["9t81"]][n]
        need(r["printed"] == printed, "VM printed semantic output")
    return {"status":"PROVISIONAL PASS", "reference":VM_REFERENCE, "input_sha256":VM_INPUT,
            "independent_review":"not established; no readiness promotion", "final_r75":ticks,
            "retention":[{k:r[k] for k in ("tier","memory_words","trace_entries","axion_events")} for r in records]}


def readiness():
    classes = [COMPLETE,PARTIAL,COMPLETE,PARTIAL,COMPLETE]
    reasons = ["1031 retained encode/decode outputs checked; no timing qualification",
               "PROVISIONAL reference pending independent review; no R75 generalization",
               "memory-only retained outputs and independent hashlib CanonHash expectations checked",
               "unit/selector/lifecycle validation only; real resource collector intentionally absent",
               "bounded ELF64-LE/Mach-O64-LE metadata parsers; unsupported formats explicit"]
    return {name:{"engineering":status,"reason":reason,"eligibility":{"correctness_only":True,
                 "qualification":False,"measurement":False}} for name,status,reason in zip(IDS,classes,reasons)}


def validate(args):
    need(platform.system() in ("Linux","Darwin"), "unsupported execution platform")
    source, source_build, build = args.source.resolve(), args.source_build.resolve(), args.build.resolve()
    source_hash = verify_source(source)
    verify_manifest((ROOT/"phase-a-manifest.json").read_bytes())
    compiler = verify_builds(source,source_build,build)
    binary = build/"foundation_check"
    identity_check = subprocess.run([str(binary),"--identity-only"],capture_output=True,text=True)
    need(identity_check.returncode == 0 and len(identity_check.stdout) < 1024 and
         json.loads(identity_check.stdout).get("native_identity") == native_identity(),
         "stale/unknown native implementation before workload execution")
    args.output.mkdir(parents=True,exist_ok=False)
    env = dict(os.environ); env.pop("T81_CANONFS_ROOT",None); env["T81_CANONFS_READ_VERIFY"] = "1"
    # Native stdout is redirected to a file so the launcher does not accumulate it
    # while the child runs. Accepted data is capped before loading/parsing.
    raw_path = args.output/"native.json"
    err_path = args.output/"native-stderr.txt"
    with raw_path.open("xb") as out, err_path.open("xb") as err:
        child = subprocess.Popen([str(binary),"--validate-only"],stdout=out,stderr=err,env=env)
        lifecycle = [{"event":"spawned","pid":child.pid,"parent_pid":os.getpid()}]
        code = child.wait()  # Reaping status only; no wait4/getrusage or timer.
    lifecycle.append({"event":"exited","pid":child.pid,"returncode":code})
    (args.output/"lifecycle.json").write_text(json.dumps(lifecycle,indent=2)+"\n")
    need(code == 0, "native correctness failed: "+str(code))
    need(raw_path.stat().st_size <= MAX_NATIVE_OUTPUT, "native evidence exceeds accepted bound")
    raw = raw_path.read_bytes(); native = json.loads(raw)
    need(native["native_identity"] == native_identity(), "stale native implementation")
    need(native["worker_pid"] == child.pid and native["parent_pid"] == os.getpid(), "worker identity mismatch")
    codec = verify_codec_record(native["codec"])
    canonfs = verify_canonfs(native["canonfs"])
    vm = verify_vm(native["vm"])
    worker = validate_lifecycle(lifecycle)
    selector_scope("RUSAGE_SELF",child.pid,native["worker_pid"])
    selector_scope("WAITED_CHILD",child.pid,native["worker_pid"],reaped=True)
    artifacts = []
    linked_archives=[source_build/("libt81_"+name+".a")
                     for name in ("vm","axion","isa","experimental","ffi","core")]
    for path in [binary]+linked_archives:
        need(path.stat().st_size <= MAX_ARTIFACT, "artifact file bound")
        item = artifact_metadata(path.read_bytes()); item["path"] = str(path); artifacts.append(item)
    need(artifacts[0]["status"] == "SUPPORTED", "worker artifact format unsupported")
    for label, path in [("source-cache",source_build/"CMakeCache.txt"),("native-cache",build/"CMakeCache.txt"),
                        ("source-compile-commands",source_build/"compile_commands.json"),
                        ("native-compile-commands",build/"compile_commands.json")]:
        (args.output/(label+path.suffix+".gz")).write_bytes(gzip.compress(path.read_bytes(),mtime=0))
    report = {"schema":"base01.b1-foundation.validation.v1","status":"PASS","mode":"correctness-only",
              "source_sha":SOURCE,"source_tracked_bytes_sha256":source_hash,
              "runner_sha":git(ROOT,"rev-parse","HEAD").decode().strip(),
              "runner_files":{p.name:digest(p.read_bytes()) for p in sorted(ROOT.iterdir()) if p.is_file()},
              "manifest_commit":MANIFEST_COMMIT,"manifest_sha256":MANIFEST_HASH,
              "native_identity":native["native_identity"],"worker":worker,"codec":codec,"canonfs":canonfs,"vm":vm,
              "workloads":readiness(),"artifacts":artifacts,
              "platform":{"os":platform.system(),"kernel":platform.release(),"architecture":platform.machine(),"python":sys.version},
              "oracle_providers":{"codec":"accepted B0 pure Python positional oracle",
                                  "canonhash":{"algorithm":"SHA3-512 prefix","prefix_bytes":32,
                                               "module":hashlib.sha3_512.__module__}},
              "compiler":{"path":compiler,"version":subprocess.check_output([compiler,"--version"],text=True)},
              "resource_observer":{"status":"FIXTURE/SELECTOR VALIDATION ONLY","samples":[],
                                   "accounting_collector":"not implemented", "selector_bindings":selector_bindings(platform.system())},
              "measurement_enabled":False,"performance_samples":[]}
    need(verify_source(source) == source_hash, "audited source changed during validation")
    (args.output/"receipt.json").write_text(json.dumps(report,indent=2)+"\n")
    (args.output/"native.json.gz").write_bytes(gzip.compress(raw,mtime=0)); raw_path.unlink()
    files = {p.name:digest(p.read_bytes()) for p in sorted(args.output.iterdir()) if p.is_file()}
    (args.output/"sha256.json").write_text(json.dumps(files,indent=2)+"\n")
    print(json.dumps({"status":"PASS","vm":"PROVISIONAL","measurement_enabled":False,"receipt":str(args.output/"receipt.json")}))


class ClosedParser(argparse.ArgumentParser):
    def error(self, message):
        print(json.dumps({"status":"REJECTED","reason":message,"measurement_enabled":False}),file=sys.stderr)
        raise SystemExit(2)


def main(argv=None):
    parser = ClosedParser(description=__doc__,allow_abbrev=False)
    parser.add_argument("--validate-only",action="store_true",required=True)
    for name in ("source","source-build","build","output"):
        parser.add_argument("--"+name,type=Path,required=True)
    args = parser.parse_args(argv)
    try:
        validate(args)
    except (ValueError,OSError,subprocess.SubprocessError) as exc:
        print(json.dumps({"status":"FAIL","reason":str(exc),"measurement_enabled":False}),file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
