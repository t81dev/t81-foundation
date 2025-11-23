#pragma once
#include <array>
#include <cstdint>
#include <cstring>
#include <string>

namespace t81 {

// Fixed-size canonical Base-81 hash text buffer (zero-padded).
struct CanonHash81 {
  std::array<char, 81> text{};

  CanonHash81() = default;

  // Build from std::string (truncates/pads to 81).
  static CanonHash81 from_string(const std::string& s) {
    CanonHash81 h;
    const std::size_t n = s.size() < h.text.size() ? s.size() : h.text.size();
    std::memcpy(h.text.data(), s.data(), n);
    // remaining bytes are already zero-init
    return h;
  }

  // Return as std::string stopping at the first '\0' (if any).
  std::string to_string() const {
    // Find first zero terminator, if present.
    std::size_t n = 0;
    while (n < text.size() && text[n] != '\0') ++n;
    return std::string(text.data(), n);
  }

  void clear() { text.fill(0); }

  friend bool operator==(const CanonHash81& a, const CanonHash81& b) {
    return std::memcmp(a.text.data(), b.text.data(), a.text.size()) == 0;
  }
  friend bool operator!=(const CanonHash81& a, const CanonHash81& b) { return !(a == b); }
};

// Simple capability-style reference to a canonical object.
struct CanonRef {
  CanonHash81 target{};     // canonical hash text (81 bytes)
  uint16_t    permissions{0};
  uint64_t    expires_at{0}; // epoch seconds; 0 = never

  // Helpers
  static CanonRef make(const CanonHash81& t, uint16_t perms = 0, uint64_t exp = 0) {
    CanonRef r; r.target = t; r.permissions = perms; r.expires_at = exp; return r;
  }
};

// Optional permission bits (example; extend as needed)
enum : uint16_t {
  CANON_PERM_READ   = 1u << 0,
  CANON_PERM_WRITE  = 1u << 1,
  CANON_PERM_APPEND = 1u << 2,
  CANON_PERM_ADMIN  = 1u << 15
};

} // namespace t81
