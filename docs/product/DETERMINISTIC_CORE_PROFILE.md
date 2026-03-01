# Deterministic Core Profile (DCP)

Status: Active
Profile Line: v1.x
Owner: Product/Governance

This document defines the product boundary for the T81 Deterministic Core Profile (DCP).

- `T81` is the full platform (stable + experimental + research surfaces).
- `DCP` is the minimal stable, reproducible, versioned subset eligible for certification and release guarantees.

Normative references:

- `docs/spec/DETERMINISTIC_CORE_PROFILE_v1.md`
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`
- `docs/governance/FREEZE_ENFORCEMENT.md`
- `docs/governance/DETERMINISM_THREAT_MODEL.md`
- `docs/product/RELEASE_DISCIPLINE.md`
- `docs/status/STRUCTURAL_INTEGRITY_REPORT.md`

## A. Included in DCP (v1.x)

The following surfaces are included in DCP when they remain Verified in the Determinism Surface Registry.

| Surface | Scope | Spec Reference | Verification Tests/Scripts | CI/Automation |
| :--- | :--- | :--- | :--- | :--- |
| Core Data Types | `core/types/` and canonical numeric aliases in public API | `spec/t81-data-types.md` | `tests/cpp/v1_canonical_numeric_contract_test.cpp`, `tests/cpp/tisc_binary_io_determinism_test.cpp` | `ci.yml` |
| TISC ISA (Frozen) | opcode encoding/semantics (`core/isa/`) | `spec/tisc-spec.md` | `tests/cpp/vm_determinism_property_test.cpp`, `tests/cpp/test_tritwise_backend_equivalence.cpp`, `scripts/ci/check_tisc_freeze_integrity.py` | `ci.yml` |
| T81VM Interpreter (non-JIT) | reference interpreter path (`core/vm/`) | `spec/t81vm-spec.md` | `tests/cpp/vm_trace_test.cpp`, `tests/cpp/vm_determinism_property_test.cpp` | `ci.yml` |
| Soft-float / dmath | deterministic float and numeric behavior | `spec/t81-data-types.md` | `tests/cpp/test_T81Float_arithmetic.cpp`, `tests/cpp/test_T81Float_rounding.cpp` | `ci.yml` |
| Canonical serialization rules | canonical encodings and binary stability | `spec/t81-data-types.md`, `spec/supplemental/cpp-mapping.md` | `tests/cpp/tisc_binary_io_determinism_test.cpp`, `tests/cpp/v1_canonical_numeric_contract_test.cpp` | `ci.yml` |
| Determinism gate enforcement | reproducibility and determinism policy gates | `docs/governance/DETERMINISM_SURFACE_REGISTRY.md` | `scripts/ci/t81lang_repro_gate.py`, `scripts/ci/run_determinism_slice.sh`, `scripts/ci/check_core_numeric_wrapper_thinness.py` | `repro-ledger.yml`, `ci.yml` |

DCP inclusion is controlled by the Determinism Surface Registry status. If a surface is downgraded from Verified, it is not eligible for DCP guarantees until restored.

## B. Explicitly Excluded from DCP

The following are out of DCP scope unless explicitly upgraded via verified equivalence work and governance approval.

- Trace-JIT (`runtime/jit/`) unless equivalence is proven and registry status is upgraded.
- Experimental tiers (`experimental/tiers/`, including cognitive-tier surfaces) are non-DCP and non-verified unless promoted through governance and registry status upgrade.
- Hanoi VM and related experimental kernel surfaces (`experimental/hanoi/`).
- Distributed compute surfaces (`experimental/distributed/`).
- Research hardware/accelerator paths not covered by verified deterministic contracts.
- Any surface listed as Partial/Unverified/Planned in `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`.

## C. DCP Guarantees

For included DCP surfaces only:

- Bit-exact execution: identical input + configuration yields identical output/trace for supported platforms.
- Cross-architecture determinism: reproducibility across supported architectures as defined in project compatibility policy.
- Frozen semantics for DCP-covered ISA/data-type behaviors under v1.x.
- Forward compatibility discipline: DCP-compatible minors must preserve behavior of prior DCP guarantees.

Non-guarantees (outside DCP): wall-clock performance determinism, network timing determinism, and experimental/research behavior.

## D. Versioning Discipline Under DCP

This section is interpreted in conjunction with `docs/governance/FREEZE_ENFORCEMENT.md`.

- MAJOR (x.0.0): any breaking change to DCP guarantees, frozen ISA semantics, deterministic behavior, or public API compatibility for DCP surfaces.
- MINOR (0.x.0): additive, backward-compatible DCP features that do not alter existing deterministic semantics.
- PATCH (0.0.x): bug fixes, implementation hardening, and performance work with no DCP behavioral changes.

DCP certification is broken by any unresolved regression on Verified determinism surfaces, any unapproved freeze-boundary break, or any release lacking required determinism/structural integrity evidence.

## Release Readiness Requirements (DCP)

A release claiming DCP compliance must include:

- Determinism Surface Registry status snapshot.
- Determinism verification hash summary (repro gate outputs/artifacts).
- Structural integrity status from `docs/status/STRUCTURAL_INTEGRITY_REPORT.md`.
- Release discipline conformance from `docs/product/RELEASE_DISCIPLINE.md`.
