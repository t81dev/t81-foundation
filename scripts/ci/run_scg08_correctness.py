"""Build and test only the pinned PR785 correctness slice; retain failed stages."""

import argparse
import hashlib
import importlib.metadata
import json
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import sys
import xml.etree.ElementTree as ET


SOURCE_SHA = "300cdea8896d56c025f7099c0cdf0c41d7bd6237"
EXPECTED_TESTS = {
    "t81_exp_scg08_01_test",
    "t81_exp_scg08_01_result_rejection",
    "t81_exp_scg08_01_result_rejection_driver",
    "t81_exp_scg08_01_schema_validation",
    "t81_exp_scg08_01_schema_negative_validation",
}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--build", type=Path, required=True)
    parser.add_argument("--evidence", type=Path, required=True)
    parser.add_argument("--asio-source", type=Path, help="Optional local dependency cache")
    args = parser.parse_args()
    source, build, evidence = (p.resolve() for p in (args.source, args.build, args.evidence))
    evidence.mkdir(parents=True, exist_ok=True)
    record = {
        "expected_source_sha": SOURCE_SHA,
        "platform": platform.platform(),
        "python": sys.version,
        "toolchain": os.environ.get("SCG08_TOOLCHAIN", "local"),
        "harness_sha": None,
        "harness_script_sha256": hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
        "workflow_sha": os.environ.get("GITHUB_SHA"),
        "run_id": os.environ.get("GITHUB_RUN_ID"),
        "run_attempt": os.environ.get("GITHUB_RUN_ATTEMPT"),
        "commands": [],
        "status": "failed",
        "scope": "Correctness only; no benchmarks, Gate 2, or scientific acceptance",
        "limits": [
            "Hosted runner results apply only to this pinned source and toolchain",
            "Codex-authored harness; not independent scientific validation",
            "Original historical execution/provenance is not authenticated",
            "Artifact retention is 90 days; archive the downloaded packet for longer retention",
        ],
    }

    def run(label, argv, cwd=source, timeout=600, require_success=True):
        argv = [str(arg) for arg in argv]
        entry = {"label": label, "argv": argv, "cwd": str(cwd), "timeout_seconds": timeout}
        record["commands"].append(entry)
        print(f"{label}: {argv!r}", flush=True)
        log = evidence / (label + ".log")
        try:
            with log.open("wb") as stream:
                result = subprocess.run(argv, cwd=cwd, stdout=stream, stderr=subprocess.STDOUT,
                                        timeout=timeout, check=False, shell=False)
            entry["exit_code"] = result.returncode
        except (OSError, subprocess.TimeoutExpired) as exc:
            entry["error"] = str(exc)
            raise
        print(log.read_text(encoding="utf-8", errors="replace"), flush=True)
        if require_success and result.returncode != 0:
            raise RuntimeError(f"{label} failed with exit {result.returncode}")
        return log.read_text(encoding="utf-8", errors="replace")

    try:
        record["harness_sha"] = run("harness-sha", ["git", "rev-parse", "HEAD"],
                                    cwd=Path(__file__).resolve().parents[2]).strip()
        actual = run("source-sha", ["git", "rev-parse", "HEAD"]).strip()
        record["source_sha"] = actual
        if actual != SOURCE_SHA:
            raise RuntimeError(f"Wrong source revision: {actual}")
        run("source-clean-before", ["git", "diff", "--exit-code", "HEAD"])
        record["jsonschema_version"] = importlib.metadata.version("jsonschema")
        run("python-packages", [sys.executable, "-m", "pip", "freeze"])
        for tool in ("cmake", "ctest", "ninja"):
            run(tool + "-version", [tool, "--version"])
        cc = os.environ.get("SCG08_CC", "clang")
        cxx = os.environ.get("SCG08_CXX", "clang++")
        # cl emits its banner and a nonzero status without input; this is metadata,
        # not a compiler capability pass. Configure/build must still succeed.
        run("compiler-version", [cxx] if Path(cxx).name.lower() == "cl" else [cxx, "--version"],
            require_success=False)
        source_files = subprocess.check_output(["git", "ls-files", "experiments/scg08_arithmetic_work",
                                                "include", "cmake", "CMakeLists.txt",
                                                "scripts/ci/validate_scg08_gate1_evidence.py"],
                                               cwd=source, text=True).splitlines()
        record["source_sha256"] = {
            name: hashlib.sha256((source / name).read_bytes()).hexdigest()
            for name in source_files
        }
        configure = ["cmake", "-S", source, "-B", build, "-G", "Ninja",
                     "-DCMAKE_BUILD_TYPE=Release", f"-DCMAKE_C_COMPILER={cc}",
                     f"-DCMAKE_CXX_COMPILER={cxx}", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
                     f"-DPython3_EXECUTABLE={sys.executable}", "-DT81_BUILD_EXPERIMENTS=ON",
                     "-DT81_BUILD_TESTS=ON", "-DT81_BUILD_BENCHMARKS=OFF",
                     "-DT81_BUILD_FUZZ_TESTS=OFF", "-DT81_BUILD_EXAMPLES=OFF",
                     "-DT81_BUILD_TUI=OFF", "-DT81_ENABLE_TERNARYOS=OFF"]
        if args.asio_source:
            configure.append(f"-DFETCHCONTENT_SOURCE_DIR_ASIO={args.asio_source.resolve()}")
        run("configure", configure)
        asio = args.asio_source.resolve() if args.asio_source else build / "_deps/asio-src"
        record["asio_sha"] = run("asio-sha", ["git", "rev-parse", "HEAD"], cwd=asio).strip()
        entries = json.loads((build / "compile_commands.json").read_text())
        target_commands = [entry for entry in entries
                           if Path(entry["file"]).name == "test_exp_scg08_01.cpp"]
        if len(target_commands) != 1:
            raise RuntimeError("Expected exactly one SCG08 translation unit")
        command = target_commands[0].get("command", " ".join(target_commands[0].get("arguments", [])))
        if not re.search(r"(?:-D|/D)NDEBUG\b", command):
            raise RuntimeError("SCG08 compile command is missing NDEBUG")
        record["experiment_compile_command"] = command
        run("build", ["cmake", "--build", build, "--config", "Release", "--target",
                      "t81_exp_scg08_01_test", "--parallel", "2", "--verbose"], timeout=900)
        inventory = json.loads(run("ctest-inventory", ["ctest", "--test-dir", build,
                               "-C", "Release", "-R", "^t81_exp_scg08_01", "--show-only=json-v1"]))
        names = [test["name"] for test in inventory["tests"]]
        if len(names) != 5 or set(names) != EXPECTED_TESTS:
            raise RuntimeError(f"Wrong CTest inventory: {names}")
        record["selected_tests"] = names
        run("ctest", ["ctest", "--test-dir", build, "-C", "Release", "-R", "^t81_exp_scg08_01",
                      "--no-tests=error", "--output-on-failure", "--verbose", "--output-junit",
                      evidence / "junit.xml"], timeout=240)
        cases = ET.parse(evidence / "junit.xml").getroot().findall(".//testcase")
        if len(cases) != 5 or {case.get("name") for case in cases} != EXPECTED_TESTS:
            raise RuntimeError("JUnit does not record exactly the five requested tests")
        if any(case.find(tag) is not None for case in cases for tag in ("failure", "error", "skipped")):
            raise RuntimeError("JUnit reports a failed or skipped test")
        if not re.search(r"\[SCG-08 SCHEMA PARITY\] \d+ rejection cases and 2 positive records passed\.",
                         (evidence / "ctest.log").read_text(encoding="utf-8")):
            raise RuntimeError("Successful schema parity output missing")
        run("source-clean-after", ["git", "diff", "--exit-code", "HEAD"])
        record["status"] = "passed"
        record["tests_passed"] = 5
    except Exception as exc:
        record["failure"] = f"{type(exc).__name__}: {exc}"
        print(record["failure"], file=sys.stderr)
    finally:
        for origin, name in ((build / "CMakeCache.txt", "CMakeCache.txt"),
                             (build / "compile_commands.json", "compile_commands.json"),
                             (build / "Testing/Temporary/LastTest.log", "LastTest.log")):
            if origin.is_file():
                shutil.copyfile(origin, evidence / name)
        (evidence / "summary.json").write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")
        hashes = {p.name: hashlib.sha256(p.read_bytes()).hexdigest()
                  for p in evidence.iterdir() if p.is_file() and p.name != "sha256.json"}
        (evidence / "sha256.json").write_text(json.dumps(hashes, indent=2) + "\n", encoding="utf-8")
    return 0 if record["status"] == "passed" else 1


if __name__ == "__main__":
    raise SystemExit(main())
