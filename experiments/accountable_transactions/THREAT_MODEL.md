# Threat model and pre-implementation decision

Decision recorded before prototype implementation, 2026-10-23. Baseline main:
`0005ab675d2729b2795200e8c78ded43d4520b0d`; reference #554:
`7af837ece1325752e555b814c94aacc33afd2c96`. No merge or dependency on #554/#536.

## Findings from existing evidence

Read the Research Charter, accountable-computation framework, RFC-00D4,
RFC-0054, RFC-00D1/00D0, deterministic_provenance README/report, #554 adapters,
workload, attacks, tests, archive auditor, report and preserved CI summaries.
#554's detached verifier accepts invented correct histories, invented observer
inventories and repeated valid envelopes. Linux/macOS archive agreement establishes
byte reproducibility under an honest host, not occurrence. Windows protected-store
failures remain a separate filename compatibility issue. The latest #554 checks
also include failing cross-platform determinism jobs; do not inherit a green-CI claim.

`fs/persistent_driver.cpp::write_object` hashes bytes and calls its Axion hook
before mutation; ordinary object/capability/evidence files are not one atomic,
power-loss-durable transaction. `set_axion_hook` does not supply cross-process
freshness. RFC-0054 defines payload identity and permission masks, not authenticated
occurrence. Deterministic provenance recomputes bounded arithmetic; unsigned
histories remain fabricable. RFC-00D4 signatures need an external identity pin and
cannot turn producer assertions into observed effects.

#536 was inspected only for relevant risks: integer arithmetic UB and divergent
backend definitions are reasons to use bounded positive multiplication and avoid
AI/backend/direct API paths. No cherry-pick, merge or dependency on its fixes.

## Attacker and trusted parties

The producer may fabricate all histories, invent hashes/keys, change amounts,
replay messages/receipts, race another process and crash at any checkpoint. Both
runtime arms are untrusted request calculators. Neither gets signing authority.
The trusted witness owns the SQLite purchase-order ledger, current policy,
transaction IDs, signing key and receipt outbox. It computes the exact admitted
payload itself. The effect is insertion of that payload into this ledger, not
CanonFS archival and not transmission to a supplier. Direct file writes elsewhere
are not ledger effects. The witness never signs a claimed external file inventory.

Deployment requires OS/process isolation: producer cannot open the witness DB,
private key, admin policy API, or verifier state. This local research harness runs
roles under one UID for reproducibility; it exercises API separation, **not an OS
security sandbox**. Same-UID/administrator access can bypass it. Preserve this
negative control rather than claiming protection against a malicious host.
Witness/OS/compiler/SQLite/crypto compromise, storage rollback/cloning, key theft,
network authentication, distributed witnesses, hardware failure, malicious storage,
and truth of business facts are excluded. Availability is not guaranteed.

Verifier trust configuration (key, ledger epoch and policy versions) and its own
consumed-receipt DB arrive out of band. Losing/rolling back that DB re-enables
replay. Independent verifiers with independent DBs may each accept once; this is
local durable consumption, not worldwide freshness or current authorization.

## Smallest defensible design

Use Ed25519 (cryptography), SHA-256 and SQLite, shared unchanged by both arms.
Reserve a UUID transaction durably. At final admission, serialize against policy
updates using BEGIN IMMEDIATE, validate current version and amount, then atomically
insert effect, signed receipt and consumed state in **one database**. Never publish
a signature before commit returns. No ordinary multi-file atomicity assumption.
An interrupted pending transaction is explicitly aborted by recovery; a committed
one exposes its identical stored receipt without executing again. Failure to open
or validate witness/verifier storage fails closed. No production interface change
is necessary; the witness ledger replaces #554's local file effect, a deliberate
scope change that does not make CanonFS itself transactional.

This experiment tests durable local purchase-order issuance, not arbitrary external
effects. Coupling a supplier API or production CanonFS write would need a different
commit protocol and may have an uncertain state. Stop before changing those APIs.
