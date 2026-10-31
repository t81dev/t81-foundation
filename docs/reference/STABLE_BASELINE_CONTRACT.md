# Stable Baseline Contract

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Stable Baseline Contract](#stable-baseline-contract)
  - [Purpose](#purpose)
  - [Contract Enforcement](#contract-enforcement)
  - [Scope](#scope)
  - [Frozen Laws](#frozen-laws)
    - [1. Policy Before Side Effects](#1-policy-before-side-effects)
    - [2. Deny Means No Artifact Mutation For The Denied Action](#2-deny-means-no-artifact-mutation-for-the-denied-action)
    - [3. Deterministic Result Identity](#3-deterministic-result-identity)
  - [Deterministic Identity Clarification](#deterministic-identity-clarification)
    - [4. CanonFS-Backed Provenance And Artifact Identity](#4-canonfs-backed-provenance-and-artifact-identity)
    - [5. Fixed Object Law](#5-fixed-object-law)
    - [6. Canonical Bundle Law](#6-canonical-bundle-law)
  - [Bundle Structural Integrity](#bundle-structural-integrity)
  - [Policy Evaluation Boundary](#policy-evaluation-boundary)
  - [Admitted Family Only](#admitted-family-only)
  - [Admitted Family Freeze Clause](#admitted-family-freeze-clause)
  - [What Is Guaranteed](#what-is-guaranteed)
  - [What Is Explicitly Excluded](#what-is-explicitly-excluded)
  - [Reference Surfaces Frozen With This Baseline](#reference-surfaces-frozen-with-this-baseline)

<!-- T81-TOC:END -->


This document defines the current stable baseline contract for the bounded
deterministic AI OS-object family in T81.

It is not a product announcement.
It is a freeze document for the current governed execution baseline.

## Purpose

Use this contract as:

- the narrowest statement of what is frozen today
- a rollback/reference anchor for the current bounded family
- a guard against accidental contract drift while future work continues

## Contract Enforcement

This contract is normative for the current stable baseline.

If an implementation or change violates any frozen law in this document:

- it must not be described as compliant with this baseline
- it must not be treated as part of the stable baseline without an explicit
  contract update
- it must be gated behind an explicit version or experimental boundary

Expected enforcement surfaces for this baseline include:

- deterministic identity invariants
- bundle schema and validation compliance
- policy pre-side-effect enforcement behavior

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

## Deterministic Identity Clarification

For the admitted family:

- canonical object identity is defined by CanonFS content-addressed hash
- identical task, model, policy, and input must produce identical bytes for:
  - result artifact
  - provenance artifact where validated
  - downstream record
  - canonical bundle

Trace or diagnostic representation may evolve outside this baseline, but must
not be treated as part of canonical object identity unless explicitly frozen by
another contract.

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

## Bundle Structural Integrity

For the current `.v1` admitted-family bundles:

- all frozen fields are required
- field names and semantics are immutable within `.v1`
- field ordering is canonicalized in the rendered bundle object
- malformed or partial bundles are invalid for baseline consumers

The baseline expects fail-closed validation for:

- wrong bundle schema
- malformed `action_ref`
- malformed `record_ref`
- other malformed required CanonFS refs

Consumers and tests must not treat malformed or partial bundles as compliant
baseline objects.

## Policy Evaluation Boundary

Policy evaluation must occur before:

- artifact materialization for the governed action
- execution of governed instructions for the bounded action
- mutation of CanonFS state on the governed artifact path

No partial execution of the governed action is permitted prior to a deny
decision.

A deny verdict must terminate the action without observable side effects on the
governed artifact path.

## Admitted Family Only

This stable baseline includes only:

- `assess-fixed`
- `route-fixed`
- `classify-fixed`

It does not imply:

- a fourth composition
- automatic admission of future compositions
- general bundle compatibility beyond the admitted family

## Admitted Family Freeze Clause

No new composition may be treated as part of this stable baseline without:

- explicit admission into the bounded family
- demonstration of compliance with the frozen laws in this document
- the required proof bar from
  [AI_OS_OBJECT_FAMILY_ADMISSION_CONTRACT.md](./AI_OS_OBJECT_FAMILY_ADMISSION_CONTRACT.md)
- an explicit contract update naming that composition as baseline-admitted

All other compositions are non-baseline by definition.

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
