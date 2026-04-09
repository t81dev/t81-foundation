# Model Artifact Thesis Test Plan

Status: Draft
Last Updated: 2026-04-08

## Purpose

This document defines the smallest useful test of the model-artifact strategy
described in `docs/explanation/MODEL_ARTIFACT_STRATEGY.md`.

The goal is to test user pull before investing in a broad product surface.

## Thesis Under Test

Teams will adopt a new intake path for `safetensors` and later `gguf` if that
path makes model artifacts:

- easier to inspect
- easier to compare
- easier to trace through transformations
- easier to trust in CI or release workflows

The thesis is not merely that T81 can parse model formats.

The thesis is that artifact legibility, identity, and provenance are painful
enough problems that users will change workflows to get them.

## Non-Goals

Do not test these yet:

- a general AI runtime story
- full `gguf` support
- broad execution integration
- new opcode surface
- a large policy system around model artifacts
- model serving or inference throughput claims

If the narrow intake thesis does not hold, broader work should not proceed.

## Smallest Prototype

Build only a `safetensors`-first thin slice.

### Command

`t81 model import <file>`

### Output

The command should produce only the minimum needed to test value:

1. a canonical manifest JSON
2. a tensor inventory JSON
3. stable artifact and tensor identities
4. a concise human-readable report

### Report Should Answer

- what tensors are in the file
- what their names, shapes, and formats are
- what metadata claims the artifact carries
- whether anything looks unsupported or suspicious
- what stable identities were assigned to imported content

Optional but valuable for the same slice:

`t81 model diff <artifact-a> <artifact-b>`

This should stay report-focused, not transformation-heavy.

## Test Workflows

The prototype should be evaluated against a few real workflows.

### Workflow 1: Third-Party Intake

Import a `safetensors` file from outside the repo.

Question:

Does T81 make it easier to understand what the file contains than existing
format-specific inspection scripts?

### Workflow 2: Revision Comparison

Compare two model artifacts that differ in some real way.

Question:

Does T81 make the difference easier to review than current ad hoc methods?

### Workflow 3: Conversion Verification

Import an artifact before and after a conversion or quantization step.

Question:

Does T81 give a clearer record of what changed and what stayed the same?

Important interpretation note:

At the current prototype stage, `model diff` should be treated as a
representation-sensitive comparison. If a `.gguf` and `safetensors` artifact
carry the same logical weights but arrive through different layout or import
conventions, the current tool may still report changed tensors. That is a
useful result for this thesis test, but it is not yet a semantic-equivalence
proof.

Observed result from the first `.gguf` experiment:

- a GGUF produced by T81's own quantize path imported successfully
- GGUF versus itself diffed cleanly
- GGUF versus the source `safetensors` diffed as changed across shared tensors
  because imported shape/layout conventions still differ

That means this workflow already surfaces a real product question:
whether T81 should stay focused on imported-representation diffing, or grow a
separate normalization-aware comparison mode.

### Workflow 4: CI Gate Trial

Run import plus validation in CI for one checked-in artifact.

Question:

Would a team keep this in a real pipeline, or is it only interesting in a demo?

## Test Users

Prefer 3 to 5 concrete evaluators rather than a broad survey.

Best-fit evaluators:

- one infra or platform engineer
- one security-minded or compliance-minded engineer
- one person who already handles model conversion or packaging
- optionally one researcher or ML engineer who imports outside artifacts often

These do not need to be external design partners at first. Internal users are
fine if the workflows are real.

## Questions To Ask

After each workflow, ask short concrete questions:

1. Did this save time compared with your current method?
2. Did it show you anything your current tools do not show clearly?
3. Would you use this in CI, release prep, or intake review?
4. What was missing that you expected to see?
5. If this existed today, how often would you use it?

Avoid asking whether the idea is "interesting." Ask whether it is useful
enough to keep in a workflow.

## Success Criteria

Treat the prototype as promising only if at least some evaluators say things
like:

- "I would use this in our intake or release flow."
- "This makes model revisions easier to review."
- "This gives us a record we do not currently have."
- "This caught something our current tooling hides or makes annoying to inspect."

More concretely:

- at least 2 evaluators say they would reuse it in a real workflow
- at least 1 workflow reveals a concrete issue or ambiguity that current tools
  did not surface as clearly
- the import/report path is reproducible enough that repeated runs produce the
  same manifest and identities

## Failure Criteria

Treat the thesis as weak if feedback sounds like:

- "I can already get enough of this from existing tools."
- "The identity and provenance are nice but not operationally important."
- "This is more ceremony than value."
- "I would not add this to CI or intake."

If most users react that way, do not expand into `gguf`, export, execution, or
policy breadth. Narrow again or stop.

## Decision Gate

Use this gate before further investment.

### Proceed

Proceed only if the prototype creates obvious workflow pull for at least a few
users.

Then the next step is:

- support `gguf`
- add artifact diff formally
- add normalized export with provenance

### Pause

Pause if the prototype is technically clean but users do not care enough to
change workflow.

### Stop

Stop if the problem turns out not to be painful enough or existing tools are
already good enough for the target user.

## What Not To Build Yet

Until the thesis is validated, do not spend time on:

- broad UI polish
- advanced policy profiles
- generalized execution integration
- new runtime abstractions for model execution
- large cross-format conversion matrices
- marketing language about a new AI runtime category

## Recommended First Milestone

The first milestone should be considered complete when we have:

1. one `safetensors` import command
2. one stable report command or import report mode
3. one golden example in-repo
4. one malformed-input negative test
5. one short write-up of 3 to 5 workflow evaluations and what users actually said

That is enough to decide whether the thesis deserves more investment.
