#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace t81::determinism {

/**
 * @brief CanonHash81: A deterministic cross-platform hash function for T81.
 *
 * Uses a variation of SipHash-2-4 (or a deterministic fallback) to ensure
 * stable hash outputs across all compilers and architectures.
 *
 * @param data The input data to hash.
 * @param length The length of the input data in bytes.
 * @return std::uint64_t The deterministic hash value.
 */
[[nodiscard]] std::uint64_t canon_hash81(const void* data, std::size_t length) noexcept;

/**
 * @brief CanonHash81 helper for string_view.
 */
[[nodiscard]] inline std::uint64_t canon_hash81(std::string_view str) noexcept {
  return canon_hash81(str.data(), str.length());
}

/**
 * @brief CanonHash81 helper for std::string.
 */
[[nodiscard]] inline std::uint64_t canon_hash81(const std::string& str) noexcept {
  return canon_hash81(str.data(), str.length());
}

}  // namespace t81::determinism

// Injection into std::hash equivalent for T81 canonical hashing
namespace t81 {
template <typename T>
struct CanonHash {
  std::size_t operator()(const T& value) const {
    if constexpr (requires { value.serialize_canonical(); }) {
      return static_cast<std::size_t>(t81::determinism::canon_hash81(value.serialize_canonical()));
    } else if constexpr (requires { value.to_canonical_string(); }) {
      return static_cast<std::size_t>(t81::determinism::canon_hash81(value.to_canonical_string()));
    } else if constexpr (std::is_integral_v<T>) {
      // Basic deterministic hashing for integers
      std::string s = std::to_string(value);
      return static_cast<std::size_t>(t81::determinism::canon_hash81(s));
    } else if constexpr (std::is_floating_point_v<T>) {
       // Float hashing isn't used much here without T81Float, but just in case
      return static_cast<std::size_t>(t81::determinism::canon_hash81(std::to_string(value)));
    } else {
      // Fallback: This shouldn't be reached for correct T81 types
      return static_cast<std::size_t>(t81::determinism::canon_hash81(reinterpret_cast<const void*>(&value), sizeof(T)));
    }
  }
};

// Specializations for basic types

template <>
struct CanonHash<std::uint8_t> {
  std::size_t operator()(std::uint8_t value) const {
    std::uint64_t v = value;
    return static_cast<std::size_t>(t81::determinism::canon_hash81(&v, sizeof(v)));
  }
};

template <>
struct CanonHash<std::string> {
  std::size_t operator()(const std::string& value) const {
    return static_cast<std::size_t>(t81::determinism::canon_hash81(value));
  }
};

template <>
struct CanonHash<std::int32_t> {
  std::size_t operator()(std::int32_t value) const {
    std::uint64_t v = value;
    return static_cast<std::size_t>(t81::determinism::canon_hash81(&v, sizeof(v)));
  }
};

template <>
struct CanonHash<std::uint32_t> {
  std::size_t operator()(std::uint32_t value) const {
    std::uint64_t v = value;
    return static_cast<std::size_t>(t81::determinism::canon_hash81(&v, sizeof(v)));
  }
};

template <>
struct CanonHash<std::int64_t> {
  std::size_t operator()(std::int64_t value) const {
    std::uint64_t v = value;
    return static_cast<std::size_t>(t81::determinism::canon_hash81(&v, sizeof(v)));
  }
};

template <>
struct CanonHash<std::uint64_t> {
  std::size_t operator()(std::uint64_t value) const {
    std::uint64_t v = value;
    return static_cast<std::size_t>(t81::determinism::canon_hash81(&v, sizeof(v)));
  }
};

template <>
struct CanonHash<double> {
  std::size_t operator()(double value) const {
    return static_cast<std::size_t>(t81::determinism::canon_hash81(&value, sizeof(value)));
  }
};

} // namespace t81