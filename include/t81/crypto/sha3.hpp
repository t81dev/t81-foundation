#pragma once

#include <array>
#include <cstdint>
#include <span>
#include <string>

namespace t81::crypto {

[[nodiscard]] std::array<uint8_t, 64> sha3_512(std::span<const uint8_t> input) noexcept;
[[nodiscard]] std::string sha3_512_hex(std::span<const uint8_t> input);

}  // namespace t81::crypto
