#pragma once
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"

namespace t81::ops {

// Naive row-major matrix multiply: (m×k) · (k×n) → (m×n)
inline T729Tensor matmul(const T729Tensor& A, const T729Tensor& B) {
  if (A.rank() != 2 || B.rank() != 2)
    throw std::invalid_argument("matmul: both inputs must be rank-2");
  const int m = A.shape()[0], kA = A.shape()[1];
  const int kB = B.shape()[0], n = B.shape()[1];
  if (kA != kB) throw std::invalid_argument("matmul: inner dimensions mismatch");

  const auto& a = A.data();
  const auto& b = B.data();
  std::vector<float> c(static_cast<size_t>(m) * static_cast<size_t>(n), 0.0f);

  for (int i = 0; i < m; ++i) {
    for (int p = 0; p < kA; ++p) {
      const float av = a[static_cast<size_t>(i) * kA + static_cast<size_t>(p)];
      const size_t b_row = static_cast<size_t>(p) * n;
      const size_t c_row = static_cast<size_t>(i) * n;
      for (int j = 0; j < n; ++j) {
        c[c_row + static_cast<size_t>(j)] += av * b[b_row + static_cast<size_t>(j)];
      }
    }
  }

  return T729Tensor({m, n}, std::move(c));
}

} // namespace t81::ops
