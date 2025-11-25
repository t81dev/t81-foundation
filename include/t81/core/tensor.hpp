/**
 * @file tensor.hpp
 * @brief Defines the core Tensor data structure for the T81 library.
 */

#pragma once

#include <cstddef>
#include <vector>

namespace t81::core {

/**
 * @brief A multi-dimensional array (Tensor) container.
 * @tparam T The data type of the elements in the tensor.
 *
 * This struct represents a tensor with a specific shape and underlying data.
 * It is a foundational data structure for all numerical and symbolic
 * computation in the T81 ecosystem.
 *
 * @note This is a minimal implementation. The full, specification-compliant
 *       tensor semantics are defined in `spec/t81-data-types.md`.
 */
template <typename T>
struct Tensor {
  /**
   * @brief The dimensions of the tensor.
   *
   * A vector of unsigned integers representing the size of each dimension.
   * For example, a 3x4 matrix would have a shape of {3, 4}.
   */
  std::vector<std::size_t> shape;

  /**
   * @brief The flattened, contiguous data of the tensor.
   *
   * The tensor's elements are stored in a row-major layout.
   */
  std::vector<T> data;

  /**
   * @brief Calculates the total number of elements in the tensor.
   * @return The size of the `data` vector.
   */
  [[nodiscard]] std::size_t total_size() const noexcept { return data.size(); }
};

}  // namespace t81::core
