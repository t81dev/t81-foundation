// RFC-0038 — Ternary Lattice Cryptography Primitives
// Negacyclic polynomial arithmetic over {-1, 0, +1} coefficients in Z[x]/(x^n + 1).
// No multiplications required — only add/sub/trit-flip.
// POLYMUL and POLYMOD are the two TISC opcodes added by this RFC.
#pragma once

#include <cstdint>
#include <stdexcept>
#include <vector>

#include "t81/tensor.hpp"
#include "t81/types/T81BigInt.hpp"

namespace t81::ops {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------
namespace lattice_detail {

// Ternary multiply: {-1,0,+1} × {-1,0,+1} → {-1,0,+1}
// Rule: 0 if either operand is 0; same sign → +1; opposite sign → -1.
inline int trit_mul(int a, int b) {
  if (a == 0 || b == 0) return 0;
  return (a == b) ? 1 : -1;
}

// Snap a float to the nearest trit in {-1, 0, +1}.
inline int snap_trit(float v, float threshold = 0.5f) {
  if (v >  threshold) return  1;
  if (v < -threshold) return -1;
  return 0;
}

}  // namespace lattice_detail

// ---------------------------------------------------------------------------
// POLYMUL — Negacyclic polynomial multiply (§RFC-0038 §5.1)
//
//   Computes C = A · B in Z[x]/(x^n + 1), where A and B are 1-D tensors of
//   length n with values in {-1, 0, +1}.
//
//   Negacyclic convolution formula:
//     C[k] = Σ_{i=0}^{n-1}  A[i] · B[(k−i+n) mod n] · neg(i, k)
//   where neg(i, k) = −1 if i > k else +1   (wraps past the modulus x^n + 1
//   pick up a sign flip per the ring relation x^n ≡ −1).
//
//   A[i] · B[j] is the ternary product (lattice_detail::trit_mul) — zero
//   multiplications in the usual sense; only add/sub/trit-flip.
//
//   Uses T81BigInt accumulators for bit-exact results.
// ---------------------------------------------------------------------------
[[nodiscard]] inline T729DynamicTensor polymul(const T729DynamicTensor& a,
                                               const T729DynamicTensor& b) {
  const auto& ash = a.shape();
  const auto& bsh = b.shape();

  if (ash.size() != 1 || bsh.size() != 1) {
    throw std::invalid_argument("polymul: both operands must be 1-D tensors");
  }
  const int n = static_cast<int>(ash[0]);
  if (n != static_cast<int>(bsh[0])) {
    throw std::invalid_argument("polymul: both polynomials must have the same degree");
  }
  if (n == 0) {
    throw std::invalid_argument("polymul: polynomial length must be > 0");
  }

  const auto av = a.snapshot_values();
  const auto bv = b.snapshot_values();

  // BigInt accumulators for the n output coefficients.
  std::vector<t81::v1::T81BigInt> acc(static_cast<std::size_t>(n));

  for (int i = 0; i < n; ++i) {
    const int ai = lattice_detail::snap_trit(av[static_cast<std::size_t>(i)]);
    if (ai == 0) continue;
    for (int j = 0; j < n; ++j) {
      const int bj = lattice_detail::snap_trit(bv[static_cast<std::size_t>(j)]);
      if (bj == 0) continue;
      const int k = (i + j) % n;
      // Negacyclic sign: wraps carry a −1 factor (x^n ≡ −1 in Z[x]/(x^n+1)).
      const int sign = (i + j >= n) ? -1 : 1;
      const int coeff = lattice_detail::trit_mul(ai, bj) * sign;
      if (coeff == 0) continue;
      // Always add the signed delta — T81BigInt supports negative values.
      acc[static_cast<std::size_t>(k)] =
          acc[static_cast<std::size_t>(k)] +
          t81::v1::T81BigInt(static_cast<std::int64_t>(coeff));
    }
  }

  std::vector<float> out;
  out.reserve(static_cast<std::size_t>(n));
  for (const auto& bi : acc) {
    out.push_back(static_cast<float>(bi.to_int64()));
  }
  return T729DynamicTensor({n}, std::move(out));
}

// ---------------------------------------------------------------------------
// POLYMOD — Centered coefficient reduction mod q (§RFC-0038 §5.2)
//
//   Maps every coefficient c of polynomial A into the centered range
//   (−q/2, q/2] via:
//     c_red = ((c % q) + q) % q
//     if c_red > (q − 1) / 2  →  c_red −= q
//
//   q must be a positive odd integer (lattice crypto convention).
//   Input may be any 1-D tensor; output is a 1-D tensor of the same length.
// ---------------------------------------------------------------------------
[[nodiscard]] inline T729DynamicTensor polymod(const T729DynamicTensor& a, std::int64_t q) {
  if (q <= 0) {
    throw std::invalid_argument("polymod: modulus q must be > 0");
  }

  const auto& ash = a.shape();
  if (ash.size() != 1) {
    throw std::invalid_argument("polymod: operand must be a 1-D tensor");
  }

  const auto av = a.snapshot_values();
  std::vector<float> out;
  out.reserve(av.size());

  const std::int64_t half = (q - 1) / 2;
  for (float fv : av) {
    std::int64_t c = static_cast<std::int64_t>(fv);
    std::int64_t r = ((c % q) + q) % q;
    if (r > half) r -= q;
    out.push_back(static_cast<float>(r));
  }
  return T729DynamicTensor(ash, std::move(out));
}

}  // namespace t81::ops
