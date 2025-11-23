#pragma once
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"

namespace t81::ops {

// Sum along a given axis for rank-2 tensors.
// axis = 0 -> sum over rows -> shape {cols}
// axis = 1 -> sum over cols -> shape {rows}
inline T729Tensor reduce_sum_2d(const T729Tensor& m, int axis) {
  if (m.rank() != 2) throw std::invalid_argument("reduce_sum_2d: expects rank-2");
  const int rows = m.shape()[0], cols = m.shape()[1];
  const auto& d = m.data();

  if (axis == 0) {
    // sum over rows -> per-column totals
    std::vector<float> out(static_cast<size_t>(cols), 0.0f);
    for (int i = 0; i < rows; ++i) {
      const size_t base = static_cast<size_t>(i) * cols;
      for (int j = 0; j < cols; ++j) out[j] += d[base + j];
    }
    return T729Tensor({cols}, std::move(out));
  } else if (axis == 1) {
    // sum over cols -> per-row totals
    std::vector<float> out(static_cast<size_t>(rows), 0.0f);
    for (int i = 0; i < rows; ++i) {
      float s = 0.0f;
      const size_t base = static_cast<size_t>(i) * cols;
      for (int j = 0; j < cols; ++j) s += d[base + j];
      out[static_cast<size_t>(i)] = s;
    }
    return T729Tensor({rows}, std::move(out));
  }
  throw std::invalid_argument("reduce_sum_2d: axis must be 0 or 1");
}

} // namespace t81::ops
