#pragma once
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"

namespace t81::ops {

// Reshape with total-size preservation. A dimension may be -1 to infer.
// Example: reshape(x, { -1, 3 }) infers the first dim from size/3.
// Throws if product(new_shape with -1 resolved) != old size or multiple -1s.
inline T729Tensor reshape(const T729Tensor& m, std::vector<int> new_shape) {
  if (new_shape.empty()) throw std::invalid_argument("reshape: new_shape must be non-empty");

  // compute old size
  size_t old_sz = m.size();

  // handle -1 inference
  int infer_idx = -1;
  size_t known_prod = 1;
  for (size_t i = 0; i < new_shape.size(); ++i) {
    int d = new_shape[i];
    if (d == -1) {
      if (infer_idx != -1) throw std::invalid_argument("reshape: at most one '-1' dimension");
      infer_idx = static_cast<int>(i);
    } else if (d <= 0) {
      throw std::invalid_argument("reshape: dimensions must be positive (except one '-1')");
    } else {
      known_prod *= static_cast<size_t>(d);
    }
  }

  if (infer_idx != -1) {
    if (old_sz % known_prod != 0) throw std::invalid_argument("reshape: size not divisible for inference");
    size_t inferred = old_sz / known_prod;
    if (inferred == 0) throw std::invalid_argument("reshape: inferred dimension cannot be zero");
    if (inferred > static_cast<size_t>(std::numeric_limits<int>::max()))
      throw std::overflow_error("reshape: inferred dimension overflow");
    new_shape[static_cast<size_t>(infer_idx)] = static_cast<int>(inferred);
  }

  // verify size match
  size_t new_sz = 1;
  for (int d : new_shape) new_sz *= static_cast<size_t>(d);
  if (new_sz != old_sz) throw std::invalid_argument("reshape: size mismatch");

  // data stays identical, only shape changes
  return T729Tensor(std::move(new_shape), m.data());
}

} // namespace t81::ops
