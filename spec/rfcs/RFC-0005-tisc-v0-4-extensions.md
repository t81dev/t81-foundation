______________________________________________________________________

# RFC-0005 — TISC v0.4 Extensions

Version 0.2 — Standards Track\
Status: Accepted\
Updated: 2026-03-15\
Author: TISC Working Group\
Applies to: TISC, T81VM, T81Lang

______________________________________________________________________

# 0. Summary

This RFC enumerates the opcode additions slated for **TISC version 0.4**.
It consolidates open proposals into a coherent plan so compilers and the VM can
target the new instructions simultaneously. Key themes:

1. Deterministic parallel-friendly vector ops.
2. Shape-safe load/store helpers for tensors.
3. Structural-type constructors (Option/Result) already prototyped in code.

______________________________________________________________________

# 1. Motivation

The base ISA (RFC-0000) predated modern tensor and structural-type needs.
Recent work (RFC-0004, Option/Result lowering) highlighted the need for ISA
support to avoid compiler contortions. TISC v0.4 aims to:

- remove ad-hoc instruction sequences (e.g., `LoadImm+Add` for MOV)
- expose canonical constructors for structural values
- provide vector/tensor utilities without sacrificing determinism

______________________________________________________________________

# 2. Design / Specification

### 2.1 Structural Opcodes

- `MAKE_OPTION_SOME`, `MAKE_OPTION_NONE`, `MAKE_RESULT_OK`, `MAKE_RESULT_ERR`
  become official parts of the ISA with the semantics already documented in
  `spec/tisc-spec.md §5.2`. This RFC freezes their encodings and trace hooks.

### 2.2 Deterministic Vector Helpers

- `VLOAD/VSTORE`: shape-aware loads that copy contiguous segments into/out of
  tensor handles. Fault on shape mismatch.
- `VADD/VFMA`: pure elementwise operations that operate on handles instead
  of raw registers, enabling deterministic parallelization later.

### 2.3 Tensor Shape Guards

- `CHKSHAPE RD, RS, SHAPE_DESC`: compares a tensor handle against an encoded
  shape tuple, writing 1/0 into `RD`. Used by the compiler to guard user-level
  assertions and by Axion policies to validate runtime behavior.

### 2.4 ISA Version Reporting

- `READ_ISA_VERSION RD`: writes the ISA version constant (currently `0x0004`)
  into `RD`. Allows programs to gate features at runtime in a deterministic way.

______________________________________________________________________

# 3. Rationale

- Structural opcodes turn current compiler/VMinternal machinery into public
  ISA, reducing risk of semantic drift.
- Vector/tensor helpers standardize deterministic parallel-friendly patterns
  without introducing nondeterministic scheduling.
- Shape guards provide a lightweight alternative to trapping on invalid shapes,
  letting T81Lang emit branchable checks when appropriate.

______________________________________________________________________

# 4. Backwards Compatibility

- New opcodes are strictly additive; ISA v0.3 programs run unchanged under
  v0.4.
- Compilers may probe `READ_ISA_VERSION` to decide whether to emit the new
  instructions or fall back to legacy lowering sequences.

______________________________________________________________________

# 5. Security Considerations

- Vector helpers mirror existing semantics; they must never bypass Axion’s
  visibility into tensor contents. Trace entries include source/destination
  handles and shape metadata.
- `READ_ISA_VERSION` exposes no additional privilege information.

______________________________________________________________________

# 6. Open Questions

1. Should `VLOAD/VSTORE` support strided access, or is canonical contiguous
   layout sufficient for v0.4?
2. Do we require deduplication for vector results similar to tensor pools?
3. How soon should we reserve opcode space for deterministic parallel launch
   (e.g., `PARALLEL_FOR`), or does that belong in a later RFC?

______________________________________________________________________

## Acceptance Criteria

| ID | Criterion | Evidence |
| :--- | :--- | :--- |
| [A-0005-01] | §2.1 Structural opcodes frozen at encodings 46–49 and handled in VM | `include/t81/isa/opcodes.hpp` (MakeOptionSome=46 … MakeResultErr=49); `core/vm/vm.cpp`; `tests/cpp/tisc_opcode_family_semantics_test.cpp`, `vm_state_transition_conformance_matrix_test.cpp` |
| [A-0005-02] | §2.2 `VLoad`/`VStore`: shape-aware reshape/copy with `ShapeFault` on element-count or shape mismatch | `tensor_vload_checked()`, `tensor_vstore_checked()` in `core/vm/tensor_helpers.cpp`; `tests/cpp/vm_tisc_v04_extensions_test.cpp` (`test_vload_reshape`, `test_vload_shape_fault`, `test_vstore_validated_copy`, `test_vstore_shape_fault`) |
| [A-0005-03] | §2.2 `VAdd`/`VFma`: elementwise add and fused multiply-accumulate on handles; `ShapeFault` on shape mismatch | `core/vm/vm.cpp` VAdd/VFma handlers; `tensor_vfma_checked()` in `tensor_helpers.cpp`; `tests/cpp/vm_tisc_v04_extensions_test.cpp` (`test_vadd_*`, `test_vfma_*`) |
| [A-0005-04] | §2.4 `READ_ISA_VERSION`: writes constant 4 into destination register | `core/vm/vm.cpp` ReadIsaVersion handler; `tests/cpp/vm_tisc_v04_extensions_test.cpp` (`test_read_isa_version`) |
| [A-0005-05] | §2.3 `CHKSHAPE`: shape guard writes 1/0 to dest register; existing since opcode 45 | `core/vm/vm.cpp` ChkShape handler; `tests/cpp/vm_tensor_test.cpp` shape-check block |
| [A-0005-06] | All v0.4 opcodes emit Axion audit events on execution | `tests/cpp/vm_tisc_v04_extensions_test.cpp` (`test_v04_axion_trace`) |
| [A-0005-07] | Binary emitter maps IR v0.4 opcodes to TISC opcodes correctly | `core/isa/binary_emitter.cpp` VLOAD/VSTORE/VADD/VFMA/READ_ISA_VERSION cases |

Open question 1 (§6): resolved as **contiguous-only** for v0.4. Strided access is deferred to a future RFC.
Open questions 2–3 remain open and do not block acceptance.

______________________________________________________________________

## Acceptance Note (2026-03-15)

All seven criteria above are met as of this date.

**§2.1 Structural Opcodes** — `MakeOptionSome` (46), `MakeOptionNone` (47), `MakeResultOk` (48), `MakeResultErr` (49) are frozen in `include/t81/isa/opcodes.hpp` and fully handled in `core/vm/vm.cpp`. Opcode assignments MUST NOT change.

**§2.2 Vector Helpers** — `VLoad`/`VStore` perform shape-validated tensor reshape and copy respectively; `VAdd` performs elementwise addition on tensor handles; `VFma` performs fused multiply-accumulate (`RD = RS1 × RS2 + RD`). All four use contiguous-only layout (open question 1 resolved). Shape mismatches produce deterministic `ShapeFault`. Implementations are in `core/vm/vm.cpp` handlers backed by `tensor_vload_checked()`, `tensor_vstore_checked()`, and `tensor_vfma_checked()` in `core/vm/tensor_helpers.cpp`.

**§2.3 Shape Guards** — `ChkShape` (opcode 45) was already implemented prior to this RFC; it writes 1/0 to the destination register based on handle-vs-shape-pool comparison.

**§2.4 ISA Version Reporting** — `ReadIsaVersion` writes the constant `4` (TISC v0.4) into the destination register. No privilege or side effects.

Test suite: `tests/cpp/vm_tisc_v04_extensions_test.cpp` — **10/10 assertions passing**.
Suite status at acceptance: **all pre-existing tensor, VM, and opcode tests unaffected**.
