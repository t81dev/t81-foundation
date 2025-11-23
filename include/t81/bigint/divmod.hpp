// include/t81/bigint/divmod.hpp
#pragma once
#include <span>
#include "t81/bigint/core.hpp"

namespace t81::bigint {

// Returns quotient, remainder: q = a / b; r = a % b
void divmod(const BigInt& a, const BigInt& b, BigInt& q, BigInt& r);

// Fast path: a / small (fits in 32/64 bits)
uint64_t div_small(const BigInt& a, uint64_t d, BigInt& q);
uint64_t mod_small(const BigInt& a, uint64_t d);

} // namespace t81::bigint
