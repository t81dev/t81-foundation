# PR #779 portable subprocess rejection candidate

Candidate only. Neither #778 nor #779 is merged or advanced. No production
arithmetic, accounting contract, RFC status, thresholds or SCG-08 gate changes.

## Exact source identities

- Accepted #779 base: `4d5f3586c04a69c520e613166291c0974877628d`.
- Original #778: `56ad0d57b90487d153b35ccfdeb134fcc858adf7`.
- Main used for reconciliation: `fbfe4f5a2d4439f5ff479014301560764de39c99`.
- Tested candidate code: `ceb1560af3a386050c1a611270aaf03a72b405a1`.
- Local candidate branch: `candidate/pr779-portable-rejections`.
- Local original-source archive branch: `archive/pr778-56ad0d57`.

The original eight-file #778 audit packet is retained byte-for-byte under
`docs/records/audits/2026-11-11-rfc00e1-corrections/`. Its correction document is
also copied there as `CONTRACT_CORRECTIONS.pr778.md`. `preservation.json` records
the original paths, retained paths and SHA-256 values. The archive branch retains
the original commit, including the sources addressed by its original manifest.
The separate local Git bundle contains both branches; no remote push is implied.

Preservation authenticates content identity only. The original macOS execution,
source-to-binary binding and provenance assertions remain author-reported.
Original failed/interrupted acquisition/configuration logs remain unchanged.
Do not interpret these original results as runs of this candidate.

## Bounded adaptation

The four executable/build/test changes are:

- `experiments/scg08_arithmetic_work/CMakeLists.txt`: register the rejection
  driver and its self-tests. Twelve ten-second child limits fit within a
  150-second outer timeout.
- `test_exp_scg08_01.cpp`: replace POSIX pipe/fork/wait with named child-case
  dispatch, keeping the existing validator fixture assertions and actual
  corrupted-object paths. Unknown/malformed CLI requests exit 2. If a selected
  rejection returns, the child immediately exits 0, so a later fixture cannot
  accidentally provide the expected failure. An unreached case exits 2.
- `test_result_rejection.py`: adapt #778's argument-vector subprocess mechanism
  to all twelve #779 rejection paths, requiring exactly exit 1 AND the correct
  stderr diagnostic; timeout and launch failure fail the test. No shell is used.
- `test_result_rejection_driver.py`: reject false passes from status 0, status 2,
  signal-like status, missing/wrong diagnostics, stdout-only diagnostics,
  launch failures and timeouts; verify per-case argument vectors.

Cases preserve numerical equality, overflow classification, exact negative
product bound, canonical trits, stored-tryte validation/decoding/add/multiply,
sign handling, binary/ternary truncation, and the trial-budget guard.

Normal C++ execution retains the report/fixture assertions. Process rejection
verification is now a separate CTest, so run the complete `-R scg08` selection;
the standalone no-argument executable alone does not execute rejection children.
All C++ checks still use the Release-active SCG08_REQUIRE macro. Python checks
use explicit conditional failure, not removable Python assertions.

`invariants.json` and `verify_preservation.py` check unchanged #779 oracle,
Tier-1/Tier-2 harnesses, implementation report, historical evidence and RFC;
production include/src diff is empty. Six removed assertions concern only the
superseded POSIX process machinery. Fixture assertions remain present. No #778
counter/event definitions or counter fixture expectations were imported.
Historical evidence remains Git blob `00731bd34cfad475acde588085827698627ec60d`.

## Locally executed verification

See `verification.json` for exact commands, source/binary hashes and code SHA;
`environment.json` records macOS ARM64, Apple clang 21 and Python 3.14.3.

- Final project CMake Release build and all five targeted CTests passed.
  Generated compile flags include `-DNDEBUG`.
- All twelve selected children exited exactly 1 with the expected diagnostic.
- Driver self-tests passed (three methods, including invalid-outcome subcases).
- ASan + UBSan with `-DNDEBUG` passed the normal suite and all twelve children.
- Actual executable path containing spaces passed all twelve rejection checks.
- Unknown/malformed child dispatch requests exited 2 with usage diagnostics.
- clang-format 18.1.8 reports no further changes for modified C++ lines;
  `git diff --check` passes.

Two intermediate and two final parent runs each recorded 179 trials: 716 total
parent trials this session. Child validator fixtures do not invoke successful
counted arithmetic; budget children deliberately exercise the >1000 sentinel.
These are bounded regression checks, not a performance campaign or new historical
experimental observations. CTest durations are not performance evidence.

Leak detection was explicitly rejected by this platform's ASan runtime; both
failed logs are retained under `asan-leak-detection-unsupported-*`. Subsequent
ASan/UBSan passes used `detect_leaks=0`; no leak-sanitizer pass is claimed.
An initial invariant-check script expected seven removed POSIX assertions;
inspection established six and the retained checker uses that exact count.
Commons discovery initially lacked jsonschema in system Python and succeeded
using the existing `/private/tmp/t81-continuity-venv` environment. No research or
execution authorization is inferred from Commons discovery.

## Limits and attribution

Codex authored this candidate and ran its local verification, with the prior
shared review context of #778/#779. The original #778 packet is also Codex-authored.
This is not an independent cross-review or authentication of original runs.
Hashes do not establish provenance truth.

Windows MSVC/clang-cl, Linux and x86 execution were not performed. Removing POSIX
calls does not establish Windows compilation or execution; platform CI and
independent review remain necessary before portability acceptance. No candidate
CI was obtained because no branch was pushed or PR opened. Required checks on
other PR heads do not apply to this candidate.

The accepted #779 bounded safety/conversion/exact-bound scope is retained.
W_ops remains a stipulated selected-kernel model, not total cost or established
comparative hardware performance. Conversion/normalization/memory exclusions,
unproven internal transient-overflow properties, and previous correctness and
provenance limitations remain. RFC stays draft, Gate 1 acceptance WITHHELD,
Gate 2 STRICT NO-GO. Neither production readiness nor full-domain arithmetic
correctness is established by these tests.
