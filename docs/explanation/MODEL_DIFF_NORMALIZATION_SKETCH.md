# Model Diff Normalization Sketch

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Model Diff Normalization Sketch](#model-diff-normalization-sketch)
  - [Purpose](#purpose)
  - [Current Boundary](#current-boundary)
  - [Problem To Solve](#problem-to-solve)
  - [Proposed Principle](#proposed-principle)
  - [Smallest Useful Normalization Scope](#smallest-useful-normalization-scope)
  - [Candidate Rules](#candidate-rules)
    - [Rule 1: Known layout transpose](#rule-1-known-layout-transpose)
    - [Rule 2: Metadata-only format summary differences](#rule-2-metadata-only-format-summary-differences)
    - [Rule 3: Unknown sparsity on one side](#rule-3-unknown-sparsity-on-one-side)
  - [Output Shape](#output-shape)
  - [First Buildable Experiment](#first-buildable-experiment)
  - [Recommended Decision](#recommended-decision)

<!-- T81-TOC:END -->


Status: Draft
Last Updated: 2026-04-08

## Purpose

This note defines the smallest useful shape of a future normalization-aware
comparison mode for model artifacts.

It is intentionally narrower than an implementation plan. The goal is to make
one question concrete before changing `t81 model diff` semantics:

What should T81 be allowed to rewrite or reinterpret before calling two
artifacts "equivalent" across formats?

## Current Boundary

Today, `t81 model diff` is representation-sensitive.

That means it compares the imported view T81 currently sees, including:

- tensor names
- tensor shapes
- per-tensor trit counts
- sparsity when known on both sides
- imported format summary

This works well for:

- same-file review
- same-format review
- persisted-manifest versus source-artifact review

It does not yet answer a different question:

Are these two artifacts semantically the same model after format-specific
layout conventions are normalized?

## Problem To Solve

The first `.gguf` experiment showed a real gap:

- a GGUF produced by T81 imported successfully
- GGUF versus itself diffed as identical
- GGUF versus the source `safetensors` diffed as changed across shared tensors

The mismatch was not an arbitrary failure. It reflected imported representation
differences such as shape or orientation conventions.

That gives us two distinct comparison modes:

1. raw imported-representation comparison
2. normalization-aware semantic comparison

Those should not be conflated.

## Proposed Principle

Keep the current `model diff` behavior unchanged.

If T81 gains normalization-aware comparison, it should be:

- explicit
- opt-in
- narrowly scoped
- accompanied by a machine-readable normalization summary

In other words, do not silently broaden what `identical` means in
`t81.model-diff.v1`.

## Smallest Useful Normalization Scope

The first version should normalize only shape/layout conventions that are:

- common
- deterministic
- reversible or explainable
- format-driven rather than model-specific guesswork

That likely means:

- dimension-order normalization for known weight classes
- limited transpose equivalence for specific tensor families
- manifesting which normalization rule was applied per tensor

It should not yet include:

- fuzzy metadata equivalence
- guessed tensor renames
- tolerance-based numeric equivalence
- hidden re-quantization or value-domain reinterpretation

## Candidate Rules

These are examples of rules worth evaluating, not admitted behavior yet.

### Rule 1: Known layout transpose

For certain known tensor families, treat `[A, B]` and `[B, A]` as equivalent if
the format adapter documents that one format stores the tensor transposed.

This is the strongest candidate because it explains the current `.gguf` versus
`safetensors` mismatch clearly.

Admission bar:

- the tensor family must be named explicitly
- the rule must be tied to a known format pair or import adapter
- the normalization must be recorded in output

### Rule 2: Metadata-only format summary differences

Allow imported-format summary strings to differ without automatically making
two artifacts non-equivalent, if the normalized tensor inventory and counts
match.

This should be treated carefully. Format summary differences are often
informative, so they should still be reported even if not fatal to equivalence.

### Rule 3: Unknown sparsity on one side

This is already partly handled today: missing sparsity should not force a
difference if the other side knows it.

That behavior should stay narrow and explicit.

## Output Shape

A future normalization-aware mode should emit a distinct contract instead of
reusing `t81.model-diff.v1` silently.

Possible direction:

- `t81 model diff <lhs> <rhs> --json --mode normalized`

With output additions like:

- `comparison_mode`
- `normalization_applied`
- `normalization_summary`
- `normalization_rules`
- per-tensor normalized change classification

The key design point is that the current raw diff and a future normalized diff
should be separable in both CLI behavior and JSON contract.

## First Buildable Experiment

The first finishable experiment should be extremely small:

1. keep `model diff` as-is
2. add one internal normalization helper for a single known tensor-family
   transpose rule
3. run it only in a new opt-in mode
4. emit which tensors were normalized and why
5. test exactly one checked pair:
   - source `safetensors`
   - GGUF derived from that source via T81's own quantize path

Success would mean:

- same-file diffs are unchanged
- raw diff still shows representation differences
- normalized diff can explain why the cross-format pair is considered
  equivalent or closer to equivalent

## Recommended Decision

Do not change `model diff` v1 semantics.

Instead:

- keep raw imported-representation diff as the default
- design a separate normalization-aware mode as a follow-on experiment
- admit only one or two explicit normalization rules at first

That keeps the current contract honest and makes any semantic-equivalence claim
something we can test instead of imply.
