// Independent group-oriented oracle. Own base expansion, admission and packing.
#include <stdexcept>
#include "arch003/contract.hpp"
namespace arch003 {
static std::uint64_t storage(std::uint8_t tag, std::uint64_t n) {
  constexpr unsigned group[]{1, 4, 5, 128, 8};
  auto units = n / group[tag] + (n % group[tag] != 0);
  return units * (tag == 3 ? 32 : tag == 4 ? 2 : 1);
}
Bytes oracle_encode(std::uint8_t tag, std::span<const std::int8_t> source) {
  if (tag >= 5 || source.size() > limit) throw std::invalid_argument("histogram source domain");
  Bytes data(storage(tag, source.size()));
  for (int v : source)
    if (v + 1 < 0 || v + 1 > 2) throw std::invalid_argument("histogram alphabet");
  if (tag == BYTE) {
    std::size_t k = 0;
    for (int v : source) data[k++] = v == -1 ? 0xff : static_cast<unsigned>(v);
  }
  if (tag == PT5) {
    for (std::size_t b = 0; b < data.size(); ++b) {
      unsigned v = 0;
      for (int lane = 4; lane >= 0; --lane) {
        auto i = b * 5 + static_cast<unsigned>(lane);
        v = 3 * v + static_cast<unsigned>((i < source.size() ? source[i] : 0) + 1);
      }
      data[b] = v;
    }
  }
  if (tag == C2 || tag == NATIVE) {
    for (std::size_t b = 0; b < data.size(); ++b) {
      unsigned v = 0;
      for (int lane = 3; lane >= 0; --lane) {
        auto i = b * 4 + static_cast<unsigned>(lane);
        int t = i < source.size() ? source[i] : 0;
        unsigned digit = tag == NATIVE ? static_cast<unsigned>(t + 1)
                         : t == -1     ? 3
                                       : static_cast<unsigned>(t);
        v = 4 * v + digit;
      }
      data[b] = v;
    }
  }
  if (tag == PLANES) {
    auto width = data.size() / 2;
    for (std::size_t b = 0; b < width; ++b) {
      unsigned p = 0, m = 0;
      for (unsigned lane = 0; lane < 8; ++lane) {
        auto i = b * 8 + lane;
        if (i < source.size()) {
          p |= static_cast<unsigned>(source[i] == 1) << lane;
          m |= static_cast<unsigned>(source[i] == -1) << lane;
        }
      }
      data[b] = p;
      data[b + width] = m;
    }
  }
  return data;
}
// Return alphabet ordinal 0,1,2; -1 marks a forbidden active symbol.
static int ordinal(const Operand& o, std::uint64_t index) {
  auto bytes = o.payload;
  switch (o.format) {
    case BYTE:
      if (bytes[index] == 255) return 0;
      if (bytes[index] < 2) return bytes[index] + 1;
      return -1;
    case PT5: {
      constexpr unsigned divisors[]{1, 3, 9, 27, 81};
      return (bytes[index / 5] / divisors[index % 5]) % 3;
    }
    case C2: {
      unsigned v = (bytes[index / 4] / (1U << (2 * (index % 4)))) % 4;
      switch (v) {
        case 0:
          return 1;
        case 1:
          return 2;
        case 3:
          return 0;
        default:
          return -1;
      }
    }
    case NATIVE: {
      unsigned v = (bytes[index / 4] / (1U << (2 * (index % 4)))) % 4;
      return v == 3 ? -1 : static_cast<int>(v);
    }
    default: {
      unsigned p = (bytes[index / 8] >> (index % 8)) & 1,
               m = (bytes[bytes.size() / 2 + index / 8] >> (index % 8)) & 1;
      return p && m ? -1 : p ? 2 : m ? 0 : 1;
    }
  }
}
static Status envelope(const Request& q, std::uint8_t tag) {
  if (q.a.format != tag || q.b.format != tag || tag >= 5) return ERR_FORMAT;
  if (q.a.n != q.b.n) return ERR_LENGTH;
  if (q.a.n > 16777216) return ERR_LIMIT;
  for (const Operand* o : {&q.a, &q.b})
    if (storage(tag, o->n) != o->payload.size()) return ERR_PAYLOAD_LENGTH;
  for (const Operand* o : {&q.a, &q.b}) {
    if (tag == PT5)
      for (unsigned v : o->payload)
        if (v > 242) return ERR_SYMBOL;
    for (std::uint64_t k = 0; k < o->n; ++k)
      if (ordinal(*o, k) < 0) return ERR_SYMBOL;
  }
  for (const Operand* o : {&q.a, &q.b}) {
    if (!o->n || tag == BYTE) continue;
    if (tag == PT5) {
      auto used = o->n % 5;
      if (used) {
        unsigned divisor = 1;
        for (std::uint64_t k = 0; k < used; ++k) divisor *= 3;
        unsigned high = o->payload.back() / divisor;
        unsigned expected = 0;
        for (std::uint64_t k = used; k < 5; ++k) expected = 3 * expected + 1;
        if (high != expected) return ERR_PADDING;
      }
    } else if (tag == C2 || tag == NATIVE) {
      for (std::uint64_t k = o->n; k < o->payload.size() * 4; ++k) {
        unsigned v = (o->payload[k / 4] >> (2 * (k % 4))) & 3;
        if (v != (tag == NATIVE ? 1 : 0)) return ERR_PADDING;
      }
    } else {
      auto rem = o->n % 8;
      if (rem) {
        auto h = o->payload.size() / 2;
        if ((o->payload[h - 1] >> rem) || (o->payload.back() >> rem)) return ERR_PADDING;
      }
    }
  }
  return OK;
}
Result oracle_evaluate(const Request& q, std::uint8_t tag) {
  auto status = envelope(q, tag);
  if (status != OK) return {status, {}};
  std::uint64_t joint[3][3]{};
  for (std::uint64_t k = 0; k < q.a.n; ++k) ++joint[ordinal(q.a, k)][ordinal(q.b, k)];
  return {OK, joint[0][0] + joint[1][1] + joint[2][2]};
}
Symbols oracle_decode(const Operand& o) {
  Symbols out(o.n);
  for (std::uint64_t k = 0; k < o.n; ++k) out[k] = static_cast<std::int8_t>(ordinal(o, k) - 1);
  return out;
}
}  // namespace arch003
