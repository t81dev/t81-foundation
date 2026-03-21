#include <immintrin.h>
#include <algorithm>
#include "t81/simd/base81_digits.hpp"

namespace t81::simd::base81_digits {

int normalize_and_carry(std::span<int16_t> digits) {
  int carry = 0;
  for (auto& d : digits) {
    int val = d + carry;
    if (val > 80) {
      carry = 1;
      val -= 81;
    } else if (val < 0) {
      carry = -1;
      val += 81;
    } else {
      carry = 0;
    }
    d = static_cast<int16_t>(val);
  }
  return carry;
}

void normalize_add(std::span<const uint8_t> raw_sums, std::span<uint8_t> out) {
  int carry = 0;
  size_t n = std::min(raw_sums.size(), out.size());
  for (size_t i = 0; i < n; ++i) {
    int val = static_cast<int>(raw_sums[i]) - 40 + carry;
    if (val > 80) {
      carry = 1;
      val -= 81;
    } else if (val < 0) {
      carry = -1;
      val += 81;
    } else {
      carry = 0;
    }
    out[i] = static_cast<uint8_t>(val);
  }
}

void normalize_sub(std::span<const int16_t> raw_diffs, std::span<uint8_t> out) {
  int carry = 0;
  size_t n = std::min(raw_diffs.size(), out.size());
  for (size_t i = 0; i < n; ++i) {
    int val = static_cast<int>(raw_diffs[i]) + 40 + carry;
    if (val > 80) {
      carry = 1;
      val -= 81;
    } else if (val < 0) {
      carry = -1;
      val += 81;
    } else {
      carry = 0;
    }
    out[i] = static_cast<uint8_t>(val);
  }
}

void add(std::span<const uint8_t> a, std::span<const uint8_t> b, std::span<uint8_t> out) {
  size_t n = std::min({a.size(), b.size(), out.size()});
  size_t i = 0;
#if defined(__AVX2__)
  for (; i + 32 <= n; i += 32) {
    __m256i va = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&a[i]));
    __m256i vb = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&b[i]));
    __m256i vres = _mm256_adds_epu8(va, vb);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out[i]), vres);
  }
#endif
  for (; i < n; ++i) {
    out[i] = a[i] + b[i];
  }
}

void sub(std::span<const uint8_t> a, std::span<const uint8_t> b, std::span<int16_t> out) {
  size_t n = std::min({a.size(), b.size(), out.size()});
  size_t i = 0;
#if defined(__AVX2__)
  for (; i + 16 <= n; i += 16) {
    __m128i va128 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&a[i]));
    __m128i vb128 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&b[i]));
    __m256i va = _mm256_cvtepu8_epi16(va128);
    __m256i vb = _mm256_cvtepu8_epi16(vb128);
    __m256i vres = _mm256_sub_epi16(va, vb);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out[i]), vres);
  }
#endif
  for (; i < n; ++i) {
    out[i] = static_cast<int16_t>(static_cast<int>(a[i]) - static_cast<int>(b[i]));
  }
}

void add_constant(std::span<const uint8_t> in, uint8_t constant, std::span<uint8_t> out) {
  size_t n = std::min(in.size(), out.size());
  size_t i = 0;
#if defined(__AVX2__)
  __m256i vconst = _mm256_set1_epi8(static_cast<char>(constant));
  for (; i + 32 <= n; i += 32) {
    __m256i vin = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&in[i]));
    __m256i vres = _mm256_add_epi8(vin, vconst);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out[i]), vres);
  }
#endif
  for (; i < n; ++i) {
    out[i] = in[i] + constant;
  }
}

void mul_constant(std::span<const uint8_t> in, uint8_t constant, std::span<int16_t> out) {
  size_t n = std::min(in.size(), out.size());
  size_t i = 0;
#if defined(__AVX2__)
  __m256i vconst = _mm256_set1_epi16(static_cast<short>(constant));
  for (; i + 16 <= n; i += 16) {
    __m128i v128 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(&in[i]));
    __m256i vin = _mm256_cvtepu8_epi16(v128);
    __m256i vres = _mm256_mullo_epi16(vin, vconst);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out[i]), vres);
  }
#endif
  for (; i < n; ++i) {
    out[i] = static_cast<int16_t>(static_cast<int>(in[i]) * constant);
  }
}

void negate(std::span<const uint8_t> in, std::span<uint8_t> out) {
  size_t n = std::min(in.size(), out.size());
  size_t i = 0;
#if defined(__AVX2__)
  __m256i v80 = _mm256_set1_epi8(80);
  for (; i + 32 <= n; i += 32) {
    __m256i vin = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&in[i]));
    __m256i vres = _mm256_sub_epi8(v80, vin);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&out[i]), vres);
  }
#endif
  for (; i < n; ++i) {
    out[i] = 80 - in[i];
  }
}

void clamp(std::span<uint8_t> digits) {
  size_t n = digits.size();
  size_t i = 0;
#if defined(__AVX2__)
  __m256i v0 = _mm256_setzero_si256();
  __m256i v80 = _mm256_set1_epi8(80);
  for (; i + 32 <= n; i += 32) {
    __m256i v = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&digits[i]));
    v = _mm256_max_epu8(v, v0);
    v = _mm256_min_epu8(v, v80);
    _mm256_storeu_si256(reinterpret_cast<__m256i*>(&digits[i]), v);
  }
#endif
  for (; i < n; ++i) {
    if (digits[i] > 80) digits[i] = 80;
  }
}

int compare(std::span<const uint8_t> a, std::span<const uint8_t> b) {
  size_t na = a.size();
  size_t nb = b.size();
  size_t n = std::max(na, nb);

  for (size_t i = n; i-- > 0;) {
    uint8_t va = (i < na) ? a[i] : 40;
    uint8_t vb = (i < nb) ? b[i] : 40;
    if (va > vb) return 1;
    if (va < vb) return -1;
  }
  return 0;
}

}  // namespace t81::simd::base81_digits
