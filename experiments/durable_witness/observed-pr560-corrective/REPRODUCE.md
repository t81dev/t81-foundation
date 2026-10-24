# Reproduction and evidence index

All commands below run from the repository root. Use a **new** output directory
for each harness run. Keys shown here are public test fixtures. The captured
interpreter is represented as `${VENV}/bin/python`, with the pinned
`wasmtime==38.0.0` and `importlib_resources==7.1.0`; bare `python3` lacked Wasmtime.
`environment.json` records versions, source hashes, branch/SHAs, build flags, and
native binary hash. `sha256.json` hashes retained artifacts (excluding itself).

## Build and corrective run

```sh
work_dir="$(mktemp -d)"
python3 -m venv "$work_dir/venv"
"$work_dir/venv/bin/pip" install -r experiments/architecture_challenge/requirements.txt
export PATH="$work_dir/venv/bin:$PATH"
cmake -S . -B "$work_dir/build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_ARCHITECTURE_CHALLENGE=ON \
  -DT81_BUILD_TESTS=OFF -DT81_BUILD_TUI=OFF \
  -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_BENCHMARKS=OFF
cmake --build "$work_dir/build" --target t81_architecture_arm_a -j 4
export T81_ARM_A_BIN="$work_dir/build/t81_architecture_arm_a"
python -m unittest discover -s experiments/durable_witness -v
python experiments/durable_witness/record_cases.py \
  --output-dir "$work_dir/cases" --arm-a-bin "$T81_ARM_A_BIN"
python experiments/durable_witness/attacks.py --output-dir "$work_dir/attacks"
python experiments/durable_witness/run.py --output-dir "$work_dir/performance" --repetitions 30
python experiments/durable_witness/verify.py \
  "$work_dir/cases/evidence_approved-A.json" \
  --db "$work_dir/cases/witness.db" \
  --secret-key public-corrective-test-fixture-only
```

Recorded build paths are represented by `${BUILD_DIR}` in published captures;
the commands above choose a fresh build directory. The first configure attempt
omitted `T81_BUILD_TUI=OFF`
and failed trying to fetch FTXUI under network restrictions. `configure.log` and
`build.log` preserve that failed attempt; `configure-native.log` and
`build-native.log` preserve the successful targeted build. The successful link
retains a duplicate `libt81_core.a` linker warning; this task did not alter core
build configuration.

On Windows, use the `.exe` binary path and equivalent environment setup. Windows
and Linux were not executed here. `fork`/`forkserver` may be unsupported on some
platforms; the attack report identifies those contexts. The recorded sandbox
blocked the forkserver socket, so `attacks-final.log` was produced by rerunning the
same local command outside the sandbox into a separate directory. Do not describe
a blocked/unsupported context as a passing test.

## Baseline reproduction without changing this working tree

The retained `baseline-test_corrective.py` is the initial nine-test regression
fixture. Reconstruct only the original modules in a temporary directory:

```sh
python - <<'PY'
from pathlib import Path
import shutil, subprocess, sys, tempfile
sha = '056f0d6307f59049ef9dfa42c482f727a02c2b96'
with tempfile.TemporaryDirectory() as tmp:
    for name in ('runner.py', 'witness.py', 'verify.py', 'guest.wat'):
        data = subprocess.check_output(['git', 'show', f'{sha}:experiments/durable_witness/{name}'])
        (Path(tmp) / name).write_bytes(data)
    shutil.copyfile('experiments/durable_witness/observed-pr560-corrective/baseline-test_corrective.py', Path(tmp) / 'test_corrective.py')
    result = subprocess.run([sys.executable, str(Path(tmp) / 'test_corrective.py')])
    print('Expected failing baseline exit code:', result.returncode)
PY
```

Expected: nine tests, seven failure reports and fourteen error reports (subtests
included). This was rerun after fixes against those original modules, producing
`baseline-reproduced.log`. The initial pre-fix run is `baseline-regressions.log`.
`baseline-tests.log` records seven original tests with four missing-native errors.
`baseline-native-tests.log` records all seven original tests passing after injecting
the fresh native path. That injection was:

```python
import os
original = runner.run_arm_a
runner.run_arm_a = lambda _path, q, p: original(
    Path(os.environ["T81_ARM_A_BIN"]), q, p)
```

## Explicit old-encoding negative control

The regular suite includes a mutation test. To see the underlying security test
fail, restore only the historical signer in memory:

```sh
PYTHONPATH=experiments/durable_witness python - <<'PY'
import unittest
from unittest.mock import patch
import witness, verify
from test_corrective import CorrectiveContracts, historical_signature
with patch.object(witness, 'compute_receipt_signature', historical_signature), \
     patch.object(verify, 'compute_receipt_signature', historical_signature):
    result = unittest.TextTestRunner(verbosity=2).run(unittest.TestSuite([
        CorrectiveContracts('test_historical_collision_and_substitution')]))
assert len(result.failures) == 1 and not result.errors
PY
```

Expected inner result: **FAIL**, because distinct field tuples again have identical
HMACs. The outer assertion checks that the negative control actually failed;
`negative-control-old-encoding.log` preserves the failure. The ordinary canonical
encoding run separately checks byte inequality and offline verifier rejection of
substituted signed fields.

## Retained outputs

- `corrected-tests-first.log`, `corrected-tests.log`, and
  `corrected-tests-final.log`: successive suites; final is 22 tests, no skips.
- `cases/`: real native and Wasmtime approved/denied/revoked evidence, explicitly
  injected wrong adapter outputs, actual wrong Wasm guest, unavailable binary,
  SQLite DB and row counts, and detached verifier commands/output.
- `attacks/`, `attacks.log`: sandboxed run with forkserver failure preserved.
- `attacks-unsandboxed/`: first run of all supported contexts outside sandbox.
- `attacks-final/`: finalized queue/exit-status checks and partial-write trigger;
  attack 8 preserves pre/post SQLite snapshots and both accepted attempts.
- `attacks-first.log`: earlier sandboxed run, including the old read-only probe.
  It is historical output, not evidence for the final partial-write claim.
- `performance/`, `performance-final/`: separate 30-trial-per-arm measurements,
  receipts, SQLite stores and raw timing records; timing variation is retained.

Timestamps, scheduling, generated receipts and timing samples vary. Authorization
outcomes, the HMAC collision/negative control, admission counts, rollback reuse,
and verifier verdicts are the reproducible claims. HMAC test keys and local DBs
are public artifacts, not a deployed independent witness.

## Publication review

Local paths in logs and metadata were normalized before publication. Placeholders
`${REPO_ROOT}`, `${BUILD_DIR}`, `${FAILED_BUILD_DIR}`, `${VENV}`, `${TEMP_CASE}`,
`${EARLY_ATTACK_DIR}`, `${TOOLS}`, `${PYTHON_STDLIB}` and `${TOOLCHAIN}` denote path
roles, not literal directories to create. Memory addresses are shown as
`<address>`, and trailing log whitespace is trimmed. Compiler/dependency versions, failure messages, line numbers, test
results and measurement samples are retained. Receipt objects and SQLite files
were not altered. The pre-normalization captures remain outside the repository;
`sha256.json` records content hashes of the published copies; it is not a signature.
The `head_sha` in `environment.json` is the reference head at capture time; source
hashes identify the corrective implementation independently of that base SHA.

`pre-push-tests.log` records the additional final 22-test run.
`pre-push-receipt-verification.json` records independent re-verification of all
60 final measurement receipts against a temporary copy of their witness DB.
