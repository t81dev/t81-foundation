#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace t81::hash {

struct CanonHash81 {
    std::array<std::uint8_t, 32> bytes; // 256-bit hash; size is your call

    bool operator==(const CanonHash81& other) const noexcept {
        return bytes == other.bytes;
    }
};

/// Hash arbitrary bytes into CanonHash81 (not cryptographically strong).
CanonHash81 hash_bytes(const std::vector<std::uint8_t>& data);

/// Hash a string (UTF-8 bytes).
CanonHash81 hash_string(std::string_view s);

/// Encode hash as base-81 string (using t81::codec::base81).
std::string to_string(const CanonHash81& h);

/// Decode hash from base-81 string.
bool from_string(std::string_view s, CanonHash81& out);

} // namespace t81::hash
