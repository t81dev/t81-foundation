#include "t81/lang/compiler.hpp"

#include <vector>
#include <unordered_map>
#include <functional>
#include <expected>
#include <algorithm>

namespace t81::lang {
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
    } else if (std::holds_alternative<StatementLoop>(s.node)) {
      const auto& loop = std::get<StatementLoop>(s.node);
      if (returns_all(loop.body)) saw_return = true;
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
  const int kMaxRegs = 26; // R26 reserved
  struct FunctionInfo {
    std::vector<int> param_regs;
    std::size_t entry_pc{0};
  };
  struct PendingCall {
    std::size_t load_index{0};
    std::string callee;
  };
  std::unordered_map<std::string, FunctionInfo> fn_info;
  const Function* entry_fn_ptr = nullptr;
  for (const auto& fn : module.functions) {
    FunctionInfo info;
    for (std::size_t i = 0; i < fn.params.size(); ++i) {
      info.param_regs.push_back(static_cast<int>(i + 1));
    }
    fn_info.emplace(fn.name, std::move(info));
    if (!entry_fn_ptr && fn.name == "main") {
      entry_fn_ptr = &fn;
    }
  }
  if (!entry_fn_ptr) {
    entry_fn_ptr = &module.functions.front();
  }
  std::vector<PendingCall> pending_calls;
  std::vector<const Function*> ordered_functions;
  ordered_functions.push_back(entry_fn_ptr);
  for (const auto& fn : module.functions) {
    if (&fn != entry_fn_ptr) ordered_functions.push_back(&fn);
  }
  for (const Function* fn_ptr : ordered_functions) {
    const auto& fn = *fn_ptr;
    int next_reg = 1; // R0 reserved for return
    if (fn.return_type != Type::T81Int) {
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
    auto info_it = fn_info.find(fn.name);
    if (info_it == fn_info.end()) return CompileError::UnknownFunction;
    auto& fn_meta = info_it->second;
    fn_meta.entry_pc = program.insns.size();
    bool is_entry_fn = (fn_ptr == entry_fn_ptr);
    for (const auto& param : fn.params) {
      if (param.type != Type::T81Int) return CompileError::UnsupportedType;
      if (next_reg >= kMaxRegs) return CompileError::RegisterOverflow;
      declare(param.name, next_reg);
      ++next_reg;
    }
    auto move_reg = [&](int src, int dst) {
      if (src == dst) return;
      program.insns.push_back({t81::tisc::Opcode::LoadImm, dst, 0, 0});
      program.insns.push_back({t81::tisc::Opcode::Add, dst, dst, src});
    };
    auto map_expr_error = [&](int code) -> CompileError {
      if (code == -1) return CompileError::UndeclaredIdentifier;
      if (code == -2) return CompileError::RegisterOverflow;
      if (code == -3) return CompileError::UnknownFunction;
      if (code == -4) return CompileError::InvalidCall;
      if (code == -5) return CompileError::UnsupportedLiteral;
      return CompileError::None;
    };
    std::function<int(const Expr&, std::optional<int>)> emit_expr_env = [&](const Expr& e, std::optional<int> target) -> int {
      if (std::holds_alternative<ExprLiteral>(e.node)) {
        const auto& lit = std::get<ExprLiteral>(e.node);
        if (lit.value.kind != LiteralValue::Kind::Int) return -5;
        int reg = target.value_or(next_reg);
        if (reg >= kMaxRegs) return -2;
        if (!target.has_value()) ++next_reg;
        program.insns.push_back(
            {t81::tisc::Opcode::LoadImm, reg, static_cast<std::int32_t>(lit.value.int_value), 0});
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
      if (std::holds_alternative<ExprCall>(e.node)) {
        const auto& call = std::get<ExprCall>(e.node);
        auto callee_it = fn_info.find(call.callee);
        if (callee_it == fn_info.end()) return -3;
        const auto& callee_meta = callee_it->second;
        if (call.args.size() != callee_meta.param_regs.size()) return -4;
        int saved_limit = next_reg;
        for (int r = 1; r < saved_limit; ++r) {
          program.insns.push_back({t81::tisc::Opcode::Push, r, 0, 0});
        }
        for (std::size_t i = 0; i < call.args.size(); ++i) {
          int param_reg = callee_meta.param_regs[i];
          int arg_reg = emit_expr_env(call.args[i], param_reg);
          if (arg_reg < 0) return arg_reg;
        }
        int reserve_floor = 1;
        if (!callee_meta.param_regs.empty()) {
          reserve_floor = *std::max_element(callee_meta.param_regs.begin(),
                                            callee_meta.param_regs.end()) +
                          1;
        }
        if (next_reg < reserve_floor) next_reg = reserve_floor;
        int call_reg = next_reg;
        if (call_reg >= kMaxRegs) return -2;
        ++next_reg;
        program.insns.push_back({t81::tisc::Opcode::LoadImm, call_reg, 0, 0});
        pending_calls.push_back({program.insns.size() - 1, call.callee});
        program.insns.push_back({t81::tisc::Opcode::Call, 0, call_reg, 0});
        int result_tmp = next_reg;
        if (result_tmp >= kMaxRegs) return -2;
        ++next_reg;
        move_reg(0, result_tmp);
        for (int r = saved_limit - 1; r >= 1; --r) {
          program.insns.push_back({t81::tisc::Opcode::Pop, r, 0, 0});
        }
        int out_reg = target.value_or(next_reg);
        if (out_reg >= kMaxRegs) return -2;
        if (!target.has_value()) ++next_reg;
        move_reg(result_tmp, out_reg);
        return out_reg;
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

    std::function<CompileError(const Statement&)> emit_stmt =
        [&](const Statement& stmt) -> CompileError {
          if (std::holds_alternative<StatementReturn>(stmt.node)) {
            const auto& sr = std::get<StatementReturn>(stmt.node);
            int reg = emit_expr_env(sr.expr, 0);
            if (reg < 0) {
              auto err = map_expr_error(reg);
              if (err != CompileError::None) return err;
            }
            program.insns.push_back(
                {is_entry_fn ? t81::tisc::Opcode::Halt : t81::tisc::Opcode::Ret, 0, 0, 0});
            return CompileError::None;
          }
          if (std::holds_alternative<StatementLet>(stmt.node)) {
            const auto& sl = std::get<StatementLet>(stmt.node);
            if (!sl.declared_type.has_value()) return CompileError::MissingType;
            if (sl.declared_type.value() != Type::T81Int) return CompileError::UnsupportedType;
            int reg = emit_expr_env(sl.expr, std::nullopt);
            if (reg < 0) {
              auto err = map_expr_error(reg);
              if (err != CompileError::None) return err;
            }
            declare(sl.name, reg);
            return CompileError::None;
          }
          if (std::holds_alternative<StatementAssign>(stmt.node)) {
            const auto& sa = std::get<StatementAssign>(stmt.node);
            int dst = lookup(sa.name);
            if (dst < 0) return CompileError::UndeclaredIdentifier;
            int reg = emit_expr_env(sa.expr, dst);
            if (reg < 0) {
              auto err = map_expr_error(reg);
              if (err != CompileError::None) return err;
            }
            return CompileError::None;
          }
          if (std::holds_alternative<StatementIf>(stmt.node)) {
            const auto& sif = std::get<StatementIf>(stmt.node);
            int cond_reg = emit_expr_env(sif.condition, std::nullopt);
            if (cond_reg < 0) {
              auto err = map_expr_error(cond_reg);
              if (err != CompileError::None) return err;
            }
            std::size_t jmp_ifz_index = program.insns.size();
            program.insns.push_back({t81::tisc::Opcode::JumpIfZero, 0, cond_reg, 0});

            push_scope();
            for (const auto& inner : sif.then_body) {
              auto res = emit_stmt(inner);
              if (res != CompileError::None) { pop_scope(); return res; }
            }
            pop_scope();

            std::size_t jmp_over_else_index = program.insns.size();
            program.insns.push_back({t81::tisc::Opcode::Jump, 0, 0, 0});

            std::size_t else_target = program.insns.size();
            program.insns[jmp_ifz_index].a = static_cast<std::int32_t>(else_target);

            push_scope();
            for (const auto& inner : sif.else_body) {
              auto res = emit_stmt(inner);
              if (res != CompileError::None) { pop_scope(); return res; }
            }
            pop_scope();
            program.insns[jmp_over_else_index].a = static_cast<std::int32_t>(program.insns.size());
            return CompileError::None;
          }
          if (std::holds_alternative<StatementLoop>(stmt.node)) {
            const auto& loop = std::get<StatementLoop>(stmt.node);
            std::size_t loop_start = program.insns.size();
            push_scope();
            for (const auto& inner : loop.body) {
              auto res = emit_stmt(inner);
              if (res != CompileError::None) { pop_scope(); return res; }
            }
            pop_scope();
            program.insns.push_back(
                {t81::tisc::Opcode::Jump, static_cast<std::int32_t>(loop_start), 0, 0});
            return CompileError::None;
          }
          if (std::holds_alternative<StatementExpr>(stmt.node)) {
            const auto& se = std::get<StatementExpr>(stmt.node);
            int reg = emit_expr_env(se.expr, std::nullopt);
            if (reg < 0) {
              auto err = map_expr_error(reg);
              if (err != CompileError::None) return err;
            }
            return CompileError::None;
          }
          return CompileError::None;
        };

    for (const auto& stmt : fn.body) {
      auto err = emit_stmt(stmt);
      if (err != CompileError::None) return err;
    }
  }
  for (const auto& pending : pending_calls) {
    auto it = fn_info.find(pending.callee);
    if (it == fn_info.end()) return CompileError::UnknownFunction;
    program.insns[pending.load_index].b = static_cast<std::int32_t>(it->second.entry_pc);
  }
  return program;
}
}  // namespace t81::lang
