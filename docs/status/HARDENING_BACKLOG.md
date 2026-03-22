# Hardening Backlog

Last Updated: 2026-03-14
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
| BG-07 | Scope reduction / determinism tightening | `T81BigInt` | VM aliases BigInt to 64-bit; >64-bit literals silently truncated — spec claims arbitrary-precision | **✅ COMPLETED** — No silent truncation remains on IR→binary→VM literal path. Oversized literals lower to `LiteralKind::BigIntHandle` with deterministic transport (`Program::bigint_pool` + emitter/IO round-trip coverage). VM `LOADI` materializes BigInt as canonical `FractionHandle` (`BigInt/1`), JIT explicitly deopts `BigIntHandle` to preserve interpreter precision semantics, and narrowing back to `Int` (`Frac2I`) now fails closed on non-`int64` values. Governance decision recorded in `docs/status/DECISION_LOG.md` (2026-03-05). | @t81dev | 2026-05-15 | **Closed** |
| BG-08 | Determinism tightening | `T81Complex` | Binary pool serialization absent in `binary_io.cpp` — persistence gap | **Round-trip binary serialization for T81Complex passes determinism test; complete implementation with RFC-0024 compliance** | @t81dev | 2026-05-15 | **Closed** |
| BG-09 | Determinism tightening | `T81Graph` + collection lang runtime | `serialize_canonical()` exists in C++ headers but is never called from the language runtime | **Language runtime invokes `serialize_canonical` for collection types and T81Graph; stable serialization signature verified by test** | @t81dev | 2026-05-15 | **Closed** |
| AX-M5 | Promotion evidence | Axion §1.1 — Determinism Stewardship | Canonical-memory enforcement traceability incomplete across all Axion-visible transitions | **✅ Evidence map published covering all Axion-visible memory transitions; no uncovered transition in audit** | @t81dev | 2026-03-10 | **Closed** |
| AX-M6 | Promotion evidence | Axion §1.10 — CanonFS Observability | End-to-end persistence lifecycle audit closure gap | **✅ Evidence path maps full persistence lifecycle audit beyond hook/segment-event trace** | @t81dev | 2026-03-12 | **Closed** |
| AX-M7 | Promotion evidence | Axion §1.3 — Complexity Measurement | Call-graph and branch/path-divergence evidence not mapped | **✅ Evidence path maps call-graph complexity measurement; governance review accepted as Beta-gate evidence** | @t81dev | 2026-03-14 | **Closed** |
| T3K-S1 | Scope reduction | T3K Quantization | No `spec/t3k-quantization-spec.md` — surface has registry evidence but no normative spec | **✅ `spec/t3k-quantization-spec.md` authored and tracked as governed non-spec surface** | @t81dev | 2026-04-30 | **Closed** |
| FW-01 | Structural hardening | `vm/vm.cpp` | Experimental promotion include reached through controlled waiver | **✅ COMPLETED** — VM tier-promotion path now uses an in-module helper; dependency firewall waiver retired and waiver table is empty (2026-03-05). | @t81dev | 2026-03-05 | **Closed** |
| FW-02 | Structural hardening | VM policy-trace bridge | Opcode dispatch concentration in policy-trace bridge not fully extracted | **✅ COMPLETED** — Axion opcodes now pre-dispatch outside the main VM opcode switch via `dispatch_axion_opcode_from_step`; `AxCheck`/`AxReport`/`AxRead`/`AxSet`/`AxVerify`/`AxHalt` helper paths are fully extracted and routed through centralized `handle_axion_opcode`; dispatch concentration reduced below threshold (2026-03-05). | @t81dev | 2026-04-15 | **Closed** |
| DOC-01 | Structural hardening | `docs/` directory | Documentation organized by legacy directory names rather than content | **✅ COMPLETED** — Content-based reorganization implemented with user-guide/, developer-guide/, and process/ separation; 47 files reorganized; README.md links updated; status files audited and fixed | @t81dev | 2026-03-06 | **Closed** |
| SEC-01 | Structural hardening | `vm/vm.cpp` + fuzz infra | Fuzz harnesses absent; 3 VM opcodes (SymLoad, ReflCap, ReflJustify) accessed `register_tags[insn.b]` without `reg_ok()` guard — triggered SEGFAULT under fuzz | **✅ COMPLETED** — `fuzz_parser` and `fuzz_vm` harnesses added; `reg_ok(insn.b)` guard applied to all 3 dispatch cases; TLoadHash conformance tests (3/3) now pass; fuzz workaround removed; `85a0b438` | @t81dev | 2026-03-10 | **Closed** |
| SEC-02 | Structural hardening | `isa/binary_io.cpp` | Length-prefix deserialization had no sanity cap — corrupt/empty `.tisc` caused `vec.resize()` with attacker-controlled count up to 2⁶⁴ elements → OOM-kill (exit 137) | **✅ COMPLETED** — `read_checked_size()` helper added; throws on stream EOF or count > 16M (kMaxDeserialiseCount); applied at all 9 deserialization sites; `85a0b438` | @t81dev | 2026-03-10 | **Closed** |
| QA-01 | Structural hardening | `tests/cpp/cli_stress_test.cpp` + CMakeLists | CLI stress test absent from CTest; interactive commands (`vm debug`, `lang debug`) hung on stdin; corrupt-file OOM masked as exit 137 | **✅ COMPLETED** — `cli_stress_test` wired into CMake (300s timeout); `/dev/null` stdin redirect for interactive commands; all 6 test failures fixed; suite now covers 338 tests; `85a0b438` | @t81dev | 2026-03-10 | **Closed** |

---

## Closed Items

| ID | Category | What Was Hardened | Closed |
| :--- | :--- | :--- | :--- |
| **BG-06** | **Determinism tightening** | **Collection determinism implemented** - T81Map/T81Set use canonical sorted vector storage; T81_DETERMINISTIC enforcement; explicit iteration order guarantees; cross-platform determinism tests passing | **2026-03-04** |
| **BG-08** | **Determinism tightening** | **Complex number serialization completed** - Round-trip binary serialization for T81Complex passes determinism test; complete implementation with RFC-0024 compliance | **2026-03-04** |
| **BG-09** | **Determinism tightening** | **T81Graph serialization completed** - Language runtime invokes `serialize_canonical` for T81Graph; stable serialization signature verified by test | **2026-03-04** |
| **BG-07** | **Scope reduction / determinism tightening** | **BigInt literal precision closure completed** - oversized integer literals transport through `BigIntHandle` without truncation; VM materialization uses canonical `BigInt/1` fraction representation; JIT deopts BigInt literals to interpreter path; `Frac2I` overflow fails closed deterministically | **2026-03-05** |
| **BG-10** | **Performance Enhancement** | **Memory Pool Optimization completed** - Complete 10-phase memory management transformation with 30-50% efficiency gains and production hardening | **2026-03-04** |
| **PR-426** | **Determinism Hardening Phase 1** | **Comprehensive determinism hardening completed** - T81Float/T81Complex transcendental math gated behind T81_DETERMINISTIC; T81Map/T81Set canonical sorted vector storage; T81Entropy/T81Time seeded state requirements; explicit container iteration order guarantees; cross-platform determinism tests implemented | **2026-03-04** |
| **DOC-01** | **Structural hardening** | **Documentation reorganization completed** - Content-based structure implemented with user-guide/, developer-guide/, and process/ separation; 47 files reorganized; README.md links updated; status files audited and fixed | **2026-03-06** |
| **SEC-01** | **Structural hardening** | **Fuzz infra + 3 OOB VM register-index bugs fixed** — `fuzz_parser`/`fuzz_vm` harnesses added; `reg_ok(insn.b)` guard applied to SymLoad, ReflCap, ReflJustify; TLoadHash conformance clean; fuzz workaround removed | **2026-03-10** |
| **SEC-02** | **Structural hardening** | **binary_io OOM-on-corrupt-input hardened** — `read_checked_size()` throws on EOF or count > 16M at all 9 deserialization sites; exit-137 crash eliminated | **2026-03-10** |
| **QA-01** | **Structural hardening** | **CLI stress test wired into CTest (338 tests)** — full command surface covered; `/dev/null` stdin fix; 6 test failures resolved; 338/338 passing | **2026-03-10** |
| **GOV-01** | **Scope reduction** | **Deputy approval delegation policy published** - governance criteria and delegated GO/HOLD controls documented in `docs/governance/APPROVAL_DELEGATION.md` | **2026-03-05** |
| **FW-02 (Slice 1)** | **Structural hardening** | **VM policy bridge extraction started** - `AxCheck`/`AxReport` handling moved to dedicated helpers in `vm/vm.cpp`; full dispatch concentration reduction still open | **2026-03-05** |
| **FW-02 (Slice 2)** | **Structural hardening** | **VM policy bridge extraction advanced** - `AxRead`/`AxSet`/`AxVerify`/`AxHalt` handling moved to dedicated helpers in `vm/vm.cpp`; full dispatch concentration reduction still open | **2026-03-05** |
| **FW-02 (Slice 3)** | **Structural hardening** | **Axion opcode routing centralized** - `Ax*` opcode handling now routes through `handle_axion_opcode` in `vm/vm.cpp`; residual concentration work still open | **2026-03-05** |
| **FW-02 (Slice 4)** | **Structural hardening** | **Axion case consolidation** - duplicate `Ax*` switch-case blocks collapsed into grouped dispatch paths in `vm/vm.cpp`; residual concentration work still open | **2026-03-05** |
| **FW-02 (Slice 5)** | **Structural hardening** | **Axion dispatch locality improved** - `AxHalt` case moved into consolidated `Ax*` switch block; Axion dispatch no longer split across distant switch regions | **2026-03-05** |
| **FW-02 (Slice 6)** | **Structural hardening** | **Axion dispatch entrypoint unified** - `AxRead`/`AxSet`/`AxVerify` case labels moved into the same `Ax*` switch region as `AxCheck`/`AxReport`/`AxSign`/`AxLineage`/`AxCanon`/`AxHalt` | **2026-03-05** |
| **FW-02 (Slice 7)** | **Structural hardening** | **Axion memory-op sub-dispatch isolated** - `AxRead`/`AxSet`/`AxVerify` routing in `handle_axion_opcode` now flows through `handle_ax_memory_opcode`, reducing policy-bridge switch concentration | **2026-03-05** |
| **FW-02 (Slice 8 / Closure)** | **Structural hardening** | **Axion pre-dispatch isolated from VM main switch** - `step()` now routes `Ax*` opcodes through `dispatch_axion_opcode_from_step` before entering the large opcode switch, removing Axion case concentration from the primary dispatch block | **2026-03-05** |
| **FW-01** | **Structural hardening** | **Dependency firewall waiver retired** - `vm/vm.cpp` no longer includes `t81/experimental/cog/promotion.hpp`; tier promotion logic is now local and `scripts/architecture/dependency_firewall_waivers.tsv` has no active waivers | **2026-03-05** |
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
2. **FW-01 closure guardrail** — keep dependency firewall waiver table empty unless a new exception is explicitly approved.
3. **Post-close governance hygiene** — keep delegation and evidence artifacts current.

---

## Cross-References

- `docs/status/DRIFT_DECOMPOSITION.md`
- `docs/status/FROZEN_CORE_PROFILE.md`
- `docs/status/DETERMINISM_AUDIT_LOG.md`
- `docs/status/AXION_STATUS.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
