# T81Lang Stable Promotion Evidence

**Promotion Date:** 2026-03-16
**Surface:** T81Lang Compiler + Specification
**Prior Status:** Beta (promoted 2026-03-15)
**Promotion Result:** ✅ **STABLE**

---

## Executive Summary

T81Lang specification is promoted from **Beta → Stable** (v1.2 Draft → v1.3 Stable).
All technical promotion criteria are met. A single governance hygiene item (translation
staleness) is formally waived as non-normative. 363/363 tests pass.

---

## Promotion Gate Criteria

| ID | Criterion | Status | Evidence |
| :--- | :--- | :--- | :--- |
| TG-01 | Governance hygiene (translations current) | ✅ **WAIVED** | Non-normative; zh-CN/es/pt-BR/ru deferred to 2026-Q2 (see §Waiver below) |
| TG-02 | Deterministic compilation verified | ✅ met | 21 fixtures; hash `c8a7a5e4879fefa1c469c60846ded76d09ceb730db7a3624a9966a3c0b0b8391`; CI gate `repro-ledger.yml` |
| TG-03 | Semantic conformance complete | ✅ met | 7/7 semantic conformance tests; edge-case suite passing |
| TG-04 | Comprehensive test coverage | ✅ met | 363/363 tests; 36/36 T81Lang-specific; 16+ frontend suites |
| TG-05 | All spec sections implemented | ✅ met | §0–§9 + Appendix A complete; §3.2 VM I/O channels defined (this promotion) |
| TG-06 | Spec-section traceability | ✅ met | `spec/t81lang_features.md` 100% covered; `test_t81lang_traceability_enforcement.cpp` passing |

---

## Changes Made for Stable Promotion

### 1. §3.2 VM I/O Channels — stub resolved

The single remaining "to be defined" placeholder in the spec was §3.2:

> "VM I/O channels (to be defined)"

**Now defined** as:

> VM I/O channels are accessed via `std.io` (`io_stream`, `io_net`) and `std.async`
> (`async_thread`, `async_promise`) functions, all of which require Tier 2 or above.
> Every I/O operation lowers to `AXREAD` or `AXSET` TISC opcodes and is intercepted
> by the Axion governance kernel before any side effect is committed. The `print`
> built-in is the sole unrestricted I/O surface; it lowers to the `PRINT` opcode and
> requires no tier annotation.

This is consistent with `spec/t81lang_features.md` (feature registry) and the Axion
policy model. No implementation changes were required — the definition formalises
existing behavior.

### 2. Spec version bump: v1.2 Draft → v1.3 Stable

Header line `Version 1.2 — Draft` → `Version 1.3 — Stable`.
Status field `Draft` → `Stable`. Last Revised → 2026-03-16.

---

## Translation Staleness Waiver (TG-01)

**Waiver Decision:** Granted 2026-03-16 by @t81dev

**Rationale:**
- Translated READMEs (zh-CN, es, pt-BR, ru) are **non-normative**. The authoritative
  specification is the English `spec/t81lang-spec.md`.
- Translation maintenance is a community effort; no automated enforcement is warranted
  for spec promotion gates on non-normative artifacts.
- Translation refresh is scheduled for **2026-Q2** as a standalone maintenance task.

**Scope:** This waiver applies only to TG-01 for the Beta → Stable promotion. It does
not waive translation requirements for future normative documentation.

---

## Spec Completeness at Promotion

| Section | Title | Completeness |
| :--- | :--- | :--- |
| §0 | Language Properties | 100% |
| §1 | Core Grammar | 100% |
| §2 | Type System | 100% |
| §3 | Purity and Effects | **100%** (§3.2 stub resolved) |
| §4 | Name Resolution | 100% |
| §5 | Compilation Pipeline + DCP | 100% |
| §6 | Control Flow Semantics | 100% |
| §7 | Axion Integration | 100% |
| §8 | Interoperability Summary | 100% |
| §9 | Standard Library (§9.1–9.6) | 100% |
| Appendix A | Formal Grammar (EBNF + precedence) | 100% |

---

## Test Evidence

| Suite | Count | Result |
| :--- | :--- | :--- |
| Full test suite | 363 | ✅ 363/363 pass |
| T81Lang conformance baseline | 7 | ✅ 7/7 pass |
| T81Lang conformance edge semantics | included above | ✅ pass |
| Frontend parser (Appendix A coverage) | included above | ✅ pass |
| Literal pool determinism | included above | ✅ pass |
| Traceability enforcement | included above | ✅ pass |
| E2E compile determinism | included above | ✅ pass |

---

## Deterministic Compilation Profile (§5)

Four invariants verified:

1. **Source-stability** — identical source produces identical AST
2. **Pipeline determinism** — each stage is a pure function of its inputs
3. **Literal-pool determinism** — literal interning order is stable across runs
4. **Control-flow lowering determinism** — CFG → TISC produces identical instruction sequences

CI gate: `scripts/ci/t81lang_repro_gate.py` (`repro-ledger.yml`). 21 fixture programs verified.

---

## Cross-References

- Prior: `docs/records/audits/T81LANG_BETA_PROMOTION_EVIDENCE_2026-03-15.md`
- Feature registry: `spec/t81lang_features.md`
- Spec: `spec/t81lang-spec.md` (v1.3 Stable)
- Implementation matrix: `docs/status/IMPLEMENTATION_MATRIX.md`
- Determinism surface: `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
