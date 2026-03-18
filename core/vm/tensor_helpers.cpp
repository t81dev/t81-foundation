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
#include "t81/swar/swar.hpp"
#include "t81/tensor/transpose.hpp"
#include "t81/tensor/unary.hpp"
#include "t81/tensor/contracts.hpp"
#include "t81/tensor/mutation.hpp"
#include "t81/tracing/canonhash.hpp"
#include "t81/types/detail/dmath.hpp"

namespace t81::vm::internal {

namespace {

using t81::core::detail::DFixed;
using t81::experimental::ComputeTritVector;

std::expected<ComputeTritVector, t81::vm::Trap> encode_exact_trit_tensor(
    const t81::T729DynamicTensor& tensor) {
  if (tensor.numeric_class() != t81::TensorNumericClass::ExactTrit) {
    return std::expected<ComputeTritVector, t81::vm::Trap>(t81::unexpect, t81::vm::Trap::TypeFault);
  }

  const auto values = tensor.snapshot_values();
  std::vector<int8_t> trits;
  trits.reserve(values.size());
  for (float value : values) {
    if (value == -1.0f) {
      trits.push_back(-1);
    } else if (value == 0.0f) {
      trits.push_back(0);
    } else if (value == 1.0f) {
      trits.push_back(1);
    } else {
      return std::expected<ComputeTritVector, t81::vm::Trap>(t81::unexpect,
                                                             t81::vm::Trap::TypeFault);
    }
  }

  auto encoded = ComputeTritVector::from_trits(trits);
  if (encoded.is_err()) {
    return std::expected<ComputeTritVector, t81::vm::Trap>(t81::unexpect, t81::vm::Trap::TypeFault);
  }
  return encoded.value();
}

std::expected<t81::T729DynamicTensor, t81::vm::Trap> decode_exact_trit_tensor(
    const ComputeTritVector& vector, const std::vector<int>& shape) {
  auto trits = vector.to_trits();
  if (trits.is_err()) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::DecodeFault);
  }

  std::vector<float> values;
  values.reserve(trits.value().size());
  for (int8_t trit : trits.value()) {
    values.push_back(static_cast<float>(trit));
  }

  t81::T729DynamicTensor tensor(shape, std::move(values));
  tensor.set_numeric_class(t81::TensorNumericClass::ExactTrit);
  return tensor;
}

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

std::optional<t81::T729DynamicTensor> native_tensor_unary_exp_direct(
    const t81::weights::NativeTensor& native) {
  using TensorFloat = t81::v1::T81Float<72, 9>;

  if (native.format != t81::weights::NativeFormat::BalancedTernary) {
    return std::nullopt;
  }

  static const float kExpNegOne = static_cast<float>(
      t81::core::detail::exp(TensorFloat::from_double(-1.0)).to_double());
  static const float kExpZero = 1.0f;
  static const float kExpOne = static_cast<float>(
      t81::core::detail::exp(TensorFloat::from_double(1.0)).to_double());

  uint64_t remaining = native.trits;
  if (remaining == 0 && !native.data.empty()) {
    remaining = static_cast<uint64_t>(native.data.size()) * 48;
  }

  std::vector<float> out;
  out.reserve(static_cast<std::size_t>(remaining));
  for (uint64_t limb : native.data) {
    const uint64_t count = std::min<uint64_t>(48, remaining);
    std::vector<float> block(static_cast<std::size_t>(count), kExpZero);
    uint64_t val = limb;
    for (int i = 47; i >= 0; --i) {
      const uint64_t digit = val % 3;
      val /= 3;
      if (static_cast<uint64_t>(i) >= count) {
        continue;
      }
      switch (digit) {
        case 0:
          block[static_cast<std::size_t>(i)] = kExpNegOne;
          break;
        case 1:
          block[static_cast<std::size_t>(i)] = kExpZero;
          break;
        case 2:
          block[static_cast<std::size_t>(i)] = kExpOne;
          break;
        default:
          return std::nullopt;
      }
    }
    out.insert(out.end(), block.begin(), block.end());
    remaining -= count;
    if (remaining == 0) {
      break;
    }
  }

  std::vector<int> shape;
  shape.reserve(native.shape.size());
  for (auto dim : native.shape) {
    shape.push_back(static_cast<int>(dim));
  }
  auto tensor = t81::T729DynamicTensor::from_host_float_data(std::move(shape), std::move(out));
  tensor.set_numeric_class(t81::TensorNumericClass::ExactInt);
  return tensor;
}

std::optional<t81::T729DynamicTensor> native_tensor_unary_silu_direct(
    const t81::weights::NativeTensor& native) {
  using TensorFloat = t81::v1::T81Float<72, 9>;

  if (native.format != t81::weights::NativeFormat::BalancedTernary) {
    return std::nullopt;
  }

  static const float kExpNegOne = static_cast<float>(
      t81::core::detail::exp(TensorFloat::from_double(-1.0)).to_double());
  static const float kExpPosOne = static_cast<float>(
      t81::core::detail::exp(TensorFloat::from_double(1.0)).to_double());
  static const float kSiluNegOne = -1.0f / (1.0f + kExpPosOne);
  static const float kSiluZero = 0.0f;
  static const float kSiluOne = 1.0f / (1.0f + kExpNegOne);

  uint64_t remaining = native.trits;
  if (remaining == 0 && !native.data.empty()) {
    remaining = static_cast<uint64_t>(native.data.size()) * 48;
  }

  std::vector<float> out;
  out.reserve(static_cast<std::size_t>(remaining));
  for (uint64_t limb : native.data) {
    const uint64_t count = std::min<uint64_t>(48, remaining);
    std::vector<float> block(static_cast<std::size_t>(count), kSiluZero);
    uint64_t val = limb;
    for (int i = 47; i >= 0; --i) {
      const uint64_t digit = val % 3;
      val /= 3;
      if (static_cast<uint64_t>(i) >= count) {
        continue;
      }
      switch (digit) {
        case 0:
          block[static_cast<std::size_t>(i)] = kSiluNegOne;
          break;
        case 1:
          block[static_cast<std::size_t>(i)] = kSiluZero;
          break;
        case 2:
          block[static_cast<std::size_t>(i)] = kSiluOne;
          break;
        default:
          return std::nullopt;
      }
    }
    out.insert(out.end(), block.begin(), block.end());
    remaining -= count;
    if (remaining == 0) {
      break;
    }
  }

  std::vector<int> shape;
  shape.reserve(native.shape.size());
  for (auto dim : native.shape) {
    shape.push_back(static_cast<int>(dim));
  }
  auto tensor = t81::T729DynamicTensor::from_host_float_data(std::move(shape), std::move(out));
  tensor.set_numeric_class(t81::TensorNumericClass::ExactInt);
  return tensor;
}

std::optional<t81::T729DynamicTensor> native_tensor_unary_softmax_direct(
    const t81::weights::NativeTensor& native) {
  using TensorFloat = t81::v1::T81Float<72, 9>;

  if (native.format != t81::weights::NativeFormat::BalancedTernary || native.shape.empty()) {
    return std::nullopt;
  }

  int dim = static_cast<int>(native.shape.back());
  if (dim <= 0) {
    return std::nullopt;
  }

  static const float kExpNegTwo = static_cast<float>(
      t81::core::detail::exp(TensorFloat::from_double(-2.0)).to_double());
  static const float kExpNegOne = static_cast<float>(
      t81::core::detail::exp(TensorFloat::from_double(-1.0)).to_double());
  static const float kExpZero = 1.0f;

  uint64_t remaining = native.trits;
  if (remaining == 0 && !native.data.empty()) {
    remaining = static_cast<uint64_t>(native.data.size()) * 48;
  }

  std::vector<float> trits;
  trits.reserve(static_cast<std::size_t>(remaining));
  for (uint64_t limb : native.data) {
    const uint64_t count = std::min<uint64_t>(48, remaining);
    std::vector<float> block(static_cast<std::size_t>(count), 0.0f);
    uint64_t val = limb;
    for (int i = 47; i >= 0; --i) {
      const uint64_t digit = val % 3;
      val /= 3;
      if (static_cast<uint64_t>(i) >= count) {
        continue;
      }
      switch (digit) {
        case 0:
          block[static_cast<std::size_t>(i)] = -1.0f;
          break;
        case 1:
          block[static_cast<std::size_t>(i)] = 0.0f;
          break;
        case 2:
          block[static_cast<std::size_t>(i)] = 1.0f;
          break;
        default:
          return std::nullopt;
      }
    }
    trits.insert(trits.end(), block.begin(), block.end());
    remaining -= count;
    if (remaining == 0) {
      break;
    }
  }

  std::vector<float> out(trits);
  for (std::size_t base = 0; base < out.size(); base += static_cast<std::size_t>(dim)) {
    float max_val = out[base];
    for (int j = 1; j < dim; ++j) {
      max_val = std::max(max_val, out[base + static_cast<std::size_t>(j)]);
    }
    float sum = 0.0f;
    for (int j = 0; j < dim; ++j) {
      const float shifted = out[base + static_cast<std::size_t>(j)] - max_val;
      float exp_val = kExpZero;
      if (shifted <= -1.5f) {
        exp_val = kExpNegTwo;
      } else if (shifted <= -0.5f) {
        exp_val = kExpNegOne;
      }
      out[base + static_cast<std::size_t>(j)] = exp_val;
      sum += exp_val;
    }
    for (int j = 0; j < dim; ++j) {
      out[base + static_cast<std::size_t>(j)] /= sum;
    }
  }

  std::vector<int> shape;
  shape.reserve(native.shape.size());
  for (auto dim_value : native.shape) {
    shape.push_back(static_cast<int>(dim_value));
  }
  auto tensor = t81::T729DynamicTensor::from_host_float_data(std::move(shape), std::move(out));
  tensor.set_numeric_class(t81::TensorNumericClass::ExactInt);
  return tensor;
}

std::optional<t81::T729DynamicTensor> native_tensor_rmsnorm_direct(
    const t81::weights::NativeTensor& native, const t81::T729DynamicTensor& weights) {
  if (native.format != t81::weights::NativeFormat::BalancedTernary || native.shape.empty() ||
      weights.rank() != 1) {
    return std::nullopt;
  }

  const int dim = static_cast<int>(native.shape.back());
  if (dim <= 0 || weights.shape()[0] != dim) {
    return std::nullopt;
  }

  std::size_t element_count = 1;
  std::vector<int> shape;
  shape.reserve(native.shape.size());
  for (auto native_dim : native.shape) {
    if (native_dim == 0) {
      return std::nullopt;
    }
    shape.push_back(static_cast<int>(native_dim));
    element_count *= static_cast<std::size_t>(native_dim);
  }
  if (element_count % static_cast<std::size_t>(dim) != 0) {
    return std::nullopt;
  }

  const auto weight_values = weights.snapshot_values();
  if (weight_values.size() != static_cast<std::size_t>(dim)) {
    return std::nullopt;
  }

  std::vector<float> out(element_count, 0.0f);
  uint64_t remaining = native.trits;
  if (remaining == 0 && !native.data.empty()) {
    remaining = static_cast<uint64_t>(native.data.size()) * 48;
  }
  if (remaining < element_count) {
    return std::nullopt;
  }

  std::vector<float> row_values;
  row_values.reserve(static_cast<std::size_t>(dim));
  std::size_t out_index = 0;
  for (uint64_t limb : native.data) {
    const uint64_t count = std::min<uint64_t>(48, remaining);
    std::vector<float> block(static_cast<std::size_t>(count), 0.0f);
    uint64_t val = limb;
    for (int i = 47; i >= 0; --i) {
      const uint64_t digit = val % 3;
      val /= 3;
      if (static_cast<uint64_t>(i) >= count) {
        continue;
      }
      switch (digit) {
        case 0:
          block[static_cast<std::size_t>(i)] = -1.0f;
          break;
        case 1:
          block[static_cast<std::size_t>(i)] = 0.0f;
          break;
        case 2:
          block[static_cast<std::size_t>(i)] = 1.0f;
          break;
        default:
          return std::nullopt;
      }
    }

    for (float trit : block) {
      if (out_index >= element_count) {
        break;
      }
      row_values.push_back(trit);
      if (row_values.size() == static_cast<std::size_t>(dim)) {
        int nonzero_count = 0;
        for (float value : row_values) {
          if (value != 0.0f) {
            ++nonzero_count;
          }
        }
        const float mean_ss =
            static_cast<float>(nonzero_count) / static_cast<float>(dim) + 1e-6f;
        const float inv_ss = t81::ops::detail::deterministic_inv_sqrt(mean_ss);
        const std::size_t row_base = out_index + 1 - static_cast<std::size_t>(dim);
        for (int j = 0; j < dim; ++j) {
          out[row_base + static_cast<std::size_t>(j)] =
              (row_values[static_cast<std::size_t>(j)] * inv_ss) *
              weight_values[static_cast<std::size_t>(j)];
        }
        row_values.clear();
      }
      ++out_index;
    }

    remaining -= count;
    if (out_index >= element_count || remaining == 0) {
      break;
    }
  }

  if (!row_values.empty() || out_index != element_count) {
    return std::nullopt;
  }

  auto tensor = t81::T729DynamicTensor::from_host_float_data(std::move(shape), std::move(out));
  tensor.set_numeric_class(weights.strict_core_eligible() ? t81::TensorNumericClass::ExactInt
                                                          : t81::TensorNumericClass::HostFloat);
  return tensor;
}

std::optional<t81::T729DynamicTensor> native_tensor_rope_direct(
    const t81::weights::NativeTensor& native, int pos) {
  using TensorFloat = t81::ops::detail::TensorFloat;

  if (native.format != t81::weights::NativeFormat::BalancedTernary || native.shape.size() < 2) {
    return std::nullopt;
  }

  const int head_dim = static_cast<int>(native.shape.back());
  if (head_dim <= 0 || (head_dim % 2) != 0) {
    return std::nullopt;
  }

  std::size_t element_count = 1;
  std::vector<int> shape;
  shape.reserve(native.shape.size());
  for (auto native_dim : native.shape) {
    if (native_dim == 0) {
      return std::nullopt;
    }
    shape.push_back(static_cast<int>(native_dim));
    element_count *= static_cast<std::size_t>(native_dim);
  }
  if (element_count % static_cast<std::size_t>(head_dim) != 0) {
    return std::nullopt;
  }

  uint64_t remaining = native.trits;
  if (remaining == 0 && !native.data.empty()) {
    remaining = static_cast<uint64_t>(native.data.size()) * 48;
  }
  if (remaining < element_count) {
    return std::nullopt;
  }

  std::vector<float> data;
  data.reserve(element_count);
  for (uint64_t limb : native.data) {
    const uint64_t count = std::min<uint64_t>(48, remaining);
    std::vector<float> block(static_cast<std::size_t>(count), 0.0f);
    uint64_t val = limb;
    for (int i = 47; i >= 0; --i) {
      const uint64_t digit = val % 3;
      val /= 3;
      if (static_cast<uint64_t>(i) >= count) {
        continue;
      }
      switch (digit) {
        case 0:
          block[static_cast<std::size_t>(i)] = -1.0f;
          break;
        case 1:
          block[static_cast<std::size_t>(i)] = 0.0f;
          break;
        case 2:
          block[static_cast<std::size_t>(i)] = 1.0f;
          break;
        default:
          return std::nullopt;
      }
    }
    data.insert(data.end(), block.begin(), block.end());
    remaining -= count;
    if (data.size() >= element_count || remaining == 0) {
      break;
    }
  }
  if (data.size() < element_count) {
    return std::nullopt;
  }
  data.resize(element_count);

  const TensorFloat freq_base = TensorFloat::from_double(10000.0);
  const TensorFloat pos_float = TensorFloat::from_double(static_cast<double>(pos));
  const TensorFloat head_dim_float = TensorFloat::from_double(static_cast<double>(head_dim));
  std::vector<float> cos_terms(static_cast<std::size_t>(head_dim / 2));
  std::vector<float> sin_terms(static_cast<std::size_t>(head_dim / 2));
  for (int j = 0; j < head_dim; j += 2) {
    const TensorFloat exponent = TensorFloat::from_double(static_cast<double>(j)) / head_dim_float;
    const TensorFloat freq =
        TensorFloat::from_double(1.0) / t81::core::detail::pow(freq_base, exponent);
    const TensorFloat angle = pos_float * freq;
    cos_terms[static_cast<std::size_t>(j / 2)] =
        static_cast<float>(t81::core::detail::cos(angle).to_double());
    sin_terms[static_cast<std::size_t>(j / 2)] =
        static_cast<float>(t81::core::detail::sin(angle).to_double());
  }

  for (std::size_t base = 0; base < data.size(); base += static_cast<std::size_t>(head_dim)) {
    for (int j = 0; j < head_dim; j += 2) {
      const float f_cos = cos_terms[static_cast<std::size_t>(j / 2)];
      const float f_sin = sin_terms[static_cast<std::size_t>(j / 2)];
      float v0 = data[base + static_cast<std::size_t>(j)];
      float v1 = data[base + static_cast<std::size_t>(j + 1)];
      data[base + static_cast<std::size_t>(j)] = v0 * f_cos - v1 * f_sin;
      data[base + static_cast<std::size_t>(j + 1)] = v0 * f_sin + v1 * f_cos;
    }
  }

  auto tensor = t81::T729DynamicTensor::from_host_float_data(std::move(shape), std::move(data));
  tensor.set_numeric_class(t81::TensorNumericClass::ExactInt);
  return tensor;
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
  return t81::ops::exp(tensor);
}

t81::T729DynamicTensor tensor_unary_sqrt(const t81::T729DynamicTensor& tensor) {
  return t81::ops::sqrt(tensor);
}

t81::T729DynamicTensor tensor_unary_silu(const t81::T729DynamicTensor& tensor) {
  return t81::ops::silu(tensor);
}

t81::T729DynamicTensor tensor_unary_softmax(const t81::T729DynamicTensor& tensor) {
  return t81::ops::softmax(tensor);
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
  return t81::ops::rmsnorm(tensor, weights);
}

bool tensor_rmsnorm_compatible(const t81::T729DynamicTensor& tensor,
                               const t81::T729DynamicTensor& weights) {
  return t81::tensor_contracts::rmsnorm_compatible(tensor, weights);
}

t81::T729DynamicTensor tensor_rope(const t81::T729DynamicTensor& tensor, int pos) {
  return t81::ops::rope(tensor, pos);
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

std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_vec_sub_checked(
    const t81::T729DynamicTensor& lhs, const t81::T729DynamicTensor& rhs) {
  if (!tensor_elementwise_compatible(lhs, rhs)) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::ShapeFault);
  }
  return t81::ops::sub(lhs, rhs);
}

std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_swar_not_checked(
    const t81::T729DynamicTensor& tensor) {
  auto encoded = encode_exact_trit_tensor(tensor);
  if (!encoded) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect, encoded.error());
  }

  auto result = t81::swar::t_not_swar(*encoded);
  if (result.is_err()) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::DecodeFault);
  }
  return decode_exact_trit_tensor(result.value(), tensor.shape());
}

std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_swar_and_checked(
    const t81::T729DynamicTensor& lhs, const t81::T729DynamicTensor& rhs) {
  if (!tensor_elementwise_compatible(lhs, rhs)) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::ShapeFault);
  }

  auto lhs_encoded = encode_exact_trit_tensor(lhs);
  if (!lhs_encoded) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect, lhs_encoded.error());
  }
  auto rhs_encoded = encode_exact_trit_tensor(rhs);
  if (!rhs_encoded) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect, rhs_encoded.error());
  }

  auto result = t81::swar::t_and_swar(*lhs_encoded, *rhs_encoded);
  if (result.is_err()) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::DecodeFault);
  }
  return decode_exact_trit_tensor(result.value(), lhs.shape());
}

std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_swar_or_checked(
    const t81::T729DynamicTensor& lhs, const t81::T729DynamicTensor& rhs) {
  if (!tensor_elementwise_compatible(lhs, rhs)) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::ShapeFault);
  }

  auto lhs_encoded = encode_exact_trit_tensor(lhs);
  if (!lhs_encoded) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect, lhs_encoded.error());
  }
  auto rhs_encoded = encode_exact_trit_tensor(rhs);
  if (!rhs_encoded) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect, rhs_encoded.error());
  }

  auto result = t81::swar::t_or_swar(*lhs_encoded, *rhs_encoded);
  if (result.is_err()) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::DecodeFault);
  }
  return decode_exact_trit_tensor(result.value(), lhs.shape());
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

std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_wload_checked(
    const t81::T729DynamicTensor& src) {
  if (!t81::tensor_contracts::wload_compatible(src)) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::TypeFault);
  }
  try {
    return t81::ops::wload(src);
  } catch (...) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::TypeFault);
  }
}

std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_gather_checked(
    const t81::T729DynamicTensor& src, std::int64_t index, int axis) {
  if (!t81::tensor_contracts::gather_compatible(src, index, axis)) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::BoundsFault);
  }
  try {
    return t81::ops::gather(src, index, axis);
  } catch (const std::out_of_range&) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::BoundsFault);
  } catch (...) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::ShapeFault);
  }
}

std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_scatter_checked(
    const t81::T729DynamicTensor& dst, std::int64_t index, const t81::T729DynamicTensor& src,
    int axis) {
  if (!t81::tensor_contracts::scatter_compatible(dst, index, axis, src)) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::BoundsFault);
  }
  try {
    return t81::ops::scatter_add(dst, index, src, axis);
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

// ---------------------------------------------------------------------------
// RFC-0005 v0.4 vector helpers
// ---------------------------------------------------------------------------

std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_vload_checked(
    const t81::T729DynamicTensor& src, const std::vector<int>& new_shape) {
  // Count elements in the target shape.
  std::size_t new_count = 1;
  for (int d : new_shape) {
    if (d <= 0) {
      return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                  t81::vm::Trap::ShapeFault);
    }
    new_count *= static_cast<std::size_t>(d);
  }
  if (src.size() != new_count) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::ShapeFault);
  }
  // Materialise all element values and construct tensor with new shape.
  std::vector<float> flat;
  flat.reserve(new_count);
  for (std::size_t i = 0; i < new_count; ++i) {
    auto v = src.value_at(i);
    flat.push_back(v.value_or(0.0f));
  }
  return t81::T729DynamicTensor(new_shape, std::move(flat));
}

std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_vstore_checked(
    const t81::T729DynamicTensor& src, const std::vector<int>& expected_shape) {
  if (src.shape() != expected_shape) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::ShapeFault);
  }
  return tensor_identity_copy(src);
}

std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_vfma_checked(
    const t81::T729DynamicTensor& accumulator, const t81::T729DynamicTensor& src1,
    const t81::T729DynamicTensor& src2) {
  if (!tensor_elementwise_compatible(src1, src2) ||
      !tensor_elementwise_compatible(accumulator, src1)) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::ShapeFault);
  }
  // result = src1 * src2 + accumulator
  auto product = t81::ops::mul(src1, src2);
  return t81::ops::add(accumulator, product);
}

}  // namespace t81::vm::internal
