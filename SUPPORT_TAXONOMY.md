# T81 Foundation Support and Surface Classification

This is a descriptive navigation guide. It does not assign or promote deterministic
surfaces. The earlier directory-wide three-tier taxonomy conflated file location,
maintenance, and execution guarantees; use the existing authoritative four-class
surface model instead.

## Authoritative classification

[RFC-0048](spec/rfcs/RFC-0048-deterministic-surface-definition-and-governance-boundaries.md)
defines the classes; the [governance surface register](docs/governance/GOVERNANCE_SURFACE_REGISTER.md)
and [determinism registry](docs/governance/DETERMINISM_SURFACE_REGISTRY.md)
record their application:

1. **Deterministic Core Profile (DCP):** the specifically defined deterministic surfaces with their normative contracts and conformance evidence.
1. **Governed non-DCP:** policy-bounded surfaces with scoped evidence, without a blanket core determinism claim.
1. **Experimental:** research implementations or interfaces whose contracts and evidence remain provisional.
1. **Out of scope:** behavior excluded from the current deterministic/governance claim boundary.

A frozen ISA is not a production-readiness certificate. A passing test, accepted
RFC, public header, or location outside `experimental/` does not independently
establish DCP status. Read [SECURITY.md](SECURITY.md) for security evidence limits.

## Finding the implementation

Use [PROJECT_STRUCTURE.md](PROJECT_STRUCTURE.md) and the CMake targets to trace
current locations. For example, the interpreter source is `vm/vm.cpp`, CanonFS
implementations are in `fs/`, Axion is in `kernel/axion/`, and frontend code is
in `lang/frontend/`; `src/` largely assembles build targets. Some registry path
labels predate this layout. Preserve their classifications and resolve path drift
against source, without silently changing governance status.

T81's research mission, canonical H01–H08 lineage, and evidence standards are
introduced in the [README](README.md) and [continuity charter](docs/research/continuity/MASTER_CONTINUITY_CHARTER_v1.0.md).
Historical market demonstrations, reports, generated logs, and experimental
hardware proposals are not support or performance guarantees. See the
[root alignment audit](docs/research/ROOT_DOCUMENTATION_ALIGNMENT_AUDIT.md)
for the inspected snapshot and remaining limitations.
