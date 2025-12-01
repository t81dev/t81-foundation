#pragma once

#include <cstdint>
#include <cstring>

#include "t81/native.hpp"
#include "t81/core/T81Limb.hpp"

namespace t81 {

inline T81 from_classic(const core::T81Limb& old_limb) {
    alignas(16) int8_t trytes[16];
    std::memcpy(trytes, &old_limb, sizeof(trytes));
    int8_t digits[128]{};
    for (size_t idx = 0; idx < 16; ++idx) {
        int8_t triplet[3];
        detail::decode_tryte(trytes[idx], triplet);
        digits[idx * 3 + 0] = triplet[0];
        digits[idx * 3 + 1] = triplet[1];
        digits[idx * 3 + 2] = triplet[2];
    }
    return T81{detail::pack_digits(digits)};
}

inline core::T81Limb to_classic(const T81& native) {
    int8_t digits[128];
    detail::unpack_digits(native.data, digits);
    alignas(16) int8_t trytes[16];
    for (size_t idx = 0; idx < 16; ++idx) {
        int8_t triplet[3] = {
            digits[idx * 3 + 0],
            digits[idx * 3 + 1],
            digits[idx * 3 + 2]
        };
        trytes[idx] = detail::encode_tryte(triplet);
    }
    core::T81Limb limb;
    std::memcpy(&limb, trytes, sizeof(trytes));
    return limb;
}

} // namespace t81
