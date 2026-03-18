#pragma once

#include <cstdint>
#include <cstring>
#include <vector>

#include "t81/types/Result.hpp"

// We use ComputeTritVector as the underlying data container per the RFC
#include "t81/experimental/packed_trit_vector.hpp"

namespace t81::swar {

using ComputeTritVector = t81::experimental::ComputeTritVector;

// Forward declarations for kernel functions
namespace kernel {
void t_not(const uint8_t* src, uint8_t* dst, size_t len);
void t_and(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, size_t len);
void t_or(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, size_t len);
}  // namespace kernel

// Primary operations
Result<ComputeTritVector> t_not(const ComputeTritVector& input);
Result<ComputeTritVector> t_and(const ComputeTritVector& a, const ComputeTritVector& b);
Result<ComputeTritVector> t_or(const ComputeTritVector& a, const ComputeTritVector& b);

// In-place variants for zero-allocation scenarios
Result<bool> t_not_inplace(ComputeTritVector& input);
Result<bool> t_and_inplace(ComputeTritVector& a, const ComputeTritVector& b);
Result<bool> t_or_inplace(ComputeTritVector& a, const ComputeTritVector& b);

// Explicit SWAR selection (for testing/benchmarking)
Result<ComputeTritVector> t_not_swar(const ComputeTritVector& input);
Result<ComputeTritVector> t_and_swar(const ComputeTritVector& a, const ComputeTritVector& b);
Result<ComputeTritVector> t_or_swar(const ComputeTritVector& a, const ComputeTritVector& b);

}  // namespace t81::swar

namespace t81::swar::kernel {

inline void t_not(const uint8_t* src, uint8_t* dst, size_t n) {
  size_t i = 0;
  for (; i + 8 <= n; i += 8) {
    uint64_t x;
    std::memcpy(&x, src + i, 8);
    uint64_t low = x & 0x5555555555555555ULL;
    uint64_t res = x ^ (low << 1);
    std::memcpy(dst + i, &res, 8);
  }
  for (; i < n; ++i) {
    uint8_t x = src[i];
    uint8_t low = x & 0x55;
    dst[i] = x ^ (low << 1);
  }
}

inline void t_and(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, size_t n) {
  size_t i = 0;
  for (; i + 8 <= n; i += 8) {
    uint64_t a, b;
    std::memcpy(&a, src_a + i, 8);
    std::memcpy(&b, src_b + i, 8);

    uint64_t H = (a | b) & 0xAAAAAAAAAAAAAAAAULL;
    uint64_t L_content = (a & b) & 0x5555555555555555ULL;
    uint64_t res = H | (H >> 1) | L_content;

    std::memcpy(dst + i, &res, 8);
  }
  for (; i < n; ++i) {
    uint8_t a = src_a[i];
    uint8_t b = src_b[i];

    uint8_t H = (a | b) & 0xAA;
    uint8_t L_content = (a & b) & 0x55;
    dst[i] = H | (H >> 1) | L_content;
  }
}

inline void t_or(const uint8_t* src_a, const uint8_t* src_b, uint8_t* dst, size_t n) {
  size_t i = 0;
  for (; i + 8 <= n; i += 8) {
    uint64_t a, b;
    std::memcpy(&a, src_a + i, 8);
    std::memcpy(&b, src_b + i, 8);

    uint64_t h_a = a & 0xAAAAAAAAAAAAAAAAULL;
    uint64_t h_b = b & 0xAAAAAAAAAAAAAAAAULL;
    uint64_t l_a = a & 0x5555555555555555ULL;
    uint64_t l_b = b & 0x5555555555555555ULL;

    uint64_t H = h_a & h_b;
    uint64_t mask = (h_a | h_b) >> 1;
    uint64_t L = (l_a & l_b) | ((l_a | l_b) & ~mask);
    uint64_t res = H | (H >> 1) | L;

    std::memcpy(dst + i, &res, 8);
  }
  for (; i < n; ++i) {
    uint8_t a = src_a[i];
    uint8_t b = src_b[i];

    uint8_t h_a = a & 0xAA;
    uint8_t h_b = b & 0xAA;
    uint8_t l_a = a & 0x55;
    uint8_t l_b = b & 0x55;

    uint8_t H = h_a & h_b;
    uint8_t mask = (h_a | h_b) >> 1;
    uint8_t L = (l_a & l_b) | ((l_a | l_b) & ~mask);
    dst[i] = H | (H >> 1) | L;
  }
}

}  // namespace t81::swar::kernel

namespace t81::swar {

inline Result<bool> t_not_inplace(ComputeTritVector& input) {
  kernel::t_not(input.data().data(), input.data_mut().data(), input.data().size());
  if (input.size() % 4 != 0 && !input.data().empty()) {
    input.data_mut().back() &= (1 << ((input.size() % 4) * 2)) - 1;
  }
  return Result<bool>::success(true);
}

inline Result<bool> t_and_inplace(ComputeTritVector& a, const ComputeTritVector& b) {
  if (a.size() != b.size()) {
    return Result<bool>::failure(T81Symbol::intern("SIZE_MISMATCH"), T81String("Size mismatch"),
                                 T81Symbol::intern("SWAR"));
  }
  kernel::t_and(a.data().data(), b.data().data(), a.data_mut().data(), a.data().size());
  if (a.size() % 4 != 0 && !a.data().empty()) {
    a.data_mut().back() &= (1 << ((a.size() % 4) * 2)) - 1;
  }
  return Result<bool>::success(true);
}

inline Result<bool> t_or_inplace(ComputeTritVector& a, const ComputeTritVector& b) {
  if (a.size() != b.size()) {
    return Result<bool>::failure(T81Symbol::intern("SIZE_MISMATCH"), T81String("Size mismatch"),
                                 T81Symbol::intern("SWAR"));
  }
  kernel::t_or(a.data().data(), b.data().data(), a.data_mut().data(), a.data().size());
  if (a.size() % 4 != 0 && !a.data().empty()) {
    a.data_mut().back() &= (1 << ((a.size() % 4) * 2)) - 1;
  }
  return Result<bool>::success(true);
}

inline Result<ComputeTritVector> t_not_swar(const ComputeTritVector& input) {
  ComputeTritVector res = input;  // Copy
  if (auto r = t_not_inplace(res); r.is_err()) {
    return Result<ComputeTritVector>::failure(r.error().code, r.error().message, r.error().source);
  }
  return Result<ComputeTritVector>::success(std::move(res));
}

inline Result<ComputeTritVector> t_and_swar(const ComputeTritVector& a,
                                            const ComputeTritVector& b) {
  ComputeTritVector res = a;  // Copy
  if (auto r = t_and_inplace(res, b); r.is_err()) {
    return Result<ComputeTritVector>::failure(r.error().code, r.error().message, r.error().source);
  }
  return Result<ComputeTritVector>::success(std::move(res));
}

inline Result<ComputeTritVector> t_or_swar(const ComputeTritVector& a, const ComputeTritVector& b) {
  ComputeTritVector res = a;  // Copy
  if (auto r = t_or_inplace(res, b); r.is_err()) {
    return Result<ComputeTritVector>::failure(r.error().code, r.error().message, r.error().source);
  }
  return Result<ComputeTritVector>::success(std::move(res));
}

inline Result<ComputeTritVector> t_not(const ComputeTritVector& input) {
  ComputeTritVector res = input;
  auto r = t_not_inplace(res);
  if (r.is_err())
    return Result<ComputeTritVector>::failure(r.error().code, r.error().message, r.error().source);
  return Result<ComputeTritVector>::success(std::move(res));
}

inline Result<ComputeTritVector> t_and(const ComputeTritVector& a, const ComputeTritVector& b) {
  ComputeTritVector res = a;
  auto r = t_and_inplace(res, b);
  if (r.is_err())
    return Result<ComputeTritVector>::failure(r.error().code, r.error().message, r.error().source);
  return Result<ComputeTritVector>::success(std::move(res));
}

inline Result<ComputeTritVector> t_or(const ComputeTritVector& a, const ComputeTritVector& b) {
  ComputeTritVector res = a;
  auto r = t_or_inplace(res, b);
  if (r.is_err())
    return Result<ComputeTritVector>::failure(r.error().code, r.error().message, r.error().source);
  return Result<ComputeTritVector>::success(std::move(res));
}

}  // namespace t81::swar
