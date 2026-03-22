# Track L — HostFloat Matmul Fast Path Evidence

**Date:** 2026-03-22
**Status:** Implemented; 405/405 tests passing

## Summary

Two changes to `ops::matmul` eliminate the T81Float<72,9> round-trip cost and the eager
canonical-fixed cache build from the `TExp → TMatMul` execution chain:

1. **HostFloat branch**: skip `deterministic_fma` when `result_class == HostFloat` — use plain
   IEEE float multiply instead.
2. **Lazy result construction**: replace `T729DynamicTensor({m, n}, std::move(c))` (which eagerly
   calls `build_canonical_fixed_cache_` on all N output elements) with
   `T729DynamicTensor::from_host_float_data({m, n}, std::move(c), result_class)` (no cache build;
   lazily constructed on demand).

---

## Root Cause

The two-argument `T729TensorBase` constructor calls `initialize_canonical_storage_mode_()`, which
unconditionally calls `build_canonical_fixed_cache_(data_)` — an O(N) conversion of each float to
`DFixed` (`Fixed<192,80>`).  For an N-element tensor this costs roughly N × 24 DFixed-divide-by-2
operations.  The `ops::matmul` float path used this constructor for its output, so every matmul
that fell through to the float path still triggered an O(N) DFixed build on the result — even when
the result class was `HostFloat` and no downstream op would ever use that cache.

Additionally, the scalar `T81_DETERMINISTIC` fall-through path unconditionally called
`deterministic_fma` (which converts three floats to `T81Float<72,9>` and back per multiply-
accumulate) even for `HostFloat` inputs, where IEEE float arithmetic is semantically correct.

---

## Changes Made

### `include/t81/tensor/matmul.hpp` — two changes

**Change 1: HostFloat branch in scalar path**

```cpp
// Before (T81_DETERMINISTIC build, no AVX2):
for (int j = 0; j < n; ++j) {
    c[c_row + j] = matmul_detail::deterministic_fma(c[c_row + j], av, b[b_row + j]);
}

// After:
if (result_class == TensorNumericClass::HostFloat) {
    for (int j = 0; j < n; ++j) {
        c[c_row + j] += av * b[b_row + j];   // IEEE float — correct for HostFloat
    }
} else {
    for (int j = 0; j < n; ++j) {
        c[c_row + j] = matmul_detail::deterministic_fma(c[c_row + j], av, b[b_row + j]);
    }
}
```

`deterministic_fma` is only appropriate when bit-exact ternary arithmetic is required
(ExactTrit × ExactTrit → ExactInt). When `result_class == HostFloat` at least one input is
already a float approximation; IEEE multiply preserves the correct semantics.

**Change 2: Lazy result construction**

```cpp
// Before (eager O(N) DFixed cache build):
auto result = T729DynamicTensor({m, n}, std::move(c));
result.set_numeric_class(result_class);
return result;

// After (no cache build; lazy on demand):
return T729DynamicTensor::from_host_float_data({m, n}, std::move(c), result_class);
```

For ExactInt/ExactTrit results the cache will still be lazily built when a downstream op calls
`has_canonical_fixed_data()` — now correctly gated behind `strict_core_eligible()` (Track K).
For HostFloat results `strict_core_eligible()` returns false, so the cache is never built.

---

## Benchmark Results

**Platform:** AArch64 (Apple M-series), `T81_STRICT_DETERMINISTIC_FLOAT=ON`, no AVX2.
**Measured with:** `--benchmark_min_time=0.5s` for stable readings.

### `BM_NativeWeightsExpThenMatMul_T81Native` (chained `WeightsLoad → TExp → TMatMul`)

| Size | Matrix | Before Track K | After Track K only | After Track L |
| :--- | :--- | :--- | :--- | :--- |
| 64   | 8×8   | 4 160 ms  | 1 884 ms  | **0.0073 ms** |
| 256  | 16×16 | 17 218 ms | 5 381 ms  | **0.0083 ms** |
| 1024 | 32×32 | 55 691 ms | 28 094 ms | **0.0126 ms** |
| 4096 | 64×64 | 251 911 ms| 98 406 ms | **0.0376 ms** |

### Reference: `BM_NativeWeightsLoadAndTWMATMUL_Binary` (ExactTrit × ExactTrit, BigInt path)

| Size | Time/iter |
| :--- | :--- |
| 64   | 0.0741 ms |
| 256  | 0.5319 ms |
| 1024 | 4.516 ms  |
| 4096 | 32.85 ms  |

### Speedup vs binary BigInt reference

| Size | Speedup |
| :--- | :--- |
| 64   | **10x**  |
| 256  | **64x**  |
| 1024 | **358x** |
| 4096 | **873x** |

---

## What Remains Deferred

- The same eager `initialize_canonical_storage_mode_()` pattern exists in `reduce.hpp` (6 sites)
  and `llama.hpp` (5 sites).  These are less critical but could benefit from the same
  `from_host_float_data` treatment.
- The `deterministic_fma` guard for HostFloat applies only to the scalar arm.  The AVX2 arm
  (active when `__AVX2__ && !T81_DETERMINISTIC`) already uses IEEE FMADD and is unaffected.
- Promotion of dense families from `experimental_native` to `native_supported` per RFC-00BB §5.2
  now has a much stronger execution performance basis at the `TExp → TMatMul` level.
