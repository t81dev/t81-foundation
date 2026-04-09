# Model Diff Normalized v1 Contract

This document is the authoritative contributor-facing contract for the current
JSON surface emitted by:

- `t81 model diff <lhs> <rhs> --json --mode normalized`

It defines a narrow, opt-in normalization-aware comparison mode. It does not
change the default raw `t81.model-diff.v1` behavior.

## Stable v1 Surface

### Supported operation

- `t81 model diff <lhs> <rhs> --json --mode normalized`

### Schema identifier

- Result schema: `t81.model-diff-normalized.v1`

Reference JSON Schema:

- `spec/rfcs/RFC-00D3-model-diff-normalized-result-schema.json`

## Result Documents

### Success result

Stable top-level fields:

- `schema`
- `comparison_mode`
- `lhs`
- `rhs`
- `identical`
- `lhs_format`
- `rhs_format`
- `lhs_source_format`
- `rhs_source_format`
- `lhs_tensor_count`
- `rhs_tensor_count`
- `lhs_parameters`
- `rhs_parameters`
- `lhs_trits`
- `rhs_trits`
- `lhs_only`
- `rhs_only`
- `changed`
- `normalized_matches`
- `normalization_rules`
- `provenance_lhs_only`
- `provenance_rhs_only`
- `provenance_changed`
- `normalized_match_reasons`

Stable success values:

- `schema` must equal `t81.model-diff-normalized.v1`
- `comparison_mode` must equal `normalized`

### Failure result

Stable top-level fields:

- `schema`
- `lhs`
- `rhs`
- `error`

Stable failure values:

- `schema` must equal `t81.model-diff-normalized.v1`

The exact `error` text is not stable in v1.

## Semantics

This mode is explicitly normalization-aware and opt-in.

Today it admits one narrow class of normalization:

- known 2D transpose-style shape matching for cross-format pairs in the GGUF
  versus SafeTensors family

The current result fields mean:

- `normalized_matches`
  - tensors that would differ in raw imported representation but were accepted
    by the current normalization rules
- `normalization_rules`
  - rule identifiers used during the comparison
- `normalized_match_reasons`
  - per-tensor explanation map for entries admitted through normalization
  - keys are tensor names
  - values are rule identifiers used for that tensor
- `provenance_lhs_only`, `provenance_rhs_only`, `provenance_changed`
  - provenance-key deltas surfaced as review context
  - these do not currently change `identical`

Important boundary:

- this mode is still experimental
- it does not claim full semantic equivalence across arbitrary formats
- it only applies explicit, named normalization rules

If this contract expands, prefer adding fields or rules rather than silently
changing the meaning of `identical`.
