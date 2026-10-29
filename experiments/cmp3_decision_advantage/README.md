# EXP-T81-COMP3-01 implementation

Frozen authority: [TERNARY_ADVANTAGE_CANDIDATES.md](../../docs/research/TERNARY_ADVANTAGE_CANDIDATES.md), Protocol Sections 1–12. Software comparison/dispatch on binary hosts only.

Current validation disposition: **BLOCKED — PMU_UNAVAILABLE** on macOS ARM64. GCC and Linux collection controls remain unvalidated; see [validation evidence](evidence-2026-10-29/VALIDATION.md). No performance conclusion is available.

Reproduce the portable validation (Python standard library; C++20):

```sh
python3 experiments/cmp3_decision_advantage/validate.py --compiler clang++ --out /tmp/comp3-clang
python3 experiments/cmp3_decision_advantage/validate.py --compiler g++ --out /tmp/comp3-gcc
python3 experiments/cmp3_decision_advantage/inspect_assembly.py /tmp/comp3-clang/optimized
```

Verify that `g++ --version` actually identifies GCC. On macOS `/usr/bin/g++` identifies Apple Clang. The validation script prints every compiler, link, assembly and test command. It builds separate translation units with no LTO, then repeats correctness/analysis checks with ASan and UBSan. Assembly inspection supports GCC/Clang assembly with CFI function boundaries and ARM64 or x86 indirect branches; manual review is still required for each target toolchain.

The candidate and each baseline have separate no-inline A/C and policy entry points. There is one wrapper call per decision and a shared set of three external payload targets. The CMP3 helper is internal and inlined into its dispatch wrapper, avoiding a candidate-only call. Policy entry points evaluate the lower and upper thresholds rather than comparing against zero. Compilers may eliminate redundant comparisons or merge lowering; preserve those observations.

Corpus sampling uses the Section 6 seed/stream with portable two-limb PCG64 arithmetic, rejection sampling for bounded modulo, and the top 53 random bits for a uniform representable double in `[0,1)`. A/C sampling covers the full prescribed safe integer interval. Independent Python arbitrary-precision generation checks every pair of the 30,000-pair validation corpus. The 20 mandatory pairs are checked first for each pattern, including payload parity for all five implementations. Additional policy cases cover the thresholds and integer extremes.

On a suitable Linux host, after reviewing assembly and validating the Linux controls:

```sh
/tmp/comp3-gcc/optimized/benchmark --smoke 2>smoke.log
/tmp/comp3-gcc/optimized/benchmark >measurements.csv 2>environment-and-order.log
/tmp/comp3-gcc/optimized/analyze measurements.csv >analysis.txt
```

The full run fixes N=10,000,000 and ten repetitions; arbitrary smaller budgets cannot produce an experiment result. Smoke uses 10,000 pairs and one repetition, writes no measurement CSV, and cannot be analyzed as a complete experiment. PMU errors return BLOCKED; elapsed time never replaces counters. Linux collection uses a pinned hardware counter group, rejects unscheduled/multiplexed counters, checks ioctl/read results, pins to an allowed core, attempts performance/no-boost settings, restores changed controls, and performs ten million-iteration warmup cycles per implementation. It records the core and actual run order on stderr, rotating implementation order within matched repetition blocks. The retained ten-minute collection budget blocks on overrun. These Linux paths have not been executed in the recorded macOS validation.

Analyze only a successful full run; retain its compiler/assembly artifacts, complete stderr, exit status, and CSV together. The analyzer cannot authenticate CSV provenance or infer environment/corpus validity from counters. Missing/duplicate rows or mismatched checksums block before calculations. All three patterns and all four baselines require ten finite matched observations per metric. Reports include means, sample standard deviations, 95% Student-t intervals, raw paired differences, primary paired p-values, one-sided 99% bootstrap bounds, paired 99% regression intervals, fastest baseline per pattern, trigger reasons and the prescribed overall precedence.

Bootstrap uses 10,000 paired resamples from the Section 6 PCG64 seed/stream, reset per pattern, with inverse empirical-CDF quantiles at ranks 100 and 9900 (indices 99 and 9899). These are **one-sided** 99% bounds. The df=9 two-sided p-value uses the transformed Student-t tail integral of `sin(theta)^8`, normalized by `35*pi/256`, with 4096-panel Simpson integration; regression intervals use `t(9,.995)=3.2498355415921254`. Equal constant paired differences have p=1; nonzero constant differences have the limiting p=0 and point intervals. No fixed regression allowances or substitute statistical thresholds are used.
