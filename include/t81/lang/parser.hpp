#pragma once

#include <string_view>
#include <t81/lang/ast.hpp>
#include <t81/support/expected.hpp>

namespace t81::lang {
enum class ParseError {
  None = 0,
  UnexpectedToken,
  InvalidLiteral,
  Unterminated,
  Empty,
  MissingFunction,
  MissingType,
  InvalidType,
  MissingReturn,
  UndeclaredIdentifier,
  RegisterOverflow,
};

// Parse a minimal module with a single function:
// supports let, return, if/else, literals/add/sub/mul, parentheses.
t81::expected<Module, ParseError> parse_module(std::string_view source);
}  // namespace t81::lang
