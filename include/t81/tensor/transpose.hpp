#pragma once
#include <stdexcept>
#include <vector>
#include "t81/tensor.hpp"

namespace t81::ops {

// Transpose a rank-2 tensor (rows x cols) → (cols x rows).
inline T729DynamicTensor transpose(const T729DynamicTensor& m) {
  if (m.rank() != 2) throw std::invalid_argument("transpose: expects rank-2");
  const int rows = m.shape()[0], cols = m.shape()[1];
  if (m.has_canonical_fixed_data()) {
    std::vector<t81::core::detail::DFixed> out(static_cast<std::size_t>(rows) * static_cast<std::size_t>(cols));
    const auto& fixed = m.canonical_fixed_data();
    for (int r = 0; r < rows; ++r) {
      for (int c = 0; c < cols; ++c) {
        out[static_cast<std::size_t>(c) * static_cast<std::size_t>(rows) + static_cast<std::size_t>(r)] =
            fixed[static_cast<std::size_t>(r) * static_cast<std::size_t>(cols) + static_cast<std::size_t>(c)];
      }
    }
    return T729DynamicTensor::from_canonical_fixed({cols, rows}, std::move(out), m.numeric_class());
  }
  const auto d = m.snapshot_values();

  std::vector<float> out(static_cast<size_t>(rows) * static_cast<size_t>(cols));
  for (int r = 0; r < rows; ++r) {
    for (int c = 0; c < cols; ++c) {
      out[static_cast<size_t>(c) * rows + r] = d[static_cast<size_t>(r) * cols + c];
    }
  }
  auto result = T729DynamicTensor({cols, rows}, std::move(out));
  result.set_numeric_class(m.numeric_class());
  return result;
}

}  // namespace t81::ops
