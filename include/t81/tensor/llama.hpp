#pragma once
#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"
#include "t81/tensor/matmul.hpp"
#include "t81/types/T81Float.hpp"
#include "t81/types/detail/dmath.hpp"

#if defined(__AVX2__) || defined(__FMA__)
#include <immintrin.h>
#endif

namespace t81::ops {

namespace detail {

using t81::core::detail::DFixed;
using TensorFloat = t81::v1::T81Float<72, 9>;

inline float deterministic_exp(float value) {
  return static_cast<float>(t81::core::detail::exp(TensorFloat::from_double(value)).to_double());
}

inline float deterministic_inv_sqrt(float value) {
  return static_cast<float>(
      (TensorFloat::from_double(1.0) / t81::core::detail::sqrt(TensorFloat::from_double(value)))
          .to_double());
}

inline DFixed fixed_from_int64(std::int64_t value) {
  typename DFixed::Storage storage(value);
  storage <<= DFixed::kFractionalTrits;
  return DFixed(storage);
}

inline std::vector<DFixed> fixed_softmax_rows(const std::vector<DFixed>& input, int rows, int cols) {
  std::vector<DFixed> out(static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols),
                          DFixed::zero());
  for (int row = 0; row < rows; ++row) {
    const std::size_t base = static_cast<std::size_t>(row) * static_cast<std::size_t>(cols);
    DFixed max_val = input[base];
    for (int col = 1; col < cols; ++col) {
      const DFixed& candidate = input[base + static_cast<std::size_t>(col)];
      if ((candidate <=> max_val) == std::strong_ordering::greater) {
        max_val = candidate;
      }
    }

    DFixed sum = DFixed::zero();
    for (int col = 0; col < cols; ++col) {
      const auto exp_val =
          t81::core::detail::exp(input[base + static_cast<std::size_t>(col)] - max_val);
      out[base + static_cast<std::size_t>(col)] = exp_val;
      sum = sum + exp_val;
    }

    for (int col = 0; col < cols; ++col) {
      out[base + static_cast<std::size_t>(col)] =
          out[base + static_cast<std::size_t>(col)] / sum;
    }
  }
  return out;
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

inline bool all_nonnegative(const T729DynamicTensor& tensor) {
  if (!tensor.has_canonical_fixed_data()) {
    return false;
  }
  const auto& fixed = tensor.canonical_fixed_data();
  return std::all_of(fixed.begin(), fixed.end(),
                     [](const DFixed& value) { return !value.is_negative(); });
}

}  // namespace detail

#if defined(__AVX2__)
// Fast SIMD exponential approximation for AVX2.
// Approximation: exp(x) = 2^(x * log2(e)) = 2^n * 2^f
// n = round(x * log2(e)), f = x * log2(e) - n
inline __m256 simd_exp(__m256 x) {
  const __m256 log2e = _mm256_set1_ps(1.4426950408889634f);
  const __m256 ln2_hi = _mm256_set1_ps(0.6931471805599453f);

  __m256 n =
      _mm256_round_ps(_mm256_mul_ps(x, log2e), _MM_FROUND_TO_NEAREST_INT | _MM_FROUND_NO_EXC);
  __m256 f = _mm256_sub_ps(x, _mm256_mul_ps(n, ln2_hi));

  // 6th degree polynomial approximation for 2^f (actually e^f here)
  // p = 1 + f + f^2/2! + f^3/3! + f^4/4! + f^5/5! + f^6/6!
  const __m256 c1 = _mm256_set1_ps(1.0f);
  const __m256 c2 = _mm256_set1_ps(0.5f);
  const __m256 c3 = _mm256_set1_ps(0.16666666666666666f);
  const __m256 c4 = _mm256_set1_ps(0.041666666666666664f);
  const __m256 c5 = _mm256_set1_ps(0.008333333333333333f);
  const __m256 c6 = _mm256_set1_ps(0.0013888888888888889f);

  __m256 p = _mm256_fmadd_ps(f, c6, c5);
  p = _mm256_fmadd_ps(f, p, c4);
  p = _mm256_fmadd_ps(f, p, c3);
  p = _mm256_fmadd_ps(f, p, c2);
  p = _mm256_fmadd_ps(f, p, c1);
  p = _mm256_fmadd_ps(f, p, c1);

  // 2^n multiplication via integer shift
  __m256i imm0 = _mm256_cvtps_epi32(n);
  imm0 = _mm256_add_epi32(imm0, _mm256_set1_epi32(127));
  imm0 = _mm256_slli_epi32(imm0, 23);
  __m256 pow2n = _mm256_castsi256_ps(imm0);

  return _mm256_mul_ps(p, pow2n);
}
#endif

inline T729DynamicTensor rmsnorm(const T729DynamicTensor& x, const T729DynamicTensor& w,
                                 float eps = 1e-6f) {
  if (x.rank() == 0 || w.rank() != 1 || w.shape()[0] != x.shape().back()) {
    throw std::invalid_argument("rmsnorm: shape mismatch");
  }
  const TensorNumericClass result_class =
      x.strict_core_eligible() && w.strict_core_eligible()
          ? TensorNumericClass::ExactInt
          : TensorNumericClass::HostFloat;
  if (x.has_canonical_fixed_data() && w.has_canonical_fixed_data()) {
    const int dim = x.shape().back();
    const auto& input = x.canonical_fixed_data();
    const auto& weight = w.canonical_fixed_data();
    const detail::DFixed dim_fixed = detail::fixed_from_int64(dim);
    const detail::DFixed eps_fixed = detail::DFixed::from_decimal(0, 1, 6);
    std::vector<detail::DFixed> out(input.size(), detail::DFixed::zero());

    for (std::size_t base = 0; base < input.size(); base += static_cast<std::size_t>(dim)) {
      detail::DFixed sumsq = detail::DFixed::zero();
      for (int j = 0; j < dim; ++j) {
        const auto& value = input[base + static_cast<std::size_t>(j)];
        sumsq = sumsq + value * value;
      }
      const detail::DFixed mean = sumsq / dim_fixed;
      const detail::DFixed inv = detail::DFixed::one() / t81::core::detail::sqrt(mean + eps_fixed);
      for (int j = 0; j < dim; ++j) {
        out[base + static_cast<std::size_t>(j)] =
            input[base + static_cast<std::size_t>(j)] * inv * weight[static_cast<std::size_t>(j)];
      }
    }
    return T729DynamicTensor::from_canonical_fixed(x.shape(), std::move(out), result_class);
  }
  int dim = x.shape().back();
  std::vector<float> out = x.snapshot_values();
  const auto weights = w.snapshot_values();
  const float* w_ptr = weights.data();

  for (size_t i = 0; i < out.size(); i += static_cast<size_t>(dim)) {
    float* row = &out[i];
    float ss = 0.0f;
#if defined(__AVX2__) && defined(__FMA__)
    __m256 vss = _mm256_setzero_ps();
    int j = 0;
    for (; j <= dim - 8; j += 8) {
      __m256 v = _mm256_loadu_ps(&row[j]);
      vss = _mm256_fmadd_ps(v, v, vss);
    }
    // Optimized horizontal sum for ss
    __m128 vss_h = _mm_add_ps(_mm256_castps256_ps128(vss), _mm256_extractf128_ps(vss, 1));
    vss_h = _mm_add_ps(vss_h, _mm_movehl_ps(vss_h, vss_h));
    vss_h = _mm_add_ps(vss_h, _mm_shuffle_ps(vss_h, vss_h, _MM_SHUFFLE(1, 1, 1, 1)));
    ss = _mm_cvtss_f32(vss_h);

    for (; j < dim; ++j) ss += row[j] * row[j];
#else
    for (int j = 0; j < dim; ++j) ss += row[j] * row[j];
#endif
    const float mean_ss = ss / static_cast<float>(dim) + eps;
    const float inv_ss = detail::deterministic_inv_sqrt(mean_ss);

#if defined(__AVX2__)
    __m256 vinv = _mm256_set1_ps(inv_ss);
    int j_norm = 0;
    for (; j_norm <= dim - 8; j_norm += 8) {
      __m256 v = _mm256_loadu_ps(&row[j_norm]);
      __m256 vw = _mm256_loadu_ps(&w_ptr[j_norm]);
      v = _mm256_mul_ps(_mm256_mul_ps(v, vinv), vw);
      _mm256_storeu_ps(&row[j_norm], v);
    }
    for (; j_norm < dim; ++j_norm) row[j_norm] = (row[j_norm] * inv_ss) * w_ptr[j_norm];
#else
    for (int j_norm = 0; j_norm < dim; ++j_norm)
      row[j_norm] = (row[j_norm] * inv_ss) * w_ptr[j_norm];
#endif
  }
  return T729DynamicTensor(x.shape(), std::move(out));
}

inline T729DynamicTensor silu(const T729DynamicTensor& x) {
  if (x.strict_core_eligible() && x.has_canonical_fixed_data()) {
    std::vector<detail::DFixed> out;
    out.reserve(x.size());
    for (const auto& value : x.canonical_fixed_data()) {
      const auto denom = detail::DFixed::one() + t81::core::detail::exp(-value);
      out.push_back(value / denom);
    }
    const auto result_class =
        x.strict_core_eligible() ? TensorNumericClass::ExactInt : TensorNumericClass::HostFloat;
    return T729DynamicTensor::from_canonical_fixed(x.shape(), std::move(out), result_class);
  }
  std::vector<float> out = x.snapshot_values();
  float* data = out.data();
  size_t size = out.size();

  size_t i = 0;
#if defined(__AVX2__) && !defined(T81_DETERMINISTIC)
  __m256 vone = _mm256_set1_ps(1.0f);
  for (; i + 8 <= size; i += 8) {
    __m256 vx = _mm256_loadu_ps(&data[i]);
    __m256 vnegx = _mm256_sub_ps(_mm256_setzero_ps(), vx);
    __m256 vexp = simd_exp(vnegx);
    __m256 vres = _mm256_div_ps(vx, _mm256_add_ps(vone, vexp));
    _mm256_storeu_ps(&data[i], vres);
  }
#endif
  for (; i < size; ++i) {
    data[i] = data[i] / (1.0f + detail::deterministic_exp(-data[i]));
  }
  return T729DynamicTensor(x.shape(), std::move(out));
}

inline T729DynamicTensor softmax(const T729DynamicTensor& x) {
  if (x.rank() == 0) throw std::invalid_argument("softmax: rank 0");
  if (x.strict_core_eligible() && x.has_canonical_fixed_data()) {
    const int dim = x.shape().back();
    const int rows = static_cast<int>(x.size() / static_cast<std::size_t>(dim));
    auto out = detail::fixed_softmax_rows(x.canonical_fixed_data(), rows, dim);
    const auto result_class =
        x.strict_core_eligible() ? TensorNumericClass::ExactInt : TensorNumericClass::HostFloat;
    return T729DynamicTensor::from_canonical_fixed(x.shape(), std::move(out), result_class);
  }
  int dim = x.shape().back();
  std::vector<float> out = x.snapshot_values();
  for (size_t i = 0; i < out.size(); i += static_cast<size_t>(dim)) {
    float* row = &out[i];
    float max_val = row[0];
#if defined(__AVX2__)
    __m256 vmax = _mm256_set1_ps(max_val);
    int j_max = 0;
    for (; j_max + 8 <= dim; j_max += 8) {
      __m256 v = _mm256_loadu_ps(&row[j_max]);
      vmax = _mm256_max_ps(vmax, v);
    }
    alignas(32) float tmp_max[8];
    _mm256_store_ps(tmp_max, vmax);
    for (int k = 0; k < 8; ++k)
      if (tmp_max[k] > max_val) max_val = tmp_max[k];
    for (; j_max < dim; ++j_max)
      if (row[j_max] > max_val) max_val = row[j_max];
#else
    for (int j = 1; j < dim; ++j)
      if (row[j] > max_val) max_val = row[j];
#endif

    float sum = 0.0f;
#if defined(__AVX2__) && !defined(T81_DETERMINISTIC)
    __m256 vsum = _mm256_setzero_ps();
    const __m256 vmax_v = _mm256_set1_ps(max_val);
    int j_sum = 0;
    for (; j_sum + 8 <= dim; j_sum += 8) {
      __m256 vx = _mm256_loadu_ps(&row[j_sum]);
      __m256 vexp = simd_exp(_mm256_sub_ps(vx, vmax_v));
      _mm256_storeu_ps(&row[j_sum], vexp);
      vsum = _mm256_add_ps(vsum, vexp);
    }
    // Horizontal sum of vsum
    __m128 vsum_h = _mm_add_ps(_mm256_castps256_ps128(vsum), _mm256_extractf128_ps(vsum, 1));
    vsum_h = _mm_add_ps(vsum_h, _mm_movehl_ps(vsum_h, vsum_h));
    vsum_h = _mm_add_ps(vsum_h, _mm_shuffle_ps(vsum_h, vsum_h, _MM_SHUFFLE(1, 1, 1, 1)));
    sum = _mm_cvtss_f32(vsum_h);

    for (; j_sum < dim; ++j_sum) {
      row[j_sum] = detail::deterministic_exp(row[j_sum] - max_val);
      sum += row[j_sum];
    }
#else
    for (int j = 0; j < dim; ++j) {
      row[j] = detail::deterministic_exp(row[j] - max_val);
      sum += row[j];
    }
#endif

    float inv_sum = 1.0f / sum;
#if defined(__AVX2__)
    __m256 vinv = _mm256_set1_ps(inv_sum);
    int j_norm = 0;
    for (; j_norm + 8 <= dim; j_norm += 8) {
      __m256 v = _mm256_loadu_ps(&row[j_norm]);
      v = _mm256_mul_ps(v, vinv);
      _mm256_storeu_ps(&row[j_norm], v);
    }
    for (; j_norm < dim; ++j_norm) row[j_norm] *= inv_sum;
#else
    for (int j_norm = 0; j_norm < dim; ++j_norm) row[j_norm] *= inv_sum;
#endif
  }
  return T729DynamicTensor(x.shape(), std::move(out));
}

inline T729DynamicTensor attention(const T729DynamicTensor& q, const T729DynamicTensor& k,
                                   const T729DynamicTensor& v) {
  if (q.rank() != 2 || k.rank() != 2 || v.rank() != 2) {
    throw std::invalid_argument("attention: all inputs must be rank-2");
  }
  if (q.shape()[1] != k.shape()[1] || k.shape()[0] != v.shape()[0]) {
    throw std::invalid_argument("attention: shape mismatch");
  }

  const int q_rows = q.shape()[0];
  const int dk = q.shape()[1];
  const int k_rows = k.shape()[0];
  const int v_cols = v.shape()[1];
  const TensorNumericClass result_class =
      q.strict_core_eligible() && k.strict_core_eligible() && v.strict_core_eligible()
          ? TensorNumericClass::ExactInt
          : TensorNumericClass::HostFloat;

  if (result_class != TensorNumericClass::HostFloat &&
      q.has_canonical_fixed_data() && k.has_canonical_fixed_data() && v.has_canonical_fixed_data()) {
    std::vector<detail::DFixed> k_transposed(static_cast<std::size_t>(dk) *
                                                 static_cast<std::size_t>(k_rows),
                                             detail::DFixed::zero());
    const auto& q_fixed = q.canonical_fixed_data();
    const auto& k_fixed = k.canonical_fixed_data();
    const auto& v_fixed = v.canonical_fixed_data();
    for (int r = 0; r < k_rows; ++r) {
      for (int c = 0; c < dk; ++c) {
        k_transposed[static_cast<std::size_t>(c) * static_cast<std::size_t>(k_rows) +
                     static_cast<std::size_t>(r)] =
            k_fixed[static_cast<std::size_t>(r) * static_cast<std::size_t>(dk) +
                    static_cast<std::size_t>(c)];
      }
    }

    auto scores = detail::fixed_matmul(q_fixed, k_transposed, q_rows, dk, k_rows);
    const detail::DFixed inv_scale =
        detail::DFixed::one() / t81::core::detail::sqrt(detail::fixed_from_int64(dk));
    for (auto& score : scores) {
      score = score * inv_scale;
    }
    auto probs = detail::fixed_softmax_rows(scores, q_rows, k_rows);
    auto out = detail::fixed_matmul(probs, v_fixed, q_rows, k_rows, v_cols);
    return T729DynamicTensor::from_canonical_fixed({q_rows, v_cols}, std::move(out), result_class);
  }

  auto k_t = k.transpose2d();
  auto scores = t81::ops::matmul(q, k_t);
  const float inv_scale = detail::deterministic_inv_sqrt(static_cast<float>(dk));
  auto scaled = scores.snapshot_values();
  for (auto& x : scaled) {
    x *= inv_scale;
  }
  auto probs = t81::ops::softmax(T729DynamicTensor(scores.shape(), std::move(scaled)));
  return t81::ops::matmul(probs, v);
}

inline T729DynamicTensor embed(const T729DynamicTensor& table, std::int64_t index) {
  if (table.rank() != 2) {
    throw std::invalid_argument("embed: table must be rank-2");
  }
  if (index < 0 || index >= static_cast<std::int64_t>(table.shape()[0])) {
    throw std::out_of_range("embed: index out of range");
  }

  const int dim = table.shape()[1];
  const TensorNumericClass result_class =
      table.strict_core_eligible() ? table.numeric_class() : TensorNumericClass::HostFloat;
  const std::size_t base = static_cast<std::size_t>(index) * static_cast<std::size_t>(dim);

  if (table.has_canonical_fixed_data()) {
    std::vector<detail::DFixed> out(static_cast<std::size_t>(dim));
    const auto& fixed = table.canonical_fixed_data();
    for (int i = 0; i < dim; ++i) {
      out[static_cast<std::size_t>(i)] = fixed[base + static_cast<std::size_t>(i)];
    }
    return T729DynamicTensor::from_canonical_fixed({dim}, std::move(out), result_class);
  }

  const auto values = table.snapshot_values();
  std::vector<float> out(static_cast<std::size_t>(dim));
  for (int i = 0; i < dim; ++i) {
    out[static_cast<std::size_t>(i)] = values[base + static_cast<std::size_t>(i)];
  }
  auto result = T729DynamicTensor({dim}, std::move(out));
  result.set_numeric_class(result_class);
  return result;
}

inline T729DynamicTensor rope(const T729DynamicTensor& x, int pos) {
  if (x.rank() < 2) throw std::invalid_argument("rope: rank must be at least 2");
  if (x.has_canonical_fixed_data()) {
    const int head_dim = x.shape().back();
    if ((head_dim % 2) == 0) {
      const detail::DFixed freq_base = detail::DFixed::from_decimal(10000, 0, 0);
      const detail::DFixed pos_fixed = detail::fixed_from_int64(pos);
      const detail::DFixed head_dim_fixed = detail::fixed_from_int64(head_dim);
      const auto& input = x.canonical_fixed_data();
      std::vector<detail::DFixed> out = input;
      std::vector<detail::DFixed> cos_terms(static_cast<std::size_t>(head_dim / 2));
      std::vector<detail::DFixed> sin_terms(static_cast<std::size_t>(head_dim / 2));

      for (int j = 0; j < head_dim; j += 2) {
        const detail::DFixed exponent = detail::fixed_from_int64(j) / head_dim_fixed;
        const detail::DFixed freq =
            detail::DFixed::one() / t81::core::detail::pow(freq_base, exponent);
        const detail::DFixed angle = pos_fixed * freq;
        cos_terms[static_cast<std::size_t>(j / 2)] = t81::core::detail::cos(angle);
        sin_terms[static_cast<std::size_t>(j / 2)] = t81::core::detail::sin(angle);
      }

      for (std::size_t base = 0; base < input.size(); base += static_cast<std::size_t>(head_dim)) {
        for (int j = 0; j < head_dim; j += 2) {
          const detail::DFixed& f_cos = cos_terms[static_cast<std::size_t>(j / 2)];
          const detail::DFixed& f_sin = sin_terms[static_cast<std::size_t>(j / 2)];
          const detail::DFixed v0 = input[base + static_cast<std::size_t>(j)];
          const detail::DFixed v1 = input[base + static_cast<std::size_t>(j + 1)];
          out[base + static_cast<std::size_t>(j)] = v0 * f_cos - v1 * f_sin;
          out[base + static_cast<std::size_t>(j + 1)] = v0 * f_sin + v1 * f_cos;
        }
      }
      const auto result_class =
          x.strict_core_eligible() ? TensorNumericClass::ExactInt : TensorNumericClass::HostFloat;
      return T729DynamicTensor::from_canonical_fixed(x.shape(), std::move(out), result_class);
    }
  }
  int head_dim = x.shape().back();
  std::vector<float> data = x.snapshot_values();
  const detail::TensorFloat freq_base = detail::TensorFloat::from_double(10000.0);
  const detail::TensorFloat pos_float = detail::TensorFloat::from_double(static_cast<double>(pos));
  const detail::TensorFloat head_dim_float = detail::TensorFloat::from_double(static_cast<double>(head_dim));
  std::vector<float> cos_terms(static_cast<std::size_t>(head_dim / 2));
  std::vector<float> sin_terms(static_cast<std::size_t>(head_dim / 2));
  for (int j = 0; j < head_dim; j += 2) {
    const detail::TensorFloat exponent =
        detail::TensorFloat::from_double(static_cast<double>(j)) / head_dim_float;
    const detail::TensorFloat freq =
        detail::TensorFloat::from_double(1.0) / t81::core::detail::pow(freq_base, exponent);
    const detail::TensorFloat angle = pos_float * freq;
    cos_terms[static_cast<std::size_t>(j / 2)] =
        static_cast<float>(t81::core::detail::cos(angle).to_double());
    sin_terms[static_cast<std::size_t>(j / 2)] =
        static_cast<float>(t81::core::detail::sin(angle).to_double());
  }
  for (size_t i = 0; i < data.size(); i += static_cast<size_t>(head_dim)) {
    for (int j = 0; j < head_dim; j += 2) {
      const float f_cos = cos_terms[static_cast<std::size_t>(j / 2)];
      const float f_sin = sin_terms[static_cast<std::size_t>(j / 2)];
      float v0 = data[i + static_cast<size_t>(j)];
      float v1 = data[i + static_cast<size_t>(j + 1)];
      data[i + static_cast<size_t>(j)] = v0 * f_cos - v1 * f_sin;
      data[i + static_cast<size_t>(j + 1)] = v0 * f_sin + v1 * f_cos;
    }
  }
  return T729DynamicTensor(x.shape(), std::move(data));
}

// RFC-0026: WLOAD — policy-gate copy of a weight tensor.
// Phase-1: verifies src is non-empty and returns an independent copy.
// Full CanonFS integration is deferred to AI-M4.
inline T729DynamicTensor wload(const T729DynamicTensor& src) {
  if (src.rank() < 1) {
    throw std::invalid_argument("wload: source tensor must be at least rank-1");
  }
  if (src.has_canonical_fixed_data()) {
    return T729DynamicTensor::from_canonical_fixed(src.shape(),
                                                   std::vector<detail::DFixed>(src.canonical_fixed_data()),
                                                   src.numeric_class());
  }
  return T729DynamicTensor(src.shape(), src.snapshot_values());
}

// RFC-0026: GATHER — gathers a slice from a rank-2 tensor at index along axis.
// AI-M5: axis parameter active (0 = gather row, 1 = gather column).
// Deterministic on canonical fixed-point data.
inline T729DynamicTensor gather(const T729DynamicTensor& src, std::int64_t index, int axis = 0) {
  if (src.rank() != 2) {
    throw std::invalid_argument("gather: source must be rank-2 (phase-1)");
  }
  if (axis < 0 || axis >= static_cast<int>(src.rank())) {
    throw std::out_of_range("gather: axis out of range");
  }
  if (index < 0 ||
      index >= static_cast<std::int64_t>(src.shape()[static_cast<std::size_t>(axis)])) {
    throw std::out_of_range("gather: index out of range for axis");
  }
  const int rows = src.shape()[0];
  const int cols = src.shape()[1];
  const TensorNumericClass result_class =
      src.strict_core_eligible() ? src.numeric_class() : TensorNumericClass::HostFloat;

  if (axis == 0) {
    // Gather row `index` → output shape [cols].
    const std::size_t base = static_cast<std::size_t>(index) * static_cast<std::size_t>(cols);
    if (src.has_canonical_fixed_data()) {
      const auto& fixed = src.canonical_fixed_data();
      std::vector<detail::DFixed> out(static_cast<std::size_t>(cols));
      for (int i = 0; i < cols; ++i) {
        out[static_cast<std::size_t>(i)] = fixed[base + static_cast<std::size_t>(i)];
      }
      return T729DynamicTensor::from_canonical_fixed({cols}, std::move(out), result_class);
    }
    const auto values = src.snapshot_values();
    std::vector<float> out(static_cast<std::size_t>(cols));
    for (int i = 0; i < cols; ++i) {
      out[static_cast<std::size_t>(i)] = values[base + static_cast<std::size_t>(i)];
    }
    auto result = T729DynamicTensor({cols}, std::move(out));
    result.set_numeric_class(result_class);
    return result;
  } else {
    // axis == 1: gather column `index` → output shape [rows].
    // out[r] = src[r, index] = data[r * cols + index]
    if (src.has_canonical_fixed_data()) {
      const auto& fixed = src.canonical_fixed_data();
      std::vector<detail::DFixed> out(static_cast<std::size_t>(rows));
      for (int r = 0; r < rows; ++r) {
        out[static_cast<std::size_t>(r)] =
            fixed[static_cast<std::size_t>(r) * static_cast<std::size_t>(cols) +
                  static_cast<std::size_t>(index)];
      }
      return T729DynamicTensor::from_canonical_fixed({rows}, std::move(out), result_class);
    }
    const auto values = src.snapshot_values();
    std::vector<float> out(static_cast<std::size_t>(rows));
    for (int r = 0; r < rows; ++r) {
      out[static_cast<std::size_t>(r)] =
          values[static_cast<std::size_t>(r) * static_cast<std::size_t>(cols) +
                 static_cast<std::size_t>(index)];
    }
    auto result = T729DynamicTensor({rows}, std::move(out));
    result.set_numeric_class(result_class);
    return result;
  }
}

// RFC-0026: SCATTER — scatter-adds src (rank-1) into dst (rank-2) at index along axis.
// Returns a new tensor; dst is not mutated.
// AI-M5: axis parameter active (0 = scatter into row, 1 = scatter into column).
// Aliasing detection is enforced at the VM level by the SCATTER handler (AI-M5).
inline T729DynamicTensor scatter_add(const T729DynamicTensor& dst, std::int64_t index,
                                     const T729DynamicTensor& src, int axis = 0) {
  if (dst.rank() != 2 || src.rank() != 1) {
    throw std::invalid_argument("scatter_add: dst must be rank-2, src must be rank-1 (phase-1)");
  }
  if (axis < 0 || axis >= static_cast<int>(dst.rank())) {
    throw std::out_of_range("scatter_add: axis out of range");
  }
  const int rows = dst.shape()[0];
  const int cols = dst.shape()[1];
  // src length must match the dimension perpendicular to the scatter axis.
  const int expected_src_len = dst.shape()[static_cast<std::size_t>(1 - axis)];
  if (src.shape()[0] != expected_src_len) {
    throw std::invalid_argument("scatter_add: src length must match dst slice dimension");
  }
  if (index < 0 ||
      index >= static_cast<std::int64_t>(dst.shape()[static_cast<std::size_t>(axis)])) {
    throw std::out_of_range("scatter_add: index out of range for axis");
  }

  if (axis == 0) {
    // Scatter-add src into row `index`.
    const std::size_t base = static_cast<std::size_t>(index) * static_cast<std::size_t>(cols);
    if (dst.has_canonical_fixed_data() && src.has_canonical_fixed_data()) {
      std::vector<detail::DFixed> out(dst.canonical_fixed_data());
      const auto& src_fixed = src.canonical_fixed_data();
      for (int i = 0; i < cols; ++i) {
        out[base + static_cast<std::size_t>(i)] =
            out[base + static_cast<std::size_t>(i)] + src_fixed[static_cast<std::size_t>(i)];
      }
      return T729DynamicTensor::from_canonical_fixed({rows, cols}, std::move(out),
                                                     dst.numeric_class());
    }
    std::vector<float> out = dst.snapshot_values();
    const auto src_vals = src.snapshot_values();
    for (int i = 0; i < cols; ++i) {
      out[base + static_cast<std::size_t>(i)] += src_vals[static_cast<std::size_t>(i)];
    }
    return T729DynamicTensor({rows, cols}, std::move(out));
  } else {
    // axis == 1: scatter-add src (len=rows) into column `index`.
    // dst_new[r, index] += src[r] for all r.
    if (dst.has_canonical_fixed_data() && src.has_canonical_fixed_data()) {
      std::vector<detail::DFixed> out(dst.canonical_fixed_data());
      const auto& src_fixed = src.canonical_fixed_data();
      for (int r = 0; r < rows; ++r) {
        const std::size_t pos = static_cast<std::size_t>(r) * static_cast<std::size_t>(cols) +
                                static_cast<std::size_t>(index);
        out[pos] = out[pos] + src_fixed[static_cast<std::size_t>(r)];
      }
      return T729DynamicTensor::from_canonical_fixed({rows, cols}, std::move(out),
                                                     dst.numeric_class());
    }
    std::vector<float> out = dst.snapshot_values();
    const auto src_vals = src.snapshot_values();
    for (int r = 0; r < rows; ++r) {
      out[static_cast<std::size_t>(r) * static_cast<std::size_t>(cols) +
          static_cast<std::size_t>(index)] += src_vals[static_cast<std::size_t>(r)];
    }
    return T729DynamicTensor({rows, cols}, std::move(out));
  }
}

}  // namespace t81::ops
