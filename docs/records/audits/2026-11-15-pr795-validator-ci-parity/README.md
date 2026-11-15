# PR #795 validator execution and pinned parity verification

Tested accepted PR head: **43747b60675444ab58f693713577d6cd42e3f234**.
The PR branch remains at that SHA; no source, workflow, schema, historical
artifact or branch-head correction was needed. No merge was performed.

## Hosted CI finding

**The current hosted CI wiring does not select #795's SCG-08 validator suites.**
A green unrelated check must not be described as validator-suite coverage.

The active `.github/workflows/` files neither invoke the validator directly nor
enable `T81_BUILD_EXPERIMENTS`. Top-level CMake defaults that option to OFF;
SCG-08 test registration requires both it and `T81_BUILD_TESTS`. The regular
build matrix uses a general Release configure and CTest invocation, while the
cross-platform workflow enables the separate provenance experiment and uses
other test regexes. Those choices do not enable this SCG-08 suite. These are
source-level selection findings, supported by pinned snapshots and
`workflow-inspection.json`; they are not a claim that every pending hosted job
has finished.

Hosted run inspected:
[T81 Foundation CI #43714148467](https://github.com/t81dev/t81-foundation/actions/runs/43714148467).
`hosted-runs.json`, `hosted-jobs.json`, and `hosted-jobs-later.json` retain API
responses for the accepted head. Some broad build/test jobs were still pending
or running at the latest observation. No complete hosted-CI success is claimed.

The completed Linux no-SIMD job
[132812538011](https://github.com/t81dev/t81-foundation/actions/runs/43714148467/job/132812538011)
has its raw log in `hosted-no-simd-job-log.json`. It checked out PR merge revision
`4062a3da52e590569bd072d79ac3e80d44622e4b`, merging this exact accepted head into
main `c2ab68e582c9f773ce94a9e13bcb805016f07dd4`. Its recorded CMake command is
Debug with tests ON, without experiments ON; its selected equivalence test is
not SCG-08 validation. This log is hosted evidence of that job, **not** hosted
execution of the validator or local macOS test results.

The earlier `scg08-correctness.yml` and runner are archived under the
#779/#785 integration packet, outside active workflow/script paths. Their source
SHA guard targets a historical revision. They were not activated, altered or
relabeled to validate #795. There is no targeted SCG-08 dispatch input in the
current active workflow. Rerunning it unchanged would not add missing coverage.
No new standalone workflow or schema-engine expansion was introduced.

## Direct Python execution: all six commands pass

Local environment: **macOS 26.5.2 ARM64, Python 3.14.3**. An isolated virtual
environment uses the existing archived SCG-08 requirements pin
`jsonschema==4.26.0`, rather than the unrelated Research Commons pin.
`pip-freeze.stdout.log` retains every installed version. Dependency metadata and
version checks are in `environment.stdout.log` and `local-results.json`.

With `PY=/private/tmp/t81-pr795-parity-venv/bin/python` and
`V=scripts/ci/validate_scg08_gate1_evidence.py`, run from the accepted worktree:

```sh
$PY $V
$PY -O $V
$PY $V --test-negative
$PY -O $V --test-negative
$PY $V --test-json-files
$PY -O $V --test-json-files
```

| Direct suite | Normal | Python -O |
|---|---|---|
| Committed-record positive validation | exit 0, PASS | exit 0, PASS |
| Negative fixtures | 94 rejections, exit 0 | 94 rejections, exit 0 |
| Optional Draft 7 parity within negative suite | 61 rejection / 2 positive records, PASS | 61 rejection / 2 positive records, PASS |
| JSON-file/child CLI fixtures | 57 cases, exit 0 | 57 cases, exit 0 |

The JSON-file cases comprise 49 rejection and 8 positive controls. Parity was
actually executed, not skipped: the retained stdout contains the explicit
61/2 success marker in both negative runs. Parity is a bounded comparison of
these fixtures, not full semantic equivalence between schema and domain rules.
The declared integral-float difference remains explicitly reported.

## CTest execution: five registered tests pass

A separate **Release** build directory was configured with tests and experiments
ON and `Python3_EXECUTABLE` pointing at the same dependency-equipped virtual
environment. The exact configure invocation is in `local-results.json` and
`run.py`. No C++ target build or arithmetic executable execution was needed for
these Python-only tests during this follow-up.

```sh
ctest --test-dir /private/tmp/t81-pr795-parity-release --output-on-failure --no-tests=error -V --output-junit /private/tmp/t81-pr795-validator-verification/ctest-junit.xml -R '^t81_exp_scg08_01_schema_'
```

**5/5 PASS**:

- `t81_exp_scg08_01_schema_validation`: normal positive validation.
- `t81_exp_scg08_01_schema_negative_validation`: 94 negatives and 61/2 parity.
- `t81_exp_scg08_01_schema_json_files`: 57 file cases.
- `t81_exp_scg08_01_schema_negative_optimized`: explicit `python -O`, same negatives/parity.
- `t81_exp_scg08_01_schema_json_files_optimized`: explicit `python -O`, 57 file cases.

`ctest-inventory.stdout.log` retains the exact discovered interpreter/arguments;
`ctest-registered.stdout.log`, `ctest-LastTest.log` and `ctest-junit.xml` retain
actual executions. The optimized positive suite has **no separate registration**
on this accepted head: its success above is direct Python execution, not a sixth
CTest result. This distinction is intentional and no test count is inflated.

## Retained evidence and limits

`local-results.json` includes command arrays, working directory, UTC start times,
exact exits, raw stdout/stderr, source SHA, source hashes and environment.
Individual logs, CMake cache, generated CTest registration, runner script,
workflow inspection and hosted API/log snapshots are retained separately.
`SHA256SUMS.json` hashes the packet. `preservation.json` verifies the PR head,
clean accepted worktree and unchanged executable/schema/evidence hashes against
the earlier candidate receipt.

No source correction was indicated by these runs. The coverage omission is
reported as a CI selection gap; the accepted PR head was not advanced to wire
new CI behavior. This packet is retained separately from that accepted head.
No PR comment, workflow dispatch, rerun, merge or original #767 update was made.

All validator execution in this packet is **local macOS ARM64 execution**.
The separately retained hosted Linux job did not run these suites. No hosted
Linux/Windows/macOS validator result, other interpreter version, broader test
suite, benchmark, Gate 2 or scientific acceptance is claimed. Codex authored the
candidate and this verification with prior context; this is engineering
verification rather than independent scientific review.
