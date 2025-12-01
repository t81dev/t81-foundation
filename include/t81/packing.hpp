#pragma once

#include <array>
#include <cstdint>

namespace t81 {

inline void decode_tryte(int8_t value, int8_t digits[3]) noexcept {
    bool negative = value < 0;
    int temp = negative ? -value : value;
    for (int i = 0; i < 3; ++i) {
        int rem = temp % 3;
        digits[i] = (rem == 2) ? -1 : static_cast<int8_t>(rem - 1);
        temp = (rem == 2) ? (temp / 3 + 1) : (temp / 3);
    }
    if (negative) {
        digits[0] = -digits[0];
        digits[1] = -digits[1];
        digits[2] = -digits[2];
    }
}

inline void encode_tryte(const int8_t digits[3], int8_t& value) noexcept {
    value = digits[0] + 3 * digits[1] + 9 * digits[2];
}

inline std::array<uint8_t, 32> pack_digits(const int8_t digits[128]) noexcept {
    std::array<uint8_t, 32> bytes{};
    for (size_t i = 0; i < 32; ++i) {
        uint8_t byte = 0;
        for (int j = 0; j < 4; ++j) {
            int8_t d = digits[i * 4 + j];
            uint8_t code = (d == -1) ? 0 : (d == 0 ? 1 : 2);
            byte |= code << (j * 2);
        }
        bytes[i] = byte;
    }
    return bytes;
}

inline void unpack_digits(const std::array<uint8_t, 32>& bytes, int8_t digits[128]) noexcept {
    for (size_t i = 0; i < 32; ++i) {
        uint8_t byte = bytes[i];
        for (int j = 0; j < 4; ++j) {
            uint8_t code = (byte >> (j * 2)) & 0x3;
            digits[i * 4 + j] = (code == 0) ? -1 : (code == 1 ? 0 : 1);
        }
    }
}

} // namespace t81
