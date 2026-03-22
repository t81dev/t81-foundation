# Experimental Surface Relocation Proposal

**Date:** 2026-03-19  
**Status:** Proposed  
**Owner:** @t81dev

## Problem Statement

The repository's current `experimental/` layout no longer matches the
authoritative governance status of several implemented surfaces.

This creates avoidable confusion in three ways:

1. Some surfaces remain physically located under `experimental/` even though
   the current status documents classify them as `Stable` or `Beta` and
   `Governed non-DCP`.
2. The governance rules require a clean public API boundary under
   `include/t81/**` with no experimental headers in the public contract, but
   some graph/cognitive headers still live under `include/t81/experimental/`.
3. Users can reasonably misread "still in experimental/" as "not operational",
   even when the implementation matrix says otherwise.

This proposal defines which surfaces are ready to move out of `experimental/`
for layout and boundary clarity, and which surfaces must remain there.

## Goals

- Align directory layout with the current governance posture.
- Reduce ambiguity between `Experimental`, `Governed non-DCP`, and
  `Verified / DCP`.
- Move only surfaces that have current evidence and a stable enough API shape.
- Preserve the rule that moving out of `experimental/` does **not** imply DCP
  promotion.

## Non-Goals

- This proposal does not promote any surface into `Verified` / `DCP`.
- This proposal does not widen determinism claims beyond what the registry and
  status documents already allow.
- This proposal does not attempt to graduate Cognitive Tiers broadly, Hanoi VM,
  distributed compute, or the trace-JIT.

## Decision Summary

### Ready To Move Out Of `experimental/`

#### 1. DPE (Parallel Execution)

**Current evidence**

- [IMPLEMENTATION_MATRIX.md](/docs/status/IMPLEMENTATION_MATRIX.md)
  classifies `DPE (Parallel Execution)` as `Accepted`, implementation maturity
  `Stable`, promotion state `Governed non-DCP`.
- DPE headers and implementations already use the non-experimental namespace
  `t81::dpe`:
  - [task_graph.hpp](/include/t81/dpe/task_graph.hpp)
  - [task_runner.hpp](/include/t81/dpe/task_runner.hpp)
  - [epoch_commit.hpp](/include/t81/dpe/epoch_commit.hpp)
  - [thread_pool.hpp](/include/t81/dpe/thread_pool.hpp)
- RFC-linked conformance and determinism tests exist:
  - [task_graph_test.cpp](/tests/cpp/dpe/task_graph_test.cpp)
  - [task_runner_test.cpp](/tests/cpp/dpe/task_runner_test.cpp)
  - [epoch_commit_test.cpp](/tests/cpp/dpe/epoch_commit_test.cpp)
  - [epoch_dag_test.cpp](/tests/cpp/dpe/epoch_dag_test.cpp)
  - [epoch_parallel_test.cpp](/tests/cpp/dpe/epoch_parallel_test.cpp)
  - [thread_pool_test.cpp](/tests/cpp/dpe/thread_pool_test.cpp)
  - [dpe_float_reduction_test.cpp](/tests/cpp/dpe/dpe_float_reduction_test.cpp)

**Judgment**

`DPE` is the clearest relocation candidate. Its code already presents as a
first-class subsystem and the remaining caution is governance scope, not
implementation immaturity.

**Proposed relocation**

- Move public headers to `include/t81/dpe/`
  - `task_graph.hpp`
  - `task_runner.hpp`
  - `epoch_commit.hpp`
  - `thread_pool.hpp`
- Move implementations to `src/dpe/`
- Move tests to `tests/dpe/` or `tests/cpp/dpe/`
- Leave the namespace as `t81::dpe`
- Keep classification as `Governed non-DCP`

**Required compatibility plan**

- Move immediately and update all in-repo includes atomically.
- Update CMake target paths without changing target identity if possible
  (`t81_dpe` can remain the target name).
- Update `experimental/README.md` and the experimental inventory to remove DPE
  from the experimental list.

#### 2. Narrow T81Graph Surface

**Current evidence**

- [IMPLEMENTATION_MATRIX.md](/docs/status/IMPLEMENTATION_MATRIX.md)
  classifies `T81Graph` as implementation maturity `Beta`, promotion state
  `Governed non-DCP`.
- [SYSTEM_STATUS.md](/docs/status/SYSTEM_STATUS.md)
  describes `T81Graph` as useful and implemented, but not yet a verified
  deterministic surface as a whole.
- Stable graph type already exists in
  [T81Graph.hpp](/include/t81/types/T81Graph.hpp).
- The language/runtime symbolic graph path is now implemented under
  [symbolic_graph.hpp](/include/t81/cog/v1/symbolic_graph.hpp)
  and used in [vm.cpp](/vm/vm.cpp).
- BG-09 evidence indicates the symbolic graph serialization gap was closed in
  [BG-09_IMPLEMENTATION_EVIDENCE_T81GRAPH_SERIALIZATION.md](/docs/records/audits/BG-09_IMPLEMENTATION_EVIDENCE_T81GRAPH_SERIALIZATION.md).

**Judgment**

`T81Graph` is only partly a relocation candidate. The stable container type is
already outside `experimental/`, but the symbolic graph runtime surface is still
bundled with the cognitive-tier experimental tree.

The repo should not promote the entire cognitive tier stack merely because the
graph subset is in better shape. The right move is a narrow extraction of the
graph runtime surface only.

**Proposed relocation**

- Introduce a stable/non-experimental graph runtime API path, for example:
  - `include/t81/cog/v1/symbolic_graph.hpp`, or
  - `include/t81/graph/symbolic_graph.hpp`
- Move the matching implementation out of
  `experimental/tiers/cog/tier1/symbolic.cpp` into:
  - `src/cog/v1/symbolic_graph.cpp`, or
  - `src/graph/symbolic_graph.cpp`
- Preserve namespace `t81::cog::v1` initially to avoid a larger semantic break.
- Leave the rest of the cognitive-tier surfaces under `experimental/`.

**Important boundary**

This move should apply only to:

- `SymbolicAtom`
- `SymbolicEdge`
- `RewriteRule`
- `RewriteProgram`
- `RewriteExecutionResult`
- `ConfluenceReport`
- `SymbolicGraph`

It should **not** automatically move:

- tier 2+ headers
- promotion orchestration
- planner/self-model/distributed monad surfaces
- broader cognitive-tier architecture claims

## Not Ready To Move

### Hanoi VM

Do not move.

Evidence:

- [ALPHA_PROMOTION_PLAN.md](/experimental/hanoi/ALPHA_PROMOTION_PLAN.md)
  still identifies command-surface, test, and documentation work as promotion
  blockers.
- [EXTENSION_PROFILE.md](/docs/status/EXTENSION_PROFILE.md)
  still classifies `experimental/hanoi/` as experimental.

### Cognitive Tiers Broadly

Do not move.

Evidence:

- [EXTENSION_PROFILE.md](/docs/status/EXTENSION_PROFILE.md)
  still classifies cognitive tiers as experimental.
- [experimental-tiers.md](/docs/architecture/layers/experimental-tiers.md)
  continues to describe these tiers as experimental/stubbed.

Only the narrow symbolic graph subset is a credible extraction candidate.

### Distributed Compute

Do not move.

Evidence:

- [EXPERIMENTAL_SURFACE_INVENTORY.md](/docs/records/status-history/EXPERIMENTAL_SURFACE_INVENTORY.md)
  still describes the distributed surface as stubbed and non-deterministic by
  design.
- [EXTENSION_PROFILE.md](/docs/status/EXTENSION_PROFILE.md)
  still classifies `experimental/distributed/` as experimental.

### Trace-JIT

Do not move.

Evidence:

- [EXTENSION_PROFILE.md](/docs/status/EXTENSION_PROFILE.md)
  still classifies `runtime/jit/jit_compiler.cpp` as experimental / stub.
- [JIT_EQUIVALENCE_GAP.md](/docs/records/JIT_EQUIVALENCE_GAP.md)
  explicitly says no DCP promotion is intended without full equivalence proofs.

## Rollout Plan

### Phase 1: DPE Relocation

1. Create `include/t81/dpe/` and `src/dpe/`.
2. Move the four public DPE headers to `include/t81/dpe/`.
3. Move the four DPE implementation files to `src/dpe/`.
4. Update CMake sources and test include paths.
5. Update docs so DPE is removed from experimental inventories and appears as a
   governed non-DCP subsystem outside `experimental/`.
6. The in-repo cutover can be performed atomically without retaining
   `experimental/dpe/*` shims.

### Phase 2: Narrow T81Graph Runtime Extraction

1. Create a stable header path for symbolic graph runtime APIs.
2. Move the tier-1 symbolic graph header and implementation to that new path.
3. Update `vm/vm.cpp` and tests to include the new stable path.
4. Leave the remaining cognitive-tier files untouched in `experimental/`.
5. Update status docs to clarify that only the symbolic graph runtime moved, not
   the cognitive tiers as a whole.

### Phase 3: Governance and Boundary Cleanup

1. Update [EXTENSION_PROFILE.md](/docs/status/EXTENSION_PROFILE.md)
   so `Experimental Headers` no longer overstate the promoted paths.
2. Update [EXPERIMENTAL_SURFACE_INVENTORY.md](/docs/records/status-history/EXPERIMENTAL_SURFACE_INVENTORY.md)
   to remove relocated surfaces.
3. Update [FROZEN_CORE_PROFILE.md](/docs/status/FROZEN_CORE_PROFILE.md)
   only to clarify exclusions and governed non-DCP placement, not to claim DCP
   promotion.
4. Add a decision-log entry recording that relocation was structural and did not
   change DCP status.

## Required Tests Before And After Move

### DPE

- Build/link tests for the relocated headers and sources.
- Re-run all existing DPE RFC tests unchanged.
- Add one include-path regression test proving public includes now resolve via
  `t81/dpe/...`.

### T81Graph

- Re-run:
  - [symbolic_vm_test.cpp](/tests/cpp/symbolic_vm_test.cpp)
  - [tier1_rewrite_confluence_test.cpp](/tests/cpp/tier1_rewrite_confluence_test.cpp)
  - [tiers_structure_test.cpp](/tests/cpp/tiers_structure_test.cpp)
  - graph/language determinism fixtures tied to `graph_canonical`
- Add an include-boundary test proving the symbolic graph runtime resolves via
  `include/t81/cog/v1/` for the moved subset.

## Risks

### Risk 1: False Promotion Signaling

Moving code out of `experimental/` can be misread as "now verified".

Mitigation:

- Update docs in the same change.
- Use the phrase `Governed non-DCP` in migration notes and status diffs.
- Do not add the moved surfaces to the Determinism Surface Registry as
  `Verified`.

### Risk 2: Over-Promotion Of Cognitive Tiers

Extracting `SymbolicGraph` could accidentally blur the boundary with the rest of
 the cognitive-tier stack.

Mitigation:

- Move only the narrow symbolic graph subset.
- Leave tier 2+ and tier-promotion orchestration in `experimental/`.
- Record the scope explicitly in the decision log.

### Risk 3: Include-Path Breakage

Consumers may include current experimental paths directly.

Mitigation:

- Provide a temporary forwarding include layer if external consumers exist.
- Otherwise perform one atomic repo-wide include rewrite.

## Recommendation

Proceed with:

1. Immediate relocation of `DPE` out of `experimental/`.
2. Narrow extraction of the symbolic `T81Graph` runtime subset out of
   `include/t81/experimental/cog/tier1/`.

Do not proceed with relocation of:

- Hanoi VM
- cognitive tiers broadly
- distributed compute
- trace-JIT

## Open Questions

1. Should the stable symbolic graph path live under `t81/cog/v1/` to preserve
   conceptual continuity, or under `t81/graph/` to separate it from the rest of
   the cognitive stack?
2. Resolved by implementation on 2026-03-19: the repo absorbed a clean
   internal include-path break immediately.
3. Should `experimental/README.md` continue to list `tiers/` once the symbolic
   graph subset is extracted, or should it call out that only tier 2+ remains
   experimental?
