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

struct EvalValue {
  int reg{0};
  Type type{Type::T81Int};
};

using EvalResult = std::expected<EvalValue, CompileError>;

std::optional<Type> literal_value_type(const ExprLiteral& lit) {
  switch (lit.value.kind) {
    case LiteralValue::Kind::Int: return Type::T81Int;
    case LiteralValue::Kind::Float: return Type::T81Float;
    case LiteralValue::Kind::Fraction: return Type::T81Fraction;
    case LiteralValue::Kind::Symbol: return Type::Symbol;
  }
  return std::nullopt;
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
      program.insns.push_back({t81::tisc::Opcode::LoadImm, target_reg, handle, 0,
                               t81::tisc::LiteralKind::FloatHandle});
      return CompileError::None;
    }
    case Type::T81Fraction: {
      auto frac = parse_fraction_literal(lit.value.text);
      if (!frac.has_value()) return CompileError::UnsupportedLiteral;
      program.fraction_pool.push_back(*frac);
      int handle = static_cast<int>(program.fraction_pool.size());
      program.insns.push_back({t81::tisc::Opcode::LoadImm, target_reg, handle, 0,
                               t81::tisc::LiteralKind::FractionHandle});
      return CompileError::None;
    }
    case Type::Symbol: {
      if (lit.value.text.empty()) return CompileError::UnsupportedLiteral;
      program.symbol_pool.push_back(lit.value.text);
      int handle = static_cast<int>(program.symbol_pool.size());
      program.insns.push_back({t81::tisc::Opcode::LoadImm, target_reg, handle, 0,
                               t81::tisc::LiteralKind::SymbolHandle});
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
    std::vector<Type> param_types;
    Type return_type{Type::T81Int};
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
      info.param_types.push_back(fn.params[i].type);
    }
    info.return_type = fn.return_type;
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
    auto supported_type = [](Type type) {
      return type == Type::T81Int || type == Type::T81Float ||
             type == Type::T81Fraction || type == Type::Symbol;
    };
    if (!supported_type(fn.return_type)) {
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
      if (!supported_type(param.type)) return CompileError::UnsupportedType;
      if (next_reg >= kMaxRegs) return CompileError::RegisterOverflow;
      declare(param.name, next_reg, param.type);
      ++next_reg;
    }
    auto move_reg = [&](int src, int dst) {
      if (src == dst) return;
      program.insns.push_back({t81::tisc::Opcode::LoadImm, dst, 0, 0});
      program.insns.push_back({t81::tisc::Opcode::Add, dst, dst, src});
    };
    auto make_error = [](CompileError err) -> EvalResult {
      return EvalResult(err);
    };
    auto coerce_value = [&](EvalValue value, Type desired) -> EvalResult {
      if (value.type == desired) return value;
      switch (value.type) {
        case Type::T81Int:
          if (desired == Type::T81Float) {
            program.insns.push_back({t81::tisc::Opcode::I2F, value.reg, value.reg, 0});
            return EvalValue{value.reg, Type::T81Float};
          }
          if (desired == Type::T81Fraction) {
            program.insns.push_back({t81::tisc::Opcode::I2Frac, value.reg, value.reg, 0});
            return EvalValue{value.reg, Type::T81Fraction};
          }
          break;
        default:
          break;
      }
      return make_error(CompileError::UnsupportedType);
    };
    auto align_numeric_operands =
        [&](EvalValue& lhs_val, EvalValue& rhs_val) -> std::expected<void, CompileError> {
          auto is_numeric = [](Type type) {
            return type == Type::T81Int || type == Type::T81Float || type == Type::T81Fraction;
          };
          if (!is_numeric(lhs_val.type) || !is_numeric(rhs_val.type)) {
            return CompileError::UnsupportedType;
          }
          if (lhs_val.type == rhs_val.type) return {};
          if (lhs_val.type == Type::T81Int && rhs_val.type != Type::T81Int) {
            auto coerced = coerce_value(lhs_val, rhs_val.type);
            if (!coerced.has_value()) return coerced.error();
            lhs_val = coerced.value();
            return {};
          }
          if (rhs_val.type == Type::T81Int && lhs_val.type != Type::T81Int) {
            auto coerced = coerce_value(rhs_val, lhs_val.type);
            if (!coerced.has_value()) return coerced.error();
            rhs_val = coerced.value();
            return {};
          }
          return CompileError::UnsupportedType;
        };
    std::function<EvalResult(const Expr&, std::optional<int>)> emit_expr_env =
        [&](const Expr& e, std::optional<int> target) -> EvalResult {
      if (std::holds_alternative<ExprLiteral>(e.node)) {
        const auto& lit = std::get<ExprLiteral>(e.node);
        auto lit_type = literal_value_type(lit);
        if (!lit_type.has_value()) return make_error(CompileError::UnsupportedLiteral);
        Type type = lit_type.value();
        int reg = target.value_or(next_reg);
        if (reg >= kMaxRegs) return make_error(CompileError::RegisterOverflow);
        if (!target.has_value()) ++next_reg;
        if (type == Type::T81Int) {
          program.insns.push_back({t81::tisc::Opcode::LoadImm, reg,
                                   static_cast<std::int32_t>(lit.value.int_value), 0});
        } else {
          auto literal_err = emit_literal_constant(lit, type, reg, program);
          if (literal_err != CompileError::None) return make_error(literal_err);
        }
        return EvalValue{reg, type};
      }
      if (std::holds_alternative<ExprIdent>(e.node)) {
        const auto& id = std::get<ExprIdent>(e.node);
        auto info = lookup(id.name);
        if (!info.has_value()) return make_error(CompileError::UndeclaredIdentifier);
        int src = info->reg;
        int dst = target.value_or(src);
        move_reg(src, dst);
        return EvalValue{dst, info->type};
      }
      if (std::holds_alternative<ExprCall>(e.node)) {
        const auto& call = std::get<ExprCall>(e.node);
        auto callee_it = fn_info.find(call.callee);
        if (callee_it == fn_info.end()) return make_error(CompileError::UnknownFunction);
        const auto& callee_meta = callee_it->second;
        if (call.args.size() != callee_meta.param_regs.size()) {
          return make_error(CompileError::InvalidCall);
        }
        int saved_limit = next_reg;
        for (int r = 1; r < saved_limit; ++r) {
          program.insns.push_back({t81::tisc::Opcode::Push, r, 0, 0});
        }
        for (std::size_t i = 0; i < call.args.size(); ++i) {
          auto arg = emit_expr_env(call.args[i], callee_meta.param_regs[i]);
          if (!arg.has_value()) return arg;
          EvalValue arg_val = arg.value();
          auto coerced = coerce_value(arg_val, callee_meta.param_types[i]);
          if (!coerced.has_value()) return coerced;
        }
        int reserve_floor = 1;
        if (!callee_meta.param_regs.empty()) {
          reserve_floor = *std::max_element(callee_meta.param_regs.begin(),
                                            callee_meta.param_regs.end()) +
                          1;
        }
        if (next_reg < reserve_floor) next_reg = reserve_floor;
        int call_reg = next_reg;
        if (call_reg >= kMaxRegs) return make_error(CompileError::RegisterOverflow);
        ++next_reg;
        program.insns.push_back({t81::tisc::Opcode::LoadImm, call_reg, 0, 0});
        pending_calls.push_back({program.insns.size() - 1, call.callee});
        program.insns.push_back({t81::tisc::Opcode::Call, 0, call_reg, 0});
        int result_tmp = next_reg;
        if (result_tmp >= kMaxRegs) return make_error(CompileError::RegisterOverflow);
        ++next_reg;
        move_reg(0, result_tmp);
        for (int r = saved_limit - 1; r >= 1; --r) {
          program.insns.push_back({t81::tisc::Opcode::Pop, r, 0, 0});
        }
        int out_reg = target.value_or(next_reg);
        if (out_reg >= kMaxRegs) return make_error(CompileError::RegisterOverflow);
        if (!target.has_value()) ++next_reg;
        move_reg(result_tmp, out_reg);
        return EvalValue{out_reg, callee_meta.return_type};
      }
      const auto& bin = std::get<ExprBinary>(e.node);
      auto is_arithmetic_op = [](ExprBinary::Op op) {
        switch (op) {
          case ExprBinary::Op::Add:
          case ExprBinary::Op::Sub:
          case ExprBinary::Op::Mul:
          case ExprBinary::Op::Div:
          case ExprBinary::Op::Mod:
            return true;
          default:
            return false;
        }
      };
      auto is_comparison_op = [](ExprBinary::Op op) {
        switch (op) {
          case ExprBinary::Op::Eq:
          case ExprBinary::Op::Ne:
          case ExprBinary::Op::Lt:
          case ExprBinary::Op::Le:
          case ExprBinary::Op::Gt:
          case ExprBinary::Op::Ge:
            return true;
          default:
            return false;
        }
      };
      auto is_logical_op = [](ExprBinary::Op op) {
        return op == ExprBinary::Op::Land || op == ExprBinary::Op::Lor;
      };
      auto alloc_temp_reg = [&]() -> std::expected<int, CompileError> {
        int reg = next_reg;
        if (reg >= kMaxRegs) return CompileError::RegisterOverflow;
        ++next_reg;
        return reg;
      };
      auto emit_bool_from_branch =
          [&](auto&& branch_builder, std::optional<int> dest) -> EvalResult {
            int out_reg = dest.value_or(next_reg);
            if (out_reg >= kMaxRegs) return make_error(CompileError::RegisterOverflow);
            if (!dest.has_value()) ++next_reg;
            program.insns.push_back({t81::tisc::Opcode::LoadImm, out_reg, 0, 0});
            auto branch_idx_res = branch_builder();
            if (!branch_idx_res.has_value()) return make_error(branch_idx_res.error());
            std::size_t branch_idx = branch_idx_res.value();
            std::size_t skip_idx = program.insns.size();
            program.insns.push_back({t81::tisc::Opcode::Jump, 0, 0, 0});
            std::size_t true_pc = program.insns.size();
            program.insns.push_back({t81::tisc::Opcode::LoadImm, out_reg, 1, 0});
            std::size_t end_pc = program.insns.size();
            program.insns[branch_idx].a = static_cast<std::int32_t>(true_pc);
            program.insns[skip_idx].a = static_cast<std::int32_t>(end_pc);
            return EvalValue{out_reg, Type::T81Int};
          };
      auto emit_bool_constant = [&](bool value, std::optional<int> dest) -> EvalResult {
        int out_reg = dest.value_or(next_reg);
        if (out_reg >= kMaxRegs) return make_error(CompileError::RegisterOverflow);
        if (!dest.has_value()) ++next_reg;
        program.insns.push_back({t81::tisc::Opcode::LoadImm, out_reg, value ? 1 : 0, 0});
        return EvalValue{out_reg, Type::T81Int};
      };
      auto fold_literal_comparison =
          [&](ExprBinary::Op op) -> std::optional<bool> {
            if (!std::holds_alternative<ExprLiteral>(bin.lhs->node) ||
                !std::holds_alternative<ExprLiteral>(bin.rhs->node)) {
              return std::nullopt;
            }
            const auto& lhs_lit = std::get<ExprLiteral>(bin.lhs->node);
            const auto& rhs_lit = std::get<ExprLiteral>(bin.rhs->node);
            auto lhs_type = literal_value_type(lhs_lit);
            auto rhs_type = literal_value_type(rhs_lit);
            if (!lhs_type.has_value() || !rhs_type.has_value()) {
              return std::nullopt;
            }
            auto eval_relation = [&](int cmp) -> bool {
              switch (op) {
                case ExprBinary::Op::Eq: return cmp == 0;
                case ExprBinary::Op::Ne: return cmp != 0;
                case ExprBinary::Op::Lt: return cmp < 0;
                case ExprBinary::Op::Le: return cmp <= 0;
                case ExprBinary::Op::Gt: return cmp > 0;
                case ExprBinary::Op::Ge: return cmp >= 0;
                default: return false;
              }
            };
            auto literal_cmp = [&](const ExprLiteral& lhs_eval, Type lhs_t,
                                   const ExprLiteral& rhs_eval,
                                   Type rhs_t) -> std::optional<int> {
              if (lhs_t == Type::Symbol || rhs_t == Type::Symbol) {
                if (lhs_t != Type::Symbol || rhs_t != Type::Symbol) return std::nullopt;
                if (lhs_eval.value.text == rhs_eval.value.text) return 0;
                return (lhs_eval.value.text < rhs_eval.value.text) ? -1 : 1;
              }
              if (lhs_t == rhs_t) {
                switch (lhs_t) {
                  case Type::T81Int: {
                    auto lhs_val = lhs_eval.value.int_value;
                    auto rhs_val = rhs_eval.value.int_value;
                    if (lhs_val == rhs_val) return 0;
                    return (lhs_val < rhs_val) ? -1 : 1;
                  }
                  case Type::T81Float: {
                    auto lhs_parsed = parse_float_literal(lhs_eval.value.text);
                    auto rhs_parsed = parse_float_literal(rhs_eval.value.text);
                    if (!lhs_parsed.has_value() || !rhs_parsed.has_value()) return std::nullopt;
                    double lhs_val = *lhs_parsed;
                    double rhs_val = *rhs_parsed;
                    if (lhs_val == rhs_val) return 0;
                    return (lhs_val < rhs_val) ? -1 : 1;
                  }
                  case Type::T81Fraction: {
                    auto lhs_frac = parse_fraction_literal(lhs_eval.value.text);
                    auto rhs_frac = parse_fraction_literal(rhs_eval.value.text);
                    if (!lhs_frac.has_value() || !rhs_frac.has_value()) return std::nullopt;
                    return t81::T81Fraction::cmp(*lhs_frac, *rhs_frac);
                  }
                  default:
                    break;
                }
                return std::nullopt;
              }
              if (lhs_t == Type::T81Int && rhs_t == Type::T81Float) {
                auto rhs_parsed = parse_float_literal(rhs_eval.value.text);
                if (!rhs_parsed.has_value()) return std::nullopt;
                double lhs_val = static_cast<double>(lhs_eval.value.int_value);
                double rhs_val = *rhs_parsed;
                if (lhs_val == rhs_val) return 0;
                return (lhs_val < rhs_val) ? -1 : 1;
              }
              if (lhs_t == Type::T81Float && rhs_t == Type::T81Int) {
                auto lhs_parsed = parse_float_literal(lhs_eval.value.text);
                if (!lhs_parsed.has_value()) return std::nullopt;
                double lhs_val = *lhs_parsed;
                double rhs_val = static_cast<double>(rhs_eval.value.int_value);
                if (lhs_val == rhs_val) return 0;
                return (lhs_val < rhs_val) ? -1 : 1;
              }
              if (lhs_t == Type::T81Int && rhs_t == Type::T81Fraction) {
                auto rhs_frac = parse_fraction_literal(rhs_eval.value.text);
                if (!rhs_frac.has_value()) return std::nullopt;
                auto lhs_frac = t81::T81Fraction::from_int(lhs_eval.value.int_value);
                return t81::T81Fraction::cmp(lhs_frac, *rhs_frac);
              }
              if (lhs_t == Type::T81Fraction && rhs_t == Type::T81Int) {
                auto lhs_frac = parse_fraction_literal(lhs_eval.value.text);
                if (!lhs_frac.has_value()) return std::nullopt;
                auto rhs_frac = t81::T81Fraction::from_int(rhs_eval.value.int_value);
                return t81::T81Fraction::cmp(*lhs_frac, rhs_frac);
              }
              return std::nullopt;
            };
            auto cmp = literal_cmp(lhs_lit, lhs_type.value(), rhs_lit, rhs_type.value());
            if (!cmp.has_value()) return std::nullopt;
            if ((lhs_type.value() == Type::Symbol || rhs_type.value() == Type::Symbol) &&
                op != ExprBinary::Op::Eq && op != ExprBinary::Op::Ne) {
              return std::nullopt;
            }
            return eval_relation(*cmp);
          };
      if (is_logical_op(bin.op)) {
        auto lhs = emit_expr_env(*bin.lhs, std::nullopt);
        if (!lhs.has_value()) return lhs;
        const EvalValue lhs_val = lhs.value();
        if (lhs_val.type != Type::T81Int) return make_error(CompileError::UnsupportedType);
        int out_reg = target.value_or(next_reg);
        if (out_reg >= kMaxRegs) return make_error(CompileError::RegisterOverflow);
        if (!target.has_value()) ++next_reg;
        program.insns.push_back({t81::tisc::Opcode::LoadImm, out_reg, 0, 0});
        if (bin.op == ExprBinary::Op::Land) {
          std::size_t lhs_zero_idx = program.insns.size();
          program.insns.push_back({t81::tisc::Opcode::JumpIfZero, 0, lhs_val.reg, 0});
          auto rhs = emit_expr_env(*bin.rhs, std::nullopt);
          if (!rhs.has_value()) return rhs;
          const EvalValue rhs_val = rhs.value();
          if (rhs_val.type != Type::T81Int) return make_error(CompileError::UnsupportedType);
          std::size_t rhs_zero_idx = program.insns.size();
          program.insns.push_back({t81::tisc::Opcode::JumpIfZero, 0, rhs_val.reg, 0});
          program.insns.push_back({t81::tisc::Opcode::LoadImm, out_reg, 1, 0});
          std::size_t end_pc = program.insns.size();
          program.insns[lhs_zero_idx].a = static_cast<std::int32_t>(end_pc);
          program.insns[rhs_zero_idx].a = static_cast<std::int32_t>(end_pc);
          return EvalValue{out_reg, Type::T81Int};
        } else {
          std::size_t lhs_true_idx = program.insns.size();
          program.insns.push_back({t81::tisc::Opcode::JumpIfNotZero, 0, lhs_val.reg, 0});
          auto rhs = emit_expr_env(*bin.rhs, std::nullopt);
          if (!rhs.has_value()) return rhs;
          const EvalValue rhs_val = rhs.value();
          if (rhs_val.type != Type::T81Int) return make_error(CompileError::UnsupportedType);
          std::size_t rhs_true_idx = program.insns.size();
          program.insns.push_back({t81::tisc::Opcode::JumpIfNotZero, 0, rhs_val.reg, 0});
          std::size_t skip_true_idx = program.insns.size();
          program.insns.push_back({t81::tisc::Opcode::Jump, 0, 0, 0});
          std::size_t true_pc = program.insns.size();
          program.insns.push_back({t81::tisc::Opcode::LoadImm, out_reg, 1, 0});
          std::size_t end_pc = program.insns.size();
          program.insns[lhs_true_idx].a = static_cast<std::int32_t>(true_pc);
          program.insns[rhs_true_idx].a = static_cast<std::int32_t>(true_pc);
          program.insns[skip_true_idx].a = static_cast<std::int32_t>(end_pc);
          return EvalValue{out_reg, Type::T81Int};
        }
      }
      if (is_comparison_op(bin.op)) {
        if (auto folded = fold_literal_comparison(bin.op)) {
          return emit_bool_constant(*folded, target);
        }
        auto lhs = emit_expr_env(*bin.lhs, std::nullopt);
        if (!lhs.has_value()) return lhs;
        auto rhs = emit_expr_env(*bin.rhs, std::nullopt);
        if (!rhs.has_value()) return rhs;
        EvalValue lhs_val = lhs.value();
        EvalValue rhs_val = rhs.value();
        if (lhs_val.type == Type::Symbol || rhs_val.type == Type::Symbol) {
          if (lhs_val.type != Type::Symbol || rhs_val.type != Type::Symbol) {
            return make_error(CompileError::UnsupportedType);
          }
        } else {
          auto align_result = align_numeric_operands(lhs_val, rhs_val);
          if (!align_result.has_value()) return make_error(align_result.error());
        }
        auto emit_comparison =
            [&](ExprBinary::Op op, const EvalValue& rhs_eval,
                std::optional<int> dest) -> EvalResult {
              if (lhs_val.type == Type::Symbol) {
                if (op != ExprBinary::Op::Eq && op != ExprBinary::Op::Ne) {
                  return make_error(CompileError::UnsupportedType);
                }
              } else if (lhs_val.type != Type::T81Int && lhs_val.type != Type::T81Float &&
                         lhs_val.type != Type::T81Fraction) {
                return make_error(CompileError::UnsupportedType);
              }
              program.insns.push_back({t81::tisc::Opcode::Cmp, lhs_val.reg, rhs_eval.reg, 0});
              auto sign_reg_res = alloc_temp_reg();
              if (!sign_reg_res.has_value()) return make_error(sign_reg_res.error());
              int sign_reg = sign_reg_res.value();
              program.insns.push_back({t81::tisc::Opcode::SetF, sign_reg, 0, 0});
              auto branch_builder = [&]() -> std::expected<std::size_t, CompileError> {
                switch (op) {
                  case ExprBinary::Op::Eq: {
                    std::size_t idx = program.insns.size();
                    program.insns.push_back({t81::tisc::Opcode::JumpIfZero, 0, sign_reg, 0});
                    return idx;
                  }
                  case ExprBinary::Op::Ne: {
                    std::size_t idx = program.insns.size();
                    program.insns.push_back({t81::tisc::Opcode::JumpIfNotZero, 0, sign_reg, 0});
                    return idx;
                  }
                  case ExprBinary::Op::Lt: {
                  auto tmp_res = alloc_temp_reg();
                  if (!tmp_res.has_value()) return tmp_res.error();
                    int tmp = tmp_res.value();
                    program.insns.push_back({t81::tisc::Opcode::LoadImm, tmp, 1, 0});
                    program.insns.push_back({t81::tisc::Opcode::Add, tmp, sign_reg, tmp});
                    std::size_t idx = program.insns.size();
                    program.insns.push_back({t81::tisc::Opcode::JumpIfZero, 0, tmp, 0});
                    return idx;
                  }
                  case ExprBinary::Op::Le: {
                  auto tmp_res = alloc_temp_reg();
                  if (!tmp_res.has_value()) return tmp_res.error();
                    int tmp = tmp_res.value();
                    program.insns.push_back({t81::tisc::Opcode::LoadImm, tmp, 1, 0});
                    program.insns.push_back({t81::tisc::Opcode::Sub, tmp, sign_reg, tmp});
                    std::size_t idx = program.insns.size();
                    program.insns.push_back({t81::tisc::Opcode::JumpIfNotZero, 0, tmp, 0});
                    return idx;
                  }
                  case ExprBinary::Op::Gt: {
                  auto tmp_res = alloc_temp_reg();
                  if (!tmp_res.has_value()) return tmp_res.error();
                    int tmp = tmp_res.value();
                    program.insns.push_back({t81::tisc::Opcode::LoadImm, tmp, 1, 0});
                    program.insns.push_back({t81::tisc::Opcode::Sub, tmp, sign_reg, tmp});
                    std::size_t idx = program.insns.size();
                    program.insns.push_back({t81::tisc::Opcode::JumpIfZero, 0, tmp, 0});
                    return idx;
                  }
                  case ExprBinary::Op::Ge: {
                  auto tmp_res = alloc_temp_reg();
                  if (!tmp_res.has_value()) return tmp_res.error();
                    int tmp = tmp_res.value();
                    program.insns.push_back({t81::tisc::Opcode::LoadImm, tmp, 1, 0});
                    program.insns.push_back({t81::tisc::Opcode::Add, tmp, sign_reg, tmp});
                    std::size_t idx = program.insns.size();
                    program.insns.push_back({t81::tisc::Opcode::JumpIfNotZero, 0, tmp, 0});
                    return idx;
                  }
                  default:
                    return CompileError::UnsupportedType;
                }
              };
              return emit_bool_from_branch(branch_builder, dest);
            };
        return emit_comparison(bin.op, rhs_val, target);
      }
      auto lhs = emit_expr_env(*bin.lhs, std::nullopt);
      if (!lhs.has_value()) return lhs;
      auto rhs = emit_expr_env(*bin.rhs, std::nullopt);
      if (!rhs.has_value()) return rhs;
      EvalValue lhs_val = lhs.value();
      EvalValue rhs_val = rhs.value();
      if (!is_arithmetic_op(bin.op)) {
        return make_error(CompileError::UnsupportedType);
      }
      if (bin.op == ExprBinary::Op::Mod) {
        if (lhs_val.type != Type::T81Int || rhs_val.type != Type::T81Int) {
          return make_error(CompileError::UnsupportedType);
        }
      } else {
        auto align_result = align_numeric_operands(lhs_val, rhs_val);
        if (!align_result.has_value()) return make_error(align_result.error());
      }
      Type expr_type = lhs_val.type;
      std::optional<t81::tisc::Opcode> opcode;
      switch (expr_type) {
        case Type::T81Int:
          switch (bin.op) {
            case ExprBinary::Op::Add: opcode = t81::tisc::Opcode::Add; break;
            case ExprBinary::Op::Sub: opcode = t81::tisc::Opcode::Sub; break;
            case ExprBinary::Op::Mul: opcode = t81::tisc::Opcode::Mul; break;
            case ExprBinary::Op::Div: opcode = t81::tisc::Opcode::Div; break;
            case ExprBinary::Op::Mod: opcode = t81::tisc::Opcode::Mod; break;
            default: break;
          }
          break;
        case Type::T81Float:
          switch (bin.op) {
            case ExprBinary::Op::Add: opcode = t81::tisc::Opcode::FAdd; break;
            case ExprBinary::Op::Sub: opcode = t81::tisc::Opcode::FSub; break;
            case ExprBinary::Op::Mul: opcode = t81::tisc::Opcode::FMul; break;
            case ExprBinary::Op::Div: opcode = t81::tisc::Opcode::FDiv; break;
            default: break;
          }
          break;
        case Type::T81Fraction:
          switch (bin.op) {
            case ExprBinary::Op::Add: opcode = t81::tisc::Opcode::FracAdd; break;
            case ExprBinary::Op::Sub: opcode = t81::tisc::Opcode::FracSub; break;
            case ExprBinary::Op::Mul: opcode = t81::tisc::Opcode::FracMul; break;
            case ExprBinary::Op::Div: opcode = t81::tisc::Opcode::FracDiv; break;
            default: break;
          }
          break;
        default:
          return make_error(CompileError::UnsupportedType);
      }
      if (!opcode.has_value()) {
        return make_error(CompileError::UnsupportedType);
      }
      int out_reg = target.value_or(next_reg);
      if (out_reg >= kMaxRegs) return make_error(CompileError::RegisterOverflow);
      if (!target.has_value()) ++next_reg;
      program.insns.push_back({opcode.value(), out_reg, lhs_val.reg, rhs_val.reg});
      return EvalValue{out_reg, expr_type};
    };

    std::function<CompileError(const Statement&)> emit_stmt =
        [&](const Statement& stmt) -> CompileError {
          if (std::holds_alternative<StatementReturn>(stmt.node)) {
            const auto& sr = std::get<StatementReturn>(stmt.node);
            auto value = emit_expr_env(sr.expr, 0);
            if (!value.has_value()) return value.error();
            auto coerced = coerce_value(value.value(), fn.return_type);
            if (!coerced.has_value()) return coerced.error();
            program.insns.push_back(
                {is_entry_fn ? t81::tisc::Opcode::Halt : t81::tisc::Opcode::Ret, 0, 0, 0});
            return CompileError::None;
          }
          if (std::holds_alternative<StatementLet>(stmt.node)) {
            const auto& sl = std::get<StatementLet>(stmt.node);
            if (!sl.declared_type.has_value()) return CompileError::MissingType;
            Type decl_type = sl.declared_type.value();
            if (!supported_type(decl_type)) return CompileError::UnsupportedType;
            auto value = emit_expr_env(sl.expr, std::nullopt);
            if (!value.has_value()) return value.error();
            auto coerced = coerce_value(value.value(), decl_type);
            if (!coerced.has_value()) return coerced.error();
            const EvalValue result = coerced.value();
            declare(sl.name, result.reg, decl_type);
            return CompileError::None;
          }
          if (std::holds_alternative<StatementAssign>(stmt.node)) {
            const auto& sa = std::get<StatementAssign>(stmt.node);
            auto info = lookup(sa.name);
            if (!info.has_value()) return CompileError::UndeclaredIdentifier;
            auto value = emit_expr_env(sa.expr, info->reg);
            if (!value.has_value()) return value.error();
            auto coerced = coerce_value(value.value(), info->type);
            if (!coerced.has_value()) return coerced.error();
            return CompileError::None;
          }
          if (std::holds_alternative<StatementIf>(stmt.node)) {
            const auto& sif = std::get<StatementIf>(stmt.node);
            auto cond = emit_expr_env(sif.condition, std::nullopt);
            if (!cond.has_value()) return cond.error();
            const EvalValue cond_value = cond.value();
            if (cond_value.type != Type::T81Int) return CompileError::UnsupportedType;
            std::size_t jmp_ifz_index = program.insns.size();
            program.insns.push_back({t81::tisc::Opcode::JumpIfZero, 0, cond_value.reg, 0});

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
            auto value = emit_expr_env(se.expr, std::nullopt);
            if (!value.has_value()) return value.error();
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
