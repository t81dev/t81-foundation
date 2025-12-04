#ifndef T81_ENUM_META_HPP
#define T81_ENUM_META_HPP

#include <cstdint>
#include <limits>

namespace t81::enum_meta {

constexpr int kVariantIdBits = 16;
constexpr int kVariantIdMask = (1 << kVariantIdBits) - 1;
constexpr int kEnumIdBits = 32 - kVariantIdBits;

constexpr int kEnumIdMask = (1 << kEnumIdBits) - 1;

inline int encode_variant_id(int enum_id, int variant_id) {
    if (enum_id < 0 || variant_id < 0) {
        return -1;
    }
    if (enum_id > kEnumIdMask || variant_id > kVariantIdMask) {
        return -1;
    }
    return (enum_id << kVariantIdBits) | (variant_id & kVariantIdMask);
}

inline int decode_enum_id(int encoded_id) {
    if (encoded_id < 0) {
        return -1;
    }
    return (encoded_id >> kVariantIdBits) & kEnumIdMask;
}

inline int decode_variant_id(int encoded_id) {
    if (encoded_id < 0) {
        return -1;
    }
    return encoded_id & kVariantIdMask;
}

} // namespace t81::enum_meta

#endif // T81_ENUM_META_HPP
