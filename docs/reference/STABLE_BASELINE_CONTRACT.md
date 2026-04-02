# Stable Baseline Contract

This document defines the current stable baseline contract for the bounded
deterministic AI OS-object family in T81.

It is not a product announcement.
It is a freeze document for the current governed execution baseline.

## Purpose

Use this contract as:

- the narrowest statement of what is frozen today
- a rollback/reference anchor for the current bounded family
- a guard against accidental contract drift while future work continues

## Scope

This contract applies only to the current admitted bounded AI OS-object family:

- `assess-fixed`
- `route-fixed`
- `classify-fixed`

It does not freeze the broader AI runtime, decode evolution, orchestration, or
longer-horizon OS direction.

## Frozen Laws

The following are the current stable laws of the baseline.

### 1. Policy Before Side Effects

Execution or materialization must be subject to policy evaluation before side
effects occur.

For the current baseline:

- deny means the attempted governed action does not proceed
- allow means the action may proceed into the deterministic bounded path

### 2. Deny Means No Artifact Mutation For The Denied Action

If policy denies the bounded action being attempted, that action must not
mutate the target artifact state for that denied path.

This contract freezes the fail-closed posture, not every future policy
language feature.

### 3. Deterministic Result Identity

For identical task, model, policy, and input, the admitted family must produce
the same canonical object identities.

The identity model excludes:

- machine location
- filesystem path
- host-local environment

### 4. CanonFS-Backed Provenance And Artifact Identity

The current baseline freezes:

- CanonFS-backed content-addressed artifact identity
- provenance as a first-class persisted artifact in the admitted chain
- canonical refs as the stable linkage mechanism between chain objects

### 5. Fixed Object Law

Each admitted family member must preserve this exact object model:

1. task result artifact
2. provenance artifact
3. downstream record
4. canonical bundle

Object roles are fixed:

- task result artifact: intermediate
- provenance artifact: intermediate
- downstream record: intermediate
- bundle: canonical top-level object

### 6. Canonical Bundle Law

The canonical top-level object for the admitted family remains the bundle.

For the current baseline, the bundle field set is frozen as:

1. `schema`
2. `source_result_ref`
3. `source_provenance_ref`
4. `action_ref`
5. `record_ref`

The narrow `.v1` versioning boundary for this bundle surface is defined in:

- [AI_OS_OBJECT_BUNDLE_VERSIONING_BOUNDARY.md](./AI_OS_OBJECT_BUNDLE_VERSIONING_BOUNDARY.md)

## Admitted Family Only

This stable baseline includes only:

- `assess-fixed`
- `route-fixed`
- `classify-fixed`

It does not imply:

- a fourth composition
- automatic admission of future compositions
- general bundle compatibility beyond the admitted family

## What Is Guaranteed

For the current admitted family, the baseline guarantees:

- policy-gated bounded execution
- canonical result, provenance, record, and bundle objects
- root-independent canonical identity for identical task/model/policy/input
- bundle-first consumption through the current family contract
- fail-closed validation for malformed or unsupported bundle/record usage

## What Is Explicitly Excluded

This stable baseline does not include:

- broader inference runtime claims
- greedy decode or continuation guarantees
- multi-step reasoning/runtime composition
- orchestration or workflow engines
- rule execution in the composition layer
- generalized bundle platforms or schema registries
- bare-metal or guest OS direction
- hardware/interposer/backend convergence work

## Reference Surfaces Frozen With This Baseline

- [BOUNDED_AI_OS_OBJECT_FAMILY_BASELINE_2026_04.md](../releases/BOUNDED_AI_OS_OBJECT_FAMILY_BASELINE_2026_04.md)
- [AI_OS_OBJECT_FAMILY_ADMISSION_CONTRACT.md](./AI_OS_OBJECT_FAMILY_ADMISSION_CONTRACT.md)
- [AI_OS_OBJECT_BUNDLE_CONSUMPTION_CONTRACT.md](./AI_OS_OBJECT_BUNDLE_CONSUMPTION_CONTRACT.md)
- [AI_OS_OBJECT_BUNDLE_VERSIONING_BOUNDARY.md](./AI_OS_OBJECT_BUNDLE_VERSIONING_BOUNDARY.md)
- [BOUNDED_AI_OS_OBJECT_FAMILY_STATUS.md](../status/BOUNDED_AI_OS_OBJECT_FAMILY_STATUS.md)

If future work changes one of the frozen laws above, it should not continue to
be described as part of this stable baseline without an explicit contract
update.
