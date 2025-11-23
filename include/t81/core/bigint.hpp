#pragma once

#include <cstdint>
#include <string>

namespace t81::core {
// Canonical big integer wrapper. Real implementation should match spec/t81-data-types.md.
class BigInt {
 public:
  BigInt() = default;
  explicit BigInt(std::int64_t v) : value_(v) {}

  [[nodiscard]] std::int64_t value() const noexcept { return value_; }
  [[nodiscard]] std::string to_string() const;

 private:
  std::int64_t value_{0};
};
}  // namespace t81::core

