# T81 Contributor Roadmap

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Contributor Roadmap](#t81-contributor-roadmap)
  - [Authoritative references](#authoritative-references)
  - [Current research posture](#current-research-posture)
    - [Evidence classifications](#evidence-classifications)
  - [Phase 1: Establish trustworthy experimental comparators & Gate 1 qualifications](#phase-1-establish-trustworthy-experimental-comparators-&-gate-1-qualifications)
    - [Active Research Tracks](#active-research-tracks)
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
  - [AI-Native Deterministic Control Plane Trajectory](#ai-native-deterministic-control-plane-trajectory)
    - [Completed / Pinned Baseline (`v2.2.0` → `main@8aa086906`)](#completed--pinned-baseline-`v220`-→-`main@8aa086906`)
    - [Engineering Milestones (Gated by Verification & Maintainer Authorization)](#engineering-milestones-gated-by-verification-&-maintainer-authorization)
    - [Horizon D: Stratified Neural-Symbolic AI OS (`Deterministic` / `Stochastic` / `Hybrid`) — *Planned / Design Target*](#horizon-d-stratified-neural-symbolic-ai-os-`deterministic`--`stochastic`--`hybrid`-—-*planned--design-target*)
    - [Closed / Falsified Hypotheses & Active Research Holds (Do Not Re-Open Without New Controls)](#closed--falsified-hypotheses-&-active-research-holds-do-not-re-open-without-new-controls)
  - [Near-term execution sequence](#near-term-execution-sequence)
  - [Long-term research direction](#long-term-research-direction)

<!-- T81-TOC:END -->


**Engineering Maintenance, Experimental Validation, and Research Priorities**

**Updated:** 2026-11-26. Live `main` observed at `8aa0869063185a8a250200742b229911793d3419` (post-`v2.2.0`, Waves 0–4 and `H06-MEM-04` Phase 3 merged). This date refreshes the AI-Native Deterministic Control Plane trajectory and near-term queue below. It does not authorize `SCG-08` Gate 2, close `H01`/10A, adopt `F01`, or change `H01–H08`.

This roadmap connects the engineering maintenance program to T81's active scientific research.

T81 Foundation is an experimental computing research platform investigating balanced-ternary representation, alternative execution architectures, deterministic computation, artifact integrity, authorization, and potential hardware-level advantages.

The central scientific question remains unresolved:

**Can ternary representation or computation demonstrate a reproducible, practically meaningful advantage over an appropriately optimized conventional alternative under equivalent conditions?**

The purpose of this roadmap is to help contributors identify work that advances that question without expanding architectural scope prematurely.

## Authoritative references

- [T81 Foundation Overview](../README.md)
- [Master Continuity Charter](research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md)
- [RFC-00D2 — Target Architecture and Sequencing](../spec/rfcs/RFC-00D2-daios-target-architecture-and-sequencing.md)
- [Determinism Surface Registry](governance/DETERMINISM_SURFACE_REGISTRY.md)
- [SCG-08 Phase 0 Research Charter](research/continuity/SCG08_PHASE0_RESEARCH_CHARTER.md)
- [SCG-08 Phase 1 Gate 1 Implementation Report](../experiments/scg08_arithmetic_work/GATE1_IMPLEMENTATION_REPORT.md)
- [AI Research Continuity Handoff](ai-context/ACTIVE_RESEARCH_HANDOFF.md)

The continuity charter and canonical H01–H08 register remain authoritative for research lineage and hypothesis identity.

This roadmap does not supersede historical findings, alter accepted experimental thresholds, or convert architectural proposals into implementation commitments.

## Current research posture

T81 has accumulated substantial experimental infrastructure across languages, virtual machines, model execution, storage, provenance, and hardware exploration.

However, several proposed ternary-specific advantages remain unsupported or unresolved.

Current evidence includes:

- **Packed ternary computation:** Tested CPU implementations have not demonstrated general throughput superiority over optimized INT8 baselines (e.g., historical AVX2 INT8 17.15 GFLOP/s vs packed 2-bit ternary 12.91 GFLOP/s).
- **TISC compactness:** The tested complete TISC artifact was substantially larger than its conforming WebAssembly comparator.
- **Hardware Investigation 07:** The reported area ratio did not meet the preregistered advantage threshold; ternary dot-product hardware feasibility is currently classified as NOT SUPPORTED.
- **Investigation 08:** Conventional comparator demonstrates bounded computational parity across declared valid scenarios, with assurance supported and ternary-specific computational advantages not supported.
- **Cross-Profile Execution (CPA-004/005):** Demonstrated bit-exact execution across hosted T81VM and bare-metal AArch64 QEMU profiles on a frozen 351-byte canonical program, establishing execution correctness without asserting performance advantage.
- **SCG-08 Carry Propagation & Arithmetic Work:** Phase 1 Gate 1 harness is implemented and verified against an arbitrary-precision exact oracle, but Gate 2 empirical benchmarking remains **strictly withheld**.
- **Physical Evaluation (Investigation 10A):** Physical evaluation is blocked because matched device-characterization data are unavailable. This does not establish a universal physical disadvantage or close H01.
- **Governed Execution Comparison (`H07` — Closed 2026-11-27):** Classified **`PARTIALLY SUPPORTED (REGIME-BOUNDED)`** via reproducible head-to-head benchmark (`T81 v2.3.0` vs `Wasmtime 51.0.0 + OPA 1.23.0 + OpenSSL 3.6.2`, [H07_WASMTIME_OPA_COMPARISON.md](research/H07_WASMTIME_OPA_COMPARISON.md)). Both stacks achieved `100%` (`3/3`) fail-closed fault rejection; T81 won on footprint (`5.48 MB` vs `83.36 MB`), daemon RSS (`3.38 MiB` vs `40.14 MiB`), `p99` policy tail latency (`298 µs` vs `446 µs`), and integrated in-binary receipts, while `Wasmtime + OPA` won decisively on raw compute (`8.52 ms` vs `157.11 ms`, `18.4×`), cold-start (`54.23 ms` vs `168.22 ms`, `3.1×`), and `p50` policy-only latency (`152.1 µs` vs `166.1 µs`).

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

## Phase 1: Establish trustworthy experimental comparators & Gate 1 qualifications

**Priority: Immediate**

**Primary goal:** Ensure that T81 and conventional baselines perform equivalent, independently verifiable work before architectural comparisons are interpreted.

### Active Research Tracks

1. **SCG-08 Gate 1 Review:**
   - The Phase 1 Gate 1 arithmetic harness in `experiments/scg08_arithmetic_work/` implements multi-precision balanced-ternary and binary addition and multiplication across 5 operand bit-widths (64 to 1024 bits).
   - Independent verification against `ExactBigIntOracle` across 5 dimensions (numerical equality, overflow classification, canonical ternary digits, correct sign handling, absence of truncation).
   - Operation accounting strictly distinguishes non-aggregating diagnostic counters and normalization primitive passes from the frozen weighted metric $W_{\text{ops}}$.
   - **Gate 2 Authorization:** Execution of full-scale statistical benchmark campaigns remains strictly withheld until explicit maintainer review and gate elevation.
2. **Investigation 08 Parity:**
   - Maintain admission check parity and intermediate tensor verification for bounded attention workloads.
   - Negative tests confirm invalid models are rejected before task artifact persistence.
3. **H04-R03 Matched Codec Protocol:**
   - Preserve Phase C freeze and bounded registered-object retention boundaries.
   - Maintain mathematical parity distinctions without claiming latency or throughput advantage.

### Success criteria

- All experimental harnesses pass independent oracle verification and schema checks.
- Invalid models and out-of-budget runs fail closed.
- Negative tests and boundary conditions are explicitly verified.
- The declared regression gate passes.

## Phase 2: Measure subsystem advantages under controlled conditions

**Priority: Authorized experiments only**

**Primary goal:** Determine whether any observed difference is attributable to ternary representation or computation rather than implementation language, unequal functionality, or experimental design.

### Experimental dimensions

Evaluate separately:

1. Multi-precision arithmetic and carry-propagation dynamics.
2. Representation and storage density.
3. Memory traffic and data movement.
4. Model loading and artifact processing.
5. Authorization and provenance overhead.
6. Integrated end-to-end execution.

Where applicable, distinguish ternary-specific effects from general runtime engineering improvements.

### Measurement requirements

Each experiment must define:

- A falsifiable hypothesis.
- A comparable conventional baseline.
- Workload and correctness equivalence.
- Primary and secondary metrics.
- Hardware and software environment.
- Warmup, repetition, and randomization procedures.
- Measurement uncertainty.
- Acceptance and stopping criteria.
- Raw observations and reproduction commands.

A preliminary measurement-protocol check is not equivalent to a statistically supported performance conclusion. Gate 2 campaigns must strictly respect pre-registered trial budgets.

### Success criteria

- At least one bounded comparison reaches a documented scientific disposition.
- Results can be reproduced independently.
- Performance, storage, and assurance effects are reported separately.
- Negative findings are retained.
- Unsupported ablations are not presented as completed experiments.

A negative conclusion is a valid outcome.

## Phase 3: Resolve the physical ternary feasibility questions

**Priority: Evidence-dependent**

**Primary goal:** Identify whether a realistic physical device regime could support advantages unavailable to software emulation.

The mathematical information density of three-state representation does not by itself establish an advantage in physical memory or logic.

### Current limitation

Investigation 10A established that matched device-characterization data were insufficient to complete physical evaluation:

**PHYSICAL EVALUATION BLOCKED — MATCHED CHARACTERIZATION DATA UNAVAILABLE.**

This is an evidence limitation, not a universal physical no-go finding.

### Recommended work

- Identify candidate device technologies and their operating assumptions.
- Obtain independently traceable characterization sources.
- Require matched comparisons for capacity, energy, latency, area, retention, and error behavior where relevant.
- Include sensing, encoding, decoding, peripheral circuitry, and error correction.
- Reconcile historical ternary-memory simulation findings.
- Distinguish simulated, modeled, measured, and experimentally demonstrated results.
- Avoid claiming device-level feasibility from idealized radix efficiency.

### Success criteria

- A clearly specified physical hypothesis has sufficient matched evidence for evaluation; or
- The blocking data requirements are documented precisely enough for future experimental work.

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

This phase favors stabilization over new subsystem breadth.

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
- Do not treat merged experimental harnesses as authorization for full benchmarking campaigns.
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

## AI-Native Deterministic Control Plane Trajectory

Alongside the scientific phases below, engineering execution is structured around T81's demonstrated strength as a **local, deterministic, fail-closed AI-agent control plane and ternary-semantic runtime on binary hosts**:

### Completed / Pinned Baseline (`v2.2.0` → `main@8aa086906`)
- **Frozen Core (`DCP Verified`):** `TISC v1.9.0-Stable`, `PUBLIC_API_LOCK.json` `v2.2.0`, `T81VM` interpreter + NEON/AVX2 Dual-Rail `(P, N)` `TMatMul` + `SBDR1T2D` CanonFS caching (`t81_dualrail_sbdr_vm_test`), and `T3_K` / `T81Lang` reproducibility gates.
- **Bounded Control Plane:** Fail-closed `Axion` policy engine (`axion_root.apl`), `CanonFS` (`RFC-00D1` v1 seed), 3-member Bounded AI OS-Object family (`assess-fixed`/`route-fixed`/`classify-fixed`), and 15-tool native C++ MCP Airlock (`t81 mcp serve`, `--root` jail, `tx_begin`/`tx_commit`/`tx_abort`, `23/23` `ai_usability_eval.py` trials, TLA+ `TernaryOSTransactions` TLC `14,227,489` states / `0` errors).
- **Provider & Regime Foundations:** `M1 ProviderGovernor` (`t81.provider.evidence.v1`, `AT-01..AT-10`, PR `#865`), `H06-MEM-04` Phase 3 sparse-aware Apple Metal/CPU dispatch (`PR #882`), native `x86_64` AVX2 off-L3 DRAM sweep (`PR #864`), and `t81==2.2.0` Python wheels (`31` PyPI files, `cp39–cp314`).

### Engineering Milestones (Gated by Verification & Maintainer Authorization)
- **Milestone A (Immediate):** Native C++ Ed25519 bundle signing & DAG verification (`src/crypto/`), `M2` Persistent MCP Receipt Ledger (`ProviderGovernor` + `CanonFS` receipts in `tx_commit`/`run_subgraph`), and formal maintainer ratification of `RFC-00D4` and `RFC-00D6`.
- **Milestone B (Near-Term):** `M3` Python SDK governed provider execution + zero-copy buffer protocol & `@` Dual-Rail matmul (`v2.3.0`), 2B+ scale BitNet Safetensors streaming `SBDR1T2D` validation, and `H06-MEM-04` `RegimeSelector` promotion with discrete GPU backend abstraction (`Governed non-DCP` per `RFC-0051`).
- **Milestone C (Medium-Term):** `DPE` (`include/t81/dpe/`) level-parallel DAG unification with MCP transactional isolation (verified in TLA+), and `TernaryOS` AArch64 (`RFC-00C6..00CE`) packaged as a bounded ephemeral headless security enclave runner (`Governed non-DCP`, never a general-purpose OS).

### Horizon D: Stratified Neural-Symbolic AI OS (`Deterministic` / `Stochastic` / `Hybrid`) — *Planned / Design Target*
Architecture blueprint: [`docs/architecture/AI_OS_STRATIFIED_BLUEPRINT.md`](architecture/AI_OS_STRATIFIED_BLUEPRINT.md) (governed by `DEC-004` in [`docs/ai-context/DECISIONS_AND_GOVERNANCE.md`](ai-context/DECISIONS_AND_GOVERNANCE.md); none of the neural OS helpers below are currently implemented or authorized for production):

- **D1 — Advisory Learned Prototypes (Plane 2, Planned):** Evaluate compact (`< 100 MB` resident footprint) learned predictors (regime/batch predictor or trajectory anomaly classifier) strictly behind `RegimeSelector` (`R0`–`R4`) and fail-closed `Axion` (`{-1, 0, +1}`) gates, with compiled zero-dependency deterministic fallbacks in Planes 3/0.
- **D2 — Bounded Learned Controllers (Plane 3, Planned):** Promote only components that prove a measurable, statistically verified CI win over static heuristics while preserving `100%` bit-exact `State A → State C` fallback parity, Ed25519 `RFC-00D4` receipt sealing, and zero direct `Plane 2 → Plane 0` write authority.

### Closed / Falsified Hypotheses & Active Research Holds (Do Not Re-Open Without New Controls)

| Hypothesis / Hold | Disposition | Measured / Documentary Basis |
| :--- | :--- | :--- |
| **Software CPU Ternary Addition (`H01`/`H02`)** | **FALSIFIED & FROZEN (STOP)** | `t81_radix_comparison`: 4-trit tryte add `11.7 ns` vs 64-bit binary ALU `1.1 ns` (`10.6×` slower). |
| **In-Cache Packed Ternary MatVec (`H03`/`H06`)** | **FALSIFIED for CPU speed** | `t81_packed_ternary_matvec`: `12.91` vs `17.15 GFLOP/s` INT8 AVX2 (`75.3%` due to `VPSHUFB` decode tax); retained for `75%` storage compression only. |
| **TISC Bytecode Compactness (`Inv 09-E1`, `H03`)** | **NOT SUPPORTED** | Complete TISC binary (`1,240 B`) is `7.898×` larger than conforming WebAssembly MVP (`157 B`). |
| **`T81-AIOPT-001` M1 / M2** | **M1 FALSIFIED · M2 FROZEN** | M1 4-trit catalog filtering reduces to $O(1)$ scalar lookup; M2 execution remains frozen (`7` P1 blockers in PR `#673`). |
| **`H01` / Investigation 10A (Physical Silicon)** | **PHYSICAL EVALUATION BLOCKED** | No matched 3-state semiconductor cell characterization data available; no physical ternary silicon exists in-repo. |
| **`F01-HOST-BOLQ-v1`** | **NOT ADOPTED (`M1–M6 STOP`)** | Residual physical-overrun risks not accepted; `linux_relock_required=true`; PR `#812` `UNSUPPORTED` retained. |
| **`SCG-08` Gate 2 & `AXION-AI-001`** | **HELD (`NO-GO` / `HD01–HD06` open)** | Gate 2 empirical benchmarking strictly withheld; PR `#859` explicitly held by maintainer. |

## Near-term execution sequence

Observed 2026-11-26 against `main` at `8aa086906`. Rows are ordered for the next maintenance slice. Scientific holds stay in force: `SCG-08` Gate 2 remains withheld, `H01`/10A remains blocked, `AXION-AI-001` `HD01–HD06` remain open, and `F01` M1–M6 remain stopped.

| Order | Work | Completion condition |
|---|---|---|
| 1 | RFC-00D1 policy-profile depth on the current seed | Denial stays before host materialization. A fully denied directory export does not create its destination, a denied file export preserves existing bytes, and a partial export writes only admitted entries. No new profile names or interchange formats. |
| 2 | Independent review of SCG-08 Phase 1 Gate 1 harness | Schema validations and smoke tests pass. This review does not authorize Gate 2. |
| 3 | Record Gate 2 authorization criteria for SCG-08 | A formal maintainer decision exists before any large-scale run. |
| 4 | Investigate one subsystem-specific ternary effect | One falsifiable bounded result, with the matched control named in advance. |
| 5 | Revisit physical feasibility where matched evidence becomes available | Supported physical disposition or a documented block. |
| Ongoing | Runtime, CanonFS, CI, and contributor maintenance | Existing contracts remain reliable. Platform CI on `120b8dc90` was still running sanitizer and macOS x86_64 GCC builds when this queue was refreshed; do not treat an in-progress run as a green `main`. |

Retired from this queue on 2026-11-22: reconcile PRs #747, #748, and #750. All three were closed unmerged (#747 on 2026-11-13, #748 and #750 on 2026-11-15). Do not reopen them as the next action.

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
