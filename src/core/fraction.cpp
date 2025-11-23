#include "t81/core/fraction.hpp"

namespace t81::core {
std::string Fraction::to_string() const {
  return numerator.to_string() + "/" + denominator.to_string();
}
}  // namespace t81::core

