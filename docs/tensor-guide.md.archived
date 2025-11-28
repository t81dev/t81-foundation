______________________________________________________________________

# T81 Tensor Guide

<!-- T81-TOC:BEGIN -->

## Table of Contents

- [T81 Tensor Guide](#t81-tensor-guide)
  - [1. Canonical Shapes](#1-canonical-shapes)
  - [2. Tensor Pools & Handles](#2-tensor-pools-&-handles)
  - [3. Common Operations](#3-common-operations)
    - [3.1 Broadcasting](#31-broadcasting)
    - [3.2 Shape Guards](#32-shape-guards)
    - [3.3 Deterministic Reductions](#33-deterministic-reductions)
  - [4. Axion Visibility](#4-axion-visibility)
  - [5. Testing Checklist](#5-testing-checklist)
  - [6. Open Work](#6-open-work)

<!-- T81-TOC:END -->

> Companion to `spec/t81-data-types.md` and [RFC-0004](../spec/rfcs/RFC-0004-canonical-tensor-semantics.md)\
> Non-normative tips for working with canonical tensors across the T81 stack.

______________________________________________________________________

## 1. Canonical Shapes

- Shapes are explicit tuples. Always record them even for scalars:\
  `scalar = Tensor(shape=[], data=[...])`
- No implicit broadcasting in the spec. When you *intend* to broadcast, call
  the helper explicitly (see §3).
- Zero-length dimensions collapse the tensor to canonical zero; don’t rely on
  host library semantics.

______________________________________________________________________

## 2. Tensor Pools & Handles

TISC and the VM treat tensors like floats/fractions:

1. Compiler emits literal tensors into the tensor pool.
2. Instructions operate on **handles** (1-based indices).
3. VM deduplicates canonical tensors, so equality can use handle equality.

Tips:

- Never store raw pointers to tensor storage; always use pool handles.
- When debugging, inspect `vm->state().tensors[handle-1]` (tests show pattern).

______________________________________________________________________

## 3. Common Operations

### 3.1 Broadcasting

```t81
use std::tensor as tensor;

fn add_bias(x: Tensor[81, 27], bias: Tensor[27]) -> Tensor[81,27] {
    return tensor::broadcast_add(x, bias);
}
```

The helper checks shape rules from RFC-0004 and faults deterministically if
they’re violated.

### 3.2 Shape Guards

```t81
fn require_shape(x: Tensor[*,*], rows: T81Int, cols: T81Int) -> Tensor[*,*] {
    if (!tensor::has_shape(x, rows, cols)) {
        panic(:bad_shape);
    }
    return x;
}
```

Lowering emits the future `CHKSHAPE` opcode (RFC-0005) when available, or a
pure T81Lang check otherwise.

### 3.3 Deterministic Reductions

- Always specify axes.
- Reduction order is canonical (lexicographic), so the result is invariant.

```t81
fn sum_rows(x: Tensor[81,27]) -> Tensor[81] {
    return tensor::reduce_sum(x, axis=1);
}
```

______________________________________________________________________

## 4. Axion Visibility

Every tensor opcode logs:

- Input/output handles
- Shapes
- Fault status

If a policy denies tensors above a certain rank, Axion faults before VM memory
is touched. When writing docs/policies reference those fields explicitly.

______________________________________________________________________

## 5. Testing Checklist

- Use `tests/cpp/tensor_*_test.cpp` as templates.
- When adding new tensor ops:
  - Add spec examples.
  - Mirror them in JSON canonical vectors (future work).
  - Verify trace determinism (watch `vm->state().trace`).

______________________________________________________________________

## 6. Open Work

1. Document tensor serialization once canonical codec lands.
2. Publish a gallery of tensor policies (Axion) for common workloads.
3. Add examples using the upcoming vector helpers (`VLOAD`, `VADD`).

Contributions welcome—extend this guide as RFC-0004 evolves.

______________________________________________________________________
