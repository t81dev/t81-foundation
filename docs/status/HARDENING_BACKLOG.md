# Hardening Backlog

Last Updated: 2026-03-04
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
| BG-06 | Determinism tightening | `T81List`, `T81Map`, `T81Set`, `T81Tree` | No run-to-run determinism tests for collection types; populated-constructor determinism unverified | `cli_stdlib_fixtures_test` covers List/Map/Set/Tree determinism; `check_stdlib_surface_baseline.py` passes | @t81dev | 2026-05-15 | Open |
| BG-07 | Scope reduction / determinism tightening | `T81BigInt` | VM aliases BigInt to 64-bit; >64-bit literals silently truncated — spec claims arbitrary-precision | Native arbitrary-precision opcode path OR explicit governance decision to narrow spec claim | @t81dev | 2026-05-15 | Open |
| BG-08 | Determinism tightening | `T81Complex` | Binary pool serialization absent in `binary_io.cpp` — persistence gap | Round-trip binary serialization for T81Complex passes determinism test | @t81dev | 2026-05-15 | Open |
| BG-09 | Determinism tightening | `T81Graph` + collection lang runtime | `serialize_canonical()` exists in C++ headers but is never called from the language runtime | Language runtime invokes `serialize_canonical` for collection types and T81Graph; stable serialization signature verified by test | @t81dev | 2026-05-15 | Open |
| AX-M5 | Promotion evidence | Axion §1.1 — Determinism Stewardship | Canonical-memory enforcement traceability incomplete across all Axion-visible transitions | Evidence map published covering all Axion-visible memory transitions; no uncovered transition in audit | @t81dev | 2026-03-10 | Open |
| AX-M6 | Promotion evidence | Axion §1.10 — CanonFS Observability | End-to-end persistence lifecycle audit closure gap | Evidence path maps full persistence lifecycle audit beyond hook/segment-event trace | @t81dev | 2026-03-12 | Open |
| AX-M7 | Promotion evidence | Axion §1.3 — Complexity Measurement | Call-graph and branch/path-divergence evidence not mapped | Evidence path maps call-graph complexity measurement; governance review accepts as Beta-gate evidence | @t81dev | 2026-03-14 | Open |
| T3K-S1 | Scope reduction | T3K Quantization | No `spec/t3k-quantization-spec.md` — surface has registry evidence but no normative spec | Author `spec/t3k-quantization-spec.md` OR explicitly mark T3K as governed non-spec surface in registry | @t81dev | 2026-04-30 | Open |
| FW-01 | Structural hardening | `core/vm/vm.cpp:24` | Controlled dependency waiver — policy hook cross-boundary include | Either eliminate the waiver through header restructuring, or explicitly re-affirm and re-document it in each release | @t81dev | Ongoing | Monitoring |
| FW-02 | Structural hardening | VM policy-trace bridge | Opcode dispatch concentration in policy-trace bridge not fully extracted | `AxCheck`/`AxReport` helper paths fully extracted from dispatch loop; no dispatch path >threshold | @t81dev | 2026-04-15 | Open |
| GOV-01 | Scope reduction | `docs/governance/` | No deputy-approval policy — single-owner concentration on all GO/HOLD decisions | `docs/governance/APPROVAL_DELEGATION.md` published with deputy-owner and delegation criteria | @t81dev | 2026-04-30 | Open |

---

## Closed Items

| ID | Category | What Was Hardened | Closed |
| :--- | :--- | :--- | :--- |
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
2. **Open AX-M* items** — time-sensitive; gated to Axion Beta review.
3. **BG-06/BG-09** — collection determinism; required for T81Lang production posture.
4. **BG-07/BG-08** — structural hardening; required before BigInt/Complex DCP candidacy.
5. **T3K-S1/GOV-01** — governance discipline; no user-visible impact.

---

## Cross-References

- `docs/status/DRIFT_DECOMPOSITION.md`
- `docs/status/FROZEN_CORE_PROFILE.md`
- `docs/status/DETERMINISM_AUDIT_LOG.md`
- `docs/status/AXION_STATUS.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
