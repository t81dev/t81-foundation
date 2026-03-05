# Hardening Backlog

Last Updated: 2026-03-05
Owner: @t81dev

**This is not a feature backlog.**

Only these categories belong here:
- Structural hardening (firewall enforcement, dependency isolation)
- Determinism tightening (UB fixes, canonicalization, serialization wiring)
- Scope reduction (removing DCP overclaim, retiring unverified surfaces)
- Promotion evidence (closing drift gaps to enable registry status upgrades)

Feature work, new capabilities, and API additions live elsewhere.

---

## Open Items

| ID | Category | Surface | Gap | Acceptance Criteria | Owner | Target | State |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| BG-06 | Determinism tightening | `T81List`, `T81Map`, `T81Set`, `T81Tree` | No run-to-run determinism tests for collection types; populated-constructor determinism unverified | **✅ COMPLETED - T81Map/T81Set use canonical sorted vector storage; T81_DETERMINISTIC enforcement; explicit iteration order guarantees; cross-platform determinism tests passing** | @t81dev | 2026-05-15 | **Closed** |
| BG-07 | Scope reduction / determinism tightening | `T81BigInt` | VM aliases BigInt to 64-bit; >64-bit literals silently truncated — spec claims arbitrary-precision | Native arbitrary-precision opcode path OR explicit governance decision to narrow spec claim. Phase-2 planning notes captured in `docs/status/BG07_PHASE2_IMPLEMENTATION_NOTES.md` (2026-03-05); scaffolding slice landed for ISA/program BigInt literal transport (`LiteralKind::BigIntHandle`, `Program::bigint_pool`, emitter/IO wiring), VM `LOADI` now materializes BigInt literals as canonical `FractionHandle` (`BigInt/1`), and frontend oversized decimal/base81 integer literals now lower to `BigIntHandle` instead of hard overflow failure. Remaining gap: full binary round-trip conformance coverage for BigInt literal pools. | @t81dev | 2026-05-15 | Open |
| BG-08 | Determinism tightening | `T81Complex` | Binary pool serialization absent in `binary_io.cpp` — persistence gap | **Round-trip binary serialization for T81Complex passes determinism test; complete implementation with RFC-0024 compliance** | @t81dev | 2026-05-15 | **Closed** |
| BG-09 | Determinism tightening | `T81Graph` + collection lang runtime | `serialize_canonical()` exists in C++ headers but is never called from the language runtime | **Language runtime invokes `serialize_canonical` for collection types and T81Graph; stable serialization signature verified by test** | @t81dev | 2026-05-15 | **Closed** |
| AX-M5 | Promotion evidence | Axion §1.1 — Determinism Stewardship | Canonical-memory enforcement traceability incomplete across all Axion-visible transitions | **✅ Evidence map published covering all Axion-visible memory transitions; no uncovered transition in audit** | @t81dev | 2026-03-10 | **Closed** |
| AX-M6 | Promotion evidence | Axion §1.10 — CanonFS Observability | End-to-end persistence lifecycle audit closure gap | **✅ Evidence path maps full persistence lifecycle audit beyond hook/segment-event trace** | @t81dev | 2026-03-12 | **Closed** |
| AX-M7 | Promotion evidence | Axion §1.3 — Complexity Measurement | Call-graph and branch/path-divergence evidence not mapped | **✅ Evidence path maps call-graph complexity measurement; governance review accepted as Beta-gate evidence** | @t81dev | 2026-03-14 | **Closed** |
| T3K-S1 | Scope reduction | T3K Quantization | No `spec/t3k-quantization-spec.md` — surface has registry evidence but no normative spec | **✅ `spec/t3k-quantization-spec.md` authored and tracked as governed non-spec surface** | @t81dev | 2026-04-30 | **Closed** |
| FW-01 | Structural hardening | `core/vm/vm.cpp` | Experimental promotion include reached through controlled waiver | **✅ COMPLETED** — VM tier-promotion path now uses an in-module helper; dependency firewall waiver retired and waiver table is empty (2026-03-05). | @t81dev | 2026-03-05 | **Closed** |
| FW-02 | Structural hardening | VM policy-trace bridge | Opcode dispatch concentration in policy-trace bridge not fully extracted | `AxCheck`/`AxReport`/`AxRead`/`AxSet`/`AxVerify`/`AxHalt` helper paths fully extracted from dispatch loop; Axion opcodes routed through centralized `handle_axion_opcode`; no dispatch path >threshold | @t81dev | 2026-04-15 | **In Progress** |
| GOV-01 | Scope reduction | `docs/governance/` | No deputy-approval policy — single-owner concentration on all GO/HOLD decisions | **✅ `docs/governance/APPROVAL_DELEGATION.md` published with deputy-owner and delegation criteria** | @t81dev | 2026-04-30 | **Closed** |

---

## Closed Items

| ID | Category | What Was Hardened | Closed |
| :--- | :--- | :--- | :--- |
| **BG-06** | **Determinism tightening** | **Collection determinism implemented** - T81Map/T81Set use canonical sorted vector storage; T81_DETERMINISTIC enforcement; explicit iteration order guarantees; cross-platform determinism tests passing | **2026-03-04** |
| **BG-08** | **Determinism tightening** | **Complex number serialization completed** - Round-trip binary serialization for T81Complex passes determinism test; complete implementation with RFC-0024 compliance | **2026-03-04** |
| **BG-09** | **Determinism tightening** | **T81Graph serialization completed** - Language runtime invokes `serialize_canonical` for T81Graph; stable serialization signature verified by test | **2026-03-04** |
| **BG-10** | **Performance Enhancement** | **Memory Pool Optimization completed** - Complete 10-phase memory management transformation with 30-50% efficiency gains and production hardening | **2026-03-04** |
| **PR-426** | **Determinism Hardening Phase 1** | **Comprehensive determinism hardening completed** - T81Float/T81Complex transcendental math gated behind T81_DETERMINISTIC; T81Map/T81Set canonical sorted vector storage; T81Entropy/T81Time seeded state requirements; explicit container iteration order guarantees; cross-platform determinism tests implemented | **2026-03-04** |
| **GOV-01** | **Scope reduction** | **Deputy approval delegation policy published** - governance criteria and delegated GO/HOLD controls documented in `docs/governance/APPROVAL_DELEGATION.md` | **2026-03-05** |
| **FW-02 (Slice 1)** | **Structural hardening** | **VM policy bridge extraction started** - `AxCheck`/`AxReport` handling moved to dedicated helpers in `core/vm/vm.cpp`; full dispatch concentration reduction still open | **2026-03-05** |
| **FW-02 (Slice 2)** | **Structural hardening** | **VM policy bridge extraction advanced** - `AxRead`/`AxSet`/`AxVerify`/`AxHalt` handling moved to dedicated helpers in `core/vm/vm.cpp`; full dispatch concentration reduction still open | **2026-03-05** |
| **FW-02 (Slice 3)** | **Structural hardening** | **Axion opcode routing centralized** - `Ax*` opcode handling now routes through `handle_axion_opcode` in `core/vm/vm.cpp`; residual concentration work still open | **2026-03-05** |
| **FW-02 (Slice 4)** | **Structural hardening** | **Axion case consolidation** - duplicate `Ax*` switch-case blocks collapsed into grouped dispatch paths in `core/vm/vm.cpp`; residual concentration work still open | **2026-03-05** |
| **FW-02 (Slice 5)** | **Structural hardening** | **Axion dispatch locality improved** - `AxHalt` case moved into consolidated `Ax*` switch block; Axion dispatch no longer split across distant switch regions | **2026-03-05** |
| **FW-02 (Slice 6)** | **Structural hardening** | **Axion dispatch entrypoint unified** - `AxRead`/`AxSet`/`AxVerify` case labels moved into the same `Ax*` switch region as `AxCheck`/`AxReport`/`AxSign`/`AxLineage`/`AxCanon`/`AxHalt` | **2026-03-05** |
| **FW-02 (Slice 7)** | **Structural hardening** | **Axion memory-op sub-dispatch isolated** - `AxRead`/`AxSet`/`AxVerify` routing in `handle_axion_opcode` now flows through `handle_ax_memory_opcode`, reducing policy-bridge switch concentration | **2026-03-05** |
| **FW-01** | **Structural hardening** | **Dependency firewall waiver retired** - `core/vm/vm.cpp` no longer includes `t81/experimental/cog/promotion.hpp`; tier promotion logic is now local and `scripts/architecture/dependency_firewall_waivers.tsv` has no active waivers | **2026-03-05** |
| **AX-M5** | **Promotion evidence** | **Determinism stewardship evidence map completed** | **2026-03-04** |
| **AX-M6** | **Promotion evidence** | **CanonFS observability lifecycle evidence completed** | **2026-03-04** |
| **AX-M7** | **Promotion evidence** | **Complexity measurement evidence completed** | **2026-03-04** |
| **T3K-S1** | **Scope reduction** | **T3K quantization spec authored (`spec/t3k-quantization-spec.md`)** | **2026-03-04** |
| BG-01 | Determinism tightening | T81Lang §5 compile determinism — deterministic compile-profile traceability hardened | 2026-02-25 |
| BG-02 | Determinism tightening | T81Lang §§3/6 control-flow purity rules hardened | 2026-02-25 |
| BG-03 | Determinism tightening | T81Lang §4 name-resolution scoping gaps reduced | 2026-02-25 |
| BG-04 | Promotion evidence | T81Lang §7 language-to-Axion metadata consistency across paths | 2026-02-25 |
| BG-05 | Structural hardening | T81Lang promotion gate automation — `t81lang_promotion_gate_snapshot.py` | 2026-02-25 |
| BG-10 | Determinism tightening | `T81Quaternion`/`T81Prob`/`T81Qutrit`/`T81Uint` determinism tests added | 2026-02-28 |
| DT-01 | Determinism tightening | `Cell` signed-overflow UB fixed via `Cell::safe_add` | 2026-02-27 |
| DT-02 | Determinism tightening | `T81Float` signed-zero canonicalization enforced | 2026-02-27 |
| DT-03 | Determinism tightening | `T81Map`/`T81Set` frontend type-enforcement hardened | 2026-02-27 |
| CI-01 | Structural hardening | CodeQL `push` trigger added to `codeql.yml` — required context now populated on main push | 2026-02-26 |
| CANO-01 | Determinism tightening | `serialize_canonical` added to 10 type headers: `T81List`, `T81Set`, `T81Tree`, `T81Complex`, `T81Symbolic`, `T81Polynomial`, `T81Time`, `T81Entropy`, `T81Promise`, `T81Agent` | 2026-02-28 |
| **TEST-01** | **Structural hardening** | **T81Lang parser operator precedence** — Fixed to match T81 specification (§A.1.1) | **2026-03-03** |
| **TEST-02** | **Structural hardening** | **T81Lang AST group nodes** — Eliminated unnecessary grouping in parser output | **2026-03-03** |
| **TEST-03** | **Determinism tightening** | **T81Lang semantic analyzer narrowing** — Now prevents narrowing conversions via numeric rank | **2026-03-03** |
| **TEST-04** | **Structural hardening** | **T81Lang conformance tests** — Fixed subprocess abortions and tier calling behavior | **2026-03-03** |
| **TEST-05** | **Determinism tightening** | **T81Lang AST/IR determinism** — Updated hash to match new parser/semantic behavior | **2026-03-03** |
| **TEST-06** | **Structural hardening** | **T81Lang CLI tests** — Fixed check and pipeline tests to detect narrowing conversions | **2026-03-03** |
| **TEST-07** | **Structural hardening** | **T81Lang IR snapshot audit** — Updated expectations to match current precedence | **2026-03-03** |
| **CI-02** | **Structural hardening** | **CI lychee link checking** — Fixed broken internal/external links causing CI failures | **2026-03-04** |
| **CI-03** | **Structural hardening** | **CI documentation references** — Updated all broken cross-references in status docs | **2026-03-04** |
| **TEST-08** | **Structural hardening** | **Parser test expectations** — Updated all parser tests for new AST structure (100% test success) | **2026-03-04** |

---

## Priority Rule

When ordering work from this backlog:

1. **Verified-surface regressions** — fix immediately; do not defer.
2. **FW-02** — reduce VM dispatch concentration in policy-trace bridge.
3. **BG-07** — resolve BigInt precision scope mismatch.
4. **FW-01 closure guardrail** — keep dependency firewall waiver table empty unless a new exception is explicitly approved.
5. **Post-close governance hygiene** — keep delegation and evidence artifacts current.

---

## Cross-References

- `docs/status/DRIFT_DECOMPOSITION.md`
- `docs/status/FROZEN_CORE_PROFILE.md`
- `docs/status/DETERMINISM_AUDIT_LOG.md`
- `docs/status/AXION_STATUS.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
