#include "t81/types/fraction.hpp"
#include "t81/fraction.hpp"

namespace t81::core {
std::string Fraction::to_string() const {
  // Preserve legacy non-throw formatting for invalid denominator, while
  // canonicalizing valid values through the v1 fraction implementation.
  if (t81::T81BigInt::is_zero(denominator.canonical())) {
    return numerator.to_string() + "/" + denominator.to_string();
  }
  return t81::T81Fraction(numerator.canonical(), denominator.canonical()).to_string();
}
}  // namespace t81::core
