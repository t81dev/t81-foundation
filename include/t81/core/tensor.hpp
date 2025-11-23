#pragma once

#include <cstddef>
#include <vector>

namespace t81::core {
// Minimal tensor container. TODO: align with full spec/t81-data-types.md tensor semantics.
template <typename T>
struct Tensor {
  std::vector<std::size_t> shape;
  std::vector<T> data;

  [[nodiscard]] std::size_t total_size() const noexcept { return data.size(); }
};
}  // namespace t81::core

