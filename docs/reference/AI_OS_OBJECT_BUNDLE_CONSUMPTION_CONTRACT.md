# AI OS-Object Bundle Consumption Contract

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [AI OS-Object Bundle Consumption Contract](#ai-os-object-bundle-consumption-contract)
  - [Scope](#scope)
  - [What A Consumer May Assume](#what-a-consumer-may-assume)
  - [What The Bundle Means](#what-the-bundle-means)
  - [Current Family Reading](#current-family-reading)
  - [Minimum Consumer Workflow](#minimum-consumer-workflow)
  - [What A Consumer Must Not Assume](#what-a-consumer-must-not-assume)
  - [Stability Boundary](#stability-boundary)
  - [Relationship To Existing Family Docs](#relationship-to-existing-family-docs)

<!-- T81-TOC:END -->


This document defines the narrowest current contract for consuming a canonical
bounded-family bundle outside the immediate composition script or test that
created it.

It is a consumer contract, not a new runtime surface.

## Scope

This contract applies only to the current admitted bounded AI OS-object family:

- `t81.ai.task.assess-fixed.bundle.v1`
- `t81.ai.task.route-fixed.bundle.v1`
- `t81.ai.task.classify-fixed.bundle.v1`

It does not define:

- a general bundle platform
- a schema registry for arbitrary object families
- a network API
- a compatibility promise for non-family bundles

## What A Consumer May Assume

A consumer may assume that a valid bounded-family bundle is:

- a CanonFS-backed content-addressed artifact
- the canonical top-level persisted object for a completed admitted chain
- fixed-shape
- deterministic for identical task, model, policy, and input

For the current admitted family, a consumer may rely on this exact field set:

1. `schema`
2. `source_result_ref`
3. `source_provenance_ref`
4. `action_ref`
5. `record_ref`

Each `*_ref` field must be a CanonFS content-addressed artifact reference.

## What The Bundle Means

The bundle is not just a summary.

It is the canonical object that points to the completed bounded chain:

- the originating AI task result
- the associated provenance artifact
- the canonical downstream action identity
- the downstream typed record

That means a consumer can treat the bundle as the smallest object that names a
full completed decision chain.

## Current Family Reading

The interpretation of `action_ref` depends on the admitted family member:

- `assess-fixed`
  - `action_ref` names the selected canonical host-action artifact
- `route-fixed`
  - `action_ref` names the selected canonical path/action artifact
- `classify-fixed`
  - `action_ref` names the selected canonical rule-set artifact

The interpretation of `record_ref` is likewise family-specific, but it always
points to the typed downstream record for that chain.

## Minimum Consumer Workflow

The minimum current consumer workflow is:

1. receive or locate a bundle ref
2. read the bundle artifact from CanonFS
3. inspect `schema`
4. follow `record_ref` to the downstream typed record if family-specific fields
   are needed
5. follow `source_result_ref` and `source_provenance_ref` if the consumer needs
   the underlying AI result or execution evidence

In other words:

- consume the bundle first
- dereference deeper objects only when needed

## What A Consumer Must Not Assume

A consumer must not assume:

- bundle schemas outside the admitted family follow this contract
- family-specific downstream record fields are interchangeable across schemas
- local paths, execution environment, or host location are part of bundle
  identity
- broader AI runtime semantics such as multi-step continuation or general
  inference serving

## Stability Boundary

This contract is intentionally narrow.

It freezes only what an external consumer needs to use a current family bundle
without verbal explanation:

- bundle field set
- bundle role as the canonical top-level object
- CanonFS ref semantics
- family-specific reading of `action_ref`

It does not yet freeze:

- a versioned schema registry outside the current family docs
- transport protocols
- generalized consumer APIs
- compatibility rules for future non-family bundle types

The narrow `.v1` versioning boundary for the current family bundle is defined
in
[AI_OS_OBJECT_BUNDLE_VERSIONING_BOUNDARY.md](./AI_OS_OBJECT_BUNDLE_VERSIONING_BOUNDARY.md).

## Relationship To Existing Family Docs

Use this document together with:

- [AI_OS_OBJECT_CHAIN_CATALOG.md](./AI_OS_OBJECT_CHAIN_CATALOG.md)
- [AI_OS_OBJECT_BUNDLE_INTEGRATION_MATRIX.md](./AI_OS_OBJECT_BUNDLE_INTEGRATION_MATRIX.md)
- [AI_OS_OBJECT_BUNDLE_VERSIONING_BOUNDARY.md](./AI_OS_OBJECT_BUNDLE_VERSIONING_BOUNDARY.md)
- [AI_OS_OBJECT_FAMILY_ADMISSION_CONTRACT.md](./AI_OS_OBJECT_FAMILY_ADMISSION_CONTRACT.md)
- [BOUNDED_AI_OS_OBJECT_FAMILY_STATUS.md](../status/BOUNDED_AI_OS_OBJECT_FAMILY_STATUS.md)

If a future change alters the bundle field set or its canonical role, this
contract must be updated before that change should be treated as stable for
external consumers.
