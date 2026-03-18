# System Status

Status: Active
Last Updated: 2026-03-16
Owner: Status / Governance

## Purpose

Provide a concise operational view of subsystem maturity and governance-critical
status boundaries.

## Component Health

| Component | Maturity | Compliance Posture | Evidence Surface |
| :--- | :--- | :--- | :--- |
| **TISC ISA** | Stable | Frozen boundary enforced | Tests + freeze governance docs |
| **T81VM** | **Stable** | Stable promotion complete 2026-03-15; TV-01..TV-06 all met; AgentInvoke dispatch with Axion audit; 363/363 tests | Tests + implementation matrix + `docs/records/audits/T81VM_STABLE_PROMOTION_EVIDENCE_2026-03-15.md` |
| **Axion Governance Kernel** | **Stable** | Stable promotion complete 2026-03-15; P4 Safety & P5 Privileged Instruction satisfied; AX-M6 canonical reason strings; 54/54 tests (49/49 axion + 5/5 AX-M6) | Tests + implementation matrix + `docs/records/audits/AXION_STABLE_PROMOTION_EVIDENCE_2026-03-15.md` |
| **T81Lang** | **Stable** | Stable promotion complete 2026-03-16; spec v1.3 Stable; §3.2 VM I/O channels defined; first-class agent/behavior (RFC-0015); all spec sections complete; 363/363 tests | `docs/records/audits/T81LANG_STABLE_PROMOTION_EVIDENCE_2026-03-16.md` + implementation matrix |
| **Data Types** | Implemented (Stable) | Frozen boundary enforced; determinism audit completed 2026-02-27 — `Cell` overflow UB fixed, `T81Float` signed-zero canonicalized, `T81Map`/`T81Set` type enforcement hardened. | `docs/reports/determinism_types_audit.md` + `tests/determinism/` + `tests/cpp/` |
| **T81Graph** | **Beta** | Beta promotion complete 2026-03-15; VM opcode lowering + lang-side serialization wired; DCP verification complete; 6/6 tests passing. | Implementation matrix + `docs/records/audits/BG-09_IMPLEMENTATION_EVIDENCE_T81GRAPH_SERIALIZATION.md` |
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

## Operational Notes (2026-03-16)

- T81Lang promoted to Stable (spec v1.3); §3.2 VM I/O channels defined; TG-01 waiver
  granted for non-normative translations.
- RFC-0015 agentic constructs accepted: first-class `agent`/`behavior` declarations,
  `AGENT_INVOKE` opcode, `infer` sugar; 16/16 assertions pass; tisc-spec §5.16 added.
- RFC-0011 (Grammar Modernization) and RFC-00A2 (AI Benchmark Spec) accepted; all RFC
  drafts closed — RFC program 100% complete (49/49 active RFCs accepted).
- Full suite: 363/363 tests passing (100%).

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
