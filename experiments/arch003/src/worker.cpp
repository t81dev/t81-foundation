#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <new>
#include <stdexcept>
#include <string_view>
#include "arch003/adapters.hpp"
#include "arch003/entrypoints.hpp"
#include "arch003/evidence.hpp"
using namespace arch003;
namespace {
void check(bool b, const char* m) {
  if (!b) throw std::runtime_error(m);
}
std::uint64_t read_le(const std::uint8_t* p, unsigned n) {
  std::uint64_t v = 0;
  for (unsigned i = 0; i < n; ++i) v |= static_cast<std::uint64_t>(p[i]) << (8 * i);
  return v;
}
struct Allocation {
  std::uint8_t* base = nullptr;
  std::size_t total = 0, start = 0, size = 0;
  bool canary = false;
  Allocation(const Bytes& b, unsigned offset, bool guards) : size(b.size()), canary(guards) {
    if (!size) return;
    start = offset + (guards ? 32 : 0);
    total = start + size + (guards ? 32 : 0);
    base = static_cast<std::uint8_t*>(::operator new(total, std::align_val_t(32)));
    std::memset(base, 0xa5, total);
    std::memcpy(base + start, b.data(), size);
    check(reinterpret_cast<std::uintptr_t>(base + start) % 32 == offset,
          "FAIL_EVIDENCE: actual offset");
  }
  Allocation(const Allocation&) = delete;
  Allocation& operator=(const Allocation&) = delete;
  ~Allocation() {
    if (base) ::operator delete(base, std::align_val_t(32));
  }
  std::span<const std::uint8_t> span() const {
    return size ? std::span<const std::uint8_t>(base + start, size)
                : std::span<const std::uint8_t>();
  }
  void verify() const {
    if (!canary || !size) return;
    for (std::size_t i = 0; i < start; ++i)
      check(base[i] == 0xa5, "FAIL_CORRECTNESS: prefix write");
    for (std::size_t i = start + size; i < total; ++i)
      check(base[i] == 0xa5, "FAIL_CORRECTNESS: suffix write");
  }
};
}  // namespace
int main(int argc, char** argv) {
  try {
    check(std::getenv("ARCH003_AUTHORIZED_WORKER") != nullptr,
          "BLOCKED_AUTHORIZATION: campaign owner required");
    check(argc == 3, "usage: worker --campaign expected.bin | --G1/--G2 guard-expected.bin");
    std::string_view mode = argv[1];
    bool guard = mode == "--G1" || mode == "--G2";
    check(guard || mode == "--campaign", "unknown mode");
    std::ifstream expected(argv[2], std::ios::binary);
    std::array<std::uint8_t, 16> header{};
    expected.read(reinterpret_cast<char*>(header.data()), 16);
    check(expected && std::memcmp(header.data(), "A2R1", 4) == 0 && header[4] == 0 &&
              header[5] == 255 && header[6] == (guard ? 255 : 0) && header[7] == 0 &&
              read_le(header.data() + 8, 8) == (guard ? 1 : 408034),
          "FAIL_EVIDENCE: expected header");
    std::array<std::uint64_t, 6> digests;
    digests.fill(digest_initial);
    bool failed = false;
    auto invoke = [&](const Case& c) {
      std::array<std::uint8_t, 88> e{};
      expected.read(reinterpret_cast<char*>(e.data()), 88);
      check(static_cast<bool>(expected), "FAIL_EVIDENCE: expected EOF");
      Result truth{
          static_cast<Status>(e[11]),
          e[11] == 0 ? std::optional<std::uint64_t>(read_le(e.data() + 44, 8)) : std::nullopt};
      check(expected_frame(c, truth) == Bytes(e.begin(), e.end()),
            "FAIL_EVIDENCE: sealed input mismatch");
      Allocation a(c.a, c.oa, c.mode), b(c.b, c.ob, c.mode);
      Request r{{c.fa, c.na, a.span()}, {c.fb, c.nb, b.span()}};
      if (!a.span().empty() && !b.span().empty()) {
        auto ap = reinterpret_cast<std::uintptr_t>(a.span().data()),
             bp = reinterpret_cast<std::uintptr_t>(b.span().data());
        check(ap + a.size <= bp || bp + b.size <= ap, "FAIL_EVIDENCE: overlapping spans");
      }
      auto sealed = pair_hash(r);
      for (unsigned k = 0; k < 6; ++k)
        if (c.mask & (1U << k)) {
          check(pair_hash(r) == sealed, "FAIL_CORRECTNESS: mutation before call");
          auto result = entries[k](r);
          if (guard) result = {OK, 2};
          bool match = result == truth;
          if (mode != "--G2" && !match) failed = true;
          digests[k] = consume(digests[k], result);
          write_bytes(std::cout, observed_frame(c.seq, k, result, digests[k]));
          a.verify();
          b.verify();
          check(pair_hash(r) == sealed, "FAIL_CORRECTNESS: input mutation");
          if (failed && !guard) {
            std::cerr << "FAIL_CORRECTNESS: case_seq=" << c.seq << " implementation=" << k
                      << " bounded A/B byte prefixes:";
            for (const auto* bytes : {&c.a, &c.b}) {
              std::cerr << " [";
              for (std::size_t j = 0; j < std::min<std::size_t>(16, bytes->size()); ++j)
                std::cerr << static_cast<unsigned>((*bytes)[j]) << ',';
              std::cerr << ']';
            }
            std::cerr << '\n';
            throw std::runtime_error(
                "first mismatch; full observation and expected record retained");
          }
        }
    };
    if (guard) {
      Case c;
      c.seq = UINT64_MAX;
      c.family = 255;
      c.fa = c.fb = C2;
      c.na = c.nb = 1;
      c.a = {0};
      c.b = {0};
      c.mask = 1;
      invoke(c);
    } else
      enumerate([&](const Descriptor& d) { invoke(materialize(d, encode)); });
    check(expected.peek() == std::char_traits<char>::eof(),
          "FAIL_EVIDENCE: expected trailing bytes");
    return failed ? 1 : 0;
  } catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
