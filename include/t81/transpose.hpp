#pragma once
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"

namespace t81::ops {

// Standalone transpose for T729Tensor (2D only).
// Mirrors T729Tensor::transpose but lives in a dedicated ops header.
inline T729Tensor transpose(const T729Tensor& m) {
  if (m.rank() != 2) throw std::invalid_argument("transpose expects rank-2");
  const int rows = m.shape()[0], cols = m.shape()[1];
  const auto& in = m.data();
  std::vector<float> out(static_cast<size_t>(rows) * static_cast<size_t>(cols), 0.0f);
  for (int i = 0; i < rows; ++i)
    for (int j = 0; j < cols; ++j)
      out[static_cast<size_t>(j) * rows + i] = in[static_cast<size_t>(i) * cols + j];
  return T729Tensor({cols, rows}, std::move(out));
}

} // namespace t81::ops
