#pragma once
#include <cmath>
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"
#include "t81/types/T81Float.hpp"
#include "t81/types/detail/dmath.hpp"

#if defined(__AVX2__)
#include <immintrin.h>
#endif

namespace t81::ops {

namespace matmul_detail {

using t81::core::detail::DFixed;
using TensorFloat = t81::v1::T81Float<72, 9>;

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

inline std::vector<DFixed> fixed_scale(const std::vector<DFixed>& input, const DFixed& scale) {
  std::vector<DFixed> out;
  out.reserve(input.size());
  for (const auto& value : input) {
    out.push_back(value * scale);
  }
  return out;
}

inline float host_float_from_fixed(const DFixed& value) {
  if (value.is_zero()) {
    return 0.0F;
  }
  const auto& raw = value.v;
  float result = 0.0F;
  for (std::size_t i = DFixed::Storage::kNumTrits; i-- > DFixed::kFractionalTrits;) {
    result = result * 3.0F + static_cast<float>(trit_to_int(raw[i]));
  }

  float factor = 1.0F / 3.0F;
  for (std::size_t i = DFixed::kFractionalTrits; i-- > 0;) {
    result += static_cast<float>(trit_to_int(raw[i])) * factor;
    factor /= 3.0F;
  }
  return result;
}

inline float deterministic_mul(float lhs, float rhs) {
  return static_cast<float>(
      (TensorFloat::from_double(lhs) * TensorFloat::from_double(rhs)).to_double());
}

inline float deterministic_fma(float acc, float lhs, float rhs) {
  return static_cast<float>(
      (TensorFloat::from_double(acc) +
       TensorFloat::from_double(lhs) * TensorFloat::from_double(rhs))
          .to_double());
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

  if (A.strict_core_eligible() && B.strict_core_eligible() &&
      A.has_canonical_fixed_data() && B.has_canonical_fixed_data()) {
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

#if defined(__AVX2__) && !defined(T81_DETERMINISTIC)
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
      // When result_class is HostFloat, at least one operand is already a
      // float approximation (e.g. the output of a native unary fast path such
      // as TExp).  Using deterministic_fma here would convert each value to
      // T81Float<72,9> and back for no semantic benefit — the result is
      // HostFloat regardless.  Plain IEEE multiply is correct and fast.
      if (result_class == TensorNumericClass::HostFloat) {
        for (int j = 0; j < n; ++j) {
          c[c_row + j] += av * b[b_row + j];
        }
      } else {
        for (int j = 0; j < n; ++j) {
          c[c_row + j] = matmul_detail::deterministic_fma(c[c_row + j], av, b[b_row + j]);
        }
      }
#endif
    }
  }

  // Use from_host_float_data to skip the eager canonical-fixed cache build
  // that the two-argument constructor would trigger.  The cache is lazily
  // built on demand if a downstream op needs it (ExactInt/ExactTrit results),
  // and never built for HostFloat results (strict_core_eligible() is false).
  return T729DynamicTensor::from_host_float_data({m, n}, std::move(c), result_class);
}

inline T729DynamicTensor qmatmul(const T729DynamicTensor& activations, const T729DynamicTensor& weights,
                                 float scale) {
  auto dequantized = weights.snapshot_values();
  for (auto& value : dequantized) {
    value = matmul_detail::deterministic_mul(value, scale);
  }
  T729DynamicTensor dequantized_tensor(weights.shape(), std::move(dequantized));
  return matmul(activations, dequantized_tensor);
}

inline T729DynamicTensor qmatmul(const T729DynamicTensor& activations, const T729DynamicTensor& weights,
                                 const t81::core::detail::DFixed& scale) {
  if (activations.strict_core_eligible() && weights.strict_core_eligible() &&
      activations.has_canonical_fixed_data() && weights.has_canonical_fixed_data()) {
    auto dequantized = matmul_detail::fixed_scale(weights.canonical_fixed_data(), scale);
    auto dequantized_tensor =
        T729DynamicTensor::from_canonical_fixed(weights.shape(), std::move(dequantized),
                                                weights.numeric_class());
    return matmul(activations, dequantized_tensor);
  }

  return qmatmul(activations, weights, matmul_detail::host_float_from_fixed(scale));
}

}  // namespace t81::ops
