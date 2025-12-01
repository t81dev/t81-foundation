#pragma once

#include <array>
#include <cstdint>

#if defined(__x86_64__) && defined(__AVX2__)
#include <immintrin.h>
#endif

namespace t81 {

namespace detail {
using ByteBlock = std::array<uint8_t, 32>;

constexpr uint8_t encode_trit(int8_t value) noexcept {
    if (value <= -1) return 0;
    if (value == 0) return 1;
    return 2;
}

constexpr int8_t decode_trit(uint8_t pattern) noexcept {
    switch (pattern & 0x3u) {
        case 0: return -1;
        case 1: return 0;
        default: return +1;
    }
}

inline ByteBlock pack_digits(const int8_t digits[128]) noexcept {
    ByteBlock bytes{};
    for (size_t byte_index = 0; byte_index < 32; ++byte_index) {
        uint8_t chunk = 0;
        for (size_t trit_index = 0; trit_index < 4; ++trit_index) {
            const uint8_t pattern =
                encode_trit(digits[byte_index * 4 + trit_index]);
            chunk |= pattern << (trit_index * 2);
        }
        bytes[byte_index] = chunk;
    }
    return bytes;
}

inline void unpack_digits(const ByteBlock& value, int8_t digits[128]) noexcept {
    for (size_t byte_index = 0; byte_index < 32; ++byte_index) {
        uint8_t chunk = value[byte_index];
        for (size_t trit_index = 0; trit_index < 4; ++trit_index) {
            digits[byte_index * 4 + trit_index] =
                decode_trit((chunk >> (trit_index * 2)) & 0x3u);
        }
    }
}

inline void decode_tryte(int8_t value, int8_t digits[3]) noexcept {
    const bool negative = value < 0;
    int temp = negative ? -value : value;
    for (int idx = 0; idx < 3; ++idx) {
        int rem = temp % 3;
        if (rem == 2) {
            digits[idx] = -1;
            temp = temp / 3 + 1;
        } else {
            digits[idx] = static_cast<int8_t>(rem - 1);
            temp /= 3;
        }
    }
    if (negative) {
        digits[0] = static_cast<int8_t>(-digits[0]);
        digits[1] = static_cast<int8_t>(-digits[1]);
        digits[2] = static_cast<int8_t>(-digits[2]);
    }
}

inline int8_t encode_tryte(int8_t digits[3]) noexcept {
    return digits[0] + 3 * digits[1] + 9 * digits[2];
}
} // namespace detail

struct alignas(32) T81 {
    detail::ByteBlock data;

    constexpr T81() noexcept : data{} {}
    explicit constexpr T81(detail::ByteBlock block) noexcept : data(block) {}

    constexpr T81 operator-() const noexcept {
#if defined(__x86_64__) && defined(__AVX2__)
        alignas(32) static constexpr uint8_t neg_mask[32] = {
            0b10010000,0b10010000,0b10010000,0b10010000,
            0b10010000,0b10010000,0b10010000,0b10010000,
            0b10010000,0b10010000,0b10010000,0b10010000,
            0b10010000,0b10010000,0b10010000,0b10010000,
            0b10010000,0b10010000,0b10010000,0b10010000,
            0b10010000,0b10010000,0b10010000,0b10010000,
            0b10010000,0b10010000,0b10010000,0b10010000,
            0b10010000,0b10010000,0b10010000,0b10010000
        };
        const __m256i mask =
            _mm256_load_si256(reinterpret_cast<const __m256i*>(neg_mask));
        const __m256i current =
            _mm256_load_si256(reinterpret_cast<const __m256i*>(data.data()));
        __m256i shuffled = _mm256_shuffle_epi8(current, mask);
        T81 result;
        _mm256_store_si256(reinterpret_cast<__m256i*>(result.data.data()), shuffled);
        return result;
#else
        int8_t digits[128];
        detail::unpack_digits(data, digits);
        for (int i = 0; i < 128; ++i) {
            digits[i] = static_cast<int8_t>(-digits[i]);
        }
        return T81{detail::pack_digits(digits)};
#endif
    }

    T81 operator+(const T81& other) const noexcept {
        int8_t lhs[128];
        int8_t rhs[128];
        int8_t result[128];
        detail::unpack_digits(data, lhs);
        detail::unpack_digits(other.data, rhs);
        int carry = 0;
        for (size_t i = 0; i < 128; ++i) {
            int sum = int(lhs[i]) + int(rhs[i]) + carry;
            if (sum > 1) {
                sum -= 3;
                carry = 1;
            } else if (sum < -1) {
                sum += 3;
                carry = -1;
            } else {
                carry = 0;
            }
            result[i] = static_cast<int8_t>(sum);
        }
        return T81{detail::pack_digits(result)};
    }
};

} // namespace t81
