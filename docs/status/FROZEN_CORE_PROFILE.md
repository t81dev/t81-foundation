# Frozen Core Profile

Status: Constitutional — Do Not Edit Without Major Version Bump
Last Updated: 2026-03-19
Authority: `spec/tisc-spec.md` > `docs/product/DETERMINISTIC_CORE_PROFILE.md` > this document

## What This Document Is

The authoritative operational boundary of the Frozen Core. It answers one
question: **what is unconditionally guaranteed, and what is not**.

This is not a narrative. Changes to this document require the same governance
review as a spec change.

---

## 1. Directory Boundary

The Frozen Core is exactly these paths. Nothing else.

```
core/types/          ← Data types (T81BigInt, T81Float, T81Fixed, T81Complex, etc.)
core/isa/            ← TISC ISA definition and opcode encoding
core/vm/             ← Reference interpreter (non-JIT path only)
include/t81/         ← Public C++ API (the only stable external surface)
```

Everything outside this boundary is non-frozen unless explicitly stated below.

---

## 2. Opcode Whitelist

All opcodes are defined in `spec/tisc-spec.md` (Frozen, v1.1.0). The TISC ISA
is frozen at v1. No opcode semantics, encoding, or dispatch behavior may be
altered without:

1. A major version bump (v2.x).
2. A governance review and updated freeze document.
3. A new CI freeze-integrity check entry.

CI enforcement: `scripts/ci/check_tisc_freeze_integrity.py`

---

## 3. Determinism Guarantees

The following guarantees hold **only for the Directory Boundary above**:

| Guarantee | Scope | CI Gate |
| :--- | :--- | :--- |
| **Bit-exact execution** | Identical input + config → identical output + trace on supported platforms | `gate / tritwise-determinism`, `gate / determinism slice` |
| **Cross-arch reproducibility** | x86_64 and arm64 produce identical output | `gate / t81lang cross-arch bit-identity`, `gate / t3k cross-arch bit-identity` |
| **Frozen ISA semantics** | Opcode behavior is immutable under v1.x | `check_tisc_freeze_integrity.py` |
| **Canonical encoding stability** | Binary wire format is bit-stable across versions | `v1_canonical_numeric_contract_test.cpp`, `tisc_binary_io_determinism_test.cpp` |
| **Soft-float determinism** | `T81Float` operations use strict rounding, no hardware FPU | `test_T81Float_arithmetic.cpp`, `test_T81Float_rounding.cpp` |

These guarantees do **not** apply to wall-clock performance, network timing, or
any surface outside the Directory Boundary.

---

## 4. Verified Determinism Surfaces (Registry Snapshot)

Authoritative source: `docs/governance/DETERMINISM_SURFACE_REGISTRY.md`

| Surface | Status | Verification Path |
| :--- | :--- | :--- |
| TISC Opcode Semantics | **Verified** | `vm_determinism_property_test.cpp`, `test_tritwise_backend_equivalence.cpp` |
| VM Interpreter Execution | **Verified** | `vm_trace_test.cpp`, `vm_determinism_property_test.cpp` |
| Data Type Canonical Encoding | **Verified** | `v1_canonical_numeric_contract_test.cpp`, `tisc_binary_io_determinism_test.cpp` |
| Soft-Float Deterministic Math | **Verified** | `test_T81Float_arithmetic.cpp`, `test_T81Float_rounding.cpp` |
| Compiler Bytecode Emission | **Partial** | `scripts/ci/t81lang_repro_gate.py`, fixture corpus in `tests/fixtures/t81lang_determinism/` |
| T3K Quantization | **Verified** | `scripts/ci/t3k_repro_gate.py` |

**Partial** means: fixture-bounded determinism is verified; full spec-to-bytecode
traceability has a known gap (see `DRIFT_DECOMPOSITION.md` — Compiler Bytecode row).

---

## 5. Explicit Exclusions

The following are **outside the Frozen Core** and carry **no DCP guarantees**:

| Surface | Path | Status | Reason |
| :--- | :--- | :--- | :--- |
| Trace-JIT | `runtime/jit/` | Non-DCP | Equivalence unproven; disabled by default |
| Cognitive Tiers | `experimental/tiers/` | Non-DCP | Experimental; consensus determinism not verified |
| Hanoi VM | `experimental/hanoi/` | Non-DCP | Experimental kernel surface |
| Distributed Compute | `experimental/distributed/` | Non-DCP | Network-layer; non-deterministic by design |
| Axion Governance Kernel | `kernel/axion/` | Governed non-DCP | Scope-bounded evidence exists, including CI-enforced experimental epoch scheduler/audit parity, but broader kernel/governance behavior is outside the current DCP boundary |
| Axion OS | `userland/experimental/` | Governed non-DCP / experimental | Experimental OS kernel path governed by RFC-00B3 and external promotion gates; `axion-epoch-determinism` proves bounded pooled-vs-unbounded epoch parity only |
| T81Lang Frontend | `lang/frontend/` | Governed non-DCP | Compiler/toolchain determinism remains partial and fixture-bounded; language-spec stability does not imply DCP promotion |
| T81Graph | `lang/frontend/` (graph surface) | Governed non-DCP | Useful implemented surface, but not a verified deterministic surface as a whole |
| llama.cpp adapter | `third_party/llama.cpp`, `tooling/model/` | Governed non-DCP | AGI inference; practical reproducibility only |
| Hardware FPU | — | Excluded | Platform-dependent; use `T81Float` soft-float instead |
| Network IO | — | Excluded | Non-deterministic by design |
| Real-time Scheduling | — | Excluded | Platform-dependent |

---

## 6. Build-Layer Firewall

The dependency firewall is enforced by structural integrity checks:

- `include/t81/` must not include experimental headers.
- `core/` must not link against `runtime/jit/`, `experimental/`, or `kernel/axion/` except
  through the explicit, audited VM policy hook.
- No active dependency firewall waivers. Historical waiver at `vm/vm.cpp` was retired on 2026-03-05.
- CI enforcement: `scripts/ci/check_core_numeric_wrapper_thinness.py`
- Status: **PASS** (as of 2026-02-28)

---

## 7. Deterministic Corpus

The canonical fixture corpus that backs the Compiler Bytecode Partial verification:

- **Location:** `tests/fixtures/t81lang_determinism/` (16 programs: `01_bigint_add.t81` → `16_symbol_equality_branch_print.t81`)
- **Hash references:** `t81lang_repro_hash.txt`, `t81lang_ast_ir_repro_hash.txt`
- **Verification:** `python3 scripts/ci/t81lang_repro_gate.py`
- **Hash refresh:** `t81lang-repro-hash-refresh.yml` workflow

---

## 8. DCP Versioning Rule

| Change Type | Semver | Governance Required |
| :--- | :--- | :--- |
| Breaking change to frozen ISA, DCP behavior, or public API | **MAJOR** | Yes — governance review + spec update |
| Additive DCP feature, no behavioral change | **MINOR** | Yes — registry update |
| Bug fix, implementation hardening, no DCP behavioral change | **PATCH** | No — but must not regress any Verified surface |

A Verified-surface regression is a **critical defect** and must not be merged
unresolved regardless of PR scope.

---

## Cross-References

- `spec/tisc-spec.md` (frozen authority)
- `docs/governance/DETERMINISM_SURFACE_REGISTRY.md` (registry authority)
- `docs/product/DETERMINISTIC_CORE_PROFILE.md` (product boundary)
- `docs/governance/FREEZE_ENFORCEMENT.md` (breach protocol)
- `docs/status/DRIFT_DECOMPOSITION.md` (where the core drifts)
- `docs/status/DETERMINISM_AUDIT_LOG.md` (audit history)
- `spec/rfcs/RFC-0048-deterministic-surface-definition-and-governance-boundaries.md` (surface classification boundary)
