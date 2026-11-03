#pragma once
#include <cstdint>
#include <vector>
#include "t81/ternary.hpp"
// External definitions, no LTO: outputs escape the timed batch and are fully
// verified afterwards. Allocation in encode_i64 remains part of that API.
void base01_encode_batch(const std::vector<int64_t>& inputs,
                         std::vector<std::vector<t81::Trit>>& outputs);
void base01_decode_batch(const std::vector<std::vector<t81::Trit>>& inputs,
                         std::vector<int64_t>& outputs);
