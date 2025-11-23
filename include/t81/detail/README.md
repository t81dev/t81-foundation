# t81/detail — Internal Utilities

Small, header-only helpers used across the library. These are not part of the
stable public API; treat them as implementation details.

## Files

- `assert.hpp`
  - Lightweight assertions controlled by `T81_ENABLE_ASSERTS` (see `config.hpp`).
  - Macros: `T81_ASSERT(cond)`, `T81_ASSERT_MSG(cond, msg)`.

- `bitops.hpp`
  - Tiny bit utilities:
    - `is_power_of_two(uint64_t)`
    - `round_up_pow2(uint64_t)`
    - `clz32(uint32_t)`, `clz64(uint64_t)`
  - Uses portable fallbacks when compiler builtins are unavailable.

## Notes
- Keep dependencies minimal; these headers must remain freestanding.
- Do not include third-party libraries here.
