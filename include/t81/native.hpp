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

    explicit constexpr T81(const std::array<uint8_t, 32>& block) noexcept
        : data(block) {}

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
                digits[byte_index * 4 + trit] =
                    DecodeTrit((byte >> (trit * 2)) & 0x3u);
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
        for (int trit = 0; trit < 4; ++trit) {
            const int shift = trit * 2;
            const int8_t a = DecodeTrit((lhs >> shift) & 0x3u);
            const int8_t b = DecodeTrit((rhs >> shift) & 0x3u);
            const simd::AddEntry& entry = simd::LookupAddEntry(a, b);
            const int table_index = carry + 1;
            const int8_t sum = entry.sum[table_index];
            carry = entry.carry[table_index];
            result |= EncodeTrit(sum) << shift;
        }
        return result;
    }

    T81 operator-() const noexcept {
#if defined(__x86_64__) && defined(__AVX2__)
        const __m256i mask = _mm256_set1_epi8(static_cast<int8_t>(0b10010000));
        const __m256i v = _mm256_load_si256(reinterpret_cast<const __m256i*>(data.data()));
        const __m256i negated = _mm256_shuffle_epi8(v, mask);
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
        const __m256i lhs = _mm256_load_si256(reinterpret_cast<const __m256i*>(data.data()));
        const __m256i rhs = _mm256_load_si256(reinterpret_cast<const __m256i*>(other.data.data()));
        std::array<simd::ByteCarryMap, 32> maps{};
        simd::BuildCarryMaps(lhs, rhs, maps);
        simd::PrefixScan(maps);
        auto carries = simd::CarryIns(maps);
        alignas(32) uint8_t lhs_bytes[32];
        alignas(32) uint8_t rhs_bytes[32];
        alignas(32) uint8_t result_bytes[32];
        _mm256_store_si256(reinterpret_cast<__m256i*>(lhs_bytes), lhs);
        _mm256_store_si256(reinterpret_cast<__m256i*>(rhs_bytes), rhs);
        for (int idx = 0; idx < 32; ++idx) {
            result_bytes[idx] = AddByte(lhs_bytes[idx], rhs_bytes[idx], carries[idx]);
        }
        _mm256_store_si256(reinterpret_cast<__m256i*>(result.data.data()),
                           _mm256_load_si256(reinterpret_cast<__m256i*>(result_bytes)));
#else
        std::array<int8_t, 128> lhs_digits{};
        std::array<int8_t, 128> rhs_digits{};
        std::array<int8_t, 128> sum_digits{};
        UnpackDigits(data, lhs_digits);
        UnpackDigits(other.data, rhs_digits);
        int8_t carry = 0;
        for (int idx = 0; idx < 128; ++idx) {
            const simd::AddEntry& entry =
                simd::LookupAddEntry(lhs_digits[idx], rhs_digits[idx]);
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

    T81 operator-(const T81& other) const noexcept {
        return (*this) + (-other);
    }

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

} // namespace t81
