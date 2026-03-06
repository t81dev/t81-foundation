#pragma once

#include <cmath>
#include <cstdint>
#include <optional>

#include "t81/tensor/contracts.hpp"

namespace t81::tensor_mutation {

enum class ScalarWriteKind {
  IntValue = 0,
  FloatValue,
};

inline bool is_exact_integral_scalar(float value, long long* out = nullptr) {
  if (!std::isfinite(value)) {
    return false;
  }
  const auto integral = static_cast<long long>(value);
  if (static_cast<float>(integral) != value) {
    return false;
  }
  if (out != nullptr) {
    *out = integral;
  }
  return true;
}

inline std::optional<float> read_scalar(const T729DynamicTensor& tensor, std::int64_t index) {
  if (!t81::tensor_contracts::linear_index_in_bounds(tensor, index)) {
    return std::nullopt;
  }
  return tensor.value_at(static_cast<std::size_t>(index));
}

inline bool write_scalar(T729DynamicTensor& tensor, std::int64_t index, float value,
                         ScalarWriteKind source_kind) {
  if (!t81::tensor_contracts::linear_index_in_bounds(tensor, index)) {
    return false;
  }
  if (!tensor.set_value_at(static_cast<std::size_t>(index), value)) {
    return false;
  }

  if (source_kind == ScalarWriteKind::FloatValue || !tensor.strict_core_eligible()) {
    tensor.set_numeric_class(TensorNumericClass::HostFloat);
    tensor.rebuild_canonical_fixed_cache();
    return true;
  }

  long long integral = 0;
  if (!is_exact_integral_scalar(value, &integral)) {
    tensor.set_numeric_class(TensorNumericClass::HostFloat);
    tensor.rebuild_canonical_fixed_cache();
    return true;
  }
  if (tensor.numeric_class() == TensorNumericClass::ExactTrit && (integral < -1 || integral > 1)) {
    tensor.set_numeric_class(TensorNumericClass::ExactInt);
  }
  tensor.rebuild_canonical_fixed_cache();
  return true;
}

inline T729DynamicTensor identity_copy(const T729DynamicTensor& tensor) {
  return tensor;
}

}  // namespace t81::tensor_mutation
