#pragma once
#include <algorithm>
#include <cmath>
#include <compare>
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"
#include "t81/types/detail/dmath.hpp"

namespace t81::ops {

namespace detail {

using t81::core::detail::DFixed;

inline bool unary_all_nonnegative(const T729DynamicTensor& tensor) {
  if (!tensor.has_canonical_fixed_data()) {
    return false;
  }
  const auto& fixed = tensor.canonical_fixed_data();
  return std::all_of(fixed.begin(), fixed.end(),
                     [](const DFixed& value) { return !value.is_negative(); });
}

}  // namespace detail

// Map a unary functor over all elements (shape preserved).
template <typename Fn>
inline T729DynamicTensor unary_map(const T729DynamicTensor& x, Fn fn) {
  std::vector<float> out(x.size());
  const auto d = x.snapshot_values();
  for (std::size_t i = 0; i < d.size(); ++i) out[i] = fn(d[i]);
  return T729DynamicTensor(x.shape(), std::move(out));
}

inline T729DynamicTensor relu(const T729DynamicTensor& x) {
  return unary_map(x, [](float v) { return v < 0.0f ? 0.0f : v; });
}

inline T729DynamicTensor tanh(const T729DynamicTensor& x) {
  return unary_map(x, [](float v) { return std::tanh(v); });
}

inline T729DynamicTensor exp(const T729DynamicTensor& x) {
  if (x.has_canonical_fixed_data()) {
    std::vector<detail::DFixed> out;
    out.reserve(x.size());
    for (const auto& value : x.canonical_fixed_data()) {
      out.push_back(t81::core::detail::exp(value));
    }
    return T729DynamicTensor::from_canonical_fixed(x.shape(), std::move(out),
                                                   TensorNumericClass::HostFloat);
  }
  return unary_map(x, [](float v) { return std::exp(v); });
}

inline T729DynamicTensor sqrt(const T729DynamicTensor& x) {
  if (x.has_canonical_fixed_data()) {
    if (!detail::unary_all_nonnegative(x)) {
      throw std::domain_error("unary sqrt: negative input");
    }
    std::vector<detail::DFixed> out;
    out.reserve(x.size());
    for (const auto& value : x.canonical_fixed_data()) {
      out.push_back(t81::core::detail::sqrt(value));
    }
    return T729DynamicTensor::from_canonical_fixed(x.shape(), std::move(out),
                                                   TensorNumericClass::HostFloat);
  }
  return unary_map(x, [](float v) {
    if (v < 0.0f) throw std::domain_error("unary sqrt: negative input");
    return std::sqrt(v);
  });
}

inline T729DynamicTensor log(const T729DynamicTensor& x) {
  return unary_map(x, [](float v) {
    if (v <= 0.0f) throw std::domain_error("unary log: non-positive input");
    return std::log(v);
  });
}

}  // namespace t81::ops
