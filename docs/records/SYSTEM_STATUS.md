# System Status

Status: Active
Last Updated: 2026-03-06
Owner: Status / Governance

## Purpose

Provide a concise operational view of subsystem maturity and governance-critical
status boundaries.

## Component Health

| Component | Maturity | Compliance Posture | Evidence Surface |
| :--- | :--- | :--- | :--- |
| **TISC ISA** | Stable | Frozen boundary enforced | Tests + freeze governance docs |
| **T81VM** | Beta | Controlled under spec and determinism policy | Tests + status audits |
| **Axion Kernel** | Alpha | Partial implementation against draft surfaces | Tests + implementation matrix + `docs/status/AXION_PARTIAL_COVERAGE_ALIGNMENT_2026-03.md` |
| **T81Lang** | Beta | Draft spec / Beta implementation posture with active drift controls and promotion-gate maintenance. 2026-02-28: `List`/`Map`/`Set`/`Tree` first-class; `T81Quaternion`/`T81Prob`/`Cell` exposed; `serialize_canonical` added to 10 types; stress test suite launched; BG-06..10 opened from surface inventory gaps. | Determinism and conformance checks + `docs/status/T81LANG_PROMOTION_GATE.md` + `docs/status/T81LANG_ENGINEERING_BACKLOG_2026-03.md` + `docs/status/T81LANG_SURFACE_INVENTORY.md` |
| **Data Types** | Implemented (Stable) | Frozen boundary enforced; determinism audit completed 2026-02-27 — `Cell` overflow UB fixed, `T81Float` signed-zero canonicalized, `T81Map`/`T81Set` type enforcement hardened. | `docs/reports/determinism_types_audit.md` + `tests/determinism/` + `tests/cpp/` |
| **T81Graph** | Draft | VM native opcode lowering complete (2026-02-28); lang-side canonical serialization gap open (BG-09); no determinism tests yet. Non-DCP unless promoted. | `docs/status/T81LANG_SURFACE_INVENTORY.md`, `docs/status/T81LANG_ENGINEERING_BACKLOG_2026-03.md` |
| **Cognitive Tiers** | Concept / Experimental | Experimental / non-DCP / non-verified unless promoted through governance | `docs/status/EXPERIMENTAL_SURFACE_INVENTORY.md`, `spec/cognitive-tiers.md` |
| **Hanoi VM** | Concept | Experimental / non-DCP / non-verified unless promoted through governance | Experimental inventory and specs |

## Program Risks (Current)

- Spec-implementation drift on draft/experimental surfaces.
- Determinism overclaim risk outside Verified registry surfaces.
- Governance maintenance risk if release/status artifacts are not refreshed on cadence.

## Operational Notes (2026-03-05)

- CI recovery completed on `main` after restoring `include/t81/support/expected.hpp`
  fallback implementation (`57f1a96c`).
- Format gate reliability hardened by increasing `actions/checkout` history depth
  in `format.yml` (`b20934be`), preventing full-tree fallback checks on push.
- Required CI contexts are passing on the reference candidate (`57f1a96c`).

## Operational Notes (2026-03-06)

- Main CI currently red on Windows configure lanes due sparse-checkout omissions;
  patch queued to include `benchmarks/` and `spec/` in Windows sparse checkout.
- Determinism hardening queued with a same-machine repeatability gate that
  enforces bit-identical `vm_workload_determinism_signatures.log` across reruns.
- Cross-arch bit-identity gates for `t3k` and `t81lang` promoted into required
  quality-gate dependencies.

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
