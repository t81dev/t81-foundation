______________________________________________________________________

# RFC-0005 — TISC v0.4 Extensions

Version 0.1 — Draft (Standards Track)\
Status: Draft\
Author: TISC Working Group\
Applies to: TISC, T81VM, T81Lang

______________________________________________________________________

# 0. Summary

This RFC enumerates the opcode additions slated for **TISC version 0.4**.
It consolidates open proposals into a coherent plan so compilers and the VM can
target the new instructions simultaneously. Key themes:

1. Deterministic parallel-friendly vector ops.
1. Shape-safe load/store helpers for tensors.
1. Structural-type constructors (Option/Result) already prototyped in code.

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
1. Do we require deduplication for vector results similar to tensor pools?
1. How soon should we reserve opcode space for deterministic parallel launch
   (e.g., `PARALLEL_FOR`), or does that belong in a later RFC?

______________________________________________________________________
