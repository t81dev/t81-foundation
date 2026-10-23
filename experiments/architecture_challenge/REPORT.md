# Architecture decision: inventory replenishment under revocation

2026-10-22. Experimental comparison based on
`0005ab675d2729b2795200e8c78ded43d4520b0d` (latest main after merged #553).
PR #536 was inspected as open at `3e52b6b5247b578e5f6a861a1b9a971b6e93c7cd`;
no merge, patch dependency, numerical expansion or frozen ISA modification.

Decision: **neither arm establishes authenticated occurrence or freshness under
a malicious producer. No architectural winner is justified.** Honest-host
revocation enforcement is comparable; no custom-VM advantage is demonstrated by
this bounded integer workload. The smallest enforcement mechanism in both arms
is the host-owned serialized authorization/write boundary. T81's native hook is
useful admission machinery, but the live spending limit and transaction binding
come from the experiment adapter, not a new Axion language feature.

## Reconciliation with existing research

- [Research charter](../../docs/research/RESEARCH_CHARTER.md): treat the architecture
  as falsifiable. Its identical internal-bundle criterion is narrowed here to
  protocol outcomes; native instruction traces/addresses retain their differences.
  Its 10x complexity/latency threshold is a hypothesis, not a prior measurement.
- [Experiment framework](../../docs/research/ACCOUNTABLE_COMPUTATION_EXPERIMENT_FRAMEWORK.md):
  reuse inventory replenishment, mutable authority, detached audit and adversaries.
  Its three-way design is sequenced as A/B first; no Python/OPA/database arm yet.
  Timing cannot choose revocation order. Duplicate, malformed, interrupted and
  malicious-producer cases are additional first-class observations.
- [Baseline-X proposal](../../docs/research/BASELINE_X_PROTOTYPE.md): use a standard
  Wasm runtime and a host capability boundary; do not build another VM, CanonFS or
  policy language. Its ~500 LOC, latency and determinism table contains targets
  and assertions, not measured controls. SQLite, Sigstore and JSON-LD are not
  necessary to test this local write boundary. Omitting a signature means origin
  remains deliberately unauthenticated, not implicitly supplied by content hashes.
- [Ternary closure](../../docs/research/TERNARY_RESEARCH_CLOSURE_AND_NEXT_HYPOTHESIS.md):
  retain the CPU ternary optimization stop. Candidate 3 motivates provenance
  work but supplies no benchmark result for this transaction. Neither its math
  overhead target nor numerical conformance is tested here.
- [Deterministic provenance v2](../deterministic_provenance/README.md): reuse its
  production interpreter/CanonFS seams, separate canonical evidence from timing,
  and independently recompute identities. Its instruction-specific oracle,
  unsigned evidence and explicitly absent transaction/revocation claim do not
  substitute for this boundary comparison. Preserve v1 failures and all existing
  numerical exclusions; no float API is exercised here.
- [RFC-00D4](../../spec/rfcs/RFC-00D4-accountable-decision-bundle.md): its six
  primitives inform the protocol: request/effect, hash evidence, fixed method,
  live authority, responsible host and independent challenge. This experiment is
  **not full RFC conformance**: no signature, timestamp or asserted signer; a
  verified denial must be distinguishable from an allowed act (the draft's
  blanket ALLOW-only verifier does not model that). Rehashing all six fields
  cannot make an invented history factual. A future protocol needs a pinned
  trust policy and authenticated execution/commit witness, not a self-declared key.
- CanonFS [RFC-00D1](../../spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md),
  [identity/persistence RFC-0054](../../spec/rfcs/RFC-0054-canonfs-object-identity-and-persistence-contract.md)
  and `include/t81/canonfs/canon_driver.hpp`: object identity is preserved;
  externally serialized callbacks are not a concurrent or transactional authority
  service. This experiment neither promotes interchange schema maturity nor
  claims import/export provenance proves factual correctness.

## Security boundary and comparison

The [machine-readable contract](workload-contract.json) fixes the request, TCB,
observable side effect, threat model and acceptance criteria for both arms.
The local PO payload is the protected effect. There is no supplier/payment
integration, model inference, numerical-performance or real-world inventory claim.

| Boundary | Arm A | Arm B |
| --- | --- | --- |
| Calculation | Existing T81 interpreter, four TISC instructions, production trace, static instruction budget | Wasmtime integer multiplication, 100 fuel units, single host import, no WASI |
| Final authorization | Persistent CanonFS invokes production Axion hook immediately before write; host binds actual amount, latest limit and one-shot session state to an allow/deny policy | Host `submit` binds actual amount, latest limit and one-shot session state immediately before file creation |
| Policy update | Explicit serialized transition after compute, after preliminary check, or after commit | Same external transition order inside host callback |
| Storage | Production SHA3-512/256 + Base81 CanonFS object paths | Standard SHA-256 named ordinary files |
| Evidence | Common canonical protocol envelope archived through native CanonFS, separate from PO effect | Same envelope archived through ordinary SHA-256 CAS |
| Native detail | Real VM PC/opcode trace retained in corpus | Native API/fuel mode retained; internal instruction tracing off |

Both hosts own authority and the write path. No external actor mutates authority
between final check and write in the serial executor. This defines a linearized
bounded execution schedule, not a thread-safety test. The existing concurrent
revocation experiment remains the separate real-thread test; there is no claim
that this adapter makes arbitrary Driver callback replacement thread-safe.
The `revoke-after-check` case permits the preliminary check but denies in the
actual storage hook. `revoke-after-commit` permits the already completed write.
The intentionally broken `snapshot-control` uses the earlier authority and is
caught by the external oracle in both arms. No sleeps/race probabilities select
any outcome.

The guest can invoke only its supported calculation and the single bound effect.
Wasm forged amounts, negative values and missing-capability imports are tested.
Arm A's program is constructed from validated bounded integers, not attacker
supplied TISC. Host memory corruption, privileged filesystem bypass, untrusted
compiler/OS and malicious replacement of the trusted observer are outside the
honest-host enforcement boundary, but **producer-controlled evidence is inside
the evidence threat model**. These are distinct claims.

## Adversarial result and missing primitives

Normal cases: approved, initially over-limit, malformed request, malformed policy,
same-session repeated invocation, abrupt process interruption before/after the
write, limit reduction after computation/check/commit. Both producers abruptly
exit 75 in interruption cases before protocol archival; the parent reconstructs
an explicitly observer-produced record from diagnostics and live storage. An
after-write interruption leaves a PO without producer protocol evidence. Recovery
is observable, not an atomic effect/evidence commit or a power-loss guarantee.

Tampering with stored payloads, input/result/limit/transaction fields or stale
policy stories is rejected, including after the attacker recomputes all hashes.
Missing/extra fields, duplicate JSON keys and booleans substituted for integers
are rejected. The verifier uses a caller-pinned case, never the producer's claim
about which schedule applied. A stateless bounded oracle is not a general VM.

The preserved attack corpus demonstrates two remaining failures in both arms:

1. Without invoking either runtime or writing a PO, an attacker constructs the
   correct expected story and all hashes. Detached verification accepts it.
   A genuinely independent live store observation rejects it; a producer-invented
   copy of that observation is accepted. The missing primitive is an authenticated
   witness of execution/commit tied to an externally trusted authority/event log.
   A producer signature alone only authenticates who made the potentially false claim.
2. An old valid envelope passes verification repeatedly. The host's in-session
   consumed flag blocks a second invocation, but no durable shared transaction
   registry or trusted nonce/epoch checkpoint survives independent sessions.
   The missing primitive is durable one-shot transaction/freshness state bound
   atomically to the effect and authority version. Neither native content addressing
   nor Wasm isolation supplies it.

Cryptographic integrity detects changed bytes; authorization is relative to the
pinned policy schedule; bounded reproducibility recomputes the fixed integer
method; factual correctness requires outside evidence about occurrence and source
facts. Passing the first three cannot establish the fourth. This prevents an
unsigned evidence checker from being reported as a proof of actual execution.

## Measurement and counting rules

Each sample starts a new producer process and fresh store; no warm-up is hidden.
Cases rotate and arms alternate. Steady clocks record nanoseconds. All raw values,
stdout/stderr, exit codes, native traces, file byte inventories and expected
negative-control failures are preserved; output directories are never reused.

| Metric | Timed interval / limitation |
| --- | --- |
| `setup_ns` | Request parsing, store setup, program/runtime creation; Wasm includes compilation/instantiation |
| `execution_ns` | T81 `run_to_halt`, including intrinsic trace/policy work; Wasm invocation minus host callback. Different native observation overhead is retained |
| `authorization_ns` | Host validation and verdict/event construction at the write boundary; A includes policy text construction/parsing and real Axion evaluation |
| `storage_ns` | Protected driver/file write minus the measured authorization interval; A includes CanonFS hashing, no fsync guarantee |
| `evidence_ns` | Common canonical encoding, payload digest and whole-record SHA-256; excludes evidence staging/archive writes |
| `staging_ns` | Common temporary evidence-file creation |
| `evidence_storage_ns` | Native archive write; A includes driver setup and admission inside archival process |
| `archive_process_ns` | A's extra archive process (startup included); B's in-process archive interval |
| `observation_ns` | Parent reads actual protected-file inventory; separate from verification |
| `verification_ns` | In-process bounded replay on captured inventory |
| `verification_process_ns` | Independent CLI startup, live-store inspection and verification |
| `process_ns` | Whole producer subprocess, including startup/stdout/native trace serialization |

Intervals do not form an additive end-to-end total: producer execution and
process intervals overlap; remaining native serialization falls inside process
time. VM intrinsic authorization/trace costs cannot be disentangled through its
stable API; only **side-effect authorization** is timed separately. Tiny workload
measurements include Python FFI overhead and cold-start costs; there is no claim
about general VM throughput, warmed inference, power or a 10x improvement.

`count.py` counts physical/nonblank lines and uncompressed bytes, including
comments, for each adapter, shared protocol, verifier/harness and validation code.
It inventories actual native compile sources and repository header dependencies
from Ninja, plus Python package files and native library bytes from installed
wheel metadata. Full Wasmtime implementation LOC is absent from wheels. Python,
the compiler and system dependencies are explicit uncounted obligations. Adapter
LOC is therefore comparable by a defined rule; full-stack dependency LOC and TCB
size are **not** claimed comparable. No earlier report's performance or source-size
estimate is used as a benchmark result.

Maintenance obligations differ: A retains custom ISA/interpreter, Axion parser,
driver/hash/serialization and their compatibility tests; B retains Wasmtime
updates and sandbox/capability ABI review. Both retain host boundary correctness,
protocol schema/oracle, canonicalization, digest upgrades, replay/freshness state,
trusted witness/key management and interrupted-write recovery. A smaller local
adapter does not by itself establish lower total dependency maintenance cost.

## Decision and smallest next increment

Do not claim that T81VM/Axion/CanonFS is necessary for this honest-host write
property: the conventional capability boundary matches it in the tested domain.
Do not declare the conventional stack a full adversarial winner either. Extracting
the bounded accountable-evidence protocol remains a reasonable candidate, but its
security contract must state exactly which external witness/authority/freshness
anchors are required. First add one durable transaction/authority-version witness
shared across restart, test producer fabrication against it, and atomically couple
its receipt to the protected effect or explicitly record recovery ambiguity.
Only then repeat the same A/B adjudication and consider a third OPA/database arm.
