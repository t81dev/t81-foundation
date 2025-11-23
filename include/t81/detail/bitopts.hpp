#pragma once
#include <cstdint>
#include <limits>
#include "t81/config.hpp"

// Tiny bit-utilities used occasionally across headers.
// Header-only; no platform intrinsics assumed.

namespace t81::detail {

T81_FORCE_INLINE constexpr bool is_power_of_two(uint64_t x) {
  return x && ((x & (x - 1)) == 0);
}

T81_FORCE_INLINE constexpr uint64_t round_up_pow2(uint64_t x) {
  if (x <= 1) return 1;
  --x;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x |= x >> 32;
  return x + 1;
}

T81_FORCE_INLINE constexpr uint32_t clz32(uint32_t x) {
#if defined(__GNUC__) || defined(__clang__)
  return x ? __builtin_clz(x) : 32u;
#else
  // Portable fallback
  uint32_t n = 32;
  while (x) { --n; x >>= 1; }
  return n;
#endif
}

T81_FORCE_INLINE constexpr uint64_t clz64(uint64_t x) {
#if defined(__GNUC__) || defined(__clang__)
  return x ? __builtin_clzll(x) : 64u;
#else
  uint64_t n = 64;
  while (x) { --n; x >>= 1; }
  return n;
#endif
}

} // namespace t81::detail
