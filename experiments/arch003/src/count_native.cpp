#include "arch003/adapters.hpp"
#include "arch003/entrypoints.hpp"
namespace arch003 {
Result count_native(const Request& r) {
  auto s = admit(r, NATIVE);
  if (s != OK) return {s, {}};
  std::uint64_t count = 0;
  for (std::uint64_t i = 0; i < r.a.n; ++i)
    count +=
        ((r.a.payload[i / 4] >> (2 * (i % 4))) & 3) == ((r.b.payload[i / 4] >> (2 * (i % 4))) & 3);
  return {OK, count};
}
}  // namespace arch003
