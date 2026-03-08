# RFC-0026 Residual Strict-Core Exceptions

Last Updated: 2026-03-08
Owner: @t81dev
Purpose: record the remaining non-strict or host-float surfaces after the
phase-1 AI opcode classification cleanup, and separate true RFC-0026 closure
items from broader deterministic-math work.

## Summary

Phase-1 cleanup removed the accidental numeric-class downgrades on deterministic
fixed paths for `QMATMUL`, `ATTN`, `softmax`, `rmsnorm`, `silu`, `rope`, `exp`,
`sqrt`, and `log`.

Follow-on `RFC-0030` work also replaced several non-fixed fallback math paths
with deterministic `T81Float`/`dmath` implementations. In practice, `ATTN`,
`softmax`, `rmsnorm`, `silu`, `rope`, and tensor unary `exp`/`sqrt`/`log` no
longer depend on raw host `<cmath>` in deterministic builds.

What remains is no longer "cleanup the labels." The remaining items are either:

- legitimate host-float fallback paths that the current top-level specs still
  allow outside strict mode
- generic tensor/runtime behavior outside the AI opcode closure surface
- real deterministic-math replacement work that belongs under `RFC-0030`

## Category A: Acceptable For Now Under Current Spec

These paths still use host-float or `<cmath>` behavior in fallback lanes, but
that is consistent with the current top-level specs and strict-profile split.

| Surface | Current State | Why It Is Acceptable For Now | Key Paths |
| :--- | :--- | :--- | :--- |
| `QMATMUL` fallback lane | Fixed-point path exists for strict-core eligible inputs; non-fixed fallback now uses deterministic dequantization but still produces `HostFloat` tensors outside strict-core promotion rules | Acceptable outside strict mode; strict-core case is already covered | `include/t81/tensor/matmul.hpp`, `core/vm/vm.cpp:5096` |

## Category B: Real RFC-0026 Closure Candidates

These are still tied closely enough to the AI opcode contract that they should
be reviewed before calling RFC-0026 operationally closed.

| Surface | Current State | Why It Still Matters For RFC-0026 | Key Paths |
| :--- | :--- | :--- | :--- |
| Residual AI fallback inventory | The AI opcode family has both strict-core and host-float lanes, but there is no single status document describing where the boundary currently sits | Without that inventory, repo behavior can drift ahead of the written promotion/determinism story | This document plus `include/t81/tensor/llama.hpp`, `include/t81/tensor/matmul.hpp`, `core/vm/vm.cpp` |
| `WLOAD` promotion boundary | `WLOAD` is functionally present and audited, but phase-1 behavior is still handle-to-handle materialization rather than true policy-gated CanonFS-backed loading | This is the main remaining gap between current implementation and the fuller promotion target described in follow-on AI execution docs | `include/t81/tensor/llama.hpp:447`, `core/vm/vm.cpp:5180`, `spec/tisc-spec.md` |

## Category C: RFC-0030 Deterministic Math Dependencies

These are not simple RFC-0026 cleanup items. They require replacing genuine
host-float math with deterministic soft-math or fixed-point implementations.

| Surface | Current State | Why It Belongs Under RFC-0030 | Key Paths |
| :--- | :--- | :--- | :--- |
| Non-fixed tensor matmul | Deterministic builds now avoid the AVX/FMA float lane, but the non-fixed result still remains in the float domain rather than canonical fixed-point storage | This is now about broader float-domain parity/promotion, not small arithmetic leaks | `include/t81/tensor/matmul.hpp` |

## Category D: Outside RFC-0026 Scope

These surfaces are real host-float behavior, but they should not drive AI opcode
closure because they are generic tensor/runtime concerns.

| Surface | Current State | Why It Is Out Of Scope For RFC-0026 | Key Paths |
| :--- | :--- | :--- | :--- |
| Elementwise tensor division | Now uses deterministic arithmetic but still returns `HostFloat` because generic division is not a strict-core exact-int contract | Generic tensor arithmetic; not an AI opcode contract item | `include/t81/tensor/elementwise.hpp` |
| Tensor mutation with float writes | Writing float or non-integral values correctly degrades the tensor to `HostFloat` | This is correct behavior for mixed-domain mutation, not a determinism leak | `include/t81/tensor/mutation.hpp:37`, `include/t81/tensor/mutation.hpp:46`, `include/t81/tensor/mutation.hpp:53` |

## Recommended Next Step

Do not continue broad "strict-core cleanup" by default.

The next implementation work should be one of:

1. `RFC-0026` closure work:
   maintain this exception inventory and decide whether `WLOAD` needs any
   additional hardening to match the claimed phase-1 boundary.

2. `RFC-0030` work:
   continue with the remaining float-domain tensor/runtime policy questions,
   starting with non-fixed matmul promotion/parity rules rather than more
   isolated kernel arithmetic cleanup.

Until one of those starts, the current repo state is coherent:

- strict-core fixed paths are classified correctly
- deterministic builds avoid the main raw `<cmath>` and AVX/FMA tensor math lanes
- host-float classification boundaries remain explicit where promotion rules still require them
- the full `ctest` suite is green
