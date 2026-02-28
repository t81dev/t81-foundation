#pragma once

#include <cstdint>
#include <string>

// Shared signature-mixing utilities used by VM and Axion determinism tests.
// The mix() constant and XOR pattern are load-bearing: changing them invalidates
// all stored signature baselines.  Do not alter without a full determinism re-baseline.

namespace t81::test {

inline std::uint64_t sig_mix(std::uint64_t seed, std::uint64_t value) {
  return seed ^ (value + 0x9e3779b97f4a7c15ULL + (seed << 6U) + (seed >> 2U));
}

inline void sig_mix_string(const std::string& s, std::uint64_t* state) {
  for (unsigned char c : s) *state = sig_mix(*state, static_cast<std::uint64_t>(c));
}

}  // namespace t81::test
