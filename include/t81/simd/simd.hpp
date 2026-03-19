#pragma once

#include <cstddef>

#include "t81/experimental/packed_trit_vector.hpp"
#include "t81/types/Result.hpp"

namespace t81::simd {

using ComputeTritVector = t81::experimental::ComputeTritVector;
using PackedTritVector = t81::experimental::PackedTritVector;

inline constexpr std::size_t avx2_threshold_bytes() {
  return ComputeTritVector::AVX2_THRESHOLD_BYTES;
}

inline constexpr std::size_t neon_threshold_bytes() {
  return ComputeTritVector::NEON_THRESHOLD_BYTES;
}

inline constexpr std::size_t t_not_threshold_bytes() {
#if defined(__x86_64__) && defined(__AVX2__)
  return ComputeTritVector::AVX2_TNOT_THRESHOLD_BYTES;
#elif defined(__aarch64__) && defined(__ARM_NEON)
  return ComputeTritVector::NEON_TNOT_THRESHOLD_BYTES;
#else
  return 0;
#endif
}

inline constexpr std::size_t t_and_threshold_bytes() {
#if defined(__x86_64__) && defined(__AVX2__)
  return ComputeTritVector::AVX2_TAND_THRESHOLD_BYTES;
#elif defined(__aarch64__) && defined(__ARM_NEON)
  return ComputeTritVector::NEON_TAND_THRESHOLD_BYTES;
#else
  return 0;
#endif
}

inline constexpr std::size_t t_or_threshold_bytes() {
#if defined(__x86_64__) && defined(__AVX2__)
  return ComputeTritVector::AVX2_TOR_THRESHOLD_BYTES;
#elif defined(__aarch64__) && defined(__ARM_NEON)
  return ComputeTritVector::NEON_TOR_THRESHOLD_BYTES;
#else
  return 0;
#endif
}

inline constexpr bool is_avx2_available() {
#if defined(__x86_64__) && defined(__AVX2__)
  return true;
#else
  return false;
#endif
}

inline constexpr bool is_neon_available() {
#if defined(__aarch64__) && defined(__ARM_NEON)
  return true;
#else
  return false;
#endif
}

inline constexpr std::size_t get_optimal_threshold() {
  return t_or_threshold_bytes();
}

inline Result<ComputeTritVector> t_not(const ComputeTritVector& input) {
  return input.t_not();
}

inline Result<ComputeTritVector> t_and(const ComputeTritVector& a, const ComputeTritVector& b) {
  return a.t_and(b);
}

inline Result<ComputeTritVector> t_or(const ComputeTritVector& a, const ComputeTritVector& b) {
  return a.t_or(b);
}

inline Result<ComputeTritVector> t_neg(const ComputeTritVector& input) {
  return t_not(input);
}

inline Result<ComputeTritVector> t_min(const ComputeTritVector& a, const ComputeTritVector& b) {
  return t_and(a, b);
}

inline Result<ComputeTritVector> t_max(const ComputeTritVector& a, const ComputeTritVector& b) {
  return t_or(a, b);
}

inline Result<bool> t_not_inplace(ComputeTritVector& input) {
  return input.t_not_inplace();
}

inline Result<bool> t_and_inplace(ComputeTritVector& a, const ComputeTritVector& b) {
  return a.t_and_inplace(b);
}

inline Result<bool> t_or_inplace(ComputeTritVector& a, const ComputeTritVector& b) {
  return a.t_or_inplace(b);
}

namespace kernel {

inline void t_not(const uint8_t* src, uint8_t* dst, std::size_t len) {
  ComputeTritVector::kernel_not(src, dst, len);
}

inline void t_and(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, std::size_t len) {
  ComputeTritVector::kernel_and(src_a, src_b, dst, len);
}

inline void t_or(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, std::size_t len) {
  ComputeTritVector::kernel_or(src_a, src_b, dst, len);
}

inline void t_not_swar(const uint8_t* src, uint8_t* dst, std::size_t len) {
  ComputeTritVector::kernel_not_swar(src, dst, len);
}

inline void t_and_swar(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst,
                       std::size_t len) {
  ComputeTritVector::kernel_and_swar(src_a, src_b, dst, len);
}

inline void t_or_swar(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst,
                      std::size_t len) {
  ComputeTritVector::kernel_or_swar(src_a, src_b, dst, len);
}

}  // namespace kernel

}  // namespace t81::simd
