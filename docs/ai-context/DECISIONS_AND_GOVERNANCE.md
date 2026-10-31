# T81 Research Continuity — DECISIONS_AND_GOVERNANCE

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Research Continuity — DECISIONS_AND_GOVERNANCE](#t81-research-continuity-—-decisions_and_governance)
  - [1. Documented Decisions, Contracts and Deferred Questions](#1-documented-decisions-contracts-and-deferred-questions)
    - [1.1 Contract and Planning Inventory (Not a Record of New Acceptance)](#11-contract-and-planning-inventory-not-a-record-of-new-acceptance)
    - [1.2 Rejected, Inconclusive and Deferred Alternatives](#12-rejected-inconclusive-and-deferred-alternatives)
  - [2. Active RFC Status Catalog](#2-active-rfc-status-catalog)
  - [3. Consequential Pull Request Lineage](#3-consequential-pull-request-lineage)
  - [4. Governance Boundaries & Human Authorization Requirements](#4-governance-boundaries-&-human-authorization-requirements)
    - [4.1 Local Prototype vs. Independent Enforcement](#41-local-prototype-vs-independent-enforcement)
    - [4.2 Explicit Human Approval Thresholds](#42-explicit-human-approval-thresholds)
  - [5. Constraints on Automated Research Agents](#5-constraints-on-automated-research-agents)
  - [6. How to Amend or Supersede Documented Decisions](#6-how-to-amend-or-supersede-documented-decisions)
  - [7. Dated governance and PR disposition addendum — 2026-10-29](#7-dated-governance-and-pr-disposition-addendum-—-2026-10-29)
  - [8. Operational governance and publication boundaries — 2026-10-31](#8-operational-governance-and-publication-boundaries-—-2026-10-31)
  - [2026-10-31 authorization clarification](#2026-10-31-authorization-clarification)

<!-- T81-TOC:END -->


**Document ID:** `docs/ai-context/DECISIONS_AND_GOVERNANCE.md`
**Version:** 1.0.1 · **Last Reviewed:** 2026-10-29
**Source Snapshot:** `main` at `f8ac77159ded2c4b93f4cc83abd48f6f35655468` (checked 2026-10-29; refresh before use)
**Status:** Non-normative decision and RFC-status index; does not grant acceptance, merge or activation authority

---

## 1. Documented Decisions, Contracts and Deferred Questions

### 1.1 Contract and Planning Inventory (Not a Record of New Acceptance)

The AD identifiers below are continuity-index labels, not substitutes for formal acceptance records. A draft RFC remains draft even when a partial implementation or reference example exists.

| Index ID | Domain | Evidence-bounded posture | Source |
| :--- | :--- | :--- | :--- |
| **AD-001** | **DAIOS Target Architecture** | [RFC-00D2](../../spec/rfcs/RFC-00D2-daios-target-architecture-and-sequencing.md) is **draft/informational**. Its six layers guide sequencing (*design ahead, implement behind proof*); no broader interface is accepted by this index. | [RFC-00D2](../../spec/rfcs/RFC-00D2-daios-target-architecture-and-sequencing.md) |
| **AD-002** | **Deterministic Math** | [RFC-0030](../../spec/rfcs/RFC-0030-deterministic-math-subsystem.md) is **Accepted** as a specification. This does not independently certify all CORDIC functions or universal bit-exact replay across hosts. | [RFC-0030](../../spec/rfcs/RFC-0030-deterministic-math-subsystem.md) |
| **AD-003** | **CanonFS Interchange** | [RFC-00D1](../../spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md) is **draft overall**. Its narrow v1 JSON import/export seed has implementation and tests; broader filesystem interchange remains unadmitted. | [RFC-00D1](../../spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md) |
| **AD-004** | **Bounded AI OS-Object Family** | Preserve the documented admitted compositions `assess-fixed`, `route-fixed`, and `classify-fixed` under their exact baseline contracts. No fourth composition is authorized here. | [Stable baseline contract](../reference/STABLE_BASELINE_CONTRACT.md) |
| **AD-005** | **Detached Accountable Evidence** | [RFC-00D4](../../spec/rfcs/RFC-00D4-accountable-decision-bundle.md) is **Draft**. Experimental detached receipts and verifiers exist, but schema presence or passing local tests do not constitute RFC adoption, conformance, or production authority. | [RFC-00D4](../../spec/rfcs/RFC-00D4-accountable-decision-bundle.md); [precursor report](../../examples/governance/accountable-computation/REPORT.md) |

---

### 1.2 Rejected, Inconclusive and Deferred Alternatives

| Decision ID | Alternative / Proposal | Disposition | Rationale & Evidence |
| :--- | :--- | :--- | :--- |
| **RD-001** | **Software CPU Ternary ALU Acceleration** | **REJECTED & FROZEN (STOP)** | Benchmark `t81_radix_comparison` proved software tryte addition is $10.6\times$ slower than native binary ALU addition. CPU software ternary math is frozen. |
| **RD-002** | **Software SWAR SIMD MatVec Speedup** | **REJECTED for Execution Speed** | Benchmark `t81_packed_ternary_matvec` proved SIMD bit-shuffle decode instruction tax limits speed to $75.3\%$ of INT8 AVX2 in cache. Retained solely for storage compression. |
| **RD-003** | **Custom VM Requirement for Accountable Evidence** | **UNRESOLVED — comparative protocol incomplete** | [Precursor report](../../examples/governance/accountable-computation/REPORT.md) tested local A/B/C arms, **not** deployed Wasmtime/OPA. It observed unauthorized result creation in 0/150 A, 90/150 B and 0/150 C negative requests; cross-platform, complexity and full Baseline-X comparisons remain inconclusive. [CLM-03/04](../../experiments/research_001/CLAIM_REGISTER.md) remain under active test. |
| **RD-004** | **Commercial Desktop OS & Silicon Hardware Scope** | **DEFERRED / OUT OF SCOPE** | T81 is a hypothesis-driven research platform. Bare-metal kernel / TernaryOS ambitions are deferred until Layers 1–4 are stabilized. |

---

## 2. Active RFC Status Catalog

| RFC Identifier | Title | Current Status | Key Impact |
| :--- | :--- | :--- | :--- |
| [**RFC-0030**](../../spec/rfcs/RFC-0030-deterministic-math-subsystem.md) | Deterministic Soft-Float Trigonometry (CORDIC) | **Accepted** | Enables bit-exact cross-platform float determinism across x86_64, ARM64, and WASM. |
| [**RFC-00D0**](../../spec/rfcs/RFC-00D0-base81-aware-tcp-ip-stack.md) | Base-81 Aware TCP/IP Stack | **Draft / Design-Led** | Conceptual networking layer; restricted to narrow resolver prototypes. |
| [**RFC-00D1**](../../spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md) | CanonFS Foreign File System Interchange | **Draft (v1 Seed Stable)** | Standardizes content-addressed import/export seed contracts and manifests. |
| [**RFC-00D2**](../../spec/rfcs/RFC-00D2-daios-target-architecture-and-sequencing.md) | DAIOS Target Architecture & Sequencing | **Draft / Informational** | Target map and sequencing discipline; not an accepted runtime contract. |
| [**RFC-00D4**](../../spec/rfcs/RFC-00D4-accountable-decision-bundle.md) | Detached Accountable Evidence Verification | **Draft** | Proposed schema/protocol with bounded experimental evidence; no adopted conformance claim. |

---

## 3. Consequential Pull Request Lineage

| PR Number | Title / Objective | Impact / Outcome |
| :--- | :--- | :--- |
| **PR #551** | CanonFS Identity Experiment | Implemented content-addressed hash verification for storage objects. |
| **PR #552** | Integer Provenance Tracking | Bound numeric inputs to immutable CanonFS provenance chains. |
| **PR #553** | Representation & Conversion RFC | Formalized bit/trit conversion contracts and mathematical bounds. |
| **PR #554** | Witness Revocation Mechanism | Added HMAC witness revocation receipts to policy log streams. |
| **PR #560** | Witness Corrective Hardening | Addressed snapshot rollback vulnerabilities in SQLite witness stores. |
| **PR #561** | Foundation Phase 2 Advancement | Advanced core runtime interfaces towards DCP determinism targets. |
| **PR #565 / #566** | Governance Validation & Protocol 001 | Under review; CodeQL passed; marked NOT READY pending evidence gates. |
| **PRs #632–#637** | EXP-T81-COMP3-01 protocol/harness revisions | **Open/unmerged at this snapshot**, with divergent protocol blobs and incomplete acceptance. #634 head `21095d65cc0c849f5a6f2599a962cbce032b5cea`; #635 `b37005ceda6a3f4349ec9027b2a9be84ea176aa9`; #637 `1a71733cdd266a34a174d13185c78e5b6a07c68e`. Do not infer a frozen approved protocol from PR titles. |

---

## 4. Governance Boundaries & Human Authorization Requirements

### 4.1 Local Prototype vs. Independent Enforcement
- **Local Prototype:** Running T81 binaries, unit tests, or CTest scripts locally demonstrates software behavior; it does NOT constitute independent security certification or activated production governance.
- **Independent Enforcement:** Separate human authorization, accepted contracts, trustworthy CI enforcement and independently verifiable receipts would be required for activation. Draft [RFC-00D4](../../spec/rfcs/RFC-00D4-accountable-decision-bundle.md) does not itself activate cryptographic or multi-party controls.

### 4.2 Explicit Human Approval Thresholds
Automated agents (Jules, Codex, Claude, etc.) must seek explicit human authorization before:
1. Modifying accepted RFC documents in `spec/rfcs/`.
2. Altering canonical hypothesis definitions (H01–H08) in `HYPOTHESIS_REGISTER_v1.json`.
3. Promoting draft RFCs to accepted status.
4. Widening the scope of the protected bounded AI OS-object family.
5. Modifying CI enforcement scripts in `scripts/governance/` or `.github/workflows/`.

---

## 5. Constraints on Automated Research Agents

1. **Read-Only Posture Towards Historical Evidence:** Never delete, overwrite, or re-write historical negative results, failed tests, or archived CWEB reports.
2. **No Unbacked Claims:** Every assertion of performance or capability must cite an exact source path, commit SHA, and test command.
3. **Strict Codebase Verification:** Before declaring a plan step complete, verify file state using read-only tools (`read_file`, `list_files`).
4. **Preserve CI Formatting:** Run `clang-format-18 -i` on any modified C/C++ source or header files before submitting changes.

---

## 6. How to Amend or Supersede Documented Decisions

To amend or supersede a formally accepted architectural decision without erasing historical lineage (the draft index above is not itself an acceptance record):
1. **Never Delete:** Do not erase or overwrite the original decision entry in `DECISIONS_AND_GOVERNANCE.md`.
2. **Mark Superseded:** Change the status of the former decision to `SUPERSEDED BY AD-XXX` or `REVISED`.
3. **Create New Entry:** Record the new decision with a distinct ID, citing the empirical evidence, benchmark result, or accepted RFC that justified the change.


---

## 7. Dated governance and PR disposition addendum — 2026-10-29

**Scope:** This is a non-normative status correction, **not** a new accepted decision, governance activation, or authorization to merge. Historical PR-lineage statements above remain as dated source claims; this section is the more recent checked operational status.

| Subject | Rechecked disposition | Evidentiary limit |
| :--- | :--- | :--- |
| CMP3 #632 | Closed unmerged as exact document duplicate of #633 | File-level identity only; #633 remains open. |
| CMP3 #635 | Closed unmerged; original protocol preserved byte-identically on `main` | Preservation does not confer protocol approval. |
| CMP3 #636 | Closed unmerged as obsolete unexecuted prototype | No benchmark performance inferred. |
| CMP3 #637 | Closed unmerged as integrated/superseded | 80 of 83 changed paths match `main` by Git blob; three paths have different newer versions. |
| CMP3 #633, #634 | Open, distinct historical protocol versions | Do not assume their exact contents are preserved elsewhere. |
| CMP3 #642–#644 | Merged corrective lineage | Integration does not grant PMU/trusted-runner measurement authorization. |
| Governance #574–#577 | Open stacked correction sequence | #574 hosted Foundation CI failed; #577 complete required-gate success not established. |
| Governance #578–#580 | Open investigation/preparation stack | #580 typed contract and RFC-0057 remain draft/inactive. |
| Governance #617 | Open read-only acceptance audit | Author receipts: 18/27 pass, 9/27 fail; NOT ACCEPTED. |
| Governance #622 | Open V01 regression proposal | Nonmergeable at review; no automatic rebase or root allowlist widening. |
| Skill-policy #614 | Open draft | Ubuntu argument-boundary job passed; macOS job cancelled; workflow conclusion failure. |

**Maintainer decision boundary:** The nine reported governance failures include unresolved authority questions (root layout, API lock and RFC status). No AI agent may resolve them by silently changing expected outputs, weakening validators, changing protected branch rules, or treating a draft as accepted. A new independently executed 27-command inventory on the exact #577 cumulative head, plus full hosted CI and CodeQL evidence, is the next verification gate. The source of the historical 18/9 count is [#617](https://github.com/t81dev/t81-foundation/pull/617), not an independent rerun in this addendum.

**Operational change discipline:** For administrative PR cleanup, close only after verified content-level supersession and a reasoned public comment; closure is not a merge or scientific acceptance. No unattended GitHub Actions triage automation is authorized. Preserve append-only history, raw negative evidence, and exact approval boundaries.

---

## 8. Operational governance and publication boundaries — 2026-10-31

**Checked main:** `f775680aba7db200691c0f4e2a3174b0e7e51581`. All entries below are *current-state observations*, not newly accepted decisions. Preserve the older dated record above.

| Workstream | Checked or reported status | Maintainer gate |
| :--- | :--- | :--- |
| Archaeology [#651](https://github.com/t81dev/t81-foundation/pull/651) | Open, unmerged; head `caf9b3fc`; original inventory and unresolved scientific provenance | Avoid merging #651 alongside replacement #653 as duplicate independent authority. |
| Correction [#653](https://github.com/t81dev/t81-foundation/pull/653) | Open, unmerged; head `2229abf2`; CLM-003 still contains universal-negative wording at the inspected head | Require H04-R02 semantic correction, no unpinned Tier A, deterministic regeneration, independent regression review before disposition. |
| Original ADM-2 [#652](https://github.com/t81dev/t81-foundation/pull/652) | Open, unmerged; head `e2e5a25b`; combined maintenance/header-isolation history | Retain for historical traceability; do not merge the combined candidate. |
| Local ADM-2 candidate A | Codex reports `repair/adm2-baseline@0c22ac3`; not remotely published | Independent ancestry/diff and Linux/Windows/CI checks before integration. |
| Local ADM-2 candidate B | Codex reports `repair/adm2-m1@826887b`; stacked after A, not remotely published | Preserve compiler-versus-operational distinction; no engine activation. |
| Governance [#649](https://github.com/t81dev/t81-foundation/pull/649) | Draft, unmerged; head `3a28eae9` | Separate governance review; no check relaxation or automatic activation. |
| SDK proposal [#650](https://github.com/t81dev/t81-foundation/pull/650) | Draft, unmerged; head `137faac2` | No SDK version, RFC-00E9 acceptance or API-lock change without authorization. |

**Known reproduced-on-author-host diagnostics (Codex report, not independently rerun here):** Existing API lock checks fail on pinned baseline, A and B with version 1.9.1 versus 1.9.0 and digest mismatch; architecture-target discovery misses ten documented targets on all three. The inference worker fails to process a queued request in its reported diagnostic. Do not repair these through unrelated validator changes or include the runtime worker fixes in the ADM-2 M1 header-isolation scope.

**Autonomous AI optimization governance proposal (not accepted):** AI may investigate alternate algorithms/backend implementations with pinned experiments and deterministic equivalence checks under accepted RFC-0042–0049 and RFC-0051; changing normative semantics, stable APIs, protected checks, policy enforcement, or published evidence remains a separately approved architectural proposal. A future AI optimization protocol is **idea-stage only**, not an accepted RFC or operational self-modification authority. Experiments never auto-promote to integrated contracts.

---

## 2026-10-31 authorization clarification

Reported local ADM-2 candidates (`0c22ac323484e37cc85f95201bd4dc01729dedb4` and `826887b40c62de9c6c903f319bbfbe905baa9911`) are **not** evidence of acceptance, activation, merge or release. Selected local macOS ARM64 tests cannot authorize an expanded runtime contract. Reconcile actual GitHub PR heads, accepted RFCs, explicit maintainer decisions, and independent CI before promoting any claim. Previously recorded CMP3 approvals and blockers remain scoped to their own dated artifacts.
