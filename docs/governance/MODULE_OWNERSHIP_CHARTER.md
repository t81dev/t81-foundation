# Module Ownership Charter

Status: Active
Version: 1.0.0
Owner: Governance
Last Updated: 2026-02-25

## Purpose

Define module ownership, approval thresholds, and freeze-escalation requirements
for architecture, determinism, and public API changes.

## Scope

This charter applies to:

- `core/types/`
- `core/isa/`
- `core/vm/`
- `kernel/axion/`
- `runtime/tracing/`
- `runtime/jit/`
- `experimental/*`
- `tools/*`
- `include/t81/**` (public API surface)

## Definitions

- Primary owner: Maintainer accountable for change review and boundary integrity.
- Frozen module: Module constrained by freeze policy and deterministic-core rules.
- Freeze exception: Explicitly labeled change request that proposes boundary-impacting
  modification.

## Module Taxonomy (Authoritative)

| Module | Path | Freeze Class |
| --- | --- | --- |
| Core Data Types | `core/types/` | Frozen |
| TISC ISA | `core/isa/` | Frozen |
| T81VM | `core/vm/` | Frozen (for DCP-covered semantics) |
| Axion Governance Kernel | `kernel/axion/` | Controlled / Partial |
| Runtime Tracing | `runtime/tracing/` | Controlled |
| Runtime JIT | `runtime/jit/` | Experimental / Non-DCP unless promoted |
| Experimental Surfaces | `experimental/*` | Experimental |
| Tooling Utilities | `tools/*` | Controlled |
| Public API Surface | `include/t81/**` | Frozen for compatibility discipline |

## Ownership Model

1. Every module must have at least one primary owner in `CODEOWNERS`.
2. Frozen modules require two approvals for:
   - spec change
   - determinism change
   - public API modification
3. Experimental modules require one owner approval.
4. Ownership assignments do not supersede authority ordering:
   `/spec` > `docs/architecture/OVERVIEW.md` > `/docs` > `/book`.

## Freeze Escalation Rules

The following change classes require escalation:

- TISC ISA changes (`core/isa/`, relevant `/spec` surfaces)
- Core Data Type changes (`core/types/`, canonical encoding/semantics)
- Deterministic Core Profile surface changes

Mandatory controls:

1. ADR entry under `docs/architecture/adr/`
2. Determinism registry update in
   `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
3. Explicit PR label: `freeze-exception`
4. Two reviewer approvals

## Cross-References

- `docs/governance/SPEC_AUTHORITY_MODEL.md`
- `docs/governance/FREEZE_ENFORCEMENT.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/governance/DETERMINISM_THREAT_MODEL.md`
- `docs/product/DETERMINISTIC_CORE_PROFILE.md`

## Versioning Statement

This charter is versioned as governance policy. Any normative tightening follows
MINOR/PATCH governance updates; boundary-loosening requires documented
justification and explicit review in an ADR.
