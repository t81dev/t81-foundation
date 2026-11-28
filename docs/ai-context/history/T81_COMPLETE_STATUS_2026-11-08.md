# T81 Foundation — Comprehensive Research Status (2026-11-08)

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Foundation — Comprehensive Research Status (2026-11-08)](#t81-foundation-—-comprehensive-research-status-2026-11-08)
  - [1. Executive disposition](#1-executive-disposition)
  - [2. Repository and SCG state (observed 2026-11-08)](#2-repository-and-scg-state-observed-2026-11-08)
  - [3. SCG-04 decisions and SCG-05 feasibility](#3-scg-04-decisions-and-scg-05-feasibility)
  - [4. SCG-06 F1–F7 defect matrix — reported evidence, not completed fixes](#4-scg-06-f1–f7-defect-matrix-—-reported-evidence-not-completed-fixes)
  - [5. SCG-03 CI attribution: what is actually established](#5-scg-03-ci-attribution-what-is-actually-established)
  - [6. Other research tracks and maturity boundaries](#6-other-research-tracks-and-maturity-boundaries)
    - [Track F — deterministic execution, cross-profile TISC and ISA](#track-f-—-deterministic-execution-cross-profile-tisc-and-isa)
    - [Track R — representation and performance hypotheses](#track-r-—-representation-and-performance-hypotheses)
    - [Track M — research governance and AI-assisted auditability](#track-m-—-research-governance-and-ai-assisted-auditability)
  - [7. Current blockers and actionable next gates](#7-current-blockers-and-actionable-next-gates)
  - [8. Evidence and authority ladder](#8-evidence-and-authority-ladder)
  - [9. Reading order for a new AI instance](#9-reading-order-for-a-new-ai-instance)

<!-- T81-TOC:END -->


**Document ID:** T81-AI-CONTEXT-STATUS-2026-11-08
**Class:** Non-normative, dated research handoff; not an authorization, benchmark, certification, or independent reproduction of every historical experiment.
**Repository:** https://github.com/t81dev/t81-foundation
**Observed main:** `e45c5435aa47d4ccb8f343798214eb917352c871` (2026-11-08 GitHub inspection)
**Scope:** Program-level synthesis of existing ai-context, CPA/ISA/BASE research records, SCG-03 through SCG-06, and current GitHub PR/check evidence. Older statuses are historical and must not be overwritten.
**Refresh rule:** Recheck main, PR heads, workflows, governance and maintainer authorization before acting.

## 1. Executive disposition

T81 is a serious, falsifiable research program for balanced-ternary representation, arithmetic, deterministic execution, ISA/VM interoperability, and accountable AI-assisted engineering. It has **bounded positive software correctness and cross-profile execution evidence**, alongside **negative or conditional findings for several ternary-versus-binary performance and size hypotheses**. No general ternary superiority, physical ternary hardware advantage, production AGI, or universal ISA conformance is established.

The immediate engineering priority is **arithmetic correctness and exception-contract integrity**, not additional performance claims. SCG-03 repaired one documented `T81Int` overflow failure; SCG-06 reports that this change exposes a `T81Fraction::operator* noexcept` termination boundary (F6). A report of reproduction is not a completed repair. Other defects remain open.

**Current gate:** PR #733 is an open, unmerged evidence report requiring correction before acceptance; PR #732 is an open, unmerged generic-Result feasibility report. No production repair, ISA transition, performance measurement or merge is authorized by this context document.

## 2. Repository and SCG state (observed 2026-11-08)

| Item | Observed state | Boundary |
| --- | --- | --- |
| `main` | `e45c5435aa47d4ccb8f343798214eb917352c871` | Branch head observed, not an audit of all repository behavior |
| PR #729, SCG-03 | Merged; merge commit `2c3501e2807bb893076c5037231c3126b7f2bc72` | C05/C11 correctness repair; does not imply dependent fraction exception safety |
| PR #730 | Merged; formatting/admin correction followed SCG-03 | Distinguish later repair from behavior at original merge SHA |
| PR #731, SCG-04 | Merged; main now includes maintainer decision record | Decisions do not automatically authorize code changes |
| PR #732, SCG-05 | Open/unmerged; observed head `69b3b29447d827b50347fd4e2d3a140d9c174fcb` | Generic `Result<T,E>` feasibility, not implementation; refresh head before review |
| PR #733, SCG-06 | Open/unmerged; observed head `9d7bbd9490c7478f14a17682baf5708adf4da9e3` | Arithmetic reproduction and CI attribution; **revisions requested**, not accepted |
| Issue #717 | Arithmetic defect source | Issue allegations and PR reproductions must be distinguished |
| Codex PR #733 review | Failed to complete due to usage limits | **No completed Codex independent review**; do not call this a passed review |

Sources: https://github.com/t81dev/t81-foundation/pull/729 ; /pull/730 ; /pull/731 ; /pull/732 ; /pull/733 ; /issues/717 (all under same repository).

## 3. SCG-04 decisions and SCG-05 feasibility

The merged SCG-04 decision record selected:
- **D1 / 1B:** Dual-surface integer arithmetic; retain existing throwing operators while considering explicit checked APIs.
- **D2 / 2B:** Define binary64 conversion policy through RFC-00D5; defer API shape.
- **D3 / 3B:** Version the TISC ISA; preserve historical CPA-006 artifacts; defer DM-01 through DM-09 implementation decisions.
- **D4 / 4B:** Pursue a genuinely generic `Result<T,E>` preserving error type `E`, while retaining legacy `T81Result<T>`.

SCG-05 (PR #732) is **feasibility documentation only**. Its revision history explicitly corrected earlier overclaims. Two outstanding compatibility risks: `include/t81/types/Result.hpp` already exposes a one-parameter `Result<T>` alias, so introducing `Result<T,E>` with the same name requires a separately approved namespace/name/migration strategy; and `T81Error` copying may alter `occurred_at` and `fuel_spent`, so lossless metadata round-tripping cannot be assumed. Do not claim `std::expected` equivalence, `constexpr`, monadic behavior, or move-only support without proof.

## 4. SCG-06 F1–F7 defect matrix — reported evidence, not completed fixes

SCG-06 report and artifacts: https://github.com/t81dev/t81-foundation/pull/733 ; `docs/research/continuity/SCG06_ARITHMETIC_DEFECT_REPRODUCTION_REPORT.md` and `docs/research/continuity/evidence/scg06/` on the PR branch.

| ID | Component | Pre-repair `64941caf` | Post-repair `2c3501e` | Evidence/qualification |
| --- | --- | --- | --- | --- |
| F1 | `T81Limb27` Booth scaling | `1 × 2 → 27` | Same | Direct small counterexample reported; unresolved |
| F2 | `T81Limb54::reference_mul` | Invalid output trits in 300/300 scripted cases | Same | Script checks invalid digit ranges, **not exact mathematical product equality**; unresolved |
| F3 | `T81Limb54::karatsuba` | Carry-out observed/discarded in 66/200 scripted cases | Same | Script **does not call Karatsuba or compare final products with exact oracle**; narrow the claim or add actual end-to-end test |
| F4 | `T81Int<N>::operator*` | `T81Int<10>(243) × 243 → 0` silently | Caught `std::overflow_error` | Specific overflow case repaired; no universal correctness claim |
| F5 | `T81BigInt` subtraction | Debug abort; Release `borrow beyond MSB` exception | Same | Reproduction records both build modes; unresolved |
| F6 | `T81Fraction<81>::operator* noexcept` | `3^45 × 3^45` fraction numerator silently wraps to zero | Overflow exception crosses `noexcept` and aborts | **Confirmed failure-mode change in submitted reproduction**, not a completed safety repair |
| F7 | `T81Fraction::to_double() noexcept` | Large numerator conversion aborts | Same | Pre-existing unsafe exception boundary; unresolved |

The F2 script increments `failed_exact` without making an exact comparison; the F3 script increments `karatsuba_mismatch_count` without invoking the target multiplication. Their outputs cannot support a claim of independently measured end-to-end mismatch counts. The Python exact-integer oracle exists, but presence of an oracle file does not prove each test used it.

**Priority:** F6/F7 exception-boundary safety, followed closely by F5 termination/sign invariants and F1–F3 exact-product repairs. Prioritization is a recommendation, not implementation approval. Changes to `noexcept`, exception behavior, or `Result` API require compatibility review.

## 5. SCG-03 CI attribution: what is actually established

GitHub check-run API observations on 2026-11-08:
- PR #729 head `a35bd4d707b6d1ef4f8d7a65a9e492f4cd1d1962`: **5 failed check runs**.
- Merge commit `2c3501e2807bb893076c5037231c3126b7f2bc72`: **5 failed check runs** (38 check runs total).
- The five failed check names on the merge commit are `format / clang-format`, `lint / spec & docs`, `quality gate / required`, `Analyze (cpp)`, and `determinism / linux-x86_64`.
- **Formatting:** Run `42476947531`, job `128720301538`; direct log identifies formatting errors in SCG-03-touched `T81Int.hpp` and `test_t81int.cpp`. This is a **repair-induced code-hygiene failure** subsequently addressed, not proof of an arithmetic regression.
- **Documentation links:** Run `42476947508`, job `128720301959`; lychee HTTP 503 reported in SCG-06.
- **Quality gate:** Same run, job `128720364984`; dependent failure, not an independent root defect.
- **CodeQL C++:** Run `42476947461`, job `128720301336`; FTXUI fetch HTTP 504 reported.
- **Determinism Linux:** Run `42476947469`, job `128720301586`; **directly inspected job log** shows FTXUI v5.0.0 archive download HTTP 504 during CMake configure. No determinism test was executed. This is an external dependency-fetch failure, not demonstrated nondeterministic behavior.
- An external reviewer referred to **nine failing checks**. The exact nine check IDs/source aggregation have **not been established**. The two observed five-failure commit sets must not be casually combined or claimed to prove the number nine. Skipped/cancelled are not failed.

**Attribution conclusion:** At least one SCG-03-induced formatting failure is established; three jobs are associated with external HTTP errors (lychee, CodeQL FTXUI, determinism FTXUI), and one quality gate is dependent. No arithmetic regression was demonstrated by those five job outcomes. This is not proof of no arithmetic regression elsewhere: F6 documents a changed, unsafe runtime failure mode.

PR #733 currently overstates the nine-count explanation and calls the determinism failure an unspecified environment/configuration failure; these need revision.

## 6. Other research tracks and maturity boundaries

### Track F — deterministic execution, cross-profile TISC and ISA

Prior ai-context records report CPA-004/005 bounded execution on hosted T81VM and freestanding Axion OS AArch64 QEMU with an independent mathematical oracle. CPA-006 reports **19/19 frozen-oracle cases**, **57 hosted + 57 fresh AArch64 QEMU executions**, and **1,190 state records**. These are **source-reported bounded results**; this 2026-11-08 status update did not rerun them. They do not establish full ISA conformance or physical-hardware advantage. PR #710's earlier merge was to an experimental integration branch, **not automatically to main**.

ISA-001 found unresolved semantic discrepancies: direct vs register-indirect memory, immediate/register-derived branch targets and FLAGS predicates, TXor semantics, and extension opcode registry conflicts. The previously recommended T81-ISA-002 versioned state-transition RFC remains subject to maintainer decisions; SCG-04's D3 versioning choice does not authorize silent reinterpretation of historical 13-byte instruction records.

### Track R — representation and performance hypotheses

Retain negative/conditional evidence rather than resetting it:
- Historical Hanoi CWEB arithmetic probes: addition **422/547 failures**; multiplication **140/201 failures**; negation **27/27 passes** under the tested positional-integer interpretation.
- TISC vs conforming Wasm compactness E1: **1,240 vs 157 bytes**, ratio **7.898**; compactness hypothesis **not supported for that workload**.
- Packed ternary MatVec: reported **75% lower DRAM footprint** in the tested encoding, but CPU cache-bound throughput only **75.3% of INT8 AVX2** (`12.91` vs `17.15 GFLOP/s`); representation density is not compute speed.
- Software radix addition: **11.7 ns ternary vs 1.1 ns native binary**, approximately **10.6× slower** under the reported matched test; no software CPU ternary arithmetic speedup demonstrated.
- H04 adaptive representation results are conditional; do not claim a universal advantage. H04-R03 Phase C remains separately gated.
- BASE-01 physical Linux correctness qualification was **PHYSICAL_HOST_UNAVAILABLE** on a KVM guest; virtualized execution does not substitute for bare-metal qualification.
- Physical solid-state ternary storage / Investigation 10A remains **blocked by unverified or unmatched physical characterization evidence**, not scientifically disproven.
- Correct Base-81 information capacity is `log2(81) ≈ 6.33985` bits per four-trit digit; old `4.32`-bit claims are withdrawn. No speed/compression advantage follows from digit count alone.

Sources: `docs/ai-context/EVIDENCE_AND_FINDINGS.md`, `START_HERE.md`, `BASE01_2026-11-03.md`, `BASE81_LEGACY_CLAIMS_CORRECTION_2026-11-06.md`, and research continuity packets. These figures are historical, scope-bounded source reports, **not independently rerun in this update**.

### Track M — research governance and AI-assisted auditability

Maintain pinned baselines, raw evidence receipts, reproducible commands, exact mathematical oracles, correction logs, scope/authorization boundaries, and explicit separation of execution, passing tests, independent review, specification conformance, and comparative advantage. Codex quota exhaustion is an **unavailable review**, not a passing review. AI-authored documentation may contain overclaims and must be reviewed. SCG-05's self-corrections are evidence of useful review discipline, not evidence of runtime correctness.

## 7. Current blockers and actionable next gates

1. **PR #733 (SCG-06):** Correct F2/F3 oracle coverage, explicitly classify repair-induced formatting, explain determinism's FTXUI HTTP 504, reconcile or qualify the alleged nine checks, and make checksum file extension/format and worktree setup reproducible. Keep documentation-only. Then request bounded independent review and maintainer acceptance.
2. **PR #732 (SCG-05):** Resolve generic `Result<T,E>` naming collision and `T81Error` metadata-copy semantics in the feasibility record. Do not implement without approval.
3. **SCG-07 proposal:** Scope F6/F7 exception-contract safety and F5 invariant safety, with exact boundary regression tests, ABI/API review, and separately authorized implementation. Do not merge an unreviewed patch merely because a test run completes.
4. **ISA:** Preserve versioned-contract decisions, frozen CPA evidence, and explicit maintainer approval before any semantic transition.
5. **Scientific program:** Resume only preregistered, discriminating comparisons with matched optimized binary controls and real cost accounting. No performance claims from correctness-only runs.
6. **Physical validation:** Obtain genuine bare-metal hosts and matched characterization data before claiming BASE-01 or H01 qualification.

## 8. Evidence and authority ladder

**Execution completed ≠ tests passed ≠ mathematical correctness ≠ full specification conformance ≠ independent review ≠ physical or comparative advantage.**

Use: exact pinned source → isolated reproducible execution → independent oracle → declared coverage and counterexamples → regression attribution → bounded external review → maintainer decision. A document, bot summary, green CI badge, or merged PR alone cannot bridge missing steps.

**Non-authorizations:** This status report does not authorize merging PR #732/#733, implementing F1–F7 repairs, accepting RFC-00D5 or an ISA profile, Phase C, performance benchmarking, physical-hardware claims, or production activation.

## 9. Reading order for a new AI instance

1. This dated report (latest inspected snapshot).
2. `START_HERE.md`, `BOOTSTRAP.md`, `RESEARCH_IDENTITY.md`, `SYSTEM_ARCHITECTURE.md`.
3. `ACTIVE_RESEARCH_HANDOFF.md`, `DECISIONS_AND_GOVERNANCE.md`, `EVIDENCE_AND_FINDINGS.md`.
4. `CURRENT_STATE.json` **only as its selected dated 2026-11-02 operational inventory**, not as live state.
5. SCG-04 merged decision record, SCG-05 PR #732, SCG-06 PR #733 and Issue #717, plus the exact CI job logs listed above.
6. CPA/ISA/BASE and negative-experiment primary evidence before making domain-specific claims.

**Suggested first task:** Independently inspect the latest PR #733 revision, confirm whether F2/F3 tests now actually use an exact oracle, and reconcile the CI count without inventing the reviewer's missing records. Then return a bounded accept/revise decision. Do not re-run already settled administrative loops without a concrete contradiction.
