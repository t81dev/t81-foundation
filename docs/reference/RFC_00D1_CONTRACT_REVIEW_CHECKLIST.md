# RFC-00D1 Contract Review Checklist

Use this checklist when changing the current RFC-00D1 build-against surface.

It is a maintenance checklist for the existing seed contract, not a promotion
tool for widening the interchange surface.

## Use This For Changes To

- `canonfs import`
- `canonfs export`
- RFC-00D1 JSON result, provenance, or manifest schemas
- the checked-in CanonFS interchange example
- the frozen `v1/` fixture set
- CLI contract tests covering the current interchange seed

## The Current Build-Against Surfaces Must Stay Aligned

Review all of the following together:

1. [RFC-00D1-canonfs-foreign-filesystem-interchange.md](../../spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md)
2. [examples/storage-and-canonfs/canonfs-interchange/README.md](../../examples/storage-and-canonfs/canonfs-interchange/README.md)
3. [examples/storage-and-canonfs/canonfs-interchange/v1/README.md](../../examples/storage-and-canonfs/canonfs-interchange/v1/README.md)
4. frozen `v1/` request and output fixtures in
   `examples/storage-and-canonfs/canonfs-interchange/v1/`
5. [tests/cpp/cli_contract_test.cpp](../../tests/cpp/cli_contract_test.cpp)

If one of these changes, check whether the others now drift.

## Treat These As Breaking For The Current Seed Contract

The current build-against surface should be treated as broken unless the change
is intentionally promoted and documented across all review surfaces above.

Breaking areas include:

- schema ids:
  - `t81.canonfs-import.v1`
  - `t81.canonfs-export.v1`
  - `t81.canonfs-import-provenance.v1`
  - `t81.canonfs-export-provenance.v1`
  - `t81.canonfs-interchange-manifest.v1`
- source or target kinds:
  - `host-file`
  - `host-directory`
- linkage fields:
  - `provenance_schema`
  - `manifest_schema`
- structured error entry field set:
  - `kind`
  - `message`
  - `code`
  - `reason`
- structured error entry order in emitted JSON:
  - `kind`
  - `message`
  - `code`
  - `reason`
- built-in policy-profile names or their narrow admission semantics:
  - `permissive`
  - `import-only`
  - `export-only`
  - `deny-all`

## Keep These Questions Explicitly Deferred

Do not let these become accidentally “settled” through examples or code drift:

- symlink posture
- archive or bundle export
- text output as a co-equal contract
- schema-catalog relocation beyond the RFC-local schema artifacts

If a change touches one of these, narrow the work or update the RFC
intentionally before treating the result as part of the stable seed.

## Minimal Review Questions

Before merging a change in this lane, answer:

1. Does the RFC still describe the current seed contract accurately?
2. Does the main checked-in example still match the current CLI/core behavior?
3. Does the frozen `v1/` fixture set still match the contract tests?
4. Did any schema id, field name, field order, or policy-profile meaning
   drift?
5. Is this tightening clarity, or widening the surface?

If the answer to 5 is “widening,” it should usually stop and be narrowed first.
