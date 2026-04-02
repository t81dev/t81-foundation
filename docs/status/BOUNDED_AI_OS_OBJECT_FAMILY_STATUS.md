# Bounded AI OS-Object Family Status

This note summarizes the current bounded deterministic AI OS-object family in
one maintainer-facing place.

It does not add new rules. It gathers the current baseline, invariant,
admission gate, and negative-path posture into one status surface.

## What Is In The Family

The current admitted bounded family is:

- `assess-fixed` -> host-action decision
- `route-fixed` -> path selection
- `classify-fixed` -> canonical rule-set selection

Each member uses the same object model:

1. task result artifact
2. provenance artifact
3. downstream record
4. canonical bundle

Object roles are fixed:

- task result artifact: intermediate
- provenance artifact: intermediate
- downstream record: intermediate
- bundle: canonical top-level object

## What Is Proven

The current family is proven to satisfy the Canonical Identity Invariant for
the admitted chains:

- identical task, model, policy, and input produce identical:
  - result artifact identity and bytes
  - provenance artifact identity and bytes where checked
  - downstream record identity and bytes
  - canonical bundle identity and bytes
- CanonFS root does not affect those identities for the admitted chains

## What Admits A New Member

A new composition only joins this family if it satisfies the admission gate in:

- [AI_OS_OBJECT_FAMILY_ADMISSION_CONTRACT.md](../reference/AI_OS_OBJECT_FAMILY_ADMISSION_CONTRACT.md)

In practice that means:

- existing bounded AI task family only
- same result/provenance/record/bundle law
- no new runtime lane
- no new helper family
- no path-based identity
- no rule execution in the composition layer
- runnable example
- portable composition test
- cross-root identity proof
- catalog update
- minimal CLI/helper contract coverage

## What Is Frozen

The current rollback/reference anchor is:

- [BOUNDED_AI_OS_OBJECT_FAMILY_BASELINE_2026_04.md](../releases/BOUNDED_AI_OS_OBJECT_FAMILY_BASELINE_2026_04.md)

Recommended baseline tag:

- `baseline-bounded-ai-os-object-family-2026-04-02`

## What Is Enforced Negatively

The family now has explicit negative-path enforcement for malformed or
unsupported use, including:

- unsupported downstream schemas
- missing required fixed fields
- malformed CanonFS refs in required record or bundle fields
- bounded task denial paths in the shared task framework

The family is therefore governed on both sides:

- valid members have an admission bar
- invalid or malformed uses fail closed

## What Is Explicitly Deferred

This family is not currently:

- a workflow engine
- a general packaging system
- a rule execution layer
- an orchestration layer
- a generalized AI runtime surface
- evidence that future compositions inherit the invariant automatically

## Current Posture

The next work on this family should be:

- stabilization
- misuse-path hardening
- portability and CI boringness
- keeping the baseline, invariant, admission gate, and catalog aligned

The next work on this family should not be:

- a fourth composition by default
- a new helper family
- rule execution in the composition layer
- orchestration or workflow expansion
- broader AI runtime claims

## Reference Surfaces

- canonical chain explainer:
  - [FIRST_DETERMINISTIC_AI_OS_OBJECT_CHAIN.md](../explanation/FIRST_DETERMINISTIC_AI_OS_OBJECT_CHAIN.md)
- bounded family catalog:
  - [AI_OS_OBJECT_CHAIN_CATALOG.md](../reference/AI_OS_OBJECT_CHAIN_CATALOG.md)
- shared framework contract:
  - [AI_TASK_SHARED_FRAMEWORK_CONTRACT.md](../reference/AI_TASK_SHARED_FRAMEWORK_CONTRACT.md)
- family admission gate:
  - [AI_OS_OBJECT_FAMILY_ADMISSION_CONTRACT.md](../reference/AI_OS_OBJECT_FAMILY_ADMISSION_CONTRACT.md)
- baseline anchor:
  - [BOUNDED_AI_OS_OBJECT_FAMILY_BASELINE_2026_04.md](../releases/BOUNDED_AI_OS_OBJECT_FAMILY_BASELINE_2026_04.md)
