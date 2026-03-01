# `spec/companion` (Historical Companion)

This subtree contains long-form narrative material, primarily `t81-spec.md`.

## Status
- `t81-spec.md` is non-normative historical/companion content.
- Normative requirements live in the top-level `spec/*.md` suite.

## Usage
- Use this content for background context and historical rationale.
- Do not treat it as the source of truth for implementation decisions unless a section has been explicitly promoted into top-level specs.

## Promotion Rule
If any section here becomes normative:
1. Copy/port it into the appropriate top-level `spec/*.md` file.
2. Add explicit cross-links in `spec/index.md`.
3. Keep this subtree marked as historical companion text.
