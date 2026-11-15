# Witnessed purchase order, version 1

Experimental identifier: `witnessed-po-v1`. This is not RFC-00D4 conformance.
No floating point, VM trace claims, policy language, network API or production ABI.

## Requests and trust

A caller supplies a canonical lower-case UUID transaction ID, a request with
exactly `order="8491"`, integer `units` and `price` in 1..1000, and an integer
`policy_version`. The fixture is the #554 workload: `100 250\n`, $25,000,
initial limit $50,000, reduced limit $10,000. UUIDs identify attempts, not business
order uniqueness: a fresh UUID can issue the same business order again. A business
idempotency key and authenticated client identities are deferred.

Trusted provisioning creates a fresh ledger UUID, Ed25519 key, database and trust
file. Never auto-reinitialize missing state. Interrupted provisioning is a failed
installation requiring explicit operator handling; there is no atomic multi-file
provisioning claim. The trusted policy file pins the raw public key, SHA-256
fingerprint witness identity, ledger epoch and accepted versions/limits. It is
installed out of band, never selected by a receipt. Version 1 allows 50000;
admin-only version 2 allows 10000. Updating verifier trust requires controlled
state migration, not deleting its replay state. This prototype fixes both pins
at provisioning and does not implement migration/key rotation.

## State transitions and linearization

| Operation | Transition | Effect / publication |
| --- | --- | --- |
| reserve(valid request/version under current limit) | absent → pending | durable immutable request/version; no effect |
| reserve(stale or denied) | absent → denied | durable consumed attempt; no receipt |
| reserve(existing ID) | unchanged, rejected | no second attempt |
| commit(pending, current version, exact computed amount within limit) | pending → committed | one ledger payload + signed outbox, atomic |
| commit(pending, stale/incorrect amount/over-limit) | pending → denied | no effect or success receipt |
| commit(non-pending/unknown) | unchanged, rejected | no repeated effect |
| recover(pending) | pending → aborted | explicit unsuccessful recovery; never silently retried |
| recover(committed) | unchanged | verify stored receipt against actual payload, retrieve identical receipt |
| recover(denied/aborted) | unchanged | no success receipt |
| recover(unknown/corrupt/inconsistent/unavailable) | error | no inferred success |
| revoke | policy 1 → policy 2 | serialized with commits; no retroactive undo |

`BEGIN IMMEDIATE` serializes reserve, commit, recovery and revocation across
processes. A concurrent revocation either commits before admission (deny stale
version) or after the effect (historical authorization remains valid). No sleeps
choose the outcome. Tests also prescribe before/after order explicitly.

Inside commit, the witness reads its stored request and current policy, checks
amount = units × price, constructs `8491 AMOUNT\n`, inserts that payload into
`effects`, signs a receipt, stores it in the same transaction and marks committed.
Only after SQLite commit returns does it publish the receipt. The witness
**controls and observes insertion into its own ledger**; it does not sign a
producer's file inventory. The receipt attests that this trusted implementation
committed the protected local ledger operation. It neither attests VM execution
nor proves arbitrary remote occurrence. A compromised witness can lie.

Fault checkpoints exit the real witness process with code 75: before effect,
after SQL insertion but before signing/commit, and after successful commit but
before stdout. The first two roll back to pending and recover to aborted. The
last retains effect/outbox and recovers committed without issuing another effect.
There is no committed-effect/missing-outbox interval inside this single database.
If commit reports an I/O error, treat the attempt as unresolved until a fresh
recovery; do not infer rollback or success from the exception alone.

## Receipt encoding and independent consumption

Envelope: exactly `body` and `signature` (128 lower-case hex characters).
Body: exactly `protocol`, `witness`, `ledger`, `transaction`, `request`,
`policy_version`, `limit`, `effect`, `status="committed"`. Effect: exactly
`payload` (the exact ASCII ledger bytes as text) and `sha256` (64 lower-case hex).
Integers exclude booleans. Unknown fields, duplicate keys, non-finite numbers,
noncanonical UUIDs/hex and mismatched pins are rejected.

Ed25519 signs `b"T81-WITNESSED-PO-v1\0" + canonical(body)`. Canonical encoding
is Python sorted compact JSON, `ensure_ascii=True`, no NaN, using this restricted
integer/ASCII schema. This is a versioned encoding, not a claim of general RFC 8785
conformance. SHA-256 authenticates no identity by itself; the detached signature
and pinned witness identity provide that binding.

Copy only `verify.py` and install cryptography to verify without either runtime
or producer. The verifier independently checks structure, signature, external
key/epoch/request/version pins, exact bounded arithmetic and payload. Successful
verification inserts `(witness, ledger, transaction)` into its own SQLite database
in a serialized durable transaction **before** reporting acceptance. Repeated
receipts are rejected in-process and after restart. Rejected inputs do not consume
a valid receipt. Initialization is explicit and refuses an existing state path;
missing/corrupt/unavailable replay state fails closed. A read-only cryptographic
audit can use `checked`; it does not claim freshness or consume anything.

The verifier's state stores the entire pinned trust configuration and rejects a
changed configuration. Different verifiers can each accept a receipt once; global
freshness, a recent-time guarantee and current authorization are not provided.
An old authorized committed effect remains a historical authorized effect even
after revocation. Signature verification alone cannot tell whether storage later
lost its committed result.

## SQLite guarantees relied upon

One local SQLite database per role, rollback journal (`DELETE`),
`synchronous=EXTRA`, `fullfsync=ON` (effective on supporting platforms),
`foreign_keys=ON` for witness, ten-second busy timeout. The code checks the journal
and synchronous settings and runs `integrity_check` on open. A busy/error path
fails rather than authorizing. Integrity checking is not detection of every
semantically valid malicious SQL mutation.

Atomic commit binds request consumption, protected ledger payload and receipt
outbox. Primary keys enforce one effect per transaction. EXTRA requests journal
and directory synchronization; actual durability depends on SQLite VFS, filesystem,
OS and storage honoring flush/locking semantics. No network filesystems, multiple
attached DB transaction, ordinary file-write atomicity or tested power-loss claim.
Process-exit tests exercise rollback-journal restart recovery, not a hardware
crash simulator. macOS fullfsync and Windows locking implications are covered by
native CI configuration and observations, not by renaming platform failures.

References: [SQLite synchronous](https://www.sqlite.org/pragma.html#pragma_synchronous),
[atomic commit assumptions](https://www.sqlite.org/atomiccommit.html),
[Ed25519 API](https://cryptography.io/en/46.0.3/hazmat/primitives/asymmetric/ed25519/).
