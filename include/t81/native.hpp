#pragma once

#include <array>
#include <cstdint>

#if defined(__AVX2__)
#include <immintrin.h>
#endif

namespace t81 {

struct alignas(32) T81 {
    alignas(32) std::array<uint8_t, 32> data{};

    constexpr T81() noexcept = default;

    // From packed byte block — required for conversion and fallback
    explicit constexpr T81(const std::array<uint8_t, 32>& block) noexcept
        : data(block) {}

    // From raw AVX2 register — used in benchmarks
#if defined(__AVX2__)
    explicit T81(__m256i v) noexcept {
        _mm256_store_si256(reinterpret_cast<__m256i*>(data.data()), v);
    }

    __m256i avx() const noexcept {
        return _mm256_load_si256(reinterpret_cast<const __m256i*>(data.data()));
    }
#endif

    // ONE INSTRUCTION NEGATION — beats two’s-complement
    T81 operator-() const noexcept {
#if defined(__AVX2__)
        const __m256i mask = _mm256_set1_epi8(static_cast<int8_t>(0b10010000));
        const __m256i v = _mm256_load_si256(reinterpret_cast<const __m256i*>(data.data()));
        const __m256i negated = _mm256_shuffle_epi8(v, mask);
        T81 result;
        _mm256_store_si256(reinterpret_cast<__m256i*>(result.data.data()), negated);
        return result;
#else
        T81 result = *this;
        for (auto& b : result.data) {
            b = uint8_t((b & 0x55) << 1) | uint8_t((b & 0xAA) >> 1);
        }
        return result;
#endif
    }

    // Addition placeholder — will be SIMD-crushed later
    T81 operator+(const T81& other) const noexcept {
        T81 result;
#if defined(__AVX2__)
        const __m256i a = _mm256_load_si256(reinterpret_cast<const __m256i*>(data.data()));
        const __m256i b = _mm256_load_si256(reinterpret_cast<const __m256i*>(other.data.data()));
        _mm256_store_si256(reinterpret_cast<__m256i*>(result.data.data()), _mm256_add_epi8(a, b));
#else
        result = *this;
#endif
        return result;
    }
};

} // namespace t81
