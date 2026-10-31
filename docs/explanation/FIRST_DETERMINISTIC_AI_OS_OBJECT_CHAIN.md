# First Deterministic AI OS-Object Chain

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [First Deterministic AI OS-Object Chain](#first-deterministic-ai-os-object-chain)
  - [What Happens](#what-happens)
  - [The Four Objects](#the-four-objects)
  - [Canonical Identity Invariant](#canonical-identity-invariant)
  - [Key Helper Surfaces](#key-helper-surfaces)
  - [What This Is](#what-this-is)
  - [What This Is Not](#what-this-is-not)
  - [Core Claim](#core-claim)

<!-- T81-TOC:END -->


This is the smallest current example of T81 acting like a governed AI
OS-object substrate rather than only an AI runtime.

The canonical runnable example is:

```bash
bash examples/ai-and-inference/model-load-canonfs/run_assess_fixed_host_action.sh
```

Portable smoke path for the same chain:

```bash
./build/t81_ai_task_assess_fixed_composition_test ./build/t81
```

The shell script remains the human-readable demo. The compiled smoke path
exercises the same object flow without relying on POSIX shell behavior.

For the current bounded family of reusable AI OS-object chains built on the
same object model, see
[AI_OS_OBJECT_CHAIN_CATALOG.md](../reference/AI_OS_OBJECT_CHAIN_CATALOG.md).

## What Happens

The current `assess-fixed` chain is:

1. a bounded governed AI task runs in the strict deterministic lane
2. the task stores a result artifact and a provenance artifact in CanonFS
3. a typed downstream record is created from that result and stored
4. a final bundle artifact is created and stored
5. the bundle becomes the canonical top-level object for the whole chain

This is the actual object flow:

```text
assess-fixed task
  -> task result artifact
  -> provenance artifact
  -> downstream host-action record
  -> final bundle artifact
```

## The Four Objects

- Task result artifact:
  the bounded AI result for the assessed input. This is an intermediate object.
- Provenance artifact:
  the retained execution and policy evidence for that task result. This is an
  intermediate object.
- Downstream record:
  the typed non-AI decision record created from the AI result. This is an
  intermediate object.
- Final bundle artifact:
  the CanonFS object that links the full chain together. This is the canonical
  top-level persisted object.

The bundle is canonical because it is the smallest single object that points to
the whole completed chain: result, provenance, downstream record, and action
identity.

## Canonical Identity Invariant

For the currently validated `assess-fixed`, `route-fixed`, and
`classify-fixed` chains, object identity at every layer is a function only of:

- task
- model
- policy
- input

For those chains, the following objects keep the same content-addressed
identity and bytes for identical task/model/policy/input:

- task result artifact
- provenance artifact
- downstream record
- final bundle artifact

CanonFS root does not affect that identity.
Execution location does not affect that identity.

Within this object model:

- result artifact: intermediate
- provenance artifact: intermediate
- downstream record: intermediate
- bundle: canonical top-level object

This claim is limited to the currently validated bounded family.

## Key Helper Surfaces

The canonical example uses these typed surfaces:

- `t81 ai task assess-fixed`
- `t81 ai task read-field`
- `t81 artifact write-store-record`
- `t81 artifact store-bundle`
- `t81 artifact read-field`

These helpers do not form a general workflow engine. They only support the
current narrow typed chain family.

## What This Is

- one deterministic governed AI OS-object chain
- bounded
- policy-gated
- artifact-first
- CanonFS-backed

## What This Is Not

- chat
- agents
- general inference serving
- workflow orchestration
- a general packaging engine

## Core Claim

T81 does not just run AI here.

It produces governed, typed, persistent objects that other parts of the system
can consume.
