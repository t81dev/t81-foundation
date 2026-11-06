#include "arch003/adapters.hpp"
namespace arch003 {
static bool active_bad(const Operand& o) {
  auto b = o.payload;
  if (o.format == PT5) {
    for (auto v : b)
      if (v > 242) return true;
    return false;
  }
  for (std::uint64_t i = 0; i < o.n; ++i) {
    if (o.format == BYTE) {
      if (b[i] != 0 && b[i] != 1 && b[i] != 255) return true;
    } else if (o.format == C2 || o.format == NATIVE) {
      auto c = (b[i / 4] >> (2 * (i % 4))) & 3;
      if (c == (o.format == C2 ? 2 : 3)) return true;
    } else {
      auto z = (o.n + 7) / 8;
      if (((b[i / 8] & b[z + i / 8]) >> (i % 8)) & 1) return true;
    }
  }
  return false;
}
static bool padding_bad(const Operand& o) {
  auto b = o.payload;
  if (!o.n || o.format == BYTE) return false;
  if (o.format == PT5) {
    unsigned p = 1;
    for (unsigned j = 0; j < o.n % 5; ++j) p *= 3;
    if (o.n % 5)
      for (unsigned j = static_cast<unsigned>(o.n % 5); j < 5; ++j, p *= 3)
        if ((b.back() / p) % 3 != 1) return true;
  } else if (o.format == C2 || o.format == NATIVE) {
    const auto end = b.size() * 4;
    for (auto i = o.n; i < end; ++i)
      if (((b[i / 4] >> (2 * (i % 4))) & 3) != (o.format == C2 ? 0 : 1)) return true;
  } else {
    const auto z = (o.n + 7) / 8;
    if (o.n % 8) {
      unsigned mask = 0xffU << (o.n % 8);
      if ((b[z - 1] | b[2 * z - 1]) & mask) return true;
    }
  }
  return false;
}
Status admit(const Request& r, std::uint8_t f) {
  if (r.a.format > 4 || r.b.format > 4 || r.a.format != r.b.format || r.a.format != f)
    return ERR_FORMAT;
  if (r.a.n != r.b.n) return ERR_LENGTH;
  if (r.a.n > limit) return ERR_LIMIT;
  if (r.a.payload.size() != payload_size(f, r.a.n) || r.b.payload.size() != payload_size(f, r.b.n))
    return ERR_PAYLOAD_LENGTH;
  if (active_bad(r.a) || active_bad(r.b)) return ERR_SYMBOL;
  if (padding_bad(r.a) || padding_bad(r.b)) return ERR_PADDING;
  return OK;
}
}  // namespace arch003
