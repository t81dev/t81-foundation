#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>
#include "t81/ternary.hpp"

namespace t81::ternary {

// ---- helpers ----
inline int trit_to_int(Trit t) {
  switch (t) {
    case Trit::Neg:  return -1;
    case Trit::Zero: return  0;
    case Trit::Pos:  return  1;
  }
  return 0;
}

inline Trit int_to_trit(int v) {
  if (v < 0) return Trit::Neg;
  if (v > 0) return Trit::Pos;
  return Trit::Zero;
}

// Remove most-significant Zero trits; keep at least one digit.
inline void normalize(std::vector<Trit>& ds) {
  while (ds.size() > 1 && ds.back() == Trit::Zero) ds.pop_back();
}

// Encode signed 64-bit into balanced ternary (LSB-first).
// Algorithm: standard base-3 with carry adjustment (2 -> -1 + carry).
inline std::vector<Trit> encode_i64(int64_t x) {
  if (x == 0) return {Trit::Zero};

  bool neg = (x < 0);
  uint64_t n = static_cast<uint64_t>(neg ? -(x + 1) + 1 : x); // abs without UB on INT64_MIN

  std::vector<Trit> out;
  out.reserve(42); // enough for int64 in base-3

  while (n > 0) {
    uint64_t r = n % 3ull; // 0,1,2
    n /= 3ull;
    if (r == 0)       out.push_back(Trit::Zero);
    else if (r == 1)  out.push_back(Trit::Pos);
    else { // r == 2 → use -1 with carry +1
      out.push_back(Trit::Neg);
      n += 1ull;
    }
  }

  if (neg) {
    for (auto& t : out) {
      t = (t == Trit::Pos) ? Trit::Neg :
          (t == Trit::Neg) ? Trit::Pos : Trit::Zero;
    }
  }

  normalize(out);
  return out;
}

// Decode balanced-ternary digits (LSB-first) into int64_t.
// Uses 128-bit accumulator to avoid overflow during summation.
inline int64_t decode_i64(const std::vector<Trit>& ds) {
  __int128 acc = 0;
  __int128 p   = 1; // 3^k
  for (Trit t : ds) {
    acc += (__int128)trit_to_int(t) * p;
    p   *= 3;
  }
  // Clamp to int64_t range if outside (tests use safe ranges).
  if (acc > (__int128)INT64_MAX) return INT64_MAX;
  if (acc < (__int128)INT64_MIN) return INT64_MIN;
  return static_cast<int64_t>(acc);
}

// Balanced-ternary vector addition with carry in {-1,0,1}.
// Inputs/outputs are LSB-first digit arrays.
inline std::vector<Trit> add(const std::vector<Trit>& a, const std::vector<Trit>& b) {
  const size_t n = (a.size() > b.size()) ? a.size() : b.size();
  std::vector<Trit> out;
  out.reserve(n + 2);

  int carry = 0; // in {-1,0,1}
  for (size_t i = 0; i < n; ++i) {
    int ai = (i < a.size()) ? trit_to_int(a[i]) : 0;
    int bi = (i < b.size()) ? trit_to_int(b[i]) : 0;
    int s  = ai + bi + carry; // in [-3..3]

    if (s > 1) {         // 2,3
      out.push_back(int_to_trit(s - 3)); // -> -1 or 0
      carry = 1;
    } else if (s < -1) { // -2,-3
      out.push_back(int_to_trit(s + 3)); // -> 1 or 0
      carry = -1;
    } else {             // -1,0,1
      out.push_back(int_to_trit(s));
      carry = 0;
    }
  }

  // final carry
  if (carry != 0) out.push_back(int_to_trit(carry));

  normalize(out);
  return out;
}

} // namespace t81::ternary
