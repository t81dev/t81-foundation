#include "t81/determinism/canon_hash81.hpp"

#include <cstring>

namespace t81::determinism {

namespace {

// SipHash-2-4 implementation
// SipHash reference: https://131002.net/siphash/

#define ROTL(x, b) (std::uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))

#define U32TO8_LE(p, v) \
    (p)[0] = (std::uint8_t)((v)      ); (p)[1] = (std::uint8_t)((v) >>  8); \
    (p)[2] = (std::uint8_t)((v) >> 16); (p)[3] = (std::uint8_t)((v) >> 24);

#define U64TO8_LE(p, v) \
    U32TO8_LE((p),     (std::uint32_t)((v)      )); \
    U32TO8_LE((p) + 4, (std::uint32_t)((v) >> 32));

#define U8TO64_LE(p) \
    (((std::uint64_t)((p)[0])      ) | \
     ((std::uint64_t)((p)[1]) <<  8) | \
     ((std::uint64_t)((p)[2]) << 16) | \
     ((std::uint64_t)((p)[3]) << 24) | \
     ((std::uint64_t)((p)[4]) << 32) | \
     ((std::uint64_t)((p)[5]) << 40) | \
     ((std::uint64_t)((p)[6]) << 48) | \
     ((std::uint64_t)((p)[7]) << 56))

#define SIPROUND \
    do { \
        v0 += v1; v1 = ROTL(v1, 13); v1 ^= v0; v0 = ROTL(v0, 32); \
        v2 += v3; v3 = ROTL(v3, 16); v3 ^= v2; \
        v0 += v3; v3 = ROTL(v3, 21); v3 ^= v0; \
        v2 += v1; v1 = ROTL(v1, 17); v1 ^= v2; v2 = ROTL(v2, 32); \
    } while (0)

std::uint64_t siphash(const std::uint8_t *in, const std::size_t inlen, const std::uint8_t *k) {
    std::uint64_t v0 = 0x736f6d6570736575ULL;
    std::uint64_t v1 = 0x646f72616e646f6dULL;
    std::uint64_t v2 = 0x6c7967656e657261ULL;
    std::uint64_t v3 = 0x7465646279746573ULL;
    std::uint64_t k0 = U8TO64_LE(k);
    std::uint64_t k1 = U8TO64_LE(k + 8);
    std::uint64_t m;
    int i;
    const std::uint8_t *end = in + inlen - (inlen % sizeof(std::uint64_t));
    const int left = inlen & 7;
    std::uint64_t b = ((std::uint64_t)inlen) << 56;
    v3 ^= k1;
    v2 ^= k0;
    v1 ^= k1;
    v0 ^= k0;

    for (; in != end; in += 8) {
        m = U8TO64_LE(in);
        v3 ^= m;
        SIPROUND;
        SIPROUND;
        v0 ^= m;
    }

    switch (left) {
    case 7: b |= ((std::uint64_t)in[6]) << 48; [[fallthrough]];
    case 6: b |= ((std::uint64_t)in[5]) << 40; [[fallthrough]];
    case 5: b |= ((std::uint64_t)in[4]) << 32; [[fallthrough]];
    case 4: b |= ((std::uint64_t)in[3]) << 24; [[fallthrough]];
    case 3: b |= ((std::uint64_t)in[2]) << 16; [[fallthrough]];
    case 2: b |= ((std::uint64_t)in[1]) <<  8; [[fallthrough]];
    case 1: b |= ((std::uint64_t)in[0]); break;
    case 0: break;
    }

    v3 ^= b;
    SIPROUND;
    SIPROUND;
    v0 ^= b;

    v2 ^= 0xff;

    SIPROUND;
    SIPROUND;
    SIPROUND;
    SIPROUND;

    return v0 ^ v1 ^ v2 ^ v3;
}

} // namespace

std::uint64_t canon_hash81(const void* data, std::size_t length) noexcept {
  // Use a fixed key to guarantee deterministic output
  const std::uint8_t k[16] = {
      0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
      0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F
  };
  return siphash(static_cast<const std::uint8_t*>(data), length, k);
}

}  // namespace t81::determinism