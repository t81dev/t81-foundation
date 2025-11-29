/**
 * @file base81.hpp
 * @brief Defines the Base81String type and validation utilities for Base-81 encoding.
 *
 * This file provides a type alias, `Base81String`, for the Base-81 textual
 * representation used throughout the T81 platform. It also includes a utility
 * function, `is_base81`, for validating whether a given string is a correctly
 * encoded Base-81 string according to the canonical alphabet.
 */
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "t81/codec/base81.hpp"

namespace t81::core {
// Base-81 textual representation used throughout the platform. See spec/t81-data-types.md.
using Base81String = std::string;

inline bool is_base81(std::string_view value) {
  // Decode using the canonical alphabet; failure indicates non-canonical input.
  std::vector<std::uint8_t> sink;
  return t81::codec::base81::decode_bytes(value, sink);
}
}  // namespace t81::core
