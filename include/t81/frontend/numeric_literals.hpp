#pragma once

#include "t81/types/T81BigInt.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

namespace t81::frontend::numeric_literals {

inline std::string strip_t81_suffix(std::string_view literal) {
  std::string value(literal);
  constexpr std::string_view suffix = "t81";
  if (value.size() >= suffix.size() &&
      value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0) {
    value.erase(value.size() - suffix.size());
  }
  return value;
}

inline std::optional<std::string> normalize_decimal_integer_literal_text(std::string_view lexeme,
                                                                          bool strip_t81 = false) {
  std::string raw = strip_t81 ? strip_t81_suffix(lexeme) : std::string(lexeme);
  std::string s;
  s.reserve(raw.size());
  for (char c : raw) {
    if (c == '_') continue;
    s.push_back(c);
  }
  if (s.empty()) return std::nullopt;

  bool neg = false;
  std::size_t pos = 0;
  if (s[pos] == '+' || s[pos] == '-') {
    neg = (s[pos] == '-');
    ++pos;
    if (pos >= s.size()) return std::nullopt;
  }

  for (std::size_t i = pos; i < s.size(); ++i) {
    if (s[i] < '0' || s[i] > '9') return std::nullopt;
  }

  while (pos + 1 < s.size() && s[pos] == '0') {
    ++pos;
  }

  std::string out;
  if (neg && s[pos] != '0') {
    out.push_back('-');
  }
  out.append(s.begin() + static_cast<std::ptrdiff_t>(pos), s.end());
  return out;
}

inline std::int64_t parse_t81_integer_literal(std::string_view literal) {
  auto normalized = normalize_decimal_integer_literal_text(literal, true);
  if (!normalized.has_value()) {
    throw std::runtime_error("Invalid t81 integer literal.");
  }
  try {
    return std::stoll(*normalized);
  } catch (const std::invalid_argument& e) {
    throw std::runtime_error("Invalid t81 integer literal '" + *normalized + "': " + e.what());
  } catch (const std::out_of_range&) {
    throw std::out_of_range("t81 integer literal '" + *normalized + "' exceeds 64-bit range");
  }
}

inline t81::v1::T81BigInt parse_t81_bigint_literal(std::string_view literal) {
  auto normalized = normalize_decimal_integer_literal_text(literal, true);
  if (!normalized.has_value()) {
    throw std::invalid_argument("t81 integer literals must use decimal digits followed by 't81'");
  }
  return t81::v1::T81BigInt::from_decimal_string(*normalized);
}

}  // namespace t81::frontend::numeric_literals
