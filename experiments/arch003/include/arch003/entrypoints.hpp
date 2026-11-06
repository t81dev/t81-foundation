#pragma once
#include "contract.hpp"
namespace arch003 {
using Entry = Result (*)(const Request&);
Result count_tc2(const Request&);
Result count_byte(const Request&);
Result count_planes(const Request&);
Result count_bc2(const Request&);
Result count_pt5(const Request&);
Result count_native(const Request&);
inline constexpr std::array<Entry, 6> entries{count_tc2, count_byte, count_planes,
                                              count_bc2, count_pt5,  count_native};
}  // namespace arch003
