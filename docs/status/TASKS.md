# Active Tasks

Last Updated: 2026-03-04

Immediate, actionable items only. Structural hardening items live in `HARDENING_BACKLOG.md`.

---

## 🎯 PRIORITY 1: CRITICAL PATH (Next 30 Days)

### Production Readiness Sprint

- [ ] **BG-06 Collection Determinism** - **BLOCKER**
  - Implement `cli_stdlib_fixtures_test` for List/Map/Set/Tree determinism
  - Create `check_stdlib_surface_baseline.py` validation script
  - Target: 2026-03-18
  - Status: **CRITICAL** - Blocks T81Lang production readiness

- [ ] **AX-M5/M6/M7 Axion Beta Evidence** - **HIGH PRIORITY**
  - Map determinism stewardship transitions (AX-M5)
  - Document CanonFS observability evidence paths (AX-M6) 
  - Create complexity measurement evidence (AX-M7)
  - Target: 2026-03-25
  - Status: **TIME-SENSITIVE** - Beta review deadline 2026-04-30

- [ ] **T3K-S1 Quantization Specification**
  - Author `spec/t3k-quantization-spec.md` OR mark as governed non-spec
  - Target: 2026-03-15
  - Status: **QUICK WIN**

### Immediate Actions (Next 7 Days)

- [ ] **Day 1-2: Collection Determinism Framework**
  - Set up `cli_stdlib_fixtures_test` infrastructure
  - Implement basic List/Map/Set/Tree determinism tests

- [ ] **Day 3-4: Axion Evidence Collection**
  - Complete AX-M5 determinism stewardship mapping
  - Begin AX-M6 CanonFS observability documentation

- [ ] **Day 5-7: Quick Wins**
  - Resolve T3K-S1 quantization spec gap
  - Begin BG-07 BigInt precision governance decision

---

## 🎯 PRIORITY 2: PRODUCTION HARDENING (Next 60 Days)

### Core Infrastructure

- [ ] **BG-09 T81Graph Serialization** - **DCP BLOCKER**
  - Wire language runtime to invoke `serialize_canonical()` for T81Graph
  - Target: 2026-04-15
  - Status: **PREREQUISITE** for graph DCP candidacy

- [ ] **BG-07 BigInt Precision Resolution**
  - Governance decision: full precision vs spec narrowing
  - Implement chosen approach
  - Target: 2026-03-22
  - Status: **GOVERNANCE REQUIRED**

- [ ] **BG-08 Complex Number Serialization**
  - Implement round-trip binary serialization in `binary_io.cpp`
  - Target: 2026-03-29
  - Status: **TECHNICAL DEBT**

---

## 🎯 PRIORITY 3: GOVERNANCE & SCALING (Next 90 Days)

### Governance Improvements

- [ ] **GOV-01 Deputy Approval Policy**
  - Create `docs/governance/APPROVAL_DELEGATION.md`
  - Define deputy-owner criteria and delegation rules
  - Target: 2026-04-30
  - Status: **RISK MITIGATION**

- [ ] **FW-02 VM Policy Bridge Cleanup**
  - Extract `AxCheck`/`AxReport` helpers from dispatch loop
  - Reduce opcode dispatch concentration
  - Target: 2026-04-15
  - Status: **TECHNICAL DEBT**

### Strategic Enhancements

- [ ] **Cognitive Tier Implementation**
  - Symbolic (T243): Graph rewriting, confluence checking
  - Reflective (T729): Justification chains, trace capture
  - Target: Begin Q2 2026
  - Status: **STRATEGIC**

- [ ] **CLI Workflow Expansion**
  - Enhance `compile`, `run`, `disasm`, `debug`, `trace replay`, `weights`
  - Improve developer experience
  - Target: Ongoing
  - Status: **ECOSYSTEM**

---

## Governance

### Monthly Cadence

- [ ] **C2 Month-Close:** Execute runbook on 2026-03-31. Stamp final outcomes in `docs/records/audits/2026-03-governance-review.md`.
  - Preflight: `python3 scripts/governance/c2_month_close_preflight.py`
  - Check: `python3 scripts/governance/c2_month_close_check.py`
  - Runbook: `docs/records/status-history/C2_MONTH_CLOSE_RUNBOOK_2026-03-31.md`

### Beta Preparation

- [ ] **Axion Beta Candidacy Review** - **2026-04-30**
  - Complete all AX-M* evidence items
  - Prepare Beta review presentation
  - Schedule governance review

---

## Recently Closed

| Task | Completed |
| :--- | :--- |
| **🏆 100% Test Success Achievement** - All 285/285 tests passing | **2026-03-04** |
| **CI Lychee Link Checking** - Fixed broken internal/external links | **2026-03-04** |
| **Parser Test Expectations** - Updated for new AST structure | **2026-03-04** |
| **Documentation Reference Updates** - All broken cross-references fixed | **2026-03-04** |
| **T81Lang Parser Operator Precedence** - Fixed to match T81 spec | **2026-03-03** |
| **Semantic Analyzer Narrowing Prevention** - Enhanced type safety | **2026-03-03** |
| **T81Lang Conformance Tests** - Fixed subprocess abortions | **2026-03-03** |
| **AST/IR Determinism Hash** - Updated for new parser behavior | **2026-03-03** |
| **CLI Tests** - Fixed check and pipeline tests | **2026-03-03** |
| **IR Snapshot Audit** - Updated precedence expectations | **2026-03-03** |
| Release candidate GO stamp on `1ec312e3` | 2026-02-28 |
| T81Lang promotion to Beta implementation maturity | 2026-02-26 |
| Test suite deduplication (−16 files, −1,780 LOC, 285/285 passing) | 2026-02-28 |
| BG-01..05 engineering backlog closure | 2026-02-25 |
| BG-10 determinism tests (Quaternion/Prob/Qutrit/Uint) | 2026-02-28 |
| Cognitive Tier 1..5 implementation | 2026-02-26 |
| Runtime JIT prototype uplift | 2026-02-26 |
| CLI `t81 trace export` (JSON/CSV) | 2026-02-26 |
| Debugger cognitive tier inspection | 2026-02-26 |
| Phase 3 behavioral conformance expansion | 2026-02-28 |
| RFC-0027 SE-M1–SE-M6: spec/conformance/ suite (24 programs, spec annotations) | 2026-03-01 |
| Conformance suite activation: 21/24 programs passing `t81 run` (RFC-0027 acceptance criterion met) | 2026-03-02 |
| Spec Suite Alignment Pass (6 docs: tisc, t81-data-types, t81vm, axion-kernel, cognitive-tiers, overview) | 2026-03-01 |
| stdlib Sprint 2 fixtures: std.io, std.sys, std.async, std.agent (7 fixtures, STDLIB_MODULES + snapshot) | 2026-03-01 |
