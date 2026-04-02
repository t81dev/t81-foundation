# AI OS-Object Family Admission Contract

This document defines what a new bounded composition must satisfy before it is
treated as a member of the current deterministic AI OS-object family in T81.

It is an admission gate, not a workflow framework.

## Scope

This contract applies only to bounded compositions built from the current
validated family model described in:

- [AI_TASK_SHARED_FRAMEWORK_CONTRACT.md](./AI_TASK_SHARED_FRAMEWORK_CONTRACT.md)
- [AI_OS_OBJECT_CHAIN_CATALOG.md](./AI_OS_OBJECT_CHAIN_CATALOG.md)

It does not authorize:

- new AI runtime lanes
- orchestration
- rule execution engines
- generalized bundle systems
- generalized schema families

## Required Object Shape

A family member must preserve this exact object model:

1. task result artifact
2. provenance artifact
3. downstream record
4. canonical bundle

Object roles are fixed:

- task result artifact: intermediate
- provenance artifact: intermediate
- downstream record: intermediate
- bundle: canonical top-level object

A family member does not qualify if it skips one of these objects or changes
the role of the bundle.

## Allowed Source

A family member must start from an existing bounded AI task in the current
shared task family.

Allowed source tasks today:

- `answer_fixed.v1`
- `classify_fixed.v1`
- `route_fixed.v1`
- `assess_fixed.v1`

Using an existing task does not automatically admit a new composition. The
composition must still satisfy the rest of this contract.

## Downstream Record Rules

The downstream record must be:

- fixed-schema
- deterministic
- narrow
- specific to the new composition

The downstream record may:

- name one closed-world downstream outcome
- bind that outcome to one canonical content-addressed artifact identity
- retain the source result and provenance refs

The downstream record must not:

- execute rules
- introduce a second runtime path
- encode path-based identity as the canonical selected object
- generalize into arbitrary JSON authoring

If a composition needs free-form downstream structure, it is outside this
family.

## Bundle Rules

The bundle must remain:

- fixed-shape
- deterministic
- CanonFS-backed
- the canonical top-level persisted object for the completed chain

The current family bundle law is:

1. `schema`
2. `source_result_ref`
3. `source_provenance_ref`
4. `action_ref`
5. `record_ref`

`action_ref` must remain a canonical content-addressed artifact ref, not a
local path or environment-derived token.

## Forbidden Moves

A candidate composition is not admitted if it requires any of the following:

- a new AI runtime lane
- a new helper family
- orchestration or multi-step workflow machinery
- path-based identity as the selected downstream object
- rule execution inside the composition layer
- schema generalization beyond the specific new slice
- a second canonical top-level object instead of the bundle

## Required Proof Bar

Before a new composition is treated as a family member, it must provide all of
the following:

- one human-readable runnable example
- one portable composition test
- one cross-root identity proof for identical task/model/policy/input
- one catalog update
- one minimal CLI/helper contract slice

The proof must explicitly establish:

- identical `result_ref`
- identical `provenance_ref`
- identical `record_ref`
- identical `bundle_ref`
- identical result, record, and bundle bytes

The composition is not admitted if it only works by convention or maintainer
memory.

## Current Admitted Family

The current admitted bounded family is:

- `assess-fixed` -> host-action decision
- `route-fixed` -> path selection
- `classify-fixed` -> canonical rule-set selection

Each admitted member uses:

- the shared AI task runner
- the current typed artifact helper surface
- the canonical bundle as the top-level object

## Boundary

This contract is intentionally narrow.

It does not say that every future composition should be admitted.
It says what a new composition must preserve if it is to join this family
without widening the system.
