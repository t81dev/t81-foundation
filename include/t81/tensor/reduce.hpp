#pragma once
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"

namespace t81::ops {

// Reduce a rank-2 tensor along axis:
//   axis == 0  → reduce over rows (per-column result)    → shape {C}
//   axis == 1  → reduce over cols  (per-row result)      → shape {R}
inline T729Tensor reduce_sum_2d(const T729Tensor& m, int axis) {
  if (m.rank() != 2) throw std::invalid_argument("reduce_sum_2d: expects rank-2");
  const int R = m.shape()[0], C = m.shape()[1];
  const auto& d = m.data();

  if (axis == 0) {
    std::vector<float> out((size_t)C, 0.0f);
    for (int r = 0; r < R; ++r) {
      const size_t base = (size_t)r * C;
      for (int c = 0; c < C; ++c) out[(size_t)c] += d[base + (size_t)c];
    }
    return T729Tensor({C}, std::move(out));
  } else if (axis == 1) {
    std::vector<float> out((size_t)R, 0.0f);
    for (int r = 0; r < R; ++r) {
      float s = 0.0f;
      const size_t base = (size_t)r * C;
      for (int c = 0; c < C; ++c) s += d[base + (size_t)c];
      out[(size_t)r] = s;
    }
    return T729Tensor({R}, std::move(out));
  }
  throw std::invalid_argument("reduce_sum_2d: axis must be 0 or 1");
}

// Reduce max over axis (rank-2). Same axis semantics as sum.
inline T729Tensor reduce_max_2d(const T729Tensor& m, int axis) {
  if (m.rank() != 2) throw std::invalid_argument("reduce_max_2d: expects rank-2");
  const int R = m.shape()[0], C = m.shape()[1];
  const auto& d = m.data();

  if (axis == 0) {
    std::vector<float> out((size_t)C, -std::numeric_limits<float>::infinity());
    for (int r = 0; r < R; ++r) {
      const size_t base = (size_t)r * C;
      for (int c = 0; c < C; ++c) {
        float v = d[base + (size_t)c];
        if (v > out[(size_t)c]) out[(size_t)c] = v;
      }
    }
    return T729Tensor({C}, std::move(out));
  } else if (axis == 1) {
    std::vector<float> out((size_t)R, -std::numeric_limits<float>::infinity());
    for (int r = 0; r < R; ++r) {
      float mmax = -std::numeric_limits<float>::infinity();
      const size_t base = (size_t)r * C;
      for (int c = 0; c < C; ++c) {
        float v = d[base + (size_t)c];
        if (v > mmax) mmax = v;
      }
      out[(size_t)r] = mmax;
    }
    return T729Tensor({R}, std::move(out));
  }
  throw std::invalid_argument("reduce_max_2d: axis must be 0 or 1");
}

} // namespace t81::ops
