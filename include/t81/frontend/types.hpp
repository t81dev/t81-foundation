#pragma once
// types.hpp — T81Lang type-system representation.
//
// Kept in its own header so that ast.hpp (and any other low-level header)
// can include it without pulling in the full SemanticAnalyzer class.
//
// Rule: this file must NOT include semantic_analyzer.hpp or ir_generator.hpp.

#include <string>
#include <vector>

namespace t81::frontend {

struct Type {
  enum class Kind {
    Void,
    Bool,
    I2,
    I8,
    I16,
    I32,
    BigInt,
    Float,
    Fraction,
    Fixed,
    Complex,
    Quaternion,
    Prob,
    Cell,
    Qutrit,
    Uint,
    Vector,
    Matrix,
    Tensor,
    Graph,
    List,
    Map,
    Set,
    Tree,
    Symbol,
    InfiniteCanonicalForm,
    Option,
    Result,
    String,
    Bytes,
    Constant,
    Custom,
    Unknown,
    Error
  };

  Kind kind = Kind::Unknown;
  std::vector<Type> params;
  std::string custom_name;

  explicit Type(Kind kind_ = Kind::Unknown, std::vector<Type> params_ = {},
                std::string custom_name_ = {})
      : kind(kind_), params(std::move(params_)), custom_name(std::move(custom_name_)) {}

  [[nodiscard]] static Type constant(std::string repr);

  [[nodiscard]] bool operator==(const Type& other) const;
  [[nodiscard]] bool operator!=(const Type& other) const { return !(*this == other); }
};

}  // namespace t81::frontend
