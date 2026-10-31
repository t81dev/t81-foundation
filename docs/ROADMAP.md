# T81 Contributor Roadmap

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Contributor Roadmap](#t81-contributor-roadmap)
  - [Authoritative references](#authoritative-references)
  - [Current research posture](#current-research-posture)
    - [Evidence classifications](#evidence-classifications)
  - [Phase 1: Establish trustworthy experimental comparators](#phase-1-establish-trustworthy-experimental-comparators)
    - [Investigation 08 — Current acceptance gate](#investigation-08-—-current-acceptance-gate)
    - [Required next work](#required-next-work)
    - [Success criteria](#success-criteria)
  - [Phase 2: Measure subsystem advantages under controlled conditions](#phase-2-measure-subsystem-advantages-under-controlled-conditions)
    - [Experimental dimensions](#experimental-dimensions)
    - [Measurement requirements](#measurement-requirements)
    - [Success criteria](#success-criteria)
  - [Phase 3: Resolve the physical ternary feasibility questions](#phase-3-resolve-the-physical-ternary-feasibility-questions)
    - [Current limitation](#current-limitation)
    - [Recommended work](#recommended-work)
    - [Success criteria](#success-criteria)
  - [Phase 4: Stabilize the contributor-facing runtime](#phase-4-stabilize-the-contributor-facing-runtime)
    - [Recommended work](#recommended-work)
    - [Success criteria](#success-criteria)
  - [Phase 5: Harden the CanonFS interchange lane](#phase-5-harden-the-canonfs-interchange-lane)
    - [Existing seed surface](#existing-seed-surface)
    - [Recommended work](#recommended-work)
    - [Success criteria](#success-criteria)
  - [Phase 6: Evaluate narrowly scoped architectural extensions](#phase-6-evaluate-narrowly-scoped-architectural-extensions)
  - [Phase 7: Keep the repository reliable to operate](#phase-7-keep-the-repository-reliable-to-operate)
    - [Recommended work](#recommended-work)
    - [Success criteria](#success-criteria)
  - [What not to do next](#what-not-to-do-next)
  - [Contributor decision rule](#contributor-decision-rule)
  - [Near-term execution sequence](#near-term-execution-sequence)
  - [Long-term research direction](#long-term-research-direction)

<!-- T81-TOC:END -->


**Engineering Maintenance, Experimental Validation, and Research Priorities**

**Updated:** October 28, 2026

This roadmap connects the engineering maintenance program to T81's active scientific research.

T81 Foundation is an experimental computing research platform investigating balanced-ternary representation, alternative execution architectures, deterministic computation, artifact integrity, authorization, and potential hardware-level advantages.

The central scientific question remains unresolved:

**Can ternary representation or computation demonstrate a reproducible, practically meaningful advantage over an appropriately optimized conventional alternative under equivalent conditions?**

The purpose of this roadmap is to help contributors identify work that advances that question without expanding architectural scope prematurely.

## Authoritative references

- [T81 Foundation overview](../README.md)
- [Master Continuity Charter](research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md)
- [RFC-00D2 — Target Architecture and Sequencing](../spec/rfcs/RFC-00D2-daios-target-architecture-and-sequencing.md)
- [Investigation 08 — Comparator PR #602](https://github.com/t81dev/t81-foundation/pull/602)
- [Investigation 10A — Corrective Documentation PR #625](https://github.com/t81dev/t81-foundation/pull/625)

The continuity charter and canonical H01–H08 register remain authoritative for research lineage and hypothesis identity.

This roadmap does not supersede historical findings, alter accepted experimental thresholds, or convert architectural proposals into implementation commitments.

## Current research posture

T81 has accumulated substantial experimental infrastructure across languages, virtual machines, model execution, storage, provenance, and hardware exploration.

However, several proposed ternary-specific advantages remain unsupported or unresolved.

Current evidence includes:

- **Packed ternary computation:** Tested CPU implementations have not demonstrated general throughput superiority over optimized INT8 baselines.
- **TISC compactness:** The tested complete TISC artifact was substantially larger than its conforming WebAssembly comparator.
- **Hardware Investigation 07:** The reported area ratio did not meet the preregistered advantage threshold.
- **Investigation 08:** A corrected conventional comparator demonstrates bounded computational parity across declared valid scenarios, but independent review identified a remaining model-admission discrepancy.
- **Investigation 10A:** Physical evaluation is blocked because matched device-characterization data are unavailable. This does not establish a universal physical disadvantage or close H01.

These results are bounded to their tested workloads, implementations, and assumptions.

They do not establish that ternary computing is inherently inferior, nor do they demonstrate an architectural advantage.

### Evidence classifications

Contributors must distinguish:

- **Implemented:** Source code or an artifact exists.
- **Tested:** Defined tests have been executed.
- **Measured:** Observations were collected under a documented protocol.
- **Modeled:** Conclusions depend on explicit mathematical or simulation assumptions.
- **Inconclusive:** Available evidence does not resolve the hypothesis.
- **Unsupported:** Evidence fails to establish the stated claim.
- **Blocked:** Required evidence or experimental prerequisites are unavailable.

A passing regression suite does not automatically establish scientific validity.

A successful prototype does not automatically establish a performance advantage.

## Phase 1: Establish trustworthy experimental comparators

**Priority: Immediate**

**Primary goal:** Ensure that T81 and conventional baselines perform equivalent, independently verifiable work before architectural comparisons are interpreted.

### Investigation 08 — Current acceptance gate

PR #602 introduces an equivalent conventional comparator for a bounded model-dependent, two-layer attention workload.

Independent review at commit `7c61afa3ba0e97c3c800ca5cde567cf374d7a5cc` reported:

- PASS across 15 declared valid scenarios for intermediate tensors, scores, token selection, and classification.
- PASS for declared policy denials, tokenizer mutations, and selected malformed-model cases.
- PASS for binary CAS identity, tampering, evidence consistency, and fresh-process recovery.
- Reproduction of the measurement protocol using six pairs and 12 unique stores.
- FAIL for complete model-admission parity.

The identified discrepancy is that the comparator does not require five tensors mandated by production. A correctly re-encoded model missing one such tensor can be accepted and persisted by the comparator while production rejects it.

### Required next work

1. Correct the five missing mandatory-tensor admission checks in the existing PR.
2. Add differential negative tests for each omitted tensor.
3. Verify that rejected models do not produce comparator task artifacts.
4. Preserve all passing computational, authorization, CAS, and replay tests.
5. Obtain independent confirmation of the focused correction.
6. Record the final acceptance disposition before using the comparator as an experimental reference.

### Success criteria

- Production and comparator agree on all declared admission cases.
- Valid-model intermediate computation parity remains intact.
- Invalid models are rejected before task artifact persistence.
- The declared regression gate passes.
- An independent reviewer accepts the bounded comparator.
- The comparator's scope and limitations are documented.

**Exit condition:** The comparator is accepted for its explicitly defined workload.

This does not establish full Llama inference equivalence, continuous task-policy enforcement within the VM, or ternary architectural superiority.

## Phase 2: Measure subsystem advantages under controlled conditions

**Priority: After Phase 1 acceptance**

**Primary goal:** Determine whether any observed difference is attributable to ternary representation or computation rather than implementation language, unequal functionality, or experimental design.

### Experimental dimensions

Evaluate separately:

1. Arithmetic and tensor execution.
2. Representation and storage density.
3. Memory traffic and data movement.
4. Model loading and artifact processing.
5. Authorization and provenance overhead.
6. Integrated end-to-end execution.

Where applicable, distinguish ternary-specific effects from general runtime engineering improvements.

### Measurement requirements

Each experiment should define:

- A falsifiable hypothesis.
- A comparable conventional baseline.
- Workload and correctness equivalence.
- Primary and secondary metrics.
- Hardware and software environment.
- Warmup, repetition, and randomization procedures.
- Measurement uncertainty.
- Acceptance and stopping criteria.
- Raw observations and reproduction commands.

A preliminary measurement-protocol check is not equivalent to a statistically supported performance conclusion.

For Investigation 08, the previously proposed 30 paired observations per condition remain a planning target unless the experiment's approved protocol specifies otherwise.

### Success criteria

- At least one bounded comparison reaches a documented scientific disposition.
- Results can be reproduced independently.
- Performance, storage, and assurance effects are reported separately.
- Negative findings are retained.
- Unsupported ablations are not presented as completed experiments.

**Exit condition:** The research record supports a defensible conclusion for at least one specified workload and metric.

A negative conclusion is a valid outcome.

## Phase 3: Resolve the physical ternary feasibility questions

**Priority: Evidence-dependent**

**Primary goal:** Identify whether a realistic physical device regime could support advantages unavailable to software emulation.

The mathematical information density of three-state representation does not by itself establish an advantage in physical memory or logic.

### Current limitation

Investigation 10A established that matched device-characterization data were insufficient to complete the physical evaluation.

The correct disposition is:

**PHYSICAL EVALUATION BLOCKED — MATCHED CHARACTERIZATION DATA UNAVAILABLE.**

This is an evidence limitation, not a universal physical no-go finding.

### Recommended work

- Identify candidate device technologies and their operating assumptions.
- Obtain independently traceable characterization sources.
- Require matched comparisons for capacity, energy, latency, area, retention, and error behavior where relevant.
- Include sensing, encoding, decoding, peripheral circuitry, and error correction.
- Reconcile historical ternary-memory simulation findings, including unresolved comparator-handoff failures.
- Distinguish simulated, modeled, measured, and experimentally demonstrated results.
- Avoid claiming device-level feasibility from idealized radix efficiency.

### Success criteria

- A clearly specified physical hypothesis has sufficient matched evidence for evaluation; or
- The blocking data requirements are documented precisely enough for future experimental work.

**Exit condition:** A defensible bounded physical disposition or a documented unresolved evidence gap.

No hardware implementation schedule should be inferred from this phase.

## Phase 4: Stabilize the contributor-facing runtime

**Priority: Ongoing maintenance**

**Primary goal:** Make existing, demonstrable T81 functionality understandable and usable without requiring contributors to reconstruct the entire research history.

### Recommended work

- Maintain a clear root README and documentation entry points.
- Distinguish stable, experimental, draft, and historical interfaces.
- Provide reproducible build and smoke-test instructions.
- Keep supported execution examples synchronized with implementation.
- Document platform-specific requirements.
- Reduce duplicated status and roadmap material.
- Maintain handoff documentation without creating competing sources of truth.

### Success criteria

- A new contributor can identify the supported runtime path quickly.
- Examples execute as documented on supported environments.
- Known limitations are visible.
- Research claims link to their evidence.

This phase should favor stabilization over new subsystem breadth.

## Phase 5: Harden the CanonFS interchange lane

**Priority: Maintained, not an automatic research blocker**

**Primary goal:** Preserve and improve the RFC-00D1 interchange seed as a usable engineering contract.

### Existing seed surface

- `canonfs import` and `canonfs export`.
- `t81.canonfs-import.v1` and `t81.canonfs-export.v1`.
- Structured interchange `reason`, `kind`, and `code`.
- Explicit `provenance_schema` and `manifest_schema` result fields.
- Provenance and manifest schemas.
- `host-file` and `host-directory`.
- Current policy-profile names.
- Golden examples and frozen `v1/` fixtures.

### Recommended work

- Strengthen negative and edge-case tests.
- Clarify stable versus draft RFC-00D1 behavior.
- Improve policy-profile semantics when supported by concrete use cases.
- Maintain interoperability fixtures.
- Avoid widening the stable contract without evidence and review.

### Success criteria

Contributors can build against the documented seed contract without assuming that all RFC-00D1 proposals are implemented.

## Phase 6: Evaluate narrowly scoped architectural extensions

**Priority: Deferred until justified**

RFC-00D2 remains the target-architecture map, not an implementation schedule.

Potential future work includes:

- RFC-00D0 service descriptor and resolution prototypes.
- Experimental hardware/backend interfaces.
- Additional bounded model or execution workloads.
- Carefully isolated integration experiments.

These are candidates, not committed deliverables.

A new subsystem should be approved only when it can identify:

1. The hypothesis or engineering deficiency it addresses.
2. Its dependency on existing validated layers.
3. The smallest implementation that can answer the question.
4. A reproducible acceptance test.
5. A stopping criterion.

Avoid introducing a new architectural layer solely because the existing architecture can accommodate it.

## Phase 7: Keep the repository reliable to operate

**Priority: Continuous**

**Primary goal:** Maintain trustworthy builds, tests, documentation, and portability without allowing maintenance work to displace the scientific agenda.

### Recommended work

- Keep macOS, Linux, and Windows checks meaningful.
- Fix confirmed regressions with narrow changes.
- Maintain deterministic test fixtures.
- Preserve historical research artifacts.
- Identify inherited CI failures separately from candidate-induced failures.
- Avoid repeated broad governance rewrites without new material evidence.
- Keep tooling and dependency requirements explicit.
- Prefer focused corrections over repeated speculative restructuring.

### Success criteria

- Failures have clear ownership and reproduction paths.
- Research results are not invalidated by undocumented environment differences.
- Contributors can reproduce supported tests.
- Maintenance tasks do not repeatedly interrupt independent experimental review.

## What not to do next

- Do not claim general ternary superiority without controlled evidence.
- Do not treat mathematical radix efficiency as proof of hardware advantage.
- Do not conflate runtime assurance with ternary-specific computation.
- Do not expand several draft subsystems simultaneously.
- Do not rewrite accepted interfaces merely because the repository is large.
- Do not treat a passing CI suite as sufficient scientific validation.
- Do not revive invalidated performance comparisons.
- Do not turn blocked physical research into an unsupported no-go conclusion.
- Do not repeatedly reopen resolved governance issues without new evidence.
- Do not make the OS or bare-metal vision the only public explanation of T81.

## Contributor decision rule

Prefer work that produces one of four outcomes:

1. **Resolves a falsifiable research question.**
2. **Improves the reproducibility of an existing experiment.**
3. **Makes a supported subsystem easier to use or independently verify.**
4. **Removes a demonstrated correctness, integrity, or portability defect.**

When multiple tasks compete, prioritize experimental validity and correctness over additional architectural breadth.

Each proposed task should identify:

- Target subsystem or hypothesis.
- Existing evidence.
- Known limitations.
- Required change.
- Validation procedure.
- Expected deliverable.
- Stopping criterion.

If these cannot be stated clearly, narrow the task before implementation.

## Near-term execution sequence

| Order | Work | Completion condition |
|---|---|---|
| 1 | Correct Investigation 08 model-admission parity in PR #602 | All five negative cases and regression gate pass |
| 2 | Obtain focused independent acceptance | Explicit bounded ACCEPT disposition |
| 3 | Establish controlled paired performance measurements | Raw evidence and defensible statistical analysis |
| 4 | Investigate subsystem-specific ternary effects | At least one falsifiable bounded result |
| 5 | Revisit physical feasibility where matched evidence becomes available | Supported physical disposition or documented block |
| Ongoing | Runtime, CanonFS, CI and contributor maintenance | Existing contracts remain reliable |

## Long-term research direction

The long-term objective is not simply to produce a larger ternary software ecosystem.

It is to determine which, if any, properties of ternary representation and computation justify their use over conventional alternatives.

Possible outcomes include:

- A demonstrated advantage for a specific workload or device regime.
- A representation advantage without a computational advantage.
- A useful deterministic runtime whose benefits are independent of ternary arithmetic.
- A negative result that establishes the limits of a proposed approach.
- A physical hypothesis that remains unresolved pending new device technology.

Each is a legitimate research result when supported by evidence.

**T81 advances when uncertainty is reduced—not merely when implementation scope increases.**
