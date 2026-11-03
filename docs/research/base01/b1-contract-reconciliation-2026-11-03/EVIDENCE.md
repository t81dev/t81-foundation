# Audit commands, evidence and limits

**PROPOSED — NOT APPROVED.** No performance or resource campaign ran.

Environment recorded in `evidence/static-checks.json`: macOS 26.5.2 ARM64,
Python 3.14.3, host Python built with Clang 17.0.0. No compiler/native build was
invoked for this audit. Existing Python environment
`/private/tmp/t81-base01-venv/bin/python` supplied jsonschema for offline discovery
and schema validation; it was not installed or modified here.

The shared checkout was at `04f7b30ec4a2171380ead964402147d485be24ec`, with
unrelated untracked research/runbook files. They were left untouched. Work was
isolated in `/private/tmp/t81-base01-reconciliation`, branch
`audit/base01-b1-contract-reconciliation`, based on the observed main snapshot.

## Remote identity and local source checks

GitHub connector GETs, observed November 3, 2026:

```text
https://api.github.com/repos/t81dev/t81-foundation/branches/main
https://api.github.com/repos/t81dev/t81-foundation/pulls/694
https://api.github.com/repos/t81dev/t81-foundation/pulls/695
```

Selected PR fields are retained, rather than duplicating unrelated account/API
fields. The main response gave SHA `a98b22fb65c80f4225151cbcd5c6934ae076cbe1`.
These observations are not signed maintainer protocol decisions.

```sh
git fetch origin main
git worktree add -b audit/base01-b1-contract-reconciliation \
  /private/tmp/t81-base01-reconciliation a98b22fb65c80f4225151cbcd5c6934ae076cbe1
git merge-base --is-ancestor 52921bd08ff9a9d765d4f129791b5a98304b6b6e a98b22fb65c80f4225151cbcd5c6934ae076cbe1
git merge-base --is-ancestor 4f9edb9c3d434bbc89331fc1c428e375d71d3fba a98b22fb65c80f4225151cbcd5c6934ae076cbe1
```

The first non-escalated fetch failed with `.git/FETCH_HEAD: Operation not
permitted`; the sandbox-approved retry succeeded. Both ancestry commands exited
0. The isolated worktree was clean before this new audit directory was added.
No existing tracked file was edited. `rg` discovery and `nl -ba`/`sed` reads
covered the source-index paths. Scoped implementation search:

```sh
rg -n 'permit|Ed25519|watchdog|timeout|RLIMIT|cgroup|monotonic|steady_clock' \
  experiments/base01* --glob '*.py' --glob '*.cpp' --glob '*.hpp'
```

Absence classifications additionally use the inspected entry points, launch/wait
paths and authorization interface; keyword absence alone is not a security proof.
Other repository subsystems were outside this review.

`evidence/source-index.json` records 26 current-main source hashes/URLs. Each
working file was compared to `git show <main-sha>:<path>`. Runtime paths
`fs/in_memory_driver.cpp`, `include/t81/canonfs/canon_driver.hpp`,
`vm/tracing/canonhash81.cpp` and `benchmarks/BM_CanonFS.cpp` were also compared
with `git show cda9135d36144d4de2771af174e52ac7b30a83c9:<path>`: all matched.
No workload execution is involved in these content comparisons.

## Newly executed focused fixtures

Working directory: `experiments/base01_b1_foundation` in the isolated worktree.

```sh
python3 -B -m unittest \
  test_foundation.BoundaryTests.test_python_rejects_all_measurement_requests_before_work \
  test_foundation.BoundaryTests.test_stale_native_identity_rejected_before_worker_launch \
  test_foundation.BoundaryTests.test_entire_manifest_is_pinned -v
```

Exit 0; **3 passed, 0 skipped, 0 failed**. Raw stdout/stderr are in
`evidence/gate-fixtures.log`. unittest's displayed test duration is framework
bookkeeping, not a workload timing sample. The first test asserts mocked
validation/source/process functions uncalled; the second simulates a stale
native identity and asserts no worker or output directory; the third checks
manifest identity against corruption. No native binary ran. Do not extend these
results to permit cryptography, watchdogs, real artifact swapping or native denial.

## Commons discovery and schema checks

```sh
python3 -B tools/research_commons/commons.py --request \
  '{"protocol":"t81.research-commons.v1","operation":"get_hypothesis","arguments":{"hypothesis_id":"H07"}}'
```

Initial attempt exited 1: missing `jsonschema`. The same failure was captured in
`evidence/commons-discovery-failure.log`; it was not interpreted as successful
discovery. The first retry using `/private/tmp/t81-commons-venv/bin/python` also failed
with missing jsonschema, as did a schema check using that interpreter; retained
in `evidence/commons-retry-failure.log` and `evidence/schema-environment-failure.log`.
The directory contained package files but that interpreter did not resolve them.
No dependency installation was needed. The successful retry used the existing
BASE-01 environment:

```sh
/private/tmp/t81-base01-venv/bin/python -B tools/research_commons/commons.py --request \
  '{"protocol":"t81.research-commons.v1","operation":"get_hypothesis","arguments":{"hypothesis_id":"H07"}}'
```

Exit 0; complete response `evidence/commons-h07.json`, empty successful stderr
`evidence/commons-discovery.log`. This is offline source discovery, not experiment
execution. `review.json` uses the Commons independent-review schema with
independence limits and `independent_execution: false`; it does not update the
authoritative Commons manifest or accepted findings.

Schema and source-index verification are reproduced from the repository root:

```sh
/private/tmp/t81-base01-venv/bin/python -B - <<'PY'
import hashlib, json, subprocess
from pathlib import Path
from jsonschema import Draft202012Validator
r = Path('docs/research/base01/b1-contract-reconciliation-2026-11-03')
schema = json.loads(Path('docs/research/commons/independent-review.schema.json').read_text())
Draft202012Validator(schema).validate(json.loads((r/'review.json').read_text()))
index = json.loads((r/'evidence/source-index.json').read_text())
for item in index['sources']:
    data = subprocess.check_output(['git', 'show', item['revision']+':'+item['path']])
    assert hashlib.sha256(data).hexdigest() == item['sha256']
assert 100*1048576 == 104857600 > 16*1024**2
assert 100*1048576 - 16*1024**2 == 88080384
assert 1/100 == .01 and 1/200 == .005
assert 10*10 == 100 and 10*1048576 == 10485760
print('PASS: review schema, 26 pinned sources and static arithmetic')
PY
git diff --check
```

These static derivations generated `evidence/static-checks.json`; their values
are integer accounting and ratios, not observed memory or performance. Final
validation output is retained as `evidence/artifact-validation.log`.

`SHA256SUMS.json` inventories every file in this new audit directory except itself.
Verify hashes against bytes; this supplies content identity, not authenticity,
approval or scientific truth. Original evidence indexes were not regenerated.
No claim is made that the full integration test suite, live accounting, physical
Linux correctness runbook, R75 probe or previous scientific findings were rerun.
