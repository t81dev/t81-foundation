#pragma once
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"
#include "t81/tensor/shape.hpp"

namespace t81::ops {

// NumPy-style right-aligned broadcasting.
// Repeats elements of `src` to match `new_shape`.
// Example: {3} -> {2,3}, {1,3} -> {4,3}, {2,1,4} -> {2,3,4}.
inline T729Tensor broadcast_to(const T729Tensor& src, const std::vector<int>& new_shape) {
  if (new_shape.empty()) throw std::invalid_argument("broadcast_to: empty new_shape");
  // Build the right-aligned view of src.shape() against new_shape.
  const auto& a = src.shape();
  if (!t81::shape::can_broadcast_to(a, new_shape)) {
    throw std::invalid_argument("broadcast_to: incompatible shapes");
  }

  // Materialize an aligned "src shape" padded with leading 1s to rank(new_shape)
  const int r_out = static_cast<int>(new_shape.size());
  std::vector<int> src_aligned(r_out, 1);
  for (int i = 0; i < static_cast<int>(a.size()); ++i) {
    src_aligned[static_cast<size_t>(r_out - 1 - i)] = a[static_cast<size_t>(a.size() - 1 - i)];
  }

  // Precompute strides (row-major) for both shapes.
  const auto in_strides  = t81::shape::strides_of(src_aligned);
  const auto out_strides = t81::shape::strides_of(new_shape);

  // Total output elements.
  const std::size_t out_sz = t81::shape::size_of(new_shape);

  std::vector<float> out(out_sz);
  const auto& din = src.data();

  // Iterate flat over output, remap to input index (clamp broadcasted dims to 0).
  std::vector<int> idx(static_cast<size_t>(r_out), 0);
  for (std::size_t flat = 0; flat < out_sz; ++flat) {
    // Decode flat index into multi-index for out shape.
    std::size_t rem = flat;
    for (int d = 0; d < r_out; ++d) {
      idx[static_cast<size_t>(d)] = static_cast<int>(rem / out_strides[static_cast<size_t>(d)]);
      rem %= out_strides[static_cast<size_t>(d)];
    }
    // Map to input flat index.
    std::size_t in_flat = 0;
    for (int d = 0; d < r_out; ++d) {
      const int dim_src = src_aligned[static_cast<size_t>(d)];
      const int ii = (dim_src == 1) ? 0 : idx[static_cast<size_t>(d)];
      in_flat += static_cast<std::size_t>(ii) * in_strides[static_cast<size_t>(d)];
    }
    out[flat] = din[in_flat];
  }

  return T729Tensor(new_shape, std::move(out));
}

} // namespace t81::ops
