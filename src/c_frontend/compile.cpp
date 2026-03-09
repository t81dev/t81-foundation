#include "t81/c_frontend/compile.hpp"

#ifdef T81_HAS_C_FRONTEND

#include <clang-c/Index.h>

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <functional>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "llvm/Support/raw_ostream.h"
#include "mlir/IR/MLIRContext.h"
#include "t81/isa/opcodes.hpp"
#include "t81/isa/program.hpp"
#include "t81/mlir/tisc_to_mlir.hpp"

namespace fs = std::filesystem;

namespace t81::c_frontend {

namespace {

std::string to_string_and_dispose(CXString value) {
  const char* cstr = clang_getCString(value);
  std::string out = cstr ? cstr : "";
  clang_disposeString(value);
  return out;
}

std::string trim_copy(std::string_view text) {
  size_t start = 0;
  while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
    ++start;
  }
  size_t end = text.size();
  while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
    --end;
  }
  return std::string(text.substr(start, end - start));
}

bool source_offsets(CXSourceRange range, unsigned& begin, unsigned& end) {
  clang_getSpellingLocation(clang_getRangeStart(range), nullptr, nullptr, nullptr, &begin);
  clang_getSpellingLocation(clang_getRangeEnd(range), nullptr, nullptr, nullptr, &end);
  return end >= begin;
}

std::string cursor_text(CXCursor cursor, std::string_view source) {
  unsigned begin = 0;
  unsigned end = 0;
  if (!source_offsets(clang_getCursorExtent(cursor), begin, end)) {
    return {};
  }
  if (begin > source.size()) begin = static_cast<unsigned>(source.size());
  if (end > source.size()) end = static_cast<unsigned>(source.size());
  if (end < begin) end = begin;
  return std::string(source.substr(begin, end - begin));
}

std::vector<CXCursor> cursor_children(CXCursor cursor) {
  std::vector<CXCursor> children;
  clang_visitChildren(
      cursor,
      [](CXCursor child, CXCursor, CXClientData data) {
        auto* out = static_cast<std::vector<CXCursor>*>(data);
        out->push_back(child);
        return CXChildVisit_Continue;
      },
      &children);
  return children;
}

CXCursor unwrap_expr(CXCursor cursor) {
  while (true) {
    const CXCursorKind kind = clang_getCursorKind(cursor);
    if (kind != CXCursor_UnexposedExpr && kind != CXCursor_ParenExpr) {
      return cursor;
    }
    auto children = cursor_children(cursor);
    if (children.size() != 1) {
      return cursor;
    }
    cursor = children.front();
  }
}

bool is_supported_int_type(CXType type) {
  return type.kind == CXType_Int;
}

struct ArrayInfo {
  int64_t base_addr{0};
  int64_t size{0};
};

struct FunctionInfo {
  std::string name;
  CXCursor cursor{clang_getNullCursor()};
  std::vector<std::string> param_names;
  std::vector<int32_t> param_regs;
  size_t start_pc{0};
  bool is_main{false};
};

struct PendingCallPatch {
  size_t load_pc_insn_index{0};
  std::string caller_name;
  std::string callee_name;
};

struct GlobalLoweringState {
  static constexpr int64_t kMemorySize = 65536;

  const std::string& source;
  std::string diag_name;
  t81::tisc::Program program;
  std::unordered_map<std::string, FunctionInfo> functions;
  std::vector<std::string> function_order;
  std::vector<PendingCallPatch> call_patches;
  std::vector<std::pair<std::string, std::string>> call_edges;
  int32_t next_reg = 1;
  int64_t next_mem = 0;

  int32_t alloc_reg() {
    if (next_reg >= 242) {
      return -1;
    }
    return next_reg++;
  }

  int32_t ensure_target(int32_t preferred = -1) {
    if (preferred >= 0) {
      return preferred;
    }
    return alloc_reg();
  }

  int64_t alloc_mem(int64_t size) {
    if (size < 0 || next_mem + size > kMemorySize) {
      return -1;
    }
    const int64_t base = next_mem;
    next_mem += size;
    return base;
  }
};

struct LoweringContext {
  struct LoopControl {
    size_t continue_target_pc{0};
    std::vector<size_t> break_jumps;
    std::vector<size_t> continue_jumps;
  };

  GlobalLoweringState& global;
  FunctionInfo& function;
  std::unordered_map<std::string, int32_t> vars;
  std::unordered_map<std::string, ArrayInfo> arrays;
  std::vector<LoopControl> loop_stack;

  const std::string& source() const {
    return global.source;
  }

  size_t emit(t81::tisc::Opcode opcode, int32_t a = 0, int64_t b = 0, int32_t c = 0) {
    global.program.insns.push_back({opcode, a, b, c});
    return global.program.insns.size() - 1;
  }

  int32_t alloc_reg() {
    return global.alloc_reg();
  }

  int32_t ensure_target(int32_t preferred = -1) {
    return global.ensure_target(preferred);
  }

  size_t pc() const {
    return global.program.insns.size();
  }

  void patch_jump_target(size_t insn_index, size_t target_pc) {
    if (insn_index < global.program.insns.size()) {
      global.program.insns[insn_index].a = static_cast<int32_t>(target_pc);
    }
  }

  std::string error_prefix(CXCursor cursor) const {
    CXSourceLocation loc = clang_getCursorLocation(cursor);
    unsigned line = 0;
    unsigned column = 0;
    clang_getSpellingLocation(loc, nullptr, &line, &column, nullptr);
    std::ostringstream oss;
    oss << global.diag_name << ':' << line << ':' << column << ": ";
    return oss.str();
  }

  bool fail(CXCursor cursor, std::string_view message, std::string* error) const {
    if (error) {
      *error = error_prefix(cursor) + std::string(message);
    }
    return false;
  }
};

bool compile_expr(LoweringContext& ctx, CXCursor cursor, int32_t target, std::string* error);
bool compile_stmt(LoweringContext& ctx, CXCursor cursor, std::string* error);

bool compile_integer_literal_value(LoweringContext& ctx, CXCursor cursor, int64_t& value,
                                   std::string* error) {
  const std::string text = trim_copy(cursor_text(cursor, ctx.source()));
  char* end = nullptr;
  const long long parsed = std::strtoll(text.c_str(), &end, 0);
  if (!end || *end != '\0') {
    return ctx.fail(cursor, "unsupported integer literal", error);
  }
  value = parsed;
  return true;
}

bool compile_break_stmt(LoweringContext& ctx, CXCursor cursor, std::string* error) {
  if (ctx.loop_stack.empty()) {
    return ctx.fail(cursor, "'break' is only supported inside loops", error);
  }
  auto& loop = ctx.loop_stack.back();
  loop.break_jumps.push_back(ctx.emit(t81::tisc::Opcode::Jump, 0, 0, 0));
  return true;
}

bool compile_continue_stmt(LoweringContext& ctx, CXCursor cursor, std::string* error) {
  if (ctx.loop_stack.empty()) {
    return ctx.fail(cursor, "'continue' is only supported inside loops", error);
  }
  auto& loop = ctx.loop_stack.back();
  loop.continue_jumps.push_back(ctx.emit(t81::tisc::Opcode::Jump, 0, 0, 0));
  return true;
}

bool copy_if_needed(LoweringContext& ctx, int32_t target, int32_t value_reg, std::string* error) {
  if (target < 0 || value_reg < 0) {
    if (error) {
      *error = "internal register allocation failure";
    }
    return false;
  }
  if (target != value_reg) {
    ctx.emit(t81::tisc::Opcode::Mov, target, value_reg, 0);
  }
  return true;
}

bool compile_integer_literal(LoweringContext& ctx, CXCursor cursor, int32_t target,
                             std::string* error) {
  int64_t value = 0;
  if (!compile_integer_literal_value(ctx, cursor, value, error)) {
    return false;
  }
  ctx.emit(t81::tisc::Opcode::LoadImm, target, value, 0);
  return true;
}

std::optional<std::string> binary_operator_spelling(CXCursor cursor,
                                                    CXCursor lhs,
                                                    CXCursor rhs,
                                                    std::string_view source) {
  unsigned expr_begin = 0;
  unsigned expr_end = 0;
  unsigned lhs_end = 0;
  unsigned rhs_begin = 0;
  if (!source_offsets(clang_getCursorExtent(cursor), expr_begin, expr_end)) {
    return std::nullopt;
  }
  unsigned ignored = 0;
  if (!source_offsets(clang_getCursorExtent(lhs), ignored, lhs_end)) {
    return std::nullopt;
  }
  if (!source_offsets(clang_getCursorExtent(rhs), rhs_begin, ignored)) {
    return std::nullopt;
  }
  if (lhs_end > source.size()) lhs_end = static_cast<unsigned>(source.size());
  if (rhs_begin > source.size()) rhs_begin = static_cast<unsigned>(source.size());
  if (rhs_begin < lhs_end) {
    return std::nullopt;
  }
  const std::string between = trim_copy(source.substr(lhs_end, rhs_begin - lhs_end));
  if (between.empty()) {
    return std::nullopt;
  }
  return between;
}

std::optional<std::string> unary_operator_spelling(CXCursor cursor,
                                                   CXCursor operand,
                                                   std::string_view source) {
  unsigned expr_begin = 0;
  unsigned expr_end = 0;
  unsigned operand_begin = 0;
  unsigned operand_end = 0;
  if (!source_offsets(clang_getCursorExtent(cursor), expr_begin, expr_end) ||
      !source_offsets(clang_getCursorExtent(operand), operand_begin, operand_end)) {
    return std::nullopt;
  }
  if (expr_begin > source.size()) expr_begin = static_cast<unsigned>(source.size());
  if (expr_end > source.size()) expr_end = static_cast<unsigned>(source.size());
  if (operand_begin > source.size()) operand_begin = static_cast<unsigned>(source.size());
  if (operand_end > source.size()) operand_end = static_cast<unsigned>(source.size());
  if (operand_begin < expr_begin || operand_end > expr_end || operand_end < operand_begin) {
    return std::nullopt;
  }
  const std::string prefix = trim_copy(source.substr(expr_begin, operand_begin - expr_begin));
  const std::string suffix = trim_copy(source.substr(operand_end, expr_end - operand_end));
  return trim_copy(prefix + suffix);
}

std::optional<std::pair<std::string, int64_t>> parse_array_subscript(LoweringContext& ctx,
                                                                     CXCursor cursor,
                                                                     std::string* error) {
  auto children = cursor_children(cursor);
  if (children.size() != 2) {
    ctx.fail(cursor, "unsupported array subscript shape", error);
    return std::nullopt;
  }
  CXCursor base = unwrap_expr(children[0]);
  if (clang_getCursorKind(base) != CXCursor_DeclRefExpr) {
    ctx.fail(base, "only direct local array indexing is supported", error);
    return std::nullopt;
  }
  const std::string name = to_string_and_dispose(clang_getCursorSpelling(base));
  const auto array_it = ctx.arrays.find(name);
  if (array_it == ctx.arrays.end()) {
    ctx.fail(base, "unknown array '" + name + "'", error);
    return std::nullopt;
  }
  int64_t index = 0;
  CXCursor index_cursor = unwrap_expr(children[1]);
  if (clang_getCursorKind(index_cursor) != CXCursor_IntegerLiteral ||
      !compile_integer_literal_value(ctx, index_cursor, index, error)) {
    ctx.fail(index_cursor, "only integer-literal array indices are supported", error);
    return std::nullopt;
  }
  if (index < 0 || index >= array_it->second.size) {
    ctx.fail(index_cursor, "array index is out of bounds for the declared fixed array", error);
    return std::nullopt;
  }
  return std::make_pair(name, index);
}

bool compile_unary_expr(LoweringContext& ctx, CXCursor cursor, int32_t target, std::string* error) {
  auto children = cursor_children(cursor);
  if (children.size() != 1) {
    return ctx.fail(cursor, "unsupported unary expression", error);
  }
  CXCursor operand = unwrap_expr(children.front());
  const auto op = unary_operator_spelling(cursor, operand, ctx.source());
  if (!op.has_value()) {
    return ctx.fail(cursor, "unsupported unary expression", error);
  }
  if (*op == "&") {
    return ctx.fail(cursor, "address-of is not supported in the C subset v0", error);
  }
  if (*op == "*") {
    return ctx.fail(cursor, "pointer dereference is not supported in the C subset v0", error);
  }
  if (*op == "!") {
    const int32_t operand_reg = ctx.alloc_reg();
    const int32_t zero_reg = ctx.alloc_reg();
    if (operand_reg < 0 || zero_reg < 0) {
      if (error) *error = "internal register allocation failure";
      return false;
    }
    if (!compile_expr(ctx, operand, operand_reg, error)) {
      return false;
    }
    ctx.emit(t81::tisc::Opcode::LoadImm, zero_reg, 0, 0);
    ctx.emit(t81::tisc::Opcode::Equal, target, operand_reg, zero_reg);
    return true;
  }
  if (*op != "-") {
    return ctx.fail(cursor, "only unary minus is supported", error);
  }
  const int32_t operand_reg = ctx.alloc_reg();
  const int32_t zero_reg = ctx.alloc_reg();
  if (operand_reg < 0 || zero_reg < 0) {
    if (error) *error = "internal register allocation failure";
    return false;
  }
  if (!compile_expr(ctx, operand, operand_reg, error)) {
    return false;
  }
  ctx.emit(t81::tisc::Opcode::LoadImm, zero_reg, 0, 0);
  ctx.emit(t81::tisc::Opcode::Sub, target, zero_reg, operand_reg);
  return true;
}

bool compile_binary_expr(LoweringContext& ctx, CXCursor cursor, int32_t target, std::string* error) {
  auto children = cursor_children(cursor);
  if (children.size() != 2) {
    return ctx.fail(cursor, "unsupported binary expression shape", error);
  }
  CXCursor lhs = unwrap_expr(children[0]);
  CXCursor rhs = unwrap_expr(children[1]);
  const auto op = binary_operator_spelling(cursor, lhs, rhs, ctx.source());
  if (!op.has_value()) {
    return ctx.fail(cursor, "unable to determine binary operator", error);
  }

  t81::tisc::Opcode opcode = t81::tisc::Opcode::Nop;
  if (*op == "&&") {
    const int32_t lhs_reg = ctx.alloc_reg();
    const int32_t rhs_reg = ctx.alloc_reg();
    if (lhs_reg < 0 || rhs_reg < 0) {
      if (error) *error = "internal register allocation failure";
      return false;
    }
    if (!compile_expr(ctx, lhs, lhs_reg, error)) {
      return false;
    }
    ctx.emit(t81::tisc::Opcode::LoadImm, target, 0, 0);
    const size_t lhs_false_jump = ctx.emit(t81::tisc::Opcode::JumpIfZero, 0, lhs_reg, 0);
    if (!compile_expr(ctx, rhs, rhs_reg, error)) {
      return false;
    }
    const size_t rhs_false_jump = ctx.emit(t81::tisc::Opcode::JumpIfZero, 0, rhs_reg, 0);
    ctx.emit(t81::tisc::Opcode::LoadImm, target, 1, 0);
    const size_t end_pc = ctx.pc();
    ctx.patch_jump_target(lhs_false_jump, end_pc);
    ctx.patch_jump_target(rhs_false_jump, end_pc);
    return true;
  }
  if (*op == "||") {
    const int32_t lhs_reg = ctx.alloc_reg();
    const int32_t rhs_reg = ctx.alloc_reg();
    if (lhs_reg < 0 || rhs_reg < 0) {
      if (error) *error = "internal register allocation failure";
      return false;
    }
    if (!compile_expr(ctx, lhs, lhs_reg, error)) {
      return false;
    }
    ctx.emit(t81::tisc::Opcode::LoadImm, target, 1, 0);
    const size_t lhs_true_jump = ctx.emit(t81::tisc::Opcode::JumpIfZero, 0, lhs_reg, 0);
    const size_t end_jump = ctx.emit(t81::tisc::Opcode::Jump, 0, 0, 0);
    const size_t rhs_start = ctx.pc();
    ctx.patch_jump_target(lhs_true_jump, rhs_start);
    if (!compile_expr(ctx, rhs, rhs_reg, error)) {
      return false;
    }
    ctx.emit(t81::tisc::Opcode::LoadImm, target, 0, 0);
    const size_t rhs_false_jump = ctx.emit(t81::tisc::Opcode::JumpIfZero, 0, rhs_reg, 0);
    ctx.emit(t81::tisc::Opcode::LoadImm, target, 1, 0);
    const size_t end_pc = ctx.pc();
    ctx.patch_jump_target(rhs_false_jump, end_pc);
    ctx.patch_jump_target(end_jump, end_pc);
    return true;
  }
  if (*op == "+") {
    opcode = t81::tisc::Opcode::Add;
  } else if (*op == "-") {
    opcode = t81::tisc::Opcode::Sub;
  } else if (*op == "*") {
    opcode = t81::tisc::Opcode::Mul;
  } else if (*op == "/") {
    opcode = t81::tisc::Opcode::Div;
  } else if (*op == "%") {
    opcode = t81::tisc::Opcode::Mod;
  } else if (*op == "==") {
    opcode = t81::tisc::Opcode::Equal;
  } else if (*op == "!=") {
    opcode = t81::tisc::Opcode::NotEqual;
  } else if (*op == "<") {
    opcode = t81::tisc::Opcode::Less;
  } else if (*op == "<=") {
    opcode = t81::tisc::Opcode::LessEqual;
  } else if (*op == ">") {
    opcode = t81::tisc::Opcode::Greater;
  } else if (*op == ">=") {
    opcode = t81::tisc::Opcode::GreaterEqual;
  } else if (*op == "&") {
    opcode = t81::tisc::Opcode::BitAnd;
  } else if (*op == "|") {
    opcode = t81::tisc::Opcode::BitOr;
  } else if (*op == "^") {
    opcode = t81::tisc::Opcode::BitXor;
  } else if (*op == "<<") {
    opcode = t81::tisc::Opcode::BitShl;
  } else if (*op == ">>") {
    opcode = t81::tisc::Opcode::BitShr;
  } else {
    return ctx.fail(cursor, "only arithmetic and comparison operators are supported", error);
  }

  const int32_t lhs_reg = ctx.alloc_reg();
  const int32_t rhs_reg = ctx.alloc_reg();
  if (lhs_reg < 0 || rhs_reg < 0) {
    if (error) *error = "internal register allocation failure";
    return false;
  }
  if (!compile_expr(ctx, lhs, lhs_reg, error) || !compile_expr(ctx, rhs, rhs_reg, error)) {
    return false;
  }
  ctx.emit(opcode, target, lhs_reg, rhs_reg);
  return true;
}

bool compile_expr(LoweringContext& ctx, CXCursor cursor, int32_t target, std::string* error) {
  cursor = unwrap_expr(cursor);
  const CXCursorKind kind = clang_getCursorKind(cursor);
  target = ctx.ensure_target(target);
  if (target < 0) {
    if (error) *error = "internal register allocation failure";
    return false;
  }

  switch (kind) {
    case CXCursor_IntegerLiteral:
      return compile_integer_literal(ctx, cursor, target, error);
    case CXCursor_UnaryExpr:
      return ctx.fail(cursor, "'sizeof' is not supported in the C subset v0", error);
    case CXCursor_CStyleCastExpr:
      return ctx.fail(cursor, "casts are not supported in the C subset v0", error);
    case CXCursor_MemberRefExpr:
      return ctx.fail(cursor, "member access is not supported in the C subset v0", error);
    case CXCursor_ArraySubscriptExpr: {
      const auto parsed = parse_array_subscript(ctx, cursor, error);
      if (!parsed.has_value()) {
        return false;
      }
      const auto& [name, index] = *parsed;
      const auto array_it = ctx.arrays.find(name);
      if (array_it == ctx.arrays.end()) {
        return ctx.fail(cursor, "unknown array '" + name + "'", error);
      }
      ctx.emit(t81::tisc::Opcode::Load, target, array_it->second.base_addr + index, 0);
      return true;
    }
    case CXCursor_ConditionalOperator:
      return ctx.fail(cursor, "ternary conditionals are not supported in the C subset v0", error);
    case CXCursor_CompoundAssignOperator:
      return ctx.fail(cursor, "compound assignment is not supported in the C subset v0", error);
    case CXCursor_DeclRefExpr: {
      const std::string name = to_string_and_dispose(clang_getCursorSpelling(cursor));
      const auto it = ctx.vars.find(name);
      if (it == ctx.vars.end()) {
        return ctx.fail(cursor, "unknown variable '" + name + "'", error);
      }
      return copy_if_needed(ctx, target, it->second, error);
    }
    case CXCursor_CallExpr: {
      auto children = cursor_children(cursor);
      if (children.empty()) {
        return ctx.fail(cursor, "unsupported call expression shape", error);
      }
      CXCursor callee_cursor = unwrap_expr(children.front());
      if (clang_getCursorKind(callee_cursor) != CXCursor_DeclRefExpr) {
        return ctx.fail(callee_cursor, "only direct same-translation-unit calls are supported",
                        error);
      }
      const std::string callee_name = to_string_and_dispose(clang_getCursorSpelling(callee_cursor));
      if (callee_name == "main") {
        return ctx.fail(callee_cursor, "calling 'main' is not supported in the C subset v0",
                        error);
      }
      const auto func_it = ctx.global.functions.find(callee_name);
      if (func_it == ctx.global.functions.end()) {
        return ctx.fail(callee_cursor, "unknown function '" + callee_name + "'", error);
      }
      const FunctionInfo& callee = func_it->second;
      const size_t arg_count = children.size() - 1;
      if (arg_count != callee.param_regs.size()) {
        return ctx.fail(cursor, "call argument count does not match function signature", error);
      }
      for (size_t i = 0; i < arg_count; ++i) {
        if (!compile_expr(ctx, children[i + 1], callee.param_regs[i], error)) {
          return false;
        }
      }
      const int32_t target_reg = ctx.alloc_reg();
      if (target_reg < 0) {
        if (error) *error = "internal register allocation failure";
        return false;
      }
      const size_t load_pc_insn_index = ctx.emit(t81::tisc::Opcode::LoadImm, target_reg, 0, 0);
      ctx.global.call_patches.push_back({load_pc_insn_index, ctx.function.name, callee_name});
      ctx.global.call_edges.emplace_back(ctx.function.name, callee_name);
      ctx.emit(t81::tisc::Opcode::Call, 0, target_reg, 0);
      return copy_if_needed(ctx, target, 0, error);
    }
    case CXCursor_UnaryOperator:
      return compile_unary_expr(ctx, cursor, target, error);
    case CXCursor_BinaryOperator:
      return compile_binary_expr(ctx, cursor, target, error);
    default:
      return ctx.fail(cursor, "unsupported expression in C subset", error);
  }
}

bool compile_var_decl(LoweringContext& ctx, CXCursor cursor, std::string* error) {
  const CXType type = clang_getCursorType(cursor);
  if (type.kind == CXType_ConstantArray) {
    const CXType elem_type = clang_getArrayElementType(type);
    if (!is_supported_int_type(elem_type)) {
      return ctx.fail(cursor, "only fixed local 'int' arrays are supported", error);
    }
    const std::string name = to_string_and_dispose(clang_getCursorSpelling(cursor));
    if (name.empty()) {
      return ctx.fail(cursor, "unnamed local arrays are not supported", error);
    }
    if (ctx.vars.find(name) != ctx.vars.end() || ctx.arrays.find(name) != ctx.arrays.end()) {
      return ctx.fail(cursor, "duplicate local name '" + name + "'", error);
    }
    const long long array_size = clang_getArraySize(type);
    if (array_size <= 0) {
      return ctx.fail(cursor, "fixed local arrays must have a positive constant size", error);
    }
    const int64_t base_addr = ctx.global.alloc_mem(array_size);
    if (base_addr < 0) {
      return ctx.fail(cursor, "fixed local arrays exceed the available T81 memory surface", error);
    }
    const int32_t value_reg = ctx.alloc_reg();
    if (value_reg < 0) {
      if (error) *error = "internal register allocation failure";
      return false;
    }
    for (long long i = 0; i < array_size; ++i) {
      ctx.emit(t81::tisc::Opcode::LoadImm, value_reg, 0, 0);
      ctx.emit(t81::tisc::Opcode::Store, static_cast<int32_t>(base_addr + i), value_reg, 0);
    }
    auto children = cursor_children(cursor);
    CXCursor init = clang_getNullCursor();
    for (CXCursor child : children) {
      const CXCursorKind child_kind = clang_getCursorKind(child);
      if (child_kind == CXCursor_IntegerLiteral) {
        continue;
      }
      if (child_kind == CXCursor_InitListExpr && clang_Cursor_isNull(init)) {
        init = child;
        continue;
      }
      return ctx.fail(child, "fixed local arrays may only use an integer initializer list", error);
    }
    if (!clang_Cursor_isNull(init)) {
      if (clang_getCursorKind(init) != CXCursor_InitListExpr) {
        return ctx.fail(init, "fixed local arrays must use an integer initializer list", error);
      }
      auto init_values = cursor_children(init);
      long long init_index = 0;
      for (CXCursor init_value : init_values) {
        if (init_index >= array_size) {
          return ctx.fail(init_value, "initializer list exceeds the declared fixed array size", error);
        }
        int64_t literal = 0;
        CXCursor literal_cursor = unwrap_expr(init_value);
        if (clang_getCursorKind(literal_cursor) != CXCursor_IntegerLiteral ||
            !compile_integer_literal_value(ctx, literal_cursor, literal, error)) {
          return ctx.fail(init_value,
                          "fixed local array initializers must be integer literals", error);
        }
        ctx.emit(t81::tisc::Opcode::LoadImm, value_reg, literal, 0);
        ctx.emit(t81::tisc::Opcode::Store, static_cast<int32_t>(base_addr + init_index), value_reg,
                 0);
        ++init_index;
      }
    }
    ctx.arrays.emplace(name, ArrayInfo{base_addr, array_size});
    return true;
  }
  if (type.kind == CXType_Pointer) {
    return ctx.fail(cursor, "pointers are not supported in the C subset v0", error);
  }
  if (!is_supported_int_type(type)) {
    return ctx.fail(cursor, "only 'int' local variables are supported", error);
  }
  const std::string name = to_string_and_dispose(clang_getCursorSpelling(cursor));
  if (name.empty()) {
    return ctx.fail(cursor, "unnamed local variables are not supported", error);
  }
  if (ctx.vars.find(name) != ctx.vars.end() || ctx.arrays.find(name) != ctx.arrays.end()) {
    return ctx.fail(cursor, "duplicate local name '" + name + "'", error);
  }
  auto children = cursor_children(cursor);
  if (children.size() != 1) {
    return ctx.fail(cursor, "local variables must have an initializer", error);
  }
  const int32_t reg = ctx.alloc_reg();
  if (reg < 0) {
    if (error) *error = "internal register allocation failure";
    return false;
  }
  if (!compile_expr(ctx, children.front(), reg, error)) {
    return false;
  }
  ctx.vars.emplace(name, reg);
  return true;
}

bool compile_assignment_stmt(LoweringContext& ctx, CXCursor cursor, std::string* error) {
  auto children = cursor_children(cursor);
  if (children.size() != 2) {
    return ctx.fail(cursor, "unsupported assignment shape", error);
  }
  CXCursor lhs = unwrap_expr(children[0]);
  CXCursor rhs = unwrap_expr(children[1]);
  const auto op = binary_operator_spelling(cursor, lhs, rhs, ctx.source());
  if (!op.has_value() || *op != "=") {
    return ctx.fail(cursor, "only simple '=' assignment is supported", error);
  }
  if (clang_getCursorKind(lhs) == CXCursor_ArraySubscriptExpr) {
    const auto parsed = parse_array_subscript(ctx, lhs, error);
    if (!parsed.has_value()) {
      return false;
    }
    const auto& [name, index] = *parsed;
    const auto array_it = ctx.arrays.find(name);
    if (array_it == ctx.arrays.end()) {
      return ctx.fail(lhs, "unknown array '" + name + "'", error);
    }
    const int32_t value_reg = ctx.alloc_reg();
    if (value_reg < 0) {
      if (error) *error = "internal register allocation failure";
      return false;
    }
    if (!compile_expr(ctx, rhs, value_reg, error)) {
      return false;
    }
    ctx.emit(t81::tisc::Opcode::Store, static_cast<int32_t>(array_it->second.base_addr + index),
             value_reg, 0);
    return true;
  }
  if (clang_getCursorKind(lhs) != CXCursor_DeclRefExpr) {
    return ctx.fail(lhs, "assignment target must be a local variable or fixed array element", error);
  }
  const std::string name = to_string_and_dispose(clang_getCursorSpelling(lhs));
  const auto it = ctx.vars.find(name);
  if (it == ctx.vars.end()) {
    return ctx.fail(lhs, "unknown variable '" + name + "'", error);
  }
  return compile_expr(ctx, rhs, it->second, error);
}

bool compile_return_stmt(LoweringContext& ctx, CXCursor cursor, std::string* error) {
  auto children = cursor_children(cursor);
  if (children.size() != 1) {
    return ctx.fail(cursor, "return statement must return an expression", error);
  }
  if (!compile_expr(ctx, children.front(), 0, error)) {
    return false;
  }
  ctx.emit(ctx.function.is_main ? t81::tisc::Opcode::Halt : t81::tisc::Opcode::Ret, 0, 0, 0);
  return true;
}

bool compile_decl_stmt(LoweringContext& ctx, CXCursor cursor, std::string* error) {
  for (CXCursor decl_child : cursor_children(cursor)) {
    if (clang_getCursorKind(decl_child) != CXCursor_VarDecl) {
      return ctx.fail(decl_child, "only local variable declarations are supported", error);
    }
    if (!compile_var_decl(ctx, decl_child, error)) {
      return false;
    }
  }
  return true;
}

bool compile_expr_stmt(LoweringContext& ctx, CXCursor cursor, std::string* error) {
  CXCursor expr = cursor;
  const CXCursorKind cursor_kind = clang_getCursorKind(cursor);
  if (cursor_kind != CXCursor_BinaryOperator && cursor_kind != CXCursor_UnaryOperator) {
    auto children = cursor_children(cursor);
    if (children.size() != 1) {
      return ctx.fail(cursor, "unsupported expression statement", error);
    }
    expr = unwrap_expr(children.front());
  }
  if (clang_getCursorKind(expr) == CXCursor_BinaryOperator) {
    auto binary_children = cursor_children(expr);
    if (binary_children.size() == 2) {
      const auto op = binary_operator_spelling(expr, unwrap_expr(binary_children[0]),
                                               unwrap_expr(binary_children[1]), ctx.source());
      if (op.has_value() && *op == "=") {
        return compile_assignment_stmt(ctx, expr, error);
      }
    }
  }
  if (clang_getCursorKind(expr) == CXCursor_UnaryOperator) {
    auto unary_children = cursor_children(expr);
    if (unary_children.size() != 1) {
      return ctx.fail(expr, "unsupported unary expression statement", error);
    }
    CXCursor operand = unwrap_expr(unary_children.front());
    if (clang_getCursorKind(operand) != CXCursor_DeclRefExpr) {
      return ctx.fail(operand, "increment/decrement target must be a local variable", error);
    }
    const auto op = unary_operator_spelling(expr, operand, ctx.source());
    if (!op.has_value() || (*op != "++" && *op != "--")) {
      return ctx.fail(expr,
                      "only assignment and increment/decrement expression statements are supported",
                      error);
    }
    const std::string name = to_string_and_dispose(clang_getCursorSpelling(operand));
    const auto it = ctx.vars.find(name);
    if (it == ctx.vars.end()) {
      return ctx.fail(operand, "unknown variable '" + name + "'", error);
    }
    const int32_t delta_reg = ctx.alloc_reg();
    if (delta_reg < 0) {
      if (error) *error = "internal register allocation failure";
      return false;
    }
    ctx.emit(t81::tisc::Opcode::LoadImm, delta_reg, 1, 0);
    ctx.emit(*op == "++" ? t81::tisc::Opcode::Add : t81::tisc::Opcode::Sub, it->second, it->second,
             delta_reg);
    return true;
  }
  return ctx.fail(expr, "only assignment expression statements are supported", error);
}

bool compile_block_stmt(LoweringContext& ctx, CXCursor cursor, std::string* error) {
  for (CXCursor stmt : cursor_children(cursor)) {
    if (!compile_stmt(ctx, stmt, error)) {
      return false;
    }
  }
  return true;
}

bool compile_if_stmt(LoweringContext& ctx, CXCursor cursor, std::string* error) {
  auto children = cursor_children(cursor);
  if (children.size() < 2 || children.size() > 3) {
    return ctx.fail(cursor, "unsupported if statement shape", error);
  }
  const int32_t cond_reg = ctx.alloc_reg();
  if (cond_reg < 0) {
    if (error) *error = "internal register allocation failure";
    return false;
  }
  if (!compile_expr(ctx, children[0], cond_reg, error)) {
    return false;
  }
  const size_t jump_to_else = ctx.emit(t81::tisc::Opcode::JumpIfZero, 0, cond_reg, 0);
  if (!compile_stmt(ctx, children[1], error)) {
    return false;
  }
  if (children.size() == 2) {
    ctx.patch_jump_target(jump_to_else, ctx.pc());
    return true;
  }
  const size_t jump_to_end = ctx.emit(t81::tisc::Opcode::Jump, 0, 0, 0);
  ctx.patch_jump_target(jump_to_else, ctx.pc());
  if (!compile_stmt(ctx, children[2], error)) {
    return false;
  }
  ctx.patch_jump_target(jump_to_end, ctx.pc());
  return true;
}

bool compile_while_stmt(LoweringContext& ctx, CXCursor cursor, std::string* error) {
  auto children = cursor_children(cursor);
  if (children.size() != 2) {
    return ctx.fail(cursor, "unsupported while statement shape", error);
  }
  const size_t loop_start = ctx.pc();
  ctx.loop_stack.push_back({loop_start, {}, {}});
  const int32_t cond_reg = ctx.alloc_reg();
  if (cond_reg < 0) {
    if (error) *error = "internal register allocation failure";
    ctx.loop_stack.pop_back();
    return false;
  }
  if (!compile_expr(ctx, children[0], cond_reg, error)) {
    ctx.loop_stack.pop_back();
    return false;
  }
  const size_t exit_jump = ctx.emit(t81::tisc::Opcode::JumpIfZero, 0, cond_reg, 0);
  if (!compile_stmt(ctx, children[1], error)) {
    ctx.loop_stack.pop_back();
    return false;
  }
  auto loop = std::move(ctx.loop_stack.back());
  ctx.loop_stack.pop_back();
  for (size_t continue_jump : loop.continue_jumps) {
    ctx.patch_jump_target(continue_jump, loop_start);
  }
  ctx.emit(t81::tisc::Opcode::Jump, static_cast<int32_t>(loop_start), 0, 0);
  const size_t end_pc = ctx.pc();
  ctx.patch_jump_target(exit_jump, end_pc);
  for (size_t break_jump : loop.break_jumps) {
    ctx.patch_jump_target(break_jump, end_pc);
  }
  return true;
}

bool compile_for_stmt(LoweringContext& ctx, CXCursor cursor, std::string* error) {
  auto children = cursor_children(cursor);
  if (children.size() < 2 || children.size() > 4) {
    return ctx.fail(cursor, "unsupported for statement shape", error);
  }

  auto is_assignment_stmt_like = [&](CXCursor child) {
    const CXCursorKind kind = clang_getCursorKind(child);
    if (kind == CXCursor_DeclStmt || kind == CXCursor_NullStmt) {
      return true;
    }
    CXCursor expr = child;
    if (kind != CXCursor_BinaryOperator) {
      auto stmt_children = cursor_children(child);
      if (stmt_children.size() != 1) {
        return false;
      }
      expr = unwrap_expr(stmt_children.front());
    }
    if (clang_getCursorKind(expr) == CXCursor_UnaryOperator) {
      auto unary_children = cursor_children(expr);
      if (unary_children.size() != 1) {
        return false;
      }
      const auto op = unary_operator_spelling(expr, unwrap_expr(unary_children.front()),
                                              ctx.source());
      return op.has_value() && (*op == "++" || *op == "--");
    }
    if (clang_getCursorKind(expr) != CXCursor_BinaryOperator) {
      return false;
    }
    auto binary_children = cursor_children(expr);
    if (binary_children.size() != 2) {
      return false;
    }
    const auto op = binary_operator_spelling(expr, unwrap_expr(binary_children[0]),
                                             unwrap_expr(binary_children[1]), ctx.source());
    return op.has_value() && *op == "=";
  };
  auto is_expr_like = [](CXCursor child) {
    const CXCursorKind kind = clang_getCursorKind(child);
    return kind == CXCursor_BinaryOperator || kind == CXCursor_UnaryOperator ||
           kind == CXCursor_DeclRefExpr || kind == CXCursor_IntegerLiteral ||
           kind == CXCursor_CallExpr || kind == CXCursor_UnexposedExpr ||
           kind == CXCursor_ParenExpr;
  };

  CXCursor init = clang_getNullCursor();
  CXCursor cond = clang_getNullCursor();
  CXCursor step = clang_getNullCursor();
  CXCursor body = children.back();

  if (children.size() == 4) {
    init = children[0];
    cond = children[1];
    step = children[2];
  } else if (children.size() == 3) {
    if (is_assignment_stmt_like(children[0]) && is_expr_like(children[1])) {
      init = children[0];
      cond = children[1];
    } else if (is_expr_like(children[0]) && is_assignment_stmt_like(children[1])) {
      cond = children[0];
      step = children[1];
    } else {
      return ctx.fail(cursor, "unsupported for statement shape", error);
    }
  } else {
    if (is_assignment_stmt_like(children[0])) {
      return ctx.fail(cursor, "for statements must include a condition in the C subset v0", error);
    }
    cond = children[0];
  }

  if (clang_Cursor_isNull(cond)) {
    return ctx.fail(cursor, "for statements must include a condition in the C subset v0", error);
  }

  if (!clang_Cursor_isNull(init) && !compile_stmt(ctx, init, error)) {
    return false;
  }

  const size_t loop_start = ctx.pc();
  ctx.loop_stack.push_back({loop_start, {}, {}});
  const int32_t cond_reg = ctx.alloc_reg();
  if (cond_reg < 0) {
    if (error) *error = "internal register allocation failure";
    ctx.loop_stack.pop_back();
    return false;
  }
  if (!compile_expr(ctx, cond, cond_reg, error)) {
    ctx.loop_stack.pop_back();
    return false;
  }
  const size_t exit_jump = ctx.emit(t81::tisc::Opcode::JumpIfZero, 0, cond_reg, 0);
  if (!compile_stmt(ctx, body, error)) {
    ctx.loop_stack.pop_back();
    return false;
  }
  auto loop = std::move(ctx.loop_stack.back());
  ctx.loop_stack.pop_back();
  const size_t continue_target_pc = clang_Cursor_isNull(step) ? loop_start : ctx.pc();
  for (size_t continue_jump : loop.continue_jumps) {
    ctx.patch_jump_target(continue_jump, continue_target_pc);
  }
  if (!clang_Cursor_isNull(step) && !compile_stmt(ctx, step, error)) {
    return false;
  }
  ctx.emit(t81::tisc::Opcode::Jump, static_cast<int32_t>(loop_start), 0, 0);
  const size_t end_pc = ctx.pc();
  ctx.patch_jump_target(exit_jump, end_pc);
  for (size_t break_jump : loop.break_jumps) {
    ctx.patch_jump_target(break_jump, end_pc);
  }
  return true;
}

bool compile_stmt(LoweringContext& ctx, CXCursor cursor, std::string* error) {
  switch (clang_getCursorKind(cursor)) {
    case CXCursor_DeclStmt:
      return compile_decl_stmt(ctx, cursor, error);
    case CXCursor_ReturnStmt:
      return compile_return_stmt(ctx, cursor, error);
    case CXCursor_CompoundStmt:
      return compile_block_stmt(ctx, cursor, error);
    case CXCursor_IfStmt:
      return compile_if_stmt(ctx, cursor, error);
    case CXCursor_WhileStmt:
      return compile_while_stmt(ctx, cursor, error);
    case CXCursor_ForStmt:
      return compile_for_stmt(ctx, cursor, error);
    case CXCursor_SwitchStmt:
      return ctx.fail(cursor, "'switch' is not supported in the C subset v0", error);
    case CXCursor_DoStmt:
      return ctx.fail(cursor, "'do-while' is not supported in the C subset v0", error);
    case CXCursor_GotoStmt:
      return ctx.fail(cursor, "'goto' is not supported in the C subset v0", error);
    case CXCursor_LabelStmt:
      return ctx.fail(cursor, "labels are not supported in the C subset v0", error);
    case CXCursor_BreakStmt:
      return compile_break_stmt(ctx, cursor, error);
    case CXCursor_ContinueStmt:
      return compile_continue_stmt(ctx, cursor, error);
    case CXCursor_BinaryOperator:
    case CXCursor_UnaryOperator:
    case CXCursor_UnexposedStmt:
      return compile_expr_stmt(ctx, cursor, error);
    case CXCursor_NullStmt:
      return true;
    default:
      return ctx.fail(cursor, "unsupported statement in C subset v0", error);
  }
}

bool compile_function(GlobalLoweringState& global, FunctionInfo& function, std::string* error) {
  LoweringContext ctx{global, function, {}, {}, {}};
  CXCursor cursor = function.cursor;
  if (!is_supported_int_type(clang_getCursorResultType(cursor))) {
    return ctx.fail(cursor, "functions must return 'int' in the C subset v0", error);
  }
  if (function.is_main && clang_Cursor_getNumArguments(cursor) != 0) {
    return ctx.fail(cursor, "main may not accept parameters in the C subset v0", error);
  }
  for (size_t i = 0; i < function.param_names.size(); ++i) {
    ctx.vars.emplace(function.param_names[i], function.param_regs[i]);
  }

  auto children = cursor_children(cursor);
  CXCursor body = clang_getNullCursor();
  for (CXCursor child : children) {
    if (clang_getCursorKind(child) == CXCursor_CompoundStmt) {
      body = child;
      break;
    }
  }
  if (clang_Cursor_isNull(body)) {
    return ctx.fail(cursor, "functions must have a compound statement body", error);
  }
  function.start_pc = global.program.insns.size();
  if (!compile_block_stmt(ctx, body, error)) {
    return false;
  }
  const t81::tisc::Opcode expected =
      function.is_main ? t81::tisc::Opcode::Halt : t81::tisc::Opcode::Ret;
  if (global.program.insns.empty() || global.program.insns.back().opcode != expected) {
    return ctx.fail(body, "function must end in a reachable return statement", error);
  }
  return true;
}

bool collect_function_info(GlobalLoweringState& global, CXCursor cursor, std::string* error) {
  FunctionInfo info;
  info.name = to_string_and_dispose(clang_getCursorSpelling(cursor));
  info.cursor = cursor;
  info.is_main = (info.name == "main");
  if (info.name.empty()) {
    if (error) {
      *error = global.diag_name + ":1:1: unnamed functions are not supported in the C subset v0";
    }
    return false;
  }
  if (global.functions.find(info.name) != global.functions.end()) {
    LoweringContext dummy{global, info, {}, {}, {}};
    return dummy.fail(cursor, "duplicate function definition '" + info.name + "'", error);
  }
  std::unordered_set<std::string> seen_params;
  const int arg_count = clang_Cursor_getNumArguments(cursor);
  if (clang_Cursor_isVariadic(cursor) && arg_count > 0) {
    LoweringContext dummy{global, info, {}, {}, {}};
    return dummy.fail(cursor, "variadic functions are not supported in the C subset v0", error);
  }
  if (info.is_main && arg_count != 0) {
    LoweringContext dummy{global, info, {}, {}, {}};
    return dummy.fail(cursor, "'main' must have the exact signature 'int main()' in the C subset v0",
                      error);
  }
  for (int i = 0; i < arg_count; ++i) {
    CXCursor arg = clang_Cursor_getArgument(cursor, i);
    const CXType arg_type = clang_getCursorType(arg);
    if (arg_type.kind == CXType_Pointer) {
      LoweringContext dummy{global, info, {}, {}, {}};
      return dummy.fail(arg, "pointer parameters are not supported in the C subset v0", error);
    }
    if (arg_type.kind == CXType_ConstantArray) {
      LoweringContext dummy{global, info, {}, {}, {}};
      return dummy.fail(arg, "array parameters are not supported in the C subset v0", error);
    }
    if (!is_supported_int_type(arg_type)) {
      LoweringContext dummy{global, info, {}, {}, {}};
      return dummy.fail(arg, "only 'int' parameters are supported in the C subset v0", error);
    }
    const std::string param_name = to_string_and_dispose(clang_getCursorSpelling(arg));
    if (param_name.empty()) {
      LoweringContext dummy{global, info, {}, {}, {}};
      return dummy.fail(arg, "parameter names are required in the C subset v0", error);
    }
    if (!seen_params.emplace(param_name).second) {
      LoweringContext dummy{global, info, {}, {}, {}};
      return dummy.fail(arg, "duplicate parameter '" + param_name + "'", error);
    }
    const int32_t reg = global.alloc_reg();
    if (reg < 0) {
      if (error) *error = "internal register allocation failure";
      return false;
    }
    info.param_names.push_back(param_name);
    info.param_regs.push_back(reg);
  }
  global.function_order.push_back(info.name);
  global.functions.emplace(info.name, std::move(info));
  return true;
}

bool detect_recursive_calls(const GlobalLoweringState& global, std::string* error) {
  std::unordered_map<std::string, std::vector<std::string>> graph;
  for (const auto& edge : global.call_edges) {
    graph[edge.first].push_back(edge.second);
  }
  std::unordered_map<std::string, int> color;
  std::function<bool(const std::string&)> visit = [&](const std::string& name) -> bool {
    color[name] = 1;
    for (const std::string& next : graph[name]) {
      if (color[next] == 0) {
        if (!visit(next)) {
          return false;
        }
      } else if (color[next] == 1) {
        if (error) {
          *error = global.diag_name + ":1:1: recursive calls are not supported in the C subset v0";
        }
        return false;
      }
    }
    color[name] = 2;
    return true;
  };
  for (const auto& [name, _] : global.functions) {
    if (color[name] == 0 && !visit(name)) {
      return false;
    }
  }
  return true;
}

bool build_program_from_translation_unit(CXTranslationUnit tu,
                                         const std::string& source,
                                         const std::string& diag_name,
                                         t81::tisc::Program& program,
                                         std::string* error) {
  CXCursor root = clang_getTranslationUnitCursor(tu);
  auto children = cursor_children(root);
  GlobalLoweringState global{source, diag_name, {}, {}, {}, {}, {}, 1};
  for (CXCursor child : children) {
    const CXCursorKind kind = clang_getCursorKind(child);
    if (kind == CXCursor_FunctionDecl) {
      if (!clang_isCursorDefinition(child)) {
        FunctionInfo dummy_function;
        LoweringContext dummy{global, dummy_function, {}, {}, {}};
        return dummy.fail(child,
                          "function prototypes and extern declarations are not supported in the C subset v0",
                          error);
      }
      if (!collect_function_info(global, child, error)) {
        return false;
      }
      continue;
    }
    if (kind == CXCursor_VarDecl) {
      if (error) {
        FunctionInfo dummy_function;
        LoweringContext dummy{global, dummy_function, {}, {}, {}};
        return dummy.fail(child, "global variables are not supported in the C subset v0", error);
      }
      return false;
    }
  }

  if (global.functions.find("main") == global.functions.end()) {
    if (error) {
      std::ostringstream oss;
      oss << diag_name << ":1:1: expected a defined 'int main()' entry point in the C subset v0";
      *error = oss.str();
    }
    return false;
  }

  std::vector<std::string> compile_order;
  compile_order.push_back("main");
  for (const std::string& name : global.function_order) {
    if (name != "main") {
      compile_order.push_back(name);
    }
  }

  for (const std::string& name : compile_order) {
    auto it = global.functions.find(name);
    if (it == global.functions.end()) {
      continue;
    }
    if (!compile_function(global, it->second, error)) {
      return false;
    }
    global.program.function_metadata.push_back({name, false});
  }

  if (!detect_recursive_calls(global, error)) {
    return false;
  }

  for (const auto& patch : global.call_patches) {
    auto func_it = global.functions.find(patch.callee_name);
    if (func_it == global.functions.end()) {
      if (error) *error = diag_name + ":1:1: internal function patching failure";
      return false;
    }
    if (patch.load_pc_insn_index >= global.program.insns.size() ||
        global.program.insns[patch.load_pc_insn_index].opcode != t81::tisc::Opcode::LoadImm) {
      if (error) *error = diag_name + ":1:1: internal call target patching failure";
      return false;
    }
    global.program.insns[patch.load_pc_insn_index].b =
        static_cast<std::int64_t>(func_it->second.start_pc);
  }

  program = std::move(global.program);
  return true;
}

bool parse_and_lower_program(const std::string& source,
                             const std::string& diag_name,
                             t81::tisc::Program& program,
                             std::string* error) {
  CXIndex index = clang_createIndex(0, 0);
  const char* args[] = {"-xc", "-std=c11"};
  CXUnsavedFile unsaved;
  unsaved.Filename = diag_name.c_str();
  unsaved.Contents = source.c_str();
  unsaved.Length = source.size();
  CXTranslationUnit tu = clang_parseTranslationUnit(
      index,
      diag_name.c_str(),
      args,
      2,
      &unsaved,
      1,
      CXTranslationUnit_None);
  if (!tu) {
    if (error) {
      *error = diag_name + ":1:1: failed to parse C source via libclang";
    }
    clang_disposeIndex(index);
    return false;
  }

  const unsigned diag_count = clang_getNumDiagnostics(tu);
  for (unsigned i = 0; i < diag_count; ++i) {
    CXDiagnostic diag = clang_getDiagnostic(tu, i);
    const CXDiagnosticSeverity severity = clang_getDiagnosticSeverity(diag);
    if (severity == CXDiagnostic_Error || severity == CXDiagnostic_Fatal) {
      if (error) {
        *error = to_string_and_dispose(clang_formatDiagnostic(
            diag, clang_defaultDiagnosticDisplayOptions()));
      }
      clang_disposeDiagnostic(diag);
      clang_disposeTranslationUnit(tu);
      clang_disposeIndex(index);
      return false;
    }
    clang_disposeDiagnostic(diag);
  }

  const bool ok = build_program_from_translation_unit(tu, source, diag_name, program, error);
  clang_disposeTranslationUnit(tu);
  clang_disposeIndex(index);
  return ok;
}

}  // namespace

bool compile_source_to_mlir_text(const std::string& source,
                                 const std::string& diag_name,
                                 std::string& output,
                                 const CompileOptions& options,
                                 std::string* error_message) {
  t81::tisc::Program program;
  if (!parse_and_lower_program(source, diag_name, program, error_message)) {
    return false;
  }

  mlir::MLIRContext context;
  t81::mlir_frontend::TranslationConfig cfg;
  cfg.module_name = options.module_name.empty() ? fs::path(diag_name).stem().string() : options.module_name;
  cfg.float_mode = options.dcp_floats ? t81::mlir_frontend::FloatMode::DCP
                                      : t81::mlir_frontend::FloatMode::Compat;
  cfg.use_t81_dialect = options.use_t81_dialect;
  cfg.emit_comments = options.emit_comments;

  auto module = t81::mlir_frontend::translate(program, context, cfg);
  if (!module) {
    if (error_message) {
      *error_message = diag_name + ":1:1: failed to translate accepted C subset to MLIR";
    }
    return false;
  }

  llvm::raw_string_ostream os(output);
  module->print(os);
  os.flush();
  return true;
}

bool compile_file_to_mlir(const fs::path& input,
                          const fs::path& output_path,
                          const CompileOptions& options,
                          std::string* error_message) {
  std::ifstream in(input, std::ios::binary);
  if (!in) {
    if (error_message) {
      *error_message = "Could not open file: " + input.string();
    }
    return false;
  }
  const std::string source((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::string output;
  if (!compile_source_to_mlir_text(source, input.string(), output, options, error_message)) {
    return false;
  }

  std::ofstream out(output_path, std::ios::binary | std::ios::trunc);
  if (!out) {
    if (error_message) {
      *error_message = "Could not write file: " + output_path.string();
    }
    return false;
  }
  out << output;
  if (!out.good()) {
    if (error_message) {
      *error_message = "Failed writing file: " + output_path.string();
    }
    return false;
  }
  return true;
}

}  // namespace t81::c_frontend

#else

namespace t81::c_frontend {

bool compile_source_to_mlir_text(const std::string&,
                                 const std::string&,
                                 std::string&,
                                 const CompileOptions&,
                                 std::string* error_message) {
  if (error_message) {
    *error_message = "t81 was built without the experimental C frontend";
  }
  return false;
}

bool compile_file_to_mlir(const std::filesystem::path&,
                          const std::filesystem::path&,
                          const CompileOptions&,
                          std::string* error_message) {
  if (error_message) {
    *error_message = "t81 was built without the experimental C frontend";
  }
  return false;
}

}  // namespace t81::c_frontend

#endif
