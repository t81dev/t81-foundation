/**
 * @file T81Symbolic.hpp
 * @brief Advanced Symbolic Algebra for T81.
 *
 * Implements symbolic expressions, differentiation, and simplification.
 */
#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <variant>
#include "t81/types/T81Float.hpp"
#include "t81/types/T81Symbol.hpp"

namespace t81 {

// Forward declaration
struct Expr;

// Pointer to immutable expression
using ExprPtr = std::shared_ptr<const Expr>;

// Operation types
enum class Op { Add, Sub, Mul, Div, Pow, Neg, Sin, Cos, Exp, Log };

// Node variants
struct BinaryOp {
  Op op;
  ExprPtr lhs;
  ExprPtr rhs;
};
struct UnaryOp {
  Op op;
  ExprPtr operand;
};
struct Constant {
  T81Float<72, 9> val;
};
struct Variable {
  T81Symbol sym;
};

using ExprNode = std::variant<Constant, Variable, UnaryOp, BinaryOp>;

struct Expr : std::enable_shared_from_this<Expr> {
  ExprNode node;

  Expr(ExprNode n) : node(std::move(n)) {}

  // Helpers to create expressions
  static ExprPtr make_const(T81Float<72, 9> v) { return std::make_shared<Expr>(Constant{v}); }

  static ExprPtr make_var(T81Symbol s) { return std::make_shared<Expr>(Variable{s}); }

  static ExprPtr make_binary(Op op, ExprPtr lhs, ExprPtr rhs) {
    return std::make_shared<Expr>(BinaryOp{op, lhs, rhs});
  }

  static ExprPtr make_unary(Op op, ExprPtr operand) {
    return std::make_shared<Expr>(UnaryOp{op, operand});
  }
};

// Operator overloads for convenient syntax
inline ExprPtr operator+(ExprPtr a, ExprPtr b) { return Expr::make_binary(Op::Add, a, b); }
inline ExprPtr operator-(ExprPtr a, ExprPtr b) { return Expr::make_binary(Op::Sub, a, b); }
inline ExprPtr operator*(ExprPtr a, ExprPtr b) { return Expr::make_binary(Op::Mul, a, b); }
inline ExprPtr operator/(ExprPtr a, ExprPtr b) { return Expr::make_binary(Op::Div, a, b); }
inline ExprPtr pow(ExprPtr a, ExprPtr b) { return Expr::make_binary(Op::Pow, a, b); }
inline ExprPtr operator-(ExprPtr a) { return Expr::make_unary(Op::Neg, a); }
inline ExprPtr sin(ExprPtr a) { return Expr::make_unary(Op::Sin, a); }
inline ExprPtr cos(ExprPtr a) { return Expr::make_unary(Op::Cos, a); }
inline ExprPtr exp(ExprPtr a) { return Expr::make_unary(Op::Exp, a); }
inline ExprPtr log(ExprPtr a) { return Expr::make_unary(Op::Log, a); }

// Differentiation
ExprPtr diff(ExprPtr e, T81Symbol var);

// Simplification
ExprPtr simplify(ExprPtr e);

// Implementation details for diff
inline ExprPtr diff(ExprPtr e, T81Symbol var) {
  struct DiffVisitor {
    T81Symbol var;

    ExprPtr operator()(const Constant&) { return Expr::make_const(T81Float<72, 9>(0)); }

    ExprPtr operator()(const Variable& v) {
      if (v.sym == var) return Expr::make_const(T81Float<72, 9>(1));
      return Expr::make_const(T81Float<72, 9>(0));
    }

    ExprPtr operator()(const UnaryOp& u) {
      ExprPtr d_inner = diff(u.operand, var);
      switch (u.op) {
        case Op::Neg:
          return -d_inner;
        case Op::Sin:
          return cos(u.operand) * d_inner;
        case Op::Cos:
          return -sin(u.operand) * d_inner;
        case Op::Exp:
          return exp(u.operand) * d_inner;
        case Op::Log:
          return d_inner / u.operand;
        default:
          return Expr::make_const(T81Float<72, 9>(0));  // Should not happen
      }
    }

    ExprPtr operator()(const BinaryOp& b) {
      ExprPtr dl = diff(b.lhs, var);
      ExprPtr dr = diff(b.rhs, var);

      switch (b.op) {
        case Op::Add:
          return dl + dr;
        case Op::Sub:
          return dl - dr;
        case Op::Mul:
          return dl * b.rhs + b.lhs * dr;
        case Op::Div:
          // (u/v)' = (u'v - uv') / v^2
          return (dl * b.rhs - b.lhs * dr) / pow(b.rhs, Expr::make_const(T81Float<72, 9>(2)));
        case Op::Pow:
          // (u^v)' = v * u^(v-1) * u' + u^v * ln(u) * v'
          // Simplifying assumption: v is constant for now for standard poly diff
          // If v is constant: v * u^(v-1) * u'
          if (std::holds_alternative<Constant>(b.rhs->node)) {
            return b.rhs * pow(b.lhs, b.rhs - Expr::make_const(T81Float<72, 9>(1))) * dl;
          }
          // Full chain rule: u^v * (v' ln(u) + v u'/u)
          return pow(b.lhs, b.rhs) * (dr * log(b.lhs) + b.rhs * dl / b.lhs);
        default:
          return Expr::make_const(T81Float<72, 9>(0));
      }
    }
  };

  return std::visit(DiffVisitor{var}, e->node);
}

// Basic Simplification
inline ExprPtr simplify(ExprPtr e) {
  struct SimpVisitor {
    ExprPtr operator()(const Constant& c) { return Expr::make_const(c.val); }
    ExprPtr operator()(const Variable& v) { return Expr::make_var(v.sym); }

    ExprPtr operator()(const UnaryOp& u) {
      ExprPtr s = simplify(u.operand);
      // Constant folding
      if (std::holds_alternative<Constant>(s->node)) {
        T81Float<72, 9> val = std::get<Constant>(s->node).val;
        if (u.op == Op::Neg) return Expr::make_const(-val);
        // Other ops would require T81Float math functions which might not be fully exposed or
        // constexpr
      }
      return Expr::make_unary(u.op, s);
    }

    ExprPtr operator()(const BinaryOp& b) {
      ExprPtr l = simplify(b.lhs);
      ExprPtr r = simplify(b.rhs);

      // Constant folding
      if (std::holds_alternative<Constant>(l->node) && std::holds_alternative<Constant>(r->node)) {
        T81Float<72, 9> lv = std::get<Constant>(l->node).val;
        T81Float<72, 9> rv = std::get<Constant>(r->node).val;

        switch (b.op) {
          case Op::Add:
            return Expr::make_const(lv + rv);
          case Op::Sub:
            return Expr::make_const(lv - rv);
          case Op::Mul:
            return Expr::make_const(lv * rv);
          case Op::Div:
            return Expr::make_const(lv / rv);
          // Pow...
          default:
            break;
        }
      }

      // Identity rules
      bool l_is_0 = false, l_is_1 = false;
      bool r_is_0 = false, r_is_1 = false;

      if (auto* c = std::get_if<Constant>(&l->node)) {
        if (c->val.is_zero()) l_is_0 = true;
        if (c->val == T81Float<72, 9>(1)) l_is_1 = true;
      }
      if (auto* c = std::get_if<Constant>(&r->node)) {
        if (c->val.is_zero()) r_is_0 = true;
        if (c->val == T81Float<72, 9>(1)) r_is_1 = true;
      }

      if (b.op == Op::Add) {
        if (l_is_0) return r;
        if (r_is_0) return l;
      }
      if (b.op == Op::Mul) {
        if (l_is_0 || r_is_0) return Expr::make_const(T81Float<72, 9>(0));
        if (l_is_1) return r;
        if (r_is_1) return l;
      }

      // Power rules
      if (b.op == Op::Pow) {
        if (r_is_0) return Expr::make_const(T81Float<72, 9>(1));
        if (r_is_1) return l;
      }

      return Expr::make_binary(b.op, l, r);
    }
    [[nodiscard]] std::string serialize_canonical() const { return "Symbolic()"; }
};

  return std::visit(SimpVisitor{}, e->node);
}

}  // namespace t81
