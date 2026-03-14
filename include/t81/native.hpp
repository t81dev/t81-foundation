#pragma once

#include <array>
#include <cstdint>

#if defined(__x86_64__) && defined(__AVX2__)
#include <immintrin.h>
#endif

#include "t81/simd/prefix_scan.hpp"

namespace t81 {

struct alignas(32) T81 {
  alignas(32) std::array<uint8_t, 32> data{};

  constexpr T81() noexcept = default;

  explicit constexpr T81(const std::array<uint8_t, 32>& block) noexcept : data(block) {}

#if defined(__x86_64__) && defined(__AVX2__)
  explicit T81(__m256i v) noexcept {
    _mm256_store_si256(reinterpret_cast<__m256i*>(data.data()), v);
  }

  __m256i avx() const noexcept {
    return _mm256_load_si256(reinterpret_cast<const __m256i*>(data.data()));
  }
#endif

  static constexpr uint8_t EncodeTrit(int8_t value) noexcept {
    if (value <= -1) return 0;
    if (value == 0) return 1;
    return 2;
  }

  static constexpr int8_t DecodeTrit(uint8_t bits) noexcept {
    if (bits == 0) return -1;
    if (bits == 1) return 0;
    return +1;
  }

  static void UnpackDigits(const std::array<uint8_t, 32>& src,
                           std::array<int8_t, 128>& digits) noexcept {
    for (int byte_index = 0; byte_index < 32; ++byte_index) {
      uint8_t byte = src[byte_index];
      for (int trit = 0; trit < 4; ++trit) {
        digits[byte_index * 4 + trit] = DecodeTrit((byte >> (trit * 2)) & 0x3u);
      }
    }
  }

  static void PackDigits(const std::array<int8_t, 128>& digits,
                         std::array<uint8_t, 32>& dest) noexcept {
    for (int byte_index = 0; byte_index < 32; ++byte_index) {
      uint8_t byte = 0;
      for (int trit = 0; trit < 4; ++trit) {
        byte |= EncodeTrit(digits[byte_index * 4 + trit]) << (trit * 2);
      }
      dest[byte_index] = byte;
    }
  }

  static constexpr int8_t NormalizeBalanced(int value) noexcept {
    if (value > 1) {
      return static_cast<int8_t>(value - 3);
    }
    if (value < -1) {
      return static_cast<int8_t>(value + 3);
    }
    return static_cast<int8_t>(value);
  }

  static uint8_t AddByte(uint8_t lhs, uint8_t rhs, int8_t carry_in) noexcept {
    uint8_t result = 0;
    int8_t carry = carry_in;

    // Trit 0
    {
      const int8_t a = DecodeTrit(lhs & 0x3u);
      const int8_t b = DecodeTrit(rhs & 0x3u);
      const simd::AddEntry& entry = simd::LookupAddEntry(a, b);
      result |= EncodeTrit(entry.sum[carry + 1]);
      carry = entry.carry[carry + 1];
    }
    // Trit 1
    {
      const int8_t a = DecodeTrit((lhs >> 2) & 0x3u);
      const int8_t b = DecodeTrit((rhs >> 2) & 0x3u);
      const simd::AddEntry& entry = simd::LookupAddEntry(a, b);
      result |= EncodeTrit(entry.sum[carry + 1]) << 2;
      carry = entry.carry[carry + 1];
    }
    // Trit 2
    {
      const int8_t a = DecodeTrit((lhs >> 4) & 0x3u);
      const int8_t b = DecodeTrit((rhs >> 4) & 0x3u);
      const simd::AddEntry& entry = simd::LookupAddEntry(a, b);
      result |= EncodeTrit(entry.sum[carry + 1]) << 4;
      carry = entry.carry[carry + 1];
    }
    // Trit 3
    {
      const int8_t a = DecodeTrit((lhs >> 6) & 0x3u);
      const int8_t b = DecodeTrit((rhs >> 6) & 0x3u);
      const simd::AddEntry& entry = simd::LookupAddEntry(a, b);
      result |= EncodeTrit(entry.sum[carry + 1]) << 6;
    }
    return result;
  }

  T81 operator-() const noexcept {
#if defined(__x86_64__) && defined(__AVX2__)
    // Each trit is 2 bits: 00 -> -1 (M), 01 -> 0 (Z), 10 -> +1 (P)
    // Negation:
    // M (-1, 00) -> P (+1, 10)
    // Z ( 0, 01) -> Z ( 0, 01)
    // P (+1, 10) -> M (-1, 00)
    // Observation: 2 - bits gives the result:
    // 2 - 0 = 2 (10)
    // 2 - 1 = 1 (01)
    // 2 - 2 = 0 (00)
    // 0x2 is 10 in binary. For 4 trits in a byte, we use 0b10101010 = 0xAA.
    const __m256i v = _mm256_load_si256(reinterpret_cast<const __m256i*>(data.data()));
    const __m256i two_four_times = _mm256_set1_epi8(static_cast<int8_t>(0xAA));
    const __m256i negated = _mm256_sub_epi8(two_four_times, v);
    T81 result;
    _mm256_store_si256(reinterpret_cast<__m256i*>(result.data.data()), negated);
    return result;
#else
    std::array<int8_t, 128> digits{};
    UnpackDigits(data, digits);
    for (int idx = 0; idx < 128; ++idx) {
      digits[idx] = static_cast<int8_t>(-digits[idx]);
    }
    T81 result;
    PackDigits(digits, result.data);
    return result;
#endif
  }

  T81 operator+(const T81& other) const noexcept {
    T81 result;
#if defined(__x86_64__) && defined(__AVX2__)
    const __m256i lhs_v = _mm256_load_si256(reinterpret_cast<const __m256i*>(data.data()));
    const __m256i rhs_v = _mm256_load_si256(reinterpret_cast<const __m256i*>(other.data.data()));

    std::array<simd::ByteCarryMap, 32> maps{};
    simd::BuildCarryMaps(lhs_v, rhs_v, maps);
    simd::PrefixScan(maps);
    auto carries = simd::CarryIns(maps);

    const uint8_t* lhs_ptr = data.data();
    const uint8_t* rhs_ptr = other.data.data();
    uint8_t* res_ptr = result.data.data();

    for (int idx = 0; idx < 32; ++idx) {
      const uint8_t lb = lhs_ptr[idx];
      const uint8_t rb = rhs_ptr[idx];
      const int8_t cin = carries[idx];

      const simd::AddEntry& e0 = simd::LookupAddEntry(DecodeTrit(lb & 0x3u), DecodeTrit(rb & 0x3u));
      const simd::AddEntry& e1 = simd::LookupAddEntry(DecodeTrit((lb >> 2) & 0x3u), DecodeTrit((rb >> 2) & 0x3u));
      const simd::AddEntry& e2 = simd::LookupAddEntry(DecodeTrit((lb >> 4) & 0x3u), DecodeTrit((rb >> 4) & 0x3u));
      const simd::AddEntry& e3 = simd::LookupAddEntry(DecodeTrit((lb >> 6) & 0x3u), DecodeTrit((rb >> 6) & 0x3u));

      int8_t c = cin;
      uint8_t rb0 = EncodeTrit(e0.sum[c + 1]);
      c = e0.carry[c + 1];
      uint8_t rb1 = EncodeTrit(e1.sum[c + 1]);
      c = e1.carry[c + 1];
      uint8_t rb2 = EncodeTrit(e2.sum[c + 1]);
      c = e2.carry[c + 1];
      uint8_t rb3 = EncodeTrit(e3.sum[c + 1]);

      res_ptr[idx] = rb0 | (rb1 << 2) | (rb2 << 4) | (rb3 << 6);
    }
#else
    std::array<int8_t, 128> lhs_digits{};
    std::array<int8_t, 128> rhs_digits{};
    std::array<int8_t, 128> sum_digits{};
    UnpackDigits(data, lhs_digits);
    UnpackDigits(other.data, rhs_digits);
    int8_t carry = 0;
    for (int idx = 0; idx < 128; ++idx) {
      const simd::AddEntry& entry = simd::LookupAddEntry(lhs_digits[idx], rhs_digits[idx]);
      const int table_index = carry + 1;
      sum_digits[idx] = entry.sum[table_index];
      carry = entry.carry[table_index];
    }
    PackDigits(sum_digits, result.data);
#endif
    return result;
  }

  T81 ShiftLeftTrits(int shift) const noexcept {
    if (shift >= 128) return T81{};
    std::array<int8_t, 128> digits{};
    UnpackDigits(data, digits);
    std::array<int8_t, 128> shifted{};
    for (int idx = 0; idx + shift < 128; ++idx) {
      shifted[idx + shift] = digits[idx];
    }
    T81 result;
    PackDigits(shifted, result.data);
    return result;
  }

  T81 operator-(const T81& other) const noexcept { return (*this) + (-other); }

  T81 operator*(const T81& other) const noexcept {
    std::array<int8_t, 128> lhs_digits{};
    std::array<int8_t, 128> rhs_digits{};
    UnpackDigits(data, lhs_digits);
    UnpackDigits(other.data, rhs_digits);
    std::array<int8_t, 256> result_digits{};
    for (int i = 0; i < 128; ++i) {
      for (int j = 0; j < 128; ++j) {
        result_digits[i + j] += lhs_digits[i] * rhs_digits[j];
      }
    }
    int64_t carry = 0;
    for (int idx = 0; idx < 256; ++idx) {
      int64_t value = static_cast<int64_t>(result_digits[idx]) + carry;
      int64_t next_carry = (value >= 0) ? (value + 1) / 3 : (value - 1) / 3;
      result_digits[idx] = static_cast<int8_t>(value - next_carry * 3);
      carry = next_carry;
    }
    std::array<int8_t, 128> final_digits{};
    for (int idx = 0; idx < 128; ++idx) {
      final_digits[idx] = result_digits[idx];
    }
    T81 final_result;
    PackDigits(final_digits, final_result.data);
    return final_result;
  }
};

}  // namespace t81
