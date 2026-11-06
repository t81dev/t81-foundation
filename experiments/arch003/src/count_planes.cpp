#include <bit>
#include "arch003/adapters.hpp"
#include "arch003/entrypoints.hpp"
namespace arch003 {
Result count_planes(const Request& r) {
  auto s = admit(r, PLANES);
  if (s != OK) return {s, {}};
  std::uint64_t count = 0, z = (r.a.n + 7) / 8;
  for (std::uint64_t i = 0; i < z; ++i) {
    unsigned mask = (i + 1 == z && r.a.n % 8) ? (1U << (r.a.n % 8)) - 1 : 255;
    unsigned x = (r.a.payload[i] ^ r.b.payload[i]) | (r.a.payload[z + i] ^ r.b.payload[z + i]);
    count += std::popcount((~x) & mask);
  }
  return {OK, count};
}
}  // namespace arch003
