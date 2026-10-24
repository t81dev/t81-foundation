# Research report: durable witnessed transactions

Status: local validation complete; committed-source measurements and native CI
observations will be recorded below. No architectural winner is assumed.

## Decision

A shared protocol can bind a trusted local ledger effect to durable authorization
and a detached signed receipt without requiring T81VM or Wasmtime in the verifier.
The witness owns the effect and freshness state. Neither runtime contributes an
essential primitive to that bounded guarantee. This does not establish the same
property for arbitrary CanonFS files, external supplier APIs, or a malicious host.
OS isolation, trusted storage continuity and key custody remain requirements.

## Sources and scope

Baseline main `0005ab675d2729b2795200e8c78ded43d4520b0d` (after #553).
Reference #554 `7af837ece1325752e555b814c94aacc33afd2c96`; reviewed and audited,
never merged. #536 reviewed at `6555f0b1636f18baa77e8141118fec47d0cbdc47`
for bounded-integer/backend correctness risks only; no dependency.
See [pre-implementation review](THREAT_MODEL.md) and [exact protocol](PROTOCOL.md).
All new implementation lives under this experiment; the sole outside addition is
a narrowly triggered native CI workflow. No production interfaces/ISA/T81Float
or CanonFS filenames are modified.

The previous experiment's Linux/macOS archives independently pass its archive
byte auditor: 220 corresponding envelopes match, including stale-policy controls.
Its attack generator reproduces 14 attacks with eight accepted fabrications/replays.
That old unsigned evidence remains a negative result, not an authenticated baseline.
The previous Windows storage failure remains separate and unresolved by this work.

The effect boundary deliberately changes from #554's native file write to a
shared SQLite ledger entry. Arm A executes existing T81VM with its instruction
policy; Arm B executes Wasmtime with a named host capability. Both are untrusted
calculators whose proposals face the same witness checks. This is not an unchanged
end-to-end T81VM/Axion/CanonFS versus Wasmtime/file comparison, nor evidence that
CanonFS already has transactions. Avoiding a production interface change is possible
because no arbitrary external file write is included in the atomic effect.

## Methods and acceptance boundaries

Each test creates a fresh ledger/key/epoch, expected UUID request, verifier state,
and invokes the real arm calculator. The witness validates proposals independently.
API/CLI tests preserve raw outputs and database payload observations. Crashes use
actual abrupt process exit 75; recovery is a new process. Concurrent pairs race
reserve and commit with primary keys/BEGIN IMMEDIATE selecting a single winner.
No sleep or scheduler probability determines expected revocation ordering.

The public evidence exporter excludes private keys and raw databases; payloads,
public keys, candidate receipts, externally supplied requests and test outcomes
are preserved. Test keys/pins are public reproducibility fixtures, not a real
research-publication trust anchor. The harness is a trusted experiment observer.

| Required adversarial case | Expected / observed local result |
| --- | --- |
| Valid authorized effect | committed and independently consumed |
| Revocation before final admission | denied, no effect/receipt |
| Revocation after commit | earlier receipt remains historically valid |
| Duplicate in process | reserve and repeat commit rejected; one effect |
| Duplicate after restart | both rejected in new witness processes |
| Previously valid receipt replay | rejected in same verifier and new process |
| Internally consistent fabricated history/hashes | untrusted signature rejected, no witness effect |
| Forged witness / invalid signatures | rejected against external key pin |
| Authority-version mismatch | rejected against expected request and pinned policy |
| Interrupt before effect | exit 75; recovery aborts, no effect |
| Interrupt after effect before publication | exit 75 after commit; durable identical outbox recovered |
| Restart incomplete transaction | pending becomes explicitly aborted |
| Concurrent same transaction | one reserve and one commit succeed; one effect |
| Corrupt/unavailable witness storage | fail closed, no receipt; missing DB not recreated |
| Direct effect boundary bypass | unreserved commit and producer file claim rejected |

Extra checks: crash after uncommitted SQL insertion rolls back; forged amount is
denied; malformed JSON/schema rejected before consumption; lost/corrupt verifier
state fails closed; unknown or mismatched effect/outbox recovery refuses success.
Every observation records expected/observed outcome, trust assumptions and remaining
attack surface. Denials and aborted transactions are successful checks, never
counted as successful authorized transactions.

Two negative controls per arm remain accepted outside the stated model:
raw same-UID database writes bypass the API, and restoring verifier state before
its receipt consumption permits replay. Both are critical if those powers belong
to the attacker. No OS sandbox or anti-rollback hardware is implemented. This is a
limit to the result, not an allowlist for an in-model security failure.

## Measurements

Measurements are observational, no regression thresholds or 10x target.
Cold calculator samples launch fresh processes and retain stdout/stderr/exit status.
Cold witness admission and restart recovery include imports, key loading and DB
integrity checking. Steady-owner samples reuse a witness connection and verifier
state. A uses a persistent calculator subprocess (IPC included); B reuses an engine
and compiled module in Python, creating a store/instance per call. Runtime-call
nanoseconds are recorded separately; this asymmetry forbids a general speed ranking.
No first sample, failure or outlier is discarded.

`reserve_ns` includes transactional preliminary authorization and durable attempt
creation. `final_authorization_ns` is only the final bounded boolean predicate
(after lock acquisition and DB reads). `commit_including_signing_ns` spans final
lock, authority read, effect insertion, signing, outbox/state update and durable
SQLite commit. `signing_ns` covers canonicalization/signature creation. Verification
includes cryptographic checking and durable receipt consumption. Intervals overlap;
do not sum them as disjoint costs. Database growth and wheel-file bytes are recorded,
not claimed to be full dependency/TCB cost. Python/OS/compiler/SQLite maintenance
and full Wasmtime source are additional obligations.

## Property separation and remaining failures

- Integrity: Ed25519 authenticates these recorded bytes to the pinned witness key.
- Authorization: the trusted owner serialized current policy with local admission;
  the verifier checks the pinned historical version. This is not current authority.
- Reproducibility: bounded multiplication/payload can be recomputed independently;
  a receipt does not prove which VM executed, or that any VM executed.
- Occurrence: the trusted witness performed and committed the precise ledger
  insertion it signed. A witness that merely signs a producer story would fail.
- Durability: atomic DB/outbox and EXTRA/fullfsync rely on honest VFS/storage flush
  and locking. Process crashes were tested; real power loss was not. Continued
  storage retention or rollback prevention cannot be proven by an old signature.
- Business truth: inventory, quotes, supplier receipt and payment remain unproved.

The protocol meets the requested local-effect criteria under the explicit witness
and durable-state trust model. A portable production security guarantee has not
been established: role isolation, network authentication, admin authorization,
key rotation, anti-rollback anchors, distributed verifier coordination and external
effect coupling are absent. The evidence supports a bounded protocol experiment,
not an architecture migration or retirement of existing production surfaces.

Both arms add the same trusted witness protocol, SQLite schema/locking/recovery,
crypto dependency and verifier state. T81 retains custom VM/ISA/Axion maintenance;
B retains Wasmtime/FFI/fuel/capability review. The calculators are outside receipt
soundness because the owner recomputes the tiny workload. Runtime correctness still
matters for workloads the witness cannot independently recompute. No evidence here
supports a general architectural winner or an advantage for ternary representation.

## Recorded runs

Pending committed-source recording.
