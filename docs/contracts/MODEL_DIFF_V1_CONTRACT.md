# Model Diff v1 Contract

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Model Diff v1 Contract](#model-diff-v1-contract)
  - [Stable v1 Surface](#stable-v1-surface)
    - [Supported operation](#supported-operation)
    - [Schema identifier](#schema-identifier)
  - [Result Documents](#result-documents)
    - [Success result](#success-result)
    - [Failure result](#failure-result)
  - [Field Semantics](#field-semantics)
    - [Top-level fields](#top-level-fields)
    - [Array fields](#array-fields)
  - [Comparison Semantics](#comparison-semantics)
  - [Stability Boundary](#stability-boundary)

<!-- T81-TOC:END -->


This document is the authoritative contributor-facing contract for the current
JSON surface emitted by:

- `t81 model diff <lhs> <rhs> --json`

It defines what callers may rely on when comparing either live model
artifacts or persisted `t81.model-manifest.v1` records.

## Stable v1 Surface

### Supported operation

- `t81 model diff <lhs> <rhs> --json`

### Schema identifier

- Result schema: `t81.model-diff.v1`

Reference JSON Schema:

- `spec/rfcs/RFC-00D3-model-diff-result-schema.json`

## Result Documents

### Success result

Stable top-level fields:

- `schema`
- `lhs`
- `rhs`
- `identical`
- `lhs_format`
- `rhs_format`
- `lhs_tensor_count`
- `rhs_tensor_count`
- `lhs_parameters`
- `rhs_parameters`
- `lhs_trits`
- `rhs_trits`
- `lhs_only_count`
- `rhs_only_count`
- `changed_count`
- `provenance_lhs_only_count`
- `provenance_rhs_only_count`
- `provenance_changed_count`
- `lhs_only`
- `rhs_only`
- `changed`
- `provenance_lhs_only`
- `provenance_rhs_only`
- `provenance_changed`

Stable success values:

- `schema` must equal `t81.model-diff.v1`

### Failure result

Stable top-level fields:

- `schema`
- `lhs`
- `rhs`
- `error`

Stable failure values:

- `schema` must equal `t81.model-diff.v1`

The exact `error` text is not stable in v1. Callers may rely only on the
presence of a human-readable error string.

## Field Semantics

### Top-level fields

- `lhs`
  - the left-hand input path string after CLI resolution
- `rhs`
  - the right-hand input path string after CLI resolution
- `identical`
  - `true` when the imported views match under the current comparison rules
- `lhs_format`
  - descriptive summary of the imported left-hand artifact or manifest subject
- `rhs_format`
  - descriptive summary of the imported right-hand artifact or manifest subject
- `lhs_tensor_count`
  - tensor count on the left-hand side
- `rhs_tensor_count`
  - tensor count on the right-hand side
- `lhs_parameters`
  - total imported element count on the left-hand side
- `rhs_parameters`
  - total imported element count on the right-hand side
- `lhs_trits`
  - total imported trit count on the left-hand side
- `rhs_trits`
  - total imported trit count on the right-hand side
- `lhs_only_count`, `rhs_only_count`, `changed_count`
  - aggregate counts for the corresponding tensor-difference arrays
- `provenance_lhs_only_count`, `provenance_rhs_only_count`, `provenance_changed_count`
  - aggregate counts for the corresponding provenance-difference arrays
- `lhs_only`
  - tensor names present only on the left-hand side
- `rhs_only`
  - tensor names present only on the right-hand side
- `changed`
  - tensor names present on both sides but differing under the current comparison rules
- `provenance_lhs_only`
  - provenance keys present only on the left-hand side
- `provenance_rhs_only`
  - provenance keys present only on the right-hand side
- `provenance_changed`
  - provenance keys present on both sides but carrying different values

### Array fields

`lhs_only`, `rhs_only`, and `changed` are arrays of tensor-name strings.

`provenance_lhs_only`, `provenance_rhs_only`, and `provenance_changed` are
arrays of provenance-key strings.

## Comparison Semantics

`identical` is currently derived from:

- same imported format summary
- same aggregate parameter and trit counts
- same tensor count
- no left-only tensors
- no right-only tensors
- no changed tensors

The current tensor comparison considers:

- tensor name
- tensor shape
- per-tensor trit count
- sparsity only when both sides carry a known sparsity value

This allows persisted manifests, which intentionally omit guaranteed
per-tensor sparsity, to compare cleanly against live imported artifacts.

The current result also reports provenance-key deltas, but those deltas do not
change `identical` in v1.

Important v1 limitation:

- the comparison is representation-sensitive
- it does not currently normalize tensor layout conventions across formats
- it does not attempt cross-format semantic equivalence

For example, a `.gguf` produced from a `safetensors` source may still compare
as changed if the imported tensor shapes or orientation conventions differ,
even when aggregate parameter and trit counts match.

Companion normalized-mode contract:

- `docs/contracts/MODEL_DIFF_NORMALIZED_V1_CONTRACT.md`

## Stability Boundary

Guaranteed stable in v1:

- the existence of `t81 model diff --json`
- schema id `t81.model-diff.v1`
- the stable field names listed above
- `lhs_only`, `rhs_only`, and `changed` as string arrays

Not yet frozen in v1:

- exact `lhs_format` / `rhs_format` string values
- exact `error` text
- ordering guarantees within the difference arrays beyond the current output
- future comparison dimensions beyond the ones listed above
- any normalization-aware or semantic-equivalence comparison mode

If this contract expands, prefer adding fields rather than renaming or
redefining the stable ones listed here.
