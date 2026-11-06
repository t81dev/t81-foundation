#include <array>
#include "arch003/adapters.hpp"
#include "arch003/entrypoints.hpp"
namespace arch003 {
Result count_pt5(const Request& r) {
  auto s = admit(r, PT5);
  if (s != OK) return {s, {}};
  // Study-owned full-group table, 59,049 bytes; initialization is charged.
  static const auto table = [] {
    std::array<std::uint8_t, 243 * 243> t{};
    for (unsigned a = 0; a < 243; ++a)
      for (unsigned b = 0; b < 243; ++b) {
        auto x = a, y = b;
        for (unsigned j = 0; j < 5; ++j) {
          t[a * 243 + b] += x % 3 == y % 3;
          x /= 3;
          y /= 3;
        }
      }
    return t;
  }();
  std::uint64_t count = 0, i = 0;
  for (; i + 5 <= r.a.n; i += 5) count += table[r.a.payload[i / 5] * 243 + r.b.payload[i / 5]];
  if (i < r.a.n) {
    unsigned a = r.a.payload[i / 5], b = r.b.payload[i / 5];
    for (; i < r.a.n; ++i, a /= 3, b /= 3) count += a % 3 == b % 3;
  }
  return {OK, count};
}
}  // namespace arch003
