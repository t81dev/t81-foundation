#pragma once
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"

namespace t81::ops {

// Naive matrix multiplication: C = A(m×k) · B(k×n) => C(m×n)
// Row-major storage. Throws if ranks != 2 or inner dims mismatch.
inline T729Tensor matmul(const T729Tensor& A, const T729Tensor& B) {
  if (A.rank() != 2 || B.rank() != 2)
    throw std::invalid_argument("matmul: both operands must be rank-2");
  const int m = A.shape()[0], kA = A.shape()[1];
  const int kB = B.shape()[0], n = B.shape()[1];
  if (kA != kB) throw std::invalid_argument("matmul: inner dimensions must match");

  const auto& a = A.data();
  const auto& b = B.data();
  std::vector<float> c(static_cast<size_t>(m) * static_cast<size_t>(n), 0.0f);

  for (int i = 0; i < m; ++i) {
    for (int p = 0; p < kA; ++p) {
      const float a_ip = a[static_cast<size_t>(i) * kA + p];
      const size_t b_row = static_cast<size_t>(p) * n;
      float* crow = &c[static_cast<size_t>(i) * n];
      for (int j = 0; j < n; ++j) {
        crow[j] += a_ip * b[b_row + j];
      }
    }
  }
  return T729Tensor({m, n}, std::move(c));
}

} // namespace t81::ops
