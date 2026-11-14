# Isolated #779/#785 candidate against refreshed main

Local branch: `candidate/pr779-pr785-main-integration`.
Main refreshed to `fbfe4f5a2d4439f5ff479014301560764de39c99`; this is unchanged
from the preceding pinned comparison. No existing PR branch was advanced.

## Composition and conflicts

Applied the two accepted #779 commits, followed by the two #785 commits:

- Original #779 `5bfc0e2b7f531c4cdeca53ed9e1172f551683da6` and
  `4d5f3586c04a69c520e613166291c0974877628d`.
- Original #785 `ceb1560af3a386050c1a611270aaf03a72b405a1` and
  `300cdea8896d56c025f7099c0cdf0c41d7bd6237`.

All four cherry-picks applied cleanly, with no conflicts or hand resolutions.
The candidate intentionally does not merge either original PR. It also retains
#785's original audit preservation, including the pinned #778 packet and failed
configuration/sanitizer records, without importing #778's alternative counters.

Copied all 96 files in `docs/records/audits/2026-11-13-pr785-scoped-ci/` directly
from `8733abcd2ffc7ecb35ff8b61d9306472fced806b`, byte-for-byte. Its original
retained-sha256.json and each platform packet manifest verify unchanged.
That is historical evidence for tested source `300cdea...`, not this candidate.

## Accepted file identity

`accepted-file-comparison.json` records SHA-256 and pinned comparison revision:

- exact_oracle_adapter.hpp and tier2_work_harness.hpp exactly match accepted #779
  `4d5f3586c04a69c520e613166291c0974877628d`.
- tier1_markov_harness.hpp, GATE1_IMPLEMENTATION_REPORT.md and the RFC-00E1 contract
  also match that #779 head exactly.
- experiment CMakeLists.txt, test_exp_scg08_01.cpp and both rejection Python test
  files match accepted #785 `300cdea8896d56c025f7099c0cdf0c41d7bd6237` exactly.
  This retains #779's validator fixtures and accounting expectations with only
  the accepted portable subprocess mechanism layered over them.

The stored-tryte rejection guards, OutLimbs + 2 repack accounting, exact negative
product bounds, weighted-event definitions and Release-active checks are retained.
Production include/src, top-level CMake/cmake configuration, and active CI paths
match refreshed main. Historical gate1_evidence_record.json remains Git blob
`00731bd34cfad475acde588085827698627ec60d` in main, #779, #785 and the candidate.

## Inactive original CI, reproducible evidence

None of these #786 paths is installed in the candidate:

- `.github/workflows/scg08-correctness.yml`
- `scripts/ci/run_scg08_correctness.py`
- `scripts/ci/scg08-requirements.txt`

`archived-ci/` retains exact copies from original CI harness revision
`b24cd1b1595f0e9c2c55a94a3cc0df89b57e83c1`, outside all active CI directories.
`source-revisions.json` records original paths, blob IDs, SHA-256 and immutable
GitHub source links. Their pinned source guard remains unchanged. There is no
attempt to relabel old CI success as validation of this new main-based SHA.

For an explicitly authorized reproduction of the recorded old run, check out
source `300cdea8896d56c025f7099c0cdf0c41d7bd6237` separately, install the archived
requirements with Python 3.12, activate the original compiler environment, and
run the archived runner with --source OLD_SOURCE --build NEW_BUILD --evidence
NEW_OUTPUT. SCG08_CC/SCG08_CXX select the recorded compiler. The runner correctly
rejects this new candidate's SHA, because it reproduces the original pinned run.
The archived workflow documents the exact checkout and platform setup. These
files are audit material, not newly enabled workflows or Gate 2 authorization.

## Fresh local validation

The newly assembled main-based code/tree was tested at candidate commit
`4018646b3c229083780afb82d1f7e815af921d7c`. The final candidate adds only this
integration record and local validation outputs. No executable file changed after
that test. `local-validation/summary.json` records exact commands, environment,
compile command with NDEBUG, tested SHA and exit statuses. Raw configure/build,
CTest discovery, verbose CTest, JUnit, LastTest, dependency/package versions,
compile_commands and CMake cache are retained with hashes.

macOS ARM64 / Apple clang 21 / Python 3.14.3: Release configure and target build
passed; exactly five ^t81_exp_scg08_01 CTests passed. These include all twelve
subprocess rejections, driver self-tests, 94 schema negatives and Draft 7 parity
(61 rejected / 2 positive records). Existing clean local Asio cache was used;
source identity is the same pinned dependency retained by prior records.
No fresh sanitizer, Windows/Linux run, benchmark campaign or Gate 2 execution
was performed for this new candidate. Prior passing Linux GCC / Windows MSVC /
clang-cl results remain pinned to their actual source 300cdea... and harness b24cd1...,
and prior sanitizer failures/limitations remain preserved rather than reclassified.

## Scope and limits

Changed executable/build/contract paths against main are exactly:

- `experiments/scg08_arithmetic_work/CMakeLists.txt`
- `experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md`
- `experiments/scg08_arithmetic_work/exact_oracle_adapter.hpp`
- `experiments/scg08_arithmetic_work/test_exp_scg08_01.cpp`
- `experiments/scg08_arithmetic_work/test_result_rejection.py`
- `experiments/scg08_arithmetic_work/test_result_rejection_driver.py`
- `experiments/scg08_arithmetic_work/tier2_work_harness.hpp`
- `spec/rfcs/RFC-00E1-balanced-ternary-multi-precision-arithmetic-contract.md`


All other changes are docs/records audit packets, inactive CI snapshots, local
validation outputs and this integration index. changed-files.txt records the
complete final file list. preservation-checks.json retains identity/exclusion
checks; source-revisions.json retains lineage; integration-sha256.json hashes this
new integration packet. Source and logs are not rewritten to claim a different
execution environment or more independent validation.

Codex authored this assembly and the earlier portable-test/CI contributions,
with shared prior review context. These checks are not independent scientific
acceptance or authentication of original execution/provenance. They establish
bounded tested behavior and content identity. No full-domain arithmetic,
comparative hardware performance, production readiness or full-repository CI
claim is made. Existing baseline workflow-policy failures remain outside scope;
no checks or assertions were weakened. RFC stays draft, Gate 1 acceptance
WITHHELD, Gate 2 STRICT NO-GO. No merge, closure, release or gate advancement.

Whitespace validation: `git diff --check` on experiment/contract changes passes.
The whole-candidate check reports whitespace in retained raw CMake/CTest/JUnit
outputs; raw-evidence-whitespace-check.log.gz losslessly preserves the exact diagnostics. Those
bytes are intentionally retained rather than silently reformatted. This is not
a claim that the whole-candidate whitespace check passed.
