"""Check the compiled TAnd SIMD registry and its complete timing results."""

import argparse
import json
import math
import re
import sys
from pathlib import Path

# BM_PackedTritVector.cpp registers TAnd with RangeMultiplier(4)->Range(16, 65536).
# TOr/TNot pure-kernel benchmarks do not exist. Enabled implementations come
# from an independent --benchmark_list_tests invocation of the measured binary.
SIZES = (16, 64, 256, 1024, 4096, 16384, 65536)
NAME = re.compile(r"BM_Kernel_TAnd_(SWAR|AVX2|NEON)/(16|64|256|1024|4096|16384|65536)")
UNITS = {"ns": 1.0, "us": 1e3, "ms": 1e6, "s": 1e9}
LIMIT = 1.15


def reject_constant(token):
    raise ValueError(f"non-finite JSON constant: {token}")


def finite_float(token):
    value = float(token)
    if not math.isfinite(value):
        raise ValueError(f"JSON number outside finite range: {token}")
    return value


def unique_object(pairs):
    result = {}
    for key, value in pairs:
        if key in result:
            raise ValueError(f"duplicate JSON key: {key}")
        result[key] = value
    return result


def load_results(path):
    with Path(path).open(encoding="utf-8") as stream:
        return json.load(stream, parse_constant=reject_constant,
                         parse_float=finite_float, object_pairs_hook=unique_object)


def expected_entries(path):
    names = Path(path).read_text(encoding="utf-8").splitlines()
    entries = {}
    implementations = {}
    for name in names:
        if not name.strip():
            continue
        match = NAME.fullmatch(name)
        if match is None:
            raise ValueError(f"unsupported compiled benchmark entry: {name!r}")
        if name in entries:
            raise ValueError(f"duplicate compiled benchmark entry: {name}")
        implementation, size = match[1], int(match[2])
        entries[name] = (implementation, size)
        implementations.setdefault(implementation, set()).add(size)
    if "SWAR" not in implementations:
        raise ValueError("compiled registry is missing the SWAR baseline")
    if not (set(implementations) - {"SWAR"}):
        raise ValueError("compiled registry has no enabled SIMD comparator")
    for implementation, sizes in implementations.items():
        if sizes != set(SIZES):
            raise ValueError(f"compiled {implementation} registry must cover sizes {SIZES}")
    return entries, sorted(set(implementations) - {"SWAR"})


def positive_timing(value, label, scale):
    if type(value) not in (int, float):
        raise ValueError(f"{label} must be a positive finite number")
    try:
        normalized = float(value) * scale
    except OverflowError as exc:
        raise ValueError(f"{label} is outside finite timing range") from exc
    if not math.isfinite(normalized) or normalized <= 0:
        raise ValueError(f"{label} must be a positive finite number")
    return normalized


def check_regression(bench_file, inventory_file):
    expected, comparators = expected_entries(inventory_file)
    data = load_results(bench_file)
    if not isinstance(data, dict) or not isinstance(data.get("benchmarks"), list):
        raise ValueError("benchmark input must be an object with a benchmarks array")
    if not data["benchmarks"]:
        raise ValueError("benchmark results are empty")
    timings = {}
    for index, row in enumerate(data["benchmarks"]):
        if not isinstance(row, dict):
            raise ValueError(f"benchmark row {index} must be an object")
        name = row.get("name")
        if not isinstance(name, str) or name not in expected:
            raise ValueError(f"unexpected benchmark name: {name!r}")
        if name in timings:
            raise ValueError(f"duplicate benchmark result: {name}")
        if row.get("run_type") != "iteration":
            raise ValueError(f"{name}: expected a raw iteration result")
        if type(row.get("iterations")) is not int or row["iterations"] <= 0:
            raise ValueError(f"{name}: iterations must be a positive integer")
        if type(row.get("error_occurred", False)) is not bool or row.get("error_occurred", False):
            raise ValueError(f"{name}: benchmark error or invalid error flag")
        unit = row.get("time_unit")
        if not isinstance(unit, str) or unit not in UNITS:
            raise ValueError(f"{name}: unsupported timing unit {unit!r}")
        real_time = positive_timing(row.get("real_time"), f"{name}.real_time", UNITS[unit])
        positive_timing(row.get("cpu_time"), f"{name}.cpu_time", UNITS[unit])
        timings[name] = real_time
    missing = set(expected) - set(timings)
    if missing:
        raise ValueError(f"missing benchmark results: {sorted(missing)}")

    comparisons = 0
    failures = []
    for size in SIZES:
        if size < 16384:
            continue
        baseline = timings[f"BM_Kernel_TAnd_SWAR/{size}"]
        for implementation in comparators:
            measured = timings[f"BM_Kernel_TAnd_{implementation}/{size}"]
            ratio = measured / baseline
            comparisons += 1
            print(f"TAnd {implementation}/{size}: {measured:.2f} ns vs SWAR {baseline:.2f} ns; ratio={ratio:.4f}")
            if not math.isfinite(ratio) or ratio > LIMIT:
                failures.append(f"{implementation}/{size}: ratio {ratio:.4f} exceeds {LIMIT}")
    if failures:
        raise ValueError("SIMD regression threshold exceeded: " + "; ".join(failures))
    print(f"Regression check passed: {len(timings)} results, {comparisons} comparator pairs checked.")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("benchmark_file", type=Path)
    parser.add_argument("--inventory", required=True, type=Path,
                        help="TAnd names from the same binary's --benchmark_list_tests output")
    args = parser.parse_args()
    try:
        check_regression(args.benchmark_file, args.inventory)
    except (OSError, UnicodeError, ValueError) as exc:
        print(f"[SIMD REGRESSION ERROR] {exc}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
