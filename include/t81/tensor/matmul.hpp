#pragma once
#include <cmath>
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"
#include "t81/types/detail/dmath.hpp"

#if defined(__AVX2__)
#include <immintrin.h>
#endif

namespace t81::ops {

namespace matmul_detail {

using t81::core::detail::DFixed;

inline TensorNumericClass matmul_result_class(const T729DynamicTensor& lhs,
                                              const T729DynamicTensor& rhs) {
  if (!lhs.strict_core_eligible() || !rhs.strict_core_eligible()) {
    return TensorNumericClass::HostFloat;
  }
  return TensorNumericClass::ExactInt;
}

inline std::vector<DFixed> fixed_matmul(const std::vector<DFixed>& lhs,
                                        const std::vector<DFixed>& rhs, int m, int k, int n) {
  std::vector<DFixed> out(static_cast<std::size_t>(m) * static_cast<std::size_t>(n), DFixed::zero());
  for (int i = 0; i < m; ++i) {
    for (int p = 0; p < k; ++p) {
      const DFixed& av = lhs[static_cast<std::size_t>(i) * static_cast<std::size_t>(k) +
                             static_cast<std::size_t>(p)];
      if (av.is_zero()) {
        continue;
      }
      for (int j = 0; j < n; ++j) {
        out[static_cast<std::size_t>(i) * static_cast<std::size_t>(n) + static_cast<std::size_t>(j)] =
            out[static_cast<std::size_t>(i) * static_cast<std::size_t>(n) + static_cast<std::size_t>(j)] +
            av * rhs[static_cast<std::size_t>(p) * static_cast<std::size_t>(n) +
                     static_cast<std::size_t>(j)];
      }
    }
  }
  return out;
}

}  // namespace matmul_detail

// Optimized matrix multiply: (m×k) · (k×n) → (m×n)
inline T729DynamicTensor matmul(const T729DynamicTensor& A, const T729DynamicTensor& B) {
  if (A.rank() != 2 || B.rank() != 2)
    throw std::invalid_argument("matmul: both inputs must be rank-2");
  const int m = A.shape()[0], kA = A.shape()[1];
  const int kB = B.shape()[0], n = B.shape()[1];
  if (kA != kB) throw std::invalid_argument("matmul: inner dimensions mismatch");
  const TensorNumericClass result_class = matmul_detail::matmul_result_class(A, B);

  if (A.has_canonical_fixed_data() && B.has_canonical_fixed_data() &&
      A.strict_core_eligible() && B.strict_core_eligible()) {
    auto out = matmul_detail::fixed_matmul(A.canonical_fixed_data(), B.canonical_fixed_data(), m, kA, n);
    return T729DynamicTensor::from_canonical_fixed({m, n}, std::move(out), result_class);
  }

  const auto a = A.snapshot_values();
  const auto b = B.snapshot_values();
  std::vector<float> c(static_cast<size_t>(m) * static_cast<size_t>(n), 0.0f);

  for (int i = 0; i < m; ++i) {
    const size_t c_row = static_cast<size_t>(i) * n;
    for (int p = 0; p < kA; ++p) {
      const float av = a[static_cast<size_t>(i) * kA + static_cast<size_t>(p)];
      if (av == 0.0f) continue;
      const size_t b_row = static_cast<size_t>(p) * n;

#if defined(__AVX2__)
      const __m256 va = _mm256_set1_ps(av);
      int j = 0;
      // Unroll by 4 for better throughput
      for (; j <= n - 32; j += 32) {
        _mm256_storeu_ps(&c[c_row + j + 0], _mm256_fmadd_ps(va, _mm256_loadu_ps(&b[b_row + j + 0]),
                                                            _mm256_loadu_ps(&c[c_row + j + 0])));
        _mm256_storeu_ps(&c[c_row + j + 8], _mm256_fmadd_ps(va, _mm256_loadu_ps(&b[b_row + j + 8]),
                                                            _mm256_loadu_ps(&c[c_row + j + 8])));
        _mm256_storeu_ps(&c[c_row + j + 16],
                         _mm256_fmadd_ps(va, _mm256_loadu_ps(&b[b_row + j + 16]),
                                         _mm256_loadu_ps(&c[c_row + j + 16])));
        _mm256_storeu_ps(&c[c_row + j + 24],
                         _mm256_fmadd_ps(va, _mm256_loadu_ps(&b[b_row + j + 24]),
                                         _mm256_loadu_ps(&c[c_row + j + 24])));
      }
      for (; j <= n - 8; j += 8) {
        __m256 vb = _mm256_loadu_ps(&b[b_row + j]);
        __m256 vc = _mm256_loadu_ps(&c[c_row + j]);
        vc = _mm256_fmadd_ps(va, vb, vc);
        _mm256_storeu_ps(&c[c_row + j], vc);
      }
      for (; j < n; ++j) {
        c[c_row + j] += av * b[b_row + j];
      }
#else
      for (int j = 0; j < n; ++j) {
        c[c_row + j] += av * b[b_row + j];
      }
#endif
    }
  }

  auto result = T729DynamicTensor({m, n}, std::move(c));
  result.set_numeric_class(result_class);
  return result;
}

inline T729DynamicTensor qmatmul(const T729DynamicTensor& activations, const T729DynamicTensor& weights,
                                 float scale) {
  auto dequantized = weights.snapshot_values();
  for (auto& value : dequantized) {
    value *= scale;
  }
  T729DynamicTensor dequantized_tensor(weights.shape(), std::move(dequantized));
  return matmul(activations, dequantized_tensor);
}

}  // namespace t81::ops
