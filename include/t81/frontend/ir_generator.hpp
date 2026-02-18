// include/t81/frontend/ir_generator.hpp
#ifndef T81_FRONTEND_IR_GENERATOR_HPP
#define T81_FRONTEND_IR_GENERATOR_HPP

#include <any>
#include <iostream>
#include <limits>
#include <locale>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeinfo>
#include <unordered_map>
#include "t81/enum_meta.hpp"
#include "t81/frontend/ast.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/frontend/symbol_table.hpp"
#include "t81/tensor.hpp"
#include "t81/tisc/ir.hpp"

namespace t81::frontend {

inline int hex_digit(char value) {
  if (value >= '0' && value <= '9') return value - '0';
  if (value >= 'a' && value <= 'f') return 10 + (value - 'a');
  if (value >= 'A' && value <= 'F') return 10 + (value - 'A');
  return -1;
}

inline std::string decode_string_literal(const Token& token) {
  std::string_view view = token.lexeme;
  if (view.size() < 2 || view.front() != '"' || view.back() != '"') {
    return {};
  }
  std::string result;
  result.reserve(view.size() - 2);
  for (size_t i = 1; i + 1 < view.size(); ++i) {
    char c = view[i];
    if (c == '\\' && i + 1 < view.size() - 1) {
      ++i;
      char esc = view[i];
      switch (esc) {
        case '\\':
          result.push_back('\\');
          break;
        case '"':
          result.push_back('"');
          break;
        case 'n':
          result.push_back('\n');
          break;
        case 'r':
          result.push_back('\r');
          break;
        case 't':
          result.push_back('\t');
          break;
        case 'x': {
          if (i + 2 < view.size() - 1) {
            int hi = hex_digit(view[++i]);
            int lo = hex_digit(view[++i]);
            if (hi >= 0 && lo >= 0) {
              result.push_back(static_cast<char>((hi << 4) | lo));
            }
          }
          break;
        }
        default:
          result.push_back(esc);
          break;
      }
    } else {
      result.push_back(c);
    }
  }
  return result;
}

inline std::string escape_metadata_string(std::string_view input) {
  std::string out;
  out.reserve(input.size());
  for (char c : input) {
    if (c == '\\' || c == '"') {
      out.push_back('\\');
    }
    out.push_back(c);
  }
  return out;
}

inline std::string strip_t81_suffix(std::string_view literal) {
  std::string value(literal);
  constexpr std::string_view suffix = "t81";
  if (value.size() >= suffix.size() &&
      value.compare(value.size() - suffix.size(), suffix.size(), suffix) == 0) {
    value.erase(value.size() - suffix.size());
  }
  return value;
}

inline int64_t parse_base81_integer_literal(std::string_view literal) {
  std::string value = strip_t81_suffix(literal);
  return std::stoll(value);
}

inline double parse_base81_float_literal(std::string_view literal) {
  std::string value = strip_t81_suffix(literal);
  return std::stod(value);
}

inline std::optional<std::string> qualified_call_name(const Expr& expr) {
  if (const auto* var = dynamic_cast<const VariableExpr*>(&expr)) {
    return std::string(var->name.lexeme);
  }
  if (const auto* field = dynamic_cast<const FieldAccessExpr*>(&expr)) {
    auto head = qualified_call_name(*field->object);
    if (!head.has_value()) {
      return std::nullopt;
    }
    return *head + "." + std::string(field->field.lexeme);
  }
  return std::nullopt;
}

inline std::string canonical_stdlib_call_name(std::string_view name) {
  if (name == "std.io.println" || name == "std.io.print_int" || name == "std.io.print_float") {
    return "print";
  }
  if (name == "std.math.sin") {
    return "sin";
  }
  if (name == "std.math.cos") {
    return "cos";
  }
  if (name == "std.math.tan") {
    return "tan";
  }
  if (name == "std.tensor.load") {
    return "weights.load";
  }
  if (name == "std.tensor.from_list") {
    return "Tensor.from_list";
  }
  if (name == "std.tensor.matmul") {
    return "Tensor.matmul";
  }
  if (name == "std.tensor.vec_add") {
    return "Tensor.vec_add";
  }
  if (name == "std.text.str_len") {
    return "str_len";
  }
  if (name == "std.text.str_is_empty") {
    return "str_is_empty";
  }
  if (name == "std.text.concat") {
    return "str_concat";
  }
  if (name == "std.text.starts_with") {
    return "str_starts_with";
  }
  if (name == "std.text.ends_with") {
    return "str_ends_with";
  }
  if (name == "std.text.contains") {
    return "str_contains";
  }
  if (name == "std.text.index_of") {
    return "str_index_of";
  }
  if (name == "std.text.replace") {
    return "str_replace";
  }
  if (name == "std.bytes.len") {
    return "bytes_len";
  }
  if (name == "std.bytes.is_empty") {
    return "bytes_is_empty";
  }
  if (name == "std.bytes.concat") {
    return "bytes_concat";
  }
  if (name == "std.bytes.starts_with") {
    return "bytes_starts_with";
  }
  if (name == "std.bytes.ends_with") {
    return "bytes_ends_with";
  }
  if (name == "std.bytes.contains") {
    return "bytes_contains";
  }
  if (name == "std.bytes.index_of") {
    return "bytes_index_of";
  }
  if (name == "std.bytes.replace") {
    return "bytes_replace";
  }
  return std::string(name);
}

class IRGenerator : public ExprVisitor, public StmtVisitor {
public:
  struct LoopInfo {
    int id = -1;
    tisc::ir::Label entry_label{};
    tisc::ir::Label exit_label{};
    int depth = 0;
    bool annotated = false;
  };

  tisc::ir::IntermediateProgram generate(const std::vector<std::unique_ptr<Stmt>>& statements) {
    // Pre-pass: Identify functions and assign labels
    std::vector<const FunctionStmt*> functions;
    for (const auto& stmt : statements) {
      if (auto func = dynamic_cast<const FunctionStmt*>(stmt.get())) {
        std::string fname = std::string(func->name.lexeme);
        _function_labels[fname] = new_label();
        functions.push_back(func);
      }
    }

    // Pass 1: Emit top-level statements (script body/initialization)
    for (const auto& stmt : statements) {
      if (!dynamic_cast<const FunctionStmt*>(stmt.get())) {
        stmt->accept(*this);
      }
    }

    // Emit startup code: CALL main if exists, then HALT
    auto main_it = _function_labels.find("main");
    if (main_it != _function_labels.end()) {
      tisc::ir::Instruction load;
      auto addr_reg = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      load.opcode = tisc::ir::Opcode::LOADI;
      load.operands = {addr_reg.reg, main_it->second};
      emit(load);

      tisc::ir::Instruction call;
      call.opcode = tisc::ir::Opcode::CALL;
      call.operands = {tisc::ir::Register{0}, addr_reg.reg};
      emit(call);

      // Pop main result (i32)
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction pop;
      pop.opcode = tisc::ir::Opcode::POP;
      pop.operands = {dest.reg};
      emit(pop);
    }

    emit_simple(tisc::ir::Opcode::HALT);

    // Pass 2: Emit functions
    for (const auto* func : functions) {
      func->accept(*this);
    }

    return std::move(_program);
  }

  const std::vector<LoopInfo>& loop_infos() const { return _loop_infos; }

  void attach_semantic_analyzer(const SemanticAnalyzer* analyzer) { _semantic = analyzer; }

  // Statements
  std::any visit(const ExpressionStmt& stmt) override {
    stmt.expression->accept(*this);
    return {};
  }

  std::any visit(const BlockStmt& stmt) override {
    for (const auto& s : stmt.statements) s->accept(*this);
    return {};
  }

  std::any visit(const VarStmt& stmt) override {
    bind_variable_from_initializer(stmt.name, stmt.initializer.get());
    return {};
  }
  std::any visit(const LetStmt& stmt) override {
    bind_variable_from_initializer(stmt.name, stmt.initializer.get());
    return {};
  }
  std::any visit(const IfStmt& stmt) override {
    auto end_label = new_label();

    stmt.condition->accept(*this);
    auto cond = ensure_expr_result(stmt.condition.get());

    if (stmt.else_branch) {
      auto else_label = new_label();
      emit_jump_if_zero(else_label, cond);
      stmt.then_branch->accept(*this);
      emit_jump(end_label);
      emit_label(else_label);
      stmt.else_branch->accept(*this);
    } else {
      emit_jump_if_zero(end_label, cond);
      stmt.then_branch->accept(*this);
    }
    emit_label(end_label);
    return {};
  }

  std::any visit(const WhileStmt& stmt) override {
    auto cond_label = new_label();
    auto end_label = new_label();

    LoopInfo info;
    info.entry_label = cond_label;
    info.exit_label = end_label;
    _loop_stack.push_back(info);

    emit_label(cond_label);
    stmt.condition->accept(*this);
    auto cond = ensure_expr_result(stmt.condition.get());
    emit_jump_if_zero(end_label, cond);

    stmt.body->accept(*this);
    emit_jump(cond_label);

    emit_label(end_label);
    _loop_stack.pop_back();
    return {};
  }
  std::any visit(const ForStmt& stmt) override {
    auto entry_label = new_label();
    auto exit_label = new_label();
    if (auto binary = dynamic_cast<const BinaryExpr*>(stmt.iterable.get())) {
      if (binary->op.type == TokenType::DotDot) {
        auto start = evaluate_expr(binary->left.get());
        auto end = evaluate_expr(binary->right.get());
        auto iterator_reg = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        bind_variable(std::string(stmt.iterator.lexeme), iterator_reg);
        copy_to_dest(start, iterator_reg);
        emit_label(entry_label);
        auto cond = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
        auto instr =
            tisc::ir::Instruction{tisc::ir::Opcode::CMP, {cond.reg, iterator_reg.reg, end.reg}};
        instr.primitive = tisc::ir::PrimitiveKind::Boolean;
        instr.boolean_result = true;
        instr.relation = tisc::ir::ComparisonRelation::Less;
        emit(instr);
        emit_jump_if_zero(exit_label, cond);
        LoopInfo info;
        info.entry_label = entry_label;
        info.exit_label = exit_label;
        _loop_stack.push_back(info);
        stmt.body->accept(*this);
        auto one = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        tisc::ir::Instruction load_one;
        load_one.opcode = tisc::ir::Opcode::LOADI;
        load_one.operands = {one.reg, tisc::ir::Immediate{1}};
        emit(load_one);
        tisc::ir::Instruction add_instr;
        add_instr.opcode = tisc::ir::Opcode::ADD;
        add_instr.operands = {iterator_reg.reg, iterator_reg.reg, one.reg};
        emit(add_instr);
        emit_jump(entry_label);
        emit_label(exit_label);
        _loop_stack.pop_back();
      }
    }
    return {};
  }

  std::any visit(const ReflectStmt& stmt) override {
    auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
    tisc::ir::Instruction instr;
    instr.opcode = tisc::ir::Opcode::META_REFLECT;
    instr.operands = {dest.reg};
    emit(instr);
    for (const auto& s : stmt.body) s->accept(*this);
    return {};
  }
  std::any visit(const LoopStmt& stmt) override {
    auto entry_label = new_label();
    auto exit_label = new_label();
    auto guard_label = entry_label;

    LoopInfo info;
    info.entry_label = entry_label;
    info.exit_label = exit_label;

    if (stmt.bound_kind == LoopStmt::BoundKind::Guarded && stmt.guard_expression) {
      guard_label = new_label();
      info.entry_label = guard_label;  // continue should go to guard
      emit_label(guard_label);
      stmt.guard_expression->accept(*this);
      auto guard_value = ensure_expr_result(stmt.guard_expression.get());
      emit_jump_if_zero(exit_label, guard_value);
      emit_label(entry_label);
    } else {
      emit_label(entry_label);
    }

    _loop_stack.push_back(info);
    for (const auto& statement : stmt.body) {
      statement->accept(*this);
    }
    emit_jump(guard_label);
    emit_label(exit_label);

    if (_semantic) {
      if (const auto* meta = _semantic->loop_metadata_for(stmt)) {
        info.id = meta->id;
        info.depth = meta->depth;
        info.annotated = meta->annotated();
      }
    }
    _loop_infos.push_back(info);
    _loop_stack.pop_back();
    return {};
  }
  std::any visit(const ReturnStmt& stmt) override {
    if (stmt.value) {
      stmt.value->accept(*this);
      auto value = ensure_expr_result(stmt.value.get());
      tisc::ir::Instruction push;
      push.opcode = tisc::ir::Opcode::PUSH;
      push.operands = {value.reg};
      emit(push);
    }
    if (auto ret_addr = lookup_variable("%ret_addr")) {
      tisc::ir::Instruction push_ret;
      push_ret.opcode = tisc::ir::Opcode::PUSH;
      push_ret.operands = {ret_addr->reg};
      emit(push_ret);
    }
    emit_simple(tisc::ir::Opcode::RET);
    return {};
  }
  std::any visit(const BreakStmt&) override {
    if (!_loop_stack.empty()) {
      emit_jump(_loop_stack.back().exit_label);
    }
    return {};
  }
  std::any visit(const ContinueStmt&) override {
    if (!_loop_stack.empty()) {
      emit_jump(_loop_stack.back().entry_label);
    }
    return {};
  }
  std::any visit(const FunctionStmt& stmt) override {
    std::string name{stmt.name.lexeme};
    emit_label(_function_labels[name]);

    enter_pattern_scope();

    // Pop return address
    auto ret_reg = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
    bind_variable("%ret_addr", ret_reg);
    tisc::ir::Instruction pop_ret;
    pop_ret.opcode = tisc::ir::Opcode::POP;
    pop_ret.operands = {ret_reg.reg};
    emit(pop_ret);

    // Pop arguments in reverse order
    for (auto it = stmt.params.rbegin(); it != stmt.params.rend(); ++it) {
      auto reg = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction pop;
      pop.opcode = tisc::ir::Opcode::POP;
      pop.operands = {reg.reg};
      emit(pop);
      bind_variable(std::string(it->name.lexeme), reg);
    }

    for (const auto& statement : stmt.body) {
      statement->accept(*this);
    }

    // Implicit return (void)
    // Must push ret_addr back before RET
    tisc::ir::Instruction push_ret;
    push_ret.opcode = tisc::ir::Opcode::PUSH;
    push_ret.operands = {ret_reg.reg};
    emit(push_ret);

    emit_simple(tisc::ir::Opcode::RET);

    exit_pattern_scope();
    return {};
  }
  std::any visit(const TypeDecl& stmt) override {
    if (!_semantic) return {};
    std::string name{stmt.name.lexeme};
    auto aliases = _semantic->type_aliases();
    auto it = aliases.find(name);
    if (it == aliases.end()) return {};
    tisc::ir::TypeAliasMetadata meta;
    meta.name = name;
    for (const auto& param : stmt.params) {
      meta.params.emplace_back(param.lexeme);
    }
    if (it->second.alias) {
      meta.alias = _semantic->type_expr_to_string(*it->second.alias);
    }
    _program.add_type_alias(std::move(meta));
    return {};
  }
  std::any visit(const RecordDecl& stmt) override {
    if (!_semantic) return {};
    std::string name{stmt.name.lexeme};
    auto record_it = _semantic->record_definitions().find(name);
    if (record_it == _semantic->record_definitions().end()) return {};
    tisc::ir::TypeAliasMetadata meta;
    meta.name = name;
    meta.kind = t81::tisc::StructuralKind::Record;
    meta.schema_version = record_it->second.schema_version;
    meta.module_path = record_it->second.module_path;
    for (const auto& field : record_it->second.fields) {
      t81::tisc::FieldInfo info;
      info.name = field.name;
      info.type = _semantic->type_to_string(field.type);
      meta.fields.push_back(std::move(info));
    }
    _program.add_type_alias(std::move(meta));
    return {};
  }

  std::any visit(const EnumDecl& stmt) override {
    if (!_semantic) return {};
    std::string name{stmt.name.lexeme};
    auto enum_it = _semantic->enum_definitions().find(name);
    if (enum_it == _semantic->enum_definitions().end()) return {};
    tisc::ir::TypeAliasMetadata meta;
    meta.name = name;
    meta.kind = t81::tisc::StructuralKind::Enum;
    meta.schema_version = enum_it->second.schema_version;
    meta.module_path = enum_it->second.module_path;
    for (const auto& variant_name : enum_it->second.variant_order) {
      t81::tisc::VariantInfo info;
      info.name = variant_name;
      auto payload_it = enum_it->second.variants.find(variant_name);
      if (payload_it != enum_it->second.variants.end() && payload_it->second.payload.has_value()) {
        info.payload = _semantic->type_to_string(*payload_it->second.payload);
      }
      meta.variants.push_back(std::move(info));
    }
    _program.add_type_alias(std::move(meta));
    return {};
  }

  // Expressions
  std::any visit(const BinaryExpr& expr) override {
    auto left = evaluate_expr(expr.left.get());
    auto right = evaluate_expr(expr.right.get());
    const Type* result_type = typed_expr(&expr);
    NumericCategory kind = categorize(result_type);
    tisc::ir::PrimitiveKind primitive_kind = categorize_primitive(result_type);
    if (primitive_kind == tisc::ir::PrimitiveKind::Unknown) {
      primitive_kind = tisc::ir::PrimitiveKind::Integer;
    }

    tisc::ir::ComparisonRelation relation = relation_from_token(expr.op.type);
    if (relation != tisc::ir::ComparisonRelation::None) {
      const Type* left_type = typed_expr(expr.left.get());
      const Type* right_type = typed_expr(expr.right.get());
      bool both_bool = left_type && right_type && left_type->kind == Type::Kind::Bool &&
                       right_type->kind == Type::Kind::Bool;

      NumericCategory left_cat = categorize(left_type);
      NumericCategory right_cat = categorize(right_type);
      NumericCategory target_category = NumericCategory::Unknown;
      auto merge_category = [](NumericCategory a, NumericCategory b) {
        if (a == NumericCategory::Float || b == NumericCategory::Float)
          return NumericCategory::Float;
        if (a == NumericCategory::Fraction || b == NumericCategory::Fraction)
          return NumericCategory::Fraction;
        if (a == NumericCategory::Integer || b == NumericCategory::Integer)
          return NumericCategory::Integer;
        return NumericCategory::Unknown;
      };
      target_category = merge_category(left_cat, right_cat);

      tisc::ir::PrimitiveKind operand_primitive;
      if (!both_bool) {
        switch (target_category) {
          case NumericCategory::Float:
            operand_primitive = tisc::ir::PrimitiveKind::Float;
            break;
          case NumericCategory::Fraction:
            operand_primitive = tisc::ir::PrimitiveKind::Fraction;
            break;
          case NumericCategory::Integer:
            operand_primitive = tisc::ir::PrimitiveKind::Integer;
            break;
          default:
            operand_primitive = left.primitive;
            break;
        }
      } else {
        operand_primitive = tisc::ir::PrimitiveKind::Integer;
      }

      auto left_converted = both_bool ? left : ensure_kind(left, operand_primitive);
      auto right_converted = both_bool ? right : ensure_kind(right, operand_primitive);
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);

      auto instr = tisc::ir::Instruction{tisc::ir::Opcode::CMP,
                                         {dest.reg, left_converted.reg, right_converted.reg}};
      instr.primitive = tisc::ir::PrimitiveKind::Boolean;
      instr.boolean_result = true;
      instr.relation = relation;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }

    if (expr.op.type == TokenType::Percent && primitive_kind != tisc::ir::PrimitiveKind::Integer) {
      throw std::runtime_error("Modulo requires integer operands");
    }

    auto left_converted = ensure_kind(left, primitive_kind);
    auto right_converted = ensure_kind(right, primitive_kind);
    auto dest = allocate_typed_register(primitive_kind);
    using O = tisc::ir::Opcode;
    tisc::ir::Opcode opcode;
    switch (expr.op.type) {
      case TokenType::Plus:
        opcode = select_opcode(kind, O::ADD, O::FADD, O::FRACADD);
        break;
      case TokenType::Minus:
        opcode = select_opcode(kind, O::SUB, O::FSUB, O::FRACSUB);
        break;
      case TokenType::Star:
        opcode = select_opcode(kind, O::MUL, O::FMUL, O::FRACMUL);
        break;
      case TokenType::Slash:
        opcode = select_opcode(kind, O::DIV, O::FDIV, O::FRACDIV);
        break;
      case TokenType::Percent:
        opcode = O::MOD;
        break;
      default:
        throw std::runtime_error("Unsupported binary operator");
    }

    auto instr = tisc::ir::Instruction{opcode, {dest.reg, left_converted.reg, right_converted.reg}};
    instr.primitive = primitive_kind;
    emit(instr);
    record_result(&expr, dest);
    return {};
  }

  std::any visit(const LiteralExpr& expr) override {
    if (expr.value.type == TokenType::String) {
      std::string contents = decode_string_literal(expr.value);
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::LOADI;
      instr.operands = {dest.reg};
      instr.literal_kind = tisc::LiteralKind::SymbolHandle;
      instr.text_literal = std::move(contents);
      instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (expr.value.type == TokenType::True || expr.value.type == TokenType::False) {
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      auto instr = tisc::ir::Instruction{
          tisc::ir::Opcode::LOADI,
          {dest.reg, tisc::ir::Immediate{expr.value.type == TokenType::True ? 1 : 0}}};
      instr.primitive = tisc::ir::PrimitiveKind::Boolean;
      instr.literal_kind = tisc::LiteralKind::Bool;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (expr.value.type == TokenType::Float || expr.value.type == TokenType::Base81Float) {
      const double parsed = (expr.value.type == TokenType::Base81Float)
                                ? parse_base81_float_literal(expr.value.lexeme)
                                : std::stod(std::string(expr.value.lexeme));
      std::ostringstream out;
      out.imbue(std::locale::classic());
      out.precision(std::numeric_limits<double>::max_digits10);
      out << parsed;

      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Float);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::LOADI;
      instr.operands = {dest.reg};
      instr.literal_kind = tisc::LiteralKind::FloatHandle;
      instr.text_literal = out.str();
      instr.primitive = tisc::ir::PrimitiveKind::Float;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }

    const int64_t value = (expr.value.type == TokenType::Base81Integer)
                              ? parse_base81_integer_literal(expr.value.lexeme)
                              : std::stoll(std::string(expr.value.lexeme));
    auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
    auto instr =
        tisc::ir::Instruction{tisc::ir::Opcode::LOADI, {dest.reg, tisc::ir::Immediate{value}}};
    instr.primitive = tisc::ir::PrimitiveKind::Integer;
    emit(instr);
    record_result(&expr, dest);
    return {};
  }

  std::any visit(const GroupingExpr& expr) override {
    auto value = evaluate_expr(expr.expression.get());
    record_result(&expr, value);
    return {};
  }

  std::any visit(const UnaryExpr& expr) override {
    auto right = evaluate_expr(expr.right.get());
    auto dest = allocate_typed_register(right.primitive);
    tisc::ir::Opcode opcode;
    if (expr.op.type == TokenType::Minus) {
      opcode = tisc::ir::Opcode::NEG;
    } else {
      throw std::runtime_error("Unsupported unary operator");
    }
    auto instr = tisc::ir::Instruction{opcode, {dest.reg, right.reg}};
    instr.primitive = right.primitive;
    emit(instr);
    record_result(&expr, dest);
    return {};
  }
  std::any visit(const VariableExpr& expr) override {
    auto found = lookup_variable(expr.name.lexeme);
    if (found.has_value()) {
      record_result(&expr, *found);
    }
    return {};
  }
  std::any visit(const CallExpr& expr) override {
    if (auto callee_name = qualified_call_name(*expr.callee); callee_name.has_value()) {
      const std::string raw_name = *callee_name;
      std::string func_name = canonical_stdlib_call_name(raw_name);
      if (func_name == "Some") {
        if (expr.arguments.empty()) {
          throw std::runtime_error("Some() requires a payload");
        }
        expr.arguments[0]->accept(*this);
        auto payload = ensure_expr_result(expr.arguments[0].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        emit_make_option_some(dest, payload);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "None") {
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        emit_make_option_none(dest);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "Ok") {
        if (expr.arguments.empty()) {
          throw std::runtime_error("Ok() requires a payload");
        }
        expr.arguments[0]->accept(*this);
        auto payload = ensure_expr_result(expr.arguments[0].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        emit_make_result_ok(dest, payload);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "Err") {
        if (expr.arguments.empty()) {
          throw std::runtime_error("Err() requires a payload");
        }
        expr.arguments[0]->accept(*this);
        auto payload = ensure_expr_result(expr.arguments[0].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        emit_make_result_err(dest, payload);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "weights.load" || func_name == "Tensor.load") {
        if (expr.arguments.size() != 1) {
          throw std::runtime_error("weights.load expects a single string argument.");
        }
        auto* literal = dynamic_cast<const LiteralExpr*>(expr.arguments[0].get());
        if (!literal) {
          throw std::runtime_error("weights.load requires a string literal argument.");
        }
        std::string name = decode_string_literal(literal->value);
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::WEIGHTS_LOAD;
        instr.operands = {dest.reg};
        instr.literal_kind = tisc::LiteralKind::SymbolHandle;
        instr.text_literal = std::move(name);
        emit(instr);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "Tensor.from_list") {
        if (expr.arguments.size() != 1) {
          throw std::runtime_error("Tensor.from_list expects a single argument.");
        }
        expr.arguments[0]->accept(*this);
        auto val = ensure_expr_result(expr.arguments[0].get());
        record_result(&expr, val);
        return {};
      }
      if (func_name == "Tensor.matmul") {
        if (expr.arguments.size() != 2) {
          throw std::runtime_error("Tensor.matmul expects two arguments.");
        }
        expr.arguments[0]->accept(*this);
        auto left = ensure_expr_result(expr.arguments[0].get());
        expr.arguments[1]->accept(*this);
        auto right = ensure_expr_result(expr.arguments[1].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::TMATMUL;
        instr.operands = {dest.reg, left.reg, right.reg};
        emit(instr);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "Tensor.vec_add") {
        if (expr.arguments.size() != 2) {
          throw std::runtime_error("Tensor.vec_add expects two arguments.");
        }
        expr.arguments[0]->accept(*this);
        auto left = ensure_expr_result(expr.arguments[0].get());
        expr.arguments[1]->accept(*this);
        auto right = ensure_expr_result(expr.arguments[1].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::TVECADD;
        instr.operands = {dest.reg, left.reg, right.reg};
        emit(instr);
        record_result(&expr, dest);
        return {};
      }

      auto dot_pos = func_name.find('.');
      if (dot_pos != std::string::npos) {
        std::string obj_name = func_name.substr(0, dot_pos);
        std::string method_name = func_name.substr(dot_pos + 1);

        auto obj_reg = lookup_variable(obj_name);
        if (obj_reg) {
          if (method_name == "matmul") {
            if (expr.arguments.size() != 1) {
              throw std::runtime_error("matmul expects 1 argument");
            }
            expr.arguments[0]->accept(*this);
            auto arg_reg = ensure_expr_result(expr.arguments[0].get());
            auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
            tisc::ir::Instruction instr;
            instr.opcode = tisc::ir::Opcode::TMATMUL;
            instr.operands = {dest.reg, obj_reg->reg, arg_reg.reg};
            emit(instr);
            record_result(&expr, dest);
            return {};
          }
        }
      }

      if (func_name == "read_code") {
        if (expr.arguments.size() != 1) {
          throw std::runtime_error("read_code expects 1 argument");
        }
        expr.arguments[0]->accept(*this);
        auto addr = ensure_expr_result(expr.arguments[0].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::META_READ;
        instr.operands = {dest.reg, addr.reg};
        emit(instr);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "refine") {
        if (expr.arguments.size() != 2) {
          throw std::runtime_error("refine expects 2 arguments");
        }
        expr.arguments[0]->accept(*this);
        auto addr = ensure_expr_result(expr.arguments[0].get());
        expr.arguments[1]->accept(*this);
        auto val = ensure_expr_result(expr.arguments[1].get());

        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::META_REFINE;
        instr.operands = {addr.reg, val.reg};
        emit(instr);

        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        tisc::ir::Instruction res;
        res.opcode = tisc::ir::Opcode::LOADI;
        res.operands = {dest.reg, tisc::ir::Immediate{0}};
        emit(res);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "observe_performance") {
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::LOADI;
        instr.operands = {dest.reg, tisc::ir::Immediate{100}};
        emit(instr);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "optimize") {
        if (expr.arguments.size() != 1) {
          throw std::runtime_error("optimize expects 1 argument");
        }
        expr.arguments[0]->accept(*this);
        auto val = ensure_expr_result(expr.arguments[0].get());
        record_result(&expr, val);
        return {};
      }

      if (func_name == "sin" || func_name == "cos" || func_name == "tan") {
        if (expr.arguments.size() != 1) {
          throw std::runtime_error("Math functions expect 1 argument.");
        }
        expr.arguments[0]->accept(*this);
        auto val = ensure_expr_result(expr.arguments[0].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Float);
        tisc::ir::Instruction instr;
        if (func_name == "sin")
          instr.opcode = tisc::ir::Opcode::FSIN;
        else if (func_name == "cos")
          instr.opcode = tisc::ir::Opcode::FCOS;
        else
          instr.opcode = tisc::ir::Opcode::FTAN;
        instr.operands = {dest.reg, val.reg};
        instr.primitive = tisc::ir::PrimitiveKind::Float;
        emit(instr);
        record_result(&expr, dest);
        return {};
      }

      if (func_name == "print") {
        if (expr.arguments.size() != 1) {
          throw std::runtime_error("print expects exactly one argument.");
        }
        expr.arguments[0]->accept(*this);
        auto value = ensure_expr_result(expr.arguments[0].get());
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::PRINT;
        instr.operands = {value.reg};
        emit(instr);
        return {};
      }
      if (func_name == "str_len") {
        if (expr.arguments.size() != 1) {
          throw std::runtime_error("str_len expects exactly one argument.");
        }
        expr.arguments[0]->accept(*this);
        auto value = ensure_expr_result(expr.arguments[0].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::STRLEN;
        instr.operands = {dest.reg, value.reg};
        instr.primitive = tisc::ir::PrimitiveKind::Integer;
        emit(instr);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "str_is_empty") {
        if (expr.arguments.size() != 1) {
          throw std::runtime_error("str_is_empty expects exactly one argument.");
        }
        expr.arguments[0]->accept(*this);
        auto value = ensure_expr_result(expr.arguments[0].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::STREMPTY;
        instr.operands = {dest.reg, value.reg};
        instr.primitive = tisc::ir::PrimitiveKind::Boolean;
        emit(instr);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "str_concat") {
        if (expr.arguments.size() != 2) {
          throw std::runtime_error("str_concat expects exactly two arguments.");
        }
        expr.arguments[0]->accept(*this);
        expr.arguments[1]->accept(*this);
        auto lhs = ensure_expr_result(expr.arguments[0].get());
        auto rhs = ensure_expr_result(expr.arguments[1].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::STRCONCAT;
        instr.operands = {dest.reg, lhs.reg, rhs.reg};
        emit(instr);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "str_starts_with") {
        if (expr.arguments.size() != 2) {
          throw std::runtime_error("str_starts_with expects exactly two arguments.");
        }
        expr.arguments[0]->accept(*this);
        expr.arguments[1]->accept(*this);
        auto value = ensure_expr_result(expr.arguments[0].get());
        auto prefix = ensure_expr_result(expr.arguments[1].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::STRSTARTSWITH;
        instr.operands = {dest.reg, value.reg, prefix.reg};
        instr.primitive = tisc::ir::PrimitiveKind::Boolean;
        emit(instr);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "str_ends_with") {
        if (expr.arguments.size() != 2) {
          throw std::runtime_error("str_ends_with expects exactly two arguments.");
        }
        expr.arguments[0]->accept(*this);
        expr.arguments[1]->accept(*this);
        auto value = ensure_expr_result(expr.arguments[0].get());
        auto suffix = ensure_expr_result(expr.arguments[1].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::STRENDSWITH;
        instr.operands = {dest.reg, value.reg, suffix.reg};
        instr.primitive = tisc::ir::PrimitiveKind::Boolean;
        emit(instr);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "str_contains") {
        if (expr.arguments.size() != 2) {
          throw std::runtime_error("str_contains expects exactly two arguments.");
        }
        expr.arguments[0]->accept(*this);
        expr.arguments[1]->accept(*this);
        auto value = ensure_expr_result(expr.arguments[0].get());
        auto needle = ensure_expr_result(expr.arguments[1].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::STRCONTAINS;
        instr.operands = {dest.reg, value.reg, needle.reg};
        instr.primitive = tisc::ir::PrimitiveKind::Boolean;
        emit(instr);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "str_index_of") {
        if (expr.arguments.size() != 2) {
          throw std::runtime_error("str_index_of expects exactly two arguments.");
        }
        expr.arguments[0]->accept(*this);
        expr.arguments[1]->accept(*this);
        auto value = ensure_expr_result(expr.arguments[0].get());
        auto needle = ensure_expr_result(expr.arguments[1].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::STRINDEXOF;
        instr.operands = {dest.reg, value.reg, needle.reg};
        instr.primitive = tisc::ir::PrimitiveKind::Integer;
        emit(instr);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "str_replace") {
        if (expr.arguments.size() != 3) {
          throw std::runtime_error("str_replace expects exactly three arguments.");
        }
        expr.arguments[0]->accept(*this);
        expr.arguments[1]->accept(*this);
        expr.arguments[2]->accept(*this);
        auto source = ensure_expr_result(expr.arguments[0].get());
        auto needle = ensure_expr_result(expr.arguments[1].get());
        auto replacement = ensure_expr_result(expr.arguments[2].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
        tisc::ir::Instruction copy;
        copy.opcode = tisc::ir::Opcode::MOV;
        copy.operands = {dest.reg, source.reg};
        emit(copy);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::STRREPLACE;
        instr.operands = {dest.reg, needle.reg, replacement.reg};
        emit(instr);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "bytes_len") {
        if (expr.arguments.size() != 1) {
          throw std::runtime_error("bytes_len expects exactly one argument.");
        }
        expr.arguments[0]->accept(*this);
        auto value = ensure_expr_result(expr.arguments[0].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::STRLEN;
        instr.operands = {dest.reg, value.reg};
        instr.primitive = tisc::ir::PrimitiveKind::Integer;
        emit(instr);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "bytes_is_empty") {
        if (expr.arguments.size() != 1) {
          throw std::runtime_error("bytes_is_empty expects exactly one argument.");
        }
        expr.arguments[0]->accept(*this);
        auto value = ensure_expr_result(expr.arguments[0].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::STREMPTY;
        instr.operands = {dest.reg, value.reg};
        instr.primitive = tisc::ir::PrimitiveKind::Boolean;
        emit(instr);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "bytes_concat") {
        if (expr.arguments.size() != 2) {
          throw std::runtime_error("bytes_concat expects exactly two arguments.");
        }
        expr.arguments[0]->accept(*this);
        expr.arguments[1]->accept(*this);
        auto lhs = ensure_expr_result(expr.arguments[0].get());
        auto rhs = ensure_expr_result(expr.arguments[1].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::STRCONCAT;
        instr.operands = {dest.reg, lhs.reg, rhs.reg};
        emit(instr);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "bytes_starts_with") {
        if (expr.arguments.size() != 2) {
          throw std::runtime_error("bytes_starts_with expects exactly two arguments.");
        }
        expr.arguments[0]->accept(*this);
        expr.arguments[1]->accept(*this);
        auto value = ensure_expr_result(expr.arguments[0].get());
        auto prefix = ensure_expr_result(expr.arguments[1].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::STRSTARTSWITH;
        instr.operands = {dest.reg, value.reg, prefix.reg};
        instr.primitive = tisc::ir::PrimitiveKind::Boolean;
        emit(instr);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "bytes_ends_with") {
        if (expr.arguments.size() != 2) {
          throw std::runtime_error("bytes_ends_with expects exactly two arguments.");
        }
        expr.arguments[0]->accept(*this);
        expr.arguments[1]->accept(*this);
        auto value = ensure_expr_result(expr.arguments[0].get());
        auto suffix = ensure_expr_result(expr.arguments[1].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::STRENDSWITH;
        instr.operands = {dest.reg, value.reg, suffix.reg};
        instr.primitive = tisc::ir::PrimitiveKind::Boolean;
        emit(instr);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "bytes_contains") {
        if (expr.arguments.size() != 2) {
          throw std::runtime_error("bytes_contains expects exactly two arguments.");
        }
        expr.arguments[0]->accept(*this);
        expr.arguments[1]->accept(*this);
        auto value = ensure_expr_result(expr.arguments[0].get());
        auto needle = ensure_expr_result(expr.arguments[1].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::STRCONTAINS;
        instr.operands = {dest.reg, value.reg, needle.reg};
        instr.primitive = tisc::ir::PrimitiveKind::Boolean;
        emit(instr);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "bytes_index_of") {
        if (expr.arguments.size() != 2) {
          throw std::runtime_error("bytes_index_of expects exactly two arguments.");
        }
        expr.arguments[0]->accept(*this);
        expr.arguments[1]->accept(*this);
        auto value = ensure_expr_result(expr.arguments[0].get());
        auto needle = ensure_expr_result(expr.arguments[1].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::STRINDEXOF;
        instr.operands = {dest.reg, value.reg, needle.reg};
        instr.primitive = tisc::ir::PrimitiveKind::Integer;
        emit(instr);
        record_result(&expr, dest);
        return {};
      }
      if (func_name == "bytes_replace") {
        if (expr.arguments.size() != 3) {
          throw std::runtime_error("bytes_replace expects exactly three arguments.");
        }
        expr.arguments[0]->accept(*this);
        expr.arguments[1]->accept(*this);
        expr.arguments[2]->accept(*this);
        auto source = ensure_expr_result(expr.arguments[0].get());
        auto needle = ensure_expr_result(expr.arguments[1].get());
        auto replacement = ensure_expr_result(expr.arguments[2].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
        tisc::ir::Instruction copy;
        copy.opcode = tisc::ir::Opcode::MOV;
        copy.operands = {dest.reg, source.reg};
        emit(copy);
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::STRREPLACE;
        instr.operands = {dest.reg, needle.reg, replacement.reg};
        emit(instr);
        record_result(&expr, dest);
        return {};
      }

      if (func_name == "T81Bytes") {
        if (expr.arguments.size() != 1) {
          throw std::runtime_error("T81Bytes conversion expects exactly one argument.");
        }
        expr.arguments[0]->accept(*this);
        auto value = ensure_expr_result(expr.arguments[0].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
        copy_to_dest(value, dest);
        record_result(&expr, dest);
        return {};
      }

      if (dynamic_cast<const VariableExpr*>(expr.callee.get())) {
        // Check for user-defined function
        auto label_it = _function_labels.find(func_name);
        if (label_it != _function_labels.end()) {
          // Push arguments
          for (const auto& arg : expr.arguments) {
            arg->accept(*this);
            auto val = ensure_expr_result(arg.get());
            tisc::ir::Instruction push;
            push.opcode = tisc::ir::Opcode::PUSH;
            push.operands = {val.reg};
            emit(push);
          }

          // Load function address
          auto addr = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
          tisc::ir::Instruction load;
          load.opcode = tisc::ir::Opcode::LOADI;
          load.operands = {addr.reg, label_it->second};
          emit(load);

          // CALL
          tisc::ir::Instruction call;
          call.opcode = tisc::ir::Opcode::CALL;
          call.operands = {tisc::ir::Register{0}, addr.reg};
          emit(call);

          // Pop result if not void
          bool returns_void = false;
          if (_semantic) {
            const Type* type = _semantic->type_of(&expr);
            if (type && type->kind == Type::Kind::Void) {
              returns_void = true;
            }
          }
          if (!returns_void) {
            auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
            tisc::ir::Instruction pop;
            pop.opcode = tisc::ir::Opcode::POP;
            pop.operands = {dest.reg};
            emit(pop);
            record_result(&expr, dest);
          }
          return {};
        }
      }
    }

    if (auto* type_expr = dynamic_cast<const SimpleTypeExpr*>(expr.callee.get())) {
      std::string type_name{type_expr->name.lexeme};
      if (type_name == "T81Bytes") {
        if (expr.arguments.size() != 1) {
          throw std::runtime_error("T81Bytes conversion expects exactly one argument.");
        }
        expr.arguments[0]->accept(*this);
        auto value = ensure_expr_result(expr.arguments[0].get());
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
        copy_to_dest(value, dest);
        record_result(&expr, dest);
        return {};
      }
      if (type_name == "T81Uint" || type_name == "T81Qutrit") {
        if (expr.arguments.size() != 1) {
          throw std::runtime_error(type_name + " conversion expects exactly one argument.");
        }
        expr.arguments[0]->accept(*this);
        auto value = ensure_expr_result(expr.arguments[0].get());
        auto integer_value = ensure_integer(value);
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        copy_to_dest(integer_value, dest);
        record_result(&expr, dest);
        return {};
      }
    }

    if (auto* generic_expr = dynamic_cast<const GenericTypeExpr*>(expr.callee.get())) {
      const Type* callee_type = typed_expr(generic_expr);
      if (callee_type && callee_type->kind == Type::Kind::Fixed) {
        if (expr.arguments.size() != 1) {
          throw std::runtime_error("T81Fixed constructor expects exactly one argument.");
        }
        expr.arguments[0]->accept(*this);
        auto value = ensure_expr_result(expr.arguments[0].get());
        auto integer_value = ensure_integer(value);
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        copy_to_dest(integer_value, dest);
        record_result(&expr, dest);
        return {};
      }

      if (callee_type && callee_type->kind == Type::Kind::Complex) {
        if (expr.arguments.size() != 2) {
          throw std::runtime_error("T81Complex constructor expects exactly two arguments.");
        }
        expr.arguments[0]->accept(*this);
        expr.arguments[1]->accept(*this);
        auto real = ensure_integer(ensure_expr_result(expr.arguments[0].get()));
        auto imag = ensure_integer(ensure_expr_result(expr.arguments[1].get()));
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        auto instr =
            tisc::ir::Instruction{tisc::ir::Opcode::MAKE_COMPLEX, {dest.reg, real.reg, imag.reg}};
        instr.primitive = tisc::ir::PrimitiveKind::Integer;
        emit(instr);
        record_result(&expr, dest);
        return {};
      }
    }

    if (auto* field_expr = dynamic_cast<const FieldAccessExpr*>(expr.callee.get())) {
      if (_semantic) {
        const Type* obj_type = _semantic->type_of(field_expr->object.get());
        if (obj_type && obj_type->kind == Type::Kind::Custom) {
          if (enum_info_for_name(obj_type->custom_name)) {
            // Enum Constructor Call
            std::string variant_name(field_expr->field.lexeme);
            std::optional<int> variant_id =
                resolve_variant_index(obj_type->custom_name, variant_name);

            if (expr.arguments.size() != 1) {
              throw std::runtime_error("Enum constructor expects 1 argument");
            }

            expr.arguments[0]->accept(*this);
            auto payload = ensure_expr_result(expr.arguments[0].get());

            tisc::ir::PrimitiveKind primitive = tisc::ir::PrimitiveKind::Integer;
            if (auto kind = categorize_primitive(typed_expr(&expr));
                kind != tisc::ir::PrimitiveKind::Unknown) {
              primitive = kind;
            }
            auto dest = allocate_typed_register(primitive);

            std::optional<int> global_id;
            if (variant_id) {
              global_id = global_variant_id_for(obj_type->custom_name, *variant_id);
            }

            if (global_id) {
              emit_make_enum_variant_payload(dest, payload, *global_id);
            } else {
              emit_simple(tisc::ir::Opcode::TRAP);
            }
            record_result(&expr, dest);
            return {};
          }
        }
      }
    }

    for (const auto& arg : expr.arguments) {
      arg->accept(*this);
    }
    return {};
  }
  std::any visit(const AssignExpr& expr) override {
    expr.value->accept(*this);
    auto value = ensure_expr_result(expr.value.get());

    if (auto var = dynamic_cast<const VariableExpr*>(expr.target.get())) {
      auto found = lookup_variable(var->name.lexeme);
      if (found.has_value()) {
        copy_to_dest(value, *found);
        record_result(&expr, *found);
      } else {
        bind_variable(std::string(var->name.lexeme), value);
        record_result(&expr, value);
      }
    } else if (auto idx = dynamic_cast<const IndexExpr*>(expr.target.get())) {
      idx->object->accept(*this);
      auto obj = ensure_expr_result(idx->object.get());

      idx->index->accept(*this);
      auto index = ensure_expr_result(idx->index.get());

      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::TSET;
      instr.operands = {obj.reg, index.reg, value.reg};
      emit(instr);

      record_result(&expr, value);
    } else {
      throw std::runtime_error("Invalid assignment target");
    }
    return {};
  }
  std::any visit(const SimpleTypeExpr&) override { return {}; }
  std::any visit(const GenericTypeExpr&) override { return {}; }

  std::any visit(const BlockExpr& expr) override {
    for (const auto& stmt : expr.statements) {
      stmt->accept(*this);
    }
    if (expr.final_expr) {
      expr.final_expr->accept(*this);
      if (_semantic) {
        const Type* type = _semantic->type_of(&expr);
        if (type && type->kind != Type::Kind::Void) {
          auto val = ensure_expr_result(expr.final_expr.get());
          record_result(&expr, val);
        }
      }
    }
    return {};
  }

  std::any visit(const IfExpr& expr) override {
    auto end_label = new_label();

    expr.condition->accept(*this);
    auto cond = ensure_expr_result(expr.condition.get());

    // Determine result type
    const Type* result_type = typed_expr(&expr);
    bool has_result = result_type && result_type->kind != Type::Kind::Void;
    TypedRegister dest{};

    if (has_result) {
      auto primitive = categorize_primitive(result_type);
      if (primitive == tisc::ir::PrimitiveKind::Unknown)
        primitive = tisc::ir::PrimitiveKind::Integer;
      dest = allocate_typed_register(primitive);
    }

    if (expr.else_branch) {
      auto else_label = new_label();
      emit_jump_if_zero(else_label, cond);

      expr.then_branch->accept(*this);
      if (has_result) {
        auto then_val = ensure_expr_result(expr.then_branch.get());
        copy_to_dest(then_val, dest);
      }
      emit_jump(end_label);

      emit_label(else_label);
      expr.else_branch->accept(*this);
      if (has_result) {
        auto else_val = ensure_expr_result(expr.else_branch.get());
        copy_to_dest(else_val, dest);
      }
    } else {
      emit_jump_if_zero(end_label, cond);
      expr.then_branch->accept(*this);
    }

    emit_label(end_label);

    if (has_result) {
      record_result(&expr, dest);
    }
    return {};
  }

  std::any visit(const MatchExpr& expr) override {
    expr.scrutinee->accept(*this);
    auto scrutinee_reg = ensure_expr_result(expr.scrutinee.get());

    const SemanticAnalyzer::MatchMetadata* metadata =
        _semantic ? _semantic->match_metadata_for(expr) : nullptr;
    const Type* result_type = typed_expr(&expr);
    auto primitive = categorize_primitive(result_type);
    if (primitive == tisc::ir::PrimitiveKind::Unknown) {
      primitive = tisc::ir::PrimitiveKind::Integer;
    }
    auto dest = allocate_typed_register(primitive);
    record_result(&expr, dest);

    auto end_label = new_label();
    auto trap_label = new_label();

    // Group arms by variant name
    std::vector<std::string> variants;
    std::unordered_map<std::string, std::vector<size_t>> arms_by_variant;
    for (size_t i = 0; i < expr.arms.size(); ++i) {
      std::string name{expr.arms[i].keyword.lexeme};
      if (arms_by_variant.find(name) == arms_by_variant.end()) {
        variants.push_back(name);
      }
      arms_by_variant[name].push_back(i);
    }

    auto flag_reg = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
    auto payload_reg = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);

    for (size_t v_idx = 0; v_idx < variants.size(); ++v_idx) {
      const std::string& v_name = variants[v_idx];
      const auto& arm_indices = arms_by_variant[v_name];
      auto next_variant_label = (v_idx + 1 < variants.size()) ? new_label() : trap_label;

      // Emit check for this variant
      if (v_name == "Some") {
        emit_option_is_some(flag_reg, scrutinee_reg);
        emit_jump_if_zero(next_variant_label, flag_reg);
      } else if (v_name == "None") {
        emit_option_is_some(flag_reg, scrutinee_reg);
        emit_jump_if_not_zero(next_variant_label, flag_reg);
      } else if (v_name == "Ok") {
        emit_result_is_ok(flag_reg, scrutinee_reg);
        emit_jump_if_zero(next_variant_label, flag_reg);
      } else if (v_name == "Err") {
        emit_result_is_ok(flag_reg, scrutinee_reg);
        emit_jump_if_not_zero(next_variant_label, flag_reg);
      } else if (metadata && metadata->kind == SemanticAnalyzer::MatchMetadata::Kind::Enum) {
        int variant_id = -1;
        for (size_t idx : arm_indices) {
          if (metadata->arms[idx].variant_id >= 0) {
            variant_id = metadata->arms[idx].variant_id;
            if (auto encoded = global_variant_id_for(metadata->arms[idx])) {
              variant_id = *encoded;
            }
            break;
          }
        }
        if (variant_id >= 0) {
          emit_enum_is_variant(flag_reg, scrutinee_reg, variant_id);
          emit_jump_if_zero(next_variant_label, flag_reg);
        } else {
          emit_jump(next_variant_label);
        }
      } else {
        emit_jump(next_variant_label);
      }

      // If we reached here, the variant matches. Now check arms sequentially.
      for (size_t a_idx = 0; a_idx < arm_indices.size(); ++a_idx) {
        size_t arm_idx = arm_indices[a_idx];
        const auto& arm = expr.arms[arm_idx];
        auto next_arm_label = (a_idx + 1 < arm_indices.size()) ? new_label() : next_variant_label;

        enter_pattern_scope();

        bool has_payload = false;
        if (v_name == "Some") {
          emit_option_unwrap(payload_reg, scrutinee_reg);
          has_payload = true;
        } else if (v_name == "Ok") {
          emit_result_unwrap_ok(payload_reg, scrutinee_reg);
          has_payload = true;
        } else if (v_name == "Err") {
          emit_result_unwrap_err(payload_reg, scrutinee_reg);
          has_payload = true;
        } else if (metadata && metadata->kind == SemanticAnalyzer::MatchMetadata::Kind::Enum) {
          if (metadata->arms[arm_idx].payload_type.kind != Type::Kind::Unknown) {
            emit_enum_unwrap_payload(payload_reg, scrutinee_reg);
            has_payload = true;
          }
        }

        if (has_payload) {
          bind_variant_payload(arm, payload_reg);
        }

        if (arm.guard && metadata) {
          const auto& arm_meta = metadata->arms[arm_idx];
          emit_guard_metadata(&arm_meta, arm_meta.variant_id >= 0
                                             ? std::optional<int>(arm_meta.variant_id)
                                             : std::nullopt);
          arm.guard->accept(*this);
          auto guard_value = ensure_expr_result(arm.guard.get());
          emit_jump_if_zero(next_arm_label, guard_value);
        }

        arm.expression->accept(*this);
        auto value = ensure_expr_result(arm.expression.get());
        copy_to_dest(value, dest);
        emit_jump(end_label);

        if (a_idx + 1 < arm_indices.size()) {
          emit_label(next_arm_label);
        }
        exit_pattern_scope();
      }

      if (v_idx + 1 < variants.size()) {
        emit_label(next_variant_label);
      }
    }

    emit_label(trap_label);
    emit_simple(tisc::ir::Opcode::TRAP);
    emit_label(end_label);
    emit_simple(tisc::ir::Opcode::NOP);
    return {};
  }

  std::any visit(const FieldAccessExpr& expr) override {
    if (_semantic) {
      const Type* obj_type = _semantic->type_of(expr.object.get());
      if (obj_type && obj_type->kind == Type::Kind::Custom) {
        if (enum_info_for_name(obj_type->custom_name)) {
          // Enum Constant Variant Access
          std::string variant_name(expr.field.lexeme);
          std::optional<int> variant_id =
              resolve_variant_index(obj_type->custom_name, variant_name);

          tisc::ir::PrimitiveKind primitive = tisc::ir::PrimitiveKind::Integer;
          if (auto kind = categorize_primitive(typed_expr(&expr));
              kind != tisc::ir::PrimitiveKind::Unknown) {
            primitive = kind;
          }
          auto dest = allocate_typed_register(primitive);

          std::optional<int> global_id;
          if (variant_id) {
            global_id = global_variant_id_for(obj_type->custom_name, *variant_id);
          }

          if (global_id) {
            emit_make_enum_variant(dest, *global_id);
          } else {
            emit_simple(tisc::ir::Opcode::TRAP);
          }
          record_result(&expr, dest);
          return {};
        }
      }
    }

    auto value = evaluate_expr(expr.object.get());
    record_result(&expr, value);
    return {};
  }

  std::any visit(const RecordLiteralExpr& expr) override {
    for (const auto& field : expr.fields) {
      field.second->accept(*this);
    }
    tisc::ir::PrimitiveKind primitive = tisc::ir::PrimitiveKind::Integer;
    if (auto kind = categorize_primitive(typed_expr(&expr));
        kind != tisc::ir::PrimitiveKind::Unknown) {
      primitive = kind;
    }
    auto dest = allocate_typed_register(primitive);
    record_result(&expr, dest);
    return {};
  }

  std::any visit(const EnumLiteralExpr& expr) override {
    std::string enum_name(expr.enum_name.lexeme);
    std::string variant_name(expr.variant.lexeme);
    std::optional<int> variant_id = resolve_variant_index(enum_name, variant_name);
    if (expr.payload) {
      expr.payload->accept(*this);
    }
    tisc::ir::PrimitiveKind primitive = tisc::ir::PrimitiveKind::Integer;
    if (auto kind = categorize_primitive(typed_expr(&expr));
        kind != tisc::ir::PrimitiveKind::Unknown) {
      primitive = kind;
    }
    auto dest = allocate_typed_register(primitive);
    std::optional<int> global_variant_id;
    if (variant_id) {
      global_variant_id = global_variant_id_for(enum_name, *variant_id);
      if (!global_variant_id) {
        global_variant_id = *variant_id;
      }
    }
    if (global_variant_id) {
      if (expr.payload) {
        auto payload_reg = ensure_expr_result(expr.payload.get());
        emit_make_enum_variant_payload(dest, payload_reg, *global_variant_id);
      } else {
        emit_make_enum_variant(dest, *global_variant_id);
      }
    } else {
      emit_simple(tisc::ir::Opcode::TRAP);
    }
    record_result(&expr, dest);
    return {};
  }

  std::any visit(const VectorLiteralExpr& expr) override {
    if (!_semantic) return {};
    const auto* data = _semantic->vector_literal_data(&expr);
    if (!data) {
      throw std::runtime_error("Vector literal data missing during IR generation.");
    }
    t81::T729Tensor tensor({static_cast<int>(data->size())}, *data);
    int handle = _program.add_tensor(std::move(tensor));
    auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
    tisc::ir::Instruction instr;
    instr.opcode = tisc::ir::Opcode::LOADI;
    instr.operands = {dest.reg, tisc::ir::Immediate{handle}};
    instr.literal_kind = tisc::LiteralKind::TensorHandle;
    emit(instr);
    record_result(&expr, dest);
    return {};
  }

  std::any visit(const IndexExpr& expr) override {
    expr.object->accept(*this);
    auto obj = ensure_expr_result(expr.object.get());
    expr.index->accept(*this);
    auto index = ensure_expr_result(expr.index.get());

    // Check if we need to convert the result from float (tensor storage) to int
    tisc::ir::PrimitiveKind dest_kind = tisc::ir::PrimitiveKind::Unknown;
    if (_semantic) {
      const Type* type = _semantic->type_of(&expr);
      dest_kind = categorize_primitive(type);
    }

    // TGet returns a FloatHandle by default as Tensors store floats
    auto temp_dest = allocate_typed_register(tisc::ir::PrimitiveKind::Float);

    tisc::ir::Instruction instr;
    instr.opcode = tisc::ir::Opcode::TGET;
    instr.operands = {temp_dest.reg, obj.reg, index.reg};
    emit(instr);

    if (dest_kind == tisc::ir::PrimitiveKind::Integer) {
      auto final_dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction conv;
      conv.opcode = tisc::ir::Opcode::F2I;
      conv.operands = {final_dest.reg, temp_dest.reg};
      emit(conv);
      record_result(&expr, final_dest);
    } else {
      record_result(&expr, temp_dest);
    }
    return {};
  }

private:
  struct TypedRegister {
    tisc::ir::Register reg;
    tisc::ir::PrimitiveKind primitive = tisc::ir::PrimitiveKind::Unknown;
  };

  enum class NumericCategory { Integer, Float, Fraction, Unknown };

  static tisc::ir::ComparisonRelation relation_from_token(TokenType type) {
    switch (type) {
      case TokenType::Less:
        return tisc::ir::ComparisonRelation::Less;
      case TokenType::LessEqual:
        return tisc::ir::ComparisonRelation::LessEqual;
      case TokenType::Greater:
        return tisc::ir::ComparisonRelation::Greater;
      case TokenType::GreaterEqual:
        return tisc::ir::ComparisonRelation::GreaterEqual;
      case TokenType::EqualEqual:
        return tisc::ir::ComparisonRelation::Equal;
      case TokenType::BangEqual:
        return tisc::ir::ComparisonRelation::NotEqual;
      default:
        return tisc::ir::ComparisonRelation::None;
    }
  }

  NumericCategory categorize(const Type* type) const {
    if (!type) return NumericCategory::Integer;
    switch (type->kind) {
      case Type::Kind::Qutrit:
      case Type::Kind::I2:
      case Type::Kind::I8:
      case Type::Kind::I16:
      case Type::Kind::I32:
      case Type::Kind::Uint:
      case Type::Kind::BigInt:
      case Type::Kind::Fixed:
      case Type::Kind::Complex:
        return NumericCategory::Integer;
      case Type::Kind::Float:
        return NumericCategory::Float;
      case Type::Kind::Fraction:
        return NumericCategory::Fraction;
      default:
        return NumericCategory::Unknown;
    }
  }

  tisc::ir::PrimitiveKind categorize_primitive(const Type* type) const {
    if (!type) return tisc::ir::PrimitiveKind::Integer;
    switch (type->kind) {
      case Type::Kind::Qutrit:
      case Type::Kind::I2:
      case Type::Kind::I8:
      case Type::Kind::I16:
      case Type::Kind::I32:
      case Type::Kind::Uint:
      case Type::Kind::BigInt:
      case Type::Kind::Fixed:
      case Type::Kind::Complex:
        return tisc::ir::PrimitiveKind::Integer;
      case Type::Kind::Float:
        return tisc::ir::PrimitiveKind::Float;
      case Type::Kind::Fraction:
        return tisc::ir::PrimitiveKind::Fraction;
      case Type::Kind::Bool:
        return tisc::ir::PrimitiveKind::Boolean;
      default:
        return tisc::ir::PrimitiveKind::Unknown;
    }
  }

  tisc::ir::Opcode select_opcode(NumericCategory kind, tisc::ir::Opcode integer_op,
                                 tisc::ir::Opcode float_op, tisc::ir::Opcode fraction_op) const {
    switch (kind) {
      case NumericCategory::Float:
        return float_op;
      case NumericCategory::Fraction:
        return fraction_op;
      default:
        return integer_op;
    }
  }

  const Type* typed_expr(const Expr* expr) const {
    return _semantic ? _semantic->type_of(expr) : nullptr;
  }

  void emit(tisc::ir::Instruction instr) { _program.add_instruction(std::move(instr)); }

  void emit_simple(tisc::ir::Opcode opcode) {
    tisc::ir::Instruction instr;
    instr.opcode = opcode;
    emit(instr);
  }

  void emit_label(tisc::ir::Label label) {
    emit(tisc::ir::Instruction{tisc::ir::Opcode::LABEL, {label}});
  }

  void emit_jump(tisc::ir::Label target) {
    emit(tisc::ir::Instruction{tisc::ir::Opcode::JMP, {target}});
  }

  void emit_jump_if_zero(tisc::ir::Label target, const TypedRegister& cond) {
    emit(tisc::ir::Instruction{tisc::ir::Opcode::JZ, {target, cond.reg}});
  }

  void emit_jump_if_not_zero(tisc::ir::Label target, const TypedRegister& cond) {
    emit(tisc::ir::Instruction{tisc::ir::Opcode::JNZ, {target, cond.reg}});
  }

  void emit_option_is_some(const TypedRegister& dest, const TypedRegister& source) {
    emit(tisc::ir::Instruction{tisc::ir::Opcode::OPTION_IS_SOME, {dest.reg, source.reg}});
  }

  void emit_option_unwrap(const TypedRegister& dest, const TypedRegister& source) {
    emit(tisc::ir::Instruction{tisc::ir::Opcode::OPTION_UNWRAP, {dest.reg, source.reg}});
  }

  void emit_result_is_ok(const TypedRegister& dest, const TypedRegister& source) {
    emit(tisc::ir::Instruction{tisc::ir::Opcode::RESULT_IS_OK, {dest.reg, source.reg}});
  }

  void emit_result_unwrap_ok(const TypedRegister& dest, const TypedRegister& source) {
    emit(tisc::ir::Instruction{tisc::ir::Opcode::RESULT_UNWRAP_OK, {dest.reg, source.reg}});
  }

  void emit_result_unwrap_err(const TypedRegister& dest, const TypedRegister& source) {
    emit(tisc::ir::Instruction{tisc::ir::Opcode::RESULT_UNWRAP_ERR, {dest.reg, source.reg}});
  }

  void emit_make_option_some(const TypedRegister& dest, const TypedRegister& payload) {
    emit(tisc::ir::Instruction{tisc::ir::Opcode::MAKE_OPTION_SOME, {dest.reg, payload.reg}});
  }

  void emit_make_option_none(const TypedRegister& dest) {
    emit(tisc::ir::Instruction{tisc::ir::Opcode::MAKE_OPTION_NONE, {dest.reg}});
  }

  void emit_make_result_ok(const TypedRegister& dest, const TypedRegister& payload) {
    emit(tisc::ir::Instruction{tisc::ir::Opcode::MAKE_RESULT_OK, {dest.reg, payload.reg}});
  }

  void emit_make_result_err(const TypedRegister& dest, const TypedRegister& payload) {
    emit(tisc::ir::Instruction{tisc::ir::Opcode::MAKE_RESULT_ERR, {dest.reg, payload.reg}});
  }

  void emit_make_enum_variant(const TypedRegister& dest, int global_variant_id) {
    tisc::ir::Instruction instr;
    instr.opcode = tisc::ir::Opcode::MAKE_ENUM_VARIANT;
    instr.operands = {dest.reg, tisc::ir::Immediate{global_variant_id}};
    emit(instr);
  }

  void emit_make_enum_variant_payload(const TypedRegister& dest, const TypedRegister& payload,
                                      int global_variant_id) {
    tisc::ir::Instruction instr;
    instr.opcode = tisc::ir::Opcode::MAKE_ENUM_VARIANT_PAYLOAD;
    instr.operands = {dest.reg, payload.reg, tisc::ir::Immediate{global_variant_id}};
    emit(instr);
  }

  void emit_enum_is_variant(const TypedRegister& dest, const TypedRegister& source,
                            int global_variant_id) {
    tisc::ir::Instruction instr;
    instr.opcode = tisc::ir::Opcode::ENUM_IS_VARIANT;
    instr.operands = {dest.reg, source.reg, tisc::ir::Immediate{global_variant_id}};
    emit(instr);
  }

  void emit_enum_unwrap_payload(const TypedRegister& dest, const TypedRegister& source) {
    emit(tisc::ir::Instruction{tisc::ir::Opcode::ENUM_UNWRAP_PAYLOAD, {dest.reg, source.reg}});
  }

  tisc::ir::Register new_register() {
    // Skip system registers R75-R80 reserved by VM
    if (_register_count >= 75 && _register_count <= 80) {
      _register_count = 81;
    }
    return tisc::ir::Register{_register_count++};
  }

  tisc::ir::Label new_label() { return tisc::ir::Label{_label_count++}; }

  TypedRegister evaluate_expr(const Expr* expr) {
    expr->accept(*this);
    auto it = _expr_registers.find(expr);
    if (it == _expr_registers.end()) {
      std::string info = typeid(*expr).name();
      if (auto var = dynamic_cast<const VariableExpr*>(expr)) {
        info = std::string("Variable(") + std::string(var->name.lexeme) + ")";
      }
      std::cerr << "Missing expression result for " << info << "\n";
      throw std::runtime_error("IRGenerator failed to record expression result for " + info);
    }
    return it->second;
  }

  void record_result(const Expr* expr, TypedRegister reg) { _expr_registers[expr] = reg; }

  TypedRegister allocate_typed_register(tisc::ir::PrimitiveKind primitive) {
    return TypedRegister{new_register(), primitive};
  }

  TypedRegister ensure_kind(TypedRegister source, tisc::ir::PrimitiveKind target) {
    if (target == tisc::ir::PrimitiveKind::Unknown || source.primitive == target) {
      return source;
    }
    if (source.primitive != tisc::ir::PrimitiveKind::Integer) {
      throw std::runtime_error("Implicit conversion only supported from integers");
    }
    tisc::ir::Opcode opcode;
    switch (target) {
      case tisc::ir::PrimitiveKind::Float:
        opcode = tisc::ir::Opcode::I2F;
        break;
      case tisc::ir::PrimitiveKind::Fraction:
        opcode = tisc::ir::Opcode::I2FRAC;
        break;
      default:
        throw std::runtime_error("Unsupported conversion target");
    }
    auto dest = allocate_typed_register(target);
    auto instr = tisc::ir::Instruction{opcode, {dest.reg, source.reg}};
    instr.primitive = target;
    instr.is_conversion = true;
    emit(instr);
    return dest;
  }

  TypedRegister ensure_integer(TypedRegister source) {
    if (source.primitive == tisc::ir::PrimitiveKind::Integer ||
        source.primitive == tisc::ir::PrimitiveKind::Boolean ||
        source.primitive == tisc::ir::PrimitiveKind::Unknown) {
      return source;
    }
    tisc::ir::Opcode opcode;
    if (source.primitive == tisc::ir::PrimitiveKind::Float) {
      opcode = tisc::ir::Opcode::F2I;
    } else if (source.primitive == tisc::ir::PrimitiveKind::Fraction) {
      opcode = tisc::ir::Opcode::FRAC2I;
    } else {
      throw std::runtime_error("Unsupported conversion source for integer coercion");
    }
    auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
    auto instr = tisc::ir::Instruction{opcode, {dest.reg, source.reg}};
    instr.primitive = tisc::ir::PrimitiveKind::Integer;
    instr.is_conversion = true;
    emit(instr);
    return dest;
  }

  TypedRegister ensure_expr_result(const Expr* expr) const {
    auto it = _expr_registers.find(expr);
    if (it == _expr_registers.end()) {
      std::string info = typeid(*expr).name();
      if (auto var = dynamic_cast<const VariableExpr*>(expr)) {
        info = std::string("Variable(") + std::string(var->name.lexeme) + ")";
      }
      std::cerr << "Missing expression result for " << info << "\n";
      throw std::runtime_error("IRGenerator missing expression result");
    }
    return it->second;
  }

  void copy_to_dest(TypedRegister source, TypedRegister dest) {
    if (source.reg.index == dest.reg.index) {
      return;
    }
    tisc::ir::Instruction instr;
    instr.opcode = tisc::ir::Opcode::MOV;
    instr.operands = {dest.reg, source.reg};
    instr.primitive = dest.primitive;
    emit(instr);
  }

  void bind_variable(const std::string& name, TypedRegister reg) {
    _variable_registers[name] = reg;
  }

  std::optional<TypedRegister> lookup_variable(std::string_view name) const {
    auto it = _variable_registers.find(std::string{name});
    if (it != _variable_registers.end()) {
      return it->second;
    }
    return std::nullopt;
  }

  void bind_variable_from_initializer(const Token& name_token, const Expr* initializer) {
    TypedRegister reg{};
    if (initializer) {
      initializer->accept(*this);
      reg = ensure_expr_result(initializer);
    } else {
      reg = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
    }
    bind_variable(std::string(name_token.lexeme), reg);
  }

  void enter_pattern_scope() { _pattern_scopes.emplace_back(); }

  void exit_pattern_scope() {
    if (_pattern_scopes.empty()) {
      return;
    }
    auto scope = std::move(_pattern_scopes.back());
    _pattern_scopes.pop_back();
    for (const auto& entry : scope) {
      if (entry.second.has_value()) {
        _variable_registers[entry.first] = entry.second.value();
      } else {
        _variable_registers.erase(entry.first);
      }
    }
  }

  void bind_pattern_variable(std::string name, const TypedRegister& reg) {
    std::optional<TypedRegister> previous;
    auto it = _variable_registers.find(name);
    if (it != _variable_registers.end()) {
      previous = it->second;
    }
    _variable_registers[name] = reg;
    if (!_pattern_scopes.empty()) {
      _pattern_scopes.back().emplace_back(name, previous);
    }
  }

  void bind_pattern_payload(const MatchPattern& pattern, const TypedRegister& reg) {
    if (pattern.kind == MatchPattern::Kind::Identifier && !pattern.binding_is_wildcard) {
      bind_pattern_variable(std::string(pattern.identifier.lexeme), reg);
    }
  }

  void bind_variant_payload(const MatchArm& arm, const TypedRegister& reg) {
    if (arm.pattern.kind == MatchPattern::Kind::Variant && arm.pattern.variant_payload) {
      bind_pattern_payload(*arm.pattern.variant_payload, reg);
      return;
    }
    // For Option/Result arms the parsed pattern already represents the payload bindings.
    bind_pattern_payload(arm.pattern, reg);
  }

  std::string guard_metadata_reason(const SemanticAnalyzer::MatchMetadata::ArmInfo& info,
                                    std::optional<int> variant_id) const {
    std::ostringstream oss;
    oss << "guard-expr \"" << escape_metadata_string(info.guard_expression) << "\"";
    if (!info.enum_name.empty()) {
      oss << " enum=" << info.enum_name;
    }
    oss << " variant=" << info.variant;
    if (variant_id.has_value()) {
      oss << " variant-id=" << *variant_id;
    }
    if (_semantic && info.payload_type.kind != Type::Kind::Unknown) {
      oss << " payload=" << _semantic->type_name(info.payload_type);
    }
    return oss.str();
  }

  void emit_guard_metadata(const SemanticAnalyzer::MatchMetadata::ArmInfo* info,
                           std::optional<int> variant_id) {
    if (!info || info->guard_expression.empty()) {
      return;
    }
    tisc::ir::Instruction instr;
    instr.opcode = tisc::ir::Opcode::NOP;
    instr.literal_kind = tisc::LiteralKind::SymbolHandle;
    instr.text_literal = guard_metadata_reason(*info, variant_id);
    emit(instr);
  }

  const EnumInfo* enum_info_for_name(std::string_view name) const {
    if (!_semantic) return nullptr;
    auto it = _semantic->enum_definitions().find(std::string(name));
    if (it == _semantic->enum_definitions().end()) {
      return nullptr;
    }
    return &it->second;
  }

  std::optional<int> global_variant_id_for(std::string_view enum_name, int variant_id) const {
    if (variant_id < 0) return std::nullopt;
    if (const auto* info = enum_info_for_name(enum_name)) {
      if (info->id >= 0) {
        int encoded = t81::enum_meta::encode_variant_id(info->id, variant_id);
        if (encoded >= 0) {
          return encoded;
        }
      }
    }
    return std::nullopt;
  }

  std::optional<int> global_variant_id_for(
      const SemanticAnalyzer::MatchMetadata::ArmInfo& arm) const {
    if (arm.enum_id < 0 || arm.variant_id < 0) {
      return std::nullopt;
    }
    int encoded = t81::enum_meta::encode_variant_id(arm.enum_id, arm.variant_id);
    if (encoded < 0) {
      return std::nullopt;
    }
    return encoded;
  }

  std::optional<int> resolve_variant_index(std::string_view enum_name,
                                           std::string_view variant_name) const {
    if (!_semantic) return std::nullopt;
    std::string name(enum_name);
    auto enum_it = _semantic->enum_definitions().find(name);
    if (enum_it == _semantic->enum_definitions().end()) return std::nullopt;
    const auto& info = enum_it->second;
    for (size_t idx = 0; idx < info.variant_order.size(); ++idx) {
      if (info.variant_order[idx] == variant_name) {
        return static_cast<int>(idx);
      }
    }
    return std::nullopt;
  }

  tisc::ir::IntermediateProgram _program;
  SymbolTable _symbols;
  const SemanticAnalyzer* _semantic = nullptr;
  int _register_count = 1;
  int _label_count = 0;
  std::unordered_map<const Expr*, TypedRegister> _expr_registers;
  std::unordered_map<std::string, TypedRegister> _variable_registers;
  std::unordered_map<std::string, tisc::ir::Label> _function_labels;
  std::vector<std::vector<std::pair<std::string, std::optional<TypedRegister>>>> _pattern_scopes;
  std::vector<LoopInfo> _loop_infos;
  std::vector<LoopInfo> _loop_stack;
};

}  // namespace t81::frontend

#endif  // T81_FRONTEND_IR_GENERATOR_HPP
