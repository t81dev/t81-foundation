#pragma once
#include "contract.hpp"
namespace arch003 {
std::uint64_t payload_size(std::uint8_t f, std::uint64_t n);
Bytes encode(std::uint8_t f, std::span<const std::int8_t> symbols);
std::int8_t decode_active(const Operand&, std::uint64_t i);
Status admit(const Request&, std::uint8_t registered_format);
}  // namespace arch003
