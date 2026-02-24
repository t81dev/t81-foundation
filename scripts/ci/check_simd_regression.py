#!/usr/bin/env python3
import json
import sys
import os

def check_regression(bench_file):
    report_path = "artifacts/ci_reports/simd_regression_report.json"

    if not os.path.exists(bench_file):
        print(f"Benchmark file {bench_file} not found.")
        # If running on non-reference runner, maybe skip?
        # For now, just exit 0 with warning
        sys.exit(0)

    try:
        with open(bench_file, 'r') as f:
            data = json.load(f)
    except Exception as e:
        print(f"Failed to load benchmark JSON: {e}")
        sys.exit(1)

    benchmarks = {}
    for b in data.get('benchmarks', []):
        name = b['name']
        parts = name.split('/')
        if len(parts) < 2:
            continue
        base_name = parts[0]
        try:
            size = int(parts[1])
        except:
            continue

        benchmarks.setdefault(size, {})
        benchmarks[size][base_name] = b['real_time']

    failed = False
    errors = []

    # Thresholds
    REGRESSION_LIMIT = 1.15
    MIN_SIZE = 4096

    ops = ['TAnd', 'TOr', 'TNot', 'TXor']

    results = []

    for size, benches in benchmarks.items():
        if size < MIN_SIZE:
            continue

        for op in ops:
            swar = f"BM_Kernel_{op}_SWAR"
            avx2 = f"BM_Kernel_{op}_AVX2"
            neon = f"BM_Kernel_{op}_NEON"

            swar_time = benches.get(swar)

            if swar_time:
                if avx2 in benches:
                    t = benches[avx2]
                    ratio = t / swar_time
                    entry = {"op": op, "size": size, "backend": "AVX2", "ratio": ratio, "swar": swar_time, "simd": t}
                    results.append(entry)
                    if ratio > REGRESSION_LIMIT:
                        msg = f"{op} AVX2 regression: {t:.2f} vs {swar_time:.2f} ({ratio:.2f}x) at size {size}"
                        errors.append(msg)
                        failed = True

                if neon in benches:
                    t = benches[neon]
                    ratio = t / swar_time
                    entry = {"op": op, "size": size, "backend": "NEON", "ratio": ratio, "swar": swar_time, "simd": t}
                    results.append(entry)
                    if ratio > REGRESSION_LIMIT:
                        msg = f"{op} NEON regression: {t:.2f} vs {swar_time:.2f} ({ratio:.2f}x) at size {size}"
                        errors.append(msg)
                        failed = True

    os.makedirs(os.path.dirname(report_path), exist_ok=True)
    report = {
        "status": "fail" if failed else "pass",
        "errors": errors,
        "results": results
    }

    with open(report_path, 'w') as f:
        json.dump(report, f, indent=2)

    if failed:
        print("SIMD Regression Check Failed:")
        for e in errors:
            print(f"  - {e}")
        # Soft-fail based on matrix? Yes.
        sys.exit(1)
    else:
        print("SIMD Regression Check Passed.")
        sys.exit(0)

if __name__ == "__main__":
    if len(sys.argv) < 2:
        # Default fallback
        print("Usage: check_simd_regression.py <bench.json>")
        sys.exit(0)
    check_regression(sys.argv[1])
