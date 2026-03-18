#pragma once

#include <array>
#include <cstdint>

#if defined(__x86_64__) && defined(__AVX2__)
#  include <immintrin.h>
#elif defined(__ARM_NEON)
#  include <arm_neon.h>
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
    // Negation trick: for 2-bit trit encoding 00=−1, 01=0, 10=+1,
    // the negation of each byte is (0xAA - byte): 0b10101010 − x
    // flips 00↔10 while leaving 01 unchanged.
#if defined(__x86_64__) && defined(__AVX2__)
    const __m256i v   = _mm256_load_si256(reinterpret_cast<const __m256i*>(data.data()));
    const __m256i mask = _mm256_set1_epi8(static_cast<int8_t>(0xAA));
    const __m256i neg  = _mm256_sub_epi8(mask, v);
    T81 result;
    _mm256_store_si256(reinterpret_cast<__m256i*>(result.data.data()), neg);
    return result;
#elif defined(__ARM_NEON)
    const uint8x16_t mask = vdupq_n_u8(0xAAu);
    const uint8x16_t lo   = vsubq_u8(mask, vld1q_u8(data.data()));
    const uint8x16_t hi   = vsubq_u8(mask, vld1q_u8(data.data() + 16));
    T81 result;
    vst1q_u8(result.data.data(),      lo);
    vst1q_u8(result.data.data() + 16, hi);
    return result;
#else
    std::array<int8_t, 128> digits{};
    UnpackDigits(data, digits);
    for (int idx = 0; idx < 128; ++idx) digits[idx] = static_cast<int8_t>(-digits[idx]);
    T81 result;
    PackDigits(digits, result.data);
    return result;
#endif
  }

  T81 operator+(const T81& other) const noexcept {
    T81 result;

#if defined(__x86_64__) && defined(__AVX2__)
    // Common byte-apply helper shared by all ISA paths.
    // Given per-byte carry-ins from PrefixScan, applies them trit-by-trit.
    auto apply_carries = [&](const std::array<int8_t, 32>& carries) {
      const uint8_t* lp = data.data();
      const uint8_t* rp = other.data.data();
      uint8_t*       dp = result.data.data();
      for (int i = 0; i < 32; ++i) {
        const uint8_t lb = lp[i], rb = rp[i];
        const simd::AddEntry& e0 = simd::LookupAddEntry(DecodeTrit( lb       & 0x3u), DecodeTrit( rb       & 0x3u));
        const simd::AddEntry& e1 = simd::LookupAddEntry(DecodeTrit((lb >> 2) & 0x3u), DecodeTrit((rb >> 2) & 0x3u));
        const simd::AddEntry& e2 = simd::LookupAddEntry(DecodeTrit((lb >> 4) & 0x3u), DecodeTrit((rb >> 4) & 0x3u));
        const simd::AddEntry& e3 = simd::LookupAddEntry(DecodeTrit((lb >> 6) & 0x3u), DecodeTrit((rb >> 6) & 0x3u));
        int8_t c = carries[i];
        const uint8_t b0 = EncodeTrit(e0.sum[c + 1]); c = e0.carry[c + 1];
        const uint8_t b1 = EncodeTrit(e1.sum[c + 1]); c = e1.carry[c + 1];
        const uint8_t b2 = EncodeTrit(e2.sum[c + 1]); c = e2.carry[c + 1];
        const uint8_t b3 = EncodeTrit(e3.sum[c + 1]);
        dp[i] = b0 | (b1 << 2) | (b2 << 4) | (b3 << 6);
      }
    };

    {
      const __m256i lv = _mm256_load_si256(reinterpret_cast<const __m256i*>(data.data()));
      const __m256i rv = _mm256_load_si256(reinterpret_cast<const __m256i*>(other.data.data()));
      std::array<simd::ByteCarryMap, 32> maps{};
      simd::BuildCarryMaps(lv, rv, maps);
      simd::PrefixScan(maps);
      apply_carries(simd::CarryIns(maps));
    }
#elif defined(__ARM_NEON)
    // Common byte-apply helper shared by all ISA paths.
    // Given per-byte carry-ins from PrefixScan, applies them trit-by-trit.
    auto apply_carries = [&](const std::array<int8_t, 32>& carries) {
      const uint8_t* lp = data.data();
      const uint8_t* rp = other.data.data();
      uint8_t*       dp = result.data.data();
      for (int i = 0; i < 32; ++i) {
        const uint8_t lb = lp[i], rb = rp[i];
        const simd::AddEntry& e0 = simd::LookupAddEntry(DecodeTrit( lb       & 0x3u), DecodeTrit( rb       & 0x3u));
        const simd::AddEntry& e1 = simd::LookupAddEntry(DecodeTrit((lb >> 2) & 0x3u), DecodeTrit((rb >> 2) & 0x3u));
        const simd::AddEntry& e2 = simd::LookupAddEntry(DecodeTrit((lb >> 4) & 0x3u), DecodeTrit((rb >> 4) & 0x3u));
        const simd::AddEntry& e3 = simd::LookupAddEntry(DecodeTrit((lb >> 6) & 0x3u), DecodeTrit((rb >> 6) & 0x3u));
        int8_t c = carries[i];
        const uint8_t b0 = EncodeTrit(e0.sum[c + 1]); c = e0.carry[c + 1];
        const uint8_t b1 = EncodeTrit(e1.sum[c + 1]); c = e1.carry[c + 1];
        const uint8_t b2 = EncodeTrit(e2.sum[c + 1]); c = e2.carry[c + 1];
        const uint8_t b3 = EncodeTrit(e3.sum[c + 1]);
        dp[i] = b0 | (b1 << 2) | (b2 << 4) | (b3 << 6);
      }
    };

    {
      // Use NEON to load the 32-byte operands into registers, then extract
      // for the carry-map build. PrefixScan and apply_carries are scalar.
      const uint8x16x2_t lv = { vld1q_u8(data.data()), vld1q_u8(data.data() + 16) };
      const uint8x16x2_t rv = { vld1q_u8(other.data.data()), vld1q_u8(other.data.data() + 16) };
      std::array<simd::ByteCarryMap, 32> maps{};
      simd::BuildCarryMaps(lv, rv, maps);
      simd::PrefixScan(maps);
      apply_carries(simd::CarryIns(maps));
    }
#else
    {
      // Portable scalar carry chain (no SIMD).
      std::array<int8_t, 128> ld{}, rd{}, sd{};
      UnpackDigits(data, ld);
      UnpackDigits(other.data, rd);
      int8_t carry = 0;
      for (int i = 0; i < 128; ++i) {
        const simd::AddEntry& e = simd::LookupAddEntry(ld[i], rd[i]);
        sd[i]  = e.sum[carry + 1];
        carry  = e.carry[carry + 1];
      }
      PackDigits(sd, result.data);
    }
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
