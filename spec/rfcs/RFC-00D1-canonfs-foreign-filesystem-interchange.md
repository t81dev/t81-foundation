# RFC-00D1: CanonFS Foreign File System Interchange

**Status:** draft
**Type:** standards-track
**Applies-To:** CanonFS import/export tooling, foreign filesystem interchange boundary, provenance and manifesting rules
**Created:** 2026-03-26
**Updated:** 2026-03-26
**Author:** @t81dev
**Discussion:** initial scope draft

---

## 1. Summary

This RFC defines the architectural boundary between CanonFS and foreign file
systems.

Its scope is broader than ingestion alone. It covers:

- import from foreign file systems into CanonFS
- export from CanonFS into foreign file systems
- the provenance, normalization, and failure semantics required for both

This RFC treats CanonFS as the authoritative internal representation and foreign
file systems as interchange surfaces. It does not yet standardize a
bidirectional live-sync or mount-through model; those may be considered later
as follow-on work once the import/export contract is stable.

Initial machine-readable schema artifacts for the v1 JSON surfaces now live
alongside this RFC:

- `RFC-00D1-canonfs-import-result-schema.json`
- `RFC-00D1-canonfs-export-result-schema.json`
- `RFC-00D1-canonfs-interchange-manifest-schema.json`
- `RFC-00D1-canonfs-import-provenance-schema.json`
- `RFC-00D1-canonfs-export-provenance-schema.json`

## 2. Motivation

CanonFS already serves as the project's canonical storage identity layer. But a
practical system cannot live entirely inside its own storage format.

T81 still needs to:

- bring files in from host file systems and artifacts produced elsewhere
- materialize CanonFS content back out into ordinary file systems
- preserve enough provenance and structure that import/export activity remains
  auditable

If this boundary is left implicit, the project will accumulate conflicting
behaviors around:

- what import means
- whether export is lossless
- how metadata is normalized
- whether round-tripping is expected to preserve identity
- what provenance must be retained

That ambiguity would weaken both determinism claims and user expectations.

## 3. Goals

- Define CanonFS import and export as first-class, governed interchange
  operations
- Make CanonFS the authoritative internal representation while still supporting
  practical exchange with foreign file systems
- Define what metadata, naming, and provenance are preserved, normalized,
  synthesized, or dropped at the boundary
- Distinguish one-way import/export guarantees from round-trip guarantees
- Leave room for future bridge/sync work without conflating it with v1

## 4. Non-Goals

- Defining a live bidirectional synchronization protocol in this RFC
- Treating foreign file systems as co-equal sources of canonical identity
- Freezing every possible foreign filesystem metadata model in v1
- Promising lossless round-trip behavior for filesystems that cannot express
  CanonFS-native semantics

## 5. Architectural Position

### 5.1 CanonFS Is Authoritative

This RFC treats CanonFS as the canonical internal storage model.

That means:

- imported content is normalized into CanonFS-owned identity and object rules
- exported content is a projection or materialization of CanonFS state into a
  foreign environment
- foreign file system metadata does not automatically become CanonFS identity

This is not an anti-interop position. It is a contract-clarity position.

### 5.2 Interchange Modes

This RFC recognizes three conceptual operation classes:

1. import
   - foreign filesystem -> CanonFS
2. export
   - CanonFS -> foreign filesystem
3. live bridge or sync
   - deferred from v1

Only the first two are standardized here.

### 5.3 Losslessness Must Be Explicit

Import and export must not be described as "lossless" without naming exactly
what is preserved.

This RFC distinguishes:

- content-preserving
- metadata-preserving
- identity-preserving
- provenance-preserving
- round-trip-preserving

These are different claims and must not be conflated.

## 6. Import Contract

### 6.1 Import Meaning

An import operation takes bytes and selected metadata from a foreign source and
produces CanonFS objects plus CanonFS-side provenance records.

Import is not a promise that the source object's original path, inode, or host
filesystem metadata becomes CanonFS identity.

Import does mean:

- CanonFS receives the content
- CanonFS computes CanonFS-native identity
- CanonFS records the foreign source context as provenance
- the resulting object is governed by CanonFS rules after admission

### 6.2 Import Inputs

The v1 import boundary should be defined broadly enough to allow later support
for multiple source kinds, including:

- host file paths
- directory trees
- archive files
- disk images
- remote object fetch results

The exact source set may expand later, but all imports should pass through the
same conceptual pipeline:

1. source discovery
2. byte acquisition
3. normalization
4. CanonFS object creation
5. provenance recording
6. manifest/report emission

#### 6.2.1 Mandatory v1 Import Kinds

This RFC adopts a narrow mandatory v1 import set.

The required import source kinds for v1 should be:

1. host file path
   - import a single file from a host filesystem path into CanonFS
2. host directory tree
   - import a bounded directory subtree from a host filesystem path into
     CanonFS

These two source kinds are enough to make CanonFS practically useful in normal
development and operator workflows without forcing the first implementation to
solve every interchange format at once.

The following source kinds are explicitly deferred from mandatory v1 support:

- archive files
- disk images
- remote object fetch results

Those may still appear later as optional extensions, but they should not be
required before the RFC advances beyond `proposed`.

### 6.3 Import Normalization

Import normalization should define:

- which metadata fields are ignored
- which metadata fields are retained as provenance
- whether executable bits or similar mode hints are preserved
- how timestamps are handled
- how path separators, case rules, and naming collisions are handled

The key rule is that CanonFS identity must be derived from canonical import
bytes and CanonFS-defined rules, not from mutable host-specific storage state.

### 6.4 Import Output

A successful import should produce:

- one or more CanonFS object references
- a manifest or report describing what was imported
- provenance linking the CanonFS result to the foreign source

Illustrative import result shape:

```text
schema: t81.canonfs-import.v1
source_kind: host-path
source_ref: /tmp/model.bin
imported_objects:
  - canonfs:K81A2M4...
provenance_ref: canonfs:P81R7...
status: ok
```

#### 6.4.1 `t81.canonfs-import.v1` Result Schema Direction

This RFC now defines a first concrete result schema direction for import
operations.

The initial reporting format should be UTF-8 JSON with an explicit top-level
`schema` field, consistent with other versioned T81 tool surfaces.

Required fields:

```text
schema: t81.canonfs-import.v1
status: ok | partial | error
source_kind: host-file | host-directory
source_ref: <string>
imported_objects: <array of canonfs refs>
provenance_ref: <canonfs ref>
```

Optional fields:

```text
manifest_ref: <canonfs ref>
imported_paths: <array of source-relative paths>
warnings: <array of strings>
errors: <array of structured error entries>
policy_result: allowed | denied | partial
normalization_summary: <object>
```

Illustrative JSON document:

```json
{
  "schema": "t81.canonfs-import.v1",
  "status": "ok",
  "source_kind": "host-file",
  "source_ref": "/tmp/model.bin",
  "imported_objects": ["canonfs:K81A2M4..."],
  "provenance_ref": "canonfs:P81R7...",
  "manifest_ref": "canonfs:M81F2...",
  "imported_paths": ["model.bin"],
  "warnings": [],
  "errors": [],
  "policy_result": "allowed",
  "normalization_summary": {
    "timestamps": "provenance-only",
    "ownership": "provenance-only",
    "mode_hint": "preserved"
  }
}
```

Field rules:

- `status`
  - required
  - `ok` means the requested import completed without recordable failure
  - `partial` means at least one requested unit succeeded and at least one
    failed or was skipped
  - `error` means no requested unit was successfully imported
- `source_kind`
  - required
  - initial allowed values: `host-file`, `host-directory`
- `source_ref`
  - required
  - the operator-facing source reference used for the operation
- `imported_objects`
  - required array
  - may be empty only when `status=error`
- `provenance_ref`
  - required
  - identifies the CanonFS-side provenance record for the import
- `manifest_ref`
  - optional but strongly preferred for multi-object imports
- `errors`
  - optional structured entries describing failures or skipped units
- `normalization_summary`
  - optional summary of important normalization decisions applied during import

### 6.4.2 Import Error Entry Shape

When `status=partial` or `status=error`, `errors` should contain structured
entries rather than only raw strings.

Illustrative entry shape:

```text
kind: source-failure | normalization-failure | policy-failure
path: <optional source-relative path>
message: <string>
code: <stable short code>
```

## 7. Export Contract

### 7.1 Export Meaning

An export operation takes CanonFS objects and materializes them into a foreign
filesystem representation.

Export is not identity transfer. It is a projection of CanonFS-governed content
into an environment that may not support CanonFS-native semantics directly.

### 7.2 Export Modes

The RFC direction is to recognize at least two export styles:

1. materialized copy
   - write bytes and selected metadata into a foreign target path
2. snapshot/export bundle
   - produce a portable artifact plus manifest describing the exported set

Live mounted views or writable bridges are deferred.

#### 7.2.1 Mandatory v1 Export Kinds

This RFC adopts a narrow mandatory v1 export set.

The required export target kinds for v1 should be:

1. host file path
   - export one CanonFS object to a specific host filesystem file path
2. host directory tree
   - export one or more CanonFS objects into a host filesystem directory tree

Bundle export is intentionally left open for later decision, but it is not
required for the initial mandatory surface.

This means the first implementation can focus on direct, inspectable material
export rather than solving both ordinary filesystem materialization and portable
archive packaging in the same milestone.

### 7.3 Export Guarantees

Export should explicitly document which properties are preserved:

- content bytes
- selected metadata fields
- CanonFS object references in accompanying manifests
- provenance linking the export back to CanonFS origin objects

Export should also document what may be dropped or synthesized:

- foreign filesystem ownership fields
- timestamp fields if CanonFS does not treat them as canonical
- permission encodings that do not map directly
- filesystem-native extended attributes unless explicitly supported

### 7.4 Export Output

A successful export should produce:

- foreign materialized files or export bundles
- a manifest/report describing what was exported
- provenance linking exported artifacts to source CanonFS object references

Illustrative export result shape:

```text
schema: t81.canonfs-export.v1
source_objects:
  - canonfs:K81A2M4...
target_kind: host-path
target_ref: /tmp/exported-model.bin
provenance_ref: canonfs:E81X3...
status: ok
```

#### 7.4.1 `t81.canonfs-export.v1` Result Schema Direction

This RFC now defines a first concrete result schema direction for export
operations.

The initial reporting format should be UTF-8 JSON with an explicit top-level
`schema` field.

Required fields:

```text
schema: t81.canonfs-export.v1
status: ok | partial | error
source_objects: <array of canonfs refs>
target_kind: host-file | host-directory
target_ref: <string>
provenance_ref: <canonfs ref>
```

Optional fields:

```text
manifest_ref: <canonfs ref>
materialized_paths: <array of target-relative paths>
warnings: <array of strings>
errors: <array of structured error entries>
policy_result: allowed | denied | partial
materialization_summary: <object>
```

Illustrative JSON document:

```json
{
  "schema": "t81.canonfs-export.v1",
  "status": "ok",
  "source_objects": ["canonfs:K81A2M4..."],
  "target_kind": "host-file",
  "target_ref": "/tmp/exported-model.bin",
  "provenance_ref": "canonfs:E81X3...",
  "manifest_ref": "canonfs:X81M8...",
  "materialized_paths": ["exported-model.bin"],
  "warnings": [],
  "errors": [],
  "policy_result": "allowed",
  "materialization_summary": {
    "timestamps": "not-restored",
    "ownership": "synthesized",
    "mode_hint": "preserved"
  }
}
```

Field rules:

- `status`
  - required
  - `ok` means all requested export units were materialized successfully
  - `partial` means at least one requested export unit succeeded and at least
    one failed or was skipped
  - `error` means no requested export unit was materialized successfully
- `source_objects`
  - required array of CanonFS references
- `target_kind`
  - required
  - initial allowed values: `host-file`, `host-directory`
- `target_ref`
  - required
  - the operator-facing export destination reference
- `provenance_ref`
  - required
  - identifies the CanonFS-side provenance record for the export
- `materialized_paths`
  - optional summary of created or updated target-relative paths
- `materialization_summary`
  - optional summary of metadata restoration or synthesis decisions

### 7.4.2 Export Error Entry Shape

When `status=partial` or `status=error`, `errors` should contain structured
entries.

Illustrative entry shape:

```text
kind: target-failure | policy-failure | materialization-failure
path: <optional target-relative path>
source_object: <optional canonfs ref>
message: <string>
code: <stable short code>
```

## 8. Round-Trip Semantics

Round-trip behavior must be defined carefully.

This RFC does not assume that:

- import followed by export
- or export followed by import

will preserve every foreign metadata field or textual naming detail.

The preferred v1 claim is narrower:

- content should be preservable where no transformation is requested
- CanonFS provenance should remain available
- CanonFS object identity should remain stable inside CanonFS

Round-trip preservation of foreign metadata should be described only for fields
that are explicitly standardized.

## 9. Metadata Preservation Matrix

This RFC should explicitly classify metadata at the interchange boundary instead
of leaving preservation behavior implicit.

The v1 direction is to sort metadata into four classes:

1. preserved as operational metadata
2. preserved as provenance only
3. synthesized on export
4. dropped unless a later extension standardizes them

### 9.1 Proposed v1 Classification

| Metadata Field | Import to CanonFS | Export from CanonFS | Notes |
| :--- | :--- | :--- | :--- |
| content bytes | preserved | preserved | Primary interoperability guarantee |
| relative path/name | provenance only | synthesized or preserved if export target names are explicit | Path should not become CanonFS identity by default |
| file/directory kind | preserved where supported | preserved where target supports it | Object classification may still be CanonFS-native internally |
| executable bit / mode hint | preserved as metadata/provenance | preserved where target supports it | Exact host permission model is not guaranteed |
| owner/group ids | provenance only | synthesized or dropped | Host ownership is not canonical inside CanonFS |
| modification timestamps | provenance only by default | synthesized, dropped, or optionally restored | Timestamps are high risk for nondeterministic drift |
| creation/access timestamps | dropped or provenance only | synthesized or dropped | Not reliable as canonical state |
| symlink target | deferred / open question for v1 | deferred / open question for v1 | Needs explicit contract rather than accidental support |
| extended attributes | dropped unless standardized later | dropped unless standardized later | Too platform-specific for implicit support |
| ACLs / permission lists | provenance only unless later standardized | synthesized or dropped | Must not be claimed as round-trip-safe without an explicit mapping |
| source filesystem ids/inodes | provenance only | dropped | Never CanonFS identity material |
| CanonFS object reference | created on import | provenance-preserved on export | Central CanonFS-side identity surface |
| import/export manifest refs | created | created | Required for audit and traceability |

This matrix is directional rather than final, but it establishes the intended
contract posture: preserve content strongly, preserve provenance explicitly,
and avoid overclaiming host-metadata fidelity.

### 9.2 Path and Naming Rules

Path handling needs explicit boundaries because foreign file systems often carry
platform-specific semantics that CanonFS should not absorb blindly.

The v1 direction is:

- import may record source-relative paths as provenance
- CanonFS object identity must not depend on source absolute paths
- export may materialize explicit target paths chosen by the operator or tool
- path separator normalization and collision handling must be deterministic

This prevents host-environment quirks from becoming implicit CanonFS identity
inputs.

### 9.3 Timestamp Rules

Timestamps deserve explicit treatment because they are one of the easiest ways
for import/export tooling to become nondeterministic without anyone noticing.

The preferred v1 rule is:

- timestamps are provenance by default, not canonical identity
- import should not require preserving host timestamps as CanonFS-native
  semantic state
- export may restore timestamps only if the tool/operator explicitly requests
  it and the behavior is documented as non-identity metadata

This keeps timestamp handling useful without letting it silently affect CanonFS
identity claims.

### 9.4 Ownership and Permission Rules

Foreign ownership and permission systems are not portable enough to be treated
as implicitly round-trip-safe.

The preferred v1 rule is:

- ownership ids are provenance only
- simple executable/read-only hints may be preserved where meaningful
- richer ACL or host-specific permission structures are not guaranteed in v1

If future work wants stronger permission round-tripping, it should be added as
an explicit extension rather than inferred from ad hoc behavior.

## 10. Provenance and Governance

Import/export operations should be auditable.

At minimum, provenance records should be able to answer:

- what source or destination was involved
- what CanonFS object references were created or exported
- what normalization or materialization rules were applied
- whether policy affected the operation
- whether the operation was partial, complete, or failed

The preferred direction is to integrate this evidence with existing Axion and
CanonFS governance surfaces rather than invent a one-off import/export log.

## 11. Failure Semantics

This RFC should distinguish at least four failure classes:

1. source failure
   - source path missing, unreadable, malformed, or unavailable
2. normalization failure
   - imported data cannot be accepted under CanonFS or policy rules
3. target failure
   - export destination unavailable, unwritable, or incompatible
4. partial-transfer failure
   - some objects or files succeeded before the operation halted

Partial success must be visible in reports and provenance. Silent partial
success is forbidden.

## 12. Import/Export API Direction

This RFC does not freeze exact CLI or library signatures, but it does define
the conceptual surface:

- `canonfs import ...`
- `canonfs export ...`

Both operations should produce structured results with schema ids, status, and
reference/provenance information.

The likely v1 direction is JSON-capable reporting with explicit schema fields,
consistent with other T81 tooling surfaces.

### 12.1 Initial Schema Artifacts

The current draft now has RFC-scoped JSON Schema companions for the first
concrete v1 document surfaces:

- import result: [RFC-00D1-canonfs-import-result-schema.json](RFC-00D1-canonfs-import-result-schema.json)
- export result: [RFC-00D1-canonfs-export-result-schema.json](RFC-00D1-canonfs-export-result-schema.json)
- interchange manifest: [RFC-00D1-canonfs-interchange-manifest-schema.json](RFC-00D1-canonfs-interchange-manifest-schema.json)
- import provenance: [RFC-00D1-canonfs-import-provenance-schema.json](RFC-00D1-canonfs-import-provenance-schema.json)
- export provenance: [RFC-00D1-canonfs-export-provenance-schema.json](RFC-00D1-canonfs-export-provenance-schema.json)

These schema artifacts are intentionally narrow. They freeze the current JSON
shape used by the RFC-00D1 CLI seed without claiming that policy integration or
Axion evidence linkage is finished.

## 13. Open Questions

1. Should symlinks be included in v1, and if so under what normalization rules?
2. Should import/export reports be JSON-first only, or dual-surface with a
   compact text projection from the start?
3. Should archive/bundle export be part of v1, or should v1 only support direct
   filesystem materialization?
4. Should any read-only bridge mode be mentioned as an experimental follow-on,
   or kept entirely out of the first RFC?
5. Should the RFC-00D1 schema files remain RFC-scoped artifacts, or move into a
   broader stable schema catalog once the interchange surface reaches
   `proposed`?

## 14. Impact

### 14.1 Compatibility

This RFC is additive. It does not redefine CanonFS object identity. It defines
the boundary conditions for moving data into and out of CanonFS in a governed
way.

### 14.2 Complexity

This is more work than ad hoc "copy a file in" tooling because it requires:

- normalization rules
- provenance capture
- manifest/report generation
- explicit failure semantics

That complexity is justified if CanonFS is meant to be a serious canonical
storage layer rather than an internal blob cache with undefined edges.

### 14.3 Governance Value

A real interchange contract strengthens:

- reproducibility
- auditability
- policy enforcement before storage-side effects
- user expectations around what import and export actually mean

## 15. Alternatives Considered

### 15.1 Import-Only RFC

Rejected.

That would leave export semantics undefined and would split one conceptual
boundary into two incomplete documents.

### 15.2 Treat Foreign Filesystems as Co-Equal Identity Sources

Rejected.

That would weaken CanonFS's role as the canonical internal storage model and
make provenance and normalization harder to reason about.

### 15.3 Promise Full Round-Trip Losslessness

Rejected.

Different filesystems expose different metadata and naming semantics. The RFC
should standardize only what can be defended precisely.

## 16. Next Steps

Before this RFC should move from `draft` to `proposed`, follow-on work should
settle:

- the provenance/evidence schema integration beyond the initial RFC-scoped JSON
  schemas
- whether bundle export is in v1
- whether symlinks are in or out for v1
- whether a compact text projection ships alongside the JSON result schemas
- whether the RFC-scoped schema files should be promoted into a stable shared
  schema catalog

## 17. References

- RFC-0054: CanonFS Object Identity and Persistence Contract
- RFC-00CF: Slice6 CanonFS Operator Actions
- RFC-00D0: Base-81-Aware TCP/IP Stack
- [RFC-00D1-canonfs-import-result-schema.json](RFC-00D1-canonfs-import-result-schema.json)
- [RFC-00D1-canonfs-export-result-schema.json](RFC-00D1-canonfs-export-result-schema.json)
- [RFC-00D1-canonfs-interchange-manifest-schema.json](RFC-00D1-canonfs-interchange-manifest-schema.json)
- [RFC-00D1-canonfs-import-provenance-schema.json](RFC-00D1-canonfs-import-provenance-schema.json)
- [RFC-00D1-canonfs-export-provenance-schema.json](RFC-00D1-canonfs-export-provenance-schema.json)
