# Determinism Surface Registry

## 1. Purpose

Define what is and is not guaranteed to be deterministic in the T81 system.

Clarify that determinism guarantees apply only to explicitly verified surfaces.

Threat modeling for these surfaces is documented in `DETERMINISM_THREAT_MODEL.md`.

Reference:

* `spec/supplemental/deterministic-core-profile-v1.md` (Core Definitions)
* `docs/governance/FREEZE_ENFORCEMENT.md` (Break Protocols)
* `docs/governance/SPEC_AUTHORITY_MODEL.md` (Hierarchy)
* `docs/status/VERIFIED_SURFACE_AUDIT.md` (Traceability Audit)
* `docs/guides/jit-equivalence-plan.md` (Future Roadmap)

---

## 2. Definition of a Determinism Surface

A determinism surface is:

> A subsystem boundary for which identical input and configuration must produce bit-identical output across supported architectures. “Supported architectures” are defined by the platforms listed in the root README under Compatibility for the current major version.

Clarify:

* Surfaces must have explicit verification.
* Surfaces without verification are not guaranteed.

---

## 3. Verified Determinism Surfaces

| Surface | Scope | Guarantee | Verification Mechanism | CI Enforced | Status |
| ------- | ----- | --------- | ---------------------- | ----------- | ------ |
| **TISC Opcode Semantics** | Instruction Behavior | Bit-Exact | `tests/cpp/vm_determinism_property_test.cpp`<br>`tests/cpp/test_tritwise_backend_equivalence.cpp` | Yes (`ci.yml`) | **Verified** |
| **VM Interpreter Execution** | Dispatch Loop | Bit-Exact | `tests/cpp/vm_trace_test.cpp`<br>`tests/cpp/vm_determinism_property_test.cpp` | Yes (`ci.yml`) | **Verified** |
| **Data Type Canonical Encoding** | Binary Format (Trit/Tryte) | Bit-Exact | `tests/cpp/v1_canonical_numeric_contract_test.cpp`<br>`tests/cpp/tisc_binary_io_determinism_test.cpp` | Yes (`ci.yml`) | **Verified** |
| **Soft-Float Deterministic Math** | `T81Float` Operations | Strict Rounding | `tests/cpp/test_T81Float_arithmetic.cpp`<br>`tests/cpp/test_T81Float_rounding.cpp` | Yes (`ci.yml`) | **Verified** |
| **Compiler Bytecode Emission** | T81Lang to TISC | Bit-Exact (Fixtures) | `scripts/ci/t81lang_repro_gate.py` | Yes (`repro-ledger.yml`) | **Partial** |
| **T3K Quantization** | GGUF Encoding | Bit-Exact | `scripts/ci/t3k_repro_gate.py` | Yes (`repro-ledger.yml`) | **Verified** |

---

## 4. Non-Deterministic or Undefined Surfaces

Explicitly list areas that are:

* Experimental
* Not yet verified
* Outside determinism scope

**Excluded / Out of Scope:**

* **Real-time Scheduling**: Execution timing is platform-dependent.
* **Network IO**: Network latency and packet ordering are not deterministic.
* **Hardware FPU**: Native hardware floating-point behavior (outside of `T81Float` soft-float wrappers) is not guaranteed.
* **Performance Determinism**: Runtime latency is not guaranteed to be constant.

**Experimental / Planned:**

* **Distributed Cognitive Tiers** (`experimental/tiers/`, `experimental/distributed/`): Consensus determinism is planned but not verified.
* **JIT Optimizations**: Trace-JIT equivalence is planned but currently stubbed.
* **External Hardware Accelerators**: Behavior on non-CPU devices is currently undefined.

---

## 5. Verification Map

| Surface | Test Path | CI Job | Repro Script |
| ------- | --------- | ------ | ------------ |
| **TISC Execution** | `tests/cpp/vm_determinism_property_test.cpp` | `ci.yml` | N/A |
| **Tritwise Ops** | `tests/cpp/test_tritwise_backend_equivalence.cpp` | `ci.yml` | N/A |
| **Compiler Repro** | `tests/fixtures/t81lang_determinism/*.t81` | `repro-ledger.yml` | `scripts/ci/t81lang_repro_gate.py` |
| **Quantization** | N/A | `repro-ledger.yml` | `scripts/ci/t3k_repro_gate.py` |

---

## 6. Breach Handling Reference

Reference:

* `docs/governance/FREEZE_ENFORCEMENT.md`

State:

> A regression on a verified surface is classified as a critical defect and must not be merged unresolved.

---

## 7. Scope Boundaries

Clarify:

Determinism does NOT mean:

* Real-time scheduling guarantees
* Network determinism
* Performance determinism
* Hardware FPU determinism (outside soft-float)

This prevents overclaim drift.
