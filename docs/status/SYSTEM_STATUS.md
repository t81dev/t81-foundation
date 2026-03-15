# System Status

Status: Active
Last Updated: 2026-03-15
Owner: Status / Governance

## Purpose

Provide a concise operational view of subsystem maturity and governance-critical
status boundaries.

## Component Health

| Component | Maturity | Compliance Posture | Evidence Surface |
| :--- | :--- | :--- | :--- |
| **TISC ISA** | Stable | Frozen boundary enforced | Tests + freeze governance docs |
| **T81VM** | Beta | Controlled under spec and determinism policy | Tests + status audits |
| **Axion Kernel** | Beta | Partial implementation against draft surfaces | Tests + implementation matrix + `docs/status/AXION_PARTIAL_COVERAGE_ALIGNMENT_2026-03.md` |
| **T81Lang** | Beta | Draft spec / Beta implementation posture with active drift controls. Frontend refactor complete (2026-03-08): typed AST, unified builtin registry, IRGen to .cpp. BG-06..10 all closed. 329/332 core tests; 3 pre-existing TLOADHASH SEGFAULTs monitored (R-18). | Determinism and conformance checks + `docs/status/T81LANG_PROMOTION_GATE.md` |
| **Data Types** | Implemented (Stable) | Frozen boundary enforced; determinism audit completed 2026-02-27 — `Cell` overflow UB fixed, `T81Float` signed-zero canonicalized, `T81Map`/`T81Set` type enforcement hardened. | `docs/reports/determinism_types_audit.md` + `tests/determinism/` + `tests/cpp/` |
| **T81Graph** | Draft | VM native opcode lowering complete; lang-side canonical serialization wired (BG-09 closed 2026-03-04). Non-DCP unless promoted. | `docs/status/T81LANG_SURFACE_INVENTORY.md` |
| **Cognitive Tiers** | Concept / Experimental | Experimental / non-DCP / non-verified unless promoted through governance | `docs/status/EXPERIMENTAL_SURFACE_INVENTORY.md`, `spec/cognitive-tiers.md` |
| **Hanoi VM** | Concept | Experimental / non-DCP / non-verified unless promoted through governance | Experimental inventory and specs |

## Program Risks (Current)

- Spec-implementation drift on draft/experimental surfaces.
- Determinism overclaim risk outside Verified registry surfaces.
- Governance maintenance risk if release/status artifacts are not refreshed on cadence.

## Control References

- `docs/status/PROJECT_CONTROL_CENTER.md`
- `docs/status/IMPLEMENTATION_MATRIX.md`
- `docs/status/AXION_PARTIAL_COVERAGE_ALIGNMENT_2026-03.md`
- `docs/status/T81LANG_PROMOTION_GATE.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/product/DETERMINISTIC_CORE_PROFILE.md`

## Versioning Statement

This status document is descriptive and must remain aligned with the authority
model and governance artifacts.
