# External Dependency Pins

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [External Dependency Pins](#external-dependency-pins)
  - [Purpose](#purpose)
  - [Scope](#scope)
  - [Pinned Dependencies](#pinned-dependencies)
    - [llama.cpp](#llamacpp)
  - [Change Control](#change-control)
  - [Versioning Statement](#versioning-statement)

<!-- T81-TOC:END -->


Status: Active
Last Updated: 2026-02-25

## Purpose

Record immutable commit pins for externally sourced repositories used by governed
non-DCP surfaces.

## Scope

- Applies to external source dependencies pulled into `third_party/`.
- Does not, by itself, grant DCP certification to dependent components.

## Pinned Dependencies

### llama.cpp

- Repository: `https://github.com/ggml-org/llama.cpp.git`
- Local path: `third_party/llama.cpp`
- Pin type: Git commit
- Pinned commit: `3769fe6eb70b0a0fbb30b80917f1caae68c902f7`
- Classification: Governed non-DCP (experimental inference surface)
- Build gate: `-DT81_ENABLE_LLAMA_CPP=ON`

## Change Control

Any pin update must include:

1. Commit delta review summary.
2. Axion/policy compatibility validation for governed inference path.
3. Build reproducibility verification on supported platforms.
4. Update to this file with new commit hash and date.

## Versioning Statement

This file is governance policy metadata for dependency provenance and does not
override specification authority.
