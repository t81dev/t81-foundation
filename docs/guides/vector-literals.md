# Vector Literals & Canonical Tensors

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [Vector Literals & Canonical Tensors](#vector-literals-&-canonical-tensors)
  - [Inference & Canonical Forms](#inference-&-canonical-forms)
  - [IR & Runtime Canonicalization](#ir-&-runtime-canonicalization)
  - [Testing & Coverage](#testing-&-coverage)

<!-- T81-TOC:END -->






Vector literals (`[a, b, c]`) are now first-class, canonical constructs in the C++ frontend and obey the deterministic rules listed in [`spec/t81lang-spec.md` §2.3–§2.5](../spec/t81lang-spec.md#2-3-vector-type).

## Inference & Canonical Forms

1. **Every element** must be a numeric literal. Non-empty lists infer their `Vector[T]` type from the *first* numeric element, and subsequent elements must either share that primitive type or widen deterministically (e.g., `i32` → `Float`).  
2. **Empty vectors** (`[]`) are only legal when a contextual `Vector[T]` (or `Tensor[T, ...]`) type guides inference; otherwise the semantic analyzer reports an error.  
3. The analyzer records the parsed floating-point payload, so later passes can treat the literal as immutable, canonical data.

## IR & Runtime Canonicalization

When the semantic pass resolves a vector literal:

1. The literal payload is converted into a rank-1 `T729Tensor` (shape `[N]`, data stored as `float`).  
2. `IRGenerator` interns that tensor in the IR tensor pool and emits a `LOADI` literal referring to a `TensorHandle`, so every literal becomes a canonical handle rather than ad-hoc runtime construction.  
3. `binary_emitter` copies the IR tensor pool into the `Program::tensor_pool`, which the VM loads lazily when a `TensorHandle` literal appears (`LoadImm` with `LiteralKind::TensorHandle`).

The result is deterministic, canonical tensors that survive serialization/deserialization, match the spec’s tensor literal policy, and keep the `t81` CLI/VM aligned with the semantic analyzer.

## Testing & Coverage

- `tests/cpp/semantic_analyzer_vector_literal_test.cpp` verifies inference, context requirements, and rejection of untyped empty vectors.  
- `tests/cpp/cli_option_result_test.cpp` exercises the full CLI path (including vector-literal-heavy match programs) so the canonical tensors reach `t81::cli::compile` and `t81::cli::run_tisc`.  
- Existing VM tests cover tensor handles and `ChkShape` sequences that now rely on the canonical literal metadata.

Use this guide when extending vector/tensor syntax or when updating downstream consumers that expect canonical, deterministic `T729Tensor` literals.
