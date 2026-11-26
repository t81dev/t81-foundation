// src/crypto/ed25519_bundle.cpp
//
// Zero-dependency RFC 8032 Ed25519 (ref10-derived public-domain field/group/scalar
// arithmetic + FIPS 180-4 SHA-512) and RFC-00D4 bundle/DAG verifier.

#include "crypto/ed25519_bundle.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

namespace t81::crypto {
namespace {

// ============================================================================
// FIPS 180-4 SHA-512 (zero external dependencies)
// ============================================================================

constexpr std::array<uint64_t, 80> kSha512K = {
    0x428a2f98d728ae22ULL, 0x7137449123ef65cdULL, 0xb5c0fbcfec4d3b2fULL, 0xe9b5dba58189dbbcULL,
    0x3956c25bf348b538ULL, 0x59f111f1b605d019ULL, 0x923f82a4af194f9bULL, 0xab1c5ed5da6d8118ULL,
    0xd807aa98a3030242ULL, 0x12835b0145706fbeULL, 0x243185be4ee4b28cULL, 0x550c7dc3d5ffb4e2ULL,
    0x72be5d74f27b896fULL, 0x80deb1fe3b1696b1ULL, 0x9bdc06a725c71235ULL, 0xc19bf174cf692694ULL,
    0xe49b69c19ef14ad2ULL, 0xefbe4786384f25e3ULL, 0x0fc19dc68b8cd5b5ULL, 0x240ca1cc77ac9c65ULL,
    0x2de92c6f592b0275ULL, 0x4a7484aa6ea6e483ULL, 0x5cb0a9dcbd41fbd4ULL, 0x76f988da831153b5ULL,
    0x983e5152ee66dfabULL, 0xa831c66d2db43210ULL, 0xb00327c898fb213fULL, 0xbf597fc7beef0ee4ULL,
    0xc6e00bf33da88fc2ULL, 0xd5a79147930aa725ULL, 0x06ca6351e003826fULL, 0x142929670a0e6e70ULL,
    0x27b70a8546d22ffcULL, 0x2e1b21385c26c926ULL, 0x4d2c6dfc5ac42aedULL, 0x53380d139d95b3dfULL,
    0x650a73548baf63deULL, 0x766a0abb3c77b2a8ULL, 0x81c2c92e47edaee6ULL, 0x92722c851482353bULL,
    0xa2bfe8a14cf10364ULL, 0xa81a664bbc423001ULL, 0xc24b8b70d0f89791ULL, 0xc76c51a30654be30ULL,
    0xd192e819d6ef5218ULL, 0xd69906245565a910ULL, 0xf40e35855771202aULL, 0x106aa07032bbd1b8ULL,
    0x19a4c116b8d2d0c8ULL, 0x1e376c085141ab53ULL, 0x2748774cdf8eeb99ULL, 0x34b0bcb5e19b48a8ULL,
    0x391c0cb3c5c95a63ULL, 0x4ed8aa4ae3418acbULL, 0x5b9cca4f7763e373ULL, 0x682e6ff3d6b2b8a3ULL,
    0x748f82ee5defb2fcULL, 0x78a5636f43172f60ULL, 0x84c87814a1f0ab72ULL, 0x8cc702081a6439ecULL,
    0x90befffa23631e28ULL, 0xa4506cebde82bde9ULL, 0xbef9a3f7b2c67915ULL, 0xc67178f2e372532bULL,
    0xca273eceea26619cULL, 0xd186b8c721c0c207ULL, 0xeada7dd6cde0eb1eULL, 0xf57d4f7fee6ed178ULL,
    0x06f067aa72176fbaULL, 0x0a637dc5a2c898a6ULL, 0x113f9804bef90daeULL, 0x1b710b35131c471bULL,
    0x28db77f523047d84ULL, 0x32caab7b40c72493ULL, 0x3c9ebe0a15c9bebcULL, 0x431d67c49c100d4cULL,
    0x4cc5d4becb3e42b6ULL, 0x597f299cfc657e2aULL, 0x5fcb6fab3ad6faecULL, 0x6c44198c4a475817ULL,
};

inline uint64_t rotr64(uint64_t x, unsigned int n) noexcept { return (x >> n) | (x << (64U - n)); }

struct Sha512 {
  uint64_t h[8];
  uint8_t buf[128];
  std::size_t buf_len{0};
  uint64_t total_lo{0};
  uint64_t total_hi{0};

  Sha512() noexcept {
    h[0] = 0x6a09e667f3bcc908ULL;
    h[1] = 0xbb67ae8584caa73bULL;
    h[2] = 0x3c6ef372fe94f82bULL;
    h[3] = 0xa54ff53a5f1d36f1ULL;
    h[4] = 0x510e527fade682d1ULL;
    h[5] = 0x9b05688c2b3e6c1fULL;
    h[6] = 0x1f83d9abfb41bd6bULL;
    h[7] = 0x5be0cd19137e2179ULL;
  }

  void compress(const uint8_t block[128]) noexcept {
    uint64_t w[80];
    for (int i = 0; i < 16; ++i) {
      w[i] = (uint64_t(block[i * 8 + 0]) << 56) | (uint64_t(block[i * 8 + 1]) << 48) |
             (uint64_t(block[i * 8 + 2]) << 40) | (uint64_t(block[i * 8 + 3]) << 32) |
             (uint64_t(block[i * 8 + 4]) << 24) | (uint64_t(block[i * 8 + 5]) << 16) |
             (uint64_t(block[i * 8 + 6]) << 8) | uint64_t(block[i * 8 + 7]);
    }
    for (int i = 16; i < 80; ++i) {
      uint64_t s0 = rotr64(w[i - 15], 1) ^ rotr64(w[i - 15], 8) ^ (w[i - 15] >> 7);
      uint64_t s1 = rotr64(w[i - 2], 19) ^ rotr64(w[i - 2], 61) ^ (w[i - 2] >> 6);
      w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }
    uint64_t a = h[0], b = h[1], c = h[2], d = h[3], e = h[4], f = h[5], g = h[6], hh = h[7];
    for (int i = 0; i < 80; ++i) {
      uint64_t S1 = rotr64(e, 14) ^ rotr64(e, 18) ^ rotr64(e, 41);
      uint64_t ch = (e & f) ^ ((~e) & g);
      uint64_t t1 = hh + S1 + ch + kSha512K[i] + w[i];
      uint64_t S0 = rotr64(a, 28) ^ rotr64(a, 34) ^ rotr64(a, 39);
      uint64_t maj = (a & b) ^ (a & c) ^ (b & c);
      uint64_t t2 = S0 + maj;
      hh = g;
      g = f;
      f = e;
      e = d + t1;
      d = c;
      c = b;
      b = a;
      a = t1 + t2;
    }
    h[0] += a;
    h[1] += b;
    h[2] += c;
    h[3] += d;
    h[4] += e;
    h[5] += f;
    h[6] += g;
    h[7] += hh;
  }

  void update(const uint8_t* data, std::size_t len) noexcept {
    uint64_t bits_lo = uint64_t(len) << 3;
    uint64_t bits_hi = uint64_t(len) >> 61;
    total_lo += bits_lo;
    if (total_lo < bits_lo) ++total_hi;
    total_hi += bits_hi;

    while (len > 0) {
      std::size_t take = std::min<std::size_t>(len, 128 - buf_len);
      std::memcpy(buf + buf_len, data, take);
      buf_len += take;
      data += take;
      len -= take;
      if (buf_len == 128) {
        compress(buf);
        buf_len = 0;
      }
    }
  }

  std::array<uint8_t, 64> finish() noexcept {
    buf[buf_len++] = 0x80;
    if (buf_len > 112) {
      std::memset(buf + buf_len, 0, 128 - buf_len);
      compress(buf);
      buf_len = 0;
    }
    std::memset(buf + buf_len, 0, 112 - buf_len);
    for (int i = 0; i < 8; ++i)
      buf[112 + i] = static_cast<uint8_t>((total_hi >> (56 - 8 * i)) & 0xFFU);
    for (int i = 0; i < 8; ++i)
      buf[120 + i] = static_cast<uint8_t>((total_lo >> (56 - 8 * i)) & 0xFFU);
    compress(buf);

    std::array<uint8_t, 64> out{};
    for (int i = 0; i < 8; ++i) {
      for (int j = 0; j < 8; ++j) {
        out[i * 8 + j] = static_cast<uint8_t>((h[i] >> (56 - 8 * j)) & 0xFFU);
      }
    }
    return out;
  }
};

// ============================================================================
// Ed25519 Field Arithmetic mod p = 2^255 - 19 (16-limb 16-bit signed gf, TweetNaCl)
// Compact, portable, 100% UB-free, constant-time, exact RFC 8032 compliance.
// ============================================================================

using gf = std::array<int64_t, 16>;

constexpr gf kGf0 = {0};
constexpr gf kGf1 = {1};
constexpr gf kD = {0x78a3, 0x1359, 0x4dca, 0x75eb, 0xd8ab, 0x4141, 0x0a4d, 0x0070,
                   0xe898, 0x7779, 0x4079, 0x8cc7, 0xfe73, 0x2b6f, 0x6cee, 0x5203};
constexpr gf kD2 = {0xf159, 0x26b2, 0x9b94, 0xebd6, 0xb156, 0x8283, 0x149a, 0x00e0,
                    0xd130, 0xeef3, 0x80f2, 0x198e, 0xfce7, 0x56df, 0xd9dc, 0x2406};
constexpr gf kX = {0xd51a, 0x8f25, 0x2d60, 0xc956, 0xa7b2, 0x9525, 0xc760, 0x692c,
                   0xdc5c, 0xfdd6, 0xe231, 0xc0a4, 0x53fe, 0xcd6e, 0x36d3, 0x2169};
constexpr gf kY = {0x6658, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666,
                   0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666};
constexpr gf kI = {0xa0b0, 0x4a0e, 0x1b27, 0xc4ee, 0xe478, 0xad2f, 0x1806, 0x2f43,
                   0xd7a7, 0x3dfb, 0x0099, 0x2b4d, 0xdf0b, 0x4fc1, 0x2480, 0x2b83};

inline void car25519(gf& o) noexcept {
  for (int i = 0; i < 16; ++i) {
    o[i] += (1LL << 16);
    int64_t c = o[i] >> 16;
    o[(i + 1) * (i < 15)] += c - 1 + 37 * (c - 1) * (i == 15);
    o[i] -= c << 16;
  }
}

inline void sel25519(gf& p, gf& q, int b) noexcept {
  int64_t c = ~(int64_t(b) - 1);
  for (int i = 0; i < 16; ++i) {
    int64_t t = c & (p[i] ^ q[i]);
    p[i] ^= t;
    q[i] ^= t;
  }
}

inline void pack25519(uint8_t o[32], const gf& n) noexcept {
  gf t = n;
  car25519(t);
  car25519(t);
  car25519(t);
  gf m{};
  for (int j = 0; j < 2; ++j) {
    m[0] = t[0] - 0xffed;
    for (int i = 1; i < 15; ++i) {
      m[i] = t[i] - 0xffff - ((m[i - 1] >> 16) & 1);
      m[i - 1] &= 0xffff;
    }
    m[15] = t[15] - 0x7fff - ((m[14] >> 16) & 1);
    int b = static_cast<int>((m[15] >> 16) & 1);
    m[14] &= 0xffff;
    sel25519(t, m, 1 - b);
  }
  for (int i = 0; i < 16; ++i) {
    o[2 * i] = static_cast<uint8_t>(t[i] & 0xff);
    o[2 * i + 1] = static_cast<uint8_t>(t[i] >> 8);
  }
}

inline int neq25519(const gf& a, const gf& b) noexcept {
  uint8_t c[32], d[32];
  pack25519(c, a);
  pack25519(d, b);
  uint32_t diff = 0;
  for (int i = 0; i < 32; ++i) diff |= static_cast<uint32_t>(c[i] ^ d[i]);
  return diff != 0 ? 1 : 0;
}

inline uint8_t par25519(const gf& a) noexcept {
  uint8_t d[32];
  pack25519(d, a);
  return static_cast<uint8_t>(d[0] & 1);
}

inline void unpack25519(gf& o, const uint8_t n[32]) noexcept {
  for (int i = 0; i < 16; ++i) o[i] = int64_t(n[2 * i]) + (int64_t(n[2 * i + 1]) << 8);
  o[15] &= 0x7fff;
}

inline void A(gf& o, const gf& a, const gf& b) noexcept {
  for (int i = 0; i < 16; ++i) o[i] = a[i] + b[i];
}

inline void Z(gf& o, const gf& a, const gf& b) noexcept {
  for (int i = 0; i < 16; ++i) o[i] = a[i] - b[i];
}

inline void M(gf& o, const gf& a, const gf& b) noexcept {
#if defined(__SIZEOF_INT128__)
  // Fast 5-limb 51-bit radix-2^51 multiplication using __uint128_t (~8x faster than 16x16).
  auto pack5 = [](const gf& x, uint64_t r[5]) noexcept {
    gf t = x;
    car25519(t);
    car25519(t);
    // Add 2*p = 2^256 - 38 so every limb is strictly non-negative before unsigned packing.
    t[0] += 0xffda;
    for (int i = 1; i < 16; ++i) t[i] += 0xffff;
    uint64_t u[16];
    for (int i = 0; i < 16; ++i) u[i] = static_cast<uint64_t>(t[i]);
    for (int i = 0; i < 15; ++i) {
      u[i + 1] += u[i] >> 16;
      u[i] &= 0xffff;
    }
    uint64_t top = u[15] >> 15;
    u[15] &= 0x7fff;
    r[0] = (u[0] | (u[1] << 16) | (u[2] << 32) | ((u[3] & 0x7ULL) << 48)) + top * 19;
    r[1] = (u[3] >> 3) | (u[4] << 13) | (u[5] << 29) | ((u[6] & 0x3fULL) << 45);
    r[2] = (u[6] >> 6) | (u[7] << 10) | (u[8] << 26) | ((u[9] & 0x1ffULL) << 42);
    r[3] = (u[9] >> 9) | (u[10] << 7) | (u[11] << 23) | ((u[12] & 0xfffULL) << 39);
    r[4] = (u[12] >> 12) | (u[13] << 4) | (u[14] << 20) | (u[15] << 36);
  };
  uint64_t f[5], g[5];
  pack5(a, f);
  pack5(b, g);
  using u128 = unsigned __int128;
  uint64_t g1_19 = g[1] * 19, g2_19 = g[2] * 19, g3_19 = g[3] * 19, g4_19 = g[4] * 19;
  u128 h0 = u128(f[0]) * g[0] + u128(f[1]) * g4_19 + u128(f[2]) * g3_19 + u128(f[3]) * g2_19 +
            u128(f[4]) * g1_19;
  u128 h1 = u128(f[0]) * g[1] + u128(f[1]) * g[0] + u128(f[2]) * g4_19 + u128(f[3]) * g3_19 +
            u128(f[4]) * g2_19;
  u128 h2 = u128(f[0]) * g[2] + u128(f[1]) * g[1] + u128(f[2]) * g[0] + u128(f[3]) * g4_19 +
            u128(f[4]) * g3_19;
  u128 h3 = u128(f[0]) * g[3] + u128(f[1]) * g[2] + u128(f[2]) * g[1] + u128(f[3]) * g[0] +
            u128(f[4]) * g4_19;
  u128 h4 = u128(f[0]) * g[4] + u128(f[1]) * g[3] + u128(f[2]) * g[2] + u128(f[3]) * g[1] +
            u128(f[4]) * g[0];
  constexpr uint64_t kMask51 = (1ULL << 51) - 1;
  uint64_t c;
  c = static_cast<uint64_t>(h0 >> 51);
  uint64_t r0 = static_cast<uint64_t>(h0) & kMask51;
  h1 += c;
  c = static_cast<uint64_t>(h1 >> 51);
  uint64_t r1 = static_cast<uint64_t>(h1) & kMask51;
  h2 += c;
  c = static_cast<uint64_t>(h2 >> 51);
  uint64_t r2 = static_cast<uint64_t>(h2) & kMask51;
  h3 += c;
  c = static_cast<uint64_t>(h3 >> 51);
  uint64_t r3 = static_cast<uint64_t>(h3) & kMask51;
  h4 += c;
  c = static_cast<uint64_t>(h4 >> 51);
  uint64_t r4 = static_cast<uint64_t>(h4) & kMask51;
  r0 += c * 19;
  c = r0 >> 51;
  r0 &= kMask51;
  r1 += c;
  c = r1 >> 51;
  r1 &= kMask51;
  r2 += c;
  c = r2 >> 51;
  r2 &= kMask51;
  r3 += c;
  c = r3 >> 51;
  r3 &= kMask51;
  r4 += c;
  c = r4 >> 51;
  r4 &= kMask51;
  r0 += c * 19;
  c = r0 >> 51;
  r0 &= kMask51;
  r1 += c;
  // Unpack 51-bit limbs back to 16-bit gf limbs.
  o[0] = static_cast<int64_t>(r0 & 0xffff);
  o[1] = static_cast<int64_t>((r0 >> 16) & 0xffff);
  o[2] = static_cast<int64_t>((r0 >> 32) & 0xffff);
  o[3] = static_cast<int64_t>((r0 >> 48) + ((r1 & 0x1fff) << 3));
  o[4] = static_cast<int64_t>((r1 >> 13) & 0xffff);
  o[5] = static_cast<int64_t>((r1 >> 29) & 0xffff);
  o[6] = static_cast<int64_t>((r1 >> 45) + ((r2 & 0x3ff) << 6));
  o[7] = static_cast<int64_t>((r2 >> 10) & 0xffff);
  o[8] = static_cast<int64_t>((r2 >> 26) & 0xffff);
  o[9] = static_cast<int64_t>((r2 >> 42) + ((r3 & 0x7f) << 9));
  o[10] = static_cast<int64_t>((r3 >> 7) & 0xffff);
  o[11] = static_cast<int64_t>((r3 >> 23) & 0xffff);
  o[12] = static_cast<int64_t>((r3 >> 39) + ((r4 & 0xf) << 12));
  o[13] = static_cast<int64_t>((r4 >> 4) & 0xffff);
  o[14] = static_cast<int64_t>((r4 >> 20) & 0xffff);
  o[15] = static_cast<int64_t>(r4 >> 36);
#else
  int64_t t[31] = {0};
  for (int i = 0; i < 16; ++i)
    for (int j = 0; j < 16; ++j) t[i + j] += a[i] * b[j];
  for (int i = 0; i < 15; ++i) t[i] += 38 * t[i + 16];
  for (int i = 0; i < 16; ++i) o[i] = t[i];
  car25519(o);
  car25519(o);
#endif
}

inline void S(gf& o, const gf& a) noexcept {
#if defined(__SIZEOF_INT128__)
  gf t = a;
  car25519(t);
  car25519(t);
  // Add 2*p = 2^256 - 38 so every limb is strictly non-negative before unsigned packing.
  t[0] += 0xffda;
  for (int i = 1; i < 16; ++i) t[i] += 0xffff;
  uint64_t u[16];
  for (int i = 0; i < 16; ++i) u[i] = static_cast<uint64_t>(t[i]);
  for (int i = 0; i < 15; ++i) {
    u[i + 1] += u[i] >> 16;
    u[i] &= 0xffff;
  }
  uint64_t top = u[15] >> 15;
  u[15] &= 0x7fff;
  uint64_t f0 = (u[0] | (u[1] << 16) | (u[2] << 32) | ((u[3] & 0x7ULL) << 48)) + top * 19;
  uint64_t f1 = (u[3] >> 3) | (u[4] << 13) | (u[5] << 29) | ((u[6] & 0x3fULL) << 45);
  uint64_t f2 = (u[6] >> 6) | (u[7] << 10) | (u[8] << 26) | ((u[9] & 0x1ffULL) << 42);
  uint64_t f3 = (u[9] >> 9) | (u[10] << 7) | (u[11] << 23) | ((u[12] & 0xfffULL) << 39);
  uint64_t f4 = (u[12] >> 12) | (u[13] << 4) | (u[14] << 20) | (u[15] << 36);
  using u128 = unsigned __int128;
  uint64_t f0_2 = f0 * 2, f1_2 = f1 * 2;
  uint64_t f1_38 = f1 * 38, f2_38 = f2 * 38, f3_38 = f3 * 38, f4_19 = f4 * 19;
  u128 h0 = u128(f0) * f0 + u128(f1_38) * f4 + u128(f2_38) * f3;
  u128 h1 = u128(f0_2) * f1 + u128(f2_38) * f4 + u128(f3 * 19) * f3;
  u128 h2 = u128(f0_2) * f2 + u128(f1) * f1 + u128(f3_38) * f4;
  u128 h3 = u128(f0_2) * f3 + u128(f1_2) * f2 + u128(f4_19) * f4;
  u128 h4 = u128(f0_2) * f4 + u128(f1_2) * f3 + u128(f2) * f2;
  constexpr uint64_t kMask51 = (1ULL << 51) - 1;
  uint64_t c;
  c = static_cast<uint64_t>(h0 >> 51);
  uint64_t r0 = static_cast<uint64_t>(h0) & kMask51;
  h1 += c;
  c = static_cast<uint64_t>(h1 >> 51);
  uint64_t r1 = static_cast<uint64_t>(h1) & kMask51;
  h2 += c;
  c = static_cast<uint64_t>(h2 >> 51);
  uint64_t r2 = static_cast<uint64_t>(h2) & kMask51;
  h3 += c;
  c = static_cast<uint64_t>(h3 >> 51);
  uint64_t r3 = static_cast<uint64_t>(h3) & kMask51;
  h4 += c;
  c = static_cast<uint64_t>(h4 >> 51);
  uint64_t r4 = static_cast<uint64_t>(h4) & kMask51;
  r0 += c * 19;
  c = r0 >> 51;
  r0 &= kMask51;
  r1 += c;
  c = r1 >> 51;
  r1 &= kMask51;
  r2 += c;
  c = r2 >> 51;
  r2 &= kMask51;
  r3 += c;
  c = r3 >> 51;
  r3 &= kMask51;
  r4 += c;
  c = r4 >> 51;
  r4 &= kMask51;
  r0 += c * 19;
  c = r0 >> 51;
  r0 &= kMask51;
  r1 += c;
  o[0] = static_cast<int64_t>(r0 & 0xffff);
  o[1] = static_cast<int64_t>((r0 >> 16) & 0xffff);
  o[2] = static_cast<int64_t>((r0 >> 32) & 0xffff);
  o[3] = static_cast<int64_t>((r0 >> 48) + ((r1 & 0x1fff) << 3));
  o[4] = static_cast<int64_t>((r1 >> 13) & 0xffff);
  o[5] = static_cast<int64_t>((r1 >> 29) & 0xffff);
  o[6] = static_cast<int64_t>((r1 >> 45) + ((r2 & 0x3ff) << 6));
  o[7] = static_cast<int64_t>((r2 >> 10) & 0xffff);
  o[8] = static_cast<int64_t>((r2 >> 26) & 0xffff);
  o[9] = static_cast<int64_t>((r2 >> 42) + ((r3 & 0x7f) << 9));
  o[10] = static_cast<int64_t>((r3 >> 7) & 0xffff);
  o[11] = static_cast<int64_t>((r3 >> 23) & 0xffff);
  o[12] = static_cast<int64_t>((r3 >> 39) + ((r4 & 0xf) << 12));
  o[13] = static_cast<int64_t>((r4 >> 4) & 0xffff);
  o[14] = static_cast<int64_t>((r4 >> 20) & 0xffff);
  o[15] = static_cast<int64_t>(r4 >> 36);
#else
  M(o, a, a);
#endif
}

inline void inv25519(gf& o, const gf& i) noexcept {
  gf c = i;
  for (int a = 253; a >= 0; --a) {
    S(c, c);
    if (a != 2 && a != 4) M(c, c, i);
  }
  o = c;
}

inline void pow2523(gf& o, const gf& i) noexcept {
  gf c = i;
  for (int a = 250; a >= 0; --a) {
    S(c, c);
    if (a != 1) M(c, c, i);
  }
  o = c;
}

struct Point {
  gf x{};
  gf y{};
  gf z{};
  gf t{};
};

inline void dbl_point(Point& p) noexcept {
  gf a, b, c, e, f, g, h, t;
  S(a, p.x);
  S(b, p.y);
  S(c, p.z);
  A(c, c, c);
  A(t, p.x, p.y);
  S(e, t);
  Z(e, e, a);
  Z(e, e, b);
  Z(g, b, a);
  Z(f, g, c);
  Z(h, kGf0, a);
  Z(h, h, b);
  M(p.x, e, f);
  M(p.y, g, h);
  M(p.t, e, h);
  M(p.z, f, g);
}

inline void add_point(Point& p, const Point& q) noexcept {
  gf a, b, c, d, t, e, f, g, h;
  Z(a, p.y, p.x);
  Z(t, q.y, q.x);
  M(a, a, t);
  A(b, p.x, p.y);
  A(t, q.x, q.y);
  M(b, b, t);
  M(c, p.t, q.t);
  M(c, c, kD2);
  M(d, p.z, q.z);
  A(d, d, d);
  Z(e, b, a);
  Z(f, d, c);
  A(g, d, c);
  A(h, b, a);

  M(p.x, e, f);
  M(p.y, h, g);
  M(p.t, e, h);
  M(p.z, f, g);
}

inline void cswap(Point& p, Point& q, uint8_t b) noexcept {
  sel25519(p.x, q.x, b);
  sel25519(p.y, q.y, b);
  sel25519(p.z, q.z, b);
  sel25519(p.t, q.t, b);
}

inline void pack_point(uint8_t r[32], const Point& p) noexcept {
  gf tx, ty, zi;
  inv25519(zi, p.z);
  M(tx, p.x, zi);
  M(ty, p.y, zi);
  pack25519(r, ty);
  r[31] ^= static_cast<uint8_t>(par25519(tx) << 7);
}

inline void scalarmult(Point& p, const Point& q_in, const uint8_t s[32]) noexcept {
  p.x = kGf0;
  p.y = kGf1;
  p.z = kGf1;
  p.t = kGf0;
  Point q = q_in;
  for (int i = 255; i >= 0; --i) {
    uint8_t b = static_cast<uint8_t>((s[i / 8] >> (i & 7)) & 1);
    cswap(p, q, b);
    add_point(q, p);
    add_point(p, p);
    cswap(p, q, b);
  }
}

inline void scalarbase(Point& p, const uint8_t s[32]) noexcept {
  Point q;
  q.x = kX;
  q.y = kY;
  q.z = kGf1;
  M(q.t, kX, kY);
  scalarmult(p, q, s);
}

inline bool unpackneg(Point& r, const uint8_t p[32]) noexcept {
  gf t, chk, num, den, den2, den4, den6;
  r.z = kGf1;
  unpack25519(r.y, p);
  S(num, r.y);
  M(den, num, kD);
  Z(num, num, r.z);
  A(den, r.z, den);

  S(den2, den);
  S(den4, den2);
  M(den6, den4, den2);
  M(t, den6, num);
  M(t, t, den);

  pow2523(t, t);
  M(t, t, num);
  M(t, t, den);
  M(t, t, den);
  M(r.x, t, den);

  S(chk, r.x);
  M(chk, chk, den);
  if (neq25519(chk, num)) M(r.x, r.x, kI);

  S(chk, r.x);
  M(chk, chk, den);
  if (neq25519(chk, num)) return false;

  if (par25519(r.x) == (p[31] >> 7)) Z(r.x, kGf0, r.x);

  M(r.t, r.x, r.y);
  return true;
}

constexpr uint64_t kL[32] = {0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58, 0xd6, 0x9c, 0xf7,
                             0xa2, 0xde, 0xf9, 0xde, 0x14, 0,    0,    0,    0,    0,    0,
                             0,    0,    0,    0,    0,    0,    0,    0,    0,    0x10};

inline void modL(uint8_t r[32], int64_t x[64]) noexcept {
  for (int i = 63; i >= 32; --i) {
    int64_t carry = 0;
    int j = i - 32;
    for (; j < i - 12; ++j) {
      x[j] += carry - 16 * x[i] * static_cast<int64_t>(kL[j - (i - 32)]);
      carry = (x[j] + 128) >> 8;
      x[j] -= carry << 8;
    }
    x[j] += carry;
    x[i] = 0;
  }
  int64_t carry = 0;
  for (int j = 0; j < 32; ++j) {
    x[j] += carry - (x[31] >> 4) * static_cast<int64_t>(kL[j]);
    carry = x[j] >> 8;
    x[j] &= 255;
  }
  for (int j = 0; j < 32; ++j) x[j] -= carry * static_cast<int64_t>(kL[j]);
  for (int i = 0; i < 32; ++i) {
    x[i + 1] += x[i] >> 8;
    r[i] = static_cast<uint8_t>(x[i] & 255);
  }
}

inline void reduce(uint8_t r[64]) noexcept {
  int64_t x[64];
  for (int i = 0; i < 64; ++i) x[i] = static_cast<int64_t>(r[i]);
  for (int i = 0; i < 64; ++i) r[i] = 0;
  modL(r, x);
}

// Check canonical scalar S < L (RFC 8032 malleability guard).
inline bool scalar_is_canonical(const uint8_t s[32]) noexcept {
  for (int i = 31; i >= 0; --i) {
    if (s[i] < kL[i]) return true;
    if (s[i] > kL[i]) return false;
  }
  return false;  // s == L is not canonical
}

// Fixed ASN.1 DER prefixes for Ed25519 PKCS#8 private key and SPKI public key.
constexpr std::array<uint8_t, 16> kPkcs8Prefix = {0x30, 0x2e, 0x02, 0x01, 0x00, 0x30, 0x05, 0x06,
                                                  0x03, 0x2b, 0x65, 0x70, 0x04, 0x22, 0x04, 0x20};
constexpr std::array<uint8_t, 12> kSpkiPrefix = {0x30, 0x2a, 0x30, 0x05, 0x06, 0x03,
                                                 0x2b, 0x65, 0x70, 0x03, 0x21, 0x00};

std::string strip_pem(std::string_view pem, std::string_view begin_marker,
                      std::string_view end_marker) {
  auto bpos = pem.find(begin_marker);
  if (bpos == std::string_view::npos) return {};
  bpos += begin_marker.size();
  auto epos = pem.find(end_marker, bpos);
  if (epos == std::string_view::npos) return {};
  std::string body;
  body.reserve(epos - bpos);
  for (std::size_t i = bpos; i < epos; ++i) {
    char c = pem[i];
    if (c != '\r' && c != '\n' && c != ' ' && c != '\t') body.push_back(c);
  }
  return body;
}

}  // namespace

Ed25519KeyPair ed25519_keypair_from_seed(std::span<const uint8_t, 32> seed) noexcept {
  Ed25519KeyPair kp;
  std::memcpy(kp.seed.data(), seed.data(), 32);

  Sha512 h;
  h.update(seed.data(), 32);
  auto az = h.finish();
  az[0] &= 248;
  az[31] &= 127;
  az[31] |= 64;

  Point p;
  scalarbase(p, az.data());
  pack_point(kp.public_key.data(), p);
  return kp;
}

Ed25519KeyPair ed25519_generate_keypair() {
  std::array<uint8_t, 32> seed{};
  std::random_device rd;
  for (std::size_t i = 0; i < 32; i += 4) {
    uint32_t v = rd();
    seed[i + 0] = static_cast<uint8_t>(v & 0xFFU);
    seed[i + 1] = static_cast<uint8_t>((v >> 8) & 0xFFU);
    seed[i + 2] = static_cast<uint8_t>((v >> 16) & 0xFFU);
    seed[i + 3] = static_cast<uint8_t>((v >> 24) & 0xFFU);
  }
  return ed25519_keypair_from_seed(seed);
}

std::array<uint8_t, 64> ed25519_sign(std::span<const uint8_t, 32> seed,
                                     std::span<const uint8_t, 32> public_key,
                                     std::span<const uint8_t> message) noexcept {
  Sha512 h_sk;
  h_sk.update(seed.data(), 32);
  auto az = h_sk.finish();
  az[0] &= 248;
  az[31] &= 127;
  az[31] |= 64;

  Sha512 h_r;
  h_r.update(az.data() + 32, 32);
  h_r.update(message.data(), message.size());
  auto r_hash = h_r.finish();
  reduce(r_hash.data());

  Point R;
  scalarbase(R, r_hash.data());
  std::array<uint8_t, 64> sig{};
  pack_point(sig.data(), R);

  Sha512 h_k;
  h_k.update(sig.data(), 32);
  h_k.update(public_key.data(), 32);
  h_k.update(message.data(), message.size());
  auto k_hash = h_k.finish();
  reduce(k_hash.data());

  int64_t x[64] = {0};
  for (int i = 0; i < 32; ++i) x[i] = static_cast<int64_t>(r_hash[i]);
  for (int i = 0; i < 32; ++i) {
    for (int j = 0; j < 32; ++j) {
      x[i + j] += static_cast<int64_t>(k_hash[i]) * static_cast<int64_t>(az[j]);
    }
  }
  modL(sig.data() + 32, x);
  return sig;
}

bool ed25519_verify(std::span<const uint8_t, 32> public_key, std::span<const uint8_t> message,
                    std::span<const uint8_t, 64> signature) noexcept {
  if (!scalar_is_canonical(signature.data() + 32)) return false;

  // Wycheproof / RFC 8032 canonical y < p = 2^255 - 19 check for both A and R.
  auto y_is_canonical = [](const uint8_t pt[32]) noexcept -> bool {
    if ((pt[31] & 0x7f) < 0x7f) return true;
    for (int i = 30; i >= 1; --i) {
      if (pt[i] < 0xff) return true;
    }
    return pt[0] < 0xed;
  };
  if (!y_is_canonical(public_key.data()) || !y_is_canonical(signature.data())) return false;

  // Reject all 8 Ed25519 small-order torsion points for A and R (fail-closed).
  auto is_small_order = [](const uint8_t pt[32]) noexcept -> bool {
    static constexpr std::array<std::array<uint8_t, 32>, 8> kSmallOrder = {{
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        {0xec, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
         0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0x7f},
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
         0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80},
        {0xc7, 0x17, 0x6a, 0x70, 0x3d, 0x4d, 0xd8, 0x4f, 0xba, 0x3c, 0x0b,
         0x76, 0x0d, 0x10, 0x67, 0x0f, 0x2a, 0x20, 0x53, 0xfa, 0x2c, 0x39,
         0xcc, 0xc6, 0x4e, 0xc7, 0xfd, 0x77, 0x92, 0xac, 0x03, 0x7a},
        {0xc7, 0x17, 0x6a, 0x70, 0x3d, 0x4d, 0xd8, 0x4f, 0xba, 0x3c, 0x0b,
         0x76, 0x0d, 0x10, 0x67, 0x0f, 0x2a, 0x20, 0x53, 0xfa, 0x2c, 0x39,
         0xcc, 0xc6, 0x4e, 0xc7, 0xfd, 0x77, 0x92, 0xac, 0x03, 0xfa},
        {0x26, 0xe8, 0x95, 0x8f, 0xc2, 0xb2, 0x27, 0xb0, 0x45, 0xc3, 0xf4,
         0x89, 0xf2, 0xef, 0x98, 0xf0, 0xd5, 0xdf, 0xac, 0x05, 0xd3, 0xc6,
         0x33, 0x39, 0xb1, 0x38, 0x02, 0x88, 0x6d, 0x53, 0xfc, 0x05},
        {0x26, 0xe8, 0x95, 0x8f, 0xc2, 0xb2, 0x27, 0xb0, 0x45, 0xc3, 0xf4,
         0x89, 0xf2, 0xef, 0x98, 0xf0, 0xd5, 0xdf, 0xac, 0x05, 0xd3, 0xc6,
         0x33, 0x39, 0xb1, 0x38, 0x02, 0x88, 0x6d, 0x53, 0xfc, 0x85},
    }};
    for (const auto& bad : kSmallOrder) {
      if (std::memcmp(pt, bad.data(), 32) == 0) return true;
    }
    return false;
  };
  if (is_small_order(public_key.data()) || is_small_order(signature.data())) return false;

  Point q;
  if (!unpackneg(q, public_key.data())) return false;

  Sha512 h_k;
  h_k.update(signature.data(), 32);
  h_k.update(public_key.data(), 32);
  h_k.update(message.data(), message.size());
  auto k_hash = h_k.finish();
  reduce(k_hash.data());

  // 2-bit windowed Shamir's simultaneous double-scalar mul: s*B + k*Q (public inputs only).
  static const std::array<Point, 4> b_tab = []() {
    Point B{kX, kY, kGf1, {}};
    M(B.t, kX, kY);
    std::array<Point, 4> bt{};
    bt[0] = {kGf0, kGf1, kGf1, kGf0};
    bt[1] = B;
    bt[2] = B;
    dbl_point(bt[2]);
    bt[3] = bt[2];
    add_point(bt[3], B);
    return bt;
  }();

  Point q_tab[4];
  q_tab[0] = {kGf0, kGf1, kGf1, kGf0};
  q_tab[1] = q;
  q_tab[2] = q;
  dbl_point(q_tab[2]);
  q_tab[3] = q_tab[2];
  add_point(q_tab[3], q);

  Point tbl[16];
  for (int bi = 0; bi < 4; ++bi) {
    for (int qi = 0; qi < 4; ++qi) {
      if (bi == 0) {
        tbl[bi * 4 + qi] = q_tab[qi];
      } else if (qi == 0) {
        tbl[bi * 4 + qi] = b_tab[bi];
      } else {
        tbl[bi * 4 + qi] = b_tab[bi];
        add_point(tbl[bi * 4 + qi], q_tab[qi]);
      }
    }
  }

  Point p{kGf0, kGf1, kGf1, kGf0};
  bool started = false;
  for (int i = 254; i >= 0; i -= 2) {
    if (started) {
      dbl_point(p);
      dbl_point(p);
    }
    uint8_t sb = static_cast<uint8_t>((signature[32 + i / 8] >> (i & 7)) & 3);
    uint8_t kb = static_cast<uint8_t>((k_hash[i / 8] >> (i & 7)) & 3);
    uint8_t sel = static_cast<uint8_t>((sb << 2) | kb);
    if (sel != 0) {
      if (!started) {
        p = tbl[sel];
        started = true;
      } else {
        add_point(p, tbl[sel]);
      }
    }
  }

  uint8_t check_r[32];
  pack_point(check_r, p);

  uint32_t diff = 0;
  for (int i = 0; i < 32; ++i) diff |= static_cast<uint32_t>(check_r[i] ^ signature[i]);
  return diff == 0;
}

std::string base64_encode(std::span<const uint8_t> data) {
  static constexpr char kTable[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  std::string out;
  out.reserve(((data.size() + 2) / 3) * 4);
  std::size_t i = 0;
  while (i + 3 <= data.size()) {
    uint32_t v = (uint32_t(data[i]) << 16) | (uint32_t(data[i + 1]) << 8) | uint32_t(data[i + 2]);
    out.push_back(kTable[(v >> 18) & 0x3FU]);
    out.push_back(kTable[(v >> 12) & 0x3FU]);
    out.push_back(kTable[(v >> 6) & 0x3FU]);
    out.push_back(kTable[v & 0x3FU]);
    i += 3;
  }
  const std::size_t rem = data.size() - i;
  if (rem == 1) {
    uint32_t v = uint32_t(data[i]) << 16;
    out.push_back(kTable[(v >> 18) & 0x3FU]);
    out.push_back(kTable[(v >> 12) & 0x3FU]);
    out.push_back('=');
    out.push_back('=');
  } else if (rem == 2) {
    uint32_t v = (uint32_t(data[i]) << 16) | (uint32_t(data[i + 1]) << 8);
    out.push_back(kTable[(v >> 18) & 0x3FU]);
    out.push_back(kTable[(v >> 12) & 0x3FU]);
    out.push_back(kTable[(v >> 6) & 0x3FU]);
    out.push_back('=');
  }
  return out;
}

std::optional<std::vector<uint8_t>> base64_decode(std::string_view b64) {
  if (b64.empty() || (b64.size() % 4) != 0) return std::nullopt;
  auto val = [](char c) -> int {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return -1;
  };
  std::vector<uint8_t> out;
  out.reserve((b64.size() / 4) * 3);
  for (std::size_t i = 0; i < b64.size(); i += 4) {
    int a = val(b64[i]);
    int b = val(b64[i + 1]);
    bool pad2 = (b64[i + 2] == '=');
    bool pad3 = (b64[i + 3] == '=');
    if (a < 0 || b < 0) return std::nullopt;
    if (pad2 && !pad3) return std::nullopt;
    if ((pad2 || pad3) && (i + 4 != b64.size())) return std::nullopt;
    int c = pad2 ? 0 : val(b64[i + 2]);
    int d = pad3 ? 0 : val(b64[i + 3]);
    if (c < 0 || d < 0) return std::nullopt;
    uint32_t v = (uint32_t(a) << 18) | (uint32_t(b) << 12) | (uint32_t(c) << 6) | uint32_t(d);
    out.push_back(static_cast<uint8_t>((v >> 16) & 0xFFU));
    if (!pad2) out.push_back(static_cast<uint8_t>((v >> 8) & 0xFFU));
    if (!pad3) out.push_back(static_cast<uint8_t>(v & 0xFFU));
  }
  return out;
}

std::string export_private_key_pem(std::span<const uint8_t, 32> seed) {
  std::array<uint8_t, 48> der{};
  std::memcpy(der.data(), kPkcs8Prefix.data(), 16);
  std::memcpy(der.data() + 16, seed.data(), 32);
  return "-----BEGIN PRIVATE KEY-----\n" + base64_encode(der) + "\n-----END PRIVATE KEY-----\n";
}

std::string export_public_key_pem(std::span<const uint8_t, 32> public_key) {
  std::array<uint8_t, 44> der{};
  std::memcpy(der.data(), kSpkiPrefix.data(), 12);
  std::memcpy(der.data() + 12, public_key.data(), 32);
  return "-----BEGIN PUBLIC KEY-----\n" + base64_encode(der) + "\n-----END PUBLIC KEY-----\n";
}

std::optional<std::array<uint8_t, 32>> import_private_key_pem(std::string_view pem) {
  auto body = strip_pem(pem, "-----BEGIN PRIVATE KEY-----", "-----END PRIVATE KEY-----");
  auto der = base64_decode(body);
  if (!der.has_value() || der->size() != 48) return std::nullopt;
  if (std::memcmp(der->data(), kPkcs8Prefix.data(), 16) != 0) return std::nullopt;
  std::array<uint8_t, 32> seed{};
  std::memcpy(seed.data(), der->data() + 16, 32);
  return seed;
}

std::optional<std::array<uint8_t, 32>> import_public_key_pem(std::string_view pem) {
  auto body = strip_pem(pem, "-----BEGIN PUBLIC KEY-----", "-----END PUBLIC KEY-----");
  auto der = base64_decode(body);
  if (!der.has_value() || der->size() != 44) return std::nullopt;
  if (std::memcmp(der->data(), kSpkiPrefix.data(), 12) != 0) return std::nullopt;
  std::array<uint8_t, 32> pub{};
  std::memcpy(pub.data(), der->data() + 12, 32);
  return pub;
}

std::string rfc00d4_canonical_signing_payload(std::string_view meaning_json,
                                              std::string_view evidence_json,
                                              std::string_view authority_json) {
  std::string out;
  out.reserve(meaning_json.size() + evidence_json.size() + authority_json.size() + 48);
  out += "{\"authority\":";
  out += authority_json;
  out += ",\"evidence\":";
  out += evidence_json;
  out += ",\"meaning\":";
  out += meaning_json;
  out += "}";
  return out;
}

std::string sign_rfc00d4_payload_b64(std::span<const uint8_t, 32> seed,
                                     std::span<const uint8_t, 32> public_key,
                                     std::string_view canonical_payload) {
  auto sig = ed25519_sign(
      seed, public_key,
      std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(canonical_payload.data()),
                               canonical_payload.size()));
  return base64_encode(sig);
}

bool verify_rfc00d4_payload_b64(std::span<const uint8_t, 32> public_key,
                                std::string_view canonical_payload,
                                std::string_view signature_b64) {
  auto sig_vec = base64_decode(signature_b64);
  if (!sig_vec.has_value() || sig_vec->size() != 64) return false;
  std::array<uint8_t, 64> sig{};
  std::memcpy(sig.data(), sig_vec->data(), 64);
  return ed25519_verify(
      public_key,
      std::span<const uint8_t>(reinterpret_cast<const uint8_t*>(canonical_payload.data()),
                               canonical_payload.size()),
      sig);
}

DagVerificationResult verify_dag(const std::vector<AccountableBundleNode>& nodes) {
  DagVerificationResult res;
  if (nodes.empty()) {
    res.code = "ERR_DAG_EMPTY";
    res.error = "receipt DAG contains zero nodes";
    return res;
  }

  std::unordered_map<std::string, std::size_t> id_to_idx;
  id_to_idx.reserve(nodes.size());
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    if (nodes[i].bundle_id.empty()) {
      res.code = "ERR_DAG_INVALID_ID";
      res.error = "empty bundle_id in receipt DAG";
      return res;
    }
    if (!id_to_idx.emplace(nodes[i].bundle_id, i).second) {
      res.code = "ERR_DAG_DUPLICATE_ID";
      res.error = "duplicate bundle_id: " + nodes[i].bundle_id;
      return res;
    }
  }

  // Verify every parent reference exists in the DAG.
  for (const auto& n : nodes) {
    for (const auto& pid : n.parent_bundle_ids) {
      if (id_to_idx.find(pid) == id_to_idx.end()) {
        res.code = "ERR_DAG_MISSING_PARENT";
        res.error = "bundle " + n.bundle_id + " references missing parent " + pid;
        return res;
      }
    }
  }

  // 3-color DFS cycle detection (0=unvisited, 1=visiting, 2=visited).
  std::vector<uint8_t> color(nodes.size(), 0);
  auto has_cycle = [&](auto&& self, std::size_t u) -> bool {
    color[u] = 1;
    for (const auto& pid : nodes[u].parent_bundle_ids) {
      std::size_t v = id_to_idx.at(pid);
      if (color[v] == 1) return true;
      if (color[v] == 0 && self(self, v)) return true;
    }
    color[u] = 2;
    return false;
  };
  for (std::size_t i = 0; i < nodes.size(); ++i) {
    if (color[i] == 0 && has_cycle(has_cycle, i)) {
      res.code = "ERR_DAG_CYCLE";
      res.error = "cycle detected in receipt DAG";
      return res;
    }
  }

  // Verify Ed25519 signature on every node (fail-closed).
  for (const auto& n : nodes) {
    if (!verify_rfc00d4_payload_b64(n.public_key, n.canonical_signing_payload, n.signature_b64)) {
      res.code = "ERR_RECEIPT_VERIFY_FAILED";
      res.error = "Ed25519 signature verification failed for bundle " + n.bundle_id;
      return res;
    }
    ++res.verified_nodes;
  }

  res.ok = true;
  res.code = "OK";
  return res;
}

}  // namespace t81::crypto
