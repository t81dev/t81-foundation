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

inline int IndexForCarry(int8_t carry) { return static_cast<int>(carry + 1); }

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

  auto decode = [](uint8_t pattern) -> int8_t {
    if (pattern == 0u) return -1;
    if (pattern == 1u) return 0;
    return +1;
  };

  const int8_t a0 = decode(lhs_byte & 0x3u);
  const int8_t b0 = decode(rhs_byte & 0x3u);
  const AddEntry& e0 = LookupAddEntry(a0, b0);

  const int8_t a1 = decode((lhs_byte >> 2) & 0x3u);
  const int8_t b1 = decode((rhs_byte >> 2) & 0x3u);
  const AddEntry& e1 = LookupAddEntry(a1, b1);

  const int8_t a2 = decode((lhs_byte >> 4) & 0x3u);
  const int8_t b2 = decode((rhs_byte >> 4) & 0x3u);
  const AddEntry& e2 = LookupAddEntry(a2, b2);

  const int8_t a3 = decode((lhs_byte >> 6) & 0x3u);
  const int8_t b3 = decode((rhs_byte >> 6) & 0x3u);
  const AddEntry& e3 = LookupAddEntry(a3, b3);

  for (int idx = 0; idx < 3; ++idx) {
    int8_t c = static_cast<int8_t>(idx - 1);
    c = e0.carry[c + 1];
    c = e1.carry[c + 1];
    c = e2.carry[c + 1];
    c = e3.carry[c + 1];
    map.carry_out[idx] = c;
  }
  return map;
}

// Portable helpers — not guarded by any ISA macro.

inline void BuildCarryMapsFromBytes(const uint8_t* lhs_bytes, const uint8_t* rhs_bytes,
                                    std::array<ByteCarryMap, 32>& maps) {
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

// ISA-specific BuildCarryMaps overloads that extract bytes then call the portable helper.

#if defined(__x86_64__) && defined(__AVX2__)

inline void BuildCarryMaps(__m256i lhs, __m256i rhs, std::array<ByteCarryMap, 32>& maps) {
  alignas(32) uint8_t lhs_bytes[32];
  alignas(32) uint8_t rhs_bytes[32];
  _mm256_store_si256(reinterpret_cast<__m256i*>(lhs_bytes), lhs);
  _mm256_store_si256(reinterpret_cast<__m256i*>(rhs_bytes), rhs);
  BuildCarryMapsFromBytes(lhs_bytes, rhs_bytes, maps);
}

#elif defined(__ARM_NEON)
#include <arm_neon.h>

inline void BuildCarryMaps(const uint8x16x2_t& lhs, const uint8x16x2_t& rhs,
                           std::array<ByteCarryMap, 32>& maps) {
  alignas(16) uint8_t lhs_bytes[32];
  alignas(16) uint8_t rhs_bytes[32];
  vst1q_u8(lhs_bytes,      lhs.val[0]);
  vst1q_u8(lhs_bytes + 16, lhs.val[1]);
  vst1q_u8(rhs_bytes,      rhs.val[0]);
  vst1q_u8(rhs_bytes + 16, rhs.val[1]);
  BuildCarryMapsFromBytes(lhs_bytes, rhs_bytes, maps);
}

#endif

}  // namespace t81::simd
