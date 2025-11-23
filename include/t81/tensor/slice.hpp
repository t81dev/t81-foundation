#pragma once
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"

namespace t81::ops {

// Slice rank-2 tensor rows[r0:r1) and cols[c0:c1).
// Indices are half-open, 0-based. Throws on rank≠2 or bad bounds.
inline T729Tensor slice2d(const T729Tensor& m, int r0, int r1, int c0, int c1) {
  if (m.rank() != 2) throw std::invalid_argument("slice2d: expects rank-2");
  const int rows = m.shape()[0], cols = m.shape()[1];
  if (r0 < 0 || r1 < r0 || r1 > rows) throw std::invalid_argument("slice2d: bad row range");
  if (c0 < 0 || c1 < c0 || c1 > cols) throw std::invalid_argument("slice2d: bad col range");

  const int out_r = r1 - r0;
  const int out_c = c1 - c0;
  std::vector<float> out(static_cast<size_t>(out_r) * static_cast<size_t>(out_c));

  const auto& d = m.data();
  for (int r = 0; r < out_r; ++r) {
    const size_t src_base = static_cast<size_t>(r0 + r) * cols + static_cast<size_t>(c0);
    const size_t dst_base = static_cast<size_t>(r) * out_c;
    for (int c = 0; c < out_c; ++c) {
      out[dst_base + static_cast<size_t>(c)] = d[src_base + static_cast<size_t>(c)];
    }
  }
  return T729Tensor({out_r, out_c}, std::move(out));
}

} // namespace t81::ops
