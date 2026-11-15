#!/usr/bin/env python3
"""Build and run bounded collector correctness tests, retaining raw OS records."""
import argparse
import io
import json
import os
from pathlib import Path
import platform
import shutil
import subprocess
import sys
from types import SimpleNamespace
import unittest
from unittest.mock import patch


class Log(io.StringIO):
    def writeln(self, text=""):
        self.write(text + "\n")


def main():
    parser = argparse.ArgumentParser(allow_abbrev=False)
    parser.add_argument("--validate-only", action="store_true", required=True)
    parser.add_argument("--build-dir", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--cc", default="cc")
    args = parser.parse_args()  # Unknown entry points rejected before imports/build/fixtures.
    import collector as c
    root = Path(__file__).resolve().parent
    evidence = dict(schema="base01.resource-correctness.v1", purpose="collector-correctness-only",
                    measurement_authorized=False, performance_samples=[],
                    audited_source_pin=c.SOURCE_PIN, accepted_foundation_pin=c.FOUNDATION_PIN,
                    host=dict(system=platform.system(), release=platform.release(),
                              version=platform.version(), architecture=platform.machine(),
                              python=sys.version), argv=sys.argv,
                    implementation_sha256={name: c.digest(root / name) for name in
                                           ("collector.py", "test_collector.py", "validate.py", "fixture.c")})
    for filename, key in (("/etc/os-release", "distribution"), ("/proc/cpuinfo", "cpuinfo")):
        if Path(filename).exists():
            evidence["host"][key] = Path(filename).read_text()
    try:
        c.supported()
    except c.AccountingError as exc:
        evidence.update(status="unsupported", error=str(exc), tests=dict(passed=0, failed=0, skipped=1))
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(evidence, indent=2) + "\n")
        return 2
    evidence["apis"] = {name: True for name in ("os.wait4", "os.posix_spawn", "getrusage(RUSAGE_SELF)", "getrusage(RUSAGE_CHILDREN)")}
    compiler = shutil.which(args.cc)
    if compiler is None:
        parser.error("compiler unavailable")
    args.build_dir.mkdir(parents=True, exist_ok=True)
    binary = args.build_dir.resolve() / "fixture"
    command = [compiler, "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", str(root / "fixture.c"), "-o", str(binary)]
    compilation = subprocess.run(command, capture_output=True, text=True)
    build = dict(command=command, compiler_version=subprocess.run([compiler, "--version"], capture_output=True, text=True).stdout,
                 compile_returncode=compilation.returncode, stdout=compilation.stdout, stderr=compilation.stderr,
                 fixture_source_sha256=c.digest(root / "fixture.c"))
    evidence["build"] = build
    if compilation.returncode:
        evidence.update(status="build-failed", tests=dict(passed=0, failed=1, skipped=0))
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(evidence, indent=2) + "\n")
        return 1
    build["fixture_binary_sha256"] = c.digest(binary)
    (args.build_dir / "build.json").write_text(json.dumps(build, indent=2) + "\n")
    import test_collector as tests
    tests.BUILD = args.build_dir
    tests.OBSERVATIONS.clear()
    log = Log()
    suite = unittest.defaultTestLoader.loadTestsFromModule(tests)
    # Drive TestResult directly: TextTestRunner.run() reads an elapsed clock.
    # Neither elapsed time nor duration-based acceptance belongs in this suite.
    result = unittest.TextTestResult(log, True, 2)
    result.startTestRun()
    # Recent unittest versions also time each TestCase internally. Replace
    # only that module binding with a constant sentinel; retain no durations.
    with patch.object(unittest.case, "time", SimpleNamespace(perf_counter=lambda: 0), create=True):
        suite.run(result)
    result.stopTestRun()
    result.printErrors()
    evidence["test_log"] = log.getvalue()
    evidence["tests"] = dict(run=result.testsRun, passed=result.testsRun - len(result.failures) - len(result.errors) - len(result.skipped),
                             failed=len(result.failures) + len(result.errors), skipped=len(result.skipped))
    evidence["raw_observations"] = tests.OBSERVATIONS
    evidence["status"] = "passed" if result.wasSuccessful() else "failed"
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(evidence, indent=2, allow_nan=False) + "\n")
    print(json.dumps(dict(status=evidence["status"], tests=evidence["tests"], evidence=str(args.output))))
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    sys.exit(main())
