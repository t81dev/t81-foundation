#pragma once
#include <limits>
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"
#include "t81/types/detail/dmath.hpp"

namespace t81::ops {

namespace reduce_detail {

using t81::core::detail::DFixed;

inline TensorNumericClass contract_result_class(const T729DynamicTensor& lhs,
                                                const T729DynamicTensor& rhs) {
  if (!lhs.strict_core_eligible() || !rhs.strict_core_eligible()) {
    return TensorNumericClass::HostFloat;
  }
  return TensorNumericClass::ExactInt;
}

}  // namespace reduce_detail

// Reduce a rank-2 tensor along axis:
//   axis == 0  → reduce over rows (per-column result)    → shape {C}
//   axis == 1  → reduce over cols  (per-row result)      → shape {R}
inline T729DynamicTensor reduce_sum_2d(const T729DynamicTensor& m, int axis) {
  if (m.rank() != 2) throw std::invalid_argument("reduce_sum_2d: expects rank-2");
  const int R = m.shape()[0], C = m.shape()[1];
  const auto d = m.snapshot_values();

  if (axis == 0) {
    std::vector<float> out((size_t)C, 0.0f);
    for (int r = 0; r < R; ++r) {
      const size_t base = (size_t)r * C;
      for (int c = 0; c < C; ++c) out[(size_t)c] += d[base + (size_t)c];
    }
    return T729DynamicTensor({C}, std::move(out));
  } else if (axis == 1) {
    std::vector<float> out((size_t)R, 0.0f);
    for (int r = 0; r < R; ++r) {
      float s = 0.0f;
      const size_t base = (size_t)r * C;
      for (int c = 0; c < C; ++c) s += d[base + (size_t)c];
      out[(size_t)r] = s;
    }
    return T729DynamicTensor({R}, std::move(out));
  }
  throw std::invalid_argument("reduce_sum_2d: axis must be 0 or 1");
}

// Reduce max over axis (rank-2). Same axis semantics as sum.
inline T729DynamicTensor reduce_max_2d(const T729DynamicTensor& m, int axis) {
  if (m.rank() != 2) throw std::invalid_argument("reduce_max_2d: expects rank-2");
  const int R = m.shape()[0], C = m.shape()[1];
  const auto d = m.snapshot_values();

  if (axis == 0) {
    std::vector<float> out((size_t)C, -std::numeric_limits<float>::infinity());
    for (int r = 0; r < R; ++r) {
      const size_t base = (size_t)r * C;
      for (int c = 0; c < C; ++c) {
        float v = d[base + (size_t)c];
        if (v > out[(size_t)c]) out[(size_t)c] = v;
      }
    }
    return T729DynamicTensor({C}, std::move(out));
  } else if (axis == 1) {
    std::vector<float> out((size_t)R, -std::numeric_limits<float>::infinity());
    for (int r = 0; r < R; ++r) {
      float mmax = -std::numeric_limits<float>::infinity();
      const size_t base = (size_t)r * C;
      for (int c = 0; c < C; ++c) {
        float v = d[base + (size_t)c];
        if (v > mmax) mmax = v;
      }
      out[(size_t)r] = mmax;
    }
    return T729DynamicTensor({R}, std::move(out));
  }
  throw std::invalid_argument("reduce_max_2d: axis must be 0 or 1");
}

// Reduce min over axis (rank-2). Same axis semantics as sum.
inline T729DynamicTensor reduce_min_2d(const T729DynamicTensor& m, int axis) {
  if (m.rank() != 2) throw std::invalid_argument("reduce_min_2d: expects rank-2");
  const int R = m.shape()[0], C = m.shape()[1];
  const auto d = m.snapshot_values();

  if (axis == 0) {
    std::vector<float> out((size_t)C, std::numeric_limits<float>::infinity());
    for (int r = 0; r < R; ++r) {
      const size_t base = (size_t)r * C;
      for (int c = 0; c < C; ++c) {
        float v = d[base + (size_t)c];
        if (v < out[(size_t)c]) out[(size_t)c] = v;
      }
    }
    return T729DynamicTensor({C}, std::move(out));
  } else if (axis == 1) {
    std::vector<float> out((size_t)R, std::numeric_limits<float>::infinity());
    for (int r = 0; r < R; ++r) {
      float mmin = std::numeric_limits<float>::infinity();
      const size_t base = (size_t)r * C;
      for (int c = 0; c < C; ++c) {
        float v = d[base + (size_t)c];
        if (v < mmin) mmin = v;
      }
      out[(size_t)r] = mmin;
    }
    return T729DynamicTensor({R}, std::move(out));
  }
  throw std::invalid_argument("reduce_min_2d: axis must be 0 or 1");
}

// Reduce mean over axis (rank-2). Same axis semantics as sum.
inline T729DynamicTensor reduce_mean_2d(const T729DynamicTensor& m, int axis) {
  if (m.rank() != 2) throw std::invalid_argument("reduce_mean_2d: expects rank-2");
  const int R = m.shape()[0], C = m.shape()[1];

  T729DynamicTensor sums = reduce_sum_2d(m, axis);
  float count = static_cast<float>(axis == 0 ? R : C);
  auto out = sums.snapshot_values();
  for (auto& v : out) v /= count;
  return T729DynamicTensor(sums.shape(), std::move(out));
}

inline T729DynamicTensor contract_dot(const T729DynamicTensor& a, const T729DynamicTensor& b) {
  if (a.rank() != 1 || b.rank() != 1) {
    throw std::invalid_argument("contract_dot: both inputs must be vectors");
  }
  if (a.shape()[0] != b.shape()[0]) {
    throw std::invalid_argument("contract_dot: size mismatch");
  }
  const TensorNumericClass result_class = reduce_detail::contract_result_class(a, b);
  if (a.strict_core_eligible() && b.strict_core_eligible() &&
      a.has_canonical_fixed_data() && b.has_canonical_fixed_data()) {
    reduce_detail::DFixed sum = reduce_detail::DFixed::zero();
    const auto& lhs = a.canonical_fixed_data();
    const auto& rhs = b.canonical_fixed_data();
    for (std::size_t i = 0; i < lhs.size(); ++i) {
      sum = sum + lhs[i] * rhs[i];
    }
    return T729DynamicTensor::from_canonical_fixed({1}, std::vector<reduce_detail::DFixed>{sum},
                                                   result_class);
  }
  const auto lhs = a.snapshot_values();
  const auto rhs = b.snapshot_values();
  float sum = 0.0f;
  for (std::size_t i = 0; i < lhs.size(); ++i) {
    sum += lhs[i] * rhs[i];
  }
  auto result = T729DynamicTensor({1}, std::vector<float>{sum});
  result.set_numeric_class(result_class);
  return result;
}

}  // namespace t81::ops
