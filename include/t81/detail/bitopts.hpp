#pragma once
#include <cstdint>
#include "t81/config.hpp"

namespace t81::detail {

// Portable popcount for 32/64-bit
T81_NODISCARD inline uint32_t popcount32(uint32_t x) {
#if defined(__GNUG__) || defined(__clang__)
  return static_cast<uint32_t>(__builtin_popcount(x));
#elif defined(_MSC_VER)
  return static_cast<uint32_t>(__popcnt(x));
#else
  // Hacker's Delight fallback
  x = x - ((x >> 1) & 0x55555555u);
  x = (x & 0x33333333u) + ((x >> 2) & 0x33333333u);
  return (((x + (x >> 4)) & 0x0F0F0F0Fu) * 0x01010101u) >> 24;
#endif
}

T81_NODISCARD inline uint32_t popcount64(uint64_t x) {
#if defined(__GNUG__) || defined(__clang__)
  return static_cast<uint32_t>(__builtin_popcountll(x));
#elif defined(_MSC_VER) && (defined(_M_X64) || defined(_M_ARM64))
  return static_cast<uint32_t>(__popcnt64(x));
#else
  // Split into 32-bit halves
  return popcount32(static_cast<uint32_t>(x)) +
         popcount32(static_cast<uint32_t>(x >> 32));
#endif
}

// Count leading zeros for 32/64-bit (returns bit-width if x==0)
T81_NODISCARD inline uint32_t clz32(uint32_t x) {
#if defined(__GNUG__) || defined(__clang__)
  return x ? static_cast<uint32_t>(__builtin_clz(x)) : 32u;
#elif defined(_MSC_VER)
  unsigned long idx;
  if (_BitScanReverse(&idx, x)) return 31u - idx;
  return 32u;
#else
  if (!x) return 32u;
  uint32_t n = 0;
  for (int i = 31; i >= 0; --i) { if (x & (1u << i)) break; ++n; }
  return n;
#endif
}

T81_NODISCARD inline uint32_t clz64(uint64_t x) {
#if defined(__GNUG__) || defined(__clang__)
  return x ? static_cast<uint32_t>(__builtin_clzll(x)) : 64u;
#elif defined(_MSC_VER)
  unsigned long idx;
#if defined(_M_X64) || defined(_M_ARM64)
  if (_BitScanReverse64(&idx, x)) return 63u - idx;
  return 64u;
#else
  // 32-bit MSVC: emulate using two scans
  uint32_t hi = static_cast<uint32_t>(x >> 32);
  if (hi) { _BitScanReverse(&idx, hi); return 31u - idx; }
  uint32_t lo = static_cast<uint32_t>(x);
  if (_BitScanReverse(&idx, lo)) return 63u - (32u + idx);
  return 64u;
#endif
#else
  if (!x) return 64u;
  uint32_t n = 0;
  for (int i = 63; i >= 0; --i) { if (x & (1ull << i)) break; ++n; }
  return n;
#endif
}

// Next power of two (returns 1 for x==0). 64-bit.
T81_NODISCARD inline uint64_t next_pow2_u64(uint64_t x) {
  if (x <= 1) return 1ull;
  x--;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x |= x >> 32;
  return x + 1;
}

// Check power-of-two
T81_NODISCARD inline bool is_pow2_u64(uint64_t x) {
  return x && ((x & (x - 1)) == 0);
}

} // namespace t81::detail
