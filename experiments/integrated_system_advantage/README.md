# Investigation 08 — conventional comparator corrective gate

This is a bounded **pure-Python implementation comparison** with the production
`t81 ai task assess-fixed` path. It does not establish architectural performance
or assurance superiority. Investigation 08 remains open; performance, assurance,
composition, and ternary-specific advantage remain **INCONCLUSIVE**.

The research scripts require Python 3.10 or later. The comparator interprets checked T81W1/T81W2 balanced-ternary models for the
16-wide, single-row, two-layer attention slice used by the demo. It is not a
full transformer or general Axion implementation. Its supported APL subset is
`tier`, `allowed-ternary-model-hashes`, and `require-axion-event (reason ...)`;
unsupported clauses fail explicitly. The task's admission hash/marker checks
are distinct from propagation of arbitrary constraints into the VM probe.

The initial-probe CLI uses eight sampled candidates (or fewer for a small
vocabulary) and `max_score.v1`, retaining the **first sampled candidate on ties**.
This differs from the separate full-vocabulary greedy API. Projection matmuls
use input/output row-major orientation and snap activations before accumulation.
The single-row attention softmax has one element, so its output is the value row.

The expanded gate compares:

- All nine intermediate tensors, captured at production VM instructions by the
  opt-in `t81_investigation08_probe`; no assertion-only checks or assumed pool
  ordering. Candidate IDs/scores also come from the actual decoder and CLI
  provenance, independently of the Python computation.
- Labels, selected tokens, normalization, policy decisions, model identities,
  exact result bytes/references, provenance linkage and integrity, and replay.
  Provenance and conventional recovery bundles have backend-specific contents;
  their references are verified against their own bytes, not claimed identical.
- Original/unknown/whitespace/context inputs, nonsymmetric valid weight mutations,
  tokenizer changes including BPE pieces and wrapped sampling ties, malformed
  models/policies, invalid binary objects, tampering and inconsistent links.
- Exact tokenizer text, including the reference reader's limited escape handling:
  it removes a backslash and retains the following character rather than decoding
  Unicode escapes. Literal UTF-8 and escaped JSON can therefore select different
  tokens. Matching this behavior does not certify general JSON/BPE correctness.
- New-runner and fresh-process recovery from persisted input/model/policy/tokenizer
  references, with **no override arguments**. Reference CLI artifact recovery
  uses fresh `canonfs get` processes. Its timed rerun still needs original fixture
  files; it is not standalone model recovery from its result object.
- Store uniqueness across repeated invocations. Every observation has separate
  arm roots; populated trials are explicitly prepopulated before measurement.

Object references retain the reference protocol's `sha3-256:` label: the bytes
are SHA3-512 hashed, truncated to 32 bytes and Base-81 encoded. Binary bytes are
never decoded before hashing. Integrity establishes identity, not provenance truth.
Stores assume a trusted local owner and sequential access; no hostile-host,
symlink-race, concurrency, durability, or signature-authentication guarantee is made.

Build the research instrumentation explicitly (normal builds leave it off):

```sh
cmake -S . -B build -G Ninja -DT81_BUILD_TESTS=ON -DT81_BUILD_EXAMPLES=ON -DT81_BUILD_INVESTIGATION08_PROBE=ON
cmake --build build --parallel 6 --target t81 t81_make_assess_fixed_demo t81_investigation08_probe
T81_BUILD_DIR=build python3 experiments/integrated_system_advantage/test_differential_parity.py
```

Run new observations only through the gate (default 30 observations per condition;
a smaller declared count is a smoke measurement with correspondingly weak uncertainty):

```sh
python3 experiments/integrated_system_advantage/run_experiments.py --build-dir build --iterations 30 --seed 602 --out /tmp/investigation08-new-run.json
```

The runner executes the complete parity suite before any timing trials and stops
on failure. It refuses to overwrite a result file. Both arms launch a fresh process
for every observation and replay; order is randomized using a recorded seed.
Raw observations include roots, initial/pre-observation entry counts, ordering,
identity, verification, wall times and separate measurable phases. Python module
initialization, fixture/model loading, policy evaluation, computation, persistence,
lookup, and replay are measured separately. The parent/worker residual includes
startup, teardown, serialization and pipes; it is not isolated startup cost.
The existing T81 CLI exposes no phase clocks, so its internal phases are **null /
unmeasured**. Its process wall time cannot be decomposed into invented costs.

Even with bounded parity, Python loops and optimized C++/SIMD code are different
implementation conditions. Cache/page residency, OS scheduling and trial order
remain confounders; fresh stores are not cold OS caches. Report dispersion and
sample count, retain raw observations, and infer no intrinsic T81 advantage or
disadvantage. Missing C–F ablation seams remain unimplemented, not negative results.

Earlier committed observations are retained byte for byte in `historical/`, with
source commits and invalidation reasons. They are not silently regenerated or
promoted by passing the new gate. See `CORRECTIVE_REVIEW.md` for the actual review
head, commands, outcomes, remaining limitations and scientific disposition.
