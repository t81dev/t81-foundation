#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <numeric>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "t81/types/T81Int.hpp"
#include "t81/types/detail/dmath_types.hpp"

namespace t81 {

enum class TensorNumericClass {
  HostFloat = 0,
  ExactInt,
  ExactTrit,
};

inline constexpr const char* tensor_numeric_class_name(TensorNumericClass numeric_class) noexcept {
  switch (numeric_class) {
    case TensorNumericClass::HostFloat:
      return "host_float";
    case TensorNumericClass::ExactInt:
      return "exact_int";
    case TensorNumericClass::ExactTrit:
      return "exact_trit";
  }
  return "unknown";
}

// Lightweight row-major tensor template.
template <typename T>
class T729TensorBase {
public:
  // --- ctors ---
  T729TensorBase() = default;

  explicit T729TensorBase(std::vector<int> shape)
      : shape_(std::move(shape)), data_(size_from_shape_(shape_)) {
    if (!valid_shape_(shape_)) throw std::invalid_argument("T729Tensor: invalid shape");
    numeric_class_ = infer_numeric_class_(data_);
    initialize_canonical_storage_mode_();
  }

  T729TensorBase(std::initializer_list<int> shape) : T729TensorBase(std::vector<int>(shape)) {}

  T729TensorBase(std::vector<int> shape, std::vector<T> data)
      : shape_(std::move(shape)), data_(std::move(data)) {
    if (!valid_shape_(shape_)) throw std::invalid_argument("T729Tensor: invalid shape");
    if (data_.size() != size_from_shape_(shape_))
      throw std::invalid_argument("T729Tensor: data size mismatch");
    numeric_class_ = infer_numeric_class_(data_);
    initialize_canonical_storage_mode_();
  }

  // --- basics ---
  int rank() const { return static_cast<int>(shape_.size()); }
  const std::vector<int>& shape() const { return shape_; }
  std::vector<T>& data() {
    abandon_canonical_storage_authority_();
    return data_;
  }
  const std::vector<T>& data() const {
    materialize_host_cache_from_canonical_();
    return data_;
  }
  TensorNumericClass numeric_class() const { return numeric_class_; }
  void set_numeric_class(TensorNumericClass numeric_class) { numeric_class_ = numeric_class; }
  void reclassify_numeric_class() {
    materialize_host_cache_from_canonical_();
    numeric_class_ = infer_numeric_class_(data_);
  }
  bool strict_core_eligible() const { return numeric_class_ != TensorNumericClass::HostFloat; }
  bool canonical_fixed_authoritative() const {
    if constexpr (!std::is_same_v<T, float>) {
      return false;
    } else {
      return canonical_fixed_authoritative_ && canonical_fixed_cache_.has_value();
    }
  }
  bool has_canonical_fixed_data() const {
    if constexpr (!std::is_same_v<T, float>) {
      return false;
    } else {
      return ensure_canonical_fixed_cache_();
    }
  }
  const std::vector<t81::core::detail::DFixed>& canonical_fixed_data() const {
    if (!ensure_canonical_fixed_cache_()) {
      throw std::runtime_error("T729Tensor: canonical fixed cache unavailable");
    }
    return *canonical_fixed_cache_;
  }
  bool rebuild_canonical_fixed_cache() const {
    materialize_host_cache_from_canonical_();
    invalidate_canonical_fixed_cache_();
    return ensure_canonical_fixed_cache_();
  }
  void clear_canonical_fixed_cache() const {
    materialize_host_cache_from_canonical_();
    invalidate_canonical_fixed_cache_();
  }

  std::optional<T> value_at(std::size_t index) const {
    if (index >= size()) {
      return std::nullopt;
    }
    if constexpr (!std::is_same_v<T, float>) {
      return data_[index];
    } else {
      if (canonical_fixed_authoritative()) {
        return host_float_from_fixed_(canonical_fixed_data()[index]);
      }
      materialize_host_cache_from_canonical_();
      return data_[index];
    }
  }

  std::vector<T> snapshot_values() const {
    if constexpr (!std::is_same_v<T, float>) {
      return data_;
    } else {
      if (canonical_fixed_authoritative()) {
        std::vector<T> out;
        out.reserve(canonical_fixed_cache_->size());
        for (const auto& value : *canonical_fixed_cache_) {
          out.push_back(host_float_from_fixed_(value));
        }
        return out;
      }
      materialize_host_cache_from_canonical_();
      return data_;
    }
  }

  bool set_value_at(std::size_t index, T value) {
    if (index >= size()) {
      return false;
    }
    if constexpr (!std::is_same_v<T, float>) {
      data_[index] = value;
      return true;
    } else {
      if (canonical_fixed_authoritative()) {
        auto fixed = canonical_fixed_from_float_(value);
        if (fixed.has_value()) {
          (*canonical_fixed_cache_)[index] = *fixed;
          if (host_cache_valid_) {
            data_[index] = host_float_from_fixed_(*fixed);
          }
          return true;
        }
      }
      materialize_host_cache_from_canonical_();
      data_[index] = value;
      host_cache_valid_ = true;
      invalidate_canonical_fixed_cache_();
      return true;
    }
  }

  std::size_t size() const {
    if constexpr (std::is_same_v<T, float>) {
      if (canonical_fixed_authoritative()) {
        return canonical_fixed_cache_->size();
      }
    }
    return data_.size();
  }

  // --- utilities ---
  // Dot-product of two rank-1 tensors → rank-1 {1} tensor.
  static T729TensorBase contract_dot(const T729TensorBase& a, const T729TensorBase& b) {
    if (a.rank() != 1 || b.rank() != 1)
      throw std::invalid_argument("contract_dot: both inputs must be vectors");
    if (a.shape_[0] != b.shape_[0]) throw std::invalid_argument("contract_dot: size mismatch");
    const auto& a_data = a.data();
    const auto& b_data = b.data();
    T s{};
    for (std::size_t i = 0; i < a_data.size(); ++i) s += a_data[i] * b_data[i];
    return T729TensorBase({1}, std::vector<T>{s});
  }

  // 2D transpose → swaps {rows, cols}.
  T729TensorBase transpose2d() const {
    if (rank() != 2) throw std::invalid_argument("transpose2d: rank must be 2");
    const int R = shape_[0], C = shape_[1];
    const auto& values = data();
    std::vector<T> out(static_cast<std::size_t>(R) * C);
    for (int r = 0; r < R; ++r) {
      for (int c = 0; c < C; ++c) {
        out[static_cast<std::size_t>(c) * R + r] =
            values[static_cast<std::size_t>(r) * C + c];
      }
    }
    return T729TensorBase({C, R}, std::move(out));
  }

  // Broadcast (naive repeat) to new shape if compatible (right-aligned).
  T729TensorBase broadcast(std::vector<int> new_shape) const {
    if (new_shape.empty()) throw std::invalid_argument("broadcast: empty new_shape");
    // Align shapes from the right
    int nr = static_cast<int>(new_shape.size());
    std::vector<int> cur(nr, 1);
    for (int i = 0; i < rank(); ++i) cur[nr - 1 - i] = shape_[rank() - 1 - i];

    // Check compatibility
    for (int i = 0; i < nr; ++i) {
      if (!(cur[i] == new_shape[i] || cur[i] == 1)) {
        throw std::invalid_argument("broadcast: incompatible shapes");
      }
    }

    // Compute strides
    auto strides = [](const std::vector<int>& s) {
      std::vector<std::size_t> st(s.size(), 1);
      for (int i = (int)s.size() - 2; i >= 0; --i) {
        std::size_t next = st[(std::size_t)(i + 1)];
        std::size_t dim = static_cast<std::size_t>(s[(std::size_t)(i + 1)]);
        if (next != 0 && dim > std::numeric_limits<std::size_t>::max() / next) {
          throw std::overflow_error("broadcast: stride overflow");
        }
        st[(std::size_t)i] = next * dim;
      }
      return st;
    };
    auto in_strides = strides(cur);
    auto out_strides = strides(new_shape);
    const auto& values = data();

    const std::size_t out_sz = std::accumulate(
        new_shape.begin(), new_shape.end(), std::size_t{1}, [](std::size_t a, int b) {
          if (b <= 0) throw std::invalid_argument("broadcast: non-positive dim");
          if (a != 0 && static_cast<std::size_t>(b) > std::numeric_limits<std::size_t>::max() / a) {
            throw std::overflow_error("broadcast: size overflow");
          }
          return a * static_cast<std::size_t>(b);
        });

    std::vector<T> out(out_sz);
    std::vector<int> idx(nr, 0);

    for (std::size_t flat = 0; flat < out_sz; ++flat) {
      // decode flat -> idx
      std::size_t rem = flat;
      for (int d = 0; d < nr; ++d) {
        idx[(std::size_t)d] = static_cast<int>(rem / out_strides[(std::size_t)d]);
        rem %= out_strides[(std::size_t)d];
      }
      // map to input flat (clamp broadcasted dims to 0)
      std::size_t in_flat = 0;
      for (int d = 0; d < nr; ++d) {
        int dim = cur[(std::size_t)d];
        int ii = (dim == 1) ? 0 : idx[(std::size_t)d];
        in_flat += (std::size_t)ii * in_strides[(std::size_t)d];
      }
      out[flat] = values[in_flat];
    }

    return T729TensorBase(std::move(new_shape), std::move(out));
  }

  static T729TensorBase from_canonical_fixed(std::vector<int> shape,
                                             std::vector<t81::core::detail::DFixed> fixed,
                                             TensorNumericClass numeric_class =
                                                 TensorNumericClass::HostFloat) {
    if constexpr (!std::is_same_v<T, float>) {
      throw std::logic_error("from_canonical_fixed requires float tensor storage");
    } else {
      T729TensorBase tensor(std::move(shape));
      tensor.numeric_class_ = numeric_class;
      tensor.data_.clear();
      tensor.data_.shrink_to_fit();
      tensor.canonical_fixed_cache_ = std::move(fixed);
      tensor.canonical_fixed_authoritative_ = true;
      tensor.host_cache_valid_ = false;
      return tensor;
    }
  }

  static T729TensorBase from_host_float_data(std::vector<int> shape, std::vector<T> data,
                                             TensorNumericClass numeric_class =
                                                 TensorNumericClass::HostFloat) {
    if constexpr (!std::is_same_v<T, float>) {
      throw std::logic_error("from_host_float_data requires float tensor storage");
    } else {
      if (!valid_shape_(shape)) throw std::invalid_argument("T729Tensor: invalid shape");
      if (data.size() != size_from_shape_(shape))
        throw std::invalid_argument("T729Tensor: data size mismatch");
      T729TensorBase tensor;
      tensor.shape_ = std::move(shape);
      tensor.data_ = std::move(data);
      tensor.numeric_class_ = numeric_class;
      tensor.canonical_fixed_cache_.reset();
      tensor.canonical_fixed_authoritative_ = false;
      tensor.host_cache_valid_ = true;
      return tensor;
    }
  }

private:
  std::vector<int> shape_;
  mutable std::vector<T> data_;
  mutable std::optional<std::vector<t81::core::detail::DFixed>> canonical_fixed_cache_;
  mutable bool canonical_fixed_authoritative_{false};
  mutable bool host_cache_valid_{true};

public:
  static constexpr uint64_t kSerializationV2Mask = 1ULL << 63U;
  enum class SerializedPayloadKind : uint8_t {
    RawElements = 0,
    CanonicalFixed = 1,
  };

  // --- Serialization ---
  void serialize(std::ostream& os) const {
    if constexpr (std::is_same_v<T, float>) {
      if (has_canonical_fixed_data()) {
        uint64_t shape_size = static_cast<uint64_t>(shape_.size()) | kSerializationV2Mask;
        os.write(reinterpret_cast<const char*>(&shape_size), sizeof(shape_size));
        const auto payload_kind = static_cast<uint8_t>(SerializedPayloadKind::CanonicalFixed);
        os.write(reinterpret_cast<const char*>(&payload_kind), sizeof(payload_kind));
        const auto numeric_class = static_cast<uint8_t>(numeric_class_);
        os.write(reinterpret_cast<const char*>(&numeric_class), sizeof(numeric_class));
        os.write(reinterpret_cast<const char*>(shape_.data()),
                 static_cast<std::streamsize>(shape_.size() * sizeof(int)));

        uint64_t data_size = size();
        os.write(reinterpret_cast<const char*>(&data_size), sizeof(data_size));
        for (const auto& value : canonical_fixed_data()) {
          value.v.serialize(os);
        }
        return;
      }
    }

    uint64_t shape_size = shape_.size();
    os.write(reinterpret_cast<const char*>(&shape_size), sizeof(shape_size));
    os.write(reinterpret_cast<const char*>(shape_.data()),
             static_cast<std::streamsize>(shape_size * sizeof(int)));

    const auto& values = data();
    uint64_t data_size = values.size();
    os.write(reinterpret_cast<const char*>(&data_size), sizeof(data_size));
    os.write(reinterpret_cast<const char*>(values.data()),
             static_cast<std::streamsize>(data_size * sizeof(T)));
  }

  void deserialize(std::istream& is) {
    uint64_t shape_size;
    is.read(reinterpret_cast<char*>(&shape_size), sizeof(shape_size));
    uint64_t data_size = 0;
    bool data_size_loaded = false;
    if ((shape_size & kSerializationV2Mask) != 0U) {
      const uint64_t decoded_shape_size = shape_size & ~kSerializationV2Mask;
      uint8_t payload_kind = 0;
      uint8_t numeric_class = 0;
      is.read(reinterpret_cast<char*>(&payload_kind), sizeof(payload_kind));
      is.read(reinterpret_cast<char*>(&numeric_class), sizeof(numeric_class));

      shape_.resize(decoded_shape_size);
      is.read(reinterpret_cast<char*>(shape_.data()),
              static_cast<std::streamsize>(decoded_shape_size * sizeof(int)));

      is.read(reinterpret_cast<char*>(&data_size), sizeof(data_size));
      data_size_loaded = true;
      if constexpr (std::is_same_v<T, float>) {
        if (static_cast<SerializedPayloadKind>(payload_kind) ==
            SerializedPayloadKind::CanonicalFixed) {
          std::vector<t81::core::detail::DFixed> fixed;
          fixed.reserve(data_size);
          for (uint64_t i = 0; i < data_size; ++i) {
            typename t81::core::detail::DFixed::Storage storage;
            storage.deserialize(is);
            fixed.emplace_back(storage);
          }
          *this = from_canonical_fixed(shape_, std::move(fixed),
                                       static_cast<TensorNumericClass>(numeric_class));
          return;
        }
      }
      if (static_cast<SerializedPayloadKind>(payload_kind) != SerializedPayloadKind::RawElements) {
        throw std::runtime_error("T729Tensor deserialize: unsupported payload kind");
      }
    } else {
      shape_.resize(shape_size);
      is.read(reinterpret_cast<char*>(shape_.data()),
              static_cast<std::streamsize>(shape_size * sizeof(int)));
    }

    if (!data_size_loaded) {
      is.read(reinterpret_cast<char*>(&data_size), sizeof(data_size));
    }
    data_.resize(data_size);
    is.read(reinterpret_cast<char*>(data_.data()),
            static_cast<std::streamsize>(data_size * sizeof(T)));
    numeric_class_ = infer_numeric_class_(data_);
    host_cache_valid_ = true;
    invalidate_canonical_fixed_cache_();
    initialize_canonical_storage_mode_();
  }

private:
  static bool valid_shape_(const std::vector<int>& s) {
    return std::all_of(s.begin(), s.end(), [](int d) { return d >= 0; });
  }

  static std::size_t size_from_shape_(const std::vector<int>& s) {
    if (s.empty()) return 0;
    std::size_t n = 1;
    for (int d : s) {
      if (d < 0) throw std::invalid_argument("size_from_shape_: negative dim");
      if (d == 0) return 0;  // Any zero dimension → empty tensor
      if (n != 0 && static_cast<std::size_t>(d) > std::numeric_limits<std::size_t>::max() / n) {
        throw std::overflow_error("size_from_shape_: overflow");
      }
      n *= (std::size_t)d;
    }
    return n;
  }

  static TensorNumericClass infer_numeric_class_(const std::vector<T>& data) {
    if constexpr (!std::is_same_v<T, float>) {
      return TensorNumericClass::HostFloat;
    } else {
      bool all_integral = true;
      bool all_trit = true;
      for (float value : data) {
        if (!std::isfinite(value)) {
          return TensorNumericClass::HostFloat;
        }
        if (value > static_cast<float>(std::numeric_limits<long long>::max()) ||
            value < static_cast<float>(std::numeric_limits<long long>::min())) {
          return TensorNumericClass::HostFloat;
        }
        const auto integral = static_cast<long long>(value);
        if (static_cast<float>(integral) != value) {
          all_integral = false;
          all_trit = false;
          break;
        }
        if (integral < -1 || integral > 1) {
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
  }

  static t81::core::detail::DFixed fixed_from_int64_(std::int64_t value) {
    using DFixed = t81::core::detail::DFixed;
    typename DFixed::Storage storage(value);
    storage <<= DFixed::kFractionalTrits;
    return DFixed(storage);
  }

  static float host_float_from_fixed_(const t81::core::detail::DFixed& value) {
    if (value.is_zero()) {
      return 0.0F;
    }
    const auto& raw = value.v;
    float result = 0.0F;
    for (std::size_t i = t81::core::detail::DFixed::Storage::kNumTrits;
         i-- > t81::core::detail::DFixed::kFractionalTrits;) {
      result = result * 3.0F + static_cast<float>(trit_to_int(raw[i]));
    }

    float scale = 1.0F / 3.0F;
    for (std::size_t i = t81::core::detail::DFixed::kFractionalTrits; i-- > 0;) {
      result += static_cast<float>(trit_to_int(raw[i])) * scale;
      scale /= 3.0F;
    }
    return result;
  }

  static std::optional<t81::core::detail::DFixed> canonical_fixed_from_float_(float value) {
    using DFixed = t81::core::detail::DFixed;
    if (!std::isfinite(value)) {
      return std::nullopt;
    }

    std::uint32_t bits = 0;
    std::memcpy(&bits, &value, sizeof(bits));
    const bool negative = (bits >> 31U) != 0U;
    const std::uint32_t exponent = (bits >> 23U) & 0xFFU;
    const std::uint32_t fraction = bits & 0x7FFFFFU;

    if (exponent == 0U && fraction == 0U) {
      return DFixed::zero();
    }

    std::uint64_t mantissa = 0;
    int binary_exponent = 0;
    if (exponent == 0U) {
      mantissa = fraction;
      binary_exponent = -149;
    } else {
      mantissa = (1ULL << 23U) | static_cast<std::uint64_t>(fraction);
      binary_exponent = static_cast<int>(exponent) - 150;
    }

    DFixed out = fixed_from_int64_(static_cast<std::int64_t>(mantissa));
    const DFixed two(2);
    if (binary_exponent > 0) {
      for (int i = 0; i < binary_exponent; ++i) {
        out = out * two;
      }
    } else if (binary_exponent < 0) {
      for (int i = 0; i < -binary_exponent; ++i) {
        out = out / two;
      }
    }

    if (negative) {
      out = -out;
    }
    return out;
  }

  static std::optional<std::vector<t81::core::detail::DFixed>> build_canonical_fixed_cache_(
      const std::vector<T>& data) {
    if constexpr (!std::is_same_v<T, float>) {
      return std::nullopt;
    } else {
      std::vector<t81::core::detail::DFixed> out;
      out.reserve(data.size());
      for (float value : data) {
        auto fixed = canonical_fixed_from_float_(value);
        if (!fixed.has_value()) {
          return std::nullopt;
        }
        out.push_back(*fixed);
      }
      return out;
    }
  }

  bool ensure_canonical_fixed_cache_() const {
    if constexpr (!std::is_same_v<T, float>) {
      return false;
    } else {
      if (canonical_fixed_cache_.has_value()) {
        canonical_fixed_authoritative_ = true;
        return true;
      }
      materialize_host_cache_from_canonical_();
      canonical_fixed_cache_ = build_canonical_fixed_cache_(data_);
      canonical_fixed_authoritative_ = canonical_fixed_cache_.has_value();
      return canonical_fixed_cache_.has_value();
    }
  }

  void invalidate_canonical_fixed_cache_() const {
    if constexpr (std::is_same_v<T, float>) {
      canonical_fixed_cache_.reset();
      canonical_fixed_authoritative_ = false;
    }
  }

  void materialize_host_cache_from_canonical_() const {
    if constexpr (std::is_same_v<T, float>) {
      if (host_cache_valid_ || !canonical_fixed_authoritative_ || !canonical_fixed_cache_.has_value()) {
        return;
      }
      data_.clear();
      data_.reserve(canonical_fixed_cache_->size());
      for (const auto& value : *canonical_fixed_cache_) {
        data_.push_back(host_float_from_fixed_(value));
      }
      host_cache_valid_ = true;
    }
  }

  void initialize_canonical_storage_mode_() {
    if constexpr (std::is_same_v<T, float>) {
      host_cache_valid_ = true;
      canonical_fixed_cache_ = build_canonical_fixed_cache_(data_);
      canonical_fixed_authoritative_ = canonical_fixed_cache_.has_value();
    }
  }

  void abandon_canonical_storage_authority_() const {
    if constexpr (std::is_same_v<T, float>) {
      materialize_host_cache_from_canonical_();
      invalidate_canonical_fixed_cache_();
      host_cache_valid_ = true;
    }
  }

  TensorNumericClass numeric_class_{TensorNumericClass::HostFloat};
};

using T729DynamicTensor = T729TensorBase<float>;
using T729IntTensor = T729TensorBase<T81Int<81>>;

}  // namespace t81
