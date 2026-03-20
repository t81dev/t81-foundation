# Mission Alignment Execution Plan

**Date:** 2026-03-19  
**Status:** Proposed  
**Owner:** @t81dev

## Problem Statement

The repository contains several strong subsystems, but the project still makes
more architectural claims than it consistently proves with executable evidence,
clear maturity boundaries, and operationally meaningful replay.

If the goal is to support high-impact, technically differentiating capability,
the next phase should not be feature expansion by default. It should be
reduction of ambiguity across:

1. deterministic vs governed vs experimental surfaces
2. normative contract vs implementation reality
3. CLI discoverability vs actual readiness
4. benchmark anecdote vs benchmark decision value
5. architecture narrative vs deployable and auditable system behavior

This proposal defines a 90-day execution plan to make the strongest T81
surfaces more credible, more auditable, and easier to defend as real technical
advantage rather than aspirational architecture.

## Goals

- Reduce overclaim and contract drift across core system surfaces.
- Increase replayability and evidence quality for core workflows.
- Tighten governance and provenance boundaries around persistence, policy, and
  execution.
- Shrink the gap between the public CLI surface and the strongest supported
  workflows.
- Produce a smaller number of stronger claims, each backed by tests, docs, and
  reproducible evidence.

## Non-Goals

- This plan does not attempt broad new subsystem creation.
- This plan does not promote governed non-DCP surfaces into DCP by default.
- This plan does not assume experimental subsystems should be relocated unless
  their boundaries, evidence, and docs support it.
- This plan does not optimize for maximum benchmark numbers independent of
  determinism, auditability, and failure clarity.

## Strategic Thesis

The most valuable next step for T81 is not adding more architectural breadth.
It is converting the existing strongest surfaces into:

- clearly classified trust boundaries
- reproducible operational workflows
- evidence-backed governance claims
- intentionally pruned user-facing interfaces

That is the right preparation for serious users who care about technical
advantage, not just interesting implementation breadth.

## 90-Day Plan

### Phase 1: Days 0-30

#### 1. CanonFS Contract Closure

**Intent**

Resolve the remaining strategic ambiguity around CanonFS so the project can
state exactly what the current persistent store guarantees and what remains
future-versioned.

**Required work**

1. Review and refine
   [RFC-0054-canonfs-object-identity-and-persistence-contract.md](/spec/rfcs/RFC-0054-canonfs-object-identity-and-persistence-contract.md).
2. Finalize the current-release CanonFS contract in
   [canonfs-spec.md](/spec/supplemental/canonfs-spec.md).
3. Expand CanonFS contract testing beyond the new identity rule:
   - capability enforcement
   - corruption handling
   - reopen/persistence behavior
   - deterministic failure surfaces
4. Decide whether the long-term CanonFS direction is:
   - minimal deterministic object store, or
   - richer typed/capability/parity substrate with explicit migration work

**Primary files**

- [RFC-0054-canonfs-object-identity-and-persistence-contract.md](/spec/rfcs/RFC-0054-canonfs-object-identity-and-persistence-contract.md)
- [canonfs-spec.md](/spec/supplemental/canonfs-spec.md)
- [canonfs_identity_contract_test.cpp](/tests/cpp/canonfs_identity_contract_test.cpp)
- [canonfs_persistent_driver_test.cpp](/tests/cpp/canonfs_persistent_driver_test.cpp)
- [canonfs_integrity_matrix_test.cpp](/tests/cpp/canonfs_integrity_matrix_test.cpp)

**Success condition**

There is no remaining contradiction between CanonFS spec text, driver behavior,
CLI usage, and test expectations for the current release line.

#### 2. TernaryOS Promotion Audit

**Intent**

Evaluate whether any subset of
[experimental/ternaryos](/experimental/ternaryos)
is ready to leave `experimental/` under the same standard recently applied to
DPE and narrow T81Graph.

**Required work**

1. Inventory the public and semi-public TernaryOS surface.
2. Identify implemented vs aspirational vs build-gated behavior.
3. Review tests, docs, workflows, and maturity claims.
4. Produce an explicit relocation recommendation:
   - move now
   - move subset only
   - do not move yet

**Primary files**

- [experimental/ternaryos](/experimental/ternaryos)
- [IMPLEMENTATION_MATRIX.md](/docs/status/IMPLEMENTATION_MATRIX.md)
- [EXTENSION_PROFILE.md](/docs/status/EXTENSION_PROFILE.md)

**Success condition**

The repo has a documented yes/no answer on whether TernaryOS has a
promotion-ready subset.

#### 3. Governance Surface Register

**Intent**

Create one authoritative operational register for:

- deterministic core profile
- governed non-DCP
- experimental
- out-of-scope host behavior

**Required work**

1. Align the register to
   [RFC-0048-deterministic-surface-definition-and-governance-boundaries.md](/spec/rfcs/RFC-0048-deterministic-surface-definition-and-governance-boundaries.md).
2. Reconcile status pages and README claims against it.
3. Identify and remove any maturity overclaim.

**Success condition**

No lower-authority documentation can claim stronger guarantees than the
register without being obviously wrong.

### Phase 2: Days 31-60

#### 4. Replay Bundle Program

**Intent**

Make core workflows independently replayable and auditable by producing
first-class evidence bundles rather than scattered test outputs.

**Priority workflows**

1. `t81 code build`
2. `t81 code run`
3. `t81 determinism verify-run`
4. `t81 trace replay`
5. `t81 canonfs put/get/verify`
6. `t81 ai inference run`

**Bundle contents**

- exact inputs
- exact command
- expected outputs
- deterministic trace or declared non-deterministic fields
- exit-code expectations
- failure signature expectations

**Primary surfaces**

- [main.cpp](/tooling/cli/main.cpp)
- [driver.cpp](/tooling/cli/driver.cpp)
- [cli_contract_test.cpp](/tests/cpp/cli_contract_test.cpp)
- [cli_stress_test.cpp](/tests/cpp/cli_stress_test.cpp)

**Success condition**

A third party can rerun the strongest workflows without reconstructing hidden
assumptions from the source tree.

#### 5. Axion Hardening Pass

**Intent**

Strengthen the project’s highest-value governance surface by focusing on
negative-path determinism, audit completeness, and fail-closed behavior.

**Required work**

1. Expand deny-path coverage:
   - denied reads
   - denied writes
   - malformed policy
   - missing required event
   - runtime/policy mismatch
2. Ensure all important failures are explicit, deterministic, and test-covered.
3. Verify policy-visible side effects remain policy-visible in core workflows.

**Primary surfaces**

- [spec/rfcs/RFC-0003-axion-safety-model.md](/spec/rfcs/RFC-0003-axion-safety-model.md)
- [spec/rfcs/RFC-0022-axion-policy-language.md](/spec/rfcs/RFC-0022-axion-policy-language.md)
- [spec/axion-kernel.md](/spec/axion-kernel.md)

**Success condition**

The project can demonstrate deterministic governance failure behavior, not just
successful governed execution.

#### 6. Comparative Benchmark Program

**Intent**

Make benchmark output decision-useful rather than merely impressive.

**Required work**

1. Expand matched comparisons for:
   - direct vs DPE
   - policy-off vs policy-on
   - CanonFS-backed vs local artifact path
   - trace-off vs trace-on
   - governed AI path vs ungovened or reduced-governance path where applicable
2. Separate:
   - parallel-friendly wins
   - serialization-dominated flat spots
   - mixed-profile aggregate results

**Primary surfaces**

- [BM_DPE.cpp](/benchmarks/BM_DPE.cpp)
- [benchmark_runner.cpp](/benchmarks/runner/benchmark_runner.cpp)
- [benchmarks/README.md](/benchmarks/README.md)

**Success condition**

Benchmark documentation can answer when a feature helps, when it does not, and
what workload shape drives the difference.

### Phase 3: Days 61-90

#### 7. CLI Surface Pruning and Normalization

**Intent**

Reduce the difference between what the CLI advertises and what the team is
willing to defend as stable or meaningfully usable.

**Required work**

1. Hide or clearly mark internal-only and build-gated commands.
2. Reduce avoidable overlap where multiple command families do the same thing.
3. Keep machine-readable contracts consistent.
4. Preserve first-class `t81 ai ...` while preventing secondary drift between
   integrated and standalone AI entry points.

**Primary surfaces**

- [main.cpp](/tooling/cli/main.cpp)
- [ai_cli_shared.cpp](/tooling/cli/ai/ai_cli_shared.cpp)
- [cli-user-manual.md](/docs/user-guide/reference/cli-user-manual.md)

**Success condition**

The default CLI advertises a smaller, stronger, more coherent public surface.

#### 8. Experimental Relocation Round 2

**Intent**

Move only the next truly promotion-ready subset out of `experimental/`, if the
evidence supports it.

**Required work**

1. Use the TernaryOS audit result to determine whether any subset should move.
2. If yes, move code, docs, and tests atomically.
3. If no, document blockers and stop there.

**Success condition**

No ambiguous “half-promoted” subsystem remains without explicit status.

#### 9. Operational Profile Definition

**Intent**

Define supported build and runtime profiles so users can tell what guarantees
apply to what they built.

**Required profiles**

- hosted dev
- governed lab
- AI-enabled
- TernaryOS / kernel
- benchmark

Each profile should state:

- build flags
- supported commands
- determinism claims
- excluded features
- required evidence gates

**Success condition**

The supported profiles are explicit enough that build configuration no longer
acts as tribal knowledge.

## Priority Issues

### Critical

- CanonFS long-term product direction remains unresolved even after current
  contract narrowing.
- TernaryOS maturity and relocation status are still unproven.
- Governance class claims remain too distributed across status/docs surfaces.

### High

- Replay bundles are not yet first-class operational artifacts.
- Axion deny-path evidence can still be materially stronger.
- The CLI still advertises some surfaces more broadly than their maturity
  justifies.

### Medium

- Benchmarking is improving but still needs broader operational comparisons.
- Some documentation still reflects architectural ambition more strongly than
  current implementation proof.

## Suggested Work Breakdown

### Workstream A: Persistence and Provenance

- CanonFS contract closure
- CanonFS corruption and capability regression tests
- artifact identity and CLI naming cleanup

### Workstream B: Governance and Safety

- Axion negative-path hardening
- governance register publication
- maturity overclaim cleanup

### Workstream C: Tooling and UX

- CLI surface pruning
- profile definition
- replay bundle generation and docs

### Workstream D: Promotion and Benchmarking

- TernaryOS promotion audit
- comparative benchmark expansion
- relocation execution if justified

## Acceptance Criteria

- CanonFS current-release guarantees are explicit and test-backed.
- A governance surface register exists and lower-authority docs conform to it.
- Core workflows have replayable evidence bundles.
- Axion deny-path behavior is deterministic and test-covered.
- Benchmark docs explain tradeoffs, not just peak wins.
- The default CLI surface is narrower and more honest.
- A documented yes/no decision exists for the next experimental promotion
  candidate.

## Recommended Immediate Next Steps

1. Keep RFC-0054 in draft review and close the remaining CanonFS open questions.
2. Start the TernaryOS promotion audit.
3. Author the governance surface register before more status drift accumulates.
4. Define the replay bundle schema and apply it to one core CLI workflow first.

