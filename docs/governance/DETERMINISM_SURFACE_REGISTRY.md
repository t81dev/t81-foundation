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
* `spec/rfcs/RFC-0042-deterministic-backend-equivalence-contract.md` (backend substitution constitution)
* `spec/rfcs/RFC-0043-deterministic-conformance-validation-framework.md` (proof and CI model)
* `spec/rfcs/RFC-0044-stable-packed-trit-vector-interface.md` (packed-trit substrate)
* `spec/rfcs/RFC-0045-deterministic-memory-model.md` (state visibility and aliasing)
* `spec/rfcs/RFC-0046-deterministic-scheduling-and-execution-ordering.md` (ordering constitution)
* `spec/rfcs/RFC-0047-deterministic-jit-and-lowering-rules.md` (lowering and trace equivalence constraints)
* `spec/rfcs/RFC-0048-deterministic-surface-definition-and-governance-boundaries.md` (surface classification constitution)
* `spec/rfcs/RFC-0049-canonical-ternary-arithmetic-semantics.md` (arithmetic oracle)
* `spec/rfcs/RFC-0050-vectorized-ternary-operations-for-tisc.md` (ISA/VM vector semantics)
* `spec/rfcs/RFC-0051-deterministic-heterogeneous-acceleration.md` (accelerator boundary)
* `spec/rfcs/RFC-0052-canonical-dataflow-and-state-driven-execution.md` (dataflow/state model)
* `spec/rfcs/RFC-0053-distributed-deterministic-execution-protocol.md` (distributed execution constitution)

---

## 2. Definition of a Determinism Surface

A determinism surface is:

> A subsystem boundary for which identical input and configuration must produce bit-identical output across supported architectures. “Supported architectures” are defined by the platforms listed in the root README under Compatibility for the current major version.

Clarify:

* Surfaces must have explicit verification.
* Surfaces without verification are not guaranteed.
* Backend acceleration does not expand semantics; it must inherit scalar-oracle equivalence per RFC-0042.
* Verification claims must be justified by executable conformance and replay artifacts per RFC-0043.

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

## 3.1 Governing Draft RFCs For Verified Surfaces

The following draft RFCs define the constitutional model that verified deterministic surfaces are expected to converge toward:

| RFC | Role |
| ------- | ----- |
| **RFC-0042** | Scalar-oracle backend equivalence for scalar ↔ SWAR ↔ SIMD ↔ future backends |
| **RFC-0043** | Common conformance, replay, breach-classification, and CI proof model |
| **RFC-0044** | Stable packed-trit substrate beneath SWAR and SIMD surfaces |
| **RFC-0045** | Canonical memory visibility, handle identity, and aliasing model |
| **RFC-0046** | Program-order / dependency-order / canonical-commit scheduling constitution |
| **RFC-0047** | Deterministic lowering and JIT constraints for alternate execution paths |
| **RFC-0048** | DCP / governed non-DCP / experimental boundary constitution |
| **RFC-0049** | Canonical arithmetic oracle for trit, packed, native, VM, and backend arithmetic |
| **RFC-0050** | Semantic vector execution model at the ISA/VM layer |
| **RFC-0051** | Heterogeneous acceleration rules for GPU and other accelerator backends |
| **RFC-0052** | Canonical dataflow and state-driven propagation model above DPE |
| **RFC-0053** | Distributed deterministic execution protocol for future cross-node execution |

Until these RFCs are accepted and fully integrated, they should be treated as draft governance direction rather than an expansion of currently verified guarantees.

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
* **JIT Optimizations**: Trace-JIT equivalence remains constrained by RFC-0042, RFC-0043, and RFC-0047 direction, but is not yet a fully verified surface.
* **Backend Acceleration Beyond Current Verified CPU Paths**: Any future backend must satisfy RFC-0042 equivalence, RFC-0043 validation, RFC-0049 arithmetic, and RFC-0051 accelerator-boundary requirements before deterministic claims are expanded.
* **Dataflow/Reactive Execution Beyond Current DPE Scope**: Any future state-driven execution surface must satisfy RFC-0052 before deterministic claims are expanded.
* **Distributed Execution**: Any future cross-node execution surface must satisfy RFC-0053 before deterministic claims are expanded.
* **External Hardware Accelerators**: Behavior on non-CPU devices is currently undefined.

---

## 5. Verification Map

| Surface | Test Path | CI Job | Repro Script |
| ------- | --------- | ------ | ------------ |
| **TISC Execution** | `tests/cpp/vm_determinism_property_test.cpp` | `ci.yml` | N/A |
| **Tritwise Ops** | `tests/cpp/test_tritwise_backend_equivalence.cpp` | `ci.yml` | N/A |
| **Compiler Repro** | `tests/fixtures/t81lang_determinism/*.t81` | `repro-ledger.yml` | `scripts/ci/t81lang_repro_gate.py` |
| **Quantization** | N/A | `repro-ledger.yml` | `scripts/ci/t3k_repro_gate.py` |

Proof model note:

> Verified-surface claims should be interpreted through RFC-0043 once that framework is accepted; until then, the table above remains authoritative for what is actually CI-enforced today.

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

Determinism governance DOES mean:

* backend substitution must preserve scalar-oracle semantics
* memory visibility must be canonical at the governed boundary
* scheduling may use internal parallelism, but observable ordering must remain deterministic

This prevents overclaim drift.
