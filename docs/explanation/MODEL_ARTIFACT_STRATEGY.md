# Model Artifact Strategy

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Model Artifact Strategy](#model-artifact-strategy)
  - [Purpose](#purpose)
  - [Narrow Thesis](#narrow-thesis)
  - [Problem](#problem)
  - [User](#user)
  - [Product Wedge](#product-wedge)
  - [What T81 Should Do](#what-t81-should-do)
    - [Import](#import)
    - [Report](#report)
    - [Diff](#diff)
    - [Export](#export)
  - [Why This Fits T81](#why-this-fits-t81)
    - [CanonFS](#canonfs)
    - [Axion](#axion)
    - [Deterministic Runtime Pieces](#deterministic-runtime-pieces)
  - [What This Is Not](#what-this-is-not)
  - [Messaging Shift](#messaging-shift)
  - [Smallest Buildable Milestone](#smallest-buildable-milestone)
    - [Scope](#scope)
    - [Deliverables](#deliverables)
    - [Success Criteria](#success-criteria)
  - [Recommended Sequencing](#recommended-sequencing)
    - [Phase 1](#phase-1)
    - [Phase 2](#phase-2)
    - [Phase 3](#phase-3)
  - [Current Repo Implications](#current-repo-implications)
  - [Bottom Line](#bottom-line)

<!-- T81-TOC:END -->


Status: Draft
Last Updated: 2026-04-08

## Purpose

This memo reframes one practical direction for T81 around a concrete present-day
hardship:

- model artifacts such as `safetensors` and `gguf` are operationally important
- they are often handled as opaque files
- current intake, conversion, and execution workflows lose too much clarity
  about identity, structure, and transformation history

The intended use of this memo is strategic and build-planning only. It does not
change spec authority or freeze any new contract surface on its own.

## Narrow Thesis

T81 should treat model artifacts as auditable execution inputs rather than
opaque blobs.

In this framing, T81 does not start by claiming to be a general AI runtime or
an operating-system-like layer. It starts by solving a narrower workflow:

- ingest a model artifact
- normalize it into a stable internal representation
- preserve artifact identity and provenance
- make the structure reviewable before later transformation or execution

This is a better fit for the strongest current repo surfaces:

- CanonFS content-addressed storage
- CLI and JSON contract work
- provenance and schema discipline
- explicit import/export behavior
- pre-side-effect policy checks around artifact handling

## Problem

Model artifacts already behave like important software supply-chain inputs, but
teams rarely handle them that way.

Typical workflow today:

1. download or receive a `safetensors` or `gguf` file
2. inspect it with format-specific scripts
3. convert or quantize it with another tool
4. move it into evaluation or inference infrastructure
5. lose a clean record of what changed, what was trusted, and what was used

Common consequences:

- weak chain of custody for model files
- hard-to-review changes between artifact revisions
- inconsistent surfacing of metadata and format assumptions
- conversion steps that are hard to reproduce exactly
- poor linkage between source artifact and downstream execution

## User

Best early users:

- platform engineers importing third-party or cross-team model artifacts
- infra teams performing normalization, conversion, or quantization
- security-conscious teams that need a trustworthy intake path
- teams that must answer "what artifact did we actually run?"

These users do not primarily need a new VM. They need a reliable intake and
normalization workflow for model artifacts.

## Product Wedge

The first product wedge should be model-artifact intake and normalization.

Plainly:

T81 imports `safetensors` and `gguf`, emits a canonical manifest and stored
tensor objects, and keeps a stable record of what the artifact contained and
what happened to it.

That gives T81 a concrete story:

- not "AI operating system"
- not "novel opcodes"
- not "governance" as an abstract selling point
- but "auditable model-artifact intake and transformation"

## What T81 Should Do

### Import

`t81 model import <file>`

Responsibilities:

- detect supported artifact format
- validate structure and basic consistency
- extract tensor inventory and metadata
- normalize into CanonFS-backed stored objects
- emit an import manifest and provenance record

### Report

`t81 model report <artifact>`

Responsibilities:

- show tensor names, shapes, counts, and encoding or quantization format
- show source metadata claims
- identify unsupported or suspicious content
- expose stable identities and hashes for the imported result

### Diff

`t81 model diff <artifact-a> <artifact-b>`

Responsibilities:

- compare tensors, shapes, formats, and metadata
- report added, removed, renamed, or changed content
- preserve artifact-level provenance linkage

Current practical boundary:

- the existing diff path is representation-sensitive
- it works well for same-format and manifest-versus-source review
- it does not yet normalize cross-format layout conventions into a semantic
  equivalence layer

This boundary matters because a `.gguf` produced from a `safetensors` source
may still compare as changed today even when aggregate counts match.

### Export

`t81 model export <artifact>`

Responsibilities:

- emit normalized outputs with recorded source linkage
- keep import identity and derived-artifact identity explicit

## Why This Fits T81

This direction uses existing repo strengths without requiring a broad new
platform claim.

### CanonFS

CanonFS already fits immutable object storage and artifact identity.

### Axion

Axion is useful here when scoped carefully:

- permit or deny import
- permit or deny export
- permit or deny later transformation steps

The value is stronger when tied to artifact handling than when stated as an
abstract property of all computation.

### Deterministic Runtime Pieces

Deterministic normalization matters for:

- stable manifests
- repeatable content hashing
- reproducible import/export results

This is a more believable use of deterministic behavior than trying to make all
AI inference itself the center of the product story.

## What This Is Not

This memo does not claim that a `safetensors` or `gguf` file is executable code
in the same sense as a machine binary.

A better description is:

- structured model artifact
- execution input with meaningful semantics
- transformation input whose identity and history matter

T81's job is to make that structure explicit and accountable.

## Messaging Shift

Messages to avoid leading with:

- deterministic AI runtime
- AI operating system
- novel opcodes
- governance as a standalone reason to adopt T81

Messages to lead with:

- model artifacts are too opaque
- teams need trustworthy model intake
- T81 turns model files into inspectable canonical artifacts
- T81 preserves identity and provenance through import, transformation, and use

## Smallest Buildable Milestone

The first milestone should be narrow and finishable.

### Scope

- support `safetensors` first
- defer `gguf` until the first path is clear and tested

Status update:

That first `safetensors`-first milestone now exists as a prototype, and a
follow-on `.gguf` experiment has been run using a GGUF produced by T81's own
quantize path. The result was useful:

- `.gguf` import works through the current intake/report path
- persisted manifests and same-file diffs work for `.gguf`
- cross-format diff against the source `safetensors` is still
  representation-sensitive rather than semantic

So the next build question is not "should T81 read `.gguf` at all?" It is
"should T81 add a normalization-aware comparison mode, and if so what exactly
does equivalence mean across formats?"

### Deliverables

1. `t81 model import <file>`
2. canonical manifest JSON
3. tensor inventory JSON
4. CanonFS-backed stored tensor payloads
5. provenance record linking source file to imported objects
6. `t81 model report <artifact>`
7. negative tests for malformed imports and unsupported content

### Success Criteria

The first milestone is successful if a contributor can:

1. import one checked-in `safetensors` example
2. inspect a stable report of what was imported
3. see CanonFS identities for the imported result
4. reproduce the same manifest and object identities on repeat import

## Recommended Sequencing

### Phase 1

- add a narrow `safetensors` intake path
- define a manifest JSON contract
- store imported tensor payloads in CanonFS
- add one golden example and matching negative tests

### Phase 2

- add `gguf` intake
- add artifact diff
- add normalized export

### Phase 3

- connect imported artifacts to selected T81-native execution paths
- record which imported artifact was later used in evaluation or inference

## Current Repo Implications

If this direction is accepted, contributor guidance should prefer work like:

- artifact import contract definition
- CanonFS-backed model-artifact storage
- import/report JSON stability
- negative-path and malformed-input handling
- provenance-preserving export or transformation

Less useful immediate work would be:

- broadening operating-system messaging
- expanding new opcode surface without a tied workflow
- claiming generalized AI runtime breadth before artifact intake is solid

## Bottom Line

T81 has a more credible near-term story if it starts with model-artifact
legibility, identity, and transformation tracking.

The practical center becomes:

T81 is an auditable intake and normalization layer for model artifacts,
starting with `safetensors` and `gguf`.
