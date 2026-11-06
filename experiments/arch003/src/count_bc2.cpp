#include "arch003/adapters.hpp"
#include "arch003/entrypoints.hpp"
namespace arch003 {
Result count_bc2(const Request& r) {
  auto s = admit(r, C2);
  if (s != OK) return {s, {}};
  // Independent conventional nibble equality-count table.
  constexpr unsigned eq[16] = {2, 1, 1, 1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0};
  std::uint64_t count = 0, i = 0;
  for (; i + 4 <= r.a.n; i += 4) {
    unsigned x = r.a.payload[i / 4] ^ r.b.payload[i / 4];
    count += eq[x & 15] + eq[x >> 4];
  }
  for (; i < r.a.n; ++i) {
    unsigned x = (r.a.payload[i / 4] ^ r.b.payload[i / 4]) >> (2 * (i % 4));
    count += (x & 3) == 0;
  }
  return {OK, count};
}
}  // namespace arch003
