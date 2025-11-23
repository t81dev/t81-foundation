#pragma once
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"

namespace t81::ops {

// Transpose a rank-2 tensor (rows x cols) → (cols x rows).
inline T729Tensor transpose(const T729Tensor& m) {
  if (m.rank() != 2) throw std::invalid_argument("transpose: expects rank-2");
  const int rows = m.shape()[0], cols = m.shape()[1];
  const auto& d = m.data();

  std::vector<float> out(static_cast<size_t>(rows) * static_cast<size_t>(cols));
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      out[static_cast<size_t>(c) * rows + r] =
          d[static_cast<size_t>(r) * cols + c];
    }
  }
  return T729Tensor({cols, rows}, std::move(out));
}

} // namespace t81::ops
