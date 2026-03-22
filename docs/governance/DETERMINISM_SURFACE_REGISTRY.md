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
* `docs/developer-guide/internals/jit-equivalence-plan.md` (Future Roadmap)
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
| **VM Interpreter Execution** | Dispatch Loop | Bit-Exact | `tests/cpp/vm_trace_test.cpp`<br>`tests/cpp/vm_determinism_property_test.cpp`<br>`tests/cpp/vm_tensor_get_set_conformance_test.cpp`<br>`tests/cpp/vm_state_transition_conformance_matrix_test.cpp` | Yes (`ci.yml`) | **Verified** |
| **Data Type Canonical Encoding** | Binary Format (Trit/Tryte) | Bit-Exact | `tests/cpp/v1_canonical_numeric_contract_test.cpp`<br>`tests/cpp/tisc_binary_io_determinism_test.cpp` | Yes (`ci.yml`) | **Verified** |
| **Soft-Float Deterministic Math** | `T81Float` Operations | Strict Rounding | `tests/cpp/test_T81Float_arithmetic.cpp`<br>`tests/cpp/test_T81Float_rounding.cpp` | Yes (`ci.yml`) | **Verified** |
| **Compiler Bytecode Emission** | T81Lang to TISC | Bit-Exact (Fixtures) | `scripts/ci/t81lang_repro_gate.py` | Yes (`repro-ledger.yml`) | **Partial** |
| **T3K Quantization** | GGUF Encoding | Bit-Exact | `scripts/ci/t3k_repro_gate.py` | Yes (`repro-ledger.yml`) | **Verified** |

---

## 3.1 Governing RFCs For Verified Surfaces

| RFC | Status | Role |
| --- | ------ | ---- |
| **RFC-0042** | **accepted** | Scalar-oracle backend equivalence for scalar ↔ SWAR ↔ SIMD ↔ future backends |
| **RFC-0043** | **accepted** | Common conformance, replay, breach-classification, and CI proof model |
| **RFC-0044** | **accepted** | Stable packed-trit substrate beneath SWAR and SIMD surfaces |
| **RFC-0045** | **accepted** | Canonical memory visibility, handle identity, and aliasing model |
| **RFC-0046** | **accepted** | Program-order / dependency-order / canonical-commit scheduling constitution |
| **RFC-0047** | **accepted** | Deterministic lowering and JIT constraints for alternate execution paths |
| **RFC-0048** | **accepted** | DCP / governed non-DCP / experimental boundary constitution |
| **RFC-0049** | **accepted** | Canonical arithmetic oracle for trit, packed, native, VM, and backend arithmetic |
| **RFC-0050** | **accepted** | Semantic vector execution model at the ISA/VM layer |
| **RFC-0051** | **accepted** | Heterogeneous acceleration rules for GPU and other accelerator backends |
| **RFC-0052** | **accepted** | Canonical dataflow and state-driven propagation model above DPE |
| **RFC-0053** | **accepted** | Distributed deterministic execution protocol for future cross-node execution |
| **RFC-0054** | **accepted** | CanonFS object identity and persistence contract (payload-only hash rule) |
| **RFC-0055** | **accepted** | Native ternary hardware target classification and interop governance |

RFCs marked `accepted` are fully integrated and binding.  Draft RFCs define
governance direction but do not yet expand verified guarantees.

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
* **Axion Epoch Runtime (TernaryOS + DPE)**: governed non-DCP proof surface with CI-enforced scheduler/audit parity in the experimental kernel lane; not part of the interpreter DCP boundary.
* **Backend Acceleration Beyond Current Verified CPU Paths**: Any future backend must satisfy RFC-0042 equivalence, RFC-0043 validation, RFC-0049 arithmetic, and RFC-0051 accelerator-boundary requirements before deterministic claims are expanded.
* **Dataflow/Reactive Execution Beyond Current DPE Scope**: Any future state-driven execution surface must satisfy RFC-0052 before deterministic claims are expanded.
* **Distributed Execution**: Any future cross-node execution surface must satisfy RFC-0053 before deterministic claims are expanded.
* **External Hardware Accelerators (governed non-DCP)**: Classified as governed non-DCP per RFC-0051 §1 and `spec/tisc-spec.md §5.2.5`.  Any future accelerator backend must satisfy RFC-0042 equivalence, RFC-0043 validation, RFC-0045 memory transfer audit, RFC-0046 scheduling audit, and RFC-0048 boundary promotion before any DCP claim may be made.

---

## 5. Verification Map

| Surface | Test Path | CI Job | Repro Script |
| ------- | --------- | ------ | ------------ |
| **TISC Execution** | `tests/cpp/vm_determinism_property_test.cpp` | `ci.yml` | N/A |
| **VM Memory / State Determinism** | `tests/cpp/vm_tensor_get_set_conformance_test.cpp`<br>`tests/cpp/vm_state_transition_conformance_matrix_test.cpp` | `ci.yml` (`determinism-slice`) | N/A |
| **Tritwise Backend Equivalence** (RFC-0042) | `tests/cpp/test_tritwise_backend_equivalence.cpp` — scalar vs SWAR vs AVX2/NEON, sizes 1–4097 trits | `ci.yml` | N/A |
| **Arithmetic Backend Equivalence** (RFC-0042 + RFC-0049) | `tests/cpp/test_arithmetic_backend_equivalence.cpp` — scalar trit oracle vs T81BigInt for ADD/SUB/MUL/NEG/comparison/carry | `ci.yml` | N/A |
| **Axion Epoch Runtime (scheduler + audit parity)** | `experimental/ternaryos/tests/epoch_submission_test.cpp`<br>`experimental/ternaryos/tests/epoch_audit_test.cpp` | `ci.yml` (`axion-epoch-determinism`) | N/A |
| **Compiler Repro** | `tests/fixtures/t81lang_determinism/*.t81` | `repro-ledger.yml` | `scripts/ci/t81lang_repro_gate.py` |
| **Quantization** | N/A | `repro-ledger.yml` | `scripts/ci/t3k_repro_gate.py` |

Proof model note:

> Verified-surface claims are governed by RFC-0043 (Deterministic Conformance Validation Framework, `accepted`).
> The table above is the authoritative CI-enforced record.  RFC-0043 §8 defines the promotion
> rules that every entry must satisfy before a surface may be marked `Verified`.

### 5.1 Conformance Layer Mapping (RFC-0043 §1)

Each verified surface maps to one or more RFC-0043 conformance layers:

| Surface | Semantic | Backend Equiv | Serialization | Cross-Platform | Gov Gate |
| ------- | :------: | :-----------: | :-----------: | :------------: | :------: |
| TISC Execution | Yes | — | — | Yes | Yes |
| VM Memory / State | Yes | — | Yes | Yes | Yes |
| Tritwise Backend Equiv | Yes | Yes | — | Yes | Yes |
| Arithmetic Backend Equiv | Yes | Yes | — | Yes | Yes |
| Axion Epoch Runtime | Yes | — | — | Yes | Yes |
| Compiler Repro | — | — | Yes | Yes | Yes |
| Quantization | — | — | Yes | Yes | Yes |

### 5.2 Cross-Platform Replay Artifacts

The supported architecture set for the current major version is **x86_64 + AArch64**.

| Architecture | Kernel | Compiler | Backend Paths Active |
| ------------ | ------ | -------- | -------------------- |
| x86_64 + AVX2 | Linux / macOS | Clang 18+ / GCC 14+ | scalar, SWAR, AVX2 (≥ 64 B threshold) |
| AArch64 + NEON | Linux / macOS | AppleClang 17+ / Clang 18+ | scalar, SWAR, NEON-OR (≥ 64 B threshold) |

Replay succeeds only when all governed test binaries produce **bit-identical pass/fail outcomes**
across both rows.  Architecture-specific exclusions (NEON TNot/TAnd disabled) are documented
in `spec/tisc-spec.md §5.2.3` and do not constitute a divergence.

---

## 6. Breach Handling Reference

Reference:

* `docs/governance/FREEZE_ENFORCEMENT.md`
* `spec/rfcs/RFC-0043-deterministic-conformance-validation-framework.md` §6

Breach classification (per RFC-0043 §6):

* **Hard Divergence** — final bytes, trap class, trace hash, or canonical serialization differ.
  Merge-blocking critical defect on a verified surface.
* **Soft Divergence** — non-DCP diagnostic strings or non-governed metadata differ.
  Not a DCP failure unless it crosses a governed boundary.
* **Undefined Behavior Exposure** — backend depends on UB, memory layout assumptions break,
  or ABI mismatch changes results.  Treated as a Hard Divergence if on a verified surface.

State:

> A regression on a verified surface is classified as a Hard Divergence (RFC-0043 §6)
> and must not be merged unresolved.

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
