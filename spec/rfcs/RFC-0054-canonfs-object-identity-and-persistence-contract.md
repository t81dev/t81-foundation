# RFC-0054: CanonFS Object Identity and Persistence Contract

- **RFC-ID:** RFC-0054
- **Title:** CanonFS Object Identity and Persistence Contract
- **Status:** accepted
- **Type:** standards-track
- **Applies-To:** CanonFS object identity, persistent-driver guarantees, capability persistence, parity semantics, CanonFS CLI/user-visible naming
- **Created:** 2026-03-19
- **Updated:** 2026-03-22
- **Supersedes:** None
- **Superseded-By:** None
- **Discussion:** Reconciles `spec/supplemental/canonfs-spec.md` with `include/t81/canonfs/canon_driver.hpp`, `include/t81/canonfs/canon_types.hpp`, `src/canonfs/persistent_driver.cpp`, and `src/canonfs/in_memory_driver.cpp`

---

## Summary

This RFC defines the minimum CanonFS contract that the T81 implementation must satisfy and that user-facing surfaces are allowed to claim.

Its purpose is to close current drift between the CanonFS supplemental spec and the actual drivers. The reconciliation focuses on:

- the canonical object identity function
- whether `ObjectType` participates in object identity
- what the persistent driver actually guarantees on disk
- what capability persistence means in the current implementation
- whether parity is normative or optional in the persistent driver
- what hash/reference names user-facing tooling may expose

This RFC does not attempt to redesign CanonFS from first principles. It narrows ambiguity so CanonFS claims become auditable.

## Motivation

CanonFS already has normative text in [canonfs-spec.md](../supplemental/canonfs-spec.md), but several important details do not currently match the implementation:

- the spec defines `CanonHash-81` using a BLAKE3-derived construction
- the implementation computes object hashes through `t81::hash::hash_bytes(...)`
- the spec defines object identity as `hash(TypeID || SerializedPayload)`
- both current drivers hash payload bytes only and ignore the `ObjectType` parameter
- the spec presents a rich signed capability object, while the persistent driver stores only permission masks
- the spec presents parity and self-healing as protocol-level behavior, while the persistent driver currently treats `parity/` as a placeholder directory
- user-facing CLI surfaces often expose `sha3-256:<hash>`-style strings rather than CanonFS-specific terminology

Without a reconciliation RFC, CanonFS risks having:

- normative text that overstates the real durable contract
- user-visible naming that does not match the storage model
- future migrations with no declared compatibility boundary
- deterministic claims that cannot be defended precisely

## Proposal

### 1. CanonFS Must Define One Canonical Object Identity Contract

CanonFS MUST define exactly one object identity function for the current release line.

That contract MUST answer, unambiguously:

1. what bytes are hashed
2. whether `ObjectType` is part of identity
3. what string form identifies an object in APIs and CLIs
4. what compatibility rules apply if the identity function changes later

No CanonFS surface may describe object identity using one rule while implementing another.

### 2. Current Release Baseline Must Match the Existing Drivers

Until a migration is explicitly implemented, the current CanonFS baseline MUST reflect the behavior of the shipping drivers in:

- [canon_driver.hpp](/include/t81/canonfs/canon_driver.hpp)
- [canon_types.hpp](/include/t81/canonfs/canon_types.hpp)
- [persistent_driver.cpp](/src/canonfs/persistent_driver.cpp)
- [in_memory_driver.cpp](/src/canonfs/in_memory_driver.cpp)

That means the release baseline must explicitly choose one of these paths:

1. declare the current payload-only identity rule normative for the current release and defer `TypeID`-prefixed identity to a later migration
2. implement `TypeID || payload` hashing immediately and provide a migration rule for already-addressed objects

This RFC does not choose between those two paths by implication. It requires the project to choose one explicitly and document it normatively.

### 3. ObjectType Semantics Must Be Resolved Explicitly

The current driver interface exposes:

```cpp
write_object(ObjectType type, std::span<const std::byte> bytes)
```

but both current drivers ignore `type` for object identity.

RFC-0054 requires one of the following:

- `ObjectType` is semantically part of CanonFS identity, in which case the drivers and tests MUST be updated to hash `TypeID || SerializedPayload`
- `ObjectType` is metadata only for the current release, in which case the supplemental spec MUST stop claiming that type is part of identity

Leaving `ObjectType` in the API while leaving its identity role undefined is forbidden.

### 4. Persistent CanonFS Must Define Its Real Durable Contract

The persistent driver currently provides a narrow but concrete durable contract:

- content-addressed blob persistence under `objects/<hash>.blk`
- optional capability-mask files under `caps/<hash>.cap`
- deterministic read-back verification by recomputing content identity
- Axion hook invocation before persistent mutations and reads

RFC-0054 requires the persistent-driver contract to be specified as such, rather than implied from a broader future-facing CanonFS design.

The persistent driver MUST state:

1. whether on-disk objects are raw payload blobs or typed CanonObject envelopes
2. whether object identity is computed before or after any future compression/encryption layers
3. whether object storage is guaranteed to be immutable once written
4. which directories and files are normative parts of the on-disk contract
5. which features are placeholders rather than implemented guarantees

### 5. Capability Semantics Must Distinguish Current Guarantees from Future Design

The persistent driver currently stores decimal permission masks, not full signed capability records.

Therefore the normative contract must distinguish between:

- the current implemented capability persistence model
- the richer future capability object model described in the supplemental spec

For the current release line:

- if signed capability objects are not persisted or verified end-to-end, the spec MUST NOT describe them as current driver guarantees
- if the richer model remains desired, it MUST be marked as future work or versioned extension material

### 6. Parity and Self-Healing Must Be Classified Correctly

Parity/self-healing semantics are currently asymmetric across implementations:

- the persistent driver creates a `parity/` directory but does not implement durable parity shard writing or full repair
- the in-memory path contains stronger parity-related logic

RFC-0054 requires parity to be classified explicitly as one of:

1. mandatory current CanonFS behavior
2. optional implementation extension
3. future versioned feature not yet guaranteed by the persistent reference driver

The persistent driver MUST NOT be described as self-healing in the strong sense unless it actually persists and consumes parity data accordingly.

### 7. User-Facing Hash Naming Must Be Canonical

The project must choose a canonical user-visible reference vocabulary for CanonFS objects.

Allowed outcomes include:

- `canonfs:<hash>`
- `canonhash81:<hash>`
- another explicitly standardized CanonFS prefix

If generic hash prefixes such as `sha3-256:<hash>` remain in CLI output, they MUST only be used if they are in fact the canonical CanonFS identity format for the release.

CanonFS documentation, CLI help text, examples, and tests MUST use one canonical naming rule.

### 8. Compatibility and Migration Rule

If CanonFS identity changes in any release after this RFC, the project MUST define:

1. an object-version discriminator or equivalent migration boundary
2. whether old and new object identities can coexist
3. whether read compatibility is required
4. whether CLI tools will expose both schemes during migration

Silent identity changes are forbidden.

## Determinism / Safety Considerations

Determinism considerations:

- object identity must be derived from canonical bytes, not host paths or mutable metadata
- read verification must recompute the same canonical identity function every time
- user-facing references must not alias multiple identity schemes ambiguously

Safety considerations:

- mismatched identity rules create type-confusion and provenance risks
- overstating capability persistence weakens the governance story
- claiming self-healing without durable parity semantics creates recovery expectations the reference driver cannot satisfy

## Compatibility

RFC-0054 is primarily a reconciliation RFC.

It may result in either:

- spec narrowing, where the current implementation becomes the explicit current-release contract
- implementation migration, where drivers are changed to satisfy the richer current supplemental spec

Compatibility must be preserved explicitly either way. Existing persisted objects and user-visible object references cannot be invalidated by undocumented behavior changes.

## Implementation Plan

1. Decide the canonical CanonFS object identity rule for the current release line.
2. Update [canonfs-spec.md](../supplemental/canonfs-spec.md) so it matches the chosen identity and persistence contract.
3. If `ObjectType` is normative for identity, update both drivers and all affected tests and tools.
4. Classify capability persistence and parity semantics as current, optional, or future-versioned.
5. Standardize CanonFS hash/reference naming across CLI help, tests, and docs.
6. Add regression tests proving the chosen identity and persistence contract.

## Open Questions

- Should CanonFS v1 standardize the current `hash_bytes(payload)` behavior, or should it migrate to `hash(TypeID || SerializedPayload)` now?
- Is the current `CanonHash-81` name supposed to denote a specific algorithm, or a stable project-defined identity function independent of the historical BLAKE3 wording?
- Should the persistent driver remain a minimal reference driver, or should it be upgraded to match the richer capability and parity model?
- Does the project want one CanonFS object-reference prefix, or a generic hash-prefix scheme shared with non-CanonFS artifact surfaces?

## Acceptance Criteria

- The normative CanonFS text defines exactly one current-release object identity rule.
- The role of `ObjectType` in object identity is explicit and matches the implementation.
- The persistent-driver on-disk contract is described without placeholder features being presented as current guarantees.
- Capability persistence semantics are classified accurately as implemented or future work.
- Parity/self-healing semantics are classified accurately as implemented, optional, or future work.
- CanonFS user-visible reference naming is standardized across CLI/docs/tests.
- Regression tests exist for the chosen identity rule and persistent-driver verification behavior.

## Implementation Record (2026-03-22)

All acceptance criteria are satisfied as of this date.  The chosen path is **option 1**:
the current `CanonHash-81(payload_bytes)` rule is declared normative for the current release
line; TypeID-prefixed identity is deferred to a future explicit migration.

**AC1 — Normative CanonFS text defines exactly one current-release object identity rule:**
`spec/supplemental/canonfs-spec.md` v0.4.2 §2 Architectural Invariant 2 states:
"Current-release object identity is `CanonHash-81(payload_bytes)`."  §4 ("CanonHash-81 —
Current Release Identity Function") defines the function as `t81::hash::hash_bytes(...)` over
payload bytes.  §5 codifies the chosen path: "Future typed-envelope CanonFS versions MAY define
identities over `TypeID || SerializedPayload`, but they MUST do so under an explicit
compatibility or migration contract."  One rule, one release line, one section.

**AC2 — Role of ObjectType explicit and matches implementation:**
`spec/supplemental/canonfs-spec.md §5` states: "The T81 CanonFS API carries an `ObjectType`,
but the current drivers do not include `ObjectType` in object identity.  In the current release
line, `ObjectType` is descriptive API metadata rather than part of the hashed persistent
identity."  `include/t81/canonfs/canon_types.hpp` confirms: `CanonBlock::hash()` calls
`t81::hash::hash_bytes(raw)` over the tryte payload with no type prefix.
`tests/cpp/canonfs_identity_contract_test.cpp` asserts that writing the same payload with
`ObjectType::RawBlock` and `ObjectType::CanonTensor` produces identical hashes in both the
in-memory and persistent drivers.

**AC3 — Persistent-driver on-disk contract described without placeholder overclaim:**
`spec/supplemental/canonfs-spec.md §7.1` documents the exact on-disk layout: objects at
`objects/<hash>.blk`, capabilities at `caps/<hash>.cap` as decimal permission masks, and
`parity/` as a placeholder directory.  §7.1 explicitly states: "The driver currently treats
`parity/` as a placeholder directory for future repair shards."  §2 Invariant 5 ("Extension
Honesty") forbids describing parity, typed envelopes, or compression-aware canonicalization
as current persistent-driver guarantees unless implemented end to end.

**AC4 — Capability persistence classified accurately:**
`spec/supplemental/canonfs-spec.md §6.1` ("Current Persistent-Driver Capability Contract")
specifies the current guarantee: per-object permission mask at `caps/<hash>.cap`, with
`target`, `permissions`, bootstrap access, and read/write enforcement.  §6.2 explicitly
labels the full `CapabilityGrant` signed structure as "a future-versioned CanonFS object
model, not a current persistent-driver guarantee."

**AC5 — Parity/self-healing classified accurately:**
`spec/supplemental/canonfs-spec.md §8` ("CanonParity — Optional / Future-Versioned
Recovery") states: "CanonFS parity support is not a current persistent-driver guarantee"
and "Any CanonParity wire format or repair workflow described below is future-versioned
until the persistent reference driver implements it end to end."  This accurately describes
the gap: the in-memory path may have stronger parity logic, while the persistent reference
driver has only the `parity/` placeholder.

**AC6 — CanonFS user-visible reference naming standardized:**
`spec/supplemental/canonfs-spec.md §4` documents the current CLI naming convention:
"User-facing CanonFS and related CLI surfaces currently expose CanonFS object references
as `sha3-256:<hash>`" and states that changing the identity function requires an explicit
migration plan per RFC-0054 §8.  The naming is standardized as `sha3-256:<hash>` for
the current release, with RFC-0054 §7 defining the compatibility rule if the prefix changes.

**AC7 — Regression tests exist for chosen identity rule and persistent-driver verification:**
`tests/cpp/canonfs_identity_contract_test.cpp` verifies: (a) identical payloads with
different ObjectTypes produce identical hashes in both drivers (payload-only rule), and
(b) the test covers both `make_in_memory_driver()` and `make_persistent_driver()`.
`tests/cpp/canonfs_read_verify_test.cpp` and `tests/cpp/canonfs_persistent_driver_test.cpp`
provide complementary verification of the read-path identity recomputation (`CanonHash-81`
recomputed on read and compared to the stored reference, failing deterministically on mismatch).
