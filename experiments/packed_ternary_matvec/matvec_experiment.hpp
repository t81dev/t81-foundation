// experiments/packed_ternary_matvec/matvec_experiment.hpp
//
// Standalone performance and correctness comparison for Matrix-Vector Multiplication:
// 1. FP32 weights (scalar and AVX2)
// 2. INT8 weights (scalar and AVX2)
// 3. Ternary weights (1 byte per weight: conditional and branchless)
// 4. Ternary weights (2 bits packed per weight: 0.25 bytes/weight)
//    - On-the-fly scalar decoding
//    - Pre-unpacked batch execution
//    - 4-trit extraction / SWAR
//    - 4-trit LUT extraction
//
// Independent double-precision oracle included for verification.

#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <type_traits>
#include <vector>

#if defined(__x86_64__) && defined(__AVX2__)
#include <immintrin.h>
#endif

namespace t81::research::matvec {

// Prevent compiler optimization from eliminating work
template <typename T>
inline void do_not_optimize(T&& value) {
#if defined(__GNUC__) || defined(__clang__)
  asm volatile("" : : "r,m"(value) : "memory");
#else
  static volatile std::decay_t<T> sink;
  sink = value;
#endif
}

// ---------------------------------------------------------------------------
// 2-bit Trit Packing Definitions (4 trits per byte)
// Encoding: 0 -> 00, +1 -> 01, -1 -> 11 (10 is invalid/padding -> 0)
// ---------------------------------------------------------------------------
inline constexpr uint8_t TRIT_2BIT_ZERO = 0x00;  // 00
inline constexpr uint8_t TRIT_2BIT_POS  = 0x01;  // 01
inline constexpr uint8_t TRIT_2BIT_NEG  = 0x03;  // 11

inline uint8_t encode_trit_2bit(int8_t t) {
  if (t == 1) return TRIT_2BIT_POS;
  if (t == -1) return TRIT_2BIT_NEG;
  return TRIT_2BIT_ZERO;
}

inline int8_t decode_trit_2bit(uint8_t code) {
  code &= 0x03;
  if (code == TRIT_2BIT_POS) return 1;
  if (code == TRIT_2BIT_NEG) return -1;
  return 0;
}

// Pack array of ternary trits {-1, 0, +1} into 2-bit byte vector.
inline std::vector<uint8_t> pack_ternary_2bit(const int8_t* trits, size_t count) {
  size_t bytes = (count + 3) / 4;
  std::vector<uint8_t> packed(bytes, 0);
  for (size_t i = 0; i < count; ++i) {
    size_t byte_idx = i / 4;
    size_t lane = i % 4;
    uint8_t enc = encode_trit_2bit(trits[i]);
    packed[byte_idx] |= static_cast<uint8_t>(enc << (lane * 2));
  }
  return packed;
}

// Unpack 2-bit byte vector back into ternary trits {-1, 0, +1}.
inline void unpack_ternary_2bit(const uint8_t* packed, size_t count, int8_t* out_trits) {
  for (size_t i = 0; i < count; ++i) {
    size_t byte_idx = i / 4;
    size_t lane = i % 4;
    uint8_t code = (packed[byte_idx] >> (lane * 2)) & 0x03;
    out_trits[i] = decode_trit_2bit(code);
  }
}

// ---------------------------------------------------------------------------
// 4-Trit Lookup Table
// ---------------------------------------------------------------------------
struct TritLut4 {
  int8_t trits[4];
};

inline const std::array<TritLut4, 256>& get_trit_lut_4() {
  static const auto lut = []() {
    std::array<TritLut4, 256> table{};
    for (int b = 0; b < 256; ++b) {
      for (int lane = 0; lane < 4; ++lane) {
        uint8_t code = (b >> (lane * 2)) & 0x03;
        table[b].trits[lane] = decode_trit_2bit(static_cast<uint8_t>(code));
      }
    }
    return table;
  }();
  return lut;
}

// ---------------------------------------------------------------------------
// Representation Structures
// ---------------------------------------------------------------------------
struct MatrixData {
  size_t m{0};  // rows
  size_t k{0};  // cols

  // Logical ternary weights {-1, 0, +1}
  std::vector<int8_t> logical_trits;

  // 1. FP32
  std::vector<float> w_fp32;

  // 2. INT8
  std::vector<int8_t> w_int8;

  // 3. Ternary 1-Byte
  std::vector<int8_t> w_ternary_byte;

  // 4. Ternary 2-Bit Packed
  std::vector<uint8_t> w_packed_2bit;

  static MatrixData create(size_t m, size_t k, const std::vector<int8_t>& trits) {
    MatrixData data;
    data.m = m;
    data.k = k;
    data.logical_trits = trits;

    size_t total = m * k;
    data.w_fp32.resize(total);
    data.w_int8.resize(total);
    data.w_ternary_byte.resize(total);

    for (size_t i = 0; i < total; ++i) {
      int8_t t = trits[i];
      data.w_fp32[i] = static_cast<float>(t);
      data.w_int8[i] = t;
      data.w_ternary_byte[i] = t;
    }

    data.w_packed_2bit = pack_ternary_2bit(trits.data(), total);
    return data;
  }
};

// ---------------------------------------------------------------------------
// Matrix-Vector Multiplication Kernels
// ---------------------------------------------------------------------------

// Reference Double Precision Oracle
inline void matvec_oracle_f64(size_t m, size_t k, const int8_t* logical_trits,
                             const float* x, float* y_out) {
  for (size_t i = 0; i < m; ++i) {
    double acc = 0.0;
    size_t row_offset = i * k;
    for (size_t j = 0; j < k; ++j) {
      acc += static_cast<double>(logical_trits[row_offset + j]) * static_cast<double>(x[j]);
    }
    y_out[i] = static_cast<float>(acc);
  }
}

// 1a. FP32 MatVec (Scalar)
inline void matvec_fp32(size_t m, size_t k, const float* w, const float* x, float* y_out) {
  for (size_t i = 0; i < m; ++i) {
    float acc = 0.0f;
    size_t row_offset = i * k;
    for (size_t j = 0; j < k; ++j) {
      acc += w[row_offset + j] * x[j];
    }
    y_out[i] = acc;
  }
}

// 1b. FP32 MatVec (AVX2 Vectorized)
inline void matvec_fp32_avx2(size_t m, size_t k, const float* w, const float* x, float* y_out) {
#if defined(__x86_64__) && defined(__AVX2__)
  for (size_t i = 0; i < m; ++i) {
    size_t row_offset = i * k;
    __m256 vacc0 = _mm256_setzero_ps();
    __m256 vacc1 = _mm256_setzero_ps();
    size_t j = 0;
    for (; j + 15 < k; j += 16) {
      __m256 w0 = _mm256_loadu_ps(w + row_offset + j);
      __m256 x0 = _mm256_loadu_ps(x + j);
      vacc0 = _mm256_fmadd_ps(w0, x0, vacc0);

      __m256 w1 = _mm256_loadu_ps(w + row_offset + j + 8);
      __m256 x1 = _mm256_loadu_ps(x + j + 8);
      vacc1 = _mm256_fmadd_ps(w1, x1, vacc1);
    }
    for (; j + 7 < k; j += 8) {
      __m256 w0 = _mm256_loadu_ps(w + row_offset + j);
      __m256 x0 = _mm256_loadu_ps(x + j);
      vacc0 = _mm256_fmadd_ps(w0, x0, vacc0);
    }
    __m256 vsum = _mm256_add_ps(vacc0, vacc1);
    alignas(32) float tmp[8];
    _mm256_storeu_ps(tmp, vsum);
    float acc = tmp[0] + tmp[1] + tmp[2] + tmp[3] + tmp[4] + tmp[5] + tmp[6] + tmp[7];
    for (; j < k; ++j) {
      acc += w[row_offset + j] * x[j];
    }
    y_out[i] = acc;
  }
#else
  matvec_fp32(m, k, w, x, y_out);
#endif
}

// 2a. INT8 MatVec (Scalar FP32 Accumulation)
inline void matvec_int8(size_t m, size_t k, const int8_t* w, const float* x, float* y_out) {
  for (size_t i = 0; i < m; ++i) {
    float acc = 0.0f;
    size_t row_offset = i * k;
    for (size_t j = 0; j < k; ++j) {
      acc += static_cast<float>(w[row_offset + j]) * x[j];
    }
    y_out[i] = acc;
  }
}

// 2b. INT8 MatVec (AVX2 Vectorized FP32 Accumulation)
inline void matvec_int8_avx2(size_t m, size_t k, const int8_t* w, const float* x, float* y_out) {
#if defined(__x86_64__) && defined(__AVX2__)
  for (size_t i = 0; i < m; ++i) {
    size_t row_offset = i * k;
    __m256 vacc0 = _mm256_setzero_ps();
    __m256 vacc1 = _mm256_setzero_ps();
    size_t j = 0;
    for (; j + 15 < k; j += 16) {
      __m128i raw8 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(w + row_offset + j));
      __m256i raw32_0 = _mm256_cvtepi8_epi32(raw8);
      __m256i raw32_1 = _mm256_cvtepi8_epi32(_mm_srli_si128(raw8, 8));

      __m256 fw0 = _mm256_cvtepi32_ps(raw32_0);
      __m256 fw1 = _mm256_cvtepi32_ps(raw32_1);

      __m256 x0 = _mm256_loadu_ps(x + j);
      __m256 x1 = _mm256_loadu_ps(x + j + 8);

      vacc0 = _mm256_fmadd_ps(fw0, x0, vacc0);
      vacc1 = _mm256_fmadd_ps(fw1, x1, vacc1);
    }
    for (; j + 7 < k; j += 8) {
      int64_t raw64;
      std::memcpy(&raw64, w + row_offset + j, 8);
      __m128i raw8 = _mm_cvtsi64_si128(raw64);
      __m256i raw32 = _mm256_cvtepi8_epi32(raw8);
      __m256 fw = _mm256_cvtepi32_ps(raw32);
      __m256 x0 = _mm256_loadu_ps(x + j);
      vacc0 = _mm256_fmadd_ps(fw, x0, vacc0);
    }
    __m256 vsum = _mm256_add_ps(vacc0, vacc1);
    alignas(32) float tmp[8];
    _mm256_storeu_ps(tmp, vsum);
    float acc = tmp[0] + tmp[1] + tmp[2] + tmp[3] + tmp[4] + tmp[5] + tmp[6] + tmp[7];
    for (; j < k; ++j) {
      acc += static_cast<float>(w[row_offset + j]) * x[j];
    }
    y_out[i] = acc;
  }
#else
  matvec_int8(m, k, w, x, y_out);
#endif
}

// 3a. Ternary 1-Byte MatVec (Conditional Arithmetic)
inline void matvec_ternary_byte_cond(size_t m, size_t k, const int8_t* w, const float* x, float* y_out) {
  for (size_t i = 0; i < m; ++i) {
    float acc = 0.0f;
    size_t row_offset = i * k;
    for (size_t j = 0; j < k; ++j) {
      int8_t wt = w[row_offset + j];
      if (wt == 1) {
        acc += x[j];
      } else if (wt == -1) {
        acc -= x[j];
      }
    }
    y_out[i] = acc;
  }
}

// 3b. Ternary 1-Byte MatVec (Branchless multiply-free accumulator)
inline void matvec_ternary_byte_branchless(size_t m, size_t k, const int8_t* w, const float* x, float* y_out) {
  for (size_t i = 0; i < m; ++i) {
    float acc = 0.0f;
    size_t row_offset = i * k;
    for (size_t j = 0; j < k; ++j) {
      int8_t wt = w[row_offset + j];
      acc += static_cast<float>(wt) * x[j];
    }
    y_out[i] = acc;
  }
}

// 4a. Ternary 2-bit Packed MatVec (Scalar On-the-Fly Decoding)
inline void matvec_ternary_2bit_onthefly(size_t m, size_t k, const uint8_t* packed_w,
                                         const float* x, float* y_out) {
  for (size_t i = 0; i < m; ++i) {
    float acc = 0.0f;
    size_t row_trit_offset = i * k;
    for (size_t j = 0; j < k; ++j) {
      size_t global_idx = row_trit_offset + j;
      size_t byte_idx = global_idx / 4;
      size_t lane = global_idx % 4;
      uint8_t code = (packed_w[byte_idx] >> (lane * 2)) & 0x03;
      int8_t wt = decode_trit_2bit(code);
      if (wt == 1) {
        acc += x[j];
      } else if (wt == -1) {
        acc -= x[j];
      }
    }
    y_out[i] = acc;
  }
}

// 4b. Ternary 2-bit Packed MatVec (Pre-Unpacked Batch)
inline void matvec_ternary_2bit_preunpacked(size_t m, size_t k, const uint8_t* packed_w,
                                            const float* x, float* y_out,
                                            std::vector<int8_t>& scratch_buf) {
  size_t total = m * k;
  if (scratch_buf.size() < total) scratch_buf.resize(total);
  unpack_ternary_2bit(packed_w, total, scratch_buf.data());
  matvec_ternary_byte_cond(m, k, scratch_buf.data(), x, y_out);
}

// 4c. Ternary 2-bit Packed MatVec (SWAR / Bitmask Extract with Alignment Handling)
inline void matvec_ternary_2bit_swar(size_t m, size_t k, const uint8_t* packed_w,
                                     const float* x, float* y_out) {
  for (size_t i = 0; i < m; ++i) {
    float acc = 0.0f;
    size_t row_trit_offset = i * k;
    size_t j = 0;

    // Align to 4-trit (1-byte) boundary if row_trit_offset is not 4-trit aligned
    while (j < k && ((row_trit_offset + j) % 4 != 0)) {
      size_t global_idx = row_trit_offset + j;
      uint8_t code = (packed_w[global_idx / 4] >> ((global_idx % 4) * 2)) & 0x03;
      int8_t wt = decode_trit_2bit(code);
      if (wt == 1) acc += x[j];
      else if (wt == -1) acc -= x[j];
      ++j;
    }

    // Now (row_trit_offset + j) is divisible by 4
    while (j + 3 < k) {
      size_t global_idx = row_trit_offset + j;
      uint8_t byte = packed_w[global_idx / 4];

      // Trit 0: bits 0..1
      uint8_t c0 = byte & 0x03;
      if (c0 == TRIT_2BIT_POS) acc += x[j];
      else if (c0 == TRIT_2BIT_NEG) acc -= x[j];

      // Trit 1: bits 2..3
      uint8_t c1 = (byte >> 2) & 0x03;
      if (c1 == TRIT_2BIT_POS) acc += x[j + 1];
      else if (c1 == TRIT_2BIT_NEG) acc -= x[j + 1];

      // Trit 2: bits 4..5
      uint8_t c2 = (byte >> 4) & 0x03;
      if (c2 == TRIT_2BIT_POS) acc += x[j + 2];
      else if (c2 == TRIT_2BIT_NEG) acc -= x[j + 2];

      // Trit 3: bits 6..7
      uint8_t c3 = (byte >> 6) & 0x03;
      if (c3 == TRIT_2BIT_POS) acc += x[j + 3];
      else if (c3 == TRIT_2BIT_NEG) acc -= x[j + 3];

      j += 4;
    }

    // Scalar remainder trits
    for (; j < k; ++j) {
      size_t global_idx = row_trit_offset + j;
      uint8_t code = (packed_w[global_idx / 4] >> ((global_idx % 4) * 2)) & 0x03;
      int8_t wt = decode_trit_2bit(code);
      if (wt == 1) acc += x[j];
      else if (wt == -1) acc -= x[j];
    }

    y_out[i] = acc;
  }
}

// 4d. Ternary 2-bit Packed MatVec (4-trit LUT Extraction)
inline void matvec_ternary_2bit_lut(size_t m, size_t k, const uint8_t* packed_w,
                                    const float* x, float* y_out) {
  const auto& lut = get_trit_lut_4();
  for (size_t i = 0; i < m; ++i) {
    float acc = 0.0f;
    size_t row_trit_offset = i * k;
    size_t j = 0;

    while (j < k && ((row_trit_offset + j) % 4 != 0)) {
      size_t global_idx = row_trit_offset + j;
      uint8_t code = (packed_w[global_idx / 4] >> ((global_idx % 4) * 2)) & 0x03;
      int8_t wt = decode_trit_2bit(code);
      acc += static_cast<float>(wt) * x[j];
      ++j;
    }

    while (j + 3 < k) {
      size_t global_idx = row_trit_offset + j;
      uint8_t byte = packed_w[global_idx / 4];
      const auto& t = lut[byte].trits;
      acc += static_cast<float>(t[0]) * x[j] +
             static_cast<float>(t[1]) * x[j + 1] +
             static_cast<float>(t[2]) * x[j + 2] +
             static_cast<float>(t[3]) * x[j + 3];
      j += 4;
    }

    for (; j < k; ++j) {
      size_t global_idx = row_trit_offset + j;
      uint8_t code = (packed_w[global_idx / 4] >> ((global_idx % 4) * 2)) & 0x03;
      int8_t wt = decode_trit_2bit(code);
      acc += static_cast<float>(wt) * x[j];
    }

    y_out[i] = acc;
  }
}

}  // namespace t81::research::matvec
