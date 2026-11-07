# T81-AIOPT-001 — M1/M2 reconciliation and decision gate

**Status:** PROPOSED — independent maintainer acceptance pending  
**Date:** 2026-11-01  
**Scope:** Evidence reconciliation and authorization boundaries; no performance claim or M2 execution authorization.

## Pinned evidence

- **M1 correctness candidate:** PR [#665](https://github.com/t81dev/t81-foundation/pull/665), commit `e113e75f9bbbcc3ed0cc75f1ed15ff188ee062db`. Verification report: `experiments/aiopt_001_fixed_catalog/M1_VERIFICATION_REPORT.md` at that commit.
- **Scientific assessment:** PR [#666](https://github.com/t81dev/t81-foundation/pull/666), corrected commit `a59f97d39ae9a20b954a76ba98ed6213b69f3c90`. Scientific disposition: `M2_REDESIGN_REQUIRED`.
- **Historical correction:** `experiments/h04_r02_adaptive_representation/RESULTS.md` on main. The original H04-R02 inference is invalidated; conditional symbolic-model AT advantages remain, and H04 is unresolved.

The above PRs are independent proposed changes, not merged accepted research merely because this reconciliation document references them. Recheck their heads, review state and workflow results before acceptance.

## What M1 supports

Under the reconstructed, prospective C40 four-trit exact-equality contract, Codex reports agreement with a separate arithmetic reference across all 81 inputs and 3,240 input-pattern decisions per implementation, 53,096 checks across four verifier processes, malformed-input and error-precedence tests, and normal/-O deterministic output. It reports zero final failures and four corrected defect categories. Evidence is local macOS ARM64/Python 3.14.3 and has prior-reviewer-context disclosure.

**Interpretation:** a bounded, locally verified correctness result for equivalent encodings. Repeated checks are not independent samples. This document does not independently reexecute those checks or declare independent-person reproduction.

## What M1 does not support

- Performance, energy, throughput, latency, memory-bandwidth or hardware superiority.
- SIMD or SWAR lane-level parallel execution: the current packed equality predicate is equivalent to scalar integer equality.
- AI-guided selection, adaptive representation, or generalization to larger vectors.
- Recovery or acceptance of the earlier purported 22-record/1,890-case historical corpus.

The C40 catalog is the first 40 lexicographic vectors of `{-1,0,+1}^4`; exact matching has at most one hit, and the entire result can be computed by `k < 40` for the base-three ordinal `k`. It is a useful correctness fixture, **not** a discriminating optimization workload.

## M1 acceptance gate

**Current state: READY_FOR_INDEPENDENT_ACCEPTANCE (recommendation only).**

A maintainer or authorized independent reviewer must:
1. Pin and inspect the final PR #665 source and evidence SHA, including the v0.1.1 prospective API clarifications and 47-file evidence footprint.
2. Inspect all latest required CI and security checks, and resolve failures rather than relying on local reports.
3. Decide whether the reported single-host reproduction and reviewer independence boundary satisfy the project's acceptance policy; if not, request targeted independent reproduction.
4. Record an explicit **ACCEPT / CORRECTION_REQUIRED / BLOCKED** decision with evidence links and scope.

No acceptance is inferred from the existence of this document, PR approval, or successful checks alone.

## Scientific-review gate

PR #666 must receive a fresh review of the corrected commit, confirming:
- H04-R02 is represented by its corrected conditional symbolic findings, not superseded negative claims.
- The Jules review does not claim tests it did not execute or independent-person peer review it did not establish.
- Negative, inconclusive, and untested outcomes are distinct, and claims are limited to tested configurations.
- Any additional overgeneralized CPU-closure or timing assertions are bounded to cited evidence.

## M2 design-only authorization proposal

**Authorized by this document:** drafting and reviewing a preregistered protocol **after explicit maintainer approval**.  
**Not authorized:** implementation, benchmarks, AI training/selection, production integration, performance claims, or merging either upstream PR.

Design question: For which workload structures, if any, can packed ternary representations offer a measurable **end-to-end** benefit over optimized conventional binary encodings under equal semantics?

The M2 protocol proposal must:
- Define nontrivial patterns with don't-care/wildcard positions, multiple matches and a parameterized vector length/catalog size/density/batch grid; justify each workload using an application or explicit synthetic research purpose.
- Include conventional integer, byte/packed-binary, LUT/bitset where feasible, and optimized SIMD baselines; allow each comparator appropriate algorithms rather than forcing a 40-row scan.
- Separate encoded storage, working-set bytes, preprocessing, validation, encode/decode, matching and end-to-end costs; preserve identical output semantics.
- Specify platform/toolchain, data distributions and seeds, correctness oracle, predeclared primary metric, effect-size threshold, uncertainty method, multiplicity control and power.
- Require bounded positive, bounded negative, **INCONCLUSIVE** and **NOT TESTED** outcomes; never equate failure to reject with equivalence.
- Distinguish gains due to data packing, algorithm, compiler, memory locality, selection policy and ternary semantics; compare any learned selector with static, rule-based and exhaustive alternatives including selection overhead.
- Preserve H04-R02's corrected conditional findings without extrapolating them to CPU execution.

## Stop condition

Stop at a reviewable protocol and decision record. M2 execution requires a **separate explicit authorization** after the protocol's scientific and governance gates pass.
