#pragma once
#include <stdexcept>
#include <vector>
#include <numeric>
#include "t81/tensor.hpp"

namespace t81::ops {

// Reshape tensor to `new_shape` (row-major, data preserved).
// Supports a single -1 dim to infer size. Throws on mismatch/invalid dims.
inline T729Tensor reshape(const T729Tensor& t, std::vector<int> new_shape) {
  if (new_shape.empty()) throw std::invalid_argument("reshape: empty new_shape");

  // Count -1s and compute product of specified dims
  int neg1_idx = -1;
  std::size_t known = 1;
  for (int i = 0; i < (int)new_shape.size(); ++i) {
    const int d = new_shape[(size_t)i];
    if (d == -1) {
      if (neg1_idx != -1) throw std::invalid_argument("reshape: multiple -1 dims");
      neg1_idx = i;
    } else if (d > 0) {
      known *= static_cast<std::size_t>(d);
    } else {
      throw std::invalid_argument("reshape: non-positive dim (except -1)");
    }
  }

  const std::size_t total = t.size();
  if (neg1_idx != -1) {
    if (known == 0 || total % known != 0)
      throw std::invalid_argument("reshape: size not divisible to infer -1");
    const std::size_t inferred = total / known;
    if (inferred == 0 || inferred > static_cast<std::size_t>(std::numeric_limits<int>::max()))
      throw std::invalid_argument("reshape: inferred dim out of range");
    new_shape[(size_t)neg1_idx] = static_cast<int>(inferred);
  }

  // Validate element count matches
  std::size_t prod = 1;
  for (int d : new_shape) prod *= static_cast<std::size_t>(d);
  if (prod != total) throw std::invalid_argument("reshape: element count mismatch");

  // Row-major reinterpretation: data unchanged
  return T729Tensor(std::move(new_shape), t.data());
}

} // namespace t81::ops
