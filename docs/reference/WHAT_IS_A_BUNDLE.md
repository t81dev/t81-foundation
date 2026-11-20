# What Is A Bundle

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [What Is A Bundle](#what-is-a-bundle)
  - [What It Represents](#what-it-represents)
  - [What It Guarantees](#what-it-guarantees)
  - [What It Is Not](#what-it-is-not)
  - [The Core Innovation](#the-core-innovation)

<!-- T81-TOC:END -->


A bundle is a **canonical decision object with identity** that represents the complete, policy-gated outcome of a bounded AI task.

## What It Represents

A bundle is the smallest object that names a full completed decision chain:

- **Decision boundary** - The point where AI inference becomes a typed decision
- **Policy approval** - Evidence that the decision passed governance requirements  
- **Provenance anchor** - Immutable links back to source AI output and execution evidence
- **Action identity** - Canonical reference to the downstream action that should occur

## What It Guarantees

Every bundle provides:

- **Content addressing** - Bundle identity is a cryptographic hash of its contents
- **Deterministic reproduction** - Same task + model + policy + input always produces same bundle
- **Type safety** - Decision fields are validated against family-specific schemas
- **Cross-system portability** - Bundle can be consumed without the original execution environment
- **Audit trail integrity** - Complete provenance from input to decision is verifiable

## What It Is Not

A bundle is **not**:

- A log file or event stream
- An inference result or model output
- A workflow or orchestration object  
- A general-purpose data container
- A live runtime interface

## The Core Innovation

Traditional AI systems produce ephemeral text that must be interpreted. 

T81 bundles produce **durable decision objects that can be safely consumed**.

This shifts AI from "interpret output" to "consume decision" - making AI decisions as verifiable and reusable as signed documents.
