#pragma once
#include <cstdint>
#include <vector>
#include <stdexcept>
#include "t81/ternary.hpp"

namespace t81::ternary {

// Balanced-ternary encode/decode using Trit {-1,0,+1}.
// Works for signed 64-bit integers. Digits are LSB-first.

// Convert int64_t -> vector<Trit> (LSB-first). 0 -> {Zero}.
inline std::vector<Trit> encode_i64(int64_t v) {
  if (v == 0) return {Trit::Zero};

  std::vector<Trit> out;
  bool neg = v < 0;
  // Work with non-negative magnitude to avoid UB on INT64_MIN
  uint64_t n = neg ? static_cast<uint64_t>(-(v + 1)) + 1ull : static_cast<uint64_t>(v);

  while (n != 0) {
    uint64_t r = n % 3ull; // remainder in {0,1,2}
    n /= 3ull;
    Trit t;
    if (r == 0) {
      t = Trit::Zero;
    } else if (r == 1) {
      t = Trit::Pos;
    } else { // r == 2 -> carry +1 with digit -1
      t = Trit::Neg;
      n += 1ull;
    }
    out.push_back(t);
  }

  // Apply sign: if original was negative, flip digits
  if (neg) {
    for (auto& d : out) {
      d = (d == Trit::Neg ? Trit::Pos : (d == Trit::Pos ? Trit::Neg : Trit::Zero));
    }
  }

  // Trim trailing zeros except keep at least one digit
  while (out.size() > 1 && out.back() == Trit::Zero) out.pop_back();
  return out;
}

// Convert vector<Trit> (LSB-first) -> int64_t. Throws on overflow.
inline int64_t decode_i64(const std::vector<Trit>& digits) {
  __int128 acc = 0;         // use wide accumulator to detect overflow
  __int128 pow3 = 1;

  for (size_t i = 0; i < digits.size(); ++i) {
    int d = 0;
    switch (digits[i]) {
      case Trit::Neg:  d = -1; break;
      case Trit::Zero: d = 0;  break;
      case Trit::Pos:  d = 1;  break;
    }
    acc += (__int128)d * pow3;
    pow3 *= 3;
    // Early overflow check bounds for int64_t
    if (acc > (__int128)INT64_MAX || acc < (__int128)INT64_MIN) {
      throw std::overflow_error("decode_i64: overflow");
    }
  }
  return static_cast<int64_t>(acc);
}

// Normalize: remove highest Zero trits (keep at least one digit).
inline void normalize(std::vector<Trit>& digits) {
  while (digits.size() > 1 && digits.back() == Trit::Zero) digits.pop_back();
}

// Ternary half-adder on Trits (balanced): returns (sum, carry).
// sum,carry in Trit, such that val(sum) + 3*val(carry) = val(a)+val(b).
inline std::pair<Trit, Trit> half_add(Trit a, Trit b) {
  int va = (a == Trit::Neg ? -1 : (a == Trit::Pos ? 1 : 0));
  int vb = (b == Trit::Neg ? -1 : (b == Trit::Pos ? 1 : 0));
  int s = va + vb; // in {-2,-1,0,1,2}
  if (s <= -2) return {Trit::Pos, Trit::Neg}; // +1 with -1 carry (3*(-1)+1 = -2)
  if (s == -1) return {Trit::Neg, Trit::Zero};
  if (s ==  0) return {Trit::Zero, Trit::Zero};
  if (s ==  1) return {Trit::Pos, Trit::Zero};
  /* s == 2 */ return {Trit::Neg, Trit::Pos};  // -1 with +1 carry (3*(+1)-1 = 2)
}

// Vector add for balanced-ternary digits (LSB-first). Pure ternary logic.
inline std::vector<Trit> add(const std::vector<Trit>& A, const std::vector<Trit>& B) {
  size_t n = A.size(), m = B.size();
  size_t L = n > m ? n : m;
  std::vector<Trit> out;
  out.reserve(L + 2);
  Trit carry = Trit::Zero;

  auto add_trit = [](Trit x, Trit y){ return half_add(x, y); };

  for (size_t i = 0; i < L; ++i) {
    Trit a = (i < n ? A[i] : Trit::Zero);
    Trit b = (i < m ? B[i] : Trit::Zero);
    auto [s1, c1] = add_trit(a, b);
    auto [s2, c2] = add_trit(s1, carry);

    // combine carries: c1 + c2 (both in {-1,0,1})
    auto [c_sum, c_carry] = add_trit(c1, c2);
    // c_sum is next carry, c_carry would be at 3^2 place; fold it:
    // carry_out = c_sum + 3*c_carry; since c_carry in {-1,0,1}, we can map:
    if (c_carry == Trit::Pos) {
      // add 3: s2 is one place; promote by adjusting next carry twice:
      // effectively carry_out = c_sum (±1/0) + Pos*3; represent as:
      // we can't store 3 directly; push s2 now, and increase carry next round twice:
      // Simpler: convert s2 to integer and fall back (rare). Use small int path:
      int vs2 = (s2==Trit::Neg?-1:(s2==Trit::Pos?1:0));
      int vc  = (c_sum==Trit::Neg?-1:(c_sum==Trit::Pos?1:0)) + 3;
      // normalize pair (vs2,vc) so that vc in {-1,0,1} by moving multiples of 3 into s2 next digit.
      vs2 -= 3; vc -= 1; // (subtract 3 from sum, reduce carry by 1)
      // map back
      s2 = (vs2==-1?Trit::Neg:vs2==0?Trit::Zero:Trit::Pos);
      c_sum = (vc==-1?Trit::Neg:vc==0?Trit::Zero:Trit::Pos);
    } else if (c_carry == Trit::Neg) {
      int vs2 = (s2==Trit::Neg?-1:(s2==Trit::Pos?1:0));
      int vc  = (c_sum==Trit::Neg?-1:(c_sum==Trit::Pos?1:0)) - 3;
      vs2 += 3; vc += 1;
      s2 = (vs2==-1?Trit::Neg:vs2==0?Trit::Zero:Trit::Pos);
      c_sum = (vc==-1?Trit::Neg:vc==0?Trit::Zero:Trit::Pos);
    }

    out.push_back(s2);
    carry = c_sum;
  }
  if (carry != Trit::Zero) out.push_back(carry);
  normalize(out);
  return out;
}

} // namespace t81::ternary
