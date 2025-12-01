#pragma once

#pragma once

#include <vector>

namespace t81::lang {
struct Type {
  enum class Kind {
    T81Int,
    T81Float,
    T81Fraction,
    Symbol,
    Option,
    Result,
    WeightsModel,
    Tensor,
  };

  Kind kind{Kind::T81Int};
  std::vector<Type> params;

  static Type primitive(Kind kind) {
    Type t;
    t.kind = kind;
    return t;
  }

  static Type option(Type inner) {
    Type t;
    t.kind = Kind::Option;
    t.params.push_back(std::move(inner));
    return t;
  }

  static Type result(Type ok, Type err) {
    Type t;
    t.kind = Kind::Result;
    t.params.push_back(std::move(ok));
    t.params.push_back(std::move(err));
    return t;
  }

  static Type weights_model() {
    Type t;
    t.kind = Kind::WeightsModel;
    return t;
  }

  static Type tensor() {
    Type t;
    t.kind = Kind::Tensor;
    return t;
  }
};

inline bool operator==(const Type& lhs, const Type& rhs) {
  if (lhs.kind != rhs.kind) return false;
  if (lhs.params.size() != rhs.params.size()) return false;
  for (std::size_t i = 0; i < lhs.params.size(); ++i) {
    if (!(lhs.params[i] == rhs.params[i])) return false;
  }
  return true;
}

inline bool operator!=(const Type& lhs, const Type& rhs) {
  return !(lhs == rhs);
}
}  // namespace t81::lang
