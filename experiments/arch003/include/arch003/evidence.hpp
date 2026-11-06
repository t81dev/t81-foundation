#pragma once
#include <ostream>
#include "schedule.hpp"
namespace arch003 {
using Hash = std::array<std::uint8_t, 32>;
class Sha256 {
  std::array<std::uint32_t, 8> h_;
  std::array<std::uint8_t, 64> block_{};
  std::uint64_t size_ = 0;
  std::size_t used_ = 0;
  void compress();

public:
  Sha256();
  void update(std::span<const std::uint8_t>);
  Hash finish();
};
Hash pair_hash(const Request&);
Bytes expected_frame(const Case&, Result);
Bytes observed_frame(std::uint64_t, std::uint8_t, Result, std::uint64_t);
void write_bytes(std::ostream&, const Bytes&);
void little(Bytes&, std::uint64_t, unsigned);
constexpr std::uint64_t digest_initial = 0xcbf29ce484222325ULL;
std::uint64_t consume(std::uint64_t, Result);
}  // namespace arch003
