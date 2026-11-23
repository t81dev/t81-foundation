# Scoped CI for PR #785

This separate CI-only change depends on #785. It never replaces or advances
#779/#778, and does not merge directly into main. The tested source is hard-pinned
to `300cdea8896d56c025f7099c0cdf0c41d7bd6237`. Harness and source SHAs are recorded
separately in every `summary.json`; the new harness SHA does not imply a new
arithmetic source revision.

`.github/workflows/scg08-correctness.yml` runs three independent, fail-fast-disabled
lanes: Ubuntu 24.04 GCC 14, Windows 2025 MSVC, and Windows 2025 clang-cl. Full source
checkout includes experiments/ and all CMake-referenced paths. A separate harness
checkout uses the CI PR head. Both checkouts disable credential persistence.
Windows uses the x64 MSVC development environment; compiler versions and actual
compile commands are retained rather than assumed. Python 3.12 and pinned
jsonschema 4.26.0 enable the existing schema validator's optional Draft 7 parity
checks. PYTHONUTF8=1 preserves its Unicode output on Windows.

The runner executes argument vectors without a shell, captures command logs,
propagates nonzero exits, and retains summary/partial results on failure. It checks
the exact source SHA, validates the experiment compile command contains NDEBUG,
and requires exactly these five names in both CTest discovery and successful
JUnit output (no skipped tests):

- t81_exp_scg08_01_test
- t81_exp_scg08_01_result_rejection
- t81_exp_scg08_01_result_rejection_driver
- t81_exp_scg08_01_schema_validation
- t81_exp_scg08_01_schema_negative_validation

Configure uses Ninja and Release with T81_BUILD_EXPERIMENTS=ON and
T81_BUILD_TESTS=ON. Benchmarks, fuzz targets, examples, TUI and TernaryOS are OFF.
Only `cmake --build BUILD --config Release --target t81_exp_scg08_01_test
--parallel 2 --verbose` is built, including its declared core dependencies.
Only `ctest --test-dir BUILD -C Release -R ^t81_exp_scg08_01 --no-tests=error
--output-on-failure --verbose --output-junit EVIDENCE/junit.xml` executes tests.
No Gate 2 invocation, performance measurement or acceptance promotion occurs.
See the runner and per-run summary.json for complete argv, not shell reconstructions.

Artifacts retain commands, source/runner identities, dependency versions, source
hashes, actual compiler command, configure/build/verbose CTest logs, JUnit,
LastTest.log, CMake cache, compile_commands.json and SHA-256 manifest. Retention
is 90 days; download/archive packets before expiry for durable research memory.
Setup failures also remain in actual Actions job logs. No passing result can be
inferred merely from the workflow existing or a baseline general-CI result.

## Local verification before publication

The runner was executed on macOS ARM64 with Apple clang 21 and Python 3.14.3,
using the existing jsonschema 4.26.0 environment and an explicitly recorded local
Asio checkout. All five CTests passed with NDEBUG; schema parity also ran.
The local/ packet preserves exact commands, source hashes and results. Its
harness HEAD is the base revision because this new runner was then uncommitted;
the runner's SHA-256 records the tested content. This is local orchestration
verification, not Linux or Windows execution. Final workflow jobs remain pending
until actual logs establish their outcome.

Global action-pin and permissions policy checks fail on both unchanged #785
and this CI change: seven pre-existing tag references and one workflow missing
explicit permissions in qemu-boot-x86.yml. Logs are retained. The new workflow
uses five immutable action references and explicit contents:read; no existing
policy check is relaxed or bypassed. Those baseline failures do not establish
anything about the targeted compiler/test outcomes.

Codex authored this harness and local verification with prior shared #778/#779/#785
context. This is not an independent cross-review or authentication of historical
runs. Exact object/source hashes establish identity, not truth of provenance.
Validators, accounting/event definitions, fixtures and historical evidence remain
unchanged. All previously stated arithmetic, modeled-cost, platform and provenance
limitations survive. RFC remains draft, Gate 1 acceptance WITHHELD, Gate 2 STRICT
NO-GO. No merge is authorized by this record.
