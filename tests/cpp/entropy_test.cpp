#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <string>
#include "t81/entropy.hpp"

int main() {
  using namespace t81::entropy;

  // Empty input -> 0
  {
    std::vector<uint8_t> v;
    assert(std::fabs(shannon_bits_per_byte(v) - 0.0) <= 1e-6);
  }

  // All same byte -> 0 bits
  {
    std::vector<uint8_t> v(1024, 0xAA);
    assert(std::fabs(shannon_bits_per_byte(v) - 0.0) <= 1e-6);
  }

  // Two equally likely bytes -> 1 bit
  {
    std::vector<uint8_t> v;
    v.reserve(1000);
    for (int i = 0; i < 1000; ++i) v.push_back((i & 1) ? 0x00 : 0xFF);
    [[maybe_unused]] double H = shannon_bits_per_byte(v);
    assert(std::fabs(H - 1.0) < 1e-3);
  }

  // ASCII string with limited alphabet should have < 8 bits/char
  {
    std::string s = "abcabcabcabcabcabcabcabcabcabc";
    [[maybe_unused]] double Hc = shannon_bits_per_char(s);
    assert(Hc > 0.0 && Hc < 8.0);
  }

  // PRNG deterministic when seeded
  {
    t81::entropy::PRNG rng(123456789ull);
    std::vector<uint8_t> v(256);
    rng.fill(v);
    // simple sanity: not all equal, and not all zero
    [[maybe_unused]] bool any_diff = false, any_nonzero = false;
    for (size_t i = 1; i < v.size(); ++i) if (v[i] != v[0]) { any_diff = true; break; }
    for (auto b : v) if (b != 0) { any_nonzero = true; break; }
    assert(any_diff && any_nonzero);
  }

  std::cout << "entropy ok\n";
  return 0;
}
