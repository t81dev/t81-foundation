# T81 BASE-PERF-00 — Correctness-Verified `Cell` Throughput Baseline

## Scope and Governance Notice

This record documents an authorized, bounded **BASE-PERF-00** engineering throughput baseline for `t81::core::Cell` (5-trit balanced ternary cell, range `[-121, +121]`) using the existing TCB-Core `benchmark_runner` infrastructure.

- **Not an SCG-08 Gate 2 experiment:** Does not execute SCG-08 measurement campaigns, modify SCG-08 historical evidence, or alter RFC-00E1 status (`DRAFT`, Gate 1 `WITHHELD`, Gate 2 `STRICT NO-GO`).
- **Unmodified production arithmetic:** `include/t81/types/cell.hpp` is completely untouched (`0` lines modified) so that these measurements establish an unoptimized-against-history engineering reference point.
- **Historical reports preserved:** `docs/reference/benchmarks.md` and `docs/reference/benchmarks_engineering.md` are unmodified.
- **No comparative superiority claims:** Binary comparative performance superiority is **NOT EVALUATED**.

---

## Phase A — Execution Identity & Provenance Classification

### Provenance Classification

- **Historical raw JSON preserved:** `docs/benchmarks/base_perf_00_cell_throughput_raw.json` (timestamp `2026-11-12T07:15:14-05:00`) is preserved byte-for-byte.
- **Author-reported build & environment assertions:** No authentic retained build or execution receipt (such as a binary digest, build log, or signed run receipt) exists for the original `2026-11-12T07:15:14-05:00` execution. Accordingly, its build configuration, compiler, flags, SIMD target, OS/kernel/virtualization assertions, and the assertion that `include/t81/types/cell.hpp` was unmodified when that binary was built are classified as **author-reported** (unverified by a retained receipt).
- **Original source-to-binary binding:** **UNVERIFIED** (no retained digest or receipt binds the repository source to `/Users/t81dev/Code/t81-foundation/.cache/base-perf-00-worktree/build-release/benchmarks/benchmark_runner` at `2026-11-12T07:15:14-05:00`).
- **Fresh reproduction vs. original authentication:** Any fresh build, CTest execution (`t81_determinism_primitives_test`), or Codex reproduction on current source validates only the current working tree on the local host and **does not authenticate or retroactively verify the original `2026-11-12T07:15:14-05:00` execution or its source-to-binary binding**.

### Author-Reported Build & Environment Identity (Original Run, Unverified by Retained Receipt)

| Property | Author-Reported Value |
|---|---|
| **Baseline `main` Commit SHA** | `d3a6cf55517a3098d7bae91b07c72694bc3472da` |
| **Host CPU** | Apple M2 (`machdep.cpu.brand_string: Apple M2`) |
| **Architecture** | `arm64` (`arm64-apple-darwin25.5.0`), 8 logical/physical cores (`hw.ncpu: 8`), L1D 64 KiB, L1I 128 KiB, L2 Unified 4 MiB |
| **System Memory** | 8 GiB (`hw.memsize: 8589934592`) |
| **Operating System / Kernel** | macOS 26.5.2 (Build `25F84`), Darwin Kernel Version 25.5.0 (`RELEASE_ARM64_T8112`) |
| **Virtualization Check** | Physical host (`sysctl kern.hv_vmm_present = 0`, non-virtualized local developer workstation: MacBook Air M2; not a dedicated datacenter reference server) |
| **Compiler** | Apple clang version 21.0.0 (`clang-2100.1.1.101`, `/usr/bin/c++`) |
| **Build Configuration** | CMake 4.3 `Release` (`-O3 -DNDEBUG -std=gnu++23 -arch arm64 -fPIE -Wall -Wextra -Wpedantic -DBENCHMARK_STATIC_DEFINE -DT81_DETERMINISTIC`) |
| **SIMD Compile Target** | `AVX2=0, SSE4.2=0, NEON=1` |
| **Benchmark Library** | Google Benchmark `v1.9.5` |
| **Real Arithmetic Verification** | Confirmed in `include/t81/types/cell.hpp`: `operator+` executes 5-trit balanced-ternary ripple-carry addition; `operator-` executes trit-wise negation plus ripple-carry addition; `operator*` executes 5-trit shift-and-add; `operator/` executes `to_int()`, `int64_t` division, and `from_int()`; `from_int` / `to_int` execute base-3 balanced-ternary conversion. |

---

## Phase B — Numerical Correctness Oracle Results

Correctness was validated against a host `int64_t` oracle in two places:
1. **Standalone CTest executable (`t81_determinism_primitives_test` in `tests/determinism/test_primitives.cpp`):** Exhaustively tests the full `Cell` state space (`[-121, +121]`, $3^5 = 243$ states, $243^2 = 59{,}049$ ordered pairs) plus the exact seeded benchmark input sequences.
2. **Untimed prevalidation phase inside `arith_throughput.cpp` and `roundtrip_accuracy.cpp`:** Prevalidates all $10{,}000$ deterministic inputs and boundary/exception conditions before any timed loop executes. If prevalidation fails, the benchmark aborts via `state.SkipWithError(...)`.

| Operation / Check | Tested Domain | Oracle Result | Details |
|---|---|---|---|
| **Conversion Round-Trip (`from_int` / `to_int`)** | Exhaustive `[-121, +121]` + out-of-range `{-10000, -243, -122, +122, +243, +10000}` | **PASS** (`243 / 243` exact; `6 / 6` out-of-range throw `std::overflow_error`) | Bijective round-trip, canonical trit uniqueness, strict ordering (`prev < c`), and involution (`-(-c) == c`). |
| **Addition (`operator+`)** | Exhaustive `[-121, +121]^2` (`59,049` pairs) | **PASS** (`0` mismatches) | `44,287` in-range pairs match `int64_t` `a + b` exactly; all `14,762` pairs with `|a + b| > 121` deterministically throw `std::overflow_error`. |
| **Subtraction (`operator-`)** | Exhaustive `[-121, +121]^2` (`59,049` pairs) | **PASS** (`0` mismatches) | `44,287` in-range pairs match `int64_t` `a - b` exactly; all `14,762` pairs with `|a - b| > 121` deterministically throw `std::overflow_error`. |
| **Multiplication (`operator*`) — Benchmark Domain** | Exhaustive `[-10, +10]^2` (`441` pairs) | **PASS** (`441 / 441` exact, `0` exceptions) | All **operands** in `[-10, +10]` fit in three balanced trits (trits 0–2, `1 + 3 + 9 = 13 >= 10`); products lie in `[-100, +100]` and can require five trits. Because operands fit in three trits, shifts in `operator*` stop at index 2, shifted operands (`*this << i`, `i <= 2`) have magnitude `<= 10 * 9 = 90 <= 121`, and partial sums remain representable in `[-121, +121]`; exhaustive testing confirms `441 / 441` pairs. |
| **Multiplication (`operator*`) — Full Domain Characterization** | Exhaustive `[-121, +121]^2` (`59,049` pairs) | **PASS** (`0` silent corruptions) | `2,553` pairs succeed and match `int64_t` `a * b` exactly; `56,144` pairs with `|a * b| > 121` throw `std::overflow_error`; `352` pairs with `|a * b| <= 121` throw `std::overflow_error` due to intermediate 5-trit shift (`*this << i`) or partial-sum overflow (documented limitation below). |
| **Division (`operator/`)** | Exhaustive `[-121, +121]^2` (`59,049` pairs) | **PASS** (`0` mismatches) | All `58,806` pairs with `b != 0` match C++ truncated `int64_t` division `a / b`; all `243` pairs with `b == 0` deterministically throw `std::domain_error`. |
| **Seeded Timed Workload Prevalidation** | `std::mt19937_64(0x781)`, `DATA_SIZE = 10,000` | **PASS** (`0` failures across `50,000` prevalidated operations) | Arithmetic pairs in `[-10, +10]` (`val_b == 0 -> 1`) and conversion inputs in `[-121, +121]` prevalidated before timing. |

---

## Phase C — Measurement-Blocking Defects Repaired

1. **Separated per-operation measurement & exception accounting (`benchmarks/runner/arith_throughput.cpp`):**
   - Previously, `BM_ArithThroughput_T81Cell` bundled `+`, `-`, `*`, and `/` in a single loop and caught `std::exception` with `continue` while unconditionally reporting `state.SetItemsProcessed(state.iterations() * DATA_SIZE * 4)`.
   - Added dedicated 5-repetition benchmarks (`BM_CellAdd_T81Cell`, `BM_CellSub_T81Cell`, `BM_CellMul_T81Cell`, `BM_CellDiv_T81Cell`), prevalidated inputs before timing, tracked `completed_ops` and `failed_ops` explicitly, and called `state.SkipWithError(...)` if `failed_ops > 0`.
   - Explicitly labeled `BM_CellDiv_T81Cell` with `semantics=to_int+int64_div+from_int` and `convs_per_op=3` because `Cell::operator/` converts operands via `to_int()`, divides in `int64_t`, and converts back via `from_int()`.
2. **Compiler dead-code / `constexpr`-folding prevention & untimed oracle separation (`benchmarks/runner/roundtrip_accuracy.cpp`):**
   - Previously, `BM_RoundtripAccuracy_T81Cell` evaluated `if (Cell::from_int(val).to_int() == val)` inside the timed loop without `DoNotOptimize` barriers on the input `int64_t`, intermediate `Cell`, or output `int64_t`.
   - Added `BM_CellRoundtrip_T81Cell` (and updated `BM_RoundtripAccuracy_T81Cell`) with untimed oracle prevalidation and explicit `DoNotOptimize(val)`, `DoNotOptimize(cell)`, `DoNotOptimize(roundtrip)`, and `ClobberMemory()` barriers inside the timed loop.
3. **Per-repetition retention and median aggregation (`benchmarks/runner/benchmark_runner.cpp`):**
   - Previously, `CustomReporter::ReportRuns` overwrote `final_results[family]` on each `RT_Iteration` repetition, losing earlier repetitions and reporting only the final repetition.
   - `BenchmarkResult` now retains all per-repetition throughput and latency values, computes the **median** across repetitions (`ComputeMedian`), and reports per-repetition raw values, median, min, max, range, standard deviation, and CV% when `T81_BENCHMARK_VERBOSE_CONSOLE=1` is set.
4. **Removal of unsupported `apples-to-apples` claims (`benchmarks/runner/arith_throughput.cpp`, `benchmarks/runner/benchmark_runner.cpp`):**
   - Removed `comparison=apples-to-apples` from `arith_throughput.cpp` (`comparison=not-evaluated`) and updated `FairnessFor` / `UserRow` in `benchmark_runner.cpp` so benchmarks labeled `comparison=not-evaluated` or `comparison=semantic-tax` are never classified as `"apples-to-apples"` or assigned comparative win/loss verdicts.

---

## Phase D — Bounded Measurement Results (HOST-LOCAL / PHYSICAL WORKSTATION)

### Exact Build and Execution Commands

```bash
# 1. Configure Release build (using local Google Benchmark v1.9.5 source cache)
cmake -S . -B build-release \
  -DCMAKE_BUILD_TYPE=Release \
  -DT81_BUILD_TUI=OFF \
  -DFETCHCONTENT_SOURCE_DIR_GOOGLEBENCHMARK=/Users/t81dev/Code/t81-foundation/build/_deps/googlebenchmark-src \
  -DFETCHCONTENT_SOURCE_DIR_ASIO=/Users/t81dev/Code/t81-foundation/build/_deps/asio-src

# 2. Build correctness test and benchmark_runner
cmake --build build-release --target t81_determinism_primitives_test benchmark_runner -j4

# 3. Execute exhaustive host-integer oracle correctness test
./build-release/t81_determinism_primitives_test

# 4. Execute bounded BASE-PERF-00 benchmark (5 repetitions per operation after 0.05s warmup)
T81_BENCHMARK_VERBOSE_CONSOLE=1 T81_BENCHMARK_WRITE_REPORTS=0 \
  ./build-release/benchmarks/benchmark_runner \
  --benchmark_filter='^BM_Cell(Add|Sub|Mul|Div|Roundtrip)_T81Cell' \
  --benchmark_min_warmup_time=0.05 \
  --benchmark_min_time=0.05s \
  --benchmark_out=docs/benchmarks/base_perf_00_cell_throughput_raw.json \
  --benchmark_out_format=json
```

### Summary of Measured `Cell` CPU-Time Throughput (5 Repetitions After 0.05s Warm-Up, `DATA_SIZE = 10,000` ops/iter)

> **Timing Semantics:** All throughput figures (`ops/s`, `Mops/s`) below are **CPU-time based** (`completed_ops / cpu_time`), because Google Benchmark `UseRealTime()` is not enabled. Wall-clock batch iteration times (`real_time`) are presented separately below, and **each benchmark iteration processes a batch of `10,000` operations** (`DATA_SIZE = 10,000`). Source labels in `benchmarks/runner/arith_throughput.cpp` and `benchmarks/runner/roundtrip_accuracy.cpp` have been corrected from `timing=real_time` to `timing=cpu_time` without adding `UseRealTime()` or changing measurement semantics; `docs/benchmarks/base_perf_00_cell_throughput_raw.json` is preserved byte-for-byte with its original `timing=real_time` label string.

| Operation | Benchmark Name | Semantics | Domain | Iterations / Rep (`10,000` ops/iter) | Median CPU-Time Throughput | Mean CPU-Time Throughput | Min – Max (Range) | StdDev (CV%) | Failed Ops |
|---|---|---|---|---:|---:|---:|---:|---:|---:|
| **1. Cell Addition** | `BM_CellAdd_T81Cell` | 5-trit unrolled ripple-carry `+` | `[-10, +10]^2` | `1,177` | **163.39 Mops/s** (`163,388,259 ops/s`) | `163.80 Mops/s` | `159.34 – 168.30 Mops/s` (`8.95 Mops/s`) | `3.24 Mops/s` (`1.97%`) | `0` |
| **2. Cell Subtraction** | `BM_CellSub_T81Cell` | 5-trit negate + ripple-carry `-` | `[-10, +10]^2` | `773` | **139.04 Mops/s** (`139,036,279 ops/s`) | `133.27 Mops/s` | `120.43 – 139.66 Mops/s` (`19.23 Mops/s`) | `8.67 Mops/s` (`6.51%`) | `0` |
| **3. Cell Multiplication** | `BM_CellMul_T81Cell` | 5-trit shift-and-add `*` | `[-10, +10]^2` | `157` | **22.34 Mops/s** (`22,344,301 ops/s`) | `22.37 Mops/s` | `22.31 – 22.50 Mops/s` (`0.18 Mops/s`) | `72.96 Kops/s` (`0.33%`) | `0` |
| **4. Cell Division** | `BM_CellDiv_T81Cell` | Conversion-backed `to_int + int64_div + from_int` | `[-10, +10] \times ([-10, +10] \setminus \{0\})` | `349` | **61.60 Mops/s** (`61,596,569 ops/s`) | `58.36 Mops/s` | `50.69 – 64.20 Mops/s` (`13.51 Mops/s`) | `6.78 Mops/s` (`11.61%`) | `0` |
| **5. Cell Conversion Round-Trip** | `BM_CellRoundtrip_T81Cell` | Materialized `Cell::from_int(v)` + `cell.to_int()` | `[-121, +121]` | `173` | **24.84 Mops/s** (`24,840,261 ops/s`) | `24.96 Mops/s` | `24.20 – 25.97 Mops/s` (`1.78 Mops/s`) | `641.30 Kops/s` (`2.57%`) | `0` |

### Wall-Clock Batch Iteration Times (`10,000` Operations per Iteration)

Each Google Benchmark iteration executes a batch of `10,000` operations (`DATA_SIZE = 10,000`). The table below reports wall-clock (`real_time`) batch iteration times separately from the CPU-time throughput figures above, alongside the corresponding median `cpu_time` per `10,000`-operation batch iteration.

| Operation | Benchmark Name | Batch Size (ops / iter) | Median Wall-Clock Time / Batch Iter (`real_time`) | Median CPU Time / Batch Iter (`cpu_time`) |
|---|---|---:|---:|---:|
| **1. Cell Addition** | `BM_CellAdd_T81Cell` | `10,000` | `61.55 µs` (`61,550.23 ns`) | `61.20 µs` (`61,203.91 ns`) |
| **2. Cell Subtraction** | `BM_CellSub_T81Cell` | `10,000` | `72.05 µs` (`72,045.65 ns`) | `71.92 µs` (`71,923.67 ns`) |
| **3. Cell Multiplication** | `BM_CellMul_T81Cell` | `10,000` | `447.54 µs` (`447,542.99 ns`) | `447.54 µs` (`447,541.40 ns`) |
| **4. Cell Division** | `BM_CellDiv_T81Cell` | `10,000` | `162.34 µs` (`162,343.36 ns`) | `162.35 µs` (`162,346.70 ns`) |
| **5. Cell Conversion Round-Trip** | `BM_CellRoundtrip_T81Cell` | `10,000` | `402.58 µs` (`402,577.55 ns`) | `402.57 µs` (`402,572.25 ns`) |

### Raw Per-Repetition Measurements (`CPU-time ops/s` / `wall-clock real_time_ns per 10,000-op iter`)

| Operation | Rep 0 (`CPU ops/s` / `real_time_ns`) | Rep 1 (`CPU ops/s` / `real_time_ns`) | Rep 2 (`CPU ops/s` / `real_time_ns`) | Rep 3 (`CPU ops/s` / `real_time_ns`) | Rep 4 (`CPU ops/s` / `real_time_ns`) |
|---|---|---|---|---|---|
| **`BM_CellAdd_T81Cell`** | `164,869,029` / `61,039.86 ns` | `159,344,751` / `64,593.21 ns` | `168,296,728` / `59,422.22 ns` | `163,107,496` / `62,667.20 ns` | `163,388,259` / `61,550.23 ns` |
| **`BM_CellSub_T81Cell`** | `120,429,370` / `83,056.98 ns` | `128,045,851` / `78,677.23 ns` | `139,036,279` / `72,045.65 ns` | `139,168,947` / `71,883.68 ns` | `139,656,730` / `71,604.57 ns` |
| **`BM_CellMul_T81Cell`** | `22,495,415` / `444,534.24 ns` | `22,312,546` / `448,250.00 ns` | `22,332,542` / `447,778.93 ns` | `22,355,119` / `447,331.74 ns` | `22,344,301` / `447,542.99 ns` |
| **`BM_CellDiv_T81Cell`** | `61,596,569` / `162,343.36 ns` | `63,960,414` / `156,346.23 ns` | `64,201,619` / `155,759.07 ns` | `51,366,587` / `194,861.03 ns` | `50,694,323` / `197,257.64 ns` |
| **`BM_CellRoundtrip_T81Cell`** | `24,809,981` / `403,062.62 ns` | `24,197,835` / `416,819.36 ns` | `24,840,261` / `402,577.55 ns` | `24,986,279` / `400,279.38 ns` | `25,973,246` / `385,010.36 ns` |

---

## Limitations and Unresolved Technical Findings

1. **Conversion-backed `Cell::operator/`:**
   - Despite the source comment in `include/t81/types/cell.hpp` (`Division (restoring division, exact when divisible)`), `Cell::operator/` actually converts both `Cell` operands to `int64_t` via `to_int()`, performs host integer division `a / b`, and converts back via `Cell::from_int(a / b)`. Consequently, `BM_CellDiv_T81Cell` measures a hybrid conversion + host-division path (`3` conversions per division), not a native balanced-ternary restoring division algorithm.
2. **Intermediate shift overflow in `Cell::operator*` outside `[-13, +13]`:**
   - `Cell::operator*` computes `*this << i` into a 5-trit `Cell` before adding or subtracting into the running 5-trit accumulator. When the multiplicand uses trit 3 or 4 (`|a| > 13`) and the multiplier has non-zero trits at index `i >= 1` that cancel down into range (e.g., `50 * 2 = 100`, where `2 = 3 - 1`), `*this << 1` (`150 > 121`) throws `std::overflow_error("Shift overflow (non-zero trit lost)")` even though the true product `100` is within `[-121, +121]`. Across all `59,049` pairs in `[-121, +121]^2`, `352` mathematically in-range pairs throw `std::overflow_error` for this reason (`0` pairs ever produce a wrong numerical answer).
   - **Bounded safety argument for `[-10, +10]^2`:** Within the benchmark operand domain `[-10, +10]^2`, all **operands** fit in three balanced trits (`[-13, +13]`, indices `0..2`), whereas products lie in `[-100, +100]` and can require five trits (`[-121, +121]`). Because operands fit in three trits, multiplier trits at indices `3` and `4` are `0`, so shifts `*this << i` stop at index `2`; every shifted operand (`*this << i` for `i <= 2`) has magnitude `<= 10 * 9 = 90 <= 121` (and loses no non-zero trit); and all partial sums remain representable within `[-121, +121]` (`|s_0| <= 10`, `|s_1| <= 10 + 30 = 40`, `|s_2| <= 100`). Exhaustive testing confirms `441 / 441` pairs succeed with exact `int64_t` agreement and `0` exceptions. Widening the internal accumulator of `Cell::operator*` is intentionally deferred so production arithmetic remains untouched during BASE-PERF-00 baseline establishment.
3. **Host-local workstation environment:**
   - Although the host is physical (`kern.hv_vmm_present = 0`), it is a passively cooled Apple M2 workstation (`MacBook Air`) with background OS activity (`load_avg ~ 4.38–4.86`) and no `pthread` thread-affinity pinning on macOS/Darwin (`PTHREAD_AFFINITY` unsupported). Therefore, this establishes a **HOST-LOCAL THROUGHPUT BASELINE**, not a locked datacenter **REFERENCE-HARDWARE BASELINE**.

---

## Dispositions

- **NUMERICAL CORRECTNESS:** `PASS`
- **MEASUREMENT ACCOUNTING:** `PASS`
- **HOST-LOCAL THROUGHPUT BASELINE:** `ESTABLISHED`
- **REFERENCE-HARDWARE BASELINE:** `NOT ESTABLISHED`
- **BINARY COMPARATIVE ADVANTAGE:** `NOT EVALUATED`

