#include "arch003/evidence.hpp"
namespace arch003 {
std::uint64_t consume(std::uint64_t d, Result r) {
  d = (d ^ r.status) * 0x100000001b3ULL;
  auto c = r.count.value_or(0);
  for (unsigned i = 0; i < 8; ++i) {
    d = (d ^ (c & 255)) * 0x100000001b3ULL;
    c >>= 8;
  }
  return d;
}
}  // namespace arch003
