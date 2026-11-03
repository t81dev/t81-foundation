#pragma once
#include <cstdint>
#include <vector>
#include "t81/ternary/arith.hpp"
using Digits = std::vector<t81::Trit>;
void encode_all(const std::vector<int64_t>& inputs, std::vector<Digits>& outputs);
void decode_all(const std::vector<Digits>& inputs, std::vector<int64_t>& outputs);
