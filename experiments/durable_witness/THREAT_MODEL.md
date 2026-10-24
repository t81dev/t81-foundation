# Threat model and unresolved boundaries

## Implemented boundary

`execute_runner(..., secret_key=...)` performs runtime execution, calls
`submit_transaction(..., secret_key=...)` in the **same Python process**, and
writes evidence. The CLI also accepts that key in process arguments. Explicit
key injection removes an implicit default; it supplies no isolation.

The T81 C++ subprocess receives an input file and fresh temporary CanonFS store,
not the witness key. The Wasmtime guest receives only an integer submit import.
The Python host nevertheless owns the signing material, witness database,
policies, and evidence-generation code. HMAC verification also requires signing
capability. The trusted host and verifier must therefore remain inside the TCB.

The retained known-key phantom-receipt test demonstrates the consequence:
offline HMAC verification accepts evidence for an unadmitted transaction. Checking
an honestly maintained witness DB rejects that phantom. A malicious host with
both key and DB access can fabricate both; this experiment provides no defense
against that actor. No malicious-host-producer resistance is claimed.

## Smallest proposed isolated interface (deferred)

A separately administered witness would own the key and SQLite file, accepting
only `{tx_id, authority_version, po_data, computed_amount}` through a narrow local
IPC or authenticated request interface. It would validate amount and policy,
atomically record the transaction/effect, and return a receipt or structured
denial. Authority administration would be separate from producer submission.
The producer would receive neither key nor direct DB access; verification would
query that trusted service, or eventually use a public verification key.

A child process under the same unrestricted host identity, handed a key by the
producer, would not establish that boundary. Independent credentials, file
permissions, lifecycle management, authenticated access, and a verifier trust
model require work beyond this corrective experiment. Implementation is deferred
to a separate PR. This deferral is an **unresolved trust-boundary limitation**,
not a completed isolation feature.

## Tested properties and limits

- Native commitments and denials are parsed as structured runtime evidence.
  Missing/malformed/failed runtimes cannot silently execute Python arithmetic.
- Both runtime amounts must match bounded integer PO arithmetic before witness
  writes. This checks a result, not runtime attestation or an external effect.
- HMAC canonical encoding separates the historical `tx:1`/`v2` and
  `tx`/`1:v2` field tuples. Restoring the delimiter signer makes the regression
  fail. Signatures authenticate fields only against actors without the key.
- `BEGIN IMMEDIATE` serializes policy lookup and admission against SQLite
  writers. A revocation committed before admission rejects the transaction.
  An uncontrolled race may legitimately admit before revocation; it does not
  establish ordering by itself.
- Duplicate IDs are rejected in a continuous, honest witness DB. The concurrency
  harness checks process exits, both responses, and permitted race outcomes.
- A trigger aborting transaction insertion after effect insertion leaves both
  tables empty. The former read-only-file probe did not establish this ordering.
- SQLite snapshot restoration permits reuse of previously committed IDs. Both
  pre/post snapshots and both submission attempts are retained. No rollback
  resistance is claimed; SQLite rollback resistance remains unresolved.
- The verifier checks HMAC, payload and amount binding, and (when requested)
  transaction presence in the supplied DB. It does not establish the integrity
  of a hostile DB, runtime identity, or historical revocation state by itself.
- `effect_observed` is separate. The optional caller-supplied audit-log dictionary
  is a test input, not an authenticated external observation channel. No physical
  effect authentication is implemented.

Recorded execution is macOS ARM64. Other OS/architecture executions are not
established by this run; supported multiprocessing contexts are reported
individually, and unavailable contexts are not counted as executed successes.
