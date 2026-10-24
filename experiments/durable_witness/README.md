# Durable witness experiment

This bounded experiment compares T81VM and Wasmtime computation followed by
SQLite witness admission. It tests authorization outcomes, receipt field binding,
and replay rejection in a continuous database. The Python runner holds the HMAC
key and database access: **witness isolation is unresolved**. This is not evidence
of resistance to a malicious producer controlling that host process.

See [RESULTS.md](RESULTS.md) for the corrective findings and retained failures,
[THREAT_MODEL.md](THREAT_MODEL.md) for trust boundaries, and
[observed-pr560-corrective/REPRODUCE.md](observed-pr560-corrective/REPRODUCE.md)
for commands, versions, and baseline reproduction.

## Run

From the repository root, install
`experiments/architecture_challenge/requirements.txt` in a virtual environment.
Build the actual native adapter:

```sh
work_dir="$(mktemp -d)"
cmake -S . -B "$work_dir/build" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DT81_BUILD_ARCHITECTURE_CHALLENGE=ON \
  -DT81_BUILD_TESTS=OFF -DT81_BUILD_TUI=OFF \
  -DT81_BUILD_EXAMPLES=OFF -DT81_BUILD_BENCHMARKS=OFF
cmake --build "$work_dir/build" --target t81_architecture_arm_a -j 4
export T81_ARM_A_BIN="$work_dir/build/t81_architecture_arm_a"
python -m unittest discover -s experiments/durable_witness -v
```

Use fresh output directories; harnesses refuse to replace existing evidence:

```sh
python experiments/durable_witness/record_cases.py \
  --arm-a-bin "$T81_ARM_A_BIN" --output-dir "$work_dir/cases"
python experiments/durable_witness/attacks.py --output-dir "$work_dir/attacks"
python experiments/durable_witness/run.py \
  --output-dir "$work_dir/performance" --repetitions 30
python experiments/durable_witness/verify.py \
  "$work_dir/cases/evidence_approved-A.json" \
  --db "$work_dir/cases/witness.db" \
  --secret-key public-corrective-test-fixture-only
```

The displayed key is a public experiment fixture, not a deployment secret.
Runtime unavailability yields `RUNTIME_UNAVAILABLE`; no Python computation
substitutes for a runtime. Native-only corrective tests report `UNAVAILABLE` via
unittest skips when the binary is absent; that is not native validation.

## Admission contract

Both arms accept integer quantity and unit price in `[1, 1000]`, matching the
native adapter's input grammar. Other inputs yield `MALFORMED_INPUT` before
execution. For valid inputs, the runtime computes the submitted amount. The
witness requires `computed_amount` and validates it against the PO before any
writes. Mismatches yield `COMPUTED_AMOUNT_MISMATCH`. Python multiplication here
is an admission check, never an execution fallback.

The common outcome precedence is input validity, runtime availability/response,
amount binding, replay, authority existence/revocation, policy limits, and native
admission veto. Witness policy rows can further restrict the contract limits.
A native `denied` response is retained under `runtime`; it cannot produce a
successful receipt. The native adapter's temporary CanonFS commitment is distinct
from the durable witness's SQLite transaction. Its built-in limit is 50,000;
it does not implement the witness's authority-version registry.

`SUCCESS`, `POLICY_LIMIT_EXCEEDED`, `REVOCATION_DETECTED`, and
`INVALID_AUTHORITY_VERSION` describe witness-level authorization. Malformed native
JSON/response shape yields `MALFORMED_RUNTIME_RESPONSE`; nonzero exit, timeout, or
Wasm trap yields `RUNTIME_FAILURE`. An additional native denial under otherwise
admissible witness policy yields `RUNTIME_POLICY_DENIED`.

Receipts sign the validated amount, payload hash, identities, and timestamp using
versioned canonical JSON. Attempt files are created exclusively; retries preserve
the first evidence and append `.attempt-N.json`. IDs are percent-encoded in file
names to handle separators and Windows-invalid colons. A database row records a
local state transition; it does not prove payment, shipment, or another external
effect.
