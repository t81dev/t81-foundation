# `src/simd`

SIMD-accelerated numeric kernels used by ternary/base-81 pathways.

## Scope
- AVX2 implementations for base-81 digit operations.
- Fast-path arithmetic helpers used by higher-level numeric code.

## Key Files
- `base81_digits_avx2.cpp`: vectorized add/sub/negate/clamp/mul-constant helpers plus normalization utilities.

## Behavioral Notes
- AVX2 paths are guarded with `__AVX2__`; scalar fallbacks remain active for unsupported targets.
- Digit-domain assumptions are explicit in the code (`0..80` base-81 digit range).
- Determinism must match scalar behavior bit-for-bit for equivalent inputs.

## Testing Guidance
- Add regression tests for any new intrinsic path.
- Validate parity between AVX2 and scalar outputs.
