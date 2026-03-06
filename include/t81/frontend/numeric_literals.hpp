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

inline std::optional<std::string> normalize_integer_literal_text(std::string_view lexeme,
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

  int base = 10;
  if (pos + 1 < s.size() && s[pos] == '0') {
    char prefix = s[pos + 1];
    if (prefix == 'x' || prefix == 'X') {
      base = 16;
      pos += 2;
    } else if (prefix == 'b' || prefix == 'B') {
      base = 2;
      pos += 2;
    } else if (prefix == 'o' || prefix == 'O') {
      base = 8;
      pos += 2;
    }
  }

  if (pos >= s.size()) {
    if (base == 10) return "0";
    return std::nullopt;
  }

  for (std::size_t i = pos; i < s.size(); ++i) {
    char c = s[i];
    if (base == 10 && (c < '0' || c > '9')) return std::nullopt;
    if (base == 16 && !((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) return std::nullopt;
    if (base == 2 && (c != '0' && c != '1')) return std::nullopt;
    if (base == 8 && (c < '0' || c > '7')) return std::nullopt;
  }

  if (base == 10) {
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

  t81::v1::T81BigInt result = t81::v1::T81BigInt::zero();
  t81::v1::T81BigInt b(base);
  for (std::size_t i = pos; i < s.size(); ++i) {
    char c = s[i];
    int digit = 0;
    if (c >= '0' && c <= '9') digit = c - '0';
    else if (c >= 'a' && c <= 'f') digit = c - 'a' + 10;
    else if (c >= 'A' && c <= 'F') digit = c - 'A' + 10;
    result = result * b + t81::v1::T81BigInt(digit);
  }
  if (neg && !result.is_zero()) {
    result = -result;
  }
  return result.to_decimal_string();
}

inline std::optional<std::string> normalize_decimal_integer_literal_text(std::string_view lexeme,
                                                                          bool strip_t81 = false) {
  return normalize_integer_literal_text(lexeme, strip_t81);
}

inline std::int64_t parse_t81_integer_literal(std::string_view literal) {
  auto normalized = normalize_integer_literal_text(literal, true);
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
  auto normalized = normalize_integer_literal_text(literal, true);
  if (!normalized.has_value()) {
    throw std::invalid_argument("t81 integer literals must use valid digits followed by 't81'");
  }
  return t81::v1::T81BigInt::from_decimal_string(*normalized);
}

}  // namespace t81::frontend::numeric_literals
