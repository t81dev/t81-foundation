# PR #560 corrective engineering results

Recorded on 2026-10-24 on macOS ARM64, from reference head
`056f0d6307f59049ef9dfa42c482f727a02c2b96`, on branch
`corrective/pr560-witness`. Corrective source hashes and runtime/dependency details
are in [environment.json](observed-pr560-corrective/environment.json).
This report supersedes the earlier unqualified parity and isolation claims.

## Reproduced before source changes

The original suite ran seven tests: with the fresh native binary explicitly
injected, all passed. The default-path run instead recorded four missing-binary
errors. Neither result exposed the two runtime/admission defects.

The nine initial corrective tests then produced seven failure reports and fourteen
error reports (including subtests). Native over-limit output raised an exception;
a deliberately wrong runtime result could obtain a successful receipt; malformed
Arm B input could be admitted. These logs and the initial regression fixture are
retained. Reconstructing the original modules from the reference Git SHA repeats
the same failure/error counts; see `baseline-reproduced.log` and
[REPRODUCE.md](observed-pr560-corrective/REPRODUCE.md).

## Fixed and tested

| Finding | Correction | Evidence |
| --- | --- | --- |
| Native denial discarded | Retain native JSON status, amount, events and trace; return structured witness denial | Real native 100 × 600 computation yields `runtime.status=denied`, `POLICY_LIMIT_EXCEEDED`, no receipt |
| Authorization parity overstated | Define shared bounded input and authorization outcomes, retain runtime-specific evidence separately | Real T81VM and Wasmtime agree for approved, over-limit, revoked, revoked+over-limit, unknown authority, and malformed inputs |
| Historical HMAC regression used wrong tuple | Correct shifted authority to `1:v2`; compare historical bytes, canonical bytes, signatures and verifier substitution | Old `tx:1:v2:eff1` bytes collide; canonical signatures differ; signed-field substitution fails; restored old signer makes the security test fail |
| Runtime amount ignored | Mandatory `computed_amount`; strict integer equality before witness writes; receipt signs that validated value | Injected wrong results for both adapters and an actual incorrect Wasmtime guest return `COMPUTED_AMOUNT_MISMATCH`; neither witness table gains a row |
| Local denial could be overridden | Native denial is an additional witness admission veto | Injected denial for an otherwise allowed amount yields `RUNTIME_POLICY_DENIED` |
| Evidence amount could contradict receipt | Verifier checks submitted amount against signed amount and PO | Tampered evidence yields `AMOUNT_BINDING_MISMATCH` |
| Replay could overwrite original evidence | Exclusive attempt files; harnesses refuse existing output directories | Original success and later rejection are retained separately |
| Weak partial-write probe | Inject failure between effect and transaction insertion and inspect both tables | `INTEGRITY_ERROR`, zero effects, zero transactions |

Admission also now reads persisted policy limits and starts `BEGIN IMMEDIATE`
before replay/policy lookup, serializing admission with policy writers. Tests
confirm stricter persisted limits deny admission.

The final suite ran **22 tests without failures or skips**; the evidence above,
rather than that count alone, defines the supported result. Negative controls
that mock native output are explicitly fault injections, not native execution.
`cases/` retains real native/Wasmtime envelopes and detached verifier subprocess
outputs. Exactly the two approved cases produced witness transaction/effect rows.

## Attack and measurement observations

The finalized ten-scenario attack run reports nine blocked/handled cases and one
reproduced vulnerability: **restoring an older SQLite snapshot allows transaction
ID reuse**. Original and rolled-back submission evidence and pre/post DB snapshots
are retained in `attacks-final/attack_8/`.

Duplicate submissions in `spawn`, `forkserver`, and `fork` each produced one
success and one rejection. The uncontrolled revocation races are observations of
allowed ordering outcomes, not proof that revocation won every possible schedule.
The sandboxed run could not create forkserver IPC (`Operation not permitted`);
its failure remains in `attacks.log`. A separate unsandboxed run executed all three
contexts. `attacks-first.log`, `attacks-unsandboxed.log`, and earlier test failures
are also preserved.

Thirty approved trials per arm completed with all receipts verified. The final
local mean runner times were about 17.39 ms (A) and 7.87 ms (B); standard deviations
were 9.38 ms and 18.22 ms. These noisy end-to-end timings include asymmetric native
subprocess/temporary CanonFS work and Wasmtime setup, SQLite, and JSON publication.
They are **not a VM performance comparison**. Earlier measurement output is retained
separately. See `performance-final/{raw.jsonl,report.json}` for individual trials.

## Unresolved and deferred

**Signing-key isolation remains unresolved.** The evidence-producing Python host
receives the witness key and can directly access the database. The known-key
phantom receipt test still succeeds offline and is rejected against the honest DB.
Removing a default key did not establish a separate trusted witness. The smallest
proposed service boundary and reasons to defer implementation are in
[THREAT_MODEL.md](THREAT_MODEL.md).

**SQLite rollback resistance remains unresolved.** Authenticated external-effect
observations also remain absent. The native adapter's temporary CanonFS write is not a durable
witness transaction. A witness `effects` row proves only local SQLite state.
These results do not establish broad runtime/security equivalence, malicious-host
resistance, or that custom VM instructions are unnecessary for every accountability
workload.

Only macOS ARM64 was run here. Linux, Windows and other architectures were not
executed; percent-encoded filenames, explicit paths, and existing module-level
multiprocessing targets retain portable implementation choices without claiming
cross-platform validation. No ISA or T81Float changes were made. PR #560 was not
merged. Corrective changes use a separate branch.
