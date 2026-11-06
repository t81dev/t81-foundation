#include "arch003/adapters.hpp"
#include <stdexcept>
namespace arch003 {
std::uint64_t payload_size(std::uint8_t f, std::uint64_t n) {
  if (n > limit || f > PLANES) throw std::invalid_argument("size outside admitted domain");
  switch (f) {
    case BYTE:
      return n;
    case C2:
      return (n + 3) / 4;
    case PT5:
      return (n + 4) / 5;
    case NATIVE:
      return 32 * ((n + 127) / 128);
    default:
      return 2 * ((n + 7) / 8);
  }
}
Bytes encode(std::uint8_t f, std::span<const std::int8_t> s) {
  Bytes b(payload_size(f, s.size()), f == NATIVE ? 0x55 : f == PT5 ? 121 : 0);
  for (std::size_t i = 0; i < s.size(); ++i) {
    const int t = s[i];
    if (t < -1 || t > 1) throw std::invalid_argument("source symbol");
    if (f == BYTE)
      b[i] = static_cast<std::uint8_t>(t);
    else if (f == C2 || f == NATIVE) {
      const unsigned sh = 2 * (i % 4);
      const unsigned c = f == C2 ? (t < 0 ? 3 : t) : t + 1;
      b[i / 4] = static_cast<std::uint8_t>((b[i / 4] & ~(3U << sh)) | (c << sh));
    } else if (f == PT5) {
      unsigned p = 1;
      for (unsigned j = 0; j < i % 5; ++j) p *= 3;
      b[i / 5] = static_cast<std::uint8_t>(b[i / 5] + t * static_cast<int>(p));
    } else {
      const auto z = (s.size() + 7) / 8;
      if (t) b[(t < 0 ? z : 0) + i / 8] |= 1U << (i % 8);
    }
  }
  return b;
}
std::int8_t decode_active(const Operand& o, std::uint64_t i) {
  const auto b = o.payload;
  switch (o.format) {
    case BYTE:
      return b[i] == 255 ? -1 : static_cast<std::int8_t>(b[i]);
    case C2: {
      auto c = (b[i / 4] >> (2 * (i % 4))) & 3;
      return c == 3 ? -1 : static_cast<std::int8_t>(c);
    }
    case NATIVE:
      return static_cast<std::int8_t>(((b[i / 4] >> (2 * (i % 4))) & 3) - 1);
    case PT5: {
      unsigned p = 1;
      for (unsigned j = 0; j < i % 5; ++j) p *= 3;
      return static_cast<std::int8_t>((b[i / 5] / p) % 3 - 1);
    }
    default: {
      auto z = (o.n + 7) / 8;
      return static_cast<std::int8_t>(((b[i / 8] >> (i % 8)) & 1) -
                                      ((b[z + i / 8] >> (i % 8)) & 1));
    }
  }
}
}  // namespace arch003
