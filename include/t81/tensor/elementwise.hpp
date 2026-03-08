#pragma once
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"
#include "t81/tensor/broadcast.hpp"
#include "t81/tensor/shape.hpp"
#include "t81/types/T81Float.hpp"

namespace t81::ops {

namespace elemwise_detail {

using TensorFloat = t81::v1::T81Float<72, 9>;

inline TensorNumericClass binary_result_class(const T729DynamicTensor& lhs,
                                              const T729DynamicTensor& rhs, bool multiply) {
  if (!lhs.strict_core_eligible() || !rhs.strict_core_eligible()) {
    return TensorNumericClass::HostFloat;
  }
  if (multiply && lhs.numeric_class() == TensorNumericClass::ExactTrit &&
      rhs.numeric_class() == TensorNumericClass::ExactTrit) {
    return TensorNumericClass::ExactTrit;
  }
  return TensorNumericClass::ExactInt;
}

}  // namespace elemwise_detail

// Elementwise binary op with NumPy-style right-aligned broadcasting.
template <typename T, typename Op>
inline T729TensorBase<T> elemwise_binary(const T729TensorBase<T>& A, const T729TensorBase<T>& B,
                                         Op op) {
  // Fast path: exact same shape
  if (A.shape() == B.shape()) {
    std::vector<T> out(A.size());
    const auto a = A.snapshot_values();
    const auto b = B.snapshot_values();
    for (std::size_t i = 0; i < out.size(); ++i) out[i] = op(a[i], b[i]);
    return T729TensorBase<T>(A.shape(), std::move(out));
  }

  // General case: broadcast both to the joined shape
  auto out_shape = t81::shape::broadcast_shape(A.shape(), B.shape());
  T729TensorBase<T> Ab = (A.shape() == out_shape) ? A : t81::ops::broadcast_to(A, out_shape);
  T729TensorBase<T> Bb = (B.shape() == out_shape) ? B : t81::ops::broadcast_to(B, out_shape);

  std::vector<T> out(Ab.size());
  const auto a = Ab.snapshot_values();
  const auto b = Bb.snapshot_values();
  for (std::size_t i = 0; i < out.size(); ++i) out[i] = op(a[i], b[i]);
  return T729TensorBase<T>(std::move(out_shape), std::move(out));
}

// Convenience wrappers
inline T729DynamicTensor add(const T729DynamicTensor& A, const T729DynamicTensor& B) {
  auto out = elemwise_binary(A, B, [](float x, float y) { return x + y; });
  out.set_numeric_class(elemwise_detail::binary_result_class(A, B, false));
  return out;
}
inline T729DynamicTensor sub(const T729DynamicTensor& A, const T729DynamicTensor& B) {
  auto out = elemwise_binary(A, B, [](float x, float y) { return x - y; });
  out.set_numeric_class(elemwise_detail::binary_result_class(A, B, false));
  return out;
}
inline T729DynamicTensor mul(const T729DynamicTensor& A, const T729DynamicTensor& B) {
  auto out = elemwise_binary(A, B, [](float x, float y) { return x * y; });
  out.set_numeric_class(elemwise_detail::binary_result_class(A, B, true));
  return out;
}
inline T729DynamicTensor div(const T729DynamicTensor& A, const T729DynamicTensor& B) {
  auto out = elemwise_binary(A, B, [](float x, float y) {
    if (y == 0.0f) throw std::domain_error("elemwise div: divide by zero");
    return static_cast<float>((elemwise_detail::TensorFloat::from_double(x) /
                               elemwise_detail::TensorFloat::from_double(y))
                                  .to_double());
  });
  out.set_numeric_class(TensorNumericClass::HostFloat);
  return out;
}

// Ternary convenience wrappers
inline T729IntTensor add(const T729IntTensor& A, const T729IntTensor& B) {
  return elemwise_binary(A, B, [](const T81Int<81>& x, const T81Int<81>& y) { return x + y; });
}
inline T729IntTensor sub(const T729IntTensor& A, const T729IntTensor& B) {
  return elemwise_binary(A, B, [](const T81Int<81>& x, const T81Int<81>& y) { return x - y; });
}
inline T729IntTensor mul(const T729IntTensor& A, const T729IntTensor& B) {
  return elemwise_binary(A, B, [](const T81Int<81>& x, const T81Int<81>& y) { return x * y; });
}

}  // namespace t81::ops
