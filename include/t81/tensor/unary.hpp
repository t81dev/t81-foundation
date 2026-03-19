#pragma once
#include <algorithm>
#include <cmath>
#include <compare>
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"
#include "t81/types/T81Float.hpp"
#include "t81/types/detail/dmath.hpp"

namespace t81::ops {

namespace detail {

using t81::core::detail::DFixed;
using TensorFloat = t81::v1::T81Float<72, 9>;

inline bool unary_all_nonnegative(const T729DynamicTensor& tensor) {
  if (!tensor.has_canonical_fixed_data()) {
    return false;
  }
  const auto& fixed = tensor.canonical_fixed_data();
  return std::all_of(fixed.begin(), fixed.end(),
                     [](const DFixed& value) { return !value.is_negative(); });
}

inline int classify_exact_trit_fixed(const DFixed& value) {
  static const auto kNegOneRaw = (-DFixed::one()).v.raw_data();
  static const auto kZeroRaw = DFixed::zero().v.raw_data();
  static const auto kOneRaw = DFixed::one().v.raw_data();

  const auto& raw = value.v.raw_data();
  if (raw == kNegOneRaw) {
    return -1;
  }
  if (raw == kZeroRaw) {
    return 0;
  }
  if (raw == kOneRaw) {
    return 1;
  }
  return 2;
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
    if (x.numeric_class() == TensorNumericClass::ExactTrit) {
      static const detail::DFixed kExpNegOne = t81::core::detail::exp(-detail::DFixed::one());
      static const detail::DFixed kExpZero = detail::DFixed::one();
      static const detail::DFixed kExpOne = t81::core::detail::exp(detail::DFixed::one());

      std::vector<detail::DFixed> out;
      out.reserve(x.size());
      for (const auto& value : x.canonical_fixed_data()) {
        switch (detail::classify_exact_trit_fixed(value)) {
          case -1:
            out.push_back(kExpNegOne);
            break;
          case 0:
            out.push_back(kExpZero);
            break;
          case 1:
            out.push_back(kExpOne);
            break;
          default:
            out.push_back(t81::core::detail::exp(value));
            break;
        }
      }
      const auto result_class =
          x.strict_core_eligible() ? TensorNumericClass::ExactInt : TensorNumericClass::HostFloat;
      return T729DynamicTensor::from_canonical_fixed(x.shape(), std::move(out), result_class);
    }
    std::vector<detail::DFixed> out;
    out.reserve(x.size());
    for (const auto& value : x.canonical_fixed_data()) {
      out.push_back(t81::core::detail::exp(value));
    }
    const auto result_class =
        x.strict_core_eligible() ? TensorNumericClass::ExactInt : TensorNumericClass::HostFloat;
    return T729DynamicTensor::from_canonical_fixed(x.shape(), std::move(out), result_class);
  }
  return unary_map(x, [](float v) {
    return static_cast<float>(t81::core::detail::exp(detail::TensorFloat::from_double(v)).to_double());
  });
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
    const auto result_class =
        x.strict_core_eligible() ? TensorNumericClass::ExactInt : TensorNumericClass::HostFloat;
    return T729DynamicTensor::from_canonical_fixed(x.shape(), std::move(out), result_class);
  }
  return unary_map(x, [](float v) {
    if (v < 0.0f) throw std::domain_error("unary sqrt: negative input");
    return static_cast<float>(t81::core::detail::sqrt(detail::TensorFloat::from_double(v)).to_double());
  });
}

inline T729DynamicTensor log(const T729DynamicTensor& x) {
  if (x.has_canonical_fixed_data()) {
    if (!detail::unary_all_nonnegative(x) ||
        std::any_of(x.canonical_fixed_data().begin(), x.canonical_fixed_data().end(),
                    [](const detail::DFixed& value) { return value.is_zero(); })) {
      throw std::domain_error("unary log: non-positive input");
    }
    std::vector<detail::DFixed> out;
    out.reserve(x.size());
    for (const auto& value : x.canonical_fixed_data()) {
      out.push_back(t81::core::detail::log(value));
    }
    const auto result_class =
        x.strict_core_eligible() ? TensorNumericClass::ExactInt : TensorNumericClass::HostFloat;
    return T729DynamicTensor::from_canonical_fixed(x.shape(), std::move(out), result_class);
  }
  return unary_map(x, [](float v) {
    if (v <= 0.0f) throw std::domain_error("unary log: non-positive input");
    return static_cast<float>(t81::core::detail::log(detail::TensorFloat::from_double(v)).to_double());
  });
}

}  // namespace t81::ops
