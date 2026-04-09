# CanonFS Interchange v1 Contract

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [CanonFS Interchange v1 Contract](#canonfs-interchange-v1-contract)
  - [Stable v1 Surface](#stable-v1-surface)
    - [Supported operations](#supported-operations)
    - [Schema identifiers](#schema-identifiers)
    - [Supported source and target kinds](#supported-source-and-target-kinds)
    - [Supported policy-profile names](#supported-policy-profile-names)
  - [Result Documents](#result-documents)
    - [Import result](#import-result)
    - [Export result](#export-result)
  - [Structured Error Contract](#structured-error-contract)
    - [Deterministic reason mapping](#deterministic-reason-mapping)
      - [Import reasons](#import-reasons)
      - [Export reasons](#export-reasons)
  - [Result Linkage Fields](#result-linkage-fields)
  - [CLI/Core Alignment](#clicore-alignment)
  - [Guaranteed Stable in v1](#guaranteed-stable-in-v1)
  - [Explicitly Deferred / Not in v1](#explicitly-deferred--not-in-v1)
  - [Canonical Examples](#canonical-examples)

<!-- T81-TOC:END -->


This document is the authoritative contributor-facing contract for the current
CanonFS interchange v1 surface.

It defines what a caller may rely on today without reading implementation
files. Anything not listed here as stable is either deferred or still RFC-00D1
draft material.

## Stable v1 Surface

### Supported operations

- `t81 canonfs import <path> --json`
- `t81 canonfs export <sha3-256:hash> --out <path> --json`

These are the only CanonFS interchange operations covered by this contract.

### Schema identifiers

- Import result: `t81.canonfs-import.v1`
- Export result: `t81.canonfs-export.v1`
- Import provenance: `t81.canonfs-import-provenance.v1`
- Export provenance: `t81.canonfs-export-provenance.v1`
- Interchange manifest: `t81.canonfs-interchange-manifest.v1`

### Supported source and target kinds

- Import `source_kind`
  - `host-file`
  - `host-directory`
- Export `target_kind`
  - `host-file`
  - `host-directory`

### Supported policy-profile names

- `permissive`
- `import-only`
- `export-only`
- `deny-all`

These names are stable and are emitted verbatim in result JSON as
`policy_profile`.

## Result Documents

### Import result

Stable top-level fields:

- `schema`
- `status`
- `source_kind`
- `source_ref`
- `imported_objects`
- `provenance_ref`
- `provenance_schema`
- `manifest_ref`
- `manifest_schema`
- `imported_paths`
- `warnings`
- `errors`
- `policy_result`
- `policy_profile`
- `normalization_summary`

Stable `status` values:

- `ok`
- `partial`
- `error`

Stable `policy_result` values:

- `allowed`
- `partial`
- `denied`

### Export result

Stable top-level fields:

- `schema`
- `status`
- `source_objects`
- `target_kind`
- `target_ref`
- `provenance_ref`
- `provenance_schema`
- `manifest_ref`
- `manifest_schema`
- `materialized_paths`
- `warnings`
- `errors`
- `policy_result`
- `policy_profile`
- `materialization_summary`

Stable `status` values:

- `ok`
- `partial`
- `error`

Stable `policy_result` values:

- `allowed`
- `partial`
- `denied`

## Structured Error Contract

Every CanonFS interchange error entry is a JSON object with these required
fields:

- `kind`
- `code`
- `reason`
- `message`

### Deterministic reason mapping

The mapping from `reason` to `kind` and `code` is part of the v1 contract.

#### Import reasons

| reason | kind | code |
| :--- | :--- | :--- |
| `policy_denied` | `policy-failure` | `canonfs-policy-denied` |
| `missing_source` | `source-failure` | `canonfs-import-missing-source` |
| `unsupported_source_kind` | `source-failure` | `canonfs-import-unsupported-source-kind` |
| `symlink_not_supported` | `normalization-failure` | `canonfs-import-symlink-not-supported` |
| `invalid_policy_profile` | `source-failure` | `canonfs-import-invalid-policy-profile` |
| `invalid_policy_document` | `source-failure` | `canonfs-import-invalid-policy-document` |
| `manifest_write_failed` | `source-failure` | `canonfs-import-manifest-write-failed` |
| `provenance_write_failed` | `source-failure` | `canonfs-import-provenance-write-failed` |
| `storage_write_failed` | `source-failure` | `canonfs-import-storage-write-failed` |

Any undocumented import reason currently falls back to:

- `kind`: `source-failure`
- `code`: `canonfs-import-failure`

That fallback is stable for v1, but callers should not depend on undocumented
reason strings.

#### Export reasons

| reason | kind | code |
| :--- | :--- | :--- |
| `policy_denied` | `policy-failure` | `canonfs-policy-denied` |
| `unsafe_target_path` | `target-failure` | `canonfs-export-unsafe-target-path` |
| `target_directory_create_failed` | `target-failure` | `canonfs-export-target-directory-create-failed` |
| `target_open_failed` | `target-failure` | `canonfs-export-target-open-failed` |
| `target_write_failed` | `target-failure` | `canonfs-export-target-write-failed` |
| `invalid_source_ref` | `materialization-failure` | `canonfs-export-invalid-source-ref` |
| `missing_object` | `materialization-failure` | `canonfs-export-missing-object` |
| `hash_mismatch` | `materialization-failure` | `canonfs-export-hash-mismatch` |
| `invalid_schema` | `materialization-failure` | `canonfs-export-invalid-schema` |
| `malformed_manifest` | `materialization-failure` | `canonfs-export-malformed-manifest` |
| `invalid_policy_profile` | `materialization-failure` | `canonfs-export-invalid-policy-profile` |
| `invalid_policy_document` | `materialization-failure` | `canonfs-export-invalid-policy-document` |
| `provenance_write_failed` | `materialization-failure` | `canonfs-export-provenance-write-failed` |

Any undocumented export reason currently falls back to:

- `kind`: `materialization-failure`
- `code`: `canonfs-export-failure`

That fallback is stable for v1, but callers should not depend on undocumented
reason strings.

## Result Linkage Fields

These linkage fields are part of the v1 contract:

- `provenance_ref`
- `provenance_schema`
- `manifest_ref`
- `manifest_schema`

Stable linkage schema values:

- Import `provenance_schema`: `t81.canonfs-import-provenance.v1`
- Export `provenance_schema`: `t81.canonfs-export-provenance.v1`
- `manifest_schema` when present: `t81.canonfs-interchange-manifest.v1`

Interpretation:

- `provenance_ref` links the result document to the CanonFS-stored provenance
  record for the operation
- `manifest_ref` links multi-entry interchange operations to the CanonFS-stored
  manifest object
- `provenance_schema` and `manifest_schema` make those linked document kinds
  explicit in-band

## CLI/Core Alignment

For `canonfs import` and `canonfs export`, the JSON contract is rendered by the
shared CanonFS interchange layer and validated before the CLI prints it.

Contributor expectation:

- the CLI is a thin wrapper over core interchange behavior
- `reason` values originate from core outcomes
- `kind` and `code` are derived deterministically from `reason`
- undocumented CLI-only CanonFS interchange JSON fields are not part of v1

## Guaranteed Stable in v1

The following are guaranteed stable for the current v1 contract surface:

- `canonfs import` and `canonfs export` JSON result schemas
- the schema identifiers listed above
- `host-file` and `host-directory`
- `permissive`, `import-only`, `export-only`, `deny-all`
- the required structured error fields:
  `kind`, `code`, `reason`, `message`
- the enumerated `reason -> kind/code` mappings in this document
- the linkage fields:
  `provenance_ref`, `provenance_schema`, `manifest_ref`, `manifest_schema`
- deterministic JSON field ordering as emitted by the current renderer

## Explicitly Deferred / Not in v1

The following are not part of this v1 contract:

- live bridge or sync behavior
- networking or resolver work
- new interchange formats beyond the current host file and host directory lane
- new built-in policy profiles
- richer policy language guarantees beyond the currently accepted `.apl` subset
- text output as a co-equal contract surface
- symlink support
- archive or bundle import/export
- broader schema-catalog promotion beyond the current RFC-scoped schema ids
- any AI/runtime, benchmarking, or policy-test expansion outside CanonFS

## Canonical Examples

The canonical byte-for-byte example fixtures for this contract live in:

- [examples/storage-and-canonfs/canonfs-interchange/v1/](../../examples/storage-and-canonfs/canonfs-interchange/v1/)

Those fixtures include:

- successful import
- successful export
- missing object failure
- invalid schema failure
- policy denial failure

They are exercised by the contract tests.
