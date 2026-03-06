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

}  // namespace t81::tensor_contracts
