#include <bit>
#include <cstring>
#include "arch003/adapters.hpp"
#include "arch003/entrypoints.hpp"
namespace arch003 {
Result count_tc2(const Request& r) {
  auto s = admit(r, C2);
  if (s != OK) return {s, {}};
  std::uint64_t c = 0, i = 0;
  for (; i + 32 <= r.a.n; i += 32) {
    std::uint64_t a, b;
    std::memcpy(&a, r.a.payload.data() + i / 4, 8);
    std::memcpy(&b, r.b.payload.data() + i / 4, 8);
    auto x = a ^ b;
    c += std::popcount(~(x | (x >> 1)) & 0x5555555555555555ULL);
  }
  for (; i < r.a.n; ++i)
    c += ((r.a.payload[i / 4] >> (2 * (i % 4))) & 3) == ((r.b.payload[i / 4] >> (2 * (i % 4))) & 3);
  return {OK, c};
}
}  // namespace arch003
