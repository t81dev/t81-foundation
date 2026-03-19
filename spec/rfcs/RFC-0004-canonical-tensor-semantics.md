______________________________________________________________________

# RFC-0004 — Canonical Tensor Semantics

Version 0.1 — Standards Track\
Status: Accepted\
Updated: 2026-03-15\
Author: T81 Foundation Tensor Working Group\
Applies to: Data Types, TISC, T81VM, T81Lang, Axion

______________________________________________________________________

# 0. Summary

This RFC normatively defines tensor semantics across the full T81 stack.
It connects the requirements already sketched in `spec/t81-data-types.md §5`
and `spec/t81lang-spec.md §5.13` into a single canonical contract so that:

1. **Tensor shapes** remain deterministic at every layer.
2. **Tensor arithmetic** maps unambiguously to TISC opcodes.
3. **VM layout** for tensor pools is canonical and Axion-visible.

______________________________________________________________________

# 1. Motivation

Tensors are the core data structure for Axion-supervised cognition, yet the
current specifications distribute their requirements across multiple documents.
This RFC consolidates the canonical rules so compiler, VM, and ISA changes do
not diverge. It also provides a home for future tensor features (broadcast,
contraction, distributed layout) before they land in the base specs.

Goals:

- single source of truth for ranks, shapes, padding, strides
- deterministic lowering from T81Lang to TISC tensor opcodes
- canonical serialization so Axion can validate tensor traces

Non-goals:

- defining new tensor instructions (covered by later RFCs such as RFC-0005)
- changing cognitive tier limits (see `spec/cognitive-tiers.md`)

______________________________________________________________________

# 2. Design / Specification

### 2.1 Canonical Shape Model

- Shapes are immutable tuples of non-negative balanced-ternary integers.
- Zero-length dimensions are permitted but must carry explicit intent;
  they collapse arithmetic to canonical zero tensors.
- Shape arithmetic (broadcast, contraction) MUST be deterministic and
  defined purely as functions over shape tuples with no hidden state.

### 2.2 Tensor Pools

- TISC programs reference tensors via 1-based handles, mirroring the
  existing float/fraction pools.
- VM must intern tensors deterministically: identical contents may share
  handles but deduplication MUST be pure (no randomization).
- Axion MUST see metadata: shape, rank, canonical hash.

### 2.3 Canonical Operations

- `TVECADD`, `TMATMUL`, `TTEN DOT` already exist in the ISA; this RFC
  defines their canonical semantics (shape validation, overflow semantics).
- Broadcasting semantics follow numpy-style rules but encoded in base-81:
  trailing dimensions align, dimension mismatch triggers a deterministic fault.
- Tensor contraction MUST specify index ordering explicitly; no implicit
  transposition is allowed.

### 2.4 Language Integration

- T81Lang tensor literals MUST include explicit rank annotations.
- Type checker enforces shape compatibility using canonical rules above.
- Lowering emits tensor handles plus opcodes listed in §2.3.

### 2.5 Axion Hooks

- Every tensor opcode records shape metadata into the execution trace.
- Axion policies can veto operations whose shapes violate tier limits.

______________________________________________________________________

# 3. Rationale

- Canonical shapes prevent silent dimension drift common in binary tensor
  stacks.
- Deduplicated tensor handles enable structural equality without expensive
  deep compares.
- Broadcasting + contraction rules align with existing data-type semantics,
  minimizing cognitive overhead for users of the stack.

______________________________________________________________________

# 4. Backwards Compatibility

- No existing canonical program regresses; all previously valid tensors already
  satisfied the implied rules.
- VM/ISA updates are additive: new metadata tables and trace hooks are
  backward-compatible.

______________________________________________________________________

# 5. Security Considerations

- Deterministic shape metadata gives Axion better insight into tensor-heavy
  cognition, lowering the risk of unbounded memory growth.
- Canonical serialization eliminates hash collisions that could otherwise be
  exploited to hide malicious tensors.

______________________________________________________________________

# 6. Open Questions

1. Should tensor pooling deduplicate by hash or by full structural compare?
2. How should distributed tensors (future T6561 tiers) extend this RFC?
3. Do we reserve opcodes for stride-aware loads/stores now or defer to RFC-0005?

______________________________________________________________________

# 7. Acceptance Criteria

| ID | Criterion | Evidence |
| :--- | :--- | :--- |
| [A-0004-01] | Canonical shape model: shape tuples are immutable, deterministic, and fault on mismatch | `tests/cpp/tensor_shape_test.cpp`, `tests/cpp/vm_tensor_shape_faults_test.cpp` |
| [A-0004-02] | Tensor pools: VM maintains 1-based handles; `add_tensor()` returns deterministic handles | `include/t81/vm/state.hpp` (`tensors` vector); `tests/cpp/vm_tensor_test.cpp`, `tests/cpp/vm_tensor_get_set_conformance_test.cpp` |
| [A-0004-03] | Canonical ops (`TVECADD`, `TMATMUL`): shape validation enforced; mismatch triggers deterministic fault | `tests/cpp/tensor_matmul_test.cpp`, `tests/cpp/tensor_elementwise_test.cpp`, `tests/cpp/vm_tensor_shape_faults_test.cpp` |
| [A-0004-04] | Axion hooks: shape metadata recorded in execution trace for every tensor opcode | `tests/cpp/vm_tensor_provenance_trace_test.cpp`, `tests/cpp/canonfs_axion_trace_test` |
| [A-0004-05] | T81Lang integration: tensor literals with rank annotations type-checked; lowering emits canonical handles + opcodes | `tests/cpp/frontend_ir_generator_test.cpp`, `tests/cpp/tensor_broadcast_test.cpp` |
| [A-0004-06] | Deterministic serialization: tensor contents serialize canonically; identical tensors hash identically | `tests/cpp/tensor_serialization_canonical_fixed_test.cpp`, `tests/cpp/tensor_native_decode_test.cpp` |

______________________________________________________________________

## Acceptance Note (2026-03-15)

All six criteria above are met as of this date.

Key implementation mapping:

- **§2.1 Canonical Shape Model** — `T729DynamicTensor` shape tuples in `include/t81/tensor.hpp` are immutable after construction; `vm_tensor_shape_faults_test.cpp` verifies deterministic faults on dimension mismatches across all tensor ops.
- **§2.2 Tensor Pools** — `State::tensors` (`std::vector<std::optional<T729DynamicTensor>>`) provides 1-based handle allocation; `free_tensor_indices` enables deterministic recycling; `vm_tensor_get_set_conformance_test.cpp` confirms handle semantics.
- **§2.3 Canonical Operations** — `TVECADD`, `TMATMUL`, and elementwise ops are implemented in `core/vm/vm.cpp` with shape-validation guards; `tensor_matmul_test.cpp` and `tensor_elementwise_test.cpp` cover correct and fault paths.
- **§2.4 Language Integration** — The IR generator lowers T81Lang tensor literals to handle-allocation + opcode sequences; `tensor_broadcast_test.cpp` confirms numpy-style trailing-dimension alignment with deterministic fault on mismatch.
- **§2.5 Axion Hooks** — `vm_tensor_provenance_trace_test.cpp` verifies that tensor opcodes record shape metadata into the Axion trace. Policies using `(deny-shape ...)` are enforced by `policy_engine.cpp`.

Open questions 1–3 (§6) are deferred: deduplication strategy, distributed tensor layout, and stride-aware load/store opcodes will be addressed in follow-on work (stride-aware ops are a candidate for RFC-0005 v0.4 or a dedicated RFC).

Suite status at acceptance: **30+ tensor-specific tests passing; 329/332 total**.
