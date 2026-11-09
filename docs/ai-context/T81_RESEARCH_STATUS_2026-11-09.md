# T81 Foundation — AI Research Continuity Update (2026-11-09)

**Class:** Non-normative, dated handoff; not an independent rerun, experiment authorization, or normative amendment.
**Repository:** https://github.com/t81dev/t81-foundation
**Observed `main`:** `8dec43ac2a9a702c9ae54229cf6891ee63f285c0` (GitHub branch inspection, 2026-11-09).
**Primary evidence:** [SCG-07 Phase 6 qualification](../research/continuity/SCG07_PHASE6_CONSOLIDATED_QUALIFICATION.md), [PR #741](https://github.com/t81dev/t81-foundation/pull/741), preceding SCG-07 receipts and [SCG-06 reproduction](../research/continuity/SCG06_ARITHMETIC_DEFECT_REPRODUCTION_REPORT.md).
**Freshness rule:** Recheck current main, relevant PRs, authorization, and experiment documents before acting. This document supersedes older *operational next-step* instructions, not historical evidence.

## Executive handoff

**SCG-07 arithmetic correctness campaign is complete at its qualified scope.** PR #741 is merged at `8dec43ac2a9a702c9ae54229cf6891ee63f285c0`, integrating Phase 6's consolidated C++ qualification harness, CMake target, and evidence report. The report's **QUALIFIED WITH LIMITATIONS** is bounded to specified operations, contracts, tests, and configurations; it is **not** a proof that all T81 arithmetic is correct, a claim of ternary performance advantage, or a physical hardware qualification. The final PR head was `3e471695b15a5ee5e969f15c04df1a801c99b7b7`; the baseline used by its qualification was `ecb1e32b9458a1a1df51d7cdd4a19496c0584103`.

**Next proposed track:** SCG-08 Phase 0, a **documentation-only architectural-advantage research charter**. A Jules task was requested to reconcile existing H01–H08 evidence, select unresolved falsifiable hypotheses, define matched binary controls and cost models, and preregister one next experiment. **No SCG-08 PR, accepted charter, experiment execution, measurement authorization, or result is verified by this snapshot.** Refresh before inferring progress.

## Arithmetic finding disposition (F1–F7)

| ID | Component | Merged remedy or verification | Qualified boundary |
| --- | --- | --- | --- |
| F1 | `T81Limb27` multiplication | PR #738 repaired Booth-pattern scaling; Phase 6 independent exact-integer comparisons | Wide 54-trit product of two 27-trit operands |
| F2 | `T81Limb54::reference_mul` | PR #739 repaired carry normalization; Phase 6 exact-oracle and historical replay | Canonical product residue modulo `3^54`, **not** a full 108-trit product |
| F3 | `T81Limb54::karatsuba` | PR #740 verified correctness on post-F1/F2 baseline without arithmetic changes; Phase 6 historical replay | **CORRECT ON CURRENT BASELINE — historical causal attribution incomplete**; intermediate carry discard invariant only modulo `3^54` |
| F4 | `T81Int<N>::operator*` | PR #729 repaired overflow detection and throwing behavior | Exact representable result or `std::overflow_error`; inherited `assert()` tests may be inactive under `-DNDEBUG` |
| F5 | `T81BigInt` signed subtraction | PR #737 repaired negative-result fallback | Independently sampled signed subtraction and boundary cases; not exhaustive all magnitudes |
| F6 | `T81Fraction` exception propagation | PR #736 removed invalid `noexcept` boundaries | Catchable integer overflow in tested rational arithmetic paths |
| F7 | `T81Fraction::to_double()` | PR #736 removed terminating conversion path; Phase 6 added rational checks | Tested widths 13, 40, 41, 81, 324; **no universal relative-error or correctly-rounded binary64 guarantee**; RFC-00D5 / SCG-04 D2 deferred |

Phase 6 new harness: **2,000** deterministic random F1 pairs; **2,500** F2/F3 pairs; **300** SCG-06 historical F2/F3 cases (68 carry-loss and 232 no-carry-loss); **800** F5 random pairs; plus focused F4/F6/F7 and boundary checks. Prior **10,000-case** and **1,200-case** suites are historical/regression evidence and must not be silently counted as newly run Phase 6 trials. The suite uses unconditional checks active with `-DNDEBUG`. The Phase 6 report describes Debug, Release and ASan/UBSan runs; the standalone sanitizer invocation did **not** instrument the linked `libt81_core.a` archive, so its sanitizer conclusions are limited to the harness and header-defined paths. These execution results are **author-reported in the merged report**, not locally re-executed by this AI-context update.

## Decision and governance boundaries

- SCG-04 D1: dual throwing/checked arithmetic surfaces; **not** an implementation authorization.
- SCG-04 D2: binary64 conversion policy remains deferred to RFC-00D5.
- SCG-04 D3: versioned TISC profile and preservation of CPA historical artifacts; **no implicit ISA transition**.
- SCG-04 D4: genuinely generic `Result<T,E>` while preserving legacy `T81Result<T>`; **not** authorization to introduce an incompatible name/API. Review SCG-05 feasibility and collisions before any proposal.
- Preserve immutable historical receipts, test provenance, negative results, and distinct merged-vs-open status. A merged PR establishes integration, not universal correctness or independent replication.
- Physical BASE-01 qualification remains blocked where a bare-metal host is required; virtualized KVM execution is not equivalent evidence.
- H01 physical device advantage remains unsupported without matched semiconductor characterization. Previously observed negative/conditional ternary-vs-binary outcomes must be retained.

## Research program: next decision

The next scientific question is **whether balanced ternary offers an advantage under specified representation, algorithm, workload, physical-substrate, and cost assumptions**, compared against optimized binary controls. Separate:

1. **Mathematical information density:** `log2(3) ≈ 1.585` bits per trit is a radix identity, not a physical win.
2. **Software execution on binary CPUs:** include packing/unpacking, conversions, SIMD, carry propagation, instruction count, and data movement; do not extrapolate a hypothetical native-hardware advantage.
3. **Native ternary hardware:** explicit cell area, switching energy, reliability/noise margins, sensing and interconnect assumptions; unsupported values must remain assumptions.
4. **Existing historical results:** retain the packed ternary MatVec memory/throughput tradeoff, TISC-vs-Wasm artifact-size negative finding, H04 conditional outcomes, and BASE-01 physical host limitation. Verify source receipts and comparability before quoting quantitative outcomes.

**SCG-08 Phase 0 proposed deliverable:** `docs/research/continuity/SCG08_PHASE0_RESEARCH_CHARTER.md` (pending Jules). It should map existing H01–H08 identifiers, select a small unresolved hypothesis set, specify binary controls and falsifiers, and recommend one narrowly scoped preregistered experiment. **Documentation only: no benchmarks, no ISA change, no normative activation, no automatic merge.**

## Fast bootstrap for a new AI instance

1. Read **this dated overlay** first; older 2026-11-08 status and 2026-11-02 `CURRENT_STATE.json` selected view are historical snapshots, **not live blockers**.
2. Read [START_HERE](START_HERE.md), [ACTIVE_RESEARCH_HANDOFF](ACTIVE_RESEARCH_HANDOFF.md), [RESEARCH_IDENTITY](RESEARCH_IDENTITY.md), [SYSTEM_ARCHITECTURE](SYSTEM_ARCHITECTURE.md), and [DECISIONS_AND_GOVERNANCE](DECISIONS_AND_GOVERNANCE.md).
3. Read the [SCG-07 Phase 6 qualification](../research/continuity/SCG07_PHASE6_CONSOLIDATED_QUALIFICATION.md) and primary F1–F7 receipts, then inspect PR #741 and current `main`.
4. Before any new experimental work, inspect the H01–H08 register, preregistrations, negative results, and SCG-08 Phase 0 status.
5. Do **not** restart F1–F7 repair, reclassify F3's historical cause, assert universal F7 rounding, infer authorization from this document, or spend agent budget fixing unrelated CI failures.

**Recommended immediate action:** Await/review the SCG-08 Phase 0 Jules research-charter PR when it exists; judge its evidence and hypothesis selection independently. Keep SCG-04 policy formalization separate unless explicitly reprioritized.
