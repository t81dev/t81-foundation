#pragma once

#include <t81/support/expected.hpp>
#include "t81/lang/ast.hpp"
#include "t81/tisc/program.hpp"

namespace t81::lang {
enum class CompileError {
  None = 0,
  UnsupportedType,
  EmptyModule,
  MissingReturn,
  UndeclaredIdentifier,
  RegisterOverflow,
  MissingType,
  UnknownFunction,
  InvalidCall,
  UnsupportedLiteral,
  InvalidMatch,
};

class Compiler {
 public:
  std::expected<t81::tisc::Program, CompileError> compile(const Module& module) const;
};
}  // namespace t81::lang
