#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace t81 {

struct CanonHash81 { std::array<char,81> text{}; };

struct CanonRef {
  CanonHash81 target;
  uint16_t permissions{0};
  uint64_t expires_at{0};
};

inline CanonHash81 canonhash81_of_bytes(const void* data, size_t len) {
  (void)data; (void)len; CanonHash81 h{}; return h; // platform seam
}

} // namespace t81

