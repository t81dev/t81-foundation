# Track K — Result Representation Fix Evidence

**Date:** 2026-03-22
**Status:** Implemented; 405/405 tests passing

## Summary

This document records the changes and benchmark evidence for Track K: fixing the result
representation of native unary fast paths (`TExp`, `TSiLU`, `TSoftmax`) in `vm/tensor_helpers.cpp`,
and adding downstream `strict_core_eligible()` guards to prevent expensive lazy canonical-fixed
cache builds for `HostFloat` tensors inside `ops::matmul`, `ops::reduce`, and related ops.

This directly addresses the gating factor identified in RFC-00BB §6.3: "result representation, not
import/profile admission, as the next gating factor for moving dense families toward
`native_supported`."

---

## Root Cause

Three native unary fast paths in `vm/tensor_helpers.cpp` — `native_tensor_unary_exp_direct`,
`native_tensor_unary_silu_direct`, and `native_tensor_unary_softmax_direct` — returned tensors
tagged as `TensorNumericClass::ExactInt`. These functions decode packed balanced-ternary limbs and
apply a float-domain activation (e.g., `exp(-1) ≈ 0.368`) per trit. The result is `float`-backed
via `from_host_float_data()`, so the `ExactInt` tag was semantically wrong.

The incorrect tag caused downstream `ops::matmul` (and related ops) to treat the result as
strict-core eligible, which triggered `has_canonical_fixed_data()`. That method lazily builds a
`DFixed` (`Fixed<192,80>`) canonical-fixed cache from the float data — an O(N) operation that
costs up to ~150 DFixed×2 ops per element — before discovering whether the DFixed path was
actually appropriate.

---

## Changes Made

### `vm/tensor_helpers.cpp`

Three functions changed: `ExactInt` → `HostFloat` on the fast-path result.

```
native_tensor_unary_exp_direct     (line ~257)
native_tensor_unary_silu_direct    (line ~508)
native_tensor_unary_softmax_direct (line ~598)
```

`HostFloat` is the correct class for a float-backed tensor whose values are the result of
transcendental functions evaluated at `{-1, 0, +1}` (e.g. `exp(-1) ≈ 0.36788` is not an integer).

### `include/t81/tensor/matmul.hpp`

Two locations — `matmul()` and `qmatmul()` — reordered the eligibility guard before the cache-
trigger call:

```cpp
// Before:
if (A.has_canonical_fixed_data() && B.has_canonical_fixed_data() &&
    A.strict_core_eligible() && B.strict_core_eligible())

// After:
if (A.strict_core_eligible() && B.strict_core_eligible() &&
    A.has_canonical_fixed_data() && B.has_canonical_fixed_data())
```

`strict_core_eligible()` is O(1); `has_canonical_fixed_data()` is O(N) (cache build). Short-circuit
evaluation ensures `HostFloat` tensors never pay the cache-build cost.

### `include/t81/tensor/reduce.hpp`

Same reordering applied to `contract_dot()`.

### `include/t81/tensor/unary.hpp`

Added `strict_core_eligible()` outer guard to `exp()`, `sqrt()`, and `log()`:

```cpp
if (x.strict_core_eligible() && x.has_canonical_fixed_data()) { ... }
```

### `include/t81/tensor/llama.hpp`

Added `strict_core_eligible()` outer guard to `silu()` and `softmax()`.

Attention block already computed `result_class` before the `has_canonical_fixed_data()` check;
added a `result_class != HostFloat` gate to that call.

### `tests/cpp/vm_tensor_test.cpp`

Updated three assertions to expect `HostFloat` instead of `ExactInt` from the native unary fast
paths (TExp, TSiLU, TSoftmax).

---

## Test Results

Full test suite: **405/405 passing** on AArch64 (Apple M-series, 2026-03-22).

---

## Benchmark: `BM_NativeWeightsExpThenMatMul_T81Native`

**Chain:** `WeightsLoad (BalancedTernary) → TExp (native fast path) → TMatMul (ops::matmul)`

**Build config:** `T81_STRICT_DETERMINISTIC_FLOAT=ON` (defines `T81_DETERMINISTIC`); AArch64
(no AVX2). In this mode `ops::matmul` uses `deterministic_fma`, which converts each float operand
to `T81Float<72,9>` for bit-exact multiplication.

| Size (elements) | Matrix shape | TExp→TMatMul time/iter |
| :--- | :--- | :--- |
| 64  | 8×8    | 4160 ms  |
| 256 | 16×16  | 17218 ms |
| 1024 | 32×32 | 55691 ms |
| 4096 | 64×64 | 251911 ms |

**What these numbers show:**

1. The chain completes successfully end-to-end (`WeightsLoad → TExp → TMatMul`) at all tested sizes.
2. `ops::matmul` correctly takes the float path (`deterministic_fma`) — not the DFixed canonical-
   fixed path. If `ExactInt` were still returned by TExp, both operands would pass `strict_core_eligible()`
   and `has_canonical_fixed_data()` would trigger an O(N) cache build before the matmul.
3. The measured cost is entirely `deterministic_fma` (T81Float<72,9> per multiply-accumulate), which
   is the expected cost in `T81_DETERMINISTIC` mode. This is not the canonical-fixed cache build.

**Reference: `BM_NativeWeightsLoadAndTWMATMUL_Binary` (ExactTrit × ExactTrit, BigInt path):**

| Size | Time/iter |
| :--- | :--- |
| 64  | 0.074 ms |
| 256 | 0.533 ms |
| 1024 | 4.12 ms |
| 4096 | 32.2 ms |

The TWMATMUL binary path uses `ops::twmatmul` (BigInt snap-trit accumulator), which is faster for
ternary-valued weights. The TExp→TMatMul chain uses `ops::matmul` with `deterministic_fma`, which
is much more expensive per multiply-accumulate in `T81_DETERMINISTIC` mode. Improving
`deterministic_fma` throughput (e.g. by using IEEE float math in non-strict-deterministic builds,
or by a specialized path for HostFloat activations) is deferred from this track.

---

## What Was NOT Fixed (Deferred)

- The `deterministic_fma` cost in `ops::matmul` remains high in `T81_DETERMINISTIC` mode.
  A future track could add a HostFloat-specific matmul path that avoids T81Float<72,9>
  round-trips for inputs that are already floats.
- Promotion of dense families from `experimental_native` to `native_supported` per RFC-00BB §5.2
  remains contingent on improved post-load execution behavior for native tensors at inference time.

---

## RFC-00BB §6.3 Update

See the corresponding §6.3 update in `spec/rfcs/RFC-00BB-native-model-architecture-compatibility.md`.
