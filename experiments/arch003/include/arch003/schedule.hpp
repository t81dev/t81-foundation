#pragma once
#include <functional>
#include "contract.hpp"
namespace arch003 {
constexpr std::array<std::uint64_t, 28> lengths{0,   1,   2,   3,   4,    5,    7,    8,   9,   15,
                                                16,  17,  31,  32,  33,   63,   64,   65,  127, 128,
                                                129, 255, 256, 257, 1023, 1024, 1025, 4097};
constexpr std::array<std::uint64_t, 12> family_records{50,   332145, 4630,  8960, 20,  8960,
                                                       2560, 256,    49012, 580,  778, 83};
constexpr std::array<std::uint64_t, 12> family_calls{60,   398574, 5556,  10752, 24,   10752,
                                                     3072, 256,    49048, 696,   4662, 101};
struct Case {
  std::uint64_t seq = 0;
  std::uint8_t family = 0, fa = 0, fb = 0;
  std::uint64_t na = 0, nb = 0;
  Bytes a, b;
  std::uint8_t oa = 0, ob = 0, mode = 0, mask = 0;
  Request request() const {
    return {
        {fa, na, a.empty() ? std::span<const std::uint8_t>{} : std::span<const std::uint8_t>(a)},
        {fb, nb, b.empty() ? std::span<const std::uint8_t>{} : std::span<const std::uint8_t>(b)}};
  }
};
// Metadata enumeration never allocates scientific payloads. Materialization is separate.
struct Descriptor {
  std::uint64_t seq = 0;
  std::uint8_t family = 0, format = 0, mask = 0;
  std::array<std::uint64_t, 6> args{};
};
using Visitor = std::function<void(const Descriptor&)>;
void enumerate(const Visitor&);
Case materialize(const Descriptor&, Encoder);
std::vector<std::uint64_t> positions(std::uint64_t);
class SplitMix64 {
  std::uint64_t state_;

public:
  explicit SplitMix64(std::uint64_t s) : state_(s) {}
  std::uint64_t next();
  std::uint64_t bounded(std::uint64_t);
};
}  // namespace arch003
