#pragma once
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"
#include "t81/tensor/shape.hpp"
#include "t81/tensor/broadcast.hpp"

namespace t81::ops {

// Elementwise binary op with NumPy-style right-aligned broadcasting.
// `Op` must be a functor: float(float,float)
template <typename Op>
inline T729Tensor elemwise_binary(const T729Tensor& A, const T729Tensor& B, Op op) {
  // Fast path: exact same shape
  if (A.shape() == B.shape()) {
    std::vector<float> out(A.size());
    const auto& a = A.data(); const auto& b = B.data();
    for (std::size_t i = 0; i < out.size(); ++i) out[i] = op(a[i], b[i]);
    return T729Tensor(A.shape(), std::move(out));
  }

  // General case: broadcast both to the joined shape
  auto out_shape = t81::shape::broadcast_shape(A.shape(), B.shape());
  T729Tensor Ab = (A.shape() == out_shape) ? A : t81::ops::broadcast_to(A, out_shape);
  T729Tensor Bb = (B.shape() == out_shape) ? B : t81::ops::broadcast_to(B, out_shape);

  std::vector<float> out(Ab.size());
  const auto& a = Ab.data(); const auto& b = Bb.data();
  for (std::size_t i = 0; i < out.size(); ++i) out[i] = op(a[i], b[i]);
  return T729Tensor(std::move(out_shape), std::move(out));
}

// Convenience wrappers
inline T729Tensor add(const T729Tensor& A, const T729Tensor& B) {
  return elemwise_binary(A, B, [](float x, float y){ return x + y; });
}
inline T729Tensor sub(const T729Tensor& A, const T729Tensor& B) {
  return elemwise_binary(A, B, [](float x, float y){ return x - y; });
}
inline T729Tensor mul(const T729Tensor& A, const T729Tensor& B) {
  return elemwise_binary(A, B, [](float x, float y){ return x * y; });
}
inline T729Tensor div(const T729Tensor& A, const T729Tensor& B) {
  return elemwise_binary(A, B, [](float x, float y){
    if (y == 0.0f) throw std::domain_error("elemwise div: divide by zero");
    return x / y;
  });
}

} // namespace t81::ops
