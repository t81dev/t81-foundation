# Model Manifest v1 Contract

This document is the authoritative contributor-facing contract for the current
persisted manifest emitted by:

- `t81 model import <file> --manifest <out>`

It defines the stable shape of the narrower review/storage artifact used by
`t81 model diff` and related intake workflows.

Companion diff contract:

- `docs/contracts/MODEL_DIFF_V1_CONTRACT.md`

## Stable v1 Surface

### Supported operation

- `t81 model import <file> --manifest <out>`

### Schema identifier

- Manifest schema: `t81.model-manifest.v1`

Reference JSON Schema:

- `spec/rfcs/RFC-00D3-model-manifest-schema.json`

## Manifest Document

Stable top-level fields:

- `schema`
- `source_path`
- `source_format`
- `normalized_artifact_type`
- `imported_format_summary`
- `tensor_count`
- `parameters`
- `trits`
- `file_size_bytes`
- `provenance`
- `tensors`

Stable top-level values:

- `schema` must equal `t81.model-manifest.v1`

## Field Semantics

### Top-level fields

- `source_path`
  - the source path string used for the import
- `source_format`
  - the resolved input-format label
- `normalized_artifact_type`
  - stable subject label for this persisted record
- `imported_format_summary`
  - descriptive summary of how the current loader interpreted the source artifact
- `tensor_count`
  - number of entries in `tensors`
- `parameters`
  - total imported element count
- `trits`
  - total imported trit count represented by the current import
- `file_size_bytes`
  - source artifact size in bytes when known
- `provenance`
  - string map of import-side provenance values
- `tensors`
  - ordered tensor inventory for persisted review/diff workflows

### Tensor entries

Each entry in `tensors` has these stable fields:

- `name`
- `shape`
- `trits`

Field meanings:

- `name`
  - imported tensor name
- `shape`
  - tensor shape as an array of dimension sizes
  - each dimension must be a positive whole number
  - empty shapes are not valid in this v1 manifest
- `trits`
  - imported trit count for that tensor

## Provenance Map

The `provenance` object is a small explicit record.

Stable required key:

- `host_format_reader`

Stable optional keys:

- `source_sha3_512`
- `gguf_version`
- `embedded_source_sha3_512`
- `bridge_backend`
- `bridge_revision`

The current contract does not allow arbitrary extra provenance keys.

## Stability Boundary

Guaranteed stable in v1:

- the existence of `t81 model import --manifest <out>`
- schema id `t81.model-manifest.v1`
- the stable field names listed above
- tensor-entry field names
- `provenance` as a string map

Not yet frozen in v1:

- exact `artifact_format` string values
- exact `imported_format_summary` string values
- ordering guarantees beyond the current emitted tensor inventory
- optional future fields such as hashes, canonical ids, or transformation notes

## Relationship To Other Surfaces

The persisted manifest is intentionally narrower than
`t81.model-import.v1`:

- it is designed for review, storage, and comparison
- it omits fields like guaranteed per-tensor `sparsity`
- it is accepted by `t81 model diff` alongside live model artifacts

Aggregate semantics:

- `tensor_count` must equal the number of `tensors` entries
- top-level `trits` is expected to equal the sum of per-tensor `trits`
- count fields are non-negative whole numbers and may exceed 32-bit range

If this contract expands, prefer adding fields rather than renaming or
redefining the stable ones listed here.
