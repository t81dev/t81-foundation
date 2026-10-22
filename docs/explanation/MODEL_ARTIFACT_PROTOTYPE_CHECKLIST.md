# Model Artifact Prototype Checklist

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Model Artifact Prototype Checklist](#model-artifact-prototype-checklist)
  - [Purpose](#purpose)
  - [Scope Boundary](#scope-boundary)
  - [Existing Surfaces To Reuse](#existing-surfaces-to-reuse)
  - [Prototype Deliverables](#prototype-deliverables)
    - [1. Narrow CLI Entry](#1-narrow-cli-entry)
    - [2. Canonical Manifest](#2-canonical-manifest)
    - [3. Tensor Inventory](#3-tensor-inventory)
    - [4. Human-Readable Report](#4-human-readable-report)
    - [5. Golden Example](#5-golden-example)
    - [6. Negative Test](#6-negative-test)
  - [Implementation Sequence](#implementation-sequence)
    - [Step 1: Find The Smallest Reuse Path](#step-1-find-the-smallest-reuse-path)
    - [Step 2: Define The Prototype Result Shape](#step-2-define-the-prototype-result-shape)
    - [Step 3: Implement `safetensors` Import](#step-3-implement-`safetensors`-import)
    - [Step 4: Add Report Mode](#step-4-add-report-mode)
    - [Step 5: Add Example And Negative Test](#step-5-add-example-and-negative-test)
  - [Acceptance Criteria](#acceptance-criteria)
  - [Evaluation Handoff](#evaluation-handoff)
  - [Stop Conditions](#stop-conditions)
  - [Immediate Next Task](#immediate-next-task)

<!-- T81-TOC:END -->


Status: Draft
Last Updated: 2026-04-08

## Purpose

This checklist translates
`docs/explanation/MODEL_ARTIFACT_THESIS_TEST_PLAN.md`
into a concrete first prototype for the repo.

The prototype is intentionally narrow:

- `safetensors` first
- import and report only
- enough JSON and example coverage to test user pull
- no broad runtime or policy expansion

## Scope Boundary

Build only what is needed to answer the thesis question.

In scope:

- `t81 model import <file>`
- canonical manifest output
- tensor inventory output
- one human-readable report path
- one checked-in golden example
- one malformed-input negative test

Out of scope:

- `gguf` support
- export or round-trip conversion
- execution integration
- advanced policy profiles
- new opcode work
- broad CLI family redesign

## Existing Surfaces To Reuse

Prefer extending existing stable-enough surfaces rather than creating a parallel
 stack.

Likely reuse points:

- current weights and model parsing surfaces in `include/t81/weights.hpp`
- existing `safetensors` examples under
  `examples/ai-and-inference/model-load-canonfs/`
- CanonFS import/export and JSON contract patterns
- existing CLI JSON rendering conventions

Do not create a second artifact-intake architecture if current weights import
and CanonFS code can be reused cleanly.

## Prototype Deliverables

### 1. Narrow CLI Entry

Add one prototype command surface:

- `t81 model import <file>`

Prototype behavior:

- accepts one `safetensors` input file
- validates file readability and basic parse success
- emits a stable JSON result

Minimum JSON fields:

- `status`
- `source_path`
- `source_format`
- `artifact_identity`
- `tensor_count`
- `manifest_path` or inline manifest object
- `warnings`
- `errors`

This does not need to be the final public contract. It does need to be stable
enough for prototype evaluation.

### 2. Canonical Manifest

Define one minimal manifest shape for imported `safetensors`.

Required fields:

- source artifact hash
- source filename or logical source id
- import timestamp or deterministic import metadata marker
- tensor inventory summary
- per-tensor identity linkage
- parser or importer version marker

Manifest design goal:

- easy for humans to inspect
- easy to diff mechanically
- explicit about what is known today versus omitted

### 3. Tensor Inventory

For each tensor, capture:

- tensor name
- shape
- element count
- source encoding or dtype as reported by the parser
- canonical identity or stored object reference

Do not overdesign tensor semantics in phase one.

### 4. Human-Readable Report

Support one human-readable output mode, either:

- `t81 model report <artifact>`
- or `t81 model import <file> --report`

The report should answer:

- what the artifact contains
- what tensors are present
- what looks unsupported or suspicious
- what stable identities were assigned

Keep the report short and useful. Avoid trying to create a complete model
analysis surface yet.

### 5. Golden Example

Add one checked-in example workflow under `examples/`.

Prototype example should include:

- one tiny `safetensors` fixture or generator
- one import invocation
- one expected report or JSON output
- one short README explaining what the example demonstrates

Success condition:

a contributor should be able to run the example and understand the prototype
value without reading strategic docs first.

### 6. Negative Test

Add at least one malformed-input test.

Good first candidates:

- unreadable file
- malformed `safetensors` header
- mismatched tensor metadata
- unsupported dtype or encoding

The prototype should produce explicit and reviewable failure output, not a
generic parse failure.

## Implementation Sequence

### Step 1: Find The Smallest Reuse Path

Before writing new CLI glue:

- inspect current `weights import` and `load_safetensors` surfaces
- decide whether prototype should wrap them directly or extract a small shared
  importer module

Decision rule:

prefer extraction only if it reduces duplication immediately.

### Step 2: Define The Prototype Result Shape

Before adding command wiring:

- define one JSON shape for import output
- define one manifest shape
- write them down in the implementation PR or docs

This avoids a half-implicit output contract.

### Step 3: Implement `safetensors` Import

Prototype should:

- parse source file
- build tensor inventory
- compute stable identities
- emit JSON and or manifest

If CanonFS-backed storage is straightforward, use it.
If it is not yet straightforward, prefer a clearly temporary prototype storage
shape over blocking the whole thesis test.

### Step 4: Add Report Mode

Build the smallest report that is still useful in user interviews.

### Step 5: Add Example And Negative Test

Do not call the prototype complete without:

- one runnable example
- one failing malformed-input test

## Acceptance Criteria

The first prototype is complete when all of these are true:

1. `t81 model import <file>` works for one checked-in `safetensors` example
2. repeated imports of the same file produce stable manifest identities
3. tensor inventory is visible in machine-readable output
4. one human-readable report path exists
5. one malformed-input case is covered by an automated test
6. one example README explains the workflow in contributor-friendly language

## Evaluation Handoff

Once the prototype exists, gather 3 to 5 workflow evaluations as described in
`docs/explanation/MODEL_ARTIFACT_THESIS_TEST_PLAN.md`.

Do not broaden the scope until that evaluation happens.

## Stop Conditions

Pause prototype expansion if any of these become true:

- the importer mostly duplicates existing tooling without clearer output
- the manifest cannot be kept stable enough to compare meaningfully
- users do not care about the report or identities
- the work starts pulling toward a broad model-runtime project before import is
  useful on its own

## Immediate Next Task

The next concrete coding task should be:

1. inspect current `weights import` and `load_safetensors` plumbing
2. identify the smallest reusable importer surface
3. draft the prototype JSON result shape
4. implement `t81 model import <file>` for one `safetensors` example

That is enough to start real thesis testing without overcommitting.
