# CI failure propagation restoration

Baseline: `01c85019319fd020922402ba356878e392cb2231` (fetched current main).
The finding was independently reproduced before editing the workflows.

## Root cause and baseline evidence

The [main lint job](https://github.com/t81dev/t81-foundation/actions/runs/40240040326/job/121309678929)
has conclusion `success`, including its action-pinning, permissions, governance
and aggregate-audit steps. Its own log contains their failing diagnostics.
The logged runner command is `/usr/bin/bash -e {0}`: absent `pipefail`, the
status of `validator 2>&1 | tee logfile` is the successful `tee` status. The
validator fails, but the workflow step succeeds and can print later successes.

GitHub's [shell contract](https://docs.github.com/en/actions/reference/workflows-and-actions/workflow-syntax#jobsjob_idstepsshell)
distinguishes implicit Linux Bash from explicit `shell: bash`, which invokes
`bash --noprofile --norc -e -o pipefail {0}`. An early-returning `grep -q` in
a conditional pipeline is a separate trap: `errexit` does not apply to the
condition, and `pipefail` can expose producer SIGPIPE after a valid match.

Before/after comparisons executed **24 unchanged Python validator commands**
individually from the baseline checkout, with only the tee destination redirected
to an isolated evidence directory. Direct execution: **11 pass, 13 fail**.
With the old runner invocation: **24 step successes**, including all 13 failed
validators. With explicit Bash: **11 successes, 13 failures**, agreeing with the
direct result in **24/24** cases. All **24** combined stdout/stderr logs remain
available. These comparisons do not treat an aggregate job badge as a test pass.

The locally exposed failures are:

- architecture target-table validation and architecture coherence;
- action pinning and explicit workflow permissions;
- root structure, translation metadata, translation staleness and book paths;
- public API lock/version, spec-to-code path mapping and status-label coherence;
- RFC lifecycle metadata and the aggregate governance wrapper.

Translation staleness reflects the execution date/history; local diagnostic
wording is not claimed identical to the earlier hosted run. The complete direct
outputs and existing step statuses are retained in the evidence archive.
None of these checks, policies, thresholds, expected results or underlying
implementations is repaired or relaxed in this change.

## Minimal mechanism correction

Two workflows, four Linux jobs:

| Workflow | Job | Previously unprotected logged producer |
| --- | --- | --- |
| ci.yml | spec-and-docs | Python tool installation; architecture, freeze, sandbox, action/permission and governance checks |
| ci.yml | warn-strict | warnings-as-errors build |
| ci.yml | benchmarks | benchmark execution and non-regression validator |
| ai-experiments-ci.yml | test-ai-experiments | three CTest logging pipelines |

Each job explicitly selects Bash. Existing tee commands and validator arguments
are retained. The other jobs' explicit shells/pipefail and deliberately handled
QEMU timeouts are unchanged; there is no global shell change for Windows jobs.

Four existing AI target-discovery bodies now run CMake help outside the
conditional and save its output before grepping the file. CMake failure fails
the step; a successful enumeration with no optional AI target still takes its
existing skip branch. Missing required conformance targets still fail. Target
names, build commands and conformance regexes are unchanged.

Spec/docs diagnostics upload with `always()`. Existing AI diagnostics also
upload after failures or early target absence, including the CMake target list.
The tightly scoped spec/docs and benchmark artifact paths explicitly allow
hidden files so `.github/logs` is retained. Warning-log failure upload and
benchmark `always()` upload remain active. Diagnostic upload does not clear the
preceding step failure. No new unconditional-success fallback is added.

## Adversarial verification

`scripts/ci/test_failure_propagation.py` uses only the Python standard library
and Bash. It reads the actual four job defaults and executes selected checked-in
run bodies with controlled tools in private temporary directories. It does not
run CMake builds, inference, research experiments, or governance repairs.

**47/47 controls pass**, zero failures:

- Exit 0, 7 and 23 producers through tee under each job's declared shell;
  combined logs persist and commands after a failing pipeline do not execute.
- A successful producer with failed tee fails the step.
- The actual multi-validator governance body succeeds when all producers
  succeed, and stops with a first or middle validator's exact failure code.
- Actual warning-build, benchmark/threshold and three CTest run bodies propagate
  injected failures while preserving diagnostics.
- Four target probes distinguish present, absent, failed enumeration with
  matching output, and failed build. Present-target output exceeds a pipe buffer
  to exercise the old `grep -q` hazard.

Two isolated mutations prove test sensitivity to semantic masking: appending
`|| true` to the first governance pipeline makes the actual step return **0
instead of 17**; restoring the old optional CMake/grep pipeline returns **0
instead of 29** on enumeration failure. Both mutant test runs exit 1 and identify
that wrong success result, rather than a crash or unrelated rejection.

Additional local verification: all **61** run bodies in the four jobs match a
separate YAML parse and pass `bash -n` after substituting GitHub expressions for
syntax checking. A structured YAML comparison confirms every unrelated field
is unchanged: trigger filters, permissions, dependencies, policy arguments,
existing check bodies and expected conformance inputs are retained. Only shell
defaults, four target-producer wrappers, the regression step and diagnostics
handling differ. Staged whitespace validation passes.

Environment: macOS ARM64, Python 3.14.3; existing Bash. YAML cross-check used
existing PyYAML 6.0.3 locally; it is not a new dependency of the CI regression.
Hosted Linux execution must be read from the corrective PR's own check run.
No production/runtime build or native Windows/QEMU run is claimed.

## Reproduction and evidence

From the corrective checkout, use a new output directory:

```sh
python3 scripts/ci/test_failure_propagation.py --output-dir /tmp/t81-ci-injection-new
```

To reproduce the underlying shell difference independently:

```sh
bash -e -c 'bash -c "echo diagnostic; exit 7" 2>&1 | tee /tmp/t81-before.log'
# status 0, log retained
bash --noprofile --norc -e -o pipefail -c 'bash -c "echo diagnostic; exit 7" 2>&1 | tee /tmp/t81-after.log'
# status 7, log retained
```

[results.json](results.json) records exact baseline/after statuses and workflow
hashes. [evidence.tar.gz](evidence.tar.gz) preserves the fetched original job
metadata/log, baseline workflow bytes, inventory of pipeline expressions, direct
validator results, 48 before/after log files, all controlled execution records,
mutation witnesses and reproduction helpers. Its manifest binds archived bytes.

Once a real check fails in hosted CI, later success-gated checks may be skipped;
they are **not** counted as executed failures or passes. A red job with the real
failed producer and retained diagnostics is the intended outcome. Repair of the
exposed baseline defects, production changes, and integration of research PRs
remain outside this correction.
