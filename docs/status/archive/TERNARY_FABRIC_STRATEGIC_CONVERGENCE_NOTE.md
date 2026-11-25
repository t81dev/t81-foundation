# Ternary Fabric Strategic Convergence Note

This note records the current architectural decision for how T81 relates to
the separate `t81dev/ternary-fabric` repository.

## Current Decision

Do not merge `ternary-fabric` into T81 now.

Treat `ternary-fabric` as a parallel strategic track, not as:

- a T81 submodule
- a subtree import
- a direct code merge target
- part of T81's current stable build-against surface

## Why

The two repositories are thematically related, but they currently center
different architectural truths:

- T81 today:
  deterministic, policy-gated runtime;
  CanonFS/Axion provenance and admission;
  bounded AI OS-object family;
  RFC-00D1 interchange hardening
- `ternary-fabric` today:
  LD_PRELOAD/interposer acceleration;
  TFMBS device ABI;
  MLIR and compiler lowering;
  FPGA/RTL/hardware bring-up;
  performance/offload-first execution story

A full merge would blur T81's current runtime-first adoption wedge and add
subsystem breadth in exactly the areas T81 is trying to keep narrow.

## Allowed Near-Term Convergence

Allow only concept transfer and planning alignment:

- borrow clear contract-writing patterns after rewriting them into T81's
  runtime-first framing
- treat `ternary-fabric` as one future input into an experimental accelerator
  or backend note
- keep MLIR/GGUF/T3_K overlap in mind without merging compiler plugins,
  interposers, or hardware flows

## Explicitly Out Of Scope

Do not import or vendor:

- LD_PRELOAD interception
- fabric residency heuristics
- predictive scheduler/orchestrator code
- FPGA or ASIC bring-up flows
- TFMBS public ABI headers
- MLIR dialect/plugin stacks from `ternary-fabric`

## Future Integration Seam

If convergence is revisited later, the only sanctioned seam is:

- an experimental accelerator/backend boundary behind T81's runtime, outside
  current stable public APIs

That seam must preserve:

- T81 as the governed execution substrate
- CanonFS as the artifact/provenance system
- Axion as the pre-side-effect policy gate

Any future hardware or fabric acceleration should therefore be treated as an
execution-backend candidate, not as a replacement for T81's current project
identity.

## Revisit Conditions

Do not revisit deeper convergence until all of the following are true:

- RFC-00D1 is no longer an active hardening lane
- the bounded AI OS-object family remains stable without posture corrections
- T81's runtime/backends story is clear enough to add one experimental backend
  seam without confusing the public story
- there is a concrete T81 consumer for acceleration, not just thematic overlap

## Public Framing Rule

If `ternary-fabric` is mentioned alongside T81, describe it as:

- a related ternary acceleration effort
- not part of T81's current stable build-against surface
