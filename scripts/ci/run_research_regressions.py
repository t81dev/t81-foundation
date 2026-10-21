#!/usr/bin/env python3
"""Configure, build, and run the existing bounded research regression slice.

This does not reproduce historical timings or relabel hardened binaries as old
controls. See docs/research/consolidation/README.md for pinned historical runs.
"""
import argparse
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TARGETS = (
    "t81_accountable_experiment", "t81_concurrent_revocation",
    "t81_object_comparison", "t81_descriptor_comparison",
    "canonfs_policy_boundary_test", "canonfs_identity_contract_test",
    "t81_weights_load_test", "t81_vm_predispatch_policy_deny_logging_test",
)
TESTS = (
    "accountable_computation_experiment", "canonfs_concurrent_revocation_experiment",
    "object_interface_comparison", "validated_descriptor_comparison",
    *TARGETS[4:],
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", required=True, type=Path)
    parser.add_argument("--asio-source", type=Path, help="existing asio-1-30-2 source for offline builds")
    parser.add_argument("--jobs", type=int, default=2)
    args = parser.parse_args()
    if not __debug__:
        parser.error("run without -O: the unchanged object-interface test uses assertions")
    if args.jobs < 1:
        parser.error("--jobs must be positive")
    for tool in ("cmake", "ctest", "ninja", "openssl"):
        if shutil.which(tool) is None:
            parser.error(f"required tool missing: {tool}")
    build = args.build_dir.resolve()
    if build == ROOT:
        parser.error("use a separate build directory")
    configure = ["cmake", "-S", str(ROOT), "-B", str(build), "-G", "Ninja",
                 "-DCMAKE_BUILD_TYPE=Release", f"-DPython3_EXECUTABLE={sys.executable}"]
    configure += [f"-D{name}=ON" for name in (
        "T81_BUILD_TESTS", "T81_BUILD_ACCOUNTABLE_EXPERIMENT", "T81_BUILD_CONCURRENT_EXPERIMENT",
        "T81_BUILD_OBJECT_INTERFACE_EXPERIMENT", "T81_BUILD_DESCRIPTOR_EXPERIMENT")]
    configure += [f"-D{name}=OFF" for name in (
        "T81_BUILD_EXAMPLES", "T81_BUILD_BENCHMARKS", "T81_BUILD_FUZZ_TESTS", "T81_BUILD_TUI",
        "T81_ENABLE_TERNARYOS")]
    if args.asio_source:
        configure.append(f"-DFETCHCONTENT_SOURCE_DIR_ASIO={args.asio_source.resolve()}")
    subprocess.run(configure, check=True)
    subprocess.run(["cmake", "--build", str(build), "--config", "Release", "--parallel", str(args.jobs),
                    "--target", *TARGETS], check=True)
    ctest = ["ctest", "--test-dir", str(build), "-C", "Release"]
    inventory = json.loads(subprocess.check_output([*ctest, "--show-only=json-v1"], text=True))
    missing = set(TESTS) - {test["name"] for test in inventory["tests"]}
    if missing:
        raise RuntimeError(f"required tests not registered: {sorted(missing)}")
    pattern = "^(" + "|".join(re.escape(name) for name in TESTS) + ")$"
    subprocess.run([*ctest, "--output-on-failure", "--no-tests=error", "-R", pattern], check=True)


if __name__ == "__main__":
    main()
