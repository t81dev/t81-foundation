#pragma once
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>
namespace arch003 {
using Bytes = std::vector<std::uint8_t>;
using Symbols = std::vector<std::int8_t>;
constexpr std::uint64_t limit = 1ULL << 24;
enum Format : std::uint8_t { BYTE, C2, PT5, NATIVE, PLANES };
enum Status : std::uint8_t {
  OK,
  ERR_FORMAT,
  ERR_LENGTH,
  ERR_LIMIT,
  ERR_PAYLOAD_LENGTH,
  ERR_SYMBOL,
  ERR_PADDING
};
struct Operand {
  std::uint8_t format;
  std::uint64_t n;
  std::span<const std::uint8_t> payload;
};
struct Request {
  Operand a, b;
};
struct Result {
  Status status;
  std::optional<std::uint64_t> count;
  bool operator==(const Result&) const = default;
};
constexpr std::array<std::uint8_t, 6> registered{C2, BYTE, PLANES, C2, PT5, NATIVE};
constexpr std::uint8_t mask_for(std::uint8_t f) {
  std::uint8_t m = 0;
  for (unsigned i = 0; i < 6; ++i)
    if (registered[i] == f) m |= 1U << i;
  return m;
}
using Encoder = Bytes (*)(std::uint8_t, std::span<const std::int8_t>);
}  // namespace arch003
