# T81 Research Continuity — DECISIONS_AND_GOVERNANCE

**Document ID:** `docs/ai-context/DECISIONS_AND_GOVERNANCE.md`
**Version:** 1.0.0 · **Last Reviewed:** 2026-10-29
**Source Baseline Commit:** `f6d4d680709c5c6c98b305628e04346f5ef61620`
**Status:** Authoritative Architectural Decision Log & Governance Rules

---

## 1. Consequential Architectural Decisions

### 1.1 Accepted Architectural Decisions

| Decision ID | Title / Domain | Decision Summary | Source / RFC Reference |
| :--- | :--- | :--- | :--- |
| **AD-001** | **DAIOS Target Architecture** | Accepted 6-layer DAIOS target architecture map as a planning guide ([RFC-00D2](../../spec/rfcs/RFC-00D2-daios-target-architecture-and-sequencing.md)). Governed by: *design ahead, implement behind proof*. | [RFC-00D2](../../spec/rfcs/RFC-00D2-daios-target-architecture-and-sequencing.md) |
| **AD-002** | **Bit-Exact Cross-Platform Determinism** | Mandated soft-float CORDIC math ([RFC-0030](../../spec/rfcs/RFC-0030-deterministic-math-subsystem.md)) and DCP virtual state transition rules to ensure $100\%$ bitwise execution trace replay across x86_64, ARM64, and WebAssembly. | [RFC-0030](../../spec/rfcs/RFC-0030-deterministic-math-subsystem.md) |
| **AD-003** | **CanonFS Foreign File System Interchange** | Standardized CanonFS v1 JSON seed contracts (`t81.canonfs-import.v1`, `t81.canonfs-export.v1`) as the stable interchange mechanism for content-addressed artifacts ([RFC-00D1](../../spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md)). | [RFC-00D1](../../spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md) |
| **AD-004** | **Protected Bounded AI OS-Object Family** | Protected `assess-fixed`, `route-fixed`, and `classify-fixed` as the sole admitted baseline compositions. Scope expansion to a fourth composition is explicitly restricted. | [Contract Surface](../reference/STABLE_BASELINE_CONTRACT.md) |
| **AD-005** | **Detached Accountable Evidence Receipts** | Adopted detached evidence schema ([RFC-00D4](../../spec/rfcs/RFC-00D4-accountable-decision-bundle.md)) for pre-side-effect policy receipts and tamper-evident decision logs. | [RFC-00D4](../../spec/rfcs/RFC-00D4-accountable-decision-bundle.md) |

---

### 1.2 Rejected and Deferred Alternatives

| Decision ID | Alternative / Proposal | Disposition | Rationale & Evidence |
| :--- | :--- | :--- | :--- |
| **RD-001** | **Software CPU Ternary ALU Acceleration** | **REJECTED & FROZEN (STOP)** | Benchmark `t81_radix_comparison` proved software tryte addition is $10.6\times$ slower than native binary ALU addition. CPU software ternary math is frozen. |
| **RD-002** | **Software SWAR SIMD MatVec Speedup** | **REJECTED for Execution Speed** | Benchmark `t81_packed_ternary_matvec` proved SIMD bit-shuffle decode instruction tax limits speed to $75.3\%$ of INT8 AVX2 in cache. Retained solely for storage compression. |
| **RD-003** | **Custom VM Requirement for Accountable Evidence** | **REJECTED as Exclusive Requirement** | Baseline-X experiments proved Wasmtime + OPA host FFI boundaries deliver identical $0.0\%$ breach rates and detached evidence auditability. |
| **RD-004** | **Commercial Desktop OS & Silicon Hardware Scope** | **DEFERRED / OUT OF SCOPE** | T81 is a hypothesis-driven research platform. Bare-metal kernel / TernaryOS ambitions are deferred until Layers 1–4 are stabilized. |

---

## 2. Active RFC Status Catalog

| RFC Identifier | Title | Current Status | Key Impact |
| :--- | :--- | :--- | :--- |
| [**RFC-0030**](../../spec/rfcs/RFC-0030-deterministic-math-subsystem.md) | Deterministic Soft-Float Trigonometry (CORDIC) | **Accepted** | Enables bit-exact cross-platform float determinism across x86_64, ARM64, and WASM. |
| [**RFC-00D0**](../../spec/rfcs/RFC-00D0-base81-aware-tcp-ip-stack.md) | Base-81 Aware TCP/IP Stack | **Draft / Design-Led** | Conceptual networking layer; restricted to narrow resolver prototypes. |
| [**RFC-00D1**](../../spec/rfcs/RFC-00D1-canonfs-foreign-filesystem-interchange.md) | CanonFS Foreign File System Interchange | **Draft (v1 Seed Stable)** | Standardizes content-addressed import/export seed contracts and manifests. |
| [**RFC-00D2**](../../spec/rfcs/RFC-00D2-daios-target-architecture-and-sequencing.md) | DAIOS Target Architecture & Sequencing | **Accepted / Informational** | Target architecture map and sequencing discipline: *design ahead, implement behind proof*. |
| [**RFC-00D4**](../../spec/rfcs/RFC-00D4-accountable-decision-bundle.md) | Detached Accountable Evidence Verification | **Accepted** | Schema and protocol for detached, verifiable execution receipts. |

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
| **PR #634** | Preregistered Experimental Protocol (EXP-T81-COMP3-01) | Superseded PR #632; finalized 3-valued comparison protocol frozen at commit `7f83f813a05f8343b4884129f4bf84c099d81472`. |

---

## 4. Governance Boundaries & Human Authorization Requirements

### 4.1 Local Prototype vs. Independent Enforcement
- **Local Prototype:** Running T81 binaries, unit tests, or CTest scripts locally demonstrates software behavior; it does NOT constitute independent security certification or activated production governance.
- **Independent Enforcement:** Activated production enforcement requires independent cryptographic key authorization, detached witness receipts ([RFC-00D4](../../spec/rfcs/RFC-00D4-accountable-decision-bundle.md)), and multi-party policy verification.

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

## 6. How to Amend or Supersede Previous Decisions

To amend or supersede an architectural decision without erasing historical lineage:
1. **Never Delete:** Do not erase or overwrite the original decision entry in `DECISIONS_AND_GOVERNANCE.md`.
2. **Mark Superseded:** Change the status of the former decision to `SUPERSEDED BY AD-XXX` or `REVISED`.
3. **Create New Entry:** Record the new decision with a distinct ID, citing the empirical evidence, benchmark result, or accepted RFC that justified the change.
