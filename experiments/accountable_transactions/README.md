# Durable authority and witnessed transactions

An isolated, falsifiable follow-up to [draft #554](https://github.com/t81dev/t81-foundation/pull/554).
Both T81VM and Wasmtime propose the same $25,000 purchase order to the **same**
trusted SQLite ledger owner. Ed25519 receipts bind its actual local ledger commit
to the request and policy version; durable primary keys and a separate verifier
consumption database prevent duplicate use across process restart.

Read [threat model/design decision](THREAT_MODEL.md), [protocol](PROTOCOL.md) and
[report](REPORT.md). This is an experimental local-effect protocol, not a new VM,
production CanonFS transaction layer, supplier transaction or RFC-00D4 conformance
claim. The local harness does not isolate roles at the OS level. Excluded same-UID
bypass and storage-rollback attacks remain visible in results.

## Reproduce

From repository root: C++23, CMake >=3.20, Ninja, Python 3.14 (tested) and pip:

```sh
python3 -m venv /tmp/t81-witness-venv
/tmp/t81-witness-venv/bin/python -m pip install -r experiments/accountable_transactions/requirements.txt
cmake -S experiments/accountable_transactions -B /tmp/t81-witness-build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build /tmp/t81-witness-build --target accountable_arm_a --parallel 4
/tmp/t81-witness-venv/bin/python experiments/accountable_transactions/test_protocol.py \
  --runner /tmp/t81-witness-build/accountable_arm_a --output /tmp/t81-witness-tests
/tmp/t81-witness-venv/bin/python experiments/accountable_transactions/measure.py \
  --runner /tmp/t81-witness-build/accountable_arm_a --output /tmp/t81-witness-measurements --repetitions 30
/tmp/t81-witness-venv/bin/python experiments/accountable_transactions/preserve.py \
  /tmp/t81-witness-tests /tmp/t81-witness-public-tests
```

Use fresh output paths. On Windows use `python` in the virtual environment and
the `.exe` runner in a VS developer shell. CI covers Linux x86_64, macOS ARM64,
and Windows x86_64; local results alone establish no cross-platform guarantee.
No optional reference PR checkout/merge or production interface edit is needed.
Root CMake is included only to link existing libraries; production options/ISA
are unchanged. `arm_a.cpp` uses four bounded TISC instructions and the existing
instruction policy; it does not claim the CanonFS driver enforces ledger admission.
`arm_b.py` uses Wasmtime 100-fuel integer multiplication and one named host import,
without WASI. These are untrusted calculators; witness final admission is identical.

`test_protocol.py` writes raw subprocess stdout/stderr/exit codes, per-case
producer output, externally supplied expected request, trust pins, observed ledger
payloads, attack receipts, recoveries and aggregate results. Direct tests use real
witness-process exit 75, fresh recovery processes and concurrent process pairs.
`preserve.py` exports public JSON/text evidence while excluding private witness
keys and SQLite databases. Native CI uploads these public artifacts even on failure.
Keep private keys out of publication, including temporary research keys.

## Verify independently

Copy **only** `verify.py` to a separate directory; install `cryptography==46.0.3`
(and its pinned cffi/pycparser requirements). T81VM, Wasmtime and producer modules
are unnecessary. For example, with a fresh output state path and public test files:

```sh
python verify.py --trust /tmp/t81-witness-public-tests/A-valid/trust.json \
  --state /tmp/auditor-consumed.db --init-state
python verify.py --trust /tmp/t81-witness-public-tests/A-valid/trust.json \
  --state /tmp/auditor-consumed.db \
  --receipt /tmp/t81-witness-public-tests/A-valid/receipt.json \
  --expected /tmp/t81-witness-public-tests/A-valid/expected.json
```

First consumption succeeds; repeat the second command and it must exit 2 with
`receipt replay`, including in a new process. Missing durable state is an error,
not permission to initialize again. In a real deployment, trust/expected inputs
must be pinned by the verifier operator out of band. The checked-in public test
keys demonstrate verification, not a production identity or authenticated research
publication. Never infer trust from an attacker-supplied sibling `trust.json`.

The verifier establishes integrity, pinned historical authority and a trusted
witness's observation of its own local ledger effect, consumed once in this
verifier domain. Bounded arithmetic can be recomputed. It does not establish
execution of a particular runtime, current authorization, worldwide freshness,
continued storage availability or truth of inventory/supplier facts.
