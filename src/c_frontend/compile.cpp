#include "t81/c_frontend/compile.hpp"

#ifdef T81_HAS_C_FRONTEND

#include <clang-c/Index.h>

#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
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
    if (kind != CXCursor_UnexposedExpr && kind != CXCursor_ParenExpr &&
        kind != CXCursor_CStyleCastExpr) {
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

struct LoweringContext {
  const std::string& source;
  std::string diag_name;
  t81::tisc::Program program;
  std::unordered_map<std::string, int32_t> vars;
  int32_t next_reg = 1;

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

  void emit(t81::tisc::Opcode opcode, int32_t a = 0, int64_t b = 0, int32_t c = 0) {
    program.insns.push_back({opcode, a, b, c});
  }

  std::string error_prefix(CXCursor cursor) const {
    CXSourceLocation loc = clang_getCursorLocation(cursor);
    unsigned line = 0;
    unsigned column = 0;
    clang_getSpellingLocation(loc, nullptr, &line, &column, nullptr);
    std::ostringstream oss;
    oss << diag_name << ':' << line << ':' << column << ": ";
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
  const std::string text = trim_copy(cursor_text(cursor, ctx.source));
  char* end = nullptr;
  const long long value = std::strtoll(text.c_str(), &end, 0);
  if (!end || *end != '\0') {
    return ctx.fail(cursor, "unsupported integer literal", error);
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

bool compile_unary_expr(LoweringContext& ctx, CXCursor cursor, int32_t target, std::string* error) {
  auto children = cursor_children(cursor);
  if (children.size() != 1) {
    return ctx.fail(cursor, "unsupported unary expression", error);
  }
  CXCursor operand = unwrap_expr(children.front());
  unsigned cursor_begin = 0;
  unsigned operand_begin = 0;
  unsigned ignored = 0;
  if (!source_offsets(clang_getCursorExtent(cursor), cursor_begin, ignored) ||
      !source_offsets(clang_getCursorExtent(operand), operand_begin, ignored)) {
    return ctx.fail(cursor, "unsupported unary expression", error);
  }
  if (operand_begin > ctx.source.size()) operand_begin = static_cast<unsigned>(ctx.source.size());
  if (cursor_begin > operand_begin) cursor_begin = operand_begin;
  const std::string prefix = trim_copy(
      std::string_view(ctx.source).substr(cursor_begin, operand_begin - cursor_begin));
  if (prefix != "-") {
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
  const auto op = binary_operator_spelling(cursor, lhs, rhs, ctx.source);
  if (!op.has_value()) {
    return ctx.fail(cursor, "unable to determine binary operator", error);
  }

  t81::tisc::Opcode opcode = t81::tisc::Opcode::Nop;
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
  } else {
    return ctx.fail(cursor, "only +, -, *, /, and % are supported", error);
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
    case CXCursor_DeclRefExpr: {
      const std::string name = to_string_and_dispose(clang_getCursorSpelling(cursor));
      const auto it = ctx.vars.find(name);
      if (it == ctx.vars.end()) {
        return ctx.fail(cursor, "unknown variable '" + name + "'", error);
      }
      return copy_if_needed(ctx, target, it->second, error);
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
  if (!is_supported_int_type(clang_getCursorType(cursor))) {
    return ctx.fail(cursor, "only 'int' local variables are supported", error);
  }
  const std::string name = to_string_and_dispose(clang_getCursorSpelling(cursor));
  if (name.empty()) {
    return ctx.fail(cursor, "unnamed local variables are not supported", error);
  }
  if (ctx.vars.find(name) != ctx.vars.end()) {
    return ctx.fail(cursor, "duplicate local variable '" + name + "'", error);
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

bool compile_return_stmt(LoweringContext& ctx, CXCursor cursor, std::string* error) {
  auto children = cursor_children(cursor);
  if (children.size() != 1) {
    return ctx.fail(cursor, "return statement must return an expression", error);
  }
  if (!compile_expr(ctx, children.front(), 0, error)) {
    return false;
  }
  ctx.emit(t81::tisc::Opcode::Halt, 0, 0, 0);
  return true;
}

bool compile_main_function(LoweringContext& ctx, CXCursor cursor, std::string* error) {
  const std::string name = to_string_and_dispose(clang_getCursorSpelling(cursor));
  if (name != "main") {
    return ctx.fail(cursor, "only 'int main()' is supported in the C subset v0", error);
  }
  if (!is_supported_int_type(clang_getCursorResultType(cursor))) {
    return ctx.fail(cursor, "main must return 'int'", error);
  }
  if (clang_Cursor_getNumArguments(cursor) != 0) {
    return ctx.fail(cursor, "function parameters are not supported in the C subset v0", error);
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
    return ctx.fail(cursor, "main must have a compound statement body", error);
  }

  auto statements = cursor_children(body);
  if (statements.empty()) {
    return ctx.fail(body, "main must contain a return statement", error);
  }

  for (size_t i = 0; i < statements.size(); ++i) {
    const CXCursor stmt = statements[i];
    switch (clang_getCursorKind(stmt)) {
      case CXCursor_DeclStmt: {
        for (CXCursor decl_child : cursor_children(stmt)) {
          if (clang_getCursorKind(decl_child) != CXCursor_VarDecl) {
            return ctx.fail(decl_child, "only local variable declarations are supported", error);
          }
          if (!compile_var_decl(ctx, decl_child, error)) {
            return false;
          }
        }
        break;
      }
      case CXCursor_ReturnStmt:
        if (i + 1 != statements.size()) {
          return ctx.fail(stmt, "return must be the final statement in main", error);
        }
        return compile_return_stmt(ctx, stmt, error);
      default:
        return ctx.fail(stmt, "unsupported statement in C subset v0", error);
    }
  }

  return ctx.fail(body, "main must end with a return statement", error);
}

bool build_program_from_translation_unit(CXTranslationUnit tu,
                                         const std::string& source,
                                         const std::string& diag_name,
                                         t81::tisc::Program& program,
                                         std::string* error) {
  CXCursor root = clang_getTranslationUnitCursor(tu);
  auto children = cursor_children(root);
  std::vector<CXCursor> functions;
  for (CXCursor child : children) {
    const CXCursorKind kind = clang_getCursorKind(child);
    if (kind == CXCursor_FunctionDecl && clang_isCursorDefinition(child)) {
      functions.push_back(child);
      continue;
    }
    if (kind == CXCursor_VarDecl) {
      if (error) {
        LoweringContext dummy{source, diag_name, {}, {}, 1};
        return dummy.fail(child, "global variables are not supported in the C subset v0", error);
      }
      return false;
    }
  }

  if (functions.size() != 1) {
    if (error) {
      std::ostringstream oss;
      oss << diag_name << ":1:1: expected exactly one defined function in the C subset v0";
      *error = oss.str();
    }
    return false;
  }

  LoweringContext ctx{source, diag_name, {}, {}, 1};
  if (!compile_main_function(ctx, functions.front(), error)) {
    return false;
  }
  program = std::move(ctx.program);
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
