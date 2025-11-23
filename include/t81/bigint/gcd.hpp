#pragma once
#include <t81/bigint.hpp>

namespace t81 {

/**
 * Greatest common divisor of a and b.
 *
 * Contract:
 *   - gcd(a, b) >= 0   (always non-negative)
 *   - gcd(a, 0) = |a|
 *   - gcd(0, b) = |b|
 *   - gcd(0, 0) = 0
 */
T243BigInt gcd(T243BigInt a, T243BigInt b);

} // namespace t81
