# Architecture documentation index

Status: Active
Last Updated: 2026-11-26

## Purpose

High-level architecture overview, dependency firewall policy, layer specifications,
control-plane boundaries, and ADR system.

## Scope

This README is a navigation aid for this directory. It does not change
specification authority, freeze boundaries, or determinism claim scope.

## Navigation

- [OVERVIEW.md](./OVERVIEW.md) — Canonical implemented architecture snapshot & four-plane alignment
- [AI_OS_STRATIFIED_BLUEPRINT.md](./AI_OS_STRATIFIED_BLUEPRINT.md) — Stratified Neural-Symbolic AI OS Blueprint (*Planned / Design Target — Horizon D*)
- [T81_ARCHITECTURE_REPORT.md](./T81_ARCHITECTURE_REPORT.md) — Full architectural narrative & post-Wave 5 runtime posture
- [ARCHITECTURE_DOC_STANDARD.md](./ARCHITECTURE_DOC_STANDARD.md)
- [DEPENDENCY_FIREWALL.md](./DEPENDENCY_FIREWALL.md)
- [system_map.yaml](./system_map.yaml)
- [layers/README.md](./layers/README.md)
- [layers/data-types.md](./layers/data-types.md)
- [layers/tisc-isa.md](./layers/tisc-isa.md)
- [layers/vm.md](./layers/vm.md)
- [layers/axion.md](./layers/axion.md)
- [layers/canonfs.md](./layers/canonfs.md)
- [layers/experimental-tiers.md](./layers/experimental-tiers.md)
- [cross-cutting/README.md](./cross-cutting/README.md)
- [cross-cutting/determinism.md](./cross-cutting/determinism.md)
- [cross-cutting/governance-axion.md](./cross-cutting/governance-axion.md)
- [cross-cutting/execution-model.md](./cross-cutting/execution-model.md)
- [cross-cutting/capability-boundaries.md](./cross-cutting/capability-boundaries.md)
- [diagrams/README.md](./diagrams/README.md)
- [diagnostics/ARCH_ENG_002_DIAGNOSTIC_REPORT.md](./diagnostics/ARCH_ENG_002_DIAGNOSTIC_REPORT.md)
- [visualization/ARCHITECTURE_VISUALIZATION_REPORT.md](./visualization/ARCHITECTURE_VISUALIZATION_REPORT.md)
- [adr/](./adr/)

## Active Proposals & Coexistence Architecture (Experimental / Draft)

- [TERNARYOS_BOOT_TO_RECEIPT_CONTRACT_v0.1.md](../research/proposals/TERNARYOS_BOOT_TO_RECEIPT_CONTRACT_v0.1.md)
- [TERNARYOS_ARCHITECTURE_v0.2.md](../research/proposals/TERNARYOS_ARCHITECTURE_v0.2.md)
- [TERNARYOS_MULTI_AGENT_CAPABILITY_BUDGET_v0.2.md](../research/proposals/TERNARYOS_MULTI_AGENT_CAPABILITY_BUDGET_v0.2.md)
- [T81_RESEARCH_STATUS_2026-11-23_BY_AI_FOR_AI_COEXISTENCE.md](../ai-context/history/T81_RESEARCH_STATUS_2026-11-23_BY_AI_FOR_AI_COEXISTENCE.md)

## Historical Records

Archived under `docs/records/archive/architecture/`:

- DOCS_IA_MIGRATION_MAP.md
- REPO_RESTRUCTURE_MASTERPLAN.md
- RESTRUCTURE_PHASE1_PREFLIGHT.md
- RESTRUCTURE_COMPLETION_CHECKLIST.md

## Versioning Statement

Operational index only. Authority hierarchy remains:
`/spec` > `docs/architecture/OVERVIEW.md` > `/docs` > `/book`.
