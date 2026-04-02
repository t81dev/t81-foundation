# AI OS-Object Bundle Versioning Boundary

This note defines the narrow versioning boundary for the current canonical
bundle in the admitted bounded AI OS-object family.

It does not define a general versioning policy for arbitrary object types in
T81.

## Scope

This boundary applies only to:

- `t81.ai.task.assess-fixed.bundle.v1`
- `t81.ai.task.route-fixed.bundle.v1`
- `t81.ai.task.classify-fixed.bundle.v1`

It exists so a consumer can tell what `.v1` freezes today and what kind of
change would require a new bundle schema version.

## What `.v1` Freezes

For the current admitted family, `.v1` freezes all of the following:

- the role of the bundle as the canonical top-level persisted object
- the exact field set:
  - `schema`
  - `source_result_ref`
  - `source_provenance_ref`
  - `action_ref`
  - `record_ref`
- the meaning of each field as described in
  [AI_OS_OBJECT_BUNDLE_CONSUMPTION_CONTRACT.md](./AI_OS_OBJECT_BUNDLE_CONSUMPTION_CONTRACT.md)
- the requirement that each `*_ref` field be a CanonFS content-addressed
  artifact ref
- the family-level rule that consumers start from the bundle and dereference
  deeper objects only as needed

## Changes That Require A New Bundle Schema Version

The current family must use a new bundle schema version if any of the
following change:

- the bundle stops being the canonical top-level object
- a field is added, removed, or renamed
- the required field order in the canonical rendered object changes
- the meaning of `action_ref` or `record_ref` changes for the current family
- a field stops containing a CanonFS content-addressed artifact ref
- a family member requires a different top-level object law from the current
  fixed bundle law

In other words:

- if a current `.v1` consumer would need new interpretation logic, that change
  is not `.v1` compatible

## Changes That Do Not Require A New Bundle Schema Version

The following do not require a new bundle schema version by themselves:

- additional family-specific meaning in the downstream record
- new admitted family members that still preserve the current fixed bundle law
- new examples, tests, or consumer docs that do not change the bundle object
- stronger validation or misuse-path enforcement that preserves the `.v1`
  object shape and semantics

## What Is Still Outside This Boundary

This note does not freeze:

- a general schema registry outside the current bounded family
- transport or network protocols
- a generalized consumer API
- continuation or multi-step runtime semantics
- compatibility rules for non-family bundle types

## Relationship To Existing Family Docs

Use this note together with:

- [AI_OS_OBJECT_BUNDLE_CONSUMPTION_CONTRACT.md](./AI_OS_OBJECT_BUNDLE_CONSUMPTION_CONTRACT.md)
- [AI_OS_OBJECT_CHAIN_CATALOG.md](./AI_OS_OBJECT_CHAIN_CATALOG.md)
- [AI_OS_OBJECT_FAMILY_ADMISSION_CONTRACT.md](./AI_OS_OBJECT_FAMILY_ADMISSION_CONTRACT.md)

If a future change crosses one of the versioning boundaries above, the bundle
must not continue to be described as `.v1`.
