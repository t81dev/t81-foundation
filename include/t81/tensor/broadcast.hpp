#pragma once
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"
#include "t81/tensor/shape.hpp"

namespace t81::ops {

// Broadcast a rank-1 or rank-2 tensor to a larger, broadcast-compatible shape.
// Repeats values along expanded dimensions (naive CPU implementation).
inline T729Tensor broadcast_to(const T729Tensor& x, const std::vector<int>& new_shape) {
  if (new_shape.empty()) throw std::invalid_argument("broadcast_to: new_shape must be non-empty");
  if (!shape::can_broadcast_to(x.shape(), new_shape))
    throw std::invalid_argument("broadcast_to: shapes not broadcast-compatible");

  const auto& in = x.data();
  const auto& xs = x.shape();
  const int new_rank = static_cast<int>(new_shape.size());
  const size_t out_sz = shape::size_of(new_shape);
  std::vector<float> out(out_sz, 0.0f);

  // Right-align old shape to new shape.
  std::vector<int> xs_aligned(new_rank, 1);
  for (int i = 0; i < static_cast<int>(xs.size()); ++i)
    xs_aligned[new_rank - 1 - i] = xs[xs.size() - 1 - i];

  // Precompute strides for input and output (row-major).
  auto strides = [](const std::vector<int>& shp) {
    std::vector<size_t> s(shp.size(), 1);
    for (int i = static_cast<int>(shp.size()) - 2; i >= 0; --i)
      s[static_cast<size_t>(i)] = s[static_cast<size_t>(i + 1)] * static_cast<size_t>(shp[static_cast<size_t>(i + 1)]);
    return s;
  };
  std::vector<size_t> out_strides = strides(new_shape);
  std::vector<size_t> in_strides  = strides(xs_aligned);

  // Iterate over out indices and map to in index by clamping broadcasted dims to 0.
  std::vector<int> idx(new_rank, 0);
  for (size_t flat = 0; flat < out_sz; ++flat) {
    // Recover multi-index (row-major)
    size_t rem = flat;
    for (int d = 0; d < new_rank; ++d) {
      idx[static_cast<size_t>(d)] = static_cast<int>(rem / out_strides[static_cast<size_t>(d)]);
      rem %= out_strides[static_cast<size_t>(d)];
    }

    // Map to input index
    size_t in_flat = 0;
    for (int d = 0; d < new_rank; ++d) {
      int dim = xs_aligned[static_cast<size_t>(d)];
      int ii  = (dim == 1) ? 0 : idx[static_cast<size_t>(d)];
      in_flat += static_cast<size_t>(ii) * in_strides[static_cast<size_t>(d)];
    }

    out[flat] = in[in_flat];
  }

  return T729Tensor(new_shape, std::move(out));
}

} // namespace t81::ops
