# Executed scoped CI results for PR #785

All three targeted jobs completed successfully in
[Actions run 43538262449](https://github.com/t81dev/t81-foundation/actions/runs/43538262449),
at CI harness commit `b24cd1b1595f0e9c2c55a94a3cc0df89b57e83c1` on draft #786.
Each separately checked out and verified exact candidate source
`300cdea8896d56c025f7099c0cdf0c41d7bd6237`. This closes the targeted execution gap,
not general integration readiness or scientific acceptance. The original local
record's pending-platform statements describe its earlier publication state.

| Executed platform/compiler | Build | CTests |
|---|---|---|
| Ubuntu 24.04 x86_64, GCC 14.2.0 | target and declared dependencies passed | 5/5 passed |
| Windows Server 2025 x64, MSVC 19.51.36260 | target and declared dependencies passed | 5/5 passed |
| Windows Server 2025 x64, clang-cl 20.1.8 | target and declared dependencies passed | 5/5 passed |

Each lane uses Release/NDEBUG, experiments enabled, and benchmarks OFF. It builds
only t81_exp_scg08_01_test, then executes exactly the five ^t81_exp_scg08_01 tests.
All twelve child cases require exit exactly 1 and the expected stderr diagnostic.
Each lane also passes all 94 schema negatives and Draft 7 parity (61 rejection
cases, 2 positive records), plus the rejection driver's self-tests. Each parent
smoke run records 179 trials; these are correctness regressions on three platforms,
not new historical measurement records or a performance campaign. Gate 2 is not
invoked. Source remained clean before/after configuration and execution.

## Retained evidence

`remote/` contains all three downloaded Actions artifacts, unchanged. Every
per-packet sha256.json was checked against the downloaded bytes. These packets
retain exact argv, working directories, exit statuses, source/harness SHA,
compiler versions, dependency/package versions, source hashes, compile command
(including NDEBUG), build/configuration logs, five-name discovery, successful
JUnit, verbose CTest and LastTest.log, and CMake cache. All lanes record Asio
`12e0ce9e0500bf0f247dbd1ae894272656456079`.

`remote-run-status.json` retains completed jobs and their URLs.
`remote-job-logs.txt` retains actual setup/compile/test/artifact-upload logs.
`remote-results.json` indexes source identities and results. `retained-sha256.json`
hashes the complete record. The local .gitattributes disables newline conversion
for the retained packet on future cross-platform checkouts. `wrong-source-rejection/` retains a local regression
where the runner exits 1 before configure on a wrong source SHA. Nothing in this
evidence branch changes the CI harness, candidate, tests or arithmetic.

This packet is committed on a separate audit branch, so retaining results does
not advance #785 or rerun/alter the reviewed CI harness. Actions artifacts expire
after 90 days; this copied packet is retained independently of that expiry.

## Limits

These are executed hosted-CI correctness results, with a Codex-authored harness
and prior shared review context. They are not independent scientific review or
authentication of original #778/#779 execution/provenance. Hashes establish content
identity. They do not establish the truth of provenance or broad arithmetic
correctness beyond the exercised inputs and call paths.

This builds only the requested target and its dependencies, not the complete
repository Windows/Linux build/test matrix. No ARM64 or additional Windows/compiler
versions were exercised by this workflow. Compiler banner exit status alone is
not capability evidence; successful actual compilation/linking and test execution
provide the bounded evidence here. No new sanitizer pass is claimed for these
Release CI lanes; prior local sanitizer limits remain.

The existing global workflow policy failures (seven tagged action references,
one missing permissions declaration) remain documented and unmodified. No required
quality gate or integration acceptance is inferred from this new scoped workflow.
Validators, event/accounting definitions, fixtures, production arithmetic,
historical evidence and all prior correctness/model/provenance limitations are
preserved. RFC stays draft; Gate 1 acceptance WITHHELD; Gate 2 STRICT NO-GO.
No merge, release, or gate advancement.
