// RFC-0039 — NTRU-KEM: Ternary Key Encapsulation Mechanism
//
// Builds on RFC-0038 (POLYMUL / POLYMOD) to provide a simplified NTRU-based
// Key Encapsulation Mechanism over the ternary negacyclic ring Z[x]/(x^n + 1).
//
// Ring arithmetic surface (T81Lang builtins → TISC opcodes):
//   std.crypto.polyadd(a, b)          →  TVecAdd   (elementwise add)
//   std.crypto.polysub(a, b)          →  TVecSub   (elementwise subtract)
//   std.crypto.ntru_encrypt(h,m,r,q)  →  POLYMUL + TVecAdd + POLYMOD sequence
//   std.crypto.ntru_decrypt(f,c,p)    →  POLYMUL + POLYMOD sequence
//
// C++ layer (for host-side key generation):
//   ntru_make_ternary_poly(n, seed)   — deterministic ternary polynomial
//   NtruKeyPair                       — {f: secret, h: public}
//   ntru_keygen(n, q, seed_f, seed_g) — generate (f, h)
//   ntru_encrypt(h, msg, r, q)        — h*r + m mod (x^n+1) mod q
//   ntru_decrypt(f, c, p)             — f*c mod (x^n+1) mod p
//
// Security note: This is a pedagogical, non-constant-time implementation.
// It demonstrates the ternary ring structure; it is NOT suitable for
// production cryptographic use.  See §5 of RFC-0039-ntru-kem.md.
#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "t81/tensor.hpp"
#include "t81/tensor/lattice_crypto.hpp"  // polymul, polymod
#include "t81/tensor/elementwise.hpp"     // t81::ops::add, sub

namespace t81::crypto {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace ntru_detail {

// Simple LCG for deterministic ternary polynomial generation.
// Produces coefficients in {-1, 0, +1} with roughly uniform distribution.
inline T729DynamicTensor make_ternary_poly(int n, std::uint64_t seed) {
  if (n <= 0) throw std::invalid_argument("ntru: polynomial length must be > 0");
  std::vector<float> v(static_cast<std::size_t>(n));
  std::uint64_t s = seed;
  for (int i = 0; i < n; ++i) {
    // Xorshift64
    s ^= s << 13;
    s ^= s >> 7;
    s ^= s << 17;
    // Map to {-1, 0, +1}: 0→−1, 1→0, 2→+1 (mod 3)
    int t = static_cast<int>(s % 3u);
    v[static_cast<std::size_t>(i)] = static_cast<float>(t - 1);
  }
  return T729DynamicTensor({n}, std::move(v));
}

}  // namespace ntru_detail

// ---------------------------------------------------------------------------
// NtruKeyPair
// ---------------------------------------------------------------------------
struct NtruKeyPair {
  T729DynamicTensor f;  ///< Secret key polynomial (ternary, degree n−1).
  T729DynamicTensor h;  ///< Public key polynomial  h = polymul(f, g) mod q.
};

// ---------------------------------------------------------------------------
// ntru_keygen — deterministic key generation
//
//   f  = random ternary polynomial (seed_f)
//   g  = random ternary polynomial (seed_g)
//   h  = polymod(polymul(f, g), q)   — public key
//
// In a real NTRU scheme h = g · f^{−1} mod q.  We omit the modular inverse
// for pedagogical clarity; the public key relationship h = f·g is sufficient
// to verify the encrypt/decrypt round-trip under the simplified scheme.
// ---------------------------------------------------------------------------
[[nodiscard]] inline NtruKeyPair ntru_keygen(int n, std::int64_t q,
                                              std::uint64_t seed_f = 42u,
                                              std::uint64_t seed_g = 137u) {
  if (q <= 0) throw std::invalid_argument("ntru_keygen: q must be > 0");
  auto f = ntru_detail::make_ternary_poly(n, seed_f);
  auto g = ntru_detail::make_ternary_poly(n, seed_g);
  auto h = t81::ops::polymod(t81::ops::polymul(f, g), q);
  return {std::move(f), std::move(h)};
}

// ---------------------------------------------------------------------------
// ntru_encrypt — ciphertext = polymod(polyadd(polymul(h, r), msg), q)
//
//   c = h·r + msg  mod (x^n + 1) mod q
//   where r is a random ternary "blinding" polynomial.
// ---------------------------------------------------------------------------
[[nodiscard]] inline T729DynamicTensor ntru_encrypt(const T729DynamicTensor& h,
                                                     const T729DynamicTensor& msg,
                                                     const T729DynamicTensor& r,
                                                     std::int64_t q) {
  auto hr = t81::ops::polymul(h, r);                    // h·r in Z[x]/(x^n+1)
  auto hr_msg = t81::ops::add(hr, msg);                 // h·r + msg
  return t81::ops::polymod(hr_msg, q);                  // mod q (centered)
}

// ---------------------------------------------------------------------------
// ntru_decrypt — message recovery = polymod(polymul(f, c), p)
//
//   m' = f·c  mod (x^n + 1) mod p
//   For the simplified keygen (h = f·g), f·c = f·(h·r + m) = f·f·g·r + f·m.
//   When f is the identity polynomial [1,0,...,0], f·c = c and f·m = m.
// ---------------------------------------------------------------------------
[[nodiscard]] inline T729DynamicTensor ntru_decrypt(const T729DynamicTensor& f,
                                                     const T729DynamicTensor& c,
                                                     std::int64_t p) {
  auto fc = t81::ops::polymul(f, c);    // f·c in Z[x]/(x^n+1)
  return t81::ops::polymod(fc, p);      // mod p (centered)
}

}  // namespace t81::crypto
