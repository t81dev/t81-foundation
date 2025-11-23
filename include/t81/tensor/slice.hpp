#pragma once
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"

namespace t81::ops {

// 2D slice with unit strides: rows [r0, r1), cols [c0, c1).
// Throws on rank != 2 or out-of-range indices.
inline T729Tensor slice2d(const T729Tensor& m, int r0, int r1, int c0, int c1) {
  if (m.rank() != 2) throw std::invalid_argument("slice2d expects rank-2");
  const int rows = m.shape()[0], cols = m.shape()[1];
  if (r0 < 0 || c0 < 0 || r1 > rows || c1 > cols || r0 > r1 || c0 > c1)
    throw std::out_of_range("slice2d indices out of range");

  const int out_rows = r1 - r0;
  const int out_cols = c1 - c0;
  std::vector<float> out(static_cast<size_t>(out_rows) * static_cast<size_t>(out_cols));

  const auto& in = m.data();
  for (int i = 0; i < out_rows; ++i) {
    for (int j = 0; j < out_cols; ++j) {
      out[static_cast<size_t>(i) * out_cols + j] =
          in[static_cast<size_t>(r0 + i) * cols + (c0 + j)];
    }
  }
  return T729Tensor({out_rows, out_cols}, std::move(out));
}

} // namespace t81::ops
