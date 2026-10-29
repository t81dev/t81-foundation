# Final implementation validation — October 29, 2026

**Disposition: BLOCKED — PMU_UNAVAILABLE.** Portable implementation checks pass on the two recorded Clang toolchains. This is not full GCC/Linux validation or a performance result. No ternary advantage is claimed.

PR: https://github.com/t81dev/t81-foundation/pull/637

Branch: `research/exp-t81-comp3-01-implementation-2641169195584772356`

Verified starting HEAD: `4c19e6276ec13999db76df21664de550ddb1acde`. Both corrective commits (`96647127`, `4c19e62`) were inspected as unverified implementation changes. The frozen protocol was not changed. Work is limited to this experiment; runtime/CanonFS contracts and governance remain outside this change.

## Baseline and review findings

Read all three original Codex P1 findings:

- [Dispatch / wrapper asymmetry](https://github.com/t81dev/t81-foundation/pull/637#discussion_r4408546838).
- [Fixed regression allowances; 70/100 instructions and 11/10 misses](https://github.com/t81dev/t81-foundation/pull/637#discussion_r4408546841).
- [Missing / unmatched observations](https://github.com/t81dev/t81-foundation/pull/637#discussion_r4408546846).

Initial experiment checks at the starting commit:

```sh
python3 experiments/cmp3_decision_advantage/verify_pcg64.py
clang++ -std=c++20 -O3 -fomit-frame-pointer -Wall -Wextra experiments/cmp3_decision_advantage/test_classification.cpp -o /private/tmp/t81-comp3-validation/baseline-classification
/private/tmp/t81-comp3-validation/baseline-classification
clang++ -std=c++20 -O3 -fomit-frame-pointer -Wall -Wextra -Wconversion -Wshadow -Werror experiments/cmp3_decision_advantage/test_classification.cpp -o /private/tmp/t81-comp3-validation/baseline-classification
clang++ -std=c++20 -O3 -fomit-frame-pointer -Wall -Wextra -Wconversion -Wshadow -Werror experiments/cmp3_decision_advantage/benchmark.cpp -o /private/tmp/t81-comp3-validation/baseline-benchmark
```

PCG reference: 4/4 vectors pass. Original classification executable: all 8 assertions pass with ordinary warnings. Strict classification compile: FAIL, three conversion warnings promoted to errors (resample index signedness and two size-to-double conversions). Benchmark compile: FAIL, `linux/perf_event.h` absent. No baseline executable/PMU run was possible. GCC: unavailable; `/usr/bin/g++ --version` identifies Apple Clang, not GCC. These are experiment-scoped checks; unrelated repository suites were not run.

Source review additionally found: baseline dispatch forms had all been converted to common table lookup; candidate had an extra comparator call; policy wrappers were asymmetric; translation units were not separated; A/C corpus used a narrower integer interval; Pattern B consumed an unused random offset; bootstrap used MT19937 and two-sided .005/.995 percentiles; p-values were placeholders; equal zero-variance differences yielded NaN. The new regressions cover these defects through payload/corpus checks, wrapper assembly checks and statistical/CSV tests.

## Host and compiler identity

- Host: Darwin `25.5.0`, ARM64 `T8112`; macOS `26.5.2` (`25F84`).
- Python: `3.14.3`; standard library only.
- Homebrew Clang: `Homebrew clang version 22.1.1`, target `arm64-apple-darwin25.5.0`; installed `/opt/homebrew/Cellar/llvm/22.1.1/bin`.
- Apple Clang: `Apple clang version 21.0.0 (clang-2100.1.1.101)`, target `arm64-apple-darwin25.5.0`; installed `/Library/Developer/CommandLineTools/usr/bin`.
- GNU GCC: absent from the inspected system/Homebrew tool locations. No Linux container runtime (`docker`, `podman`, `limactl`, `colima`) was available.

## Exact reproduction commands and flags

```sh
python3 experiments/cmp3_decision_advantage/validate.py --compiler clang++ --out /private/tmp/t81-comp3-validation/clang22
python3 experiments/cmp3_decision_advantage/validate.py --compiler /usr/bin/clang++ --out /private/tmp/t81-comp3-validation/apple21
python3 experiments/cmp3_decision_advantage/inspect_assembly.py /private/tmp/t81-comp3-validation/clang22/optimized
python3 experiments/cmp3_decision_advantage/inspect_assembly.py /private/tmp/t81-comp3-validation/apple21/optimized
```

Every expanded command and output is preserved in [clang22.log](clang22.log) and [apple21.log](apple21.log). Optimized flags:

```text
-std=c++20 -O3 -fomit-frame-pointer -fno-lto -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror
```

Each of the nine C++ translation units is compiled separately; three executables are linked. Assembly adds `-S -fverbose-asm`. Sanitizer builds append:

```text
-O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined -fno-sanitize-recover=all
```

Assertions remain enabled. The final source was checked with `git diff --check`.

## Executed checks

All figures below are **per build configuration**, each executed on Clang 22 optimized, Clang 22 sanitized, Apple Clang 21 optimized, and Apple Clang 21 sanitized.

| Check | Pass | Fail |
| --- | ---: | ---: |
| Strict C++ TU compilation | 9 | 0 |
| Executable linking | 3 | 0 |
| C++ PCG64 frozen vectors | 4 | 0 |
| Mandatory pair / payload checks (20 x 3 patterns x 5 modes) | 300 | 0 |
| Additional policy checks (8 x 3 validation passes x 5 modes) | 120 | 0 |
| Generated-corpus payload differential checks | 150,000 | 0 |
| Repeated-generation pair identity checks | 30,000 | 0 |
| Independent Python PCG64 vectors | 4 | 0 |
| Independent Python/C++ corpus pair comparisons | 30,000 | 0 |
| CSV integration scenarios | 8 | 0 |

Classification executable passes all original scenarios and expanded regressions, including 42 malformed-series mutations, three validity-flag cases, three invalid direct-helper cases, four t-distribution landmarks (p=1, .05, .01, .001), symmetry, zero variance, the inclusive 15% boundary, noisy and constant regressions, and the original 70/100, 11/10 counterexample. CSV scenarios cover complete synthetic support, missing/empty/duplicate data, nonfinite data, payload mismatch, miss regression and fastest-baseline latency regression. Synthetic data are never represented as measurements.

ASan/UBSan: both complete sanitized suites pass, no diagnostics. Smoke preflight: all four configurations exit 6 with `BLOCKED — PMU_UNAVAILABLE`, as expected on this host. No PMU iterations or timing replacements were collected. The validation driver treats this expected preflight block separately from test failures.

## Assembly findings

All 18 optimized assembly files were generated (nine per compiler). Core dispatch/payload assembly is retained in [clang22/](clang22/) and [apple21/](apple21/); inspection output is in the corresponding `*-assembly.txt` files. All wrapper sets contain two external no-inline entry points and three external payload handlers; candidate/Baseline 2 perform table dispatch, and Baselines 1/3/4 retain their direct target forms.

Both compilers produced these static instruction totals over each complete wrapper, including all mutually exclusive basic blocks:

| Implementation | A/C wrapper | B wrapper |
| --- | ---: | ---: |
| Candidate | 8 | 9 |
| Baseline 1 | 7 | 7 |
| Baseline 2 | 8 | 10 |
| Baseline 3 | 7 | 7 |
| Baseline 4 | 9 | 10 |

Each payload handler has two instructions. Totals are **not dynamic retired instructions**, and do not estimate PMU values. The checked-in assembly provides each individual basic block and branch for inspection.

A/C candidate and Baseline 2 have identical assembly instruction sequences on both toolchains. Baselines 1 and 3 likewise lower equivalently. Policy candidate and Baseline 2 differ in conditional-select versus subtract lowering. Baseline 4's source `switch` lowers to conditional branches; no artificial jump-table flags are imposed. Call boundaries are matched; the remaining instruction/control-flow differences reflect source mechanisms and compiler lowering. The earlier runtime-pattern-flag confounder was removed using separate A/C and policy entry points. CMP3 is inlined inside the candidate wrapper, so it has no extra call or standalone assembly symbol. These findings establish compiler equivalence/differences for these builds, not ternary advantage or disadvantage.

## Corrections and changed-file inventory

All changed files are within `experiments/cmp3_decision_advantage/`:

- `benchmark.cpp`: portable preflight, complete parity validation, fixed-budget collector, grouped checked counters, permitted-core affinity, frequency-control handling, matched run order and fail-closed errors.
- `candidate.cpp`, `baseline1.cpp`, `baseline2.cpp`, `baseline3.cpp`, `baseline4.cpp`: separate implementations with matched no-inline workload entry points.
- `dispatch.hpp`, `payload.cpp`: shared declarations and observable targets.
- `pcg64.hpp`, `corpus.hpp`: portable PRNG and corrected corpus generation.
- `statistics.hpp`, `analyze.cpp`: validated statistical calculations and complete CSV reporting/classification.
- `test_classification.cpp`, `test_analysis.py`, `test_corpus.py`: classification, malformed-data, corpus and pipeline regressions.
- `validate.py`, `inspect_assembly.py`, `README.md`: reproducible verification and usage.
- `evidence-2026-10-29/VALIDATION.md`, two full validation logs, two assembly summaries and twelve core assembly artifacts: scoped evidence.

`verify_pcg64.py` is unchanged. The pre-existing untracked `build-bench/` directory is untouched.

## Unresolved issues / stopping condition

1. **PMU_UNAVAILABLE:** Linux `perf_event_open` is unavailable on macOS. Full performance measurement cannot proceed here. No protocol result or 150-row empirical dataset exists.
2. **GCC validation unavailable:** GCC compilation, warnings, sanitizers and lowering must be checked on a host with actual GNU GCC. Apple Clang is not a substitute.
3. **Linux-only source remains uncompiled/unexecuted here:** grouped PMU reads, scheduling checks, sysfs governor/boost mutation/restoration, permitted-core affinity, warmup, matched measurement order and ten-minute budget paths require Linux validation before measurement. In particular, PMU scheduling/control behavior must be observed, not inferred from the portable suite.
4. Windows and Linux/x86 execution have not been tested. Core PCG uses standard unsigned arithmetic without `__int128`, but this source property is not cross-platform test evidence.
5. Bootstrap percentile convention and numerical integration are disclosed; n=10 retains the protocol's limited statistical precision. Passing synthetic checks is not independent statistical replication.

These reproducible environment blockers prevent the required cross-compiler/PMU validation. Stop here; resume on an appropriate Linux host with GCC, Clang and hardware-counter access. **Final disposition remains BLOCKED.**
