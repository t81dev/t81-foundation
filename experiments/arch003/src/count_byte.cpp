#include "arch003/adapters.hpp"
#include "arch003/entrypoints.hpp"
namespace arch003 {
Result count_byte(const Request& r) {
  auto s = admit(r, BYTE);
  if (s != OK) return {s, {}};
  // Clang may vectorize this widening equality reduction at -O3.
  std::uint64_t count = 0;
  for (std::uint64_t i = 0; i < r.a.n; ++i) count += r.a.payload[i] == r.b.payload[i];
  return {OK, count};
}
}  // namespace arch003
