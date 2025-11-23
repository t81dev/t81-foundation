// include/t81/codec/base81.hpp
#pragma once
#include <string>
#include <span>
namespace t81::codec {
std::string base81_encode(std::span<const unsigned char> bytes);
bool base81_decode(const std::string& s, std::vector<unsigned char>& out);
} // namespace
