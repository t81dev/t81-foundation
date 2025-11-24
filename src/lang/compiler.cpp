#include "t81/lang/compiler.hpp"

#include <string_view>
#include <vector>
#include <unordered_map>
#include <functional>
#include <expected>
#include <algorithm>
#include <cmath>
#include <optional>

#include "t81/bigint.hpp"
#include "t81/fraction.hpp"

namespace t81::lang {
namespace {

struct VarInfo {
  int reg{0};
  Type type{Type::T81Int};
};

std::optional<Type> literal_value_type(const ExprLiteral& lit) {
  switch (lit.value.kind) {
    case LiteralValue::Kind::Int: return Type::T81Int;
    case LiteralValue::Kind::Float: return Type::T81Float;
    case LiteralValue::Kind::Fraction: return Type::T81Fraction;
    case LiteralValue::Kind::Symbol: return Type::Symbol;
  }
  return std::nullopt;
}

bool literal_matches_type(const ExprLiteral& lit, Type type) {
  auto lt = literal_value_type(lit);
  return lt.has_value() && lt.value() == type;
}

int digit_value_from_cp(const std::string& cp) {
  const auto& map = t81::detail::base81_digit_map();
  auto it = map.find(cp);
  if (it == map.end()) return -1;
  return it->second;
}

bool accumulate_integer_digits(std::string_view digits, long double& out) {
  std::size_t offset = 0;
  while (offset < digits.size()) {
    std::string cp = t81::detail::next_codepoint(digits, offset);
    if (cp.empty()) return false;
    int value = digit_value_from_cp(cp);
    if (value < 0) return false;
    out = out * 81.0L + static_cast<long double>(value);
  }
  return true;
}

bool accumulate_fraction_digits(std::string_view digits, long double& out) {
  std::size_t offset = 0;
  long double place = 81.0L;
  while (offset < digits.size()) {
    std::string cp = t81::detail::next_codepoint(digits, offset);
    if (cp.empty()) return false;
    int value = digit_value_from_cp(cp);
    if (value < 0) return false;
    out += static_cast<long double>(value) / place;
    place *= 81.0L;
  }
  return true;
}

std::optional<double> parse_float_literal(const std::string& text) {
  if (text.empty()) return std::nullopt;
  std::size_t pos = 0;
  bool negative = false;
  if (text[pos] == '+' || text[pos] == '-') {
    negative = (text[pos] == '-');
    ++pos;
  }
  auto t_pos = text.find("t81", pos);
  if (t_pos == std::string::npos) return std::nullopt;
  std::string_view magnitude(text.data() + pos, t_pos - pos);
  std::size_t dot_pos = magnitude.find('.');
  std::string_view int_part = magnitude.substr(0, dot_pos == std::string_view::npos ? magnitude.size() : dot_pos);
  std::string_view frac_part;
  if (dot_pos != std::string_view::npos) {
    frac_part = magnitude.substr(dot_pos + 1);
  }
  long double value = 0.0L;
  if (!int_part.empty()) {
    if (!accumulate_integer_digits(int_part, value)) return std::nullopt;
  }
  if (!frac_part.empty()) {
    if (!accumulate_fraction_digits(frac_part, value)) return std::nullopt;
  }
  std::size_t suffix_pos = t_pos + 3;
  if (suffix_pos < text.size() && (text[suffix_pos] == 'f' || text[suffix_pos] == 'F')) {
    ++suffix_pos;
  }
  long long exponent = 0;
  if (suffix_pos < text.size()) {
    if (text[suffix_pos] != 'e' && text[suffix_pos] != 'E') return std::nullopt;
    ++suffix_pos;
    bool exp_neg = false;
    if (suffix_pos < text.size() && (text[suffix_pos] == '+' || text[suffix_pos] == '-')) {
      exp_neg = (text[suffix_pos] == '-');
      ++suffix_pos;
    }
    if (suffix_pos >= text.size()) return std::nullopt;
    std::string_view exp_digits(text.data() + suffix_pos, text.size() - suffix_pos);
    try {
      t81::T81BigInt exp_big = t81::T81BigInt::from_base81_string(std::string(exp_digits));
      exponent = exp_big.to_int64();
    } catch (...) {
      return std::nullopt;
    }
    if (exp_neg) exponent = -exponent;
  }
  if (negative) value = -value;
  if (exponent != 0) {
    value *= std::pow(81.0L, static_cast<long double>(exponent));
  }
  return static_cast<double>(value);
}

std::optional<t81::T81Fraction> parse_fraction_literal(const std::string& text) {
  auto slash = text.find('/');
  auto suffix_pos = text.rfind("t81");
  if (slash == std::string::npos || suffix_pos == std::string::npos || slash > suffix_pos) return std::nullopt;
  std::string num_str = text.substr(0, slash);
  std::string den_str = text.substr(slash + 1, suffix_pos - (slash + 1));
  if (den_str.empty()) return std::nullopt;
  try {
    t81::T81BigInt num = t81::T81BigInt::from_base81_string(num_str);
    t81::T81BigInt den = t81::T81BigInt::from_base81_string(den_str);
    return t81::T81Fraction(std::move(num), std::move(den));
  } catch (...) {
    return std::nullopt;
  }
}

CompileError emit_literal_constant(const ExprLiteral& lit, Type type, int target_reg,
                                   t81::tisc::Program& program) {
  switch (type) {
    case Type::T81Float: {
      auto value = parse_float_literal(lit.value.text);
      if (!value.has_value()) return CompileError::UnsupportedLiteral;
      program.float_pool.push_back(*value);
      int handle = static_cast<int>(program.float_pool.size());
      program.insns.push_back({t81::tisc::Opcode::LoadImm, target_reg, handle, 0});
      return CompileError::None;
    }
    case Type::T81Fraction: {
      auto frac = parse_fraction_literal(lit.value.text);
      if (!frac.has_value()) return CompileError::UnsupportedLiteral;
      program.fraction_pool.push_back(*frac);
      int handle = static_cast<int>(program.fraction_pool.size());
      program.insns.push_back({t81::tisc::Opcode::LoadImm, target_reg, handle, 0});
      return CompileError::None;
    }
    case Type::Symbol: {
      if (lit.value.text.empty()) return CompileError::UnsupportedLiteral;
      program.symbol_pool.push_back(lit.value.text);
      int handle = static_cast<int>(program.symbol_pool.size());
      program.insns.push_back({t81::tisc::Opcode::LoadImm, target_reg, handle, 0});
      return CompileError::None;
    }
    default:
      break;
  }
  return CompileError::UnsupportedLiteral;
}

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
    Type return_type{Type::T81Int};
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
    auto return_supported = [](Type type) {
      return type == Type::T81Int || type == Type::T81Float ||
             type == Type::T81Fraction || type == Type::Symbol;
    };
    if (!return_supported(fn.return_type)) {
      return CompileError::UnsupportedType;
    }
    if (!returns_all(fn.body)) {
      return CompileError::MissingReturn;
    }
    std::vector<std::unordered_map<std::string, VarInfo>> scopes;
    scopes.emplace_back();
    auto push_scope = [&]() { scopes.emplace_back(); };
    auto pop_scope = [&]() { if (scopes.size() > 1) scopes.pop_back(); };
    auto declare = [&](const std::string& name, int reg, Type type) { scopes.back()[name] = VarInfo{reg, type}; };
    auto lookup = [&](const std::string& name) -> std::optional<VarInfo> {
      for (auto it = scopes.rbegin(); it != scopes.rend(); ++it) {
        auto f = it->find(name);
        if (f != it->end()) return f->second;
      }
      return std::nullopt;
    };
    auto info_it = fn_info.find(fn.name);
    if (info_it == fn_info.end()) return CompileError::UnknownFunction;
    auto& fn_meta = info_it->second;
    fn_meta.entry_pc = program.insns.size();
    bool is_entry_fn = (fn_ptr == entry_fn_ptr);
    for (const auto& param : fn.params) {
      if (param.type != Type::T81Int) return CompileError::UnsupportedType;
      if (next_reg >= kMaxRegs) return CompileError::RegisterOverflow;
      declare(param.name, next_reg, Type::T81Int);
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
        auto info = lookup(id.name);
        if (!info.has_value()) return -1;
        if (info->type != Type::T81Int) return -5;
        int src = info->reg;
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
        pending_calls.push_back({program.insns.size() - 1, call.callee, callee_meta.return_type});
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
            if (fn.return_type == Type::T81Int) {
              int reg = emit_expr_env(sr.expr, 0);
              if (reg < 0) {
                auto err = map_expr_error(reg);
                if (err != CompileError::None) return err;
              }
              program.insns.push_back(
                  {is_entry_fn ? t81::tisc::Opcode::Halt : t81::tisc::Opcode::Ret, 0, 0, 0});
            } else {
              if (!std::holds_alternative<ExprLiteral>(sr.expr.node)) {
                return CompileError::UnsupportedLiteral;
              }
              const auto& lit = std::get<ExprLiteral>(sr.expr.node);
              if (!literal_matches_type(lit, fn.return_type)) {
                return CompileError::UnsupportedLiteral;
              }
              auto literal_err = emit_literal_constant(lit, fn.return_type, 0, program);
              if (literal_err != CompileError::None) return literal_err;
              program.insns.push_back(
                  {is_entry_fn ? t81::tisc::Opcode::Halt : t81::tisc::Opcode::Ret, 0, 0, 0});
            }
            return CompileError::None;
          }
          if (std::holds_alternative<StatementLet>(stmt.node)) {
            const auto& sl = std::get<StatementLet>(stmt.node);
            if (!sl.declared_type.has_value()) return CompileError::MissingType;
            Type decl_type = sl.declared_type.value();
            if (decl_type == Type::T81Int) {
              int reg = emit_expr_env(sl.expr, std::nullopt);
              if (reg < 0) {
                auto err = map_expr_error(reg);
                if (err != CompileError::None) return err;
              }
              declare(sl.name, reg, Type::T81Int);
            } else {
              if (!std::holds_alternative<ExprLiteral>(sl.expr.node)) {
                return CompileError::UnsupportedLiteral;
              }
              const auto& lit = std::get<ExprLiteral>(sl.expr.node);
              if (!literal_matches_type(lit, decl_type)) {
                return CompileError::UnsupportedLiteral;
              }
              if (next_reg >= kMaxRegs) return CompileError::RegisterOverflow;
              int reg = next_reg++;
              auto literal_err = emit_literal_constant(lit, decl_type, reg, program);
              if (literal_err != CompileError::None) return literal_err;
              declare(sl.name, reg, decl_type);
            }
            return CompileError::None;
          }
          if (std::holds_alternative<StatementAssign>(stmt.node)) {
            const auto& sa = std::get<StatementAssign>(stmt.node);
            auto info = lookup(sa.name);
            if (!info.has_value()) return CompileError::UndeclaredIdentifier;
            if (info->type == Type::T81Int) {
              int reg = emit_expr_env(sa.expr, info->reg);
              if (reg < 0) {
                auto err = map_expr_error(reg);
                if (err != CompileError::None) return err;
              }
            } else {
              if (!std::holds_alternative<ExprLiteral>(sa.expr.node)) {
                return CompileError::UnsupportedLiteral;
              }
              const auto& lit = std::get<ExprLiteral>(sa.expr.node);
              if (!literal_matches_type(lit, info->type)) {
                return CompileError::UnsupportedLiteral;
              }
              auto literal_err = emit_literal_constant(lit, info->type, info->reg, program);
              if (literal_err != CompileError::None) return literal_err;
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
