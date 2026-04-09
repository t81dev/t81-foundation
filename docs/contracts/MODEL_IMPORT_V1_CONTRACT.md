# Model Import v1 Contract

This document is the authoritative contributor-facing contract for the current
`t81 model import --json` surface.

It defines what callers may rely on today without reading the CLI
implementation. Anything not listed here as stable is still prototype-shaped
and may change as the model-artifact intake wedge is refined.

## Stable v1 Surface

### Supported operation

- `t81 model import <file> --json`

This is the only `model` family operation covered by this contract.

### Schema identifier

- Result schema: `t81.model-import.v1`

Reference JSON Schema:

- `spec/rfcs/RFC-00D3-model-import-result-schema.json`
- Adjacent manifest schema: `spec/rfcs/RFC-00D3-model-manifest-schema.json`

### Supported input format labels

These values are accepted by `--format` and may also appear in
`source_format`:

- `safetensors`
- `bitnet`
- `gguf`

If `--format` is omitted, the CLI infers the format from the input extension.

## Result Documents

### Success result

Stable top-level fields:

- `schema`
- `ok`
- `input`
- `source_format`
- `artifact_format`
- `tensor_count`
- `parameters`
- `trits`
- `file_size_bytes`
- `provenance`
- `tensors`

Stable success values:

- `schema` must equal `t81.model-import.v1`
- `ok` must equal `true`

### Failure result

Stable top-level fields:

- `schema`
- `ok`
- `input`
- `error`

Stable failure values:

- `schema` must equal `t81.model-import.v1`
- `ok` must equal `false`

The exact `error` text is not stable in v1. Callers may rely only on the
presence of a human-readable error string.

## Field Semantics

### Top-level fields

- `input`
  - the path string passed to the command after resolution
- `source_format`
  - the resolved input-format label
- `artifact_format`
  - descriptive loader summary string
- `tensor_count`
  - number of tensor entries in `tensors`
- `parameters`
  - total imported element count
- `trits`
  - total imported trit count represented by the current import
- `file_size_bytes`
  - source artifact size in bytes when known
- `provenance`
  - string map of import-side provenance values
- `tensors`
  - ordered tensor inventory emitted by the loader

### Tensor entries

Each entry in `tensors` has these stable fields:

- `name`
- `shape`
- `trits`
- `sparsity`

Field meanings:

- `name`
  - imported tensor name
- `shape`
  - tensor shape as an array of dimension sizes
- `trits`
  - imported trit count for that tensor
- `sparsity`
  - zero-density fraction in the imported native representation

## Provenance Map

The `provenance` object is a string-to-string map.

Currently observed keys include:

- `source_path`
- `source_sha3_512`
- `bridge_backend`
- `bridge_revision`

Only the object shape is stable in v1. Specific keys are format-dependent and
may expand over time.

## Stability Boundary

Guaranteed stable in v1:

- the existence of `t81 model import --json`
- schema id `t81.model-import.v1`
- success/failure split via `ok`
- the stable field names listed above
- tensor-entry field names
- `provenance` as a string map

Not yet frozen in v1:

- exact `artifact_format` string values
- exact `error` text
- exact `provenance` key set
- ordering guarantees beyond the current emitted tensor inventory

## Contributor Expectations

The current `model import` surface is intentionally narrow:

- it is a review-oriented intake/report command
- it reuses the existing model loaders
- it does not yet write CanonFS manifests or canonical import records

If this contract expands, prefer adding fields rather than renaming or
redefining the stable ones listed here.

## Adjacent Persisted Artifact

`t81 model import --manifest <file>` now emits a narrower persisted record
with schema `t81.model-manifest.v1`.

That manifest is intentionally outside this v1 import-report contract:

- it is meant for review, storage, and later comparison
- it omits some loader-report detail such as guaranteed `sparsity`
- it is accepted by `t81 model diff` alongside live model artifacts

Treat the manifest as a related but separate surface from
`t81.model-import.v1`.

Authoritative manifest contract:

- `docs/contracts/MODEL_MANIFEST_V1_CONTRACT.md`
