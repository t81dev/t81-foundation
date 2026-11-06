#include <stdexcept>
#include "arch003/evidence.hpp"
namespace arch003 {
void little(Bytes& b, std::uint64_t v, unsigned n) {
  for (unsigned i = 0; i < n; ++i) {
    b.push_back(v & 255);
    v >>= 8;
  }
}
void write_bytes(std::ostream& o, const Bytes& b) {
  o.write(reinterpret_cast<const char*>(b.data()), static_cast<std::streamsize>(b.size()));
  if (!o) throw std::runtime_error("evidence write failed");
}
Hash pair_hash(const Request& r) {
  Sha256 h;
  constexpr std::uint8_t prefix[] = "ARCH002-R1-PAIR";
  h.update(prefix);
  for (auto o : {r.a, r.b}) {
    Bytes b{o.format};
    little(b, o.n, 8);
    little(b, o.payload.size(), 8);
    h.update(b);
    h.update(o.payload);
  }
  return h.finish();
}
Bytes expected_frame(const Case& c, Result r) {
  Bytes b;
  little(b, c.seq, 8);
  b.insert(b.end(), {c.family, c.fa, c.fb, r.status});
  for (auto v : {c.na, c.nb, static_cast<std::uint64_t>(c.a.size()),
                 static_cast<std::uint64_t>(c.b.size()), r.count.value_or(0)})
    little(b, v, 8);
  b.insert(b.end(), {c.oa, c.ob, c.mode, c.mask});
  auto hash = pair_hash(c.request());
  b.insert(b.end(), hash.begin(), hash.end());
  return b;
}
Bytes observed_frame(std::uint64_t seq, std::uint8_t id, Result r, std::uint64_t d) {
  Bytes b;
  little(b, seq, 8);
  b.insert(b.end(), {id, r.status, static_cast<std::uint8_t>(r.count.has_value()), 0});
  little(b, r.count.value_or(0), 8);
  little(b, d, 8);
  little(b, 0, 4);
  return b;
}
}  // namespace arch003
