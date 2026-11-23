#include "internal/tensor_helpers.hpp"

#include <algorithm>
#include <atomic>
#include <bit>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <mutex>
#include <sstream>
#include <unordered_map>

#ifdef __ARM_NEON
#include <arm_neon.h>
#elif defined(__AVX2__)
#include <immintrin.h>
#endif

#include "t81/swar/swar.hpp"
#include "t81/tensor/contracts.hpp"
#include "t81/tensor/elementwise.hpp"
#include "t81/tensor/llama.hpp"
#include "t81/tensor/matmul.hpp"
#include "t81/tensor/mutation.hpp"
#include "t81/tensor/native.hpp"
#include "t81/tensor/reduce.hpp"
#include "t81/tensor/ternary_native.hpp"
#include "t81/tensor/transpose.hpp"
#include "t81/tensor/unary.hpp"
#include "t81/tracing/canonhash.hpp"
#include "t81/types/detail/dmath.hpp"

namespace t81::vm::internal {

namespace {

using t81::ComputeTritVector;
using t81::core::detail::DFixed;

inline void prefetch_read_l2(const void* ptr) noexcept {
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
  _mm_prefetch(static_cast<const char*>(ptr), _MM_HINT_T1);
#elif defined(__clang__) || defined(__GNUC__)
  __builtin_prefetch(ptr, 0, 2);
#else
  (void)ptr;
#endif
}

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
    const std::size_t count = static_cast<std::size_t>(
        std::min<uint64_t>(48, static_cast<uint64_t>(total_trits - out_offset)));
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

  static const float kExpNegOne =
      static_cast<float>(t81::core::detail::exp(TensorFloat::from_double(-1.0)).to_double());
  static const float kExpZero = 1.0f;
  static const float kExpOne =
      static_cast<float>(t81::core::detail::exp(TensorFloat::from_double(1.0)).to_double());

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
  tensor.set_numeric_class(t81::TensorNumericClass::HostFloat);
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
      const int8x8_t sv = vld1_s8(trits.data() + i);
      const int16x8_t sv16 = vmovl_s8(sv);
      vst1q_f32(out.data() + i, vcvtq_f32_s32(vmovl_s16(vget_low_s16(sv16))));
      vst1q_f32(out.data() + i + 4, vcvtq_f32_s32(vmovl_s16(vget_high_s16(sv16))));
    }
#elif defined(__AVX2__)
    for (; i < total8; i += 8) {
      const __m128i sv = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(trits.data() + i));
      _mm256_storeu_ps(out.data() + i, _mm256_cvtepi32_ps(_mm256_cvtepi8_epi32(sv)));
    }
#endif
    for (; i < total; ++i) {
      out[i] = static_cast<float>(trits[i]);
    }
  }

  auto tensor = t81::T729DynamicTensor::from_host_float_data(std::move(shape), std::move(out),
                                                             t81::TensorNumericClass::ExactTrit);
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

  // BalancedTernary inputs: trits ∈ {-1, 0, +1}.
  //   Step:        snap_trit(±1) = ±1, snap_trit(0) = 0               → identity
  //   TanhQuantized: snap_trit(tanh(±1)) = snap_trit(±0.762) = ±1     → identity
  // Both modes reduce to int8→float for pre-ternary values. SIMD main path;
  // scalar tail applies the full activation for correctness on any edge values.
  std::vector<float> out(total, 0.0f);
  std::size_t i = 0;
#if defined(__ARM_NEON) || defined(__AVX2__)
  const std::size_t total8 = (total / 8) * 8;
#endif
#ifdef __ARM_NEON
  for (; i < total8; i += 8) {
    const int8x8_t sv = vld1_s8(trits.data() + i);
    const int16x8_t sv16 = vmovl_s8(sv);
    vst1q_f32(out.data() + i, vcvtq_f32_s32(vmovl_s16(vget_low_s16(sv16))));
    vst1q_f32(out.data() + i + 4, vcvtq_f32_s32(vmovl_s16(vget_high_s16(sv16))));
  }
#elif defined(__AVX2__)
  for (; i < total8; i += 8) {
    const __m128i sv = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(trits.data() + i));
    _mm256_storeu_ps(out.data() + i, _mm256_cvtepi32_ps(_mm256_cvtepi8_epi32(sv)));
  }
#endif
  for (; i < total; ++i) {
    float x = static_cast<float>(trits[i]);
    if (mode == t81::ops::kTActModeTanh) {
      x = std::tanh(x);
    }
    out[i] = static_cast<float>(t81::ops::ternary_detail::snap_trit(x));
  }

  auto tensor = t81::T729DynamicTensor::from_host_float_data(std::move(shape), std::move(out),
                                                             t81::TensorNumericClass::ExactTrit);
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
      if (v == 1.0f)
        av_trits[i] = 1;
      else if (v == -1.0f)
        av_trits[i] = -1;
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
      acc8 = _mm256_add_epi32(
          acc8, _mm256_mullo_epi32(_mm256_cvtepi8_epi32(wv), _mm256_cvtepi8_epi32(av)));
    }
    {
      const __m128i lo = _mm256_extracti128_si256(acc8, 0);
      const __m128i hi = _mm256_extracti128_si256(acc8, 1);
      const __m128i s = _mm_add_epi32(lo, hi);
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

  static const float kExpNegOne =
      static_cast<float>(t81::core::detail::exp(TensorFloat::from_double(-1.0)).to_double());
  static const float kExpPosOne =
      static_cast<float>(t81::core::detail::exp(TensorFloat::from_double(1.0)).to_double());
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
  tensor.set_numeric_class(t81::TensorNumericClass::HostFloat);
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

  static const float kExpNegTwo =
      static_cast<float>(t81::core::detail::exp(TensorFloat::from_double(-2.0)).to_double());
  static const float kExpNegOne =
      static_cast<float>(t81::core::detail::exp(TensorFloat::from_double(-1.0)).to_double());
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
  tensor.set_numeric_class(t81::TensorNumericClass::HostFloat);
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
        const float mean_ss = static_cast<float>(nonzero_count) / static_cast<float>(dim) + 1e-6f;
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

  // RoPE rotation: (v0, v1) → (v0·cos − v1·sin,  v0·sin + v1·cos) per pair.
  // NEON: vld2q_f32 deinterleaves 4 pairs into even/odd float32x4_t in one
  //       instruction; vmlsq/vmlaq compute both outputs without a temporary;
  //       vst2q_f32 re-interleaves. Covers head_dim in head_dim/8 iterations.
#ifdef __ARM_NEON
  const int hdim8 = (head_dim / 8) * 8;  // 4 pairs per NEON iteration
#endif
  for (std::size_t base = 0; base < data.size(); base += static_cast<std::size_t>(head_dim)) {
    float* const drow = data.data() + base;
    int j = 0;
#ifdef __ARM_NEON
    for (; j < hdim8; j += 8) {
      float32x4x2_t vc = vld2q_f32(drow + j);  // val[0]=v0s, val[1]=v1s
      const float32x4_t cos4 = vld1q_f32(cos_terms.data() + j / 2);
      const float32x4_t sin4 = vld1q_f32(sin_terms.data() + j / 2);
      float32x4x2_t vout;
      vout.val[0] = vmlsq_f32(vmulq_f32(vc.val[0], cos4), vc.val[1], sin4);  // v0·cos − v1·sin
      vout.val[1] = vmlaq_f32(vmulq_f32(vc.val[0], sin4), vc.val[1], cos4);  // v0·sin + v1·cos
      vst2q_f32(drow + j, vout);
    }
#endif
    for (; j < head_dim; j += 2) {
      const float f_cos = cos_terms[static_cast<std::size_t>(j / 2)];
      const float f_sin = sin_terms[static_cast<std::size_t>(j / 2)];
      const float v0 = drow[j];
      const float v1 = drow[j + 1];
      drow[j] = v0 * f_cos - v1 * f_sin;
      drow[j + 1] = v0 * f_sin + v1 * f_cos;
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
    const std::size_t count = static_cast<std::size_t>(std::min<uint64_t>(48, remaining));
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
  if (weights.format != t81::weights::NativeFormat::BalancedTernary || activations.rank() != 2 ||
      weights.shape.size() != 2) {
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

  // Two-level tiling: L2 P-block → output-row (L1) → j SIMD.
  //
  // Without P-blocking: for large K, each i-row reloads weight rows from L3.
  // With PB=256: the block weight_trits[p0..p0+PB][*] (PB×N bytes) is shared
  // across all M rows within the block, amortising the L3 load cost by M.
  //
  // Inner j loop: 16-wide unroll (NEON: 4 × float32x4_t; AVX2: 2 × __m256).
  // Four independent accumulator chains improve ILP on dual-issue SIMD FUs.
  // Prefetch hint pulls the next weight row into L2 one step ahead.
  std::vector<float> out(static_cast<std::size_t>(m) * static_cast<std::size_t>(n), 0.0f);
  const int PB = std::min(k, 256);  // L2 block size over reduction axis
#if defined(__ARM_NEON)
  const int n16 = (n / 16) * 16;  // 16-wide NEON (4 × float32x4_t)
  const int n4 = (n / 4) * 4;     // 4-wide tail
#elif defined(__AVX2__)
  const int n16 = (n / 16) * 16;  // 16-wide AVX2 (2 × __m256)
  const int n8 = (n / 8) * 8;     // 8-wide tail
#endif
  for (int p0 = 0; p0 < k; p0 += PB) {
    const int pend = std::min(p0 + PB, k);
    for (int i = 0; i < m; ++i) {
      float* const orow = out.data() + static_cast<std::size_t>(i) * static_cast<std::size_t>(n);
      for (int p = p0; p < pend; ++p) {
        const float act_value = act_vals[static_cast<std::size_t>(i) * static_cast<std::size_t>(k) +
                                         static_cast<std::size_t>(p)];
        int av = 0;
        if (exact_trit_activations) {
          if (act_value == -1.0f)
            av = -1;
          else if (act_value == 1.0f)
            av = 1;
        } else {
          av = t81::ops::ternary_detail::snap_trit(act_value);
        }
        if (av == 0) continue;

        const std::int8_t* const wrow =
            weight_trits.data() + static_cast<std::size_t>(p) * static_cast<std::size_t>(n);
        // Prefetch next weight row into L2 (read, L2 locality).
        if (p + 1 < pend) {
          prefetch_read_l2(wrow + n);
        }
        int j = 0;
#ifdef __ARM_NEON
        // No vmulq_f32 — ternary constraint eliminates all multiplies.
        const float32x4_t fav4 = vdupq_n_f32(static_cast<float>(av));
        const float32x4_t fav4n = vnegq_f32(fav4);
        const float32x4_t zero4 = vdupq_n_f32(0.0f);
// Select ±av or 0 from a float32x4_t trit vector.
#define T81_TERN_SEL(wf_) \
  vbslq_f32(vcgtq_f32((wf_), zero4), fav4, vbslq_f32(vcltq_f32((wf_), zero4), fav4n, zero4))
        // 16-wide: 2 int8x8 loads → 4 float32x4 groups, 4 independent chains.
        for (; j < n16; j += 16) {
          const int16x8_t wv16_lo = vmovl_s8(vld1_s8(wrow + j));
          const int16x8_t wv16_hi = vmovl_s8(vld1_s8(wrow + j + 8));
          const float32x4_t wf0 = vcvtq_f32_s32(vmovl_s16(vget_low_s16(wv16_lo)));
          const float32x4_t wf1 = vcvtq_f32_s32(vmovl_s16(vget_high_s16(wv16_lo)));
          const float32x4_t wf2 = vcvtq_f32_s32(vmovl_s16(vget_low_s16(wv16_hi)));
          const float32x4_t wf3 = vcvtq_f32_s32(vmovl_s16(vget_high_s16(wv16_hi)));
          float32x4_t ov0 = vld1q_f32(orow + j);
          float32x4_t ov1 = vld1q_f32(orow + j + 4);
          float32x4_t ov2 = vld1q_f32(orow + j + 8);
          float32x4_t ov3 = vld1q_f32(orow + j + 12);
          ov0 = vaddq_f32(ov0, T81_TERN_SEL(wf0));
          ov1 = vaddq_f32(ov1, T81_TERN_SEL(wf1));
          ov2 = vaddq_f32(ov2, T81_TERN_SEL(wf2));
          ov3 = vaddq_f32(ov3, T81_TERN_SEL(wf3));
          vst1q_f32(orow + j, ov0);
          vst1q_f32(orow + j + 4, ov1);
          vst1q_f32(orow + j + 8, ov2);
          vst1q_f32(orow + j + 12, ov3);
        }
        // 4-wide tail
        for (; j < n4; j += 4) {
          const float32x4_t wf =
              vcvtq_f32_s32(vmovl_s16(vget_low_s16(vmovl_s8(vld1_s8(wrow + j)))));
          float32x4_t ov = vld1q_f32(orow + j);
          vst1q_f32(orow + j, vaddq_f32(ov, T81_TERN_SEL(wf)));
        }
#undef T81_TERN_SEL
#elif defined(__AVX2__)
        // No _mm256_mul_ps — ternary constraint eliminates all multiplies.
        const __m256 fav8 = _mm256_set1_ps(static_cast<float>(av));
        const __m256 fav8n = _mm256_sub_ps(_mm256_setzero_ps(), fav8);
        const __m256 zero8 = _mm256_setzero_ps();
#define T81_AVX_SEL(wf_)                                                                          \
  _mm256_blendv_ps(_mm256_blendv_ps(zero8, fav8n, _mm256_cmp_ps((wf_), zero8, _CMP_LT_OQ)), fav8, \
                   _mm256_cmp_ps((wf_), zero8, _CMP_GT_OQ))
        // 16-wide: 2 × 8-wide AVX2 groups, 2 independent accumulator chains.
        for (; j < n16; j += 16) {
          const __m256 wf0 = _mm256_cvtepi32_ps(
              _mm256_cvtepi8_epi32(_mm_loadl_epi64(reinterpret_cast<const __m128i*>(wrow + j))));
          const __m256 wf1 = _mm256_cvtepi32_ps(_mm256_cvtepi8_epi32(
              _mm_loadl_epi64(reinterpret_cast<const __m128i*>(wrow + j + 8))));
          __m256 ov0 = _mm256_loadu_ps(orow + j);
          __m256 ov1 = _mm256_loadu_ps(orow + j + 8);
          _mm256_storeu_ps(orow + j, _mm256_add_ps(ov0, T81_AVX_SEL(wf0)));
          _mm256_storeu_ps(orow + j + 8, _mm256_add_ps(ov1, T81_AVX_SEL(wf1)));
        }
        // 8-wide tail
        for (; j < n8; j += 8) {
          const __m256 wf = _mm256_cvtepi32_ps(
              _mm256_cvtepi8_epi32(_mm_loadl_epi64(reinterpret_cast<const __m128i*>(wrow + j))));
          __m256 ov = _mm256_loadu_ps(orow + j);
          _mm256_storeu_ps(orow + j, _mm256_add_ps(ov, T81_AVX_SEL(wf)));
        }
#undef T81_AVX_SEL
#endif
        for (; j < n; ++j) {
          const int wt = static_cast<int>(wrow[static_cast<std::size_t>(j)]);
          if (wt != 0) orow[static_cast<std::size_t>(j)] += static_cast<float>(wt * av);
        }
      }
    }
  }

  auto tensor = t81::T729DynamicTensor::from_host_float_data({m, n}, std::move(out),
                                                             t81::TensorNumericClass::ExactInt);
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
      k_trits.size() < static_cast<std::size_t>(seq_k) * static_cast<std::size_t>(head_k)) {
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
  std::vector<float> scores(static_cast<std::size_t>(seq_q) * static_cast<std::size_t>(seq_k),
                            0.0f);
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
        acc8 = _mm256_add_epi32(
            acc8, _mm256_mullo_epi32(_mm256_cvtepi8_epi32(qv), _mm256_cvtepi8_epi32(kv)));
      }
      {
        const __m128i lo = _mm256_extracti128_si256(acc8, 0);
        const __m128i hi = _mm256_extracti128_si256(acc8, 1);
        const __m128i s = _mm_add_epi32(lo, hi);
        const __m128i s2 = _mm_hadd_epi32(s, s);
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
    float* const srow =
        scores.data() + static_cast<std::size_t>(i) * static_cast<std::size_t>(seq_k);
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
    float* const orow = out.data() + static_cast<std::size_t>(i) * static_cast<std::size_t>(head_v);
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
        _mm256_storeu_ps(orow + j, _mm256_fmadd_ps(alpha8, _mm256_loadu_ps(vrow + j),
                                                   _mm256_loadu_ps(orow + j)));
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
  native.trits = native.format == t81::weights::NativeFormat::CanonicalFixed
                     ? trits * DFixed::Storage::kNumTrits
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

namespace {
std::optional<t81::T729DynamicTensor> try_decode_sbdr1t2d_and_cache(
    const std::vector<std::byte>& bytes, std::string_view hash_text);
}  // namespace

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

  // Fast path: canonical SBDR1T2D sparse-block Dual-Rail 2D weight matrix.
  // Pre-populates CanonDualRailRhsCache directly from active blocks with zero transpose.
  static constexpr char kSbdr2dMagic[8] = {'S', 'B', 'D', 'R', '1', 'T', '2', 'D'};
  if (obj_res.value().size() >= 24 &&
      std::memcmp(obj_res.value().data(), kSbdr2dMagic, sizeof(kSbdr2dMagic)) == 0) {
    auto sbdr_tensor = try_decode_sbdr1t2d_and_cache(obj_res.value(), hash_text);
    if (!sbdr_tensor.has_value()) {
      return {TensorLoadHashStatus::DecodeFault, std::nullopt};
    }
    return {TensorLoadHashStatus::Ok, std::move(sbdr_tensor)};
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

namespace {

struct CachedDualRailRhs {
  std::size_t K = 0;
  std::size_t N = 0;
  std::size_t W64 = 0;
  std::vector<std::uint64_t> Bp;
  std::vector<std::uint64_t> Bn;
  // Regime-aware 128-trit SIMD sparse stream (populated when K%128==0 and sparsity >= 50%).
  bool use_sparse128 = false;
  std::vector<std::uint32_t> sp_col_offsets;
  std::vector<std::uint16_t> sp_blk_idx;
  std::vector<std::uint64_t> sp_bp;
  std::vector<std::uint64_t> sp_bn;
};

void build_sparse128_if_beneficial(CachedDualRailRhs& rhs) {
  if (rhs.W64 < 2 || (rhs.W64 % 2) != 0) return;
  const std::size_t B128 = rhs.W64 / 2;
  const std::size_t total_blocks = rhs.N * B128;
  std::size_t active = 0;
  for (std::size_t n = 0; n < rhs.N; ++n) {
    const std::uint64_t* bp = rhs.Bp.data() + n * rhs.W64;
    const std::uint64_t* bn = rhs.Bn.data() + n * rhs.W64;
    for (std::size_t b = 0; b < B128; ++b) {
      if ((bp[2 * b] | bn[2 * b] | bp[2 * b + 1] | bn[2 * b + 1]) != 0ULL) ++active;
    }
  }
  // H06-MEM-02 threshold: route to 128-trit SIMD sparse when block sparsity >= 50%.
  if (active * 2 > total_blocks) return;
  rhs.use_sparse128 = true;
  rhs.sp_col_offsets.resize(rhs.N + 1, 0);
  rhs.sp_blk_idx.reserve(active);
  rhs.sp_bp.reserve(active * 2);
  rhs.sp_bn.reserve(active * 2);
  for (std::size_t n = 0; n < rhs.N; ++n) {
    rhs.sp_col_offsets[n] = static_cast<std::uint32_t>(rhs.sp_blk_idx.size());
    const std::uint64_t* bp = rhs.Bp.data() + n * rhs.W64;
    const std::uint64_t* bn = rhs.Bn.data() + n * rhs.W64;
    for (std::size_t b = 0; b < B128; ++b) {
      const std::uint64_t p0 = bp[2 * b], n0 = bn[2 * b];
      const std::uint64_t p1 = bp[2 * b + 1], n1 = bn[2 * b + 1];
      if ((p0 | n0 | p1 | n1) == 0ULL) continue;
      rhs.sp_blk_idx.push_back(static_cast<std::uint16_t>(b));
      rhs.sp_bp.push_back(p0);
      rhs.sp_bp.push_back(p1);
      rhs.sp_bn.push_back(n0);
      rhs.sp_bn.push_back(n1);
    }
  }
  rhs.sp_col_offsets[rhs.N] = static_cast<std::uint32_t>(rhs.sp_blk_idx.size());
}

class CanonDualRailRhsCache {
public:
  static constexpr std::size_t kCapacity = 64;

  std::shared_ptr<const CachedDualRailRhs> get(std::string_view key, std::size_t K,
                                               std::size_t N) const {
    if (key.empty()) return nullptr;
    std::lock_guard<std::mutex> lock(mu_);
    auto it = map_.find(std::string(key));
    if (it == map_.end()) return nullptr;
    if (it->second->K != K || it->second->N != N) return nullptr;
    return it->second;
  }

  void put(std::string_view key, std::shared_ptr<const CachedDualRailRhs> entry) {
    if (key.empty() || !entry) return;
    std::lock_guard<std::mutex> lock(mu_);
    std::string k(key);
    if (map_.contains(k)) return;
    if (fifo_.size() >= kCapacity) {
      map_.erase(fifo_.front());
      fifo_.pop_front();
    }
    fifo_.push_back(k);
    map_.emplace(std::move(k), std::move(entry));
  }

private:
  mutable std::mutex mu_;
  std::unordered_map<std::string, std::shared_ptr<const CachedDualRailRhs>> map_;
  std::deque<std::string> fifo_;
};

CanonDualRailRhsCache& canon_dualrail_rhs_cache() {
  static CanonDualRailRhsCache cache;
  return cache;
}

std::optional<t81::T729DynamicTensor> try_decode_sbdr1t2d_and_cache(
    const std::vector<std::byte>& bytes, std::string_view hash_text) {
  if (bytes.size() < 24) return std::nullopt;
  std::uint64_t K64 = 0, N64 = 0;
  std::memcpy(&K64, bytes.data() + 8, 8);
  std::memcpy(&N64, bytes.data() + 16, 8);
  if (K64 == 0 || N64 == 0 || K64 > 65536ULL || N64 > 65536ULL) return std::nullopt;

  const std::size_t K = static_cast<std::size_t>(K64);
  const std::size_t N = static_cast<std::size_t>(N64);
  const std::size_t W64 = (K + 63) / 64;
  const std::size_t total_words = N * W64;
  const std::size_t mask_words = (total_words + 63) / 64;
  const std::size_t header_bytes = 24 + mask_words * 8;
  if (bytes.size() < header_bytes) return std::nullopt;

  std::uint64_t active_blocks = 0;
  for (std::size_t m = 0; m < mask_words; ++m) {
    std::uint64_t mw = 0;
    std::memcpy(&mw, bytes.data() + 24 + m * 8, 8);
    if (m + 1 == mask_words && (total_words % 64) != 0) {
      const std::uint64_t valid_mask = (1ULL << (total_words % 64)) - 1ULL;
      if ((mw & ~valid_mask) != 0ULL) return std::nullopt;
    }
    active_blocks += static_cast<std::uint64_t>(std::popcount(mw));
  }
  if (bytes.size() != header_bytes + static_cast<std::size_t>(active_blocks) * 16) {
    return std::nullopt;
  }

  auto built = std::make_shared<CachedDualRailRhs>();
  built->K = K;
  built->N = N;
  built->W64 = W64;
  built->Bp.assign(total_words, 0ULL);
  built->Bn.assign(total_words, 0ULL);

  std::vector<float> float_vals(K * N, 0.0f);

  std::size_t payload_off = header_bytes;
  for (std::size_t m = 0; m < mask_words; ++m) {
    std::uint64_t bits = 0;
    std::memcpy(&bits, bytes.data() + 24 + m * 8, 8);
    while (bits != 0ULL) {
      const unsigned tz = static_cast<unsigned>(std::countr_zero(bits));
      const std::size_t word_idx = (m << 6) + tz;
      std::uint64_t wp = 0, wn = 0;
      std::memcpy(&wp, bytes.data() + payload_off, 8);
      std::memcpy(&wn, bytes.data() + payload_off + 8, 8);
      payload_off += 16;
      if ((wp & wn) != 0ULL || (wp | wn) == 0ULL) return std::nullopt;
      built->Bp[word_idx] = wp;
      built->Bn[word_idx] = wn;

      const std::size_t col = word_idx / W64;
      const std::size_t k_base = (word_idx % W64) * 64;
      const std::size_t k_lim = std::min(K, k_base + 64);
      for (std::size_t k = k_base; k < k_lim; ++k) {
        const std::uint64_t bit = 1ULL << (k - k_base);
        if ((wp & bit) != 0ULL) {
          float_vals[k * N + col] = 1.0f;
        } else if ((wn & bit) != 0ULL) {
          float_vals[k * N + col] = -1.0f;
        }
      }
      bits &= (bits - 1ULL);
    }
  }

  build_sparse128_if_beneficial(*built);
  if (!hash_text.empty()) {
    canon_dualrail_rhs_cache().put(hash_text, built);
  }
  auto tensor = t81::T729DynamicTensor::from_host_float_data(
      {static_cast<int>(K), static_cast<int>(N)}, std::move(float_vals),
      t81::TensorNumericClass::ExactTrit);
  tensor.set_numeric_class(t81::TensorNumericClass::ExactTrit);
  return tensor;
}

// Fast-path Dual-Rail (P, N) ternary 2D matmul for T729DynamicTensor when both
// operands contain exact balanced-ternary values {-1.0f, 0.0f, +1.0f} and dimensions
// exceed the SIMD amortization threshold. Falls back to t81::ops::matmul() on any
// non-ternary float element with zero semantic change.
bool try_dualrail_ternary_matmul_2d(const t81::T729DynamicTensor& lhs,
                                    const t81::T729DynamicTensor& rhs,
                                    std::string_view rhs_canon_hash, t81::T729DynamicTensor& out) {
  if (lhs.shape().size() != 2 || rhs.shape().size() != 2) return false;
  const std::size_t M = static_cast<std::size_t>(lhs.shape()[0]);
  const std::size_t K = static_cast<std::size_t>(lhs.shape()[1]);
  const std::size_t K2 = static_cast<std::size_t>(rhs.shape()[0]);
  const std::size_t N = static_cast<std::size_t>(rhs.shape()[1]);
  if (K != K2 || K < 64 || M * N < 64) return false;

  const auto& a_data = lhs.data();
  const std::size_t W64 = (K + 63) / 64;

  std::vector<std::uint64_t> Ap(M * W64, 0ULL), An(M * W64, 0ULL);
  const std::size_t full_words = K / 64;
  (void)full_words;
  for (std::size_t m = 0; m < M; ++m) {
    const float* row = a_data.data() + m * K;
    std::uint64_t* dp = Ap.data() + m * W64;
    std::uint64_t* dn = An.data() + m * W64;
    std::size_t w = 0;
#if defined(__aarch64__) && defined(__ARM_NEON)
    const float32x4_t vpos = vdupq_n_f32(1.0f);
    const float32x4_t vneg = vdupq_n_f32(-1.0f);
    const float32x4_t vzero_f = vdupq_n_f32(0.0f);
    static constexpr std::uint8_t kBitWeights[16] = {1, 2, 4, 8, 16, 32, 64, 128,
                                                     1, 2, 4, 8, 16, 32, 64, 128};
    const uint8x16_t weights = vld1q_u8(kBitWeights);
    for (; w < full_words; ++w) {
      const float* p = row + w * 64;
      uint8x16_t pm[4], nm[4];
      uint32x4_t valid_all = vdupq_n_u32(0xFFFFFFFFu);
      for (int c = 0; c < 4; ++c) {
        const float* cp = p + c * 16;
        uint32x4_t p0 = vceqq_f32(vld1q_f32(cp + 0), vpos);
        uint32x4_t p1 = vceqq_f32(vld1q_f32(cp + 4), vpos);
        uint32x4_t p2 = vceqq_f32(vld1q_f32(cp + 8), vpos);
        uint32x4_t p3 = vceqq_f32(vld1q_f32(cp + 12), vpos);
        uint32x4_t n0 = vceqq_f32(vld1q_f32(cp + 0), vneg);
        uint32x4_t n1 = vceqq_f32(vld1q_f32(cp + 4), vneg);
        uint32x4_t n2 = vceqq_f32(vld1q_f32(cp + 8), vneg);
        uint32x4_t n3 = vceqq_f32(vld1q_f32(cp + 12), vneg);
        uint32x4_t z0 = vceqq_f32(vld1q_f32(cp + 0), vzero_f);
        uint32x4_t z1 = vceqq_f32(vld1q_f32(cp + 4), vzero_f);
        uint32x4_t z2 = vceqq_f32(vld1q_f32(cp + 8), vzero_f);
        uint32x4_t z3 = vceqq_f32(vld1q_f32(cp + 12), vzero_f);
        valid_all = vandq_u32(valid_all, vorrq_u32(vorrq_u32(p0, n0), z0));
        valid_all = vandq_u32(valid_all, vorrq_u32(vorrq_u32(p1, n1), z1));
        valid_all = vandq_u32(valid_all, vorrq_u32(vorrq_u32(p2, n2), z2));
        valid_all = vandq_u32(valid_all, vorrq_u32(vorrq_u32(p3, n3), z3));
        const uint8x16_t pb = vcombine_u8(vmovn_u16(vcombine_u16(vmovn_u32(p0), vmovn_u32(p1))),
                                          vmovn_u16(vcombine_u16(vmovn_u32(p2), vmovn_u32(p3))));
        const uint8x16_t nb = vcombine_u8(vmovn_u16(vcombine_u16(vmovn_u32(n0), vmovn_u32(n1))),
                                          vmovn_u16(vcombine_u16(vmovn_u32(n2), vmovn_u32(n3))));
        pm[c] = vandq_u8(pb, weights);
        nm[c] = vandq_u8(nb, weights);
      }
      if (vminvq_u32(valid_all) == 0u) return false;
      const uint8x16_t pq = vpaddq_u8(vpaddq_u8(pm[0], pm[1]), vpaddq_u8(pm[2], pm[3]));
      const uint8x16_t nq = vpaddq_u8(vpaddq_u8(nm[0], nm[1]), vpaddq_u8(nm[2], nm[3]));
      dp[w] = vgetq_lane_u64(vreinterpretq_u64_u8(vpaddq_u8(pq, pq)), 0);
      dn[w] = vgetq_lane_u64(vreinterpretq_u64_u8(vpaddq_u8(nq, nq)), 0);
    }
#elif defined(__AVX2__)
    const __m256 vpos = _mm256_set1_ps(1.0f);
    const __m256 vneg = _mm256_set1_ps(-1.0f);
    const __m256 vzero_f = _mm256_setzero_ps();
    for (; w < full_words; ++w) {
      std::uint64_t pm = 0, nm = 0;
      const float* p = row + w * 64;
      for (int c = 0; c < 8; ++c) {
        const __m256 vf = _mm256_loadu_ps(p + c * 8);
        const __m256 eqp = _mm256_cmp_ps(vf, vpos, _CMP_EQ_OQ);
        const __m256 eqn = _mm256_cmp_ps(vf, vneg, _CMP_EQ_OQ);
        const __m256 eqz = _mm256_cmp_ps(vf, vzero_f, _CMP_EQ_OQ);
        const int valid = _mm256_movemask_ps(_mm256_or_ps(_mm256_or_ps(eqp, eqn), eqz));
        if (valid != 0xFF) return false;
        pm |= static_cast<std::uint64_t>(_mm256_movemask_ps(eqp)) << (c * 8);
        nm |= static_cast<std::uint64_t>(_mm256_movemask_ps(eqn)) << (c * 8);
      }
      dp[w] = pm;
      dn[w] = nm;
    }
#endif
    for (std::size_t k = w * 64; k < K; ++k) {
      const float v = row[k];
      if (v == 1.0f) {
        dp[k >> 6] |= (1ULL << (k & 63));
      } else if (v == -1.0f) {
        dn[k >> 6] |= (1ULL << (k & 63));
      } else if (v != 0.0f) {
        return false;
      }
    }
  }

  // Seam 2: Check CanonDualRailRhsCache BEFORE touching rhs.data() so SBDR1T2D / cached
  // CanonFS weights never trigger DFixed -> float materialization on TMatMul.
  auto cached_rhs = canon_dualrail_rhs_cache().get(rhs_canon_hash, K, N);
  if (!cached_rhs) {
    const auto& b_data = rhs.data();
    auto built = std::make_shared<CachedDualRailRhs>();
    built->K = K;
    built->N = N;
    built->W64 = W64;
    built->Bp.assign(N * W64, 0ULL);
    built->Bn.assign(N * W64, 0ULL);
    for (std::size_t k = 0; k < K; ++k) {
      const float* row = b_data.data() + k * N;
      const std::size_t word = k >> 6;
      const std::uint64_t bit = 1ULL << (k & 63);
      for (std::size_t n = 0; n < N; ++n) {
        const float v = row[n];
        if (v == 1.0f) {
          built->Bp[n * W64 + word] |= bit;
        } else if (v == -1.0f) {
          built->Bn[n * W64 + word] |= bit;
        } else if (v != 0.0f) {
          return false;
        }
      }
    }
    build_sparse128_if_beneficial(*built);
    if (!rhs_canon_hash.empty()) {
      canon_dualrail_rhs_cache().put(rhs_canon_hash, built);
    }
    cached_rhs = std::move(built);
  }
  const auto& Bp = cached_rhs->Bp;
  const auto& Bn = cached_rhs->Bn;

  std::vector<float> c_vec(M * N, 0.0f);
  float* c_data = c_vec.data();

  std::atomic<std::uint64_t>& dualrail_dense_counter();
  std::atomic<std::uint64_t>& dualrail_sparse128_counter();

  if (cached_rhs->use_sparse128) {
    for (std::size_t m = 0; m < M; ++m) {
      const std::uint64_t* ap = Ap.data() + m * W64;
      const std::uint64_t* an = An.data() + m * W64;
      for (std::size_t n = 0; n < N; ++n) {
        const std::uint32_t begin = cached_rhs->sp_col_offsets[n];
        const std::uint32_t end = cached_rhs->sp_col_offsets[n + 1];
#if defined(__aarch64__) && defined(__ARM_NEON)
        uint16x8_t p_acc = vdupq_n_u16(0);
        uint16x8_t n_acc = vdupq_n_u16(0);
        for (std::uint32_t idx = begin; idx < end; ++idx) {
          const std::size_t w = static_cast<std::size_t>(cached_rhs->sp_blk_idx[idx]) * 2;
          const uint8x16_t avp = vreinterpretq_u8_u64(vld1q_u64(ap + w));
          const uint8x16_t avn = vreinterpretq_u8_u64(vld1q_u64(an + w));
          const uint8x16_t bvp =
              vreinterpretq_u8_u64(vld1q_u64(cached_rhs->sp_bp.data() + static_cast<std::size_t>(idx) * 2));
          const uint8x16_t bvn =
              vreinterpretq_u8_u64(vld1q_u64(cached_rhs->sp_bn.data() + static_cast<std::size_t>(idx) * 2));
          p_acc = vpadalq_u8(p_acc, vcntq_u8(vorrq_u8(vandq_u8(avp, bvp), vandq_u8(avn, bvn))));
          n_acc = vpadalq_u8(n_acc, vcntq_u8(vorrq_u8(vandq_u8(avp, bvn), vandq_u8(avn, bvp))));
        }
        c_data[m * N + n] = static_cast<float>(
            static_cast<std::int32_t>(vaddlvq_u16(p_acc)) -
            static_cast<std::int32_t>(vaddlvq_u16(n_acc)));
#else
        std::int32_t acc = 0;
        for (std::uint32_t idx = begin; idx < end; ++idx) {
          const std::size_t w = static_cast<std::size_t>(cached_rhs->sp_blk_idx[idx]) * 2;
          const std::uint64_t* bp = cached_rhs->sp_bp.data() + static_cast<std::size_t>(idx) * 2;
          const std::uint64_t* bn = cached_rhs->sp_bn.data() + static_cast<std::size_t>(idx) * 2;
          acc += std::popcount((ap[w] & bp[0]) | (an[w] & bn[0])) -
                 std::popcount((ap[w] & bn[0]) | (an[w] & bp[0])) +
                 std::popcount((ap[w + 1] & bp[1]) | (an[w + 1] & bn[1])) -
                 std::popcount((ap[w + 1] & bn[1]) | (an[w + 1] & bp[1]));
        }
        c_data[m * N + n] = static_cast<float>(acc);
#endif
      }
    }
    dualrail_sparse128_counter().fetch_add(1, std::memory_order_relaxed);
  } else {
    dualrail_dense_counter().fetch_add(1, std::memory_order_relaxed);
    std::vector<std::size_t> active_pairs;
    active_pairs.reserve(W64 / 2);
    std::vector<std::size_t> active_words;
    active_words.reserve(W64);

    for (std::size_t m = 0; m < M; ++m) {
    const std::uint64_t* ap = Ap.data() + m * W64;
    const std::uint64_t* an = An.data() + m * W64;

    // Primitive #3: Pre-collect active non-zero 128-trit blocks once per row (O(W64)).
    // Zero branch mispredictions in the N loop; skips empty MoE/ReLU2 activation blocks.
    active_pairs.clear();
    active_words.clear();
    std::size_t w_scan = 0;
    for (; w_scan + 2 <= W64; w_scan += 2) {
      if ((ap[w_scan] | an[w_scan] | ap[w_scan + 1] | an[w_scan + 1]) != 0ULL) {
        active_pairs.push_back(w_scan);
        if ((ap[w_scan] | an[w_scan]) != 0ULL) active_words.push_back(w_scan);
        if ((ap[w_scan + 1] | an[w_scan + 1]) != 0ULL) active_words.push_back(w_scan + 1);
      }
    }
    const bool has_tail = (w_scan < W64) && ((ap[w_scan] | an[w_scan]) != 0ULL);
    if (has_tail) active_words.push_back(w_scan);

    if (active_words.empty()) {
      std::fill(c_data + m * N, c_data + (m + 1) * N, 0.0f);
      continue;
    }

    std::size_t n = 0;
#if defined(__aarch64__) && defined(__ARM_NEON)
    for (; n + 4 <= N; n += 4) {
      const std::uint64_t* bp0 = Bp.data() + (n + 0) * W64;
      const std::uint64_t* bn0 = Bn.data() + (n + 0) * W64;
      const std::uint64_t* bp1 = Bp.data() + (n + 1) * W64;
      const std::uint64_t* bn1 = Bn.data() + (n + 1) * W64;
      const std::uint64_t* bp2 = Bp.data() + (n + 2) * W64;
      const std::uint64_t* bn2 = Bn.data() + (n + 2) * W64;
      const std::uint64_t* bp3 = Bp.data() + (n + 3) * W64;
      const std::uint64_t* bn3 = Bn.data() + (n + 3) * W64;

      uint16x8_t p0 = vdupq_n_u16(0), n0 = vdupq_n_u16(0);
      uint16x8_t p1 = vdupq_n_u16(0), n1 = vdupq_n_u16(0);
      uint16x8_t p2 = vdupq_n_u16(0), n2 = vdupq_n_u16(0);
      uint16x8_t p3 = vdupq_n_u16(0), n3 = vdupq_n_u16(0);

      for (const std::size_t w : active_pairs) {
        const uint8x16_t avp = vreinterpretq_u8_u64(vld1q_u64(ap + w));
        const uint8x16_t avn = vreinterpretq_u8_u64(vld1q_u64(an + w));
        auto acc = [&](const std::uint64_t* bp, const std::uint64_t* bn, uint16x8_t& pa,
                       uint16x8_t& na) {
          const uint8x16_t bvp = vreinterpretq_u8_u64(vld1q_u64(bp + w));
          const uint8x16_t bvn = vreinterpretq_u8_u64(vld1q_u64(bn + w));
          pa = vpadalq_u8(pa, vcntq_u8(vorrq_u8(vandq_u8(avp, bvp), vandq_u8(avn, bvn))));
          na = vpadalq_u8(na, vcntq_u8(vorrq_u8(vandq_u8(avp, bvn), vandq_u8(avn, bvp))));
        };
        acc(bp0, bn0, p0, n0);
        acc(bp1, bn1, p1, n1);
        acc(bp2, bn2, p2, n2);
        acc(bp3, bn3, p3, n3);
      }

      std::int32_t s0 =
          static_cast<std::int32_t>(vaddlvq_u16(p0)) - static_cast<std::int32_t>(vaddlvq_u16(n0));
      std::int32_t s1 =
          static_cast<std::int32_t>(vaddlvq_u16(p1)) - static_cast<std::int32_t>(vaddlvq_u16(n1));
      std::int32_t s2 =
          static_cast<std::int32_t>(vaddlvq_u16(p2)) - static_cast<std::int32_t>(vaddlvq_u16(n2));
      std::int32_t s3 =
          static_cast<std::int32_t>(vaddlvq_u16(p3)) - static_cast<std::int32_t>(vaddlvq_u16(n3));
      if (has_tail) {
        const std::size_t w = w_scan;
        s0 += std::popcount((ap[w] & bp0[w]) | (an[w] & bn0[w])) -
              std::popcount((ap[w] & bn0[w]) | (an[w] & bp0[w]));
        s1 += std::popcount((ap[w] & bp1[w]) | (an[w] & bn1[w])) -
              std::popcount((ap[w] & bn1[w]) | (an[w] & bp1[w]));
        s2 += std::popcount((ap[w] & bp2[w]) | (an[w] & bn2[w])) -
              std::popcount((ap[w] & bn2[w]) | (an[w] & bp2[w]));
        s3 += std::popcount((ap[w] & bp3[w]) | (an[w] & bn3[w])) -
              std::popcount((ap[w] & bn3[w]) | (an[w] & bp3[w]));
      }
      c_data[m * N + n + 0] = static_cast<float>(s0);
      c_data[m * N + n + 1] = static_cast<float>(s1);
      c_data[m * N + n + 2] = static_cast<float>(s2);
      c_data[m * N + n + 3] = static_cast<float>(s3);
    }
#elif defined(__AVX2__)
    const __m256i low_mask = _mm256_set1_epi8(0x0F);
    const __m256i pop_lut = _mm256_setr_epi8(0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 0, 1,
                                             1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4);
    const __m256i vzero = _mm256_setzero_si256();
    auto avx2_popcnt_u8 = [&](__m256i v) -> __m256i {
      const __m256i lo = _mm256_shuffle_epi8(pop_lut, _mm256_and_si256(v, low_mask));
      const __m256i hi =
          _mm256_shuffle_epi8(pop_lut, _mm256_and_si256(_mm256_srli_epi16(v, 4), low_mask));
      return _mm256_sad_epu8(_mm256_add_epi8(lo, hi), vzero);
    };
    auto hsum_epi64 = [](__m256i v) -> std::int32_t {
      alignas(32) std::uint64_t tmp[4];
      _mm256_store_si256(reinterpret_cast<__m256i*>(tmp), v);
      return static_cast<std::int32_t>(tmp[0] + tmp[1] + tmp[2] + tmp[3]);
    };
    for (; n < N; ++n) {
      const std::uint64_t* bp = Bp.data() + n * W64;
      const std::uint64_t* bn = Bn.data() + n * W64;
      __m256i p_acc = vzero;
      __m256i n_acc = vzero;
      std::size_t w = 0;
      for (; w + 4 <= W64; w += 4) {
        if ((ap[w] | an[w] | ap[w + 1] | an[w + 1] | ap[w + 2] | an[w + 2] | ap[w + 3] |
             an[w + 3]) == 0ULL) {
          continue;
        }
        const __m256i avp = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(ap + w));
        const __m256i avn = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(an + w));
        const __m256i bvp = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(bp + w));
        const __m256i bvn = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(bn + w));
        const __m256i agree =
            _mm256_or_si256(_mm256_and_si256(avp, bvp), _mm256_and_si256(avn, bvn));
        const __m256i dis = _mm256_or_si256(_mm256_and_si256(avp, bvn), _mm256_and_si256(avn, bvp));
        p_acc = _mm256_add_epi64(p_acc, avx2_popcnt_u8(agree));
        n_acc = _mm256_add_epi64(n_acc, avx2_popcnt_u8(dis));
      }
      std::int32_t acc = hsum_epi64(p_acc) - hsum_epi64(n_acc);
      for (; w < W64; ++w) {
        if ((ap[w] | an[w]) == 0ULL) continue;
        acc += std::popcount((ap[w] & bp[w]) | (an[w] & bn[w])) -
               std::popcount((ap[w] & bn[w]) | (an[w] & bp[w]));
      }
      c_data[m * N + n] = static_cast<float>(acc);
    }
#endif
    for (; n < N; ++n) {
      const std::uint64_t* bp = Bp.data() + n * W64;
      const std::uint64_t* bn = Bn.data() + n * W64;
      std::int32_t acc = 0;
      for (const std::size_t w : active_words) {
        acc += std::popcount((ap[w] & bp[w]) | (an[w] & bn[w])) -
               std::popcount((ap[w] & bn[w]) | (an[w] & bp[w]));
      }
      c_data[m * N + n] = static_cast<float>(acc);
    }
  }
  }
  t81::TensorNumericClass out_class = t81::TensorNumericClass::HostFloat;
  if (lhs.numeric_class() != t81::TensorNumericClass::HostFloat &&
      rhs.numeric_class() != t81::TensorNumericClass::HostFloat) {
    bool all_trits = true;
    for (std::size_t i = 0; i < M * N; ++i) {
      const float v = c_data[i];
      if (v != -1.0f && v != 0.0f && v != 1.0f) {
        all_trits = false;
        break;
      }
    }
    out_class = all_trits ? t81::TensorNumericClass::ExactTrit : t81::TensorNumericClass::ExactInt;
  }
  out = t81::T729DynamicTensor::from_host_float_data({static_cast<int>(M), static_cast<int>(N)},
                                                     std::move(c_vec), out_class);
  out.set_numeric_class(out_class);
  return true;
}

std::atomic<std::uint64_t> g_dualrail_fastpath_hits{0};
std::atomic<std::uint64_t> g_dualrail_fallback_count{0};

std::atomic<std::uint64_t>& dualrail_dense_counter() {
  static std::atomic<std::uint64_t> c{0};
  return c;
}

std::atomic<std::uint64_t>& dualrail_sparse128_counter() {
  static std::atomic<std::uint64_t> c{0};
  return c;
}

}  // namespace

DualRailTelemetry dualrail_telemetry() noexcept {
  return {g_dualrail_fastpath_hits.load(std::memory_order_relaxed),
          g_dualrail_fallback_count.load(std::memory_order_relaxed),
          dualrail_dense_counter().load(std::memory_order_relaxed),
          dualrail_sparse128_counter().load(std::memory_order_relaxed)};
}

void reset_dualrail_telemetry() noexcept {
  g_dualrail_fastpath_hits.store(0, std::memory_order_relaxed);
  g_dualrail_fallback_count.store(0, std::memory_order_relaxed);
  dualrail_dense_counter().store(0, std::memory_order_relaxed);
  dualrail_sparse128_counter().store(0, std::memory_order_relaxed);
}

t81::T729DynamicTensor tensor_matmul_2d(const t81::T729DynamicTensor& lhs,
                                        const t81::T729DynamicTensor& rhs) {
  t81::T729DynamicTensor fast_out;
  if (try_dualrail_ternary_matmul_2d(lhs, rhs, {}, fast_out)) {
    g_dualrail_fastpath_hits.fetch_add(1, std::memory_order_relaxed);
    return fast_out;
  }
  g_dualrail_fallback_count.fetch_add(1, std::memory_order_relaxed);
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
  if (!t81::tensor_contracts::linear_index_in_bounds(tensor, index)) {
    return false;
  }
  // T729DynamicTensor::set_value_at updates the canonical fixed element in O(1)
  // when canonical_fixed_authoritative() is true, and updates host float storage
  // in O(1) without triggering an O(N) DFixed rebuild when lazy/host-backed.
  if (!tensor.set_value_at(static_cast<std::size_t>(index), value)) {
    return false;
  }
  if (source_kind == t81::tensor_mutation::ScalarWriteKind::FloatValue ||
      !tensor.strict_core_eligible()) {
    tensor.set_numeric_class(t81::TensorNumericClass::HostFloat);
    return true;
  }
  long long integral = 0;
  if (!t81::tensor_mutation::is_exact_integral_scalar(value, &integral)) {
    tensor.set_numeric_class(t81::TensorNumericClass::HostFloat);
    return true;
  }
  if (tensor.numeric_class() == t81::TensorNumericClass::ExactTrit &&
      (integral < -1 || integral > 1)) {
    tensor.set_numeric_class(t81::TensorNumericClass::ExactInt);
  }
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
  return tensor_matmul_checked_cached(lhs, rhs, {});
}

std::expected<t81::T729DynamicTensor, t81::vm::Trap> tensor_matmul_checked_cached(
    const t81::T729DynamicTensor& lhs, const t81::T729DynamicTensor& rhs,
    std::string_view rhs_canon_hash) {
  if (!tensor_matmul_compatible(lhs, rhs)) {
    return std::expected<t81::T729DynamicTensor, t81::vm::Trap>(t81::unexpect,
                                                                t81::vm::Trap::ShapeFault);
  }
  t81::T729DynamicTensor fast_out;
  if (try_dualrail_ternary_matmul_2d(lhs, rhs, rhs_canon_hash, fast_out)) {
    g_dualrail_fastpath_hits.fetch_add(1, std::memory_order_relaxed);
    return fast_out;
  }
  g_dualrail_fallback_count.fetch_add(1, std::memory_order_relaxed);
  return t81::ops::matmul(lhs, rhs);
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
    const t81::T729DynamicTensor& q, const t81::T729DynamicTensor& k,
    const t81::T729DynamicTensor& v) {
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

std::expected<void, t81::vm::Trap> tensor_set_checked(
    t81::T729DynamicTensor& tensor, std::int64_t index, float value,
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
