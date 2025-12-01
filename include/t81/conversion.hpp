#pragma once

#include <cstdint>
#include <cstring>

#include "t81/native.hpp"
#include "t81/core/T81Limb.hpp"
#include "t81/packing.hpp"

namespace t81 {

// These functions were moved out of detail:: — now in t81 namespace
// (they are defined in native.hpp or will be in a new packing.hpp)
extern void decode_tryte(int8_t value, int8_t digits[3]) noexcept;
extern void encode_tryte(const int8_t digits[3], int8_t& value) noexcept;
extern std::array<uint8_t, 32> pack_digits(const int8_t digits[128]) noexcept;
extern void unpack_digits(const std::array<uint8_t, 32>& block, int8_t digits[128]) noexcept;

inline T81 from_classic(const core::T81Limb& old_limb) {
    alignas(16) int8_t trytes[16];
    std::memcpy(trytes, &old_limb, sizeof(trytes));

    int8_t digits[128]{};
    for (size_t idx = 0; idx < 16; ++idx) {
        int8_t triplet[3];
        decode_tryte(trytes[idx], triplet);
        digits[idx * 3 + 0] = triplet[0];
        digits[idx * 3 + 1] = triplet[1];
        digits[idx * 3 + 2] = triplet[2];
    }
    return T81{pack_digits(digits)};
}

inline core::T81Limb to_classic(const T81& native) {
    int8_t digits[128];
    unpack_digits(native.data, digits);

    alignas(16) int8_t trytes[16]{};
    for (size_t idx = 0; idx < 16; ++idx) {
        int8_t triplet[3] = {
            digits[idx * 3 + 0],
            digits[idx * 3 + 1],
            digits[idx * 3 + 2]
        };
        encode_tryte(triplet, trytes[idx]);
    }

    core::T81Limb limb;
    std::memcpy(&limb, trytes, sizeof(trytes));
    return limb;
}

} // namespace t81
