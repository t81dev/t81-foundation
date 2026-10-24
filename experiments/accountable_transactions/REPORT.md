# Research report: durable witnessed transactions

Status: bounded research result with committed-source local evidence. Native CI
observations are recorded below. No architectural winner is established.

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

Measured source `6bd2633f726bb160e9e83df8314a5fdee5d7ec64`, clean tree.
Native macOS ARM64, Python 3.14.3, SQLite 3.51.3; exact platform and source/file
hashes are in [measurements.json](observed-macos-arm64/measurements.json).
Pinned dependencies: cryptography 46.0.3, Wasmtime 38.0.0, cffi 2.0.0,
pycparser 2.23, importlib_resources 6.5.2. Compiler/build logs and exact commands
are preserved in the [manifest](observed-macos-arm64/manifest.json).

- 48 two-arm checks pass: 44 within the declared boundary and four excluded
  negative controls. No in-model fabrication/replay acceptance observed.
- 30 cold calculator and 30 steady transaction samples per arm, plus one cold
  witness reserve/commit pair and one cold restart retrieval per arm. No sample
  is dropped. The tiny cold-witness/recovery sample count is descriptive only.
- Standalone copy of only verify.py succeeds once, then rejects replay in a new
  process, with a Python import guard forbidding all runtime/producer imports.
  [Raw commands/results](observed-macos-arm64/detached-verification.json).
- [Aggregate test outcomes](observed-macos-arm64/tests-results.json),
  [raw subprocess evidence](observed-macos-arm64/tests-raw.json) and
  [full public fixtures](observed-macos-arm64/public-tests.tar.gz) retain attacks,
  caller pins, producer outputs and actual payload observations. Exported archive
  file bytes were checked against source artifacts; no private keys/databases.
  Digests detect changes, not independent identity of the research publication.

Steady medians, microseconds (30 samples each):

| Interval | A: T81VM | B: Wasmtime |
| --- | ---: | ---: |
| Preliminary authorization + durable reserve | 9680.75 | 9075.50 |
| Final authorization predicate only | 1.56 | 2.21 |
| Protected commit, including signing and flush | 9643.54 | 10924.27 |
| Receipt encoding/signature creation | 505.08 | 514.27 |
| Independent verification + durable consumption | 11968.65 | 12166.35 |
| Calculator round trip (different IPC paths) | 192.44 | 650.08 |
| Native runtime invocation | 35.67 | 254.00 |

Cold calculator process medians: A 5.43 ms, B 69.99 ms (30 samples each).
Single cold witness reserve/commit processes: A 128.98/132.57 ms,
B 128.47/129.80 ms. Single restart retrieval: A 143.38 ms, B 141.05 ms.
The common durable witness dominates this small local workload; sequential arm
measurement and filesystem noise preclude interpreting differences as runtime
security or general throughput advantages.

Each measured witness DB is 53,248 bytes after 31 transactions; each verifier DB
is 32,768 bytes. A executable is 1,398,976 bytes. Installed wheel file inventory:
cryptography 22,582,958; cffi 1,045,015; pycparser 1,874,701;
Wasmtime 22,470,793; importlib_resources 289,807 bytes. These counts include wheel
files/metadata and are not process RSS, full source LOC or total deployment size.

Introduced nonblank physical code lines, including comments: A calculator 43,
B calculator 35; shared witness 194 and standalone verifier 160. Test harness
331, measurement 137, public exporter 20. Existing runtime and crypto/SQLite
implementation complexity remains additional; shared additions are the same
for both arms. No full-stack LOC or maintenance-cost ratio is claimed.

[Native run 40049128010](https://github.com/t81dev/t81-foundation/actions/runs/40049128010)
passes Linux x86_64, macOS ARM64 and Windows x86_64, including all 48 checks
and ten cold/steady samples per arm. Source head is the measured `6bd2633f`;
GitHub PR jobs run its synthetic merge with main. This is narrow experiment
coverage, not a full production suite or a CanonFS Windows fix.

The unrelated broad determinism workflow fails building unchanged production
`src/direct_api/t81_direct_api.cpp` / `controlled_ai_backend.hpp` (incomplete
ControlledRandomness/PolicyEngine types). These paths match the base revision;
no experiment file is implicated. They overlap #536's documented correctness
risks. No #536 merge/dependency or production edit is made to hide this failure.
Full failure details and native receipt audits are recorded with final CI evidence.

The initial committed-source test archive above precedes the additional
receipt-completeness exporter/auditor. The later native artifacts additionally
retain every committed transaction's signed outbox and independently compare
it with observed payload bytes, preserving the raw-bypass and corruption controls.


## Receipt-complete follow-up

Source `6ea606587c1488ee21da39433785da5d75baffaf`, clean tree, repeats all 48 checks
and 30 cold/steady samples per arm with unchanged witness/calculator/verifier code.
[Receipt-complete public evidence](observed-receipt-audit/public-evidence.tar.gz),
[measurements](observed-receipt-audit/measurements.json) and
[independent audit](observed-receipt-audit/independent-audit.json) are preserved.
The independent auditor verifies 26 signed outboxes and their actual observed
payloads, exposing two intentional raw bypasses and two intentional corruptions.
The later validation-only completeness check also rejects a fabricated observation
that removes an effect while retaining its valid signed receipt; the candidate
and rejection are [preserved](observed-receipt-audit/missing-effect-attack.json).
Read-only artifact auditing makes no fresh receipt-consumption claim.

[Broader production CI failure evidence](observed-receipt-audit/existing-production-ci-failure.json)
preserves both platforms' errors and matching base/head file SHA-256 digests.
This includes ModelFile optional-style API calls as well as incomplete backend
types. These existing production paths need a separate correctness change; this
isolated experiment neither depends on #536 nor declares overall PR CI green.

A further [shared-owner check](observed-receipt-audit/shared-authority-cross-arm.json)
submits A then B, and B then A, to the **same** ledger/key/policy owner. Both first
submissions commit; both cross-arm repeats are rejected. The two effects each
have independently consumed receipts. This supplements the 48 per-arm checks and
confirms that state is shared across calculator implementations, not only reused
protocol code with separate stores.


## Independently audited native publication

[Run 40049452341](https://github.com/t81dev/t81-foundation/actions/runs/40049452341)
passes all native jobs at head `6ea606587c1488ee21da39433785da5d75baffaf`.
GitHub's measured synthetic merge is `516fca507281d38d860fc5f960e83d82a4e8b6ee`.
Linux x86_64 (SQLite 3.45.1), macOS ARM64 (SQLite 3.50.4) and Windows x86_64
(SQLite 3.50.4) each pass 48 checks and publish ten cold/steady samples per arm.
Actual SQLite versions vary with Python/platform; their exact versions, dependencies,
raw outcomes and timings are preserved, not treated as one pinned SQLite build.

The downloaded [Linux](observed-native/linux.tar.gz),
[macOS](observed-native/macos.tar.gz) and [Windows](observed-native/windows.tar.gz)
public trees were independently audited locally with the strengthened auditor.
Each verifies 26 signed outboxes against pinned test requests and observed payloads,
with exactly two raw bypasses, two corrupt-payload rejections and four accepted
excluded controls. All archived bytes were compared with the downloaded files.
[Audit/source checks](observed-native/independent-audit.json),
[GitHub artifact metadata](observed-native/github-artifacts.json),
[run/job provenance](observed-native/run.json) and
[archive hashes](observed-native/sha256.json) retain the evidence beyond CI retention.
The repackaged archive hashes are distinct from GitHub's original ZIP digests.

The initial literal source-byte equality check **failed on Windows**: its fourteen
raw source hashes match CRLF checkout of the pinned LF Git blobs exactly. Linux
and macOS match the literal Git blobs. This [negative observation](observed-native/source-byte-comparison-negative-result.json)
is preserved; no raw hash is silently normalized. It is not a protocol-byte failure:
the independent verifier enforces ASCII LF payload/signature encoding in every
receipt on every platform. Random keys/UUIDs mean entire receipt bytes are not
expected to agree between independent trials. No general cross-platform source
byte identity, power-loss test or production CanonFS Windows compatibility is claimed.

The final validation-only auditor addition checks the converse link as well:
every signed committed outbox must have its observed effect, not only every effect
must have an outbox. It passes all three downloaded native trees and rejects the
preserved missing-effect attack. Formatting and determinism/overclaim guards pass
for this experiment. The broader unchanged-production determinism build remains
red as separately recorded; this PR stays draft and is not automatically merged.
