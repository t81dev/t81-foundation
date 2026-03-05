#include "internal/tensor_helpers.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

#include "t81/tensor/llama.hpp"
#include "t81/tensor/matmul.hpp"
#include "t81/tracing/canonhash.hpp"

namespace t81::vm::internal {

std::size_t tensor_shape_complexity(const t81::T729DynamicTensor& tensor) {
  std::size_t product = 1;
  for (int dim : tensor.shape()) {
    if (dim <= 0) {
      return 0;
    }
    product *= static_cast<std::size_t>(dim);
  }
  return product * static_cast<std::size_t>(tensor.rank());
}

TensorAllocPolicyResult evaluate_tensor_alloc_policy(const State& state,
                                                     std::size_t tensor_elements) {
  if (!state.policy) {
    return TensorAllocPolicyResult::Allow;
  }

  const std::size_t active_tensors = state.tensors.size() - state.free_tensor_indices.size();
  if (state.policy->max_tensors &&
      active_tensors >= static_cast<std::size_t>(*state.policy->max_tensors)) {
    return TensorAllocPolicyResult::MaxTensorsExceeded;
  }
  if (state.policy->max_tensor_elements &&
      state.total_tensor_elements + tensor_elements >
          static_cast<std::size_t>(*state.policy->max_tensor_elements)) {
    return TensorAllocPolicyResult::MaxTensorElementsExceeded;
  }
  return TensorAllocPolicyResult::Allow;
}

std::size_t store_tensor_slot(State& state, t81::T729DynamicTensor tensor) {
  std::size_t idx_handle = 0;
  if (!state.free_tensor_indices.empty()) {
    const auto raw_idx = state.free_tensor_indices.back();
    state.free_tensor_indices.pop_back();
    state.tensors[raw_idx] = std::move(tensor);
    idx_handle = raw_idx + 1;
  } else {
    state.tensors.push_back(std::move(tensor));
    idx_handle = state.tensors.size();
  }
  return idx_handle;
}

void account_tensor_allocation(State& state, std::size_t tensor_elements) {
  state.total_tensor_elements += tensor_elements;
  state.metrics.total_tensors++;
  state.metrics.total_tensor_elements += tensor_elements;
}

std::optional<t81::T729DynamicTensor> decode_native_tensor(const t81::weights::NativeTensor& native,
                                                           TensorDecodeMode mode) {
  std::vector<float> float_data;
  float_data.reserve(native.num_trits());

  if (native.format == t81::weights::NativeFormat::T3_K) {
    const uint8_t* byte_ptr = reinterpret_cast<const uint8_t*>(native.data.data());
    const uint64_t total_trits = native.num_trits();
    for (uint64_t offset = 0; offset < total_trits; offset += 128) {
      float scale;
      std::memcpy(&scale, byte_ptr, sizeof(float));
      byte_ptr += sizeof(float);
      const uint64_t count = std::min<uint64_t>(128, total_trits - offset);
      uint64_t trit_index = 0;
      for (uint64_t packed_idx = 0; packed_idx < 26; ++packed_idx) {
        uint8_t packed = *byte_ptr++;
        if (mode == TensorDecodeMode::StrictCanonical && packed > 242) {
          return std::nullopt;
        }
        uint8_t rem = packed;
        for (uint64_t local = 0; local < 5; ++local, ++trit_index) {
          const uint8_t digit = static_cast<uint8_t>(rem % 3);
          rem = static_cast<uint8_t>(rem / 3);
          if (trit_index < count) {
            const float trit = static_cast<float>(static_cast<int>(digit) - 1);
            float_data.push_back(trit * scale);
          } else if (mode == TensorDecodeMode::StrictCanonical && digit != 1) {
            // Canonical padding requires extra trits to be zero (mapped digit=1).
            return std::nullopt;
          }
        }
      }
    }
  } else {
    uint64_t remaining = native.trits;
    if (remaining == 0 && !native.data.empty()) {
      remaining = native.data.size() * 48;
    }
    for (uint64_t limb : native.data) {
      const uint64_t count = std::min<uint64_t>(48, remaining);
      std::vector<float> block(count);
      uint64_t val = limb;
      for (int i = 47; i >= 0; --i) {
        const uint64_t digit = val % 3;
        val /= 3;
        if (static_cast<uint64_t>(i) < count) {
          block[i] = static_cast<float>(static_cast<int>(digit) - 1);
        }
      }
      float_data.insert(float_data.end(), block.begin(), block.end());
      remaining -= count;
      if (remaining == 0) {
        break;
      }
    }
  }

  std::vector<int> shape;
  shape.reserve(native.shape.size());
  for (auto dim : native.shape) {
    shape.push_back(static_cast<int>(dim));
  }
  return t81::T729DynamicTensor(std::move(shape), std::move(float_data));
}

std::optional<t81::weights::NativeTensor> parse_canon_tensor_object(
    const std::vector<std::byte>& bytes) {
  // Header is 72 bytes: type(1), version(1), format(1), rank(1), reserved(4), shape(64).
  if (bytes.size() < 72) {
    return std::nullopt;
  }

  const uint8_t* ptr = reinterpret_cast<const uint8_t*>(bytes.data());
  if (*ptr++ != 0x20) {
    return std::nullopt;
  }
  if (*ptr++ != 1) {
    return std::nullopt;
  }

  const uint8_t fmt = *ptr++;
  if (fmt != static_cast<uint8_t>(t81::weights::NativeFormat::BalancedTernary) &&
      fmt != static_cast<uint8_t>(t81::weights::NativeFormat::T3_K)) {
    return std::nullopt;
  }

  const uint8_t rank = *ptr++;
  if (rank > 8) {
    return std::nullopt;
  }
  ptr += 4;  // reserved

  t81::weights::NativeTensor native;
  native.format = static_cast<t81::weights::NativeFormat>(fmt);
  native.shape.reserve(rank);
  for (int i = 0; i < 8; ++i) {
    uint64_t dim = 0;
    for (int b = 0; b < 8; ++b) {
      dim |= (static_cast<uint64_t>(*ptr++) << (b * 8));
    }
    if (i < rank) {
      native.shape.push_back(dim);
    }
  }

  const size_t payload_bytes =
      bytes.size() - (ptr - reinterpret_cast<const uint8_t*>(bytes.data()));
  if (payload_bytes % 8 != 0) {
    return std::nullopt;
  }

  const size_t limbs = payload_bytes / 8;
  native.data.reserve(limbs);
  for (size_t i = 0; i < limbs; ++i) {
    uint64_t val = 0;
    for (int b = 0; b < 8; ++b) {
      val |= (static_cast<uint64_t>(*ptr++) << (b * 8));
    }
    native.data.push_back(val);
  }

  uint64_t trits = 1;
  for (uint64_t dim : native.shape) {
    trits *= dim;
  }
  native.trits = trits;
  return native;
}

std::optional<t81::T729DynamicTensor> decode_canon_tensor_object(
    const std::vector<std::byte>& bytes) {
  auto native = parse_canon_tensor_object(bytes);
  if (!native.has_value()) {
    return std::nullopt;
  }

  // CanonFS tensor objects are expected to carry packed payloads. If payload limbs equal
  // expanded element count, treat this as an ambiguous layout and fail closed.
  size_t payload_limbs = native->data.size();
  size_t expected_elements = 1;
  for (auto dim : native->shape) {
    expected_elements *= static_cast<size_t>(dim);
  }
  if (payload_limbs == expected_elements) {
    return std::nullopt;
  }

  return decode_native_tensor(*native, TensorDecodeMode::Lenient);
}

std::optional<t81::canonfs::CanonRef> parse_canon_tensor_ref(std::string_view hash_text) {
  std::string stripped(hash_text);
  if (stripped.rfind("sha3-256:", 0) == 0) {
    stripped = stripped.substr(9);
  }

  t81::canonfs::CanonHash ch;
  try {
    ch.h = t81::hash::CanonHash81::from_string(stripped);
  } catch (...) {
    return std::nullopt;
  }
  return t81::canonfs::CanonRef{ch};
}

TensorLoadHashResult load_canon_tensor_by_hash(t81::canonfs::Driver& driver,
                                               std::string_view hash_text) {
  auto ref = parse_canon_tensor_ref(hash_text);
  if (!ref.has_value()) {
    return {TensorLoadHashStatus::InvalidHash, std::nullopt};
  }

  auto obj_res = driver.read_object_bytes(*ref);
  if (!obj_res.has_value()) {
    return {TensorLoadHashStatus::CanonFsMiss, std::nullopt};
  }

  auto decoded = decode_canon_tensor_object(obj_res.value());
  if (!decoded.has_value()) {
    return {TensorLoadHashStatus::DecodeFault, std::nullopt};
  }
  return {TensorLoadHashStatus::Ok, std::move(decoded)};
}

t81::T729DynamicTensor tensor_unary_exp(const t81::T729DynamicTensor& tensor) {
  std::vector<float> data = tensor.data();
  for (auto& val : data) {
    val = std::exp(val);
  }
  return t81::T729DynamicTensor(tensor.shape(), std::move(data));
}

t81::T729DynamicTensor tensor_unary_sqrt(const t81::T729DynamicTensor& tensor) {
  std::vector<float> data = tensor.data();
  for (auto& val : data) {
    val = std::sqrt(val);
  }
  return t81::T729DynamicTensor(tensor.shape(), std::move(data));
}

t81::T729DynamicTensor tensor_unary_silu(const t81::T729DynamicTensor& tensor) {
  return t81::ops::silu(tensor);
}

t81::T729DynamicTensor tensor_unary_softmax(const t81::T729DynamicTensor& tensor) {
  return t81::ops::softmax(tensor);
}

bool tensor_elementwise_compatible(const t81::T729DynamicTensor& lhs,
                                   const t81::T729DynamicTensor& rhs) {
  return lhs.data().size() == rhs.data().size();
}

bool tensor_softmax_compatible(const t81::T729DynamicTensor& tensor) { return tensor.rank() != 0; }

t81::T729DynamicTensor tensor_matmul_2d(const t81::T729DynamicTensor& lhs,
                                        const t81::T729DynamicTensor& rhs) {
  return t81::ops::matmul(lhs, rhs);
}

bool tensor_matmul_compatible(const t81::T729DynamicTensor& lhs,
                              const t81::T729DynamicTensor& rhs) {
  return lhs.rank() == 2 && rhs.rank() == 2 && rhs.shape()[0] == lhs.shape()[1];
}

t81::T729DynamicTensor tensor_binary_elementwise(const t81::T729DynamicTensor& lhs,
                                                 const t81::T729DynamicTensor& rhs, bool multiply) {
  std::vector<float> data(lhs.data().size());
  if (multiply) {
    for (std::size_t i = 0; i < data.size(); ++i) {
      data[i] = lhs.data()[i] * rhs.data()[i];
    }
  } else {
    for (std::size_t i = 0; i < data.size(); ++i) {
      data[i] = lhs.data()[i] + rhs.data()[i];
    }
  }
  return t81::T729DynamicTensor(lhs.shape(), std::move(data));
}

bool tensor_transpose_2d_compatible(const t81::T729DynamicTensor& tensor) {
  return tensor.rank() == 2;
}

t81::T729DynamicTensor tensor_transpose_2d(const t81::T729DynamicTensor& tensor) {
  return tensor.transpose2d();
}

std::optional<t81::T729DynamicTensor> tensor_contract_dot(const t81::T729DynamicTensor& lhs,
                                                          const t81::T729DynamicTensor& rhs) {
  try {
    return t81::T729DynamicTensor::contract_dot(lhs, rhs);
  } catch (...) {
    return std::nullopt;
  }
}

t81::T729DynamicTensor tensor_rmsnorm(const t81::T729DynamicTensor& tensor,
                                      const t81::T729DynamicTensor& weights) {
  return t81::ops::rmsnorm(tensor, weights);
}

bool tensor_rmsnorm_compatible(const t81::T729DynamicTensor& tensor,
                               const t81::T729DynamicTensor& weights) {
  return tensor.rank() != 0 && weights.rank() == 1 && weights.shape()[0] == tensor.shape().back();
}

t81::T729DynamicTensor tensor_rope(const t81::T729DynamicTensor& tensor, int pos) {
  return t81::ops::rope(tensor, pos);
}

bool tensor_rope_compatible(const t81::T729DynamicTensor& tensor) { return tensor.rank() >= 2; }

bool tensor_attention_compatible(const t81::T729DynamicTensor& q, const t81::T729DynamicTensor& k,
                                 const t81::T729DynamicTensor& v) {
  if (q.rank() != 2 || k.rank() != 2 || v.rank() != 2) {
    return false;
  }
  if (q.shape().size() != 2 || k.shape().size() != 2 || v.shape().size() != 2) {
    return false;
  }
  const int q_d = q.shape()[1];
  const int k_d = k.shape()[1];
  const int k_seq = k.shape()[0];
  const int v_seq = v.shape()[0];
  return q_d == k_d && k_seq == v_seq;
}

bool tensor_embed_compatible(const t81::T729DynamicTensor& table, std::int64_t index) {
  if (table.rank() != 2 || table.shape().size() != 2) {
    return false;
  }
  return index >= 0 && index < static_cast<std::int64_t>(table.shape()[0]);
}

std::optional<t81::T729DynamicTensor> tensor_new_1d(std::int64_t size) {
  if (size <= 0) {
    return std::nullopt;
  }
  std::vector<int> shape = {static_cast<int>(size)};
  return t81::T729DynamicTensor(shape);
}

t81::T729DynamicTensor tensor_identity_copy(const t81::T729DynamicTensor& tensor) {
  return t81::T729DynamicTensor(tensor.shape(), std::vector<float>(tensor.data()));
}

std::optional<float> tensor_get_at(const t81::T729DynamicTensor& tensor, std::int64_t index) {
  if (index < 0 || static_cast<std::size_t>(index) >= tensor.data().size()) {
    return std::nullopt;
  }
  return tensor.data()[static_cast<std::size_t>(index)];
}

bool tensor_set_at(t81::T729DynamicTensor& tensor, std::int64_t index, float value) {
  if (index < 0 || static_cast<std::size_t>(index) >= tensor.data().size()) {
    return false;
  }
  tensor.data()[static_cast<std::size_t>(index)] = value;
  return true;
}

std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_vec_binary_checked(
    const t81::T729DynamicTensor& lhs, const t81::T729DynamicTensor& rhs, bool multiply) {
  if (!tensor_elementwise_compatible(lhs, rhs)) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::ShapeFault);
  }
  return tensor_binary_elementwise(lhs, rhs, multiply);
}

std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_transpose_checked(
    const t81::T729DynamicTensor& tensor) {
  if (!tensor_transpose_2d_compatible(tensor)) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::ShapeFault);
  }
  return tensor_transpose_2d(tensor);
}

std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_matmul_checked(
    const t81::T729DynamicTensor& lhs, const t81::T729DynamicTensor& rhs) {
  if (!tensor_matmul_compatible(lhs, rhs)) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::ShapeFault);
  }
  return tensor_matmul_2d(lhs, rhs);
}

std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_contract_dot_checked(
    const t81::T729DynamicTensor& lhs, const t81::T729DynamicTensor& rhs) {
  auto out = tensor_contract_dot(lhs, rhs);
  if (!out.has_value()) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::ShapeFault);
  }
  return std::move(*out);
}

std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_attention_checked(
    const t81::T729DynamicTensor& q, const t81::T729DynamicTensor& k, const t81::T729DynamicTensor& v) {
  if (!tensor_attention_compatible(q, k, v)) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::ShapeFault);
  }

  // Deterministic phase-1 attention: softmax((Q*K^T)/sqrt(dk)) * V
  const int dk = q.shape()[1];
  if (dk <= 0) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::ShapeFault);
  }
  auto k_t = tensor_transpose_2d(k);
  auto scores = tensor_matmul_2d(q, k_t);
  const float inv_scale = 1.0f / std::sqrt(static_cast<float>(dk));
  for (auto& x : scores.data()) {
    x *= inv_scale;
  }
  auto probs = tensor_unary_softmax(scores);
  if (!tensor_matmul_compatible(probs, v)) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::ShapeFault);
  }
  return tensor_matmul_2d(probs, v);
}

std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_embed_checked(
    const t81::T729DynamicTensor& table, std::int64_t index) {
  if (!tensor_embed_compatible(table, index)) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                 t81::vm::Trap::BoundsFault);
  }
  const int dim = table.shape()[1];
  std::vector<float> out(static_cast<std::size_t>(dim));
  const std::size_t base = static_cast<std::size_t>(index) * static_cast<std::size_t>(dim);
  for (int i = 0; i < dim; ++i) {
    out[static_cast<std::size_t>(i)] = table.data()[base + static_cast<std::size_t>(i)];
  }
  return t81::T729DynamicTensor({dim}, std::move(out));
}

std::expected<float, t81::vm::Trap> tensor_get_checked(const t81::T729DynamicTensor& tensor,
                                                       std::int64_t index) {
  auto out = tensor_get_at(tensor, index);
  if (!out.has_value()) {
    return std::expected<float, t81::vm::Trap>(t81::unexpect, t81::vm::Trap::BoundsFault);
  }
  return *out;
}

std::expected<void, t81::vm::Trap> tensor_set_checked(t81::T729DynamicTensor& tensor,
                                                      std::int64_t index, float value) {
  if (!tensor_set_at(tensor, index, value)) {
    return std::expected<void, t81::vm::Trap>(t81::unexpect, t81::vm::Trap::BoundsFault);
  }
  return {};
}

}  // namespace t81::vm::internal
