# T81 Research Continuity — DECISIONS_AND_GOVERNANCE

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
