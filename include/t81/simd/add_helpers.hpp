#pragma once

#include <array>
#include <cstdint>
#include <type_traits>

namespace t81::simd {

struct AddEntry {
  int8_t sum[3];    // result for carry-in -1/0/+1 (index 0/1/2)
  int8_t carry[3];  // carry-out given the corresponding carry-in
};

constexpr int8_t Normalize(int value) {
  if (value > 1) return static_cast<int8_t>(value - 3);
  if (value < -1) return static_cast<int8_t>(value + 3);
  return static_cast<int8_t>(value);
}

constexpr int8_t CarryOut(int value) {
  if (value > 1) return 1;
  if (value < -1) return -1;
  return 0;
}

constexpr AddEntry BuildAddEntry(int8_t a, int8_t b) {
  AddEntry entry{};
  for (int idx = 0; idx < 3; ++idx) {
    const int8_t carry_in = static_cast<int8_t>(idx - 1);
    const int sum = static_cast<int>(a) + static_cast<int>(b) + carry_in;
    entry.sum[idx] = Normalize(sum);
    entry.carry[idx] = CarryOut(sum);
  }
  return entry;
}

constexpr std::array<AddEntry, 27 * 27> BuildAddTable() {
  std::array<AddEntry, 27 * 27> table{};
  for (int a = -1; a <= 1; ++a) {
    for (int b = -1; b <= 1; ++b) {
      const int index = (a + 1) * 3 + (b + 1);
      table[index] = BuildAddEntry(static_cast<int8_t>(a), static_cast<int8_t>(b));
    }
  }
  return table;
}

constexpr auto kAddTable = BuildAddTable();

constexpr const AddEntry& LookupAddEntry(int8_t lhs, int8_t rhs) {
  const int idx = (lhs + 1) * 3 + (rhs + 1);
  return kAddTable[idx];
}

}  // namespace t81::simd
