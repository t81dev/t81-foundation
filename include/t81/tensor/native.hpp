#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <sstream>
#include <vector>

#include "t81/tensor.hpp"
#include "t81/types/detail/dmath.hpp"
#include "t81/weights.hpp"

namespace t81::tensor_native {

enum class DecodeMode {
  StrictCanonical = 0,
  Lenient,
};

inline std::size_t element_count(const std::vector<uint64_t>& shape) {
  std::size_t count = 1;
  for (uint64_t dim : shape) {
    count *= static_cast<std::size_t>(dim);
  }
  return count;
}

inline TensorNumericClass infer_fixed_numeric_class(const std::vector<core::detail::DFixed>& data) {
  using t81::Trit;
  using t81::core::detail::DFixed;

  bool all_integral = true;
  bool all_trit = true;
  const DFixed zero = DFixed::zero();
  const DFixed one = DFixed::one();
  const DFixed neg_one = -DFixed::one();

  for (const auto& value : data) {
    for (std::size_t i = 0; i < DFixed::kFractionalTrits; ++i) {
      if (value.v[i] != Trit::Z) {
        all_integral = false;
        all_trit = false;
        break;
      }
    }
    if (!all_integral) {
      break;
    }
    if ((value <=> zero) != std::strong_ordering::equal &&
        (value <=> one) != std::strong_ordering::equal &&
        (value <=> neg_one) != std::strong_ordering::equal) {
      all_trit = false;
    }
  }

  if (all_trit) {
    return TensorNumericClass::ExactTrit;
  }
  if (all_integral) {
    return TensorNumericClass::ExactInt;
  }
  return TensorNumericClass::HostFloat;
}

inline std::optional<T729DynamicTensor> decode(const weights::NativeTensor& native, DecodeMode mode) {
  using t81::core::detail::DFixed;

  if (native.format == weights::NativeFormat::CanonicalFixed) {
    const std::size_t expected_elements = element_count(native.shape);
    constexpr std::size_t kSerializedStorageBytes = sizeof(uint64_t) + DFixed::Storage::kNumBytes;
    constexpr std::size_t kSerializedStorageLimbs = kSerializedStorageBytes / sizeof(uint64_t);
    if (native.data.size() != expected_elements * kSerializedStorageLimbs) {
      return std::nullopt;
    }

    std::vector<std::byte> bytes(native.data.size() * sizeof(uint64_t));
    std::memcpy(bytes.data(), native.data.data(), bytes.size());
    std::istringstream input(
        std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size()), std::ios::binary);

    std::vector<DFixed> fixed;
    fixed.reserve(expected_elements);
    for (std::size_t i = 0; i < expected_elements; ++i) {
      typename DFixed::Storage storage;
      storage.deserialize(input);
      fixed.emplace_back(storage);
    }

    std::vector<int> shape;
    shape.reserve(native.shape.size());
    for (auto dim : native.shape) {
      shape.push_back(static_cast<int>(dim));
    }
    const auto numeric_class = infer_fixed_numeric_class(fixed);
    return T729DynamicTensor::from_canonical_fixed(std::move(shape), std::move(fixed),
                                                   numeric_class);
  }

  if (native.format == weights::NativeFormat::T3_K) {
    std::vector<float> float_data;
    float_data.reserve(native.num_trits());
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
        if (mode == DecodeMode::StrictCanonical && packed > 242) {
          return std::nullopt;
        }
        uint8_t rem = packed;
        for (uint64_t local = 0; local < 5; ++local, ++trit_index) {
          const uint8_t digit = static_cast<uint8_t>(rem % 3);
          rem = static_cast<uint8_t>(rem / 3);
          if (trit_index < count) {
            const float trit = static_cast<float>(static_cast<int>(digit) - 1);
            float_data.push_back(trit * scale);
          } else if (mode == DecodeMode::StrictCanonical && digit != 1) {
            return std::nullopt;
          }
        }
      }
    }
    std::vector<int> shape;
    shape.reserve(native.shape.size());
    for (auto dim : native.shape) {
      shape.push_back(static_cast<int>(dim));
    }
    return T729DynamicTensor(std::move(shape), std::move(float_data));
  } else {
    std::vector<DFixed> fixed_data;
    fixed_data.reserve(native.num_trits());
    const DFixed zero = DFixed::zero();
    const DFixed one = DFixed::one();
    const DFixed neg_one = -DFixed::one();
    uint64_t remaining = native.trits;
    if (remaining == 0 && !native.data.empty()) {
      remaining = native.data.size() * 48;
    }
    for (uint64_t limb : native.data) {
      const uint64_t count = std::min<uint64_t>(48, remaining);
      std::vector<DFixed> block(static_cast<std::size_t>(count), zero);
      uint64_t val = limb;
      for (int i = 47; i >= 0; --i) {
        const uint64_t digit = val % 3;
        val /= 3;
        if (static_cast<uint64_t>(i) < count) {
          switch (digit) {
            case 0:
              block[static_cast<std::size_t>(i)] = neg_one;
              break;
            case 1:
              block[static_cast<std::size_t>(i)] = zero;
              break;
            case 2:
              block[static_cast<std::size_t>(i)] = one;
              break;
            default:
              return std::nullopt;
          }
        }
      }
      fixed_data.insert(fixed_data.end(), block.begin(), block.end());
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
    return T729DynamicTensor::from_canonical_fixed(std::move(shape), std::move(fixed_data),
                                                   TensorNumericClass::ExactTrit);
  }
}

}  // namespace t81::tensor_native
