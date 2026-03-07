#pragma once

#include <cstdint>

#include "t81/tensor.hpp"

namespace t81::tensor_contracts {

inline bool vec_binary_compatible(const T729DynamicTensor& lhs, const T729DynamicTensor& rhs) {
  return lhs.size() == rhs.size();
}

inline bool softmax_compatible(const T729DynamicTensor& tensor) {
  return tensor.rank() != 0;
}

inline bool matmul_compatible(const T729DynamicTensor& lhs, const T729DynamicTensor& rhs) {
  return lhs.rank() == 2 && rhs.rank() == 2 && rhs.shape()[0] == lhs.shape()[1];
}

inline bool transpose_2d_compatible(const T729DynamicTensor& tensor) {
  return tensor.rank() == 2;
}

inline bool rmsnorm_compatible(const T729DynamicTensor& tensor, const T729DynamicTensor& weights) {
  return tensor.rank() != 0 && weights.rank() == 1 && weights.shape()[0] == tensor.shape().back();
}

inline bool rope_compatible(const T729DynamicTensor& tensor) {
  return tensor.rank() >= 2;
}

inline bool attention_compatible(const T729DynamicTensor& q, const T729DynamicTensor& k,
                                 const T729DynamicTensor& v) {
  if (q.rank() != 2 || k.rank() != 2 || v.rank() != 2) {
    return false;
  }
  const int q_d = q.shape()[1];
  const int k_d = k.shape()[1];
  const int k_seq = k.shape()[0];
  const int v_seq = v.shape()[0];
  return q_d == k_d && k_seq == v_seq;
}

inline bool embed_compatible(const T729DynamicTensor& table, std::int64_t index) {
  return table.rank() == 2 && index >= 0 &&
         index < static_cast<std::int64_t>(table.shape()[0]);
}

inline bool linear_index_in_bounds(const T729DynamicTensor& tensor, std::int64_t index) {
  return index >= 0 && static_cast<std::size_t>(index) < tensor.size();
}

// RFC-0026: WLOAD — weight tensor must be at least rank-1.
inline bool wload_compatible(const T729DynamicTensor& src) {
  return src.rank() >= 1;
}

// RFC-0026: GATHER — src must be rank-2; index in bounds along the given axis (AI-M5).
inline bool gather_compatible(const T729DynamicTensor& src, std::int64_t index, int axis = 0) {
  return src.rank() == 2 &&
         axis >= 0 && axis < static_cast<int>(src.rank()) &&
         index >= 0 &&
         index < static_cast<std::int64_t>(src.shape()[static_cast<std::size_t>(axis)]);
}

// RFC-0026: SCATTER — dst must be rank-2; src must be rank-1 matching the slice
// dimension perpendicular to axis; index in bounds along axis (AI-M5).
inline bool scatter_compatible(const T729DynamicTensor& dst, std::int64_t index, int axis,
                               const T729DynamicTensor& src) {
  if (dst.rank() != 2 || src.rank() != 1) return false;
  if (axis < 0 || axis >= static_cast<int>(dst.rank())) return false;
  const int expected_src_len = dst.shape()[static_cast<std::size_t>(1 - axis)];
  return src.shape()[0] == expected_src_len &&
         index >= 0 &&
         index < static_cast<std::int64_t>(dst.shape()[static_cast<std::size_t>(axis)]);
}

}  // namespace t81::tensor_contracts
