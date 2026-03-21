#include "internal/tensor_helpers.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <sstream>

#ifdef __ARM_NEON
#  include <arm_neon.h>
#elif defined(__AVX2__)
#  include <immintrin.h>
#endif

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
#include "t81/tensor/ternary_native.hpp"
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

std::optional<std::vector<std::int8_t>> decode_balanced_ternary_trits(
    const t81::weights::NativeTensor& native) {
  if (native.format != t81::weights::NativeFormat::BalancedTernary) {
    return std::nullopt;
  }

  const std::size_t total_trits = native.num_trits();
  std::vector<std::int8_t> out(total_trits, 0);
  std::size_t out_offset = 0;
  uint64_t remaining = native.trits;
  if (remaining == 0 && !native.data.empty()) {
    remaining = static_cast<uint64_t>(native.data.size()) * 48;
  }
  if (remaining < total_trits) {
    return std::nullopt;
  }

  for (uint64_t limb : native.data) {
    const std::size_t count =
        static_cast<std::size_t>(std::min<uint64_t>(48, static_cast<uint64_t>(total_trits - out_offset)));
    uint64_t val = limb;
    for (int i = 47; i >= 0; --i) {
      const uint64_t digit = val % 3;
      val /= 3;
      if (static_cast<std::size_t>(i) >= count) {
        continue;
      }
      switch (digit) {
        case 0:
          out[out_offset + static_cast<std::size_t>(i)] = -1;
          break;
        case 1:
          out[out_offset + static_cast<std::size_t>(i)] = 0;
          break;
        case 2:
          out[out_offset + static_cast<std::size_t>(i)] = 1;
          break;
        default:
          return std::nullopt;
      }
    }
    out_offset += count;
    remaining -= std::min<uint64_t>(48, remaining);
    if (out_offset == total_trits) {
      break;
    }
  }

  if (out_offset != total_trits) {
    return std::nullopt;
  }
  return out;
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

std::optional<t81::T729DynamicTensor> native_tensor_quant_direct(
    const t81::weights::NativeTensor& native, const std::vector<std::int8_t>& trits,
    float threshold) {
  if (native.format != t81::weights::NativeFormat::BalancedTernary || threshold < 0.0f) {
    return std::nullopt;
  }

  const std::size_t total = native.num_trits();
  if (trits.size() < total) {
    return std::nullopt;
  }

  std::vector<int> shape;
  shape.reserve(native.shape.size());
  for (auto dim : native.shape) {
    shape.push_back(static_cast<int>(dim));
  }

  std::vector<float> out(total, 0.0f);
  if (threshold < 1.0f) {
    // int8 {-1,0,+1} → float: NEON 8-wide or AVX2 8-wide, scalar tail.
    std::size_t i = 0;
#if defined(__ARM_NEON) || defined(__AVX2__)
    const std::size_t total8 = (total / 8) * 8;
#endif
#ifdef __ARM_NEON
    for (; i < total8; i += 8) {
      const int8x8_t  sv   = vld1_s8(trits.data() + i);
      const int16x8_t sv16 = vmovl_s8(sv);
      vst1q_f32(out.data() + i,     vcvtq_f32_s32(vmovl_s16(vget_low_s16(sv16))));
      vst1q_f32(out.data() + i + 4, vcvtq_f32_s32(vmovl_s16(vget_high_s16(sv16))));
    }
#elif defined(__AVX2__)
    for (; i < total8; i += 8) {
      const __m128i sv   = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(trits.data() + i));
      _mm256_storeu_ps(out.data() + i, _mm256_cvtepi32_ps(_mm256_cvtepi8_epi32(sv)));
    }
#endif
    for (; i < total; ++i) {
      out[i] = static_cast<float>(trits[i]);
    }
  }

  auto tensor = t81::T729DynamicTensor::from_host_float_data(
      std::move(shape), std::move(out), t81::TensorNumericClass::ExactTrit);
  tensor.set_numeric_class(t81::TensorNumericClass::ExactTrit);
  return tensor;
}

std::optional<t81::T729DynamicTensor> native_tensor_tact_direct(
    const t81::weights::NativeTensor& native, const std::vector<std::int8_t>& trits,
    std::uint8_t mode) {
  if (native.format != t81::weights::NativeFormat::BalancedTernary ||
      (mode != t81::ops::kTActModeStep && mode != t81::ops::kTActModeTanh)) {
    return std::nullopt;
  }

  const std::size_t total = native.num_trits();
  if (trits.size() < total) {
    return std::nullopt;
  }

  std::vector<int> shape;
  shape.reserve(native.shape.size());
  for (auto dim : native.shape) {
    shape.push_back(static_cast<int>(dim));
  }

  // Apply activation function explicitly. For ExactTrit inputs {-1,0,+1},
  // snap_trit(tanh(±1)) = ±1 and snap_trit(tanh(0)) = 0, so both modes are
  // identity on pre-ternary values. Applied here for correctness in case the
  // native trit buffer ever contains out-of-range values.
  std::vector<float> out(total, 0.0f);
  for (std::size_t i = 0; i < total; ++i) {
    float x = static_cast<float>(trits[i]);
    if (mode == t81::ops::kTActModeTanh) {
      x = std::tanh(x);
    }
    out[i] = static_cast<float>(t81::ops::ternary_detail::snap_trit(x));
  }

  auto tensor = t81::T729DynamicTensor::from_host_float_data(
      std::move(shape), std::move(out), t81::TensorNumericClass::ExactTrit);
  tensor.set_numeric_class(t81::TensorNumericClass::ExactTrit);
  return tensor;
}

std::optional<t81::v1::T81BigInt> native_tensor_ternaccum_direct(
    const t81::weights::NativeTensor& native, const std::vector<std::int8_t>& trits,
    const t81::T729DynamicTensor& activations) {
  if (native.format != t81::weights::NativeFormat::BalancedTernary) {
    return std::nullopt;
  }

  const auto& act_vals = activations.data();
  const std::size_t total = native.num_trits();
  if (trits.size() < total || act_vals.size() != total) {
    return std::nullopt;
  }

  const bool exact_trit_activations =
      activations.numeric_class() == t81::TensorNumericClass::ExactTrit;

  // Fast path: ExactTrit activations — pre-snap to int8, then SIMD int8×int8
  // dot product into int32. Sum of N ternary products fits int32 for any N ≤ 2^30.
  // One T81BigInt is constructed at the end to preserve the audit-trail type.
  if (exact_trit_activations) {
    // Pre-snap float activations to int8 {-1, 0, +1}
    std::vector<std::int8_t> av_trits(total, 0);
    for (std::size_t i = 0; i < total; ++i) {
      const float v = act_vals[i];
      if (v == 1.0f)       av_trits[i] =  1;
      else if (v == -1.0f) av_trits[i] = -1;
    }

    int32_t dot = 0;
    std::size_t i = 0;
#if defined(__ARM_NEON) || defined(__AVX2__)
    const std::size_t total8 = (total / 8) * 8;
#endif
#ifdef __ARM_NEON
    // 8-wide: vmull_s8 → int16x8, vpaddlq_s16 → int32x4, vaddvq_s32 at end.
    int32x4_t acc4 = vdupq_n_s32(0);
    for (; i < total8; i += 8) {
      const int8x8_t wv = vld1_s8(trits.data() + i);
      const int8x8_t av = vld1_s8(av_trits.data() + i);
      acc4 = vaddq_s32(acc4, vpaddlq_s16(vmull_s8(wv, av)));
    }
    dot = vaddvq_s32(acc4);
#elif defined(__AVX2__)
    // 8-wide: cvtepi8_epi32 + mullo_epi32, horizontal sum at end.
    __m256i acc8 = _mm256_setzero_si256();
    for (; i < total8; i += 8) {
      const __m128i wv = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(trits.data() + i));
      const __m128i av = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(av_trits.data() + i));
      acc8 = _mm256_add_epi32(acc8,
          _mm256_mullo_epi32(_mm256_cvtepi8_epi32(wv), _mm256_cvtepi8_epi32(av)));
    }
    {
      const __m128i lo = _mm256_extracti128_si256(acc8, 0);
      const __m128i hi = _mm256_extracti128_si256(acc8, 1);
      const __m128i s  = _mm_add_epi32(lo, hi);
      const __m128i s2 = _mm_hadd_epi32(s, s);
      dot = _mm_extract_epi32(_mm_hadd_epi32(s2, s2), 0);
    }
#endif
    // Scalar tail (also full loop on non-SIMD platforms)
    for (; i < total; ++i) {
      dot += static_cast<int32_t>(trits[i]) * static_cast<int32_t>(av_trits[i]);
    }
    return t81::v1::T81BigInt(static_cast<std::int64_t>(dot));
  }

  // General path: float activations with snap_trit — T81BigInt per element.
  t81::v1::T81BigInt acc(static_cast<std::int64_t>(0));
  for (std::size_t i = 0; i < total; ++i) {
    const int wt = trits[i];
    if (wt == 0) continue;
    const int av = t81::ops::ternary_detail::snap_trit(act_vals[i]);
    if (av == 0) continue;
    if (wt > 0) {
      acc = acc + t81::v1::T81BigInt(static_cast<std::int64_t>(av));
    } else {
      acc = acc - t81::v1::T81BigInt(static_cast<std::int64_t>(av));
    }
  }
  return acc;
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

std::optional<t81::T729DynamicTensor> native_tensor_twembed_direct(
    const t81::weights::NativeTensor& native, std::int64_t index) {
  if (native.format != t81::weights::NativeFormat::BalancedTernary || native.shape.size() != 2) {
    return std::nullopt;
  }

  const std::int64_t rows = static_cast<std::int64_t>(native.shape[0]);
  const std::size_t cols = static_cast<std::size_t>(native.shape[1]);
  if (rows <= 0 || cols == 0 || index < 0 || index >= rows) {
    return std::nullopt;
  }

  const std::size_t row_offset = static_cast<std::size_t>(index) * cols;
  const std::size_t row_end = row_offset + cols;
  uint64_t remaining = native.trits;
  if (remaining == 0 && !native.data.empty()) {
    remaining = static_cast<uint64_t>(native.data.size()) * 48;
  }
  if (remaining < row_end) {
    return std::nullopt;
  }

  std::vector<float> row;
  row.reserve(cols);
  std::size_t global_offset = 0;
  for (uint64_t limb : native.data) {
    const std::size_t count =
        static_cast<std::size_t>(std::min<uint64_t>(48, remaining));
    if (global_offset >= row_end) {
      break;
    }
    if (global_offset + count <= row_offset) {
      global_offset += count;
      remaining -= count;
      continue;
    }

    // Decode base-3 limb: digit {0,1,2} → {-1,0,+1} via branchless lookup.
    // The decode is inherently serial (each quotient feeds the next division),
    // but the lookup eliminates the branch per trit.
    static constexpr float kDigitToTrit[3] = {-1.0f, 0.0f, 1.0f};
    std::vector<float> block(count, 0.0f);
    uint64_t val = limb;
    for (int i = 47; i >= 0; --i) {
      const uint64_t digit = val % 3;
      val /= 3;
      if (digit > 2) {
        return std::nullopt;
      }
      if (static_cast<std::size_t>(i) < count) {
        block[static_cast<std::size_t>(i)] = kDigitToTrit[digit];
      }
    }

    const std::size_t local_begin = row_offset > global_offset ? row_offset - global_offset : 0;
    const std::size_t local_end = std::min(count, row_end - global_offset);
    row.insert(row.end(), block.begin() + static_cast<std::ptrdiff_t>(local_begin),
               block.begin() + static_cast<std::ptrdiff_t>(local_end));

    global_offset += count;
    remaining -= count;
  }

  if (row.size() != cols) {
    return std::nullopt;
  }

  auto tensor = t81::T729DynamicTensor::from_host_float_data(
      {1, static_cast<int>(cols)}, std::move(row), t81::TensorNumericClass::ExactTrit);
  tensor.set_numeric_class(t81::TensorNumericClass::ExactTrit);
  return tensor;
}

std::optional<t81::T729DynamicTensor> native_tensor_twmatmul_direct(
    const t81::T729DynamicTensor& activations, const t81::weights::NativeTensor& weights,
    const std::vector<std::int8_t>& weight_trits) {
  if (weights.format != t81::weights::NativeFormat::BalancedTernary ||
      activations.rank() != 2 || weights.shape.size() != 2) {
    return std::nullopt;
  }

  const int m = activations.shape()[0];
  const int k = activations.shape()[1];
  const int kw = static_cast<int>(weights.shape[0]);
  const int n = static_cast<int>(weights.shape[1]);
  if (m <= 0 || k <= 0 || kw <= 0 || n <= 0 || k != kw) {
    return std::nullopt;
  }

  const auto& act_vals = activations.data();
  if (act_vals.size() != static_cast<std::size_t>(m) * static_cast<std::size_t>(k)) {
    return std::nullopt;
  }
  const bool exact_trit_activations =
      activations.numeric_class() == t81::TensorNumericClass::ExactTrit;

  const std::size_t weight_elements = static_cast<std::size_t>(kw) * static_cast<std::size_t>(n);
  if (weight_trits.size() < weight_elements) {
    return std::nullopt;
  }

  auto weight_row_ptr = [&](int row_index) -> const std::int8_t* {
    const std::size_t row_base = static_cast<std::size_t>(row_index) * static_cast<std::size_t>(n);
    return weight_trits.data() + row_base;
  };

  // Loop order: i → p → j
  //   - Sequential activation access: act_vals[i*k + 0], [i*k + 1], ...
  //   - Sequential weight row access: weight_trits[p*n + 0], [p*n + 1], ...
  //   - Output row out[i*n..] stays warm in L1 across the full p reduction
  // Previously p→i→j: strided activation access + output reloaded each p step.
  std::vector<float> out(static_cast<std::size_t>(m) * static_cast<std::size_t>(n), 0.0f);
#if defined(__ARM_NEON)
  const int n4 = (n / 4) * 4;  // 4-wide NEON lane count
#elif defined(__AVX2__)
  const int n8 = (n / 8) * 8;  // 8-wide AVX2 lane count
#endif
  for (int i = 0; i < m; ++i) {
    float* const orow = out.data() + static_cast<std::size_t>(i) * static_cast<std::size_t>(n);
    for (int p = 0; p < k; ++p) {
      const float act_value =
          act_vals[static_cast<std::size_t>(i) * static_cast<std::size_t>(k) +
                   static_cast<std::size_t>(p)];
      int av = 0;
      if (exact_trit_activations) {
        if (act_value == -1.0f) av = -1;
        else if (act_value == 1.0f) av = 1;
      } else {
        av = t81::ops::ternary_detail::snap_trit(act_value);
      }
      if (av == 0) continue;

      const std::int8_t* const wrow = weight_trits.data() +
                                      static_cast<std::size_t>(p) *
                                      static_cast<std::size_t>(n);
      int j = 0;
#ifdef __ARM_NEON
      // 4-wide NEON: convert int8 trits to float, select ±av or 0.
      // No vmulq_f32 — ternary constraint eliminates all multiplies.
      const float32x4_t fav4  = vdupq_n_f32(static_cast<float>(av));
      const float32x4_t fav4n = vnegq_f32(fav4);
      const float32x4_t zero4 = vdupq_n_f32(0.0f);
      for (; j < n4; j += 4) {
        // int8x8 → int16x8 → int32x4 → float32x4
        const int8x8_t  wv8  = vld1_s8(wrow + j);
        const int32x4_t wv32 = vmovl_s16(vget_low_s16(vmovl_s8(wv8)));
        const float32x4_t wf = vcvtq_f32_s32(wv32);
        const uint32x4_t pos = vcgtq_f32(wf, zero4);
        const uint32x4_t neg = vcltq_f32(wf, zero4);
        float32x4_t ov = vld1q_f32(orow + j);
        ov = vaddq_f32(ov, vbslq_f32(pos, fav4, vbslq_f32(neg, fav4n, zero4)));
        vst1q_f32(orow + j, ov);
      }
#elif defined(__AVX2__)
      // 8-wide AVX2: int8 trits → float, blendv select ±av or 0.
      // No _mm256_mul_ps — ternary constraint eliminates all multiplies.
      const __m256 fav8  = _mm256_set1_ps(static_cast<float>(av));
      const __m256 fav8n = _mm256_sub_ps(_mm256_setzero_ps(), fav8);
      const __m256 zero8 = _mm256_setzero_ps();
      for (; j < n8; j += 8) {
        // Load 8 int8 trits, sign-extend to int32, convert to float
        const __m128i wv8i = _mm_loadl_epi64(
            reinterpret_cast<const __m128i*>(wrow + j));
        const __m256i wv32 = _mm256_cvtepi8_epi32(wv8i);
        const __m256  wf   = _mm256_cvtepi32_ps(wv32);
        const __m256  pos  = _mm256_cmp_ps(wf, zero8, _CMP_GT_OQ);
        const __m256  neg  = _mm256_cmp_ps(wf, zero8, _CMP_LT_OQ);
        // contrib = pos ? +av : (neg ? -av : 0)
        const __m256 contrib = _mm256_blendv_ps(
            _mm256_blendv_ps(zero8, fav8n, neg), fav8, pos);
        __m256 ov = _mm256_loadu_ps(orow + j);
        ov = _mm256_add_ps(ov, contrib);
        _mm256_storeu_ps(orow + j, ov);
      }
#endif
      for (; j < n; ++j) {
        const int wt = static_cast<int>(wrow[static_cast<std::size_t>(j)]);
        if (wt != 0) orow[static_cast<std::size_t>(j)] += static_cast<float>(wt * av);
      }
    }
  }

  auto tensor = t81::T729DynamicTensor::from_host_float_data(
      {m, n}, std::move(out), t81::TensorNumericClass::ExactInt);
  tensor.set_numeric_class(t81::TensorNumericClass::ExactInt);
  return tensor;
}

std::optional<t81::T729DynamicTensor> native_tensor_tattn_direct(
    const t81::T729DynamicTensor& q, const t81::weights::NativeTensor& k_native,
    const std::vector<std::int8_t>& k_trits, const t81::T729DynamicTensor& v) {
  if (k_native.format != t81::weights::NativeFormat::BalancedTernary || q.rank() != 2 ||
      v.rank() != 2 || k_native.shape.size() != 2) {
    return std::nullopt;
  }

  const int seq_q = q.shape()[0];
  const int head_q = q.shape()[1];
  const int seq_k = static_cast<int>(k_native.shape[0]);
  const int head_k = static_cast<int>(k_native.shape[1]);
  const int seq_v = v.shape()[0];
  const int head_v = v.shape()[1];
  if (seq_q <= 0 || head_q <= 0 || seq_k <= 0 || head_k <= 0 || seq_v <= 0 || head_v <= 0 ||
      head_q != head_k || seq_k != seq_v) {
    return std::nullopt;
  }

  const auto& q_vals = q.data();
  const auto& v_vals = v.data();
  if (q_vals.size() != static_cast<std::size_t>(seq_q) * static_cast<std::size_t>(head_q) ||
      v_vals.size() != static_cast<std::size_t>(seq_v) * static_cast<std::size_t>(head_v) ||
      k_trits.size() <
          static_cast<std::size_t>(seq_k) * static_cast<std::size_t>(head_k)) {
    return std::nullopt;
  }

  std::vector<std::int8_t> q_trits(q_vals.size(), 0);
  if (q.numeric_class() == t81::TensorNumericClass::ExactTrit) {
    for (std::size_t idx = 0; idx < q_vals.size(); ++idx) {
      const float value = q_vals[idx];
      if (value == -1.0f) {
        q_trits[idx] = -1;
      } else if (value == 1.0f) {
        q_trits[idx] = 1;
      }
    }
  } else {
    for (std::size_t idx = 0; idx < q_vals.size(); ++idx) {
      q_trits[idx] = static_cast<std::int8_t>(t81::ops::ternary_detail::snap_trit(q_vals[idx]));
    }
  }

  // Score computation: reordered to i→j→p for cache efficiency.
  //   Old p→i→j: Q and K both accessed column-strided (head_q/head_k stride).
  //   New i→j→p: Q row q_row[p] and K row k_row[p] both sequential; output
  //   scores[i][j] accumulated as a scalar — stays in register across full p.
  // Inner p loop vectorized: ternary×ternary = int8×int8 → int32 sum.
  std::vector<float> scores(static_cast<std::size_t>(seq_q) * static_cast<std::size_t>(seq_k), 0.0f);
#if defined(__ARM_NEON) || defined(__AVX2__)
  const int head8s = (head_q / 8) * 8;  // SIMD width for int8 score reduction
#endif
#if defined(__ARM_NEON)
  const int headv_s = (head_v / 4) * 4;  // NEON 4-wide for float V output
#elif defined(__AVX2__)
  const int headv_s = (head_v / 8) * 8;  // AVX2 8-wide for float V output
#endif
  for (int i = 0; i < seq_q; ++i) {
    const std::int8_t* const q_row =
        q_trits.data() + static_cast<std::size_t>(i) * static_cast<std::size_t>(head_q);
    float* const srow =
        scores.data() + static_cast<std::size_t>(i) * static_cast<std::size_t>(seq_k);
    for (int j = 0; j < seq_k; ++j) {
      const std::int8_t* const k_row =
          k_trits.data() + static_cast<std::size_t>(j) * static_cast<std::size_t>(head_k);
      int32_t dot = 0;
      int p = 0;
#ifdef __ARM_NEON
      // 8 int8 trits at once: vmull_s8 → int16x8, vpaddlq_s16 → int32x4 sum.
      int32x4_t acc4 = vdupq_n_s32(0);
      for (; p < head8s; p += 8) {
        const int8x8_t qv = vld1_s8(q_row + p);
        const int8x8_t kv = vld1_s8(k_row + p);
        acc4 = vaddq_s32(acc4, vpaddlq_s16(vmull_s8(qv, kv)));
      }
      dot = vaddvq_s32(acc4);
#elif defined(__AVX2__)
      // 8 int8 trits: sign-extend to int32, mullo, accumulate.
      __m256i acc8 = _mm256_setzero_si256();
      for (; p < head8s; p += 8) {
        const __m128i qv = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(q_row + p));
        const __m128i kv = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(k_row + p));
        acc8 = _mm256_add_epi32(acc8,
            _mm256_mullo_epi32(_mm256_cvtepi8_epi32(qv), _mm256_cvtepi8_epi32(kv)));
      }
      {
        const __m128i lo  = _mm256_extracti128_si256(acc8, 0);
        const __m128i hi  = _mm256_extracti128_si256(acc8, 1);
        const __m128i s   = _mm_add_epi32(lo, hi);
        const __m128i s2  = _mm_hadd_epi32(s, s);
        dot = _mm_extract_epi32(_mm_hadd_epi32(s2, s2), 0);
      }
#endif
      for (; p < head_q; ++p) {
        dot += static_cast<int32_t>(q_row[p]) * static_cast<int32_t>(k_row[p]);
      }
      srow[j] = static_cast<float>(dot);
    }
  }

  // Softmax per query row: scale → stable exp → normalize.
  // exp() stays scalar (correctness); scale and normalize passes are SIMD.
  const float scale = 1.0f / std::sqrt(static_cast<float>(head_q));
  for (int i = 0; i < seq_q; ++i) {
    float* const srow = scores.data() + static_cast<std::size_t>(i) * static_cast<std::size_t>(seq_k);
    float row_max = -1.0e38f;
    for (int j = 0; j < seq_k; ++j) {
      srow[j] *= scale;
      row_max = std::max(row_max, srow[j]);
    }
    float row_sum = 0.0f;
    for (int j = 0; j < seq_k; ++j) {
      srow[j] = std::exp(srow[j] - row_max);
      row_sum += srow[j];
    }
    if (row_sum == 0.0f) {
      return std::nullopt;
    }
    const float inv_sum = 1.0f / row_sum;
    int j = 0;
#ifdef __ARM_NEON
    const float32x4_t inv4 = vdupq_n_f32(inv_sum);
    for (; j < (seq_k / 4) * 4; j += 4) {
      vst1q_f32(srow + j, vmulq_f32(vld1q_f32(srow + j), inv4));
    }
#elif defined(__AVX2__)
    const __m256 inv8 = _mm256_set1_ps(inv_sum);
    for (; j < (seq_k / 8) * 8; j += 8) {
      _mm256_storeu_ps(srow + j, _mm256_mul_ps(_mm256_loadu_ps(srow + j), inv8));
    }
#endif
    for (; j < seq_k; ++j) {
      srow[j] *= inv_sum;
    }
  }

  // Output: O[i][j] = sum_p(softmax[i][p] * V[p][j])
  // SIMD over head_v dimension with FMA (NEON vmlaq_f32, AVX2 _mm256_fmadd_ps).
  std::vector<float> out(static_cast<std::size_t>(seq_q) * static_cast<std::size_t>(head_v), 0.0f);
  for (int i = 0; i < seq_q; ++i) {
    const float* const srow =
        scores.data() + static_cast<std::size_t>(i) * static_cast<std::size_t>(seq_k);
    float* const orow =
        out.data() + static_cast<std::size_t>(i) * static_cast<std::size_t>(head_v);
    for (int p = 0; p < seq_k; ++p) {
      const float alpha = srow[p];
      if (alpha == 0.0f) {
        continue;
      }
      const float* const vrow =
          v_vals.data() + static_cast<std::size_t>(p) * static_cast<std::size_t>(head_v);
      int j = 0;
#ifdef __ARM_NEON
      const float32x4_t alpha4 = vdupq_n_f32(alpha);
      for (; j < headv_s; j += 4) {
        vst1q_f32(orow + j, vmlaq_f32(vld1q_f32(orow + j), alpha4, vld1q_f32(vrow + j)));
      }
#elif defined(__AVX2__)
      const __m256 alpha8 = _mm256_set1_ps(alpha);
      for (; j < headv_s; j += 8) {
        _mm256_storeu_ps(orow + j,
            _mm256_fmadd_ps(alpha8, _mm256_loadu_ps(vrow + j), _mm256_loadu_ps(orow + j)));
      }
#endif
      for (; j < head_v; ++j) {
        orow[j] += alpha * vrow[j];
      }
    }
  }

  return t81::T729DynamicTensor::from_host_float_data({seq_q, head_v}, std::move(out));
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
