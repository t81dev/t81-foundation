#include <t81/hash/canonhash.hpp>
#include <t81/codec/base81.hpp>

namespace t81::hash {

CanonHash81 hash_bytes(const std::vector<std::uint8_t>& data) {
    CanonHash81 h{};
    // Very simple, deterministic mixing; not cryptographic.
    // You can upgrade this later as long as you treat the old one as a versioned format.
    std::uint64_t s0 = 0x9e3779b185ebca87ULL;
    std::uint64_t s1 = 0xc2b2ae3d27d4eb4fULL;
    std::uint64_t s2 = 0x165667b19e3779f9ULL;
    std::uint64_t s3 = 0x85ebca6b27d4eb2fULL;

    for (std::uint8_t b : data) {
        s0 ^= b;
        s0 = (s0 << 13) | (s0 >> (64 - 13));
        s1 += s0 * 0x9e3779b185ebca87ULL;

        s1 ^= b;
        s1 = (s1 << 17) | (s1 >> (64 - 17));
        s2 += s1 * 0xc2b2ae3d27d4eb4fULL;

        s2 ^= b;
        s2 = (s2 << 19) | (s2 >> (64 - 19));
        s3 += s2 * 0x165667b19e3779f9ULL;

        s3 ^= b;
        s3 = (s3 << 23) | (s3 >> (64 - 23));
    }

    // Finalization
    std::uint64_t mix[] = {s0, s1, s2, s3};
    std::uint8_t* out = h.bytes.data();
    for (int i = 0; i < 4; ++i) {
        std::uint64_t x = mix[i];
        for (int j = 0; j < 8; ++j) {
            out[i * 8 + j] = static_cast<std::uint8_t>((x >> (8 * j)) & 0xFF);
        }
    }

    return h;
}

CanonHash81 hash_string(std::string_view s) {
    std::vector<std::uint8_t> bytes(s.begin(), s.end());
    return hash_bytes(bytes);
}

std::string to_string(const CanonHash81& h) {
    std::vector<std::uint8_t> bytes(h.bytes.begin(), h.bytes.end());
    return t81::codec::base81::encode_bytes(bytes);
}

bool from_string(std::string_view s, CanonHash81& out) {
    std::vector<std::uint8_t> bytes;
    if (!t81::codec::base81::decode_bytes(s, bytes)) {
        return false;
    }
    if (bytes.size() != out.bytes.size()) {
        return false;
    }
    std::copy(bytes.begin(), bytes.end(), out.bytes.begin());
    return true;
}

} // namespace t81::hash
