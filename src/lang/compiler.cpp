#include "t81/lang/compiler.hpp"

#include <vector>
#include <unordered_map>
#include <functional>

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
    t81::tisc::Opcode opcode = t81::tisc::Opcode::Add;
    switch (bin.op) {
      case ExprBinary::Op::Add: opcode = t81::tisc::Opcode::Add; break;
      case ExprBinary::Op::Sub: opcode = t81::tisc::Opcode::Sub; break;
      case ExprBinary::Op::Mul: opcode = t81::tisc::Opcode::Mul; break;
    }
    out.push_back({opcode, target_reg, lhs_reg, rhs_reg});
  }
}
}  // namespace

namespace {
bool returns_all(const std::vector<Statement>& stmts) {
  bool saw_return = false;
  for (const auto& s : stmts) {
    if (std::holds_alternative<StatementReturn>(s.node)) {
      saw_return = true;
    } else if (std::holds_alternative<StatementIf>(s.node)) {
      const auto& sif = std::get<StatementIf>(s.node);
      if (returns_all(sif.then_body) && returns_all(sif.else_body)) {
        saw_return = true;
      }
    }
  }
  return saw_return;
}
}  // namespace

std::expected<t81::tisc::Program, CompileError> Compiler::compile(const Module& module) const {
  if (module.functions.empty()) {
    return CompileError::EmptyModule;
  }
  t81::tisc::Program program;
  int next_reg = 1; // R0 reserved for return
  const int kMaxRegs = 26; // R26 reserved
  for (const auto& fn : module.functions) {
    if (fn.return_type != Type::I64) {
      return CompileError::UnsupportedType;
    }
    if (!returns_all(fn.body)) {
      return CompileError::MissingReturn;
    }
    std::vector<std::unordered_map<std::string, int>> scopes;
    scopes.emplace_back();
    auto push_scope = [&]() { scopes.emplace_back(); };
    auto pop_scope = [&]() { if (scopes.size() > 1) scopes.pop_back(); };
    auto declare = [&](const std::string& name, int reg) { scopes.back()[name] = reg; };
    auto lookup = [&](const std::string& name) -> int {
      for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) return f->second;
      }
      return -1;
    };
    auto move_reg = [&](int src, int dst) {
      if (src == dst) return;
      program.insns.push_back({t81::tisc::Opcode::LoadImm, dst, 0, 0});
      program.insns.push_back({t81::tisc::Opcode::Add, dst, dst, src});
    };
    std::function<int(const Expr&, std::optional<int>)> emit_expr_env = [&](const Expr& e, std::optional<int> target) -> int {
      if (std::holds_alternative<ExprLiteral>(e.node)) {
        int reg = target.value_or(next_reg++);
        const auto& lit = std::get<ExprLiteral>(e.node);
        program.insns.push_back({t81::tisc::Opcode::LoadImm, reg, static_cast<std::int32_t>(lit.value), 0});
        return reg;
      }
      if (std::holds_alternative<ExprIdent>(e.node)) {
        const auto& id = std::get<ExprIdent>(e.node);
        int src = lookup(id.name);
        if (src < 0) return -1; // missing symbol
        int dst = target.value_or(src);
        move_reg(src, dst);
        return dst;
      }
      const auto& bin = std::get<ExprBinary>(e.node);
      int lhs_reg = emit_expr_env(*bin.lhs, std::nullopt);
      int rhs_reg = emit_expr_env(*bin.rhs, std::nullopt);
      if (lhs_reg < 0 || rhs_reg < 0) return -1;
      int out_reg = target.value_or(next_reg);
      if (out_reg >= kMaxRegs) return -2;
      if (!target.has_value()) ++next_reg;
      t81::tisc::Opcode opcode = t81::tisc::Opcode::Add;
      switch (bin.op) {
        case ExprBinary::Op::Add: opcode = t81::tisc::Opcode::Add; break;
        case ExprBinary::Op::Sub: opcode = t81::tisc::Opcode::Sub; break;
        case ExprBinary::Op::Mul: opcode = t81::tisc::Opcode::Mul; break;
      }
      program.insns.push_back({opcode, out_reg, lhs_reg, rhs_reg});
      return out_reg;
    };

    for (const auto& stmt : fn.body) {
      if (std::holds_alternative<StatementReturn>(stmt.node)) {
        const auto& sr = std::get<StatementReturn>(stmt.node);
        int reg = emit_expr_env(sr.expr, 0);
        if (reg == -1) return CompileError::UndeclaredIdentifier;
        if (reg == -2) return CompileError::RegisterOverflow;
        program.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
      } else if (std::holds_alternative<StatementLet>(stmt.node)) {
        const auto& sl = std::get<StatementLet>(stmt.node);
        int reg = emit_expr_env(sl.expr, std::nullopt);
        if (reg == -1) return CompileError::UndeclaredIdentifier;
        if (reg == -2) return CompileError::RegisterOverflow;
        declare(sl.name, reg);
      } else if (std::holds_alternative<StatementIf>(stmt.node)) {
        const auto& sif = std::get<StatementIf>(stmt.node);
        int cond_reg = emit_expr_env(sif.condition, std::nullopt);
        if (cond_reg == -1) return CompileError::UndeclaredIdentifier;
        if (cond_reg == -2) return CompileError::RegisterOverflow;
        std::size_t jmp_ifz_index = program.insns.size();
        program.insns.push_back({t81::tisc::Opcode::JumpIfZero, 0, cond_reg, 0}); // placeholder target in a

        // then block
        push_scope();
        for (const auto& s : sif.then_body) {
          if (std::holds_alternative<StatementReturn>(s.node)) {
            const auto& sr = std::get<StatementReturn>(s.node);
            int reg = emit_expr_env(sr.expr, 0);
            if (reg == -1) return CompileError::UndeclaredIdentifier;
            if (reg == -2) return CompileError::RegisterOverflow;
            program.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
          }
        }
        pop_scope();
        // jump over else
        std::size_t jmp_over_else_index = program.insns.size();
        program.insns.push_back({t81::tisc::Opcode::Jump, 0, 0, 0}); // placeholder

        // else target
        std::size_t else_target = program.insns.size();
        program.insns[jmp_ifz_index].a = static_cast<std::int32_t>(else_target);
        push_scope();
        for (const auto& s : sif.else_body) {
          if (std::holds_alternative<StatementReturn>(s.node)) {
            const auto& sr = std::get<StatementReturn>(s.node);
            int reg = emit_expr_env(sr.expr, 0);
            if (reg == -1) return CompileError::UndeclaredIdentifier;
            if (reg == -2) return CompileError::RegisterOverflow;
            program.insns.push_back({t81::tisc::Opcode::Halt, 0, 0, 0});
          }
        }
        pop_scope();
        // set jump over else target
        program.insns[jmp_over_else_index].a = static_cast<std::int32_t>(program.insns.size());
      }
    }
    // ensure a return exists
    bool has_return = false;
    for (const auto& stmt : fn.body) {
      if (std::holds_alternative<StatementReturn>(stmt.node)) { has_return = true; break; }
      if (std::holds_alternative<StatementIf>(stmt.node)) {
        const auto& sif = std::get<StatementIf>(stmt.node);
        if (!sif.then_body.empty() && !sif.else_body.empty()) {
          bool then_ret = std::any_of(sif.then_body.begin(), sif.then_body.end(), [](const Statement& s){ return std::holds_alternative<StatementReturn>(s.node); });
          bool else_ret = std::any_of(sif.else_body.begin(), sif.else_body.end(), [](const Statement& s){ return std::holds_alternative<StatementReturn>(s.node); });
          if (then_ret && else_ret) has_return = true;
        }
      }
    }
    if (!has_return) return CompileError::UnsupportedType;
  }
  return program;
}
}  // namespace t81::lang
