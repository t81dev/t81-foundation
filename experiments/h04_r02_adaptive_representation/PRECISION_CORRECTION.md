# PR #572 — shared-overhead precision correction

Starting head: `13f8377f571d319707d58d388693813fb08b3b07`.
Review: https://github.com/t81dev/t81-foundation/pull/572#discussion_r4373065276
Executed 2026-10-25 America/New_York, macOS ARM64 / Apple clang 21 / Python 3.14.3.

## Reproduction and correction

The original executable with `--trace 0 --metadata-bits 1e20` emitted
`total_break_even_c3_c2 = 4.1359030627651384e-25`. Corrected output is `1.333333333333333`,
within floating-point rounding of the exact 4/3 threshold. Before/after JSON is
preserved under `evidence/precision-correction/`.

For B blocks and N values, H=B*m+N*kappa is identical for every feasible
adaptive schedule in both arms. Comparing rounded full totals can discard
payload differences. The same problem can corrupt schedule choice inside DP.
C++ now minimizes payload plus transitions with H omitted, and cancels H
analytically before crossover comparisons. Reported ledger totals still include
metadata and conversion under the unchanged cost model. Parameters are validated
before cancellation. Full double totals can still round together at huge H;
they are presentation values, not the comparison inputs.

The independent Python suffix optimizer retains H but evaluates full costs using
exact Fraction arithmetic. Its new crossover check uses exact rational full-total
comparisons. Fractions preserve the exact input float values, independently of
C++ cancellation. Exhaustive schedule enumeration is a third exact check.

## Regression outcomes

The new regression tests 7 trace/block/tier/transition/ratio cases across 7
shared-overhead profiles: **49 configurations**. It includes the reported
one-value reproducer, conversion-only overhead, partial blocks, 8/10-bit tiers,
schedule-changing traces, overhead from 1e-300 through 1e300, transition scales
1e-200 and 1e20, and payload ratios 1e-200 and 1e200. It checks identical
crossovers and C++ schedules across common-cost profiles, exact rational
crossover/optimal-cost agreement, and independently reconstructed emitted events.
Existing 128 exhaustive trace/cost cases and all packing checks remain enabled.

- Clean H04 CTest: **3/3 passed**, zero failures, 3.00 seconds.
- Standalone Python audit: **9/9 passed**, zero failures, 2.425 seconds.
- Fresh ASan+UBSan H04 CTest: **3/3 passed**, zero failures, 23.95 seconds;
  no sanitizer finding.
- Standalone oracle: executed successfully; default output is byte-identical
  to the prior oracle artifact.
- clang-format and git diff whitespace checks: passed.

The first Python run had **8 passes and 1 failure**: the artifact parity test
caught a changed final-bit crossover in `alternating`. This was investigated,
not bypassed. Its crossover changed from 1.5319148936170208 to
1.5319148936170213 (3600/2350 rounded to double). The executable regenerated
results.json; only that field changed. Other ledgers, packing data, rounded
report tables and scientific conclusions are unchanged. The initial failing
output and subsequent passing logs are preserved as gzip files.

## Exact commands

From the isolated integration worktree:

```sh
/private/tmp/t81-h04-r02-clean/experiments/h04_r02_adaptive_representation/t81_h04_r02_adaptive --trace 0 --metadata-bits 1e20 --json-out /private/tmp/h04-precision-before.json
cmake -S . -B /private/tmp/t81-h04-r02-precision-clean -G Ninja -DT81_BUILD_TESTS=ON -DT81_BUILD_TUI=OFF -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_BENCHMARKS=OFF -DT81_BUILD_FUZZ_TESTS=OFF -DT81_ENABLE_TERNARYOS=OFF -DFETCHCONTENT_SOURCE_DIR_ASIO=/private/tmp/t81-accountable-build/_deps/asio-src
cmake --build /private/tmp/t81-h04-r02-precision-clean --target t81_h04_r02_adaptive
/private/tmp/t81-h04-r02-precision-clean/experiments/h04_r02_adaptive_representation/t81_h04_r02_adaptive --json-out /private/tmp/h04-precision-default-after.json
ctest --test-dir /private/tmp/t81-h04-r02-precision-clean -R h04_r02 --output-on-failure --no-tests=error
python3 tests/ci/h04_r02_audit_regression_test.py --bin-path /private/tmp/t81-h04-r02-precision-clean/experiments/h04_r02_adaptive_representation/t81_h04_r02_adaptive -v
cmake -S . -B /private/tmp/t81-h04-r02-precision-sanitized -G Ninja -DT81_BUILD_TESTS=ON -DT81_BUILD_TUI=OFF -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_BENCHMARKS=OFF -DT81_BUILD_FUZZ_TESTS=OFF -DT81_ENABLE_TERNARYOS=OFF -DT81_ENABLE_ASAN=ON -DT81_ENABLE_UBSAN=ON -DFETCHCONTENT_SOURCE_DIR_ASIO=/private/tmp/t81-accountable-build/_deps/asio-src
cmake --build /private/tmp/t81-h04-r02-precision-sanitized --target t81_h04_r02_adaptive
ctest --test-dir /private/tmp/t81-h04-r02-precision-sanitized -R h04_r02 --output-on-failure --no-tests=error
/private/tmp/t81-h04-r02-precision-clean/experiments/h04_r02_adaptive_representation/t81_h04_r02_adaptive --trace 0 --metadata-bits 1e20 --json-out /private/tmp/h04-precision-after.json
python3 experiments/h04_r02_adaptive_representation/oracle.py
clang-format --dry-run --Werror experiments/h04_r02_adaptive_representation/adaptive_experiment.hpp
git diff --check
```

The original reproducer used the retained binary built at head 13f8377f.
The initial audit failure occurred before copying the newly generated default
JSON into results.json. The final build/CTest used the corrected implementation
and regenerated artifact. The local dependency is unchanged asio-1-30-2.

## GitHub status checked before correction push

PR #572 is open, with one unresolved COMMENTED review finding at 13f8377f.
The main CI run 40248283194 has successful lint/spec/docs and Linux benchmark
jobs, with other jobs queued or in progress. Its governance-metrics job fails:
`README.md: claims determinism but omits 'Determinism Surface Registry' boundary language or link.`
Job: https://github.com/t81dev/t81-foundation/actions/runs/40248283194/job/121334428159
This change does not modify README, governance checks or runtime systems.
Fresh review and CI must assess the corrective head; old-head status is not
reported as evidence that the correction is green remotely. No merge or closure
of #572, #570 or #571 is performed.
