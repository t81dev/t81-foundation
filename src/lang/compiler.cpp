#include "t81/lang/compiler.hpp"

#include <vector>

namespace t81::lang {
namespace {
void emit_expr(const Expr& expr, std::vector<t81::tisc::Insn>& out, int target_reg) {
  if (std::holds_alternative<ExprLiteral>(expr.node)) {
    const auto& lit = std::get<ExprLiteral>(expr.node);
    out.push_back({t81::tisc::Opcode::LoadImm, target_reg, static_cast<std::int32_t>(lit.value), 0});
  } else {
    const auto& bin = std::get<ExprBinary>(expr.node);
    int lhs_reg = target_reg;
    int rhs_reg = target_reg + 1;
    emit_expr(*bin.lhs, out, lhs_reg);
    emit_expr(*bin.rhs, out, rhs_reg);
    const auto opcode = bin.op == ExprBinary::Op::Add ? t81::tisc::Opcode::Add : t81::tisc::Opcode::Sub;
    out.push_back({opcode, target_reg, lhs_reg, rhs_reg});
  }
}
}  // namespace

std::expected<t81::tisc::Program, CompileError> Compiler::compile(const Module& module) const {
  if (module.functions.empty()) {
    return CompileError::EmptyModule;
  }
  t81::tisc::Program program;
  for (const auto& fn : module.functions) {
    if (fn.return_type != Type::I64) {
      return CompileError::UnsupportedType;
    }
    for (const auto& stmt : fn.body) {
      emit_expr(stmt.expr, program.insns, 0);
      program.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
    }
  }
  return program;
}
}  // namespace t81::lang

