#!/usr/bin/env python3
"""BASE-01 Lane A B0. Standard library only; no clock reads or trials in validation."""
import argparse
import hashlib
import json
import math
import os
from pathlib import Path
import platform
import shutil
import statistics
import subprocess
import sys

SOURCE_SHA = "cda9135d36144d4de2771af174e52ac7b30a83c9"
MANIFEST_SHA = "ecd66137bacbce50e605cfc1466ec7d36131ba9e"
IDS = ["WL-ARITH-CODEC-I64", "WL-VM-TISC-DETERMINISM-TIERS",
       "WL-CANONFS-MEM-THROUGHPUT", "WL-RES-RUSAGE-METRICS", "WL-RES-ARTIFACT-SIZE"]
ROOT = Path(__file__).resolve().parent


def command(args, cwd=None):
    proc = subprocess.run([str(x) for x in args], cwd=cwd, text=True, capture_output=True)
    if proc.returncode:
        raise RuntimeError(f"command failed ({proc.returncode}): {args}\n{proc.stdout}\n{proc.stderr}")
    return proc.stdout


def digest(data):
    return hashlib.sha256(data).hexdigest()


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":")).encode()


def rss_kib(raw, system):
    if raw < 0:
        raise ValueError("negative ru_maxrss")
    if system == "Darwin":
        return raw / 1024.0  # bytes, preserve fractional KiB
    if system == "Linux":
        return raw
    raise ValueError(f"unsupported ru_maxrss units: {system}")


def summarize(samples):
    """Future B1: 30 raw trial durations, ns. No measurement execution here."""
    if len(samples) != 30 or any(not math.isfinite(x) or x <= 0 for x in samples):
        raise ValueError("require exactly 30 finite positive independent trials")
    q1, _, q3 = statistics.quantiles(samples, n=4, method="inclusive")
    cv = statistics.stdev(samples) / statistics.mean(samples) * 100
    return {"median_ns": statistics.median(samples), "iqr_ns": q3-q1,
            "cv_pct": cv, "status": "UNSTABLE / INCONCLUSIVE" if cv > 5 else "stable"}


def verify_source(source):
    if command(["git", "-C", source, "rev-parse", "HEAD"]).strip() != SOURCE_SHA:
        raise ValueError("audited source revision mismatch")
    # git diff alone can trust stale index stat data; compare every tracked byte to Git.
    listing = subprocess.check_output(["git", "-C", str(source), "ls-tree", "-rz", "HEAD"])
    entries = []
    for entry in listing.split(b"\0"):
        if not entry:
            continue
        header, path_raw = entry.split(b"\t", 1)
        mode, kind, oid = header.split()
        if kind == b"commit" and path_raw == b"third_party/llama.cpp":
            # Optional submodule is disabled and not part of these workloads.
            entries.append([os.fsdecode(path_raw), "gitlink:" + oid.decode()])
            continue
        if kind != b"blob":
            raise ValueError("unsupported audited tree entry")
        path = source / os.fsdecode(path_raw)
        data = os.readlink(path).encode() if mode == b"120000" else path.read_bytes()
        actual = hashlib.sha1(b"blob " + str(len(data)).encode() + b"\0" + data).hexdigest()
        if actual != oid.decode():
            raise ValueError(f"modified audited source: {path}")
        entries.append([os.fsdecode(path_raw), digest(data)])
    return digest(canonical(entries))


def read_cache(path):
    result = {}
    for line in path.read_text().splitlines():
        if line and not line.startswith(("#", "//")) and "=" in line and ":" in line.split("=",1)[0]:
            typed, value = line.split("=",1)
            result[typed.split(":",1)[0]] = value
    return result


def verify_builds(source, build, native_build):
    project = read_cache(build/"CMakeCache.txt")
    native = read_cache(native_build/"CMakeCache.txt")
    required = {"T81_BUILD_BENCHMARKS":"OFF", "T81_BUILD_TUI":"OFF",
                "T81_ENABLE_LLAMA_CPP":"OFF", "T81_TRITWISE_PROFILE":"OFF",
                "T81_ENABLE_TERNARYOS":"OFF", "T81_ENABLE_DPE":"OFF",
                "T81_USE_CXX23":"ON", "CMAKE_BUILD_TYPE":"Release"}
    for key, value in required.items():
        if project.get(key) != value:
            raise ValueError(f"audited build configuration mismatch: {key}")
    paths = [(project.get("t81_SOURCE_DIR"),source),
             (native.get("BASE01_SOURCE"),source), (native.get("BASE01_BUILD"),build),
             (native.get("CMAKE_HOME_DIRECTORY"),ROOT)]
    for actual, expected in paths:
        if actual is None or Path(actual).resolve() != expected.resolve():
            raise ValueError("build source/directory mismatch")
    if (native.get("CMAKE_BUILD_TYPE") != "Release" or
        Path(native["CMAKE_CXX_COMPILER"]).resolve() != Path(project["CMAKE_CXX_COMPILER"]).resolve()):
        raise ValueError("runner/source compiler or build type mismatch")


def host_metadata():
    system = platform.system()
    info = {"os": platform.platform(), "kernel": platform.release(),
            "architecture": platform.machine(), "python": sys.version,
            "cpu": platform.processor(), "memory_bytes": None,
            "affinity": {"applied_by_runner": False, "status": "unsupported"},
            "frequency_control": {"applied_by_runner": False, "status": "uncontrolled"},
            "load_average": list(os.getloadavg()) if hasattr(os, "getloadavg") else None}
    if system == "Darwin":
        info["sysctl"] = {}
        for key in ("hw.model", "hw.memsize", "hw.ncpu", "hw.physicalcpu", "hw.logicalcpu",
                    "hw.cachelinesize", "hw.l1dcachesize", "hw.l1icachesize", "hw.l2cachesize",
                    "machdep.cpu.brand_string"):
            proc = subprocess.run(["sysctl", "-n", key], text=True, capture_output=True)
            info["sysctl"][key] = {"returncode": proc.returncode, "stdout": proc.stdout.strip(),
                                   "stderr": proc.stderr.strip()}
        memory = info["sysctl"]["hw.memsize"]
        if memory["returncode"] == 0:
            info["memory_bytes"] = int(memory["stdout"])
        cpu = info["sysctl"]["machdep.cpu.brand_string"]
        if cpu["returncode"] == 0:
            info["cpu"] = cpu["stdout"]
    elif system == "Linux":
        info["cpuinfo"] = Path("/proc/cpuinfo").read_text()
        info["meminfo"] = Path("/proc/meminfo").read_text()
        info["memory_bytes"] = int(info["meminfo"].split("MemTotal:")[1].split()[0]) * 1024
        if hasattr(os, "sched_getaffinity"):
            info["affinity"] = {"applied_by_runner": False, "status": "observed only",
                                "cpus": sorted(os.sched_getaffinity(0))}
        info["frequency_governors"] = {str(p): p.read_text().strip() for p in
            Path("/sys/devices/system/cpu").glob("cpu[0-9]*/cpufreq/scaling_governor")}
        info["topology"] = command(["lscpu", "--json"]) if shutil.which("lscpu") else "unsupported"
    return info


def resource_validation():
    if platform.system() not in ("Linux", "Darwin"):
        return {"status": "unsupported", "reason": "only Linux/macOS units qualified"}
    import resource
    before = resource.getrusage(resource.RUSAGE_SELF)
    after = resource.getrusage(resource.RUSAGE_SELF)
    if not (after.ru_maxrss >= before.ru_maxrss >= 0 and
            after.ru_utime >= before.ru_utime >= 0 and after.ru_stime >= before.ru_stime >= 0):
        raise ValueError("getrusage monotonic invariant failed")
    rss_kib(after.ru_maxrss, platform.system())
    # B0 observes API availability/invariants only; no performance numbers retained.
    return {"status": "validated", "scope": "orchestrator RUSAGE_SELF smoke check only",
            "rss_units": "KiB", "cpu_time_samples_retained": False}


def verify_codec(rows):
    expected = [-(2**63), -(2**63)+1, 2**63-2, 2**63-1, 0, 1, -1]
    seed = 0x424153453031
    for _ in range(1024):
        seed = (seed*6364136223846793005 + 1442695040888963407) % (2**64)
        magnitude = seed & (2**63-1)
        expected.append(-magnitude if seed >> 63 else magnitude)
    if [r[0] for r in rows] != expected:
        raise ValueError("codec input identity mismatch")
    for x, back, digits in rows:
        # Independent arbitrary-precision positional oracle, not candidate decode.
        if (not digits or any(d not in (-1, 0, 1) for d in digits) or
                (len(digits)>1 and digits[-1]==0) or
                sum(d * 3**i for i, d in enumerate(digits)) != x or back != x):
            raise ValueError("codec independent oracle failed")
    return digest(canonical(expected))


def validate(args):
    source = args.source.resolve()
    build = args.source_build.resolve()
    native_build = args.native_build.resolve()
    tree_identity = verify_source(source)
    verify_builds(source, build, native_build)
    manifest = json.loads((ROOT / "phase-a-manifest.json").read_text())
    if manifest["recommended_minimal_phase_b_workload_set"] != IDS:
        raise ValueError("manifest allowlist mismatch")
    if os.environ.get("T81_CANONFS_READ_VERIFY", "1") != "1":
        raise ValueError("T81_CANONFS_READ_VERIFY must be unset or 1")
    # New directory and exclusive file creation prevent overwriting prior evidence.
    args.output.mkdir(parents=True, exist_ok=False)
    records = []
    for target in ("t81_ternary_arith_test", "t81_vm_workload_determinism_tiers_test",
                   "t81_canonfs_driver_test", "test_resource_monitoring"):
        invocation = [str(build / target)]
        proc = subprocess.run(invocation, cwd=args.output, capture_output=True, text=True)
        records.append({"command": invocation, "returncode": proc.returncode,
                        "stdout": proc.stdout, "stderr": proc.stderr})
        (args.output / "correctness.json").write_text(json.dumps(records, indent=2)+"\n")
        if proc.returncode:
            raise RuntimeError(f"correctness failed: {target}")
    native = native_build / "base01_lane_a"
    raw = command([native, "--validate-only"], cwd=args.output)
    (args.output / "native-validation.json").write_text(raw)
    results = json.loads(raw)
    codec_identity = verify_codec(results["codec"])
    # Explicit semantic expectations beyond signature repeatability. The historical
    # workload tier is retained with its exact source quirks, not silently repaired.
    expected_registers = {"micro": (43, 84), "meso": (44,81), "mixed": (46,42),
                          "policy-heavy": (43,17), "workload": (9,81)}
    if [x["tier"] for x in results["vm"]] != ["workload", "micro", "meso", "mixed", "policy-heavy", "tensor-access"]:
        raise ValueError("VM tier set mismatch")
    for item in results["vm"]:
        if item["tier"] in expected_registers:
            reg, value = expected_registers[item["tier"]]
            if item["registers"][reg] != value:
                raise ValueError(f"VM semantic oracle failed: {item['tier']}")
        if item["tier"] == "tensor-access":
            handle = item["registers"][44]
            if (item["tags"][44] != 3 or handle < 1 or handle > len(item["floats"]) or
                    item["floats"][handle-1] != 9):
                raise ValueError("VM tensor FloatHandle semantic oracle failed")
    if results["canonfs"] != [{"bytes": n, "fill": 66, "verified": True} for n in (4096,65536,1048576)]:
        raise ValueError("CanonFS case set mismatch")
    artifacts = []
    paths = [native] + sorted(build.glob("libt81_*.a")) + [build / target for target in
        ("t81_ternary_arith_test", "t81_vm_workload_determinism_tiers_test",
         "t81_canonfs_driver_test", "test_resource_monitoring")]
    for path in paths:
        data = path.read_bytes()
        artifacts.append({"path": str(path), "bytes": len(data), "sha256": digest(data)})
    size_tool = shutil.which("llvm-size") or shutil.which("size")
    section_report = {"status": "unsupported"}
    if size_tool:
        proc = subprocess.run([size_tool, "-A", str(native)], capture_output=True, text=True)
        (args.output / "artifact-sections.txt").write_text(proc.stdout+proc.stderr)
        section_report = {"command": [size_tool, "-A", str(native)], "returncode": proc.returncode,
                          "status": "captured" if proc.returncode==0 else "unsupported"}
    for label, path in [("source-compile-commands", build/"compile_commands.json"),
                        ("runner-compile-commands", native_build/"compile_commands.json"),
                        ("source-cmake-cache", build/"CMakeCache.txt"),
                        ("runner-cmake-cache", native_build/"CMakeCache.txt")]:
        shutil.copyfile(path, args.output/(label+path.suffix))
    compiler_cache = (native_build/"CMakeCache.txt").read_text()
    compiler = next(line.split("=",1)[1] for line in compiler_cache.splitlines()
                    if line.startswith("CMAKE_CXX_COMPILER:"))
    report = {"protocol": "BASE-01", "lane": "A", "phase": "B0", "status": "validation passed",
              "measurement_authorized": False, "performance_samples": [], "workloads": IDS,
              "source_sha": SOURCE_SHA, "source_tracked_bytes_sha256": tree_identity,
              "runner_sha": command(["git", "-C", ROOT, "rev-parse", "HEAD"]).strip(),
              "runner_files": {p.name:digest(p.read_bytes()) for p in sorted(ROOT.iterdir()) if p.is_file()},
              "manifest_commit": MANIFEST_SHA, "manifest_sha256": digest((ROOT/"phase-a-manifest.json").read_bytes()),
              "host": host_metadata(), "compiler": {"path": compiler, "version": command([compiler,"--version"])},
              "input_identities": {IDS[0]: codec_identity,
                  IDS[1]: digest(canonical([{k:v for k,v in x.items() if k in ("tier","inputs","policy")} for x in results["vm"]])),
                  IDS[2]: [{"bytes": n, "sha256":digest(bytes([66])*n)} for n in (4096,65536,1048576)]},
              "resource_observer": resource_validation(), "artifacts": artifacts, "sections": section_report,
              "readiness": "B0 validated on this host; B1 blocked pending authorization and host qualification"}
    verify_source(source)
    (args.output/"receipt.json").write_text(json.dumps(report,indent=2)+"\n")
    # Hash every retained raw artifact. Hashes establish content identity only.
    files = {str(p.relative_to(args.output)):digest(p.read_bytes()) for p in sorted(args.output.rglob("*")) if p.is_file()}
    (args.output/"sha256.json").write_text(json.dumps(files,indent=2)+"\n")
    print(json.dumps({"status":report["status"], "receipt":str(args.output/"receipt.json"), "measurement_authorized":False}))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group(required=True)
    mode.add_argument("--validate-only", action="store_true")
    mode.add_argument("--measure", action="store_true")
    parser.add_argument("--source", type=Path)
    parser.add_argument("--source-build", type=Path)
    parser.add_argument("--native-build", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    # Unconditional B0 gate before filesystem, source access, subprocess, clock,
    # environment capture, warm-up, or resource collection. No override token.
    if args.measure:
        parser.error("BASE-01 Phase B1 requires subsequent explicit maintainer authorization; disabled in B0")
    for name in ("source", "source_build", "native_build", "output"):
        if getattr(args, name) is None:
            parser.error(f"--{name.replace('_','-')} is required for validation")
    if platform.system() not in ("Linux", "Darwin"):
        parser.error("Linux/macOS validation only; Windows is unsupported")
    args.output = args.output.resolve()
    try:
        validate(args)
    except (ValueError, RuntimeError, OSError, subprocess.SubprocessError) as exc:
        print(str(exc), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
