#include "t81/simd/add_helpers.hpp"
#include <cassert>
#include <iostream>

int main() {
  using namespace t81::simd;
  for (int lhs = -1; lhs <= 1; ++lhs) {
    for (int rhs = -1; rhs <= 1; ++rhs) {
      const AddEntry entry = LookupAddEntry(static_cast<int8_t>(lhs), static_cast<int8_t>(rhs));
      for (int carry_idx = 0; carry_idx < 3; ++carry_idx) {
        const int8_t carry_in = static_cast<int8_t>(carry_idx - 1);
        const int value = lhs + rhs + carry_in;
        const int8_t expected_sum =
            (value > 1)   ? static_cast<int8_t>(value - 3)
            : (value < -1) ? static_cast<int8_t>(value + 3)
                          : static_cast<int8_t>(value);
        const int8_t expected_carry = (value > 1)   ? 1
                                       : (value < -1) ? -1
                                                       : 0;
        if (entry.sum[carry_idx] != expected_sum || entry.carry[carry_idx] != expected_carry) {
          std::cout << "Mismatch for " << lhs << ", " << rhs << ", carry " << carry_in << "\n";
          return 1;
        }
      }
    }
  }
  std::cout << "Add helpers table OK\n";
  return 0;
}
