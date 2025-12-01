#pragma once

#include "t81/simd/add_helpers.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <type_traits>

#if defined(__x86_64__) && defined(__AVX2__)
#include <immintrin.h>
#endif

namespace t81::simd {

struct ByteCarryMap {
  std::array<int8_t, 3> carry_out{};
};

inline int IndexForCarry(int8_t carry) {
  return static_cast<int>(carry + 1);
}

inline ByteCarryMap Compose(const ByteCarryMap& left, const ByteCarryMap& right) {
  ByteCarryMap result;
  for (int idx = 0; idx < 3; ++idx) {
    const int left_idx = IndexForCarry(left.carry_out[idx]);
    result.carry_out[idx] = right.carry_out[left_idx];
  }
  return result;
}

inline ByteCarryMap MakeByteCarryMap(uint8_t lhs_byte, uint8_t rhs_byte) {
  ByteCarryMap map;
  for (int idx = 0; idx < 3; ++idx) {
    int8_t carry = static_cast<int8_t>(idx - 1);
    for (int trit = 0; trit < 4; ++trit) {
      const int shift = trit * 2;
      const auto decode = [](uint8_t pattern) -> int8_t {
        if (pattern == 0u) return -1;
        if (pattern == 1u) return 0;
        return +1;
      };
      const int8_t a = decode((lhs_byte >> shift) & 0x3u);
      const int8_t b = decode((rhs_byte >> shift) & 0x3u);
      const AddEntry& entry = LookupAddEntry(a, b);
      carry = entry.carry[IndexForCarry(carry)];
    }
    map.carry_out[idx] = carry;
  }
  return map;
}

#if defined(__x86_64__) && defined(__AVX2__)

inline void BuildCarryMaps(__m256i lhs, __m256i rhs,
                          std::array<ByteCarryMap, 32>& maps) {
  alignas(32) uint8_t lhs_bytes[32];
  alignas(32) uint8_t rhs_bytes[32];
  _mm256_store_si256(reinterpret_cast<__m256i*>(lhs_bytes), lhs);
  _mm256_store_si256(reinterpret_cast<__m256i*>(rhs_bytes), rhs);
  for (int i = 0; i < 32; ++i) {
    maps[i] = MakeByteCarryMap(lhs_bytes[i], rhs_bytes[i]);
  }
}

inline void PrefixScan(std::array<ByteCarryMap, 32>& maps) {
  for (int stride = 1; stride < 32; stride <<= 1) {
    for (int i = stride; i < 32; ++i) {
      maps[i] = Compose(maps[i - stride], maps[i]);
    }
  }
}

inline std::array<int8_t, 32> CarryIns(const std::array<ByteCarryMap, 32>& maps) {
  std::array<int8_t, 32> carries{};
  int8_t carry = 0;
  for (int i = 0; i < 32; ++i) {
    carries[i] = carry;
    carry = maps[i].carry_out[IndexForCarry(carry)];
  }
  return carries;
}

#else

inline void BuildCarryMaps(...) {}
inline void PrefixScan(...) {}
inline std::array<int8_t, 32> CarryIns(...) { return {}; }

#endif

}  // namespace t81::simd
