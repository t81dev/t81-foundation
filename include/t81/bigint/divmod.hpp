#pragma once
#include <t81/bigint.hpp>

namespace t81 {

/**
 * Compute Euclidean quotient and remainder of a / b.
 *
 * Contract (Euclidean division):
 *   - Precondition: b != 0.
 *   - Postconditions:
 *       a = b * q + r
 *       0 <= r < |b|
 *
 * This differs from C/C++ built-in integer division when a or b is negative:
 * remainder is always non-negative, and quotient is adjusted accordingly.
 */
DivModResult divmod(const T81BigInt& a, const T81BigInt& b);

} // namespace t81
