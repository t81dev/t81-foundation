#include "internal/tensor_helpers.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <sstream>

#include "t81/tensor/llama.hpp"
#include "t81/tensor/matmul.hpp"
#include "t81/tensor/elementwise.hpp"
#include "t81/tensor/native.hpp"
#include "t81/tensor/reduce.hpp"
#include "t81/tensor/transpose.hpp"
#include "t81/tensor/unary.hpp"
#include "t81/tensor/contracts.hpp"
#include "t81/tensor/mutation.hpp"
#include "t81/tracing/canonhash.hpp"
#include "t81/types/detail/dmath.hpp"

namespace t81::vm::internal {

namespace {

using t81::core::detail::DFixed;

}  // namespace

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
  const auto shared_mode = mode == TensorDecodeMode::StrictCanonical
                               ? t81::tensor_native::DecodeMode::StrictCanonical
                               : t81::tensor_native::DecodeMode::Lenient;
  return t81::tensor_native::decode(native, shared_mode);
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
      fmt != static_cast<uint8_t>(t81::weights::NativeFormat::T3_K) &&
      fmt != static_cast<uint8_t>(t81::weights::NativeFormat::CanonicalFixed)) {
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
  native.trits =
      native.format == t81::weights::NativeFormat::CanonicalFixed ? trits * DFixed::Storage::kNumTrits
                                                                  : trits;
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
  auto out = t81::ops::exp(tensor);
  out.set_numeric_class(TensorNumericClass::HostFloat);
  return out;
}

t81::T729DynamicTensor tensor_unary_sqrt(const t81::T729DynamicTensor& tensor) {
  auto out = t81::ops::sqrt(tensor);
  out.set_numeric_class(TensorNumericClass::HostFloat);
  return out;
}

t81::T729DynamicTensor tensor_unary_silu(const t81::T729DynamicTensor& tensor) {
  auto out = t81::ops::silu(tensor);
  out.set_numeric_class(TensorNumericClass::HostFloat);
  return out;
}

t81::T729DynamicTensor tensor_unary_softmax(const t81::T729DynamicTensor& tensor) {
  auto out = t81::ops::softmax(tensor);
  out.set_numeric_class(TensorNumericClass::HostFloat);
  return out;
}

bool tensor_elementwise_compatible(const t81::T729DynamicTensor& lhs,
                                   const t81::T729DynamicTensor& rhs) {
  return t81::tensor_contracts::vec_binary_compatible(lhs, rhs);
}

bool tensor_softmax_compatible(const t81::T729DynamicTensor& tensor) {
  return t81::tensor_contracts::softmax_compatible(tensor);
}

t81::T729DynamicTensor tensor_matmul_2d(const t81::T729DynamicTensor& lhs,
                                        const t81::T729DynamicTensor& rhs) {
  return t81::ops::matmul(lhs, rhs);
}

bool tensor_matmul_compatible(const t81::T729DynamicTensor& lhs,
                              const t81::T729DynamicTensor& rhs) {
  return t81::tensor_contracts::matmul_compatible(lhs, rhs);
}

t81::T729DynamicTensor tensor_binary_elementwise(const t81::T729DynamicTensor& lhs,
                                                 const t81::T729DynamicTensor& rhs, bool multiply) {
  return multiply ? t81::ops::mul(lhs, rhs) : t81::ops::add(lhs, rhs);
}

bool tensor_transpose_2d_compatible(const t81::T729DynamicTensor& tensor) {
  return t81::tensor_contracts::transpose_2d_compatible(tensor);
}

t81::T729DynamicTensor tensor_transpose_2d(const t81::T729DynamicTensor& tensor) {
  return t81::ops::transpose(tensor);
}

std::optional<t81::T729DynamicTensor> tensor_contract_dot(const t81::T729DynamicTensor& lhs,
                                                          const t81::T729DynamicTensor& rhs) {
  try {
    return t81::ops::contract_dot(lhs, rhs);
  } catch (...) {
    return std::nullopt;
  }
}

t81::T729DynamicTensor tensor_rmsnorm(const t81::T729DynamicTensor& tensor,
                                      const t81::T729DynamicTensor& weights) {
  auto out = t81::ops::rmsnorm(tensor, weights);
  out.set_numeric_class(TensorNumericClass::HostFloat);
  return out;
}

bool tensor_rmsnorm_compatible(const t81::T729DynamicTensor& tensor,
                               const t81::T729DynamicTensor& weights) {
  return t81::tensor_contracts::rmsnorm_compatible(tensor, weights);
}

t81::T729DynamicTensor tensor_rope(const t81::T729DynamicTensor& tensor, int pos) {
  auto out = t81::ops::rope(tensor, pos);
  out.set_numeric_class(TensorNumericClass::HostFloat);
  return out;
}

bool tensor_rope_compatible(const t81::T729DynamicTensor& tensor) {
  return t81::tensor_contracts::rope_compatible(tensor);
}

bool tensor_attention_compatible(const t81::T729DynamicTensor& q, const t81::T729DynamicTensor& k,
                                 const t81::T729DynamicTensor& v) {
  return t81::tensor_contracts::attention_compatible(q, k, v);
}

bool tensor_embed_compatible(const t81::T729DynamicTensor& table, std::int64_t index) {
  return t81::tensor_contracts::embed_compatible(table, index);
}

std::optional<t81::T729DynamicTensor> tensor_new_1d(std::int64_t size) {
  if (size <= 0) {
    return std::nullopt;
  }
  std::vector<int> shape = {static_cast<int>(size)};
  return t81::T729DynamicTensor(shape);
}

t81::T729DynamicTensor tensor_identity_copy(const t81::T729DynamicTensor& tensor) {
  return t81::tensor_mutation::identity_copy(tensor);
}

std::optional<float> tensor_get_at(const t81::T729DynamicTensor& tensor, std::int64_t index) {
  return t81::tensor_mutation::read_scalar(tensor, index);
}

bool tensor_set_at(t81::T729DynamicTensor& tensor, std::int64_t index, float value,
                   t81::tensor_mutation::ScalarWriteKind source_kind) {
  return t81::tensor_mutation::write_scalar(tensor, index, value, source_kind);
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
  try {
    return t81::ops::attention(q, k, v);
  } catch (...) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::ShapeFault);
  }
}

std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_embed_checked(
    const t81::T729DynamicTensor& table, std::int64_t index) {
  if (!tensor_embed_compatible(table, index)) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                 t81::vm::Trap::BoundsFault);
  }
  try {
    return t81::ops::embed(table, index);
  } catch (const std::out_of_range&) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                 t81::vm::Trap::BoundsFault);
  } catch (...) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                 t81::vm::Trap::ShapeFault);
  }
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
                                                      std::int64_t index, float value,
                                                      t81::tensor_mutation::ScalarWriteKind source_kind) {
  if (!tensor_set_at(tensor, index, value, source_kind)) {
    return std::expected<void, t81::vm::Trap>(t81::unexpect, t81::vm::Trap::BoundsFault);
  }
  return {};
}

}  // namespace t81::vm::internal
