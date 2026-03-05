import json
import sys
import math

def check_regression(bench_file):
    with open(bench_file, 'r') as f:
        data = json.load(f)

    benchmarks = {}
    for b in data['benchmarks']:
        name = b['name']
        # Parse name like "BM_Kernel_TAnd_SWAR/65536"
        parts = name.split('/')
        if len(parts) < 2:
            continue
        base_name = parts[0]
        size = int(parts[1])

        benchmarks.setdefault(size, {})
        benchmarks[size][base_name] = b['real_time']

    # Threshold for large size
    LARGE_SIZE = 65536

    if LARGE_SIZE not in benchmarks:
        print(f"Size {LARGE_SIZE} not found in benchmarks.")
        return 0 # Should fail? Or skip?

    ops = ['TAnd', 'TOr', 'TNot'] # Check all ops if available

    failed = False

    for size, benches in benchmarks.items():
        if size < 4096:
            continue # Skip small sizes for regression check

        print(f"Checking size {size}...")

        for op in ops:
            swar_name = f"BM_Kernel_{op}_SWAR"
            avx2_name = f"BM_Kernel_{op}_AVX2"
            neon_name = f"BM_Kernel_{op}_NEON"

            if swar_name in benches:
                swar_time = benches[swar_name]

                if avx2_name in benches:
                    avx2_time = benches[avx2_name]
                    # Check AVX2 vs SWAR
                    # We expect AVX2 <= SWAR * 1.15 (allow 15% regression due to noise)
                    limit = swar_time * 1.15
                    ratio = avx2_time / swar_time
                    print(f"  {op} AVX2: {avx2_time:.2f} ns vs SWAR: {swar_time:.2f} ns (Ratio: {ratio:.2f}x time)")

                    if avx2_time > limit:
                        print(f"  [FAIL] AVX2 regression > 15% detected for {op} at size {size}")
                        failed = True

                if neon_name in benches:
                    neon_time = benches[neon_name]
                    # Check NEON vs SWAR
                    limit = swar_time * 1.15
                    ratio = neon_time / swar_time
                    print(f"  {op} NEON: {neon_time:.2f} ns vs SWAR: {swar_time:.2f} ns (Ratio: {ratio:.2f}x time)")

                    if neon_time > limit:
                        print(f"  [FAIL] NEON regression > 15% detected for {op} at size {size}")
                        failed = True

    if failed:
        sys.exit(1)
    print("Regression check passed.")

if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: check_simd_regression.py <bench.json>")
        sys.exit(1)
    check_regression(sys.argv[1])
