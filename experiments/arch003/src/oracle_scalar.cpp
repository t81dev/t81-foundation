// Scalar oracle owns this encoder/decoder. No candidate or histogram helpers.
#include <stdexcept>
#include "arch003/contract.hpp"
namespace arch003 {
static std::uint64_t extent(unsigned f, std::uint64_t n) {
  switch (f) {
    case 0:
      return n;
    case 1:
      return (n + 3) / 4;
    case 2:
      return (n + 4) / 5;
    case 3:
      return ((n + 127) / 128) * 32;
    default:
      return ((n + 7) / 8) * 2;
  }
}
Bytes oracle_encode(std::uint8_t f, std::span<const std::int8_t> s) {
  if (f > 4 || s.size() > limit) throw std::invalid_argument("scalar encode domain");
  Bytes out(extent(f, s.size()), 0);
  for (auto t : s)
    if (t < -1 || t > 1) throw std::invalid_argument("scalar source");
  if (f == 0) {
    for (std::size_t i = 0; i < s.size(); ++i) out[i] = s[i] < 0 ? 255 : s[i];
  } else if (f == 1 || f == 3) {
    for (std::size_t i = 0; i < out.size() * 4; ++i) {
      int t = i < s.size() ? s[i] : 0;
      unsigned code = f == 3 ? t + 1 : (t < 0 ? 3 : t);
      out[i / 4] |= code << (2 * (i % 4));
    }
  } else if (f == 2) {
    for (std::size_t k = 0; k < out.size(); ++k) {
      unsigned p = 1, v = 0;
      for (unsigned j = 0; j < 5; ++j) {
        auto i = k * 5 + j;
        v += static_cast<unsigned>((i < s.size() ? s[i] : 0) + 1) * p;
        p *= 3;
      }
      out[k] = v;
    }
  } else {
    auto half = out.size() / 2;
    for (std::size_t i = 0; i < s.size(); ++i)
      if (s[i] != 0) out[(s[i] < 0 ? half : 0) + i / 8] |= 1U << (i % 8);
  }
  return out;
}
static int symbol(const Operand& o, std::uint64_t i) {
  if (o.format == 0) {
    auto x = o.payload[i];
    return x == 255 ? -1 : x;
  }
  if (o.format == 1) {
    auto x = (o.payload[i / 4] >> (2 * (i % 4))) & 3;
    return x == 3 ? -1 : x;
  }
  if (o.format == 3) return static_cast<int>((o.payload[i / 4] >> (2 * (i % 4))) & 3) - 1;
  if (o.format == 2) {
    unsigned v = o.payload[i / 5];
    for (unsigned j = 0; j < i % 5; ++j) v /= 3;
    return static_cast<int>(v % 3) - 1;
  }
  auto h = (o.n + 7) / 8;
  return ((o.payload[i / 8] >> (i % 8)) & 1) - ((o.payload[h + i / 8] >> (i % 8)) & 1);
}
static bool invalid(const Operand& o) {
  if (o.format == 2) {
    for (auto v : o.payload)
      if (v >= 243) return true;
    return false;
  }
  for (std::uint64_t i = 0; i < o.n; ++i) {
    if (o.format == 0) {
      auto v = o.payload[i];
      if (v != 255 && v != 0 && v != 1) return true;
    } else if (o.format == 1 || o.format == 3) {
      auto v = (o.payload[i / 4] >> (2 * (i % 4))) & 3;
      if (v == (o.format == 1 ? 2 : 3)) return true;
    } else {
      auto h = (o.n + 7) / 8;
      if ((o.payload[i / 8] & o.payload[h + i / 8]) & (1U << (i % 8))) return true;
    }
  }
  return false;
}
static bool pad(const Operand& o) {
  if (o.format == 0 || o.n == 0) return false;
  if (o.format == 1 || o.format == 3) {
    for (auto i = o.n; i < o.payload.size() * 4; ++i)
      if (((o.payload[i / 4] >> (2 * (i % 4))) & 3) != (o.format == 1 ? 0 : 1)) return true;
  }
  if (o.format == 2 && o.n % 5) {
    unsigned v = o.payload.back();
    for (unsigned j = 0; j < o.n % 5; ++j) v /= 3;
    for (unsigned j = static_cast<unsigned>(o.n % 5); j < 5; ++j) {
      if (v % 3 != 1) return true;
      v /= 3;
    }
  }
  if (o.format == 4) {
    auto h = (o.n + 7) / 8;
    for (auto i = o.n; i < h * 8; ++i)
      if ((o.payload[i / 8] | o.payload[h + i / 8]) & (1U << (i % 8))) return true;
  }
  return false;
}
Result oracle_evaluate(const Request& r, std::uint8_t registered_format) {
  if (r.a.format > 4 || r.b.format > 4 || r.a.format != r.b.format ||
      r.a.format != registered_format)
    return {ERR_FORMAT, {}};
  if (r.a.n != r.b.n) return {ERR_LENGTH, {}};
  if (r.a.n > limit) return {ERR_LIMIT, {}};
  if (r.a.payload.size() != extent(r.a.format, r.a.n) ||
      r.b.payload.size() != extent(r.b.format, r.b.n))
    return {ERR_PAYLOAD_LENGTH, {}};
  if (invalid(r.a) || invalid(r.b)) return {ERR_SYMBOL, {}};
  if (pad(r.a) || pad(r.b)) return {ERR_PADDING, {}};
  std::uint64_t c = 0;
  for (std::uint64_t i = 0; i < r.a.n; ++i) c += symbol(r.a, i) == symbol(r.b, i);
  return {OK, c};
}
Symbols oracle_decode(const Operand& o) {
  Symbols s;
  for (std::uint64_t i = 0; i < o.n; ++i) s.push_back(static_cast<std::int8_t>(symbol(o, i)));
  return s;
}
}  // namespace arch003
