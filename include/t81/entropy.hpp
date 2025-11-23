#pragma once
#include <array>
#include <cstdint>
#include <random>
#include <string>
#include <vector>
#include <stdexcept>

#include "t81/ternary.hpp"

namespace t81::entropy {

// --- Simple Shannon entropy (bits per symbol) over bytes ---
inline double shannon_bits_per_byte(const std::vector<uint8_t>& data) {
  if (data.empty()) return 0.0;
  std::array<uint64_t, 256> freq{};
  for (uint8_t b : data) ++freq[b];

  const double n = static_cast<double>(data.size());
  double H = 0.0;
  for (uint64_t f : freq) {
    if (!f) continue;
    const double p = static_cast<double>(f) / n;
    H -= p * std::log2(p);
  }
  return H; // in [0,8]
}

// --- Very small PRNG facade (non-cryptographic) ---
class PRNG {
public:
  PRNG() : eng_(seed_()) {}
  explicit PRNG(uint64_t seed) : eng_(seed) {}

  // Uniform byte
  uint8_t u8() { return static_cast<uint8_t>(dist_(eng_)); }

  // Fill buffer with random bytes
  void fill(std::vector<uint8_t>& out) {
    for (auto& b : out) b = u8();
  }

  // Random balanced Trit {-1,0,+1} (equiprobable)
  Trit trit() {
    uint32_t r = dist_(eng_) % 3;
    return r == 0 ? Trit::Neg : (r == 1 ? Trit::Zero : Trit::Pos);
  }

private:
  static uint64_t seed_() {
    // Mix multiple std::random_device reads (may be deterministic on some platforms)
    std::random_device rd;
    uint64_t s = 0x9E3779B185EBCA87ull;
    for (int i = 0; i < 4; ++i) {
      s ^= (static_cast<uint64_t>(rd()) + 0xBF58476D1CE4E5B9ull + (s<<6) + (s>>2));
    }
    return s;
  }

  std::mt19937_64 eng_;
  std::uniform_int_distribution<uint64_t> dist_{0, 0xFF};
};

// Convenience: estimate bits/symbol of a string (ASCII treated as bytes)
inline double shannon_bits_per_char(const std::string& s) {
  return shannon_bits_per_byte(std::vector<uint8_t>(s.begin(), s.end()));
}

} // namespace t81::entropy
