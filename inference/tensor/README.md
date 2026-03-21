# `src/tensor`

Implementation of tensor primitives used by runtime and ML-adjacent paths.

## Scope
- Tensor storage/manipulation internals
- Core tensor ops backing public APIs

## Related Interfaces
- `include/t81/tensor.hpp`
- `include/t81/tensor/ops.hpp`

## Notes
- Keep numeric behavior deterministic across supported build matrix.
- Add/extend tests when changing tensor math or shape semantics.
