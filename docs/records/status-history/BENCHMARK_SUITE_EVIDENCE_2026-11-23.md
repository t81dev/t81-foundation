# Benchmark Suite Evidence — 2026-11-23

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Benchmark Suite Evidence — 2026-11-23](#benchmark-suite-evidence-—-2026-11-23)
  - [Scope](#scope)
  - [Hardware caveat](#hardware-caveat)
  - [Environment](#environment)
  - [Commands](#commands)
  - [Within-run guardrails](#within-run-guardrails)
    - [VM dispatch / native loop](#vm-dispatch--native-loop)
    - [`BM_Kernel_TAnd` AVX2 versus SWAR](#`bm_kernel_tand`-avx2-versus-swar)
  - [Comparison with stored baselines](#comparison-with-stored-baselines)
    - [BASE-PERF-00 `Cell` throughput](#base-perf-00-`cell`-throughput)
    - [Inference comparison published table](#inference-comparison-published-table)
    - [Other stored benchmark text](#other-stored-benchmark-text)
  - [Smoke mean aggregates on this guest](#smoke-mean-aggregates-on-this-guest)
  - [Not run](#not-run)
  - [Raw artifacts](#raw-artifacts)
  - [Disposition](#disposition)

<!-- T81-TOC:END -->


Date: 2026-11-23
Status: Recorded
Source commit: `5a005b2dea5e1eb262d4af45698f8338d16b137c` (`main`)

## Scope

This record stores one run of the existing `benchmark_runner` suite on the
commit above. The run follows the documented build script and the benchmark
lanes already wired in CI:

- default smoke profile (`benchmarks/README.md`, `docs/user-guide/quickstart/INSTALL.md`, `kDefaultSmokeFilter` in `benchmarks/runner/benchmark_runner.cpp`)
- VM workload guardrail (`.github/workflows/bench.yml` and the `benchmark / vm workload gate` job in `.github/workflows/ci.yml`)
- packed-trit `TAnd` SIMD guardrail (the `benchmark / linux-x86_64` job in `.github/workflows/ci.yml`)
- inference comparison families (`.github/workflows/inference-bench.yml`)

Benchmark sources, registration, and the checker thresholds in
`scripts/ci/check_vm_workload_benchmark_regression.py` and
`scripts/ci/check_simd_regression.py` were not modified. Historical published
results were left in place:

- `docs/benchmarks/base_perf_00_cell_throughput_raw.json`
- `docs/benchmarks/base_perf_00_cell_throughput_baseline.json`
- `benchmarks/results/inference_comparison.md`
- `docs/reference/benchmarks.md`

`T81_BENCHMARK_WRITE_REPORTS` was `0`, so the runner did not rewrite the
reference reports.

## Hardware caveat

These numbers come from a Cursor cloud VM. The guest is KVM-virtualized.
Absolute times and throughput are not comparable to the Apple M2 workstation
that produced BASE-PERF-00, to the MacBook Air that produced
`benchmarks/results/inference_comparison.md`, or to any other physical host.
The only comparisons that the repository itself treats as pass/fail gates are
within-run ratios. Those gates were executed unchanged and both passed. Cross
host ratios below are recorded so the raw numbers can be inspected. They are
not a source regression or improvement verdict.

## Environment

| Property | Value |
|---|---|
| Commit | `5a005b2dea5e1eb262d4af45698f8338d16b137c` |
| Commit subject | Merge pull request #848 from t81dev/cursor/canonfs-export-denial-before-materialization |
| Host name | `cursor` |
| Kernel | Linux `6.12.113+` SMP PREEMPT_DYNAMIC, x86_64 |
| Virtualization | KVM full guest (`systemd-detect-virt` = `kvm`; CPU flag `hypervisor`) |
| CPU | `Intel(R) Xeon(R) Processor`, family 6, model 207, stepping 2, 4 vCPU, 1 thread per core, reported `2400.000` MHz, BogoMIPS `4800.00` |
| CPU flags used by the compiler | `-march=native` on a guest that advertises AVX2, FMA, and AVX-512 |
| Caches reported by `lscpu` | L1d 192 KiB (4 instances), L1i 128 KiB (4 instances), L2 8 MiB (4 instances), L3 320 MiB |
| Caches reported by Google Benchmark | L1d 48 KiB, L1i 32 KiB, L2 2 MiB per CPU, L3 320 MiB shared by 4 CPUs |
| Memory | `MemTotal` 16,387,328 kB. About 5 GiB available at capture time. No swap. |
| `cpu_scaling_enabled` | false (Google Benchmark context) |
| Load average at smoke start | `[2.163, 1.182, 0.457]` |
| Load average at inference start | `[1.258, 1.078, 0.451]` |
| Compiler | `g++` (Ubuntu 13.3.0-6ubuntu2~24.04.1) 13.3.0, `/usr/bin/c++` |
| C compiler | `gcc` 13.3.0, `/usr/bin/cc` |
| CMake | 3.28.3 |
| Generator | Ninja 1.11.1 |
| Google Benchmark | v1.9.5, library build type `release` |
| Build type | `Release` |
| `CMAKE_CXX_FLAGS` | `-O3 -march=native -DNDEBUG` |
| `CMAKE_CXX_FLAGS_RELEASE` | `-O3 -DNDEBUG` |
| Extra target flags | `-std=gnu++23 -fPIE -Wall -Wextra -Wpedantic -mavx2 -mfma` |
| Compile defines on `benchmark_runner` | `-DBENCHMARK_STATIC_DEFINE -DT81_DETERMINISTIC -DT81_HAS_NATIVE_INT128=1` |
| CMake options from `scripts/ci/build_benchmark_runner.sh` | `T81_BUILD_BENCHMARKS=ON`, `T81_BUILD_TESTS=OFF`, `T81_BUILD_EXAMPLES=OFF`, `T81_BUILD_FUZZ_TESTS=OFF` |
| `T81_ENABLE_DPE` | `ON` (CMake default on this configure). The DPE filter was not part of this run. |
| Binary | `build/benchmarks/benchmark_runner` |
| Binary SHA-256 | `b0f868c8fca03b911e92148c2d1f3c804bdebe3fc459826920163555ce4e8710` |
| GNU build-id | `bd0181909a52ccc35a3e5cf8b46c9ce1a607a081` |

The image's `cc` and `c++` alternatives were manually pointed at Clang 18.1.3.
That Clang selects the GCC 14 library directory, and this image has
`libstdc++.so` only under GCC 13, so `/usr/bin/c++` could not link
(`cannot find -lstdc++`). `g++` linked a trivial program. The alternatives were
switched to `/usr/bin/gcc` and `/usr/bin/g++` before configure. That is the
higher-priority alternative on this image and the compiler family used by the
Ubuntu 24.04 CI jobs for this suite. No source file was changed to make the
link succeed.

Observed compile line for `benchmarks/runner/benchmark_runner.cpp`:

```text
/usr/bin/c++ -DBENCHMARK_STATIC_DEFINE -DT81_DETERMINISTIC -DT81_HAS_NATIVE_INT128=1 -I/workspace/build/_deps/googlebenchmark-src/include -I/workspace/include -I/workspace -O3 -march=native -DNDEBUG -O3 -DNDEBUG -std=gnu++23 -fPIE -Wall -Wextra -Wpedantic -O3 -DNDEBUG -mavx2 -mfma -c /workspace/benchmarks/runner/benchmark_runner.cpp
```

## Commands

Build, matching `.github/workflows/bench.yml`:

```bash
export T81_BENCH_CXX_FLAGS='-O3 -march=native -DNDEBUG'
bash scripts/ci/build_benchmark_runner.sh
```

Smoke, matching the runner's default smoke filter (the `bench.yml` speed-oracle
filter plus the five `BM_Cell*` names). With `T81_BENCHMARK_PROFILE` unset, the
runner also appends `--benchmark_min_time=0.02s`.

```bash
export T81_BENCHMARK_WRITE_REPORTS=0
./build/benchmarks/benchmark_runner \
  --benchmark_filter='BM_(CellAdd|CellSub|CellMul|CellDiv|CellRoundtrip|ArithThroughput|NegationSpeed|RoundtripAccuracy|overflow|PackingDensity|MemoryBandwidth|Add_1024_bit|Add_2048_bit|T81LangCompile|LimbArithThroughput|LimbAdd_T81Native|LimbAdd_T81Limb|LimbAdd_Int128|vs_).*' \
  --benchmark_format=json \
  --benchmark_out=/tmp/bench-evidence/bench-smoke.json
```

VM workload guardrail, matching `bench.yml` and `ci.yml`:

```bash
./build/benchmarks/benchmark_runner \
  --benchmark_filter='BM_VMSimulation_Dispatch|BM_NativeCall_Loop' \
  --benchmark_min_time=0.02s \
  --benchmark_format=json \
  --benchmark_out=/tmp/bench-evidence/bench-vm-workload.json
python3 scripts/ci/check_vm_workload_benchmark_regression.py \
  /tmp/bench-evidence/bench-vm-workload.json
```

SIMD guardrail, matching the `ci.yml` `benchmark / linux-x86_64` job. The same
unset profile appends `--benchmark_min_time=0.02s`.

```bash
./build/benchmarks/benchmark_runner \
  --benchmark_list_tests=true \
  --benchmark_filter='^BM_Kernel_TAnd_(SWAR|AVX2|NEON)/' \
  | grep '^BM_Kernel_TAnd_' \
  > /tmp/bench-evidence/simd-regression-inventory.txt
./build/benchmarks/benchmark_runner \
  --benchmark_filter='^BM_Kernel_TAnd_(SWAR|AVX2|NEON)/' \
  --benchmark_format=json \
  --benchmark_out=/tmp/bench-evidence/bench-simd.json
python3 scripts/ci/check_simd_regression.py \
  /tmp/bench-evidence/bench-simd.json \
  --inventory /tmp/bench-evidence/simd-regression-inventory.txt
```

Inference families, matching `.github/workflows/inference-bench.yml`
(`--benchmark_min_time=0.05s`, one family per process):

```bash
./build/benchmarks/benchmark_runner --benchmark_filter='^BM_DotProduct_' --benchmark_min_time=0.05s --benchmark_format=json --benchmark_out=/tmp/bench-evidence/bench-DotProduct.json
./build/benchmarks/benchmark_runner --benchmark_filter='^BM_MatMul_' --benchmark_min_time=0.05s --benchmark_format=json --benchmark_out=/tmp/bench-evidence/bench-MatMul.json
./build/benchmarks/benchmark_runner --benchmark_filter='^BM_Attention_' --benchmark_min_time=0.05s --benchmark_format=json --benchmark_out=/tmp/bench-evidence/bench-Attention.json
./build/benchmarks/benchmark_runner --benchmark_filter='^BM_WeightLoad_' --benchmark_min_time=0.05s --benchmark_format=json --benchmark_out=/tmp/bench-evidence/bench-WeightLoad.json
./build/benchmarks/benchmark_runner --benchmark_filter='^BM_TransformerLayer' --benchmark_min_time=0.05s --benchmark_format=json --benchmark_out=/tmp/bench-evidence/bench-Transformer.json
```

Wall clock for the measured processes was about 6 seconds for smoke and about
11 seconds for the five inference families. VM and SIMD each finished in the
same second they started. No benchmark row set `error_occurred`.

`benchmark_list_tests` against `NEON` matched no registrations. This x86_64
binary contains SWAR and AVX2 `TAnd` kernels only.

## Within-run guardrails

These checks compare measurements from this same binary. They do not depend on
another machine's absolute times. Both checkers were run as they exist on this
commit. Thresholds were not edited.

### VM dispatch / native loop

Limit in `scripts/ci/check_vm_workload_benchmark_regression.py`: ratio `3.0`
for args `32` and `256`.

| Arg | `BM_VMSimulation_Dispatch` real time | `BM_NativeCall_Loop` real time | Ratio | Checker |
|---:|---:|---:|---:|---|
| 32 | 2262.50 ns | 1980.73 ns | 1.142× | pass |
| 256 | 4266.77 ns | 3659.27 ns | 1.166× | pass |

Checker stdout: `docs/records/status-history/2026-11-23-benchmark-suite/vm-regression.txt`.

### `BM_Kernel_TAnd` AVX2 versus SWAR

The checker compares real time at sizes `16384` and `65536` only, and fails
when AVX2 / SWAR exceeds `1.15`. Smaller sizes are listed because they are in
the raw JSON. They are outside the gate.

| Size | SWAR real time | AVX2 real time | AVX2 / SWAR | Inside the gate |
|---:|---:|---:|---:|---|
| 16 | 2.2951 ns | 3.5678 ns | 1.5546× | no |
| 64 | 2.0244 ns | 3.3510 ns | 1.6553× | no |
| 256 | 2.7581 ns | 1.8083 ns | 0.6556× | no |
| 1024 | 7.1032 ns | 5.2109 ns | 0.7336× | no |
| 4096 | 23.5864 ns | 17.9644 ns | 0.7616× | no |
| 16384 | 92.8630 ns | 89.4592 ns | 0.9633× | yes, pass |
| 65536 | 354.5593 ns | 322.1484 ns | 0.9086× | yes, pass |

On this guest, AVX2 `TAnd` is slower than SWAR at 16 and 64 elements and faster
from 256 elements upward. The repository gate passes. Checker stdout:
`docs/records/status-history/2026-11-23-benchmark-suite/simd-regression.txt`.

## Comparison with stored baselines

### BASE-PERF-00 `Cell` throughput

Stored record: `docs/benchmarks/BASE_PERF_00_CELL_THROUGHPUT_BASELINE.md` and
`docs/benchmarks/base_perf_00_cell_throughput_baseline.json`.

That record is an Apple M2, macOS, Apple clang 21, arm64, NEON, physical
workstation measurement at commit `d3a6cf55517a3098d7bae91b07c72694bc3472da`,
with `--benchmark_min_warmup_time=0.05` and `--benchmark_min_time=0.05s`.
Its own text already classifies the result as host-local, and it says a later
run does not authenticate that original execution.

This run's `BM_Cell*` rows came from the default smoke profile:
`--benchmark_min_time=0.02s` injected by the runner, no separate warmup flag,
five repetitions, median of the iteration `items_per_second` values. Iteration
counts are lower than the stored record. The figure below is this VM's median
divided by the stored median. A value above 1 means this VM's median
throughput number is higher.

| Benchmark | This VM median | Stored M2 median | This VM / stored | Iterations this VM | Iterations stored |
|---|---:|---:|---:|---:|---:|
| `BM_CellAdd_T81Cell` | 163,586,697 ops/s | 163,388,259 ops/s | 1.001× | 400 | 1,177 |
| `BM_CellSub_T81Cell` | 167,959,242 ops/s | 139,036,279 ops/s | 1.208× | 496 | 773 |
| `BM_CellMul_T81Cell` | 33,891,376 ops/s | 22,344,301 ops/s | 1.517× | 90 | 157 |
| `BM_CellDiv_T81Cell` | 43,674,141 ops/s | 61,596,569 ops/s | 0.709× | 118 | 349 |
| `BM_CellRoundtrip_T81Cell` | 43,628,301 ops/s | 24,840,261 ops/s | 1.756× | 118 | 173 |

The five ratios point in different directions. Addition is numerically close.
Subtraction, multiplication, and round-trip are higher on this guest.
Division is lower. Because the CPU, ISA, compiler, OS, virtualization, minimum
time, warmup, and iteration count all differ, this spread is not evidence that
`main` got faster or slower.

### Inference comparison published table

Stored table: `benchmarks/results/inference_comparison.md`, generated
2026-03-21 at commit `e80176ac` on `t81devs-MacBook-Air.local` (8 CPUs, the
table's runner line says 24 MHz, build `-O3 -march=native`). That table's
NEON columns are AArch64 NEON. This binary registered no `NEON` benchmarks.

Throughput cells below use the Google Benchmark mean aggregate
(`items_per_second`), which is the field the inference workflow's markdown
generator reads. Mac numbers are copied from the published table.

Dot product, N = 16,384:

| Path | This VM | Published Mac table |
|---|---:|---:|
| FP32 | 1.97 Gops/s | 1.15 Gops/s |
| FP16-sim | 1.07 Gops/s | 822.68 Mops/s |
| Int8 | 10.46 Gops/s | 38.79 Gops/s |
| T81 scalar | 1.43 Gops/s | 1.67 Gops/s |
| T81 NEON | not registered | 4.34 Gops/s |
| T81 scalar / FP32 | 0.73× | 1.45× (1.67 / 1.15) |

Matrix multiply, N = 512:

| Path | This VM | Published Mac table |
|---|---:|---:|
| FP32 | 16.54 Gops/s | 13.92 Gops/s |
| FP16-sim | 4.43 Gops/s | 830.69 Mops/s |
| Int8 | 13.08 Gops/s | 18.83 Gops/s |
| T81 scalar | 3.54 Gops/s | 1.96 Gops/s |
| T81 scalar / FP32 | 0.21× | 0.14× (1.96 / 13.92) |

Scaled dot-product attention, sequence length 128:

| Path | This VM | Published Mac table |
|---|---:|---:|
| FP32 | 2.59 Gops/s | 2.84 Gops/s |
| T81 scalar | 1.27 Gops/s | 709.69 Mops/s |
| T81 / FP32 | 0.49× | 0.25× |

Transformer layer, sequence length 128, mean `items_per_second`:

| Path | This VM | Published Mac table |
|---|---:|---:|
| FP32 | 16,419 ops/s | 11,531 ops/s |
| T81 ternary | 2,245 ops/s | 2,915 ops/s |
| Hybrid MLP | 4,291 ops/s | 4,725 ops/s |
| Ternary / FP32 | 0.14× | 0.25× |

The within-host ordering on this guest is FP32 ahead of scalar T81 for these
four shapes. The published Mac scalar-T81 dot product is ahead of that Mac's
FP32 number, and the Mac NEON column is ahead of both. Those orderings were
measured on different machines. This run does not treat either ordering as a
regression of `main`.

Weight-load byte counters are properties of the benchmark, not of the clock.
They match the published table for the families this binary registered:

| Weights | FP32 bytes | FP16 bytes | Ternary packed bytes |
|---:|---:|---:|---:|
| 1,024 | 4,096 | 2,048 | 256 |
| 4,096 | 16,384 | 8,192 | 1,024 |
| 16,384 | 65,536 | 32,768 | 4,096 |
| 65,536 | 262,144 | 131,072 | 16,384 |

`memory_ratio_vs_fp32` on the ternary rows is `16.0` (4,096 / 256). The
published table's ternary/FP32 column is `0.06×`, which is that same ratio
inverted. This binary has no `BM_WeightLoad_Int8` row. The published Int8 byte
column is therefore not re-measured here. Int8 dot-product and matmul timing
rows were measured and are in the raw JSON.

### Other stored benchmark text

`benchmarks/results/archive/benchmark_results_vm.txt` and
`docs/developer-guide/tools/benchmarks_engineering.md` contain older
`BM_VMSimulation_Dispatch` iteration text (about 7.94 µs and 9.89 µs) without a
paired native-loop time, compiler, or host record that could be matched to this
guest. They were not used as a regression baseline. The live comparison for
that family is the ratio gate above.

`docs/records/status-history/GOVERNED_WORKFLOW_BENCHMARK_EVIDENCE_2026-03-19.md`
records a different filter (`^BM_Governed`). That family was not run.

## Smoke mean aggregates on this guest

Mean aggregates from `smoke.json`, for the names outside the `BM_Cell*`
table. Real time is the mean aggregate. These rows have no machine-matched
stored baseline in the repository.

| Benchmark mean | Real time | `items_per_second` |
|---|---:|---:|
| `BM_ArithThroughput_T81Cell` | 597,072 ns | 67,061,395 |
| `BM_ArithThroughput_Int64` | 25,569 ns | 1,564,538,485 |
| `BM_NegationSpeed_T81Cell` | 17,169 ns | 5,837,639,363 |
| `BM_NegationSpeed_PackedCell` | 1,948 ns | 51,343,575,129 |
| `BM_NegationSpeed_Int64` | 23,713 ns | 4,217,309,509 |
| `BM_NegationSpeed_T81Native` | 88,665 ns | 1,127,868,086 |
| `BM_RoundtripAccuracy_T81Cell` | 226,363 ns | 44,190,104 |
| `BM_RoundtripAccuracy_Int64` | 1,339 ns | 7,486,554,142 |
| `BM_MemoryBandwidth_ReadWrite_T81/16777216` | 4,470,649 ns | — |
| `BM_MemoryBandwidth_ReadWrite_Binary/16777216` | 4,628,170 ns | — |
| `BM_Add_1024_bit/binary_carry_propagate` | 6.149 ns | 162,646,978 |
| `BM_Add_1024_bit/ternary_koggestone` | 2,700 ns | 370,401 |
| `BM_Add_2048_bit/binary_carry_propagate` | 16.388 ns | 61,035,604 |
| `BM_Add_2048_bit/ternary_koggestone` | 5,176 ns | 193,191 |
| `BM_T81LangCompile_T81` | 68,395,032 ns | — |
| `BM_T81LangCompile_Binary` | 983,211 ns | — |
| `BM_LimbArithThroughput_T81Limb` | 5,308,517 ns | 18,840,601 |
| `BM_LimbArithThroughput_Int128` | 136,735 ns | 731,371,980 |
| `BM_LimbAdd_T81Native` | 120,574,322 ns | 829,446 |
| `BM_LimbAdd_T81Limb` | 5,379,272 ns | 18,593,861 |
| `BM_LimbAdd_Int128` | 138,517 ns | 721,949,508 |
| `BM_vs_T81Native` | 1,264 ns | 791,056 |
| `BM_vs_T81` | 54.551 ns | 18,333,971 |
| `BM_vs_Binary` | 3.294 ns | 303,663,143 |

`BM_T81LangCompile_T81` completed a single iteration in this smoke window.
That sample is too short to support a compile-time claim even on this guest.

## Not run

- `T81_BENCHMARK_PROFILE=full` and `T81_BENCHMARK_PROFILE=deep`. The documented
  default and the CI jobs are the smoke, VM, and SIMD slices. Full and deep
  add lexer, Base81, overhead, DPE, and governed families and were left for a
  separate run.
- `^BM_DPE_` and `^BM_Governed`. DPE is compiled in (`T81_ENABLE_DPE=ON`) but
  neither family is part of the smoke, VM, SIMD, or inference commands above.
- A second `BM_Cell*` invocation with the BASE-PERF-00 warmup and `0.05s`
  minimum. The cells above are the smoke-profile rows, so the comparison keeps
  the protocol difference visible.

## Raw artifacts

`.gitignore` ignores `benchmarks/results/*` except markdown, and it ignores
`bench-*.json` in every directory. This run is a cloud VM record, not a
release baseline, so the JSON was not force-added under `benchmarks/results/`.
The process output bytes were copied unchanged into
`docs/records/status-history/2026-11-23-benchmark-suite/` under names the
ignore rules allow. SHA-256 of each JSON is the hash of the runner output.

Every invocation printed the same three-line console log, saved once as
`runner-console.txt`: report generation started, the console table stayed
suppressed, and `T81_BENCHMARK_WRITE_REPORTS` was not `1`.

Directory: `docs/records/status-history/2026-11-23-benchmark-suite/`

Checksums: `docs/records/status-history/2026-11-23-benchmark-suite/SHA256SUMS`

| Stored file | Runner output name | SHA-256 |
|---|---|---|
| `smoke.json` | `bench-smoke.json` | `2e85a860e312fdcfa112cf0e6af49531d42907a5fdd5a3e3c678c17a69982df4` |
| `vm-workload.json` | `bench-vm-workload.json` | `b61ed992b0b3ae02aef5482da72d3aebfb4436bb75e40f66896d20fa3cc3da11` |
| `simd-tand.json` | `bench-simd.json` | `377837f4fe58cac8361321126c1034e78e078d4ce0211a63abda25df1344cde7` |
| `inference-dot-product.json` | `bench-DotProduct.json` | `aa88d6679496fce42c71259113019b9f6053663d3f5a54eedf4b2247835815e8` |
| `inference-matmul.json` | `bench-MatMul.json` | `8d47732886e7fdee2b26d4d4b2b123c3fde9190156a1f8c95a8dfa1a48f9f07d` |
| `inference-attention.json` | `bench-Attention.json` | `a051a8b66d72a85b972c6bc3cfaa8a376530bc92058105a00700c3282efd5a9b` |
| `inference-weight-load.json` | `bench-WeightLoad.json` | `a543623c5b507826d3be2f1897d8cb752c2a33aef47e055af53b4a274675f6ee` |
| `inference-transformer.json` | `bench-Transformer.json` | `5c5868378e5df7676c80647198e6b4459ac4b141429fe65b4bd86b7c5220f031` |

## Disposition

- VM workload ratio gate: pass (`1.142×` and `1.166×`, limit `3.0×`).
- SIMD `TAnd` ratio gate: pass (`0.9633×` and `0.9086×`, limit `1.15×`) at the two sizes the checker evaluates.
- Absolute throughput versus BASE-PERF-00 and the 2026-03-21 inference table: recorded, and not a regression or improvement finding. The hosts, compilers, and ISAs differ, and this guest is a cloud VM.
- Weight-load byte counters for FP32, FP16-sim, and ternary packed: match the published table.
- Benchmark code and thresholds: unchanged.
