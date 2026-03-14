// lang/frontend/ir_generator.cpp
// Implementation extracted from ir_generator.hpp

#include "t81/frontend/ir_generator.hpp"

namespace t81::frontend {

tisc::ir::IntermediateProgram IRGenerator::generate(const std::vector<std::unique_ptr<Stmt>>& statements) {
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
    // Determine if main() returns void (no result to pop after call)
    bool main_returns_void = false;
    for (const auto* func : functions) {
      if (func->name.lexeme == "main") {
        if (func->return_type) {
          if (auto* st = dynamic_cast<const SimpleTypeExpr*>(func->return_type.get())) {
            main_returns_void = (st->name.lexeme == "void");
          }
        }
        break;
      }
    }

    tisc::ir::Instruction load;
    auto addr_reg = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
    load.opcode = tisc::ir::Opcode::LOADI;
    load.operands = {addr_reg.reg, main_it->second};
    load.primitive = tisc::ir::PrimitiveKind::Integer;
    emit(load);

    tisc::ir::Instruction call;
    call.opcode = tisc::ir::Opcode::CALL;
    call.operands = {tisc::ir::Register{0}, addr_reg.reg};
    call.primitive = tisc::ir::PrimitiveKind::Integer;
    emit(call);

    // Pop main result only for non-void main
    if (!main_returns_void) {
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction pop;
      pop.opcode = tisc::ir::Opcode::POP;
      pop.operands = {dest.reg};
      pop.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(pop);
    }
  }

  emit_simple(tisc::ir::Opcode::HALT);

  // Pass 2: Emit functions
  for (const auto* func : functions) {
    func->accept(*this);
  }

  return std::move(_program);
}

const std::vector<IRGenerator::LoopInfo>& IRGenerator::loop_infos() const
{ return _loop_infos; }

void IRGenerator::attach_semantic_analyzer(const SemanticAnalyzer* analyzer)
{ _semantic = analyzer; }

// Statements
std::any IRGenerator::visit(const ExpressionStmt& stmt) {
  stmt.expression->accept(*this);
  return {};
}

std::any IRGenerator::visit(const BlockStmt& stmt) {
  for (const auto& s : stmt.statements) s->accept(*this);
  return {};
}

std::any IRGenerator::visit(const VarStmt& stmt) {
  TypedRegister reg{};
  if (stmt.initializer) {
    stmt.initializer->accept(*this);
    reg = ensure_expr_result(stmt.initializer.get());
  } else {
    reg = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
  }
  tisc::ir::PrimitiveKind expected = primitive_kind_from_type_expr(stmt.type.get());
  if (expected != tisc::ir::PrimitiveKind::Unknown) {
    reg = ensure_kind(reg, expected);
  }
  bind_variable(std::string(stmt.name.lexeme), reg);
  return {};
}

std::any IRGenerator::visit(const LetStmt& stmt) {
  TypedRegister reg{};
  if (stmt.initializer) {
    stmt.initializer->accept(*this);
    reg = ensure_expr_result(stmt.initializer.get());
  } else {
    reg = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
  }
  tisc::ir::PrimitiveKind expected = primitive_kind_from_type_expr(stmt.type.get());
  if (expected != tisc::ir::PrimitiveKind::Unknown) {
    reg = ensure_kind(reg, expected);
  }
  bind_variable(std::string(stmt.name.lexeme), reg);
  return {};
}

std::any IRGenerator::visit(const IfStmt& stmt) {
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

std::any IRGenerator::visit(const WhileStmt& stmt) {
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

std::any IRGenerator::visit(const ForStmt& stmt) {
  auto entry_label = new_label();
  auto exit_label = new_label();
  if (auto binary = dynamic_cast<const BinaryExpr*>(stmt.iterable.get())) {
    const bool is_inclusive = (binary->op.type == TokenType::DotDotEq);
    if (binary->op.type == TokenType::DotDot || is_inclusive) {
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
      instr.relation = is_inclusive ? tisc::ir::ComparisonRelation::LessEqual
                                    : tisc::ir::ComparisonRelation::Less;
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

std::any IRGenerator::visit(const ReflectStmt& stmt) {
  auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
  tisc::ir::Instruction instr;
  instr.opcode = tisc::ir::Opcode::META_REFLECT;
  instr.operands = {dest.reg};
  emit(instr);
  for (const auto& s : stmt.body) s->accept(*this);
  return {};
}

std::any IRGenerator::visit(const RecurseStmt& stmt) {
  for (const auto& s : stmt.body) s->accept(*this);
  return {};
}

std::any IRGenerator::visit(const DistributedStmt& stmt) {
  for (const auto& s : stmt.body) s->accept(*this);
  return {};
}

std::any IRGenerator::visit(const InfiniteStmt& stmt) {
  for (const auto& s : stmt.body) s->accept(*this);
  return {};
}

std::any IRGenerator::visit(const TrainStmt& stmt) {
  stmt.model->accept(*this);
  auto model_reg = ensure_expr_result(stmt.model.get());

  // Evaluate body
  for (const auto& s : stmt.body) s->accept(*this);

  // Emit Backward Pass (TNeuralBwd) on the model
  tisc::ir::Instruction instr;
  instr.opcode = tisc::ir::Opcode::TNEURAL_BWD;
  instr.operands = {model_reg.reg};
  emit(instr);
  return {};
}

std::any IRGenerator::visit(const LoopStmt& stmt) {
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

std::any IRGenerator::visit(const ReturnStmt& stmt) {
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

std::any IRGenerator::visit(const AssertStmt& stmt) {
  stmt.expr->accept(*this);
  auto cond = ensure_expr_result(stmt.expr.get());
  auto pass_label = new_label();
  emit_jump_if_not_zero(pass_label, cond);
  emit_simple(tisc::ir::Opcode::TRAP);
  emit_label(pass_label);
  return {};
}

std::any IRGenerator::visit(const BreakStmt&) {
  if (!_loop_stack.empty()) {
    emit_jump(_loop_stack.back().exit_label);
  }
  return {};
}

std::any IRGenerator::visit(const ContinueStmt&) {
  if (!_loop_stack.empty()) {
    emit_jump(_loop_stack.back().entry_label);
  }
  return {};
}

std::any IRGenerator::visit(const FunctionStmt& stmt) {
  std::string name{stmt.name.lexeme};
  emit_label(_function_labels[name]);

  enter_pattern_scope();

  // Pop return address
  auto ret_reg = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
  bind_variable("%ret_addr", ret_reg);
  tisc::ir::Instruction pop_ret;
  pop_ret.opcode = tisc::ir::Opcode::POP;
  pop_ret.operands = {ret_reg.reg};
  pop_ret.primitive = tisc::ir::PrimitiveKind::Integer;
  emit(pop_ret);

  // Pop arguments in reverse order
  for (auto it = stmt.params.rbegin(); it != stmt.params.rend(); ++it) {
    auto reg = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
    tisc::ir::Instruction pop;
    pop.opcode = tisc::ir::Opcode::POP;
    pop.operands = {reg.reg};
    pop.primitive = tisc::ir::PrimitiveKind::Integer;
    emit(pop);
    bind_variable(std::string(it->name.lexeme), reg);
  }

  // RFC-0026 AI-M6: register @attention / @qmatmul annotated functions so call
  // sites are lowered to ATTN / QMATMUL instead of a regular CALL sequence.
  if (stmt.is_attention) {
    _ai_intrinsic_map[name] = tisc::ir::Opcode::ATTN;
  } else if (stmt.is_qmatmul) {
    _ai_intrinsic_map[name] = tisc::ir::Opcode::QMATMUL;
  }

  // @axion_verify: emit AxVerify opcode at function entry (AX-M7 conformance hook).
  if (stmt.is_axion_verify) {
    auto verify_result = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
    tisc::ir::Instruction axverify;
    axverify.opcode = tisc::ir::Opcode::AXVERIFY;
    axverify.operands = {verify_result.reg};
    emit(axverify);
  }

  // Determine if the function has a non-void return type.
  // For non-void functions the last ExpressionStmt is the implicit return value.
  bool is_void_return = true;
  if (stmt.return_type) {
    if (auto* st = dynamic_cast<const SimpleTypeExpr*>(stmt.return_type.get())) {
      is_void_return = (st->name.lexeme == "void");
    }
  }

  // Identify the tail expression (implicit return) for non-void functions.
  const ExpressionStmt* tail_expr_stmt = nullptr;
  std::size_t body_limit = stmt.body.size();
  if (!is_void_return && body_limit > 0) {
    if (auto* es = dynamic_cast<const ExpressionStmt*>(stmt.body.back().get())) {
      tail_expr_stmt = es;
      --body_limit;
    }
  }

  for (std::size_t i = 0; i < body_limit; ++i) {
    stmt.body[i]->accept(*this);
  }

  // For non-void implicit return: push result value before ret_addr.
  if (tail_expr_stmt) {
    tail_expr_stmt->expression->accept(*this);
    auto result = ensure_expr_result(tail_expr_stmt->expression.get());
    tisc::ir::Instruction push_val;
    push_val.opcode = tisc::ir::Opcode::PUSH;
    push_val.operands = {result.reg};
    emit(push_val);
  }

  // Push return address back and return.
  tisc::ir::Instruction push_ret;
  push_ret.opcode = tisc::ir::Opcode::PUSH;
  push_ret.operands = {ret_reg.reg};
  emit(push_ret);

  emit_simple(tisc::ir::Opcode::RET);

  exit_pattern_scope();
  return {};
}

std::any IRGenerator::visit(const TypeDecl& stmt) {
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

std::any IRGenerator::visit(const RecordDecl& stmt) {
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

std::any IRGenerator::visit(const EnumDecl& stmt) {
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
std::any IRGenerator::visit(const BinaryExpr& expr) {
  if (expr.op.type == TokenType::AmpAmp || expr.op.type == TokenType::PipePipe) {
    auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
    auto end_label = new_label();

    if (expr.op.type == TokenType::AmpAmp) {
      // Short-circuit AND: lhs && rhs
      // Initialize false (0). If lhs is false, jump to end.
      // If rhs is false, jump to end. Else set true (1).
      tisc::ir::Instruction init_false;
      init_false.opcode = tisc::ir::Opcode::LOADI;
      init_false.operands = {dest.reg, tisc::ir::Immediate{0}};
      init_false.primitive = tisc::ir::PrimitiveKind::Boolean;
      init_false.literal_kind = tisc::LiteralKind::Bool;
      emit(init_false);

      expr.left->accept(*this);
      auto left = ensure_expr_result(expr.left.get());
      emit_jump_if_zero(end_label, left);

      expr.right->accept(*this);
      auto right = ensure_expr_result(expr.right.get());
      emit_jump_if_zero(end_label, right);

      tisc::ir::Instruction set_true;
      set_true.opcode = tisc::ir::Opcode::LOADI;
      set_true.operands = {dest.reg, tisc::ir::Immediate{1}};
      set_true.primitive = tisc::ir::PrimitiveKind::Boolean;
      set_true.literal_kind = tisc::LiteralKind::Bool;
      emit(set_true);
    } else {
      // Short-circuit OR: lhs || rhs
      // Initialize true (1). If lhs is true, jump to end.
      // If rhs is true, jump to end. Else set false (0).
      tisc::ir::Instruction init_true;
      init_true.opcode = tisc::ir::Opcode::LOADI;
      init_true.operands = {dest.reg, tisc::ir::Immediate{1}};
      init_true.primitive = tisc::ir::PrimitiveKind::Boolean;
      init_true.literal_kind = tisc::LiteralKind::Bool;
      emit(init_true);

      expr.left->accept(*this);
      auto left = ensure_expr_result(expr.left.get());
      emit_jump_if_not_zero(end_label, left);

      expr.right->accept(*this);
      auto right = ensure_expr_result(expr.right.get());
      emit_jump_if_not_zero(end_label, right);

      tisc::ir::Instruction set_false;
      set_false.opcode = tisc::ir::Opcode::LOADI;
      set_false.operands = {dest.reg, tisc::ir::Immediate{0}};
      set_false.primitive = tisc::ir::PrimitiveKind::Boolean;
      set_false.literal_kind = tisc::LiteralKind::Bool;
      emit(set_false);
    }

    emit_label(end_label);
    record_result(&expr, dest);
    return {};
  }

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
    case TokenType::Amp: {
      // Qutrit TAND = min(a, b) — route to TAnd IR opcode
      const Type* lt = typed_expr(expr.left.get());
      const Type* rt = typed_expr(expr.right.get());
      if ((lt && lt->kind == Type::Kind::Qutrit) || (rt && rt->kind == Type::Kind::Qutrit)) {
        opcode = O::TAND;
      } else {
        opcode = O::BITAND;
      }
      break;
    }
    case TokenType::Pipe: {
      // Qutrit TOR = max(a, b) — route to TOr IR opcode
      const Type* lt = typed_expr(expr.left.get());
      const Type* rt = typed_expr(expr.right.get());
      if ((lt && lt->kind == Type::Kind::Qutrit) || (rt && rt->kind == Type::Kind::Qutrit)) {
        opcode = O::TOR;
      } else {
        opcode = O::BITOR;
      }
      break;
    }
    case TokenType::Caret: {
      // Qutrit TXOR — route to TXor IR opcode
      const Type* lt = typed_expr(expr.left.get());
      const Type* rt = typed_expr(expr.right.get());
      if ((lt && lt->kind == Type::Kind::Qutrit) || (rt && rt->kind == Type::Kind::Qutrit)) {
        opcode = O::TXOR;
      } else {
        opcode = O::BITXOR;
      }
      break;
    }
    case TokenType::LessLess:
      opcode = O::BITSHL;
      break;
    case TokenType::GreaterGreater:
      opcode = O::BITSHR;
      break;
    case TokenType::GreaterGreaterGreater:
      opcode = O::BITUSHR;
      break;
    case TokenType::StarStar:
      if (kind == NumericCategory::Float) {
        opcode = O::FPOW;
      } else {
        // Check for Tensor/Matrix types
        bool is_tensor = false;
        if (result_type && (result_type->kind == Type::Kind::Tensor ||
                            result_type->kind == Type::Kind::Matrix)) {
          is_tensor = true;
        }
        if (is_tensor) {
          opcode = O::TMATMUL;
        } else {
          throw std::runtime_error(
              "Operator '**' not supported for this type (only Float power and Tensor/Matrix "
              "matmul supported)");
        }
      }
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

std::any IRGenerator::visit(const LiteralExpr& expr) {
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
  if (expr.value.type == TokenType::ByteString) {
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
                              : parse_canonical_float(expr.value.lexeme);
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
  if (expr.value.type == TokenType::T81Prob) {
    // T81Prob literal - treat as float for now
    const double parsed = parse_canonical_float(expr.value.lexeme);
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
  if (expr.value.type == TokenType::T81Fixed) {
    // T81Fixed literal - parse as fixed-point number
    const std::string lexeme(expr.value.lexeme);
    // Remove 'fx' suffix and parse as float
    std::string num_str = lexeme.substr(0, lexeme.length() - 2);
    const double parsed = std::stod(num_str);

    auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
    tisc::ir::Instruction instr;
    instr.opcode = tisc::ir::Opcode::LOADI;
    instr.operands = {dest.reg, tisc::ir::Immediate{static_cast<int64_t>(
                                    parsed * 1000)}};  // Fixed-point with 3 decimal places
    instr.primitive = tisc::ir::PrimitiveKind::Integer;
    emit(instr);
    record_result(&expr, dest);
    return {};
  }

  const Type* semantic_type = _semantic ? _semantic->type_of(&expr) : nullptr;
  const bool materialize_bigint =
      semantic_type != nullptr && semantic_type->kind == Type::Kind::BigInt &&
      (expr.value.type == TokenType::Integer || expr.value.type == TokenType::Base81Integer);

  auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
  if (materialize_bigint) {
    auto normalized = numeric_literals::normalize_decimal_integer_literal_text(
        expr.value.lexeme, expr.value.type == TokenType::Base81Integer);
    if (!normalized.has_value()) {
      throw std::runtime_error("BigInt literal lowering requires canonical decimal text.");
    }
    tisc::ir::Instruction instr;
    instr.opcode = tisc::ir::Opcode::LOADI;
    instr.operands = {dest.reg};
    instr.literal_kind = tisc::LiteralKind::BigIntHandle;
    instr.text_literal = *normalized;
    instr.primitive = tisc::ir::PrimitiveKind::Integer;
    emit(instr);
    record_result(&expr, dest);
    return {};
  }

  try {
    const int64_t value = (expr.value.type == TokenType::Base81Integer)
                              ? parse_base81_integer_literal(expr.value.lexeme)
                              : parse_integer_literal_raw(expr.value.lexeme);
    auto instr =
        tisc::ir::Instruction{tisc::ir::Opcode::LOADI, {dest.reg, tisc::ir::Immediate{value}}};
    instr.primitive = tisc::ir::PrimitiveKind::Integer;
    emit(instr);
    record_result(&expr, dest);
    return {};
  } catch (const std::out_of_range&) {
    auto normalized = numeric_literals::normalize_decimal_integer_literal_text(
        expr.value.lexeme, expr.value.type == TokenType::Base81Integer);
    if (!normalized.has_value()) {
      throw std::runtime_error("Integer literal exceeds 64-bit range and is not a supported "
                               "decimal form for BigInt literal lowering.");
    }
    tisc::ir::Instruction instr;
    instr.opcode = tisc::ir::Opcode::LOADI;
    instr.operands = {dest.reg};
    instr.literal_kind = tisc::LiteralKind::BigIntHandle;
    instr.text_literal = *normalized;
    instr.primitive = tisc::ir::PrimitiveKind::Integer;
    emit(instr);
    record_result(&expr, dest);
    return {};
  }
}

std::any IRGenerator::visit(const GroupingExpr& expr) {
  auto value = evaluate_expr(expr.expression.get());
  record_result(&expr, value);
  return {};
}

std::any IRGenerator::visit(const UnaryExpr& expr) {
  if (expr.op.type == TokenType::Bang) {
    // Check if operand is T81Qutrit — trit-NOT is integer NEG
    bool is_qutrit = false;
    if (_semantic) {
      const Type* sem_type = _semantic->type_of(expr.right.get());
      is_qutrit = sem_type && sem_type->kind == Type::Kind::Qutrit;
    }
    auto right = evaluate_expr(expr.right.get());
    if (is_qutrit) {
      // TNOT(t) = -t for t in {-1, 0, +1}; integer NEG achieves this.
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction neg;
      neg.opcode = tisc::ir::Opcode::NEG;
      neg.operands = {dest.reg, right.reg};
      neg.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(neg);
      record_result(&expr, dest);
      return {};
    }
    auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
    auto zero = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
    tisc::ir::Instruction load_zero;
    load_zero.opcode = tisc::ir::Opcode::LOADI;
    load_zero.operands = {zero.reg, tisc::ir::Immediate{0}};
    load_zero.primitive = tisc::ir::PrimitiveKind::Boolean;
    load_zero.literal_kind = tisc::LiteralKind::Bool;
    emit(load_zero);

    tisc::ir::Instruction cmp;
    cmp.opcode = tisc::ir::Opcode::CMP;
    cmp.operands = {dest.reg, right.reg, zero.reg};
    cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
    cmp.boolean_result = true;
    cmp.relation = tisc::ir::ComparisonRelation::Equal;
    emit(cmp);
    record_result(&expr, dest);
    return {};
  }

  auto right = evaluate_expr(expr.right.get());
  auto dest = allocate_typed_register(right.primitive);
  if (expr.op.type == TokenType::Minus) {
    // Float negation: 0.0 - right (TISC Neg is integer-only)
    if (right.primitive == tisc::ir::PrimitiveKind::Float) {
      auto zero = allocate_typed_register(tisc::ir::PrimitiveKind::Float);
      tisc::ir::Instruction load_zero;
      load_zero.opcode = tisc::ir::Opcode::LOADI;
      load_zero.operands = {zero.reg};
      load_zero.literal_kind = tisc::LiteralKind::FloatHandle;
      load_zero.text_literal = "0";
      load_zero.primitive = tisc::ir::PrimitiveKind::Float;
      emit(load_zero);
      tisc::ir::Instruction fsub;
      fsub.opcode = tisc::ir::Opcode::FSUB;
      fsub.operands = {dest.reg, zero.reg, right.reg};
      fsub.primitive = tisc::ir::PrimitiveKind::Float;
      emit(fsub);
      record_result(&expr, dest);
      return {};
    }
    // Fraction negation: 0/1 - right (TISC Neg is integer-only)
    if (right.primitive == tisc::ir::PrimitiveKind::Fraction) {
      auto zero_int = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction load_zero_int;
      load_zero_int.opcode = tisc::ir::Opcode::LOADI;
      load_zero_int.operands = {zero_int.reg, tisc::ir::Immediate{0}};
      load_zero_int.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(load_zero_int);
      auto zero_frac = allocate_typed_register(tisc::ir::PrimitiveKind::Fraction);
      tisc::ir::Instruction i2frac;
      i2frac.opcode = tisc::ir::Opcode::I2FRAC;
      i2frac.operands = {zero_frac.reg, zero_int.reg};
      i2frac.primitive = tisc::ir::PrimitiveKind::Fraction;
      emit(i2frac);
      tisc::ir::Instruction fracsub;
      fracsub.opcode = tisc::ir::Opcode::FRACSUB;
      fracsub.operands = {dest.reg, zero_frac.reg, right.reg};
      fracsub.primitive = tisc::ir::PrimitiveKind::Fraction;
      emit(fracsub);
      record_result(&expr, dest);
      return {};
    }
    auto instr = tisc::ir::Instruction{tisc::ir::Opcode::NEG, {dest.reg, right.reg}};
    instr.primitive = right.primitive;
    emit(instr);
  } else if (expr.op.type == TokenType::Tilde) {
    auto instr = tisc::ir::Instruction{tisc::ir::Opcode::BITNOT, {dest.reg, right.reg}};
    instr.primitive = right.primitive;
    emit(instr);
  } else {
    throw std::runtime_error("Unsupported unary operator");
  }
  record_result(&expr, dest);
  return {};
}

std::any IRGenerator::visit(const VariableExpr& expr) {
  auto found = lookup_variable(expr.name.lexeme);
  if (found.has_value()) {
    record_result(&expr, *found);
    return {};
  }
  std::string name{expr.name.lexeme};
  if (name == "None") {
    auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
    emit_make_option_none(dest);
    record_result(&expr, dest);
    return {};
  }
  return {};
}

std::any IRGenerator::visit(const CallExpr& expr) {
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
    if (func_name == "T81Bytes") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("T81Bytes conversion expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto value = ensure_expr_result(expr.arguments[0].get());

      tisc::ir::PrimitiveKind dest_kind = tisc::ir::PrimitiveKind::Unknown;
      if (func_name == "symbol_intern") {
        // Symbol intern returns an integer-like handle (Symbol), but the VM handles it.
        // If Type::Kind::Symbol is treated as Integer in IR generation (categorize_primitive),
        // then we should use Integer.
        dest_kind = tisc::ir::PrimitiveKind::Integer;
      }

      auto dest = allocate_typed_register(dest_kind);
      copy_to_dest(value, dest);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "T81Uint" || func_name == "T81Qutrit") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error(func_name + " conversion expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto value = ensure_expr_result(expr.arguments[0].get());
      auto integer_value = ensure_integer(value);
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      copy_to_dest(integer_value, dest);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "T81Maybe") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("T81Maybe constructor expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      emit_make_option_none(dest);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "T81Promise") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("T81Promise constructor expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::LOADI;
      instr.operands = {dest.reg};
      instr.literal_kind = tisc::LiteralKind::SymbolHandle;
      instr.text_literal = "std.async.promise";
      instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "T81Time") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("T81Time constructor expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Float);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::LOADI;
      instr.operands = {dest.reg};
      instr.literal_kind = tisc::LiteralKind::FloatHandle;
      instr.text_literal = "0";
      instr.primitive = tisc::ir::PrimitiveKind::Float;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "T81Entropy") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("T81Entropy constructor expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      auto instr =
          tisc::ir::Instruction{tisc::ir::Opcode::LOADI, {dest.reg, tisc::ir::Immediate{0}}};
      instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "T81Agent") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("T81Agent constructor expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::META_REFLECT;
      instr.operands = {dest.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "T81Polynomial" || func_name == "T81Symbolic") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error(func_name + " constructor expects no arguments.");
      }
      auto seed = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction seed_load;
      seed_load.opcode = tisc::ir::Opcode::LOADI;
      seed_load.operands = {seed.reg};
      seed_load.literal_kind = tisc::LiteralKind::SymbolHandle;
      seed_load.text_literal =
          (func_name == "T81Polynomial") ? "t81.poly.zero" : "t81.symbolic.root";
      seed_load.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(seed_load);

      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction sym_load;
      sym_load.opcode = tisc::ir::Opcode::SYMLOAD;
      sym_load.operands = {dest.reg, seed.reg};
      emit(sym_load);
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
    // RFC-0026 AI-M6: Tensor.attention(q, k, v) → ATTN dest, q, PACK(k, v)
    if (func_name == "Tensor.attention") {
      if (expr.arguments.size() != 3) {
        throw std::runtime_error("Tensor.attention expects exactly 3 arguments (q, k, v).");
      }
      expr.arguments[0]->accept(*this);
      auto q_reg = ensure_expr_result(expr.arguments[0].get());
      expr.arguments[1]->accept(*this);
      auto k_reg = ensure_expr_result(expr.arguments[1].get());
      expr.arguments[2]->accept(*this);
      auto v_reg = ensure_expr_result(expr.arguments[2].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      const int32_t packed_kv = (k_reg.reg.index & 0xFF) | ((v_reg.reg.index & 0xFF) << 8);
      tisc::ir::Instruction attn;
      attn.opcode = tisc::ir::Opcode::ATTN;
      attn.operands = {dest.reg, q_reg.reg, tisc::ir::Immediate{packed_kv}};
      emit(attn);
      record_result(&expr, dest);
      return {};
    }
    // RFC-0026 AI-M6: Tensor.qmatmul(act, wt, scale) → QMATMUL dest, act, PACK(wt, scale)
    if (func_name == "Tensor.qmatmul") {
      if (expr.arguments.size() != 3) {
        throw std::runtime_error("Tensor.qmatmul expects exactly 3 arguments (act, wt, scale).");
      }
      expr.arguments[0]->accept(*this);
      auto act_reg = ensure_expr_result(expr.arguments[0].get());
      expr.arguments[1]->accept(*this);
      auto wt_reg = ensure_expr_result(expr.arguments[1].get());
      expr.arguments[2]->accept(*this);
      auto scale_reg = ensure_expr_result(expr.arguments[2].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      const int32_t packed_ws = (wt_reg.reg.index & 0xFF) | ((scale_reg.reg.index & 0xFF) << 8);
      tisc::ir::Instruction qmm;
      qmm.opcode = tisc::ir::Opcode::QMATMUL;
      qmm.operands = {dest.reg, act_reg.reg, tisc::ir::Immediate{packed_ws}};
      emit(qmm);
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
        // Get semantic type of the object for dispatch
        const Type* obj_sem_type = nullptr;
        if (_semantic) {
          if (const auto* fa = dynamic_cast<const FieldAccessExpr*>(expr.callee.get())) {
            obj_sem_type = _semantic->type_of(fa->object.get());
          }
        }
        auto obj_kind = obj_sem_type ? obj_sem_type->kind : Type::Kind::Unknown;

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

        // .len() → VECLEN (Vector/Tensor) or STRLEN (String)
        if (method_name == "len" && expr.arguments.empty()) {
          auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
          tisc::ir::Instruction instr;
          if (obj_kind == Type::Kind::String || obj_kind == Type::Kind::Bytes) {
            instr.opcode = tisc::ir::Opcode::STRLEN;
          } else {
            instr.opcode = tisc::ir::Opcode::VECLEN;
          }
          instr.operands = {dest.reg, obj_reg->reg};
          emit(instr);
          record_result(&expr, dest);
          return {};
        }

        // .is_some() → OPTION_IS_SOME
        if (method_name == "is_some" && expr.arguments.empty()) {
          auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
          tisc::ir::Instruction instr;
          instr.opcode = tisc::ir::Opcode::OPTION_IS_SOME;
          instr.operands = {dest.reg, obj_reg->reg};
          emit(instr);
          record_result(&expr, dest);
          return {};
        }

        // .is_none() → OPTION_IS_SOME then logical NOT
        if (method_name == "is_none" && expr.arguments.empty()) {
          auto some_dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
          tisc::ir::Instruction some_instr;
          some_instr.opcode = tisc::ir::Opcode::OPTION_IS_SOME;
          some_instr.operands = {some_dest.reg, obj_reg->reg};
          emit(some_instr);
          // NOT: load 1, subtract (1 - is_some)
          auto one = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
          tisc::ir::Instruction load_one;
          load_one.opcode = tisc::ir::Opcode::LOADI;
          load_one.operands = {one.reg, tisc::ir::Immediate{1}};
          emit(load_one);
          auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
          tisc::ir::Instruction sub_instr;
          sub_instr.opcode = tisc::ir::Opcode::SUB;
          sub_instr.operands = {dest.reg, one.reg, some_dest.reg};
          emit(sub_instr);
          record_result(&expr, dest);
          return {};
        }

        // .is_ok() → RESULT_IS_OK
        if (method_name == "is_ok" && expr.arguments.empty()) {
          auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
          tisc::ir::Instruction instr;
          instr.opcode = tisc::ir::Opcode::RESULT_IS_OK;
          instr.operands = {dest.reg, obj_reg->reg};
          emit(instr);
          record_result(&expr, dest);
          return {};
        }

        // .is_err() → RESULT_IS_OK then NOT
        if (method_name == "is_err" && expr.arguments.empty()) {
          auto ok_dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
          tisc::ir::Instruction ok_instr;
          ok_instr.opcode = tisc::ir::Opcode::RESULT_IS_OK;
          ok_instr.operands = {ok_dest.reg, obj_reg->reg};
          emit(ok_instr);
          auto one = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
          tisc::ir::Instruction load_one;
          load_one.opcode = tisc::ir::Opcode::LOADI;
          load_one.operands = {one.reg, tisc::ir::Immediate{1}};
          emit(load_one);
          auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
          tisc::ir::Instruction sub_instr;
          sub_instr.opcode = tisc::ir::Opcode::SUB;
          sub_instr.operands = {dest.reg, one.reg, ok_dest.reg};
          emit(sub_instr);
          record_result(&expr, dest);
          return {};
        }

        // .unwrap() / .unwrap_ok() → OPTION_UNWRAP or RESULT_UNWRAP_OK
        if ((method_name == "unwrap" || method_name == "unwrap_ok") && expr.arguments.empty()) {
          auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
          tisc::ir::Instruction instr;
          if (obj_kind == Type::Kind::Result) {
            instr.opcode = tisc::ir::Opcode::RESULT_UNWRAP_OK;
          } else {
            instr.opcode = tisc::ir::Opcode::OPTION_UNWRAP;
          }
          instr.operands = {dest.reg, obj_reg->reg};
          emit(instr);
          record_result(&expr, dest);
          return {};
        }

        // .unwrap_err() → RESULT_UNWRAP_ERR
        if (method_name == "unwrap_err" && expr.arguments.empty() &&
            obj_kind == Type::Kind::Result) {
          auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
          tisc::ir::Instruction instr;
          instr.opcode = tisc::ir::Opcode::RESULT_UNWRAP_ERR;
          instr.operands = {dest.reg, obj_reg->reg};
          emit(instr);
          record_result(&expr, dest);
          return {};
        }

        // .unsigned_shr(n) → BITUSHR
        if (method_name == "unsigned_shr" && expr.arguments.size() == 1) {
          expr.arguments[0]->accept(*this);
          auto shift = ensure_expr_result(expr.arguments[0].get());
          auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
          tisc::ir::Instruction instr;
          instr.opcode = tisc::ir::Opcode::BITUSHR;
          instr.operands = {dest.reg, obj_reg->reg, shift.reg};
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

    if (func_name == "sin" || func_name == "cos" || func_name == "tan" || func_name == "asin" ||
        func_name == "acos" || func_name == "atan" || func_name == "sinh" ||
        func_name == "cosh" || func_name == "tanh") {
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
      else if (func_name == "tan")
        instr.opcode = tisc::ir::Opcode::FTAN;
      else if (func_name == "asin")
        instr.opcode = tisc::ir::Opcode::FASIN;
      else if (func_name == "acos")
        instr.opcode = tisc::ir::Opcode::FACOS;
      else if (func_name == "atan")
        instr.opcode = tisc::ir::Opcode::FATAN;
      else if (func_name == "sinh")
        instr.opcode = tisc::ir::Opcode::FSINH;
      else if (func_name == "cosh")
        instr.opcode = tisc::ir::Opcode::FCOSH;
      else
        instr.opcode = tisc::ir::Opcode::FTANH;
      instr.operands = {dest.reg, val.reg};
      instr.primitive = tisc::ir::PrimitiveKind::Float;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "exp" || func_name == "log" || func_name == "sqrt") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("Math functions expect 1 argument.");
      }
      expr.arguments[0]->accept(*this);
      auto val = ensure_expr_result(expr.arguments[0].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Float);
      tisc::ir::Instruction instr;
      if (func_name == "exp")
        instr.opcode = tisc::ir::Opcode::FEXP;
      else if (func_name == "log")
        instr.opcode = tisc::ir::Opcode::FLOG;
      else
        instr.opcode = tisc::ir::Opcode::FSQRT;
      instr.operands = {dest.reg, val.reg};
      instr.primitive = tisc::ir::PrimitiveKind::Float;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "pow") {
      if (expr.arguments.size() != 2) {
        throw std::runtime_error("pow expects exactly two arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      auto base = ensure_expr_result(expr.arguments[0].get());
      auto exponent = ensure_expr_result(expr.arguments[1].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Float);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::FPOW;
      instr.operands = {dest.reg, base.reg, exponent.reg};
      instr.primitive = tisc::ir::PrimitiveKind::Float;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "clamp") {
      if (expr.arguments.size() != 3) {
        throw std::runtime_error("clamp expects exactly three arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      expr.arguments[2]->accept(*this);
      auto value = ensure_expr_result(expr.arguments[0].get());
      auto minv = ensure_expr_result(expr.arguments[1].get());
      auto maxv = ensure_expr_result(expr.arguments[2].get());

      auto value_f = ensure_kind(value, tisc::ir::PrimitiveKind::Float);
      auto min_f = ensure_kind(minv, tisc::ir::PrimitiveKind::Float);
      auto max_f = ensure_kind(maxv, tisc::ir::PrimitiveKind::Float);

      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Float);
      copy_to_dest(value_f, dest);

      auto lt_min = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction cmp_low;
      cmp_low.opcode = tisc::ir::Opcode::CMP;
      cmp_low.operands = {lt_min.reg, value_f.reg, min_f.reg};
      cmp_low.primitive = tisc::ir::PrimitiveKind::Boolean;
      cmp_low.boolean_result = true;
      cmp_low.relation = tisc::ir::ComparisonRelation::Less;
      emit(cmp_low);

      auto check_high_label = new_label();
      auto end_label = new_label();
      emit_jump_if_zero(check_high_label, lt_min);
      copy_to_dest(min_f, dest);
      emit_jump(end_label);

      emit_label(check_high_label);
      auto gt_max = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction cmp_high;
      cmp_high.opcode = tisc::ir::Opcode::CMP;
      cmp_high.operands = {gt_max.reg, value_f.reg, max_f.reg};
      cmp_high.primitive = tisc::ir::PrimitiveKind::Boolean;
      cmp_high.boolean_result = true;
      cmp_high.relation = tisc::ir::ComparisonRelation::Greater;
      emit(cmp_high);

      emit_jump_if_zero(end_label, gt_max);
      copy_to_dest(max_f, dest);
      emit_label(end_label);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "abs") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("abs expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto val = ensure_expr_result(expr.arguments[0].get());
      auto dest = allocate_typed_register(val.primitive);

      if (val.primitive == tisc::ir::PrimitiveKind::Float) {
        auto zero = allocate_typed_register(tisc::ir::PrimitiveKind::Float);
        tisc::ir::Instruction load_zero;
        load_zero.opcode = tisc::ir::Opcode::LOADI;
        load_zero.operands = {zero.reg};
        load_zero.literal_kind = tisc::LiteralKind::FloatHandle;
        load_zero.text_literal = "0.0";
        load_zero.primitive = tisc::ir::PrimitiveKind::Float;
        emit(load_zero);

        auto is_neg = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
        tisc::ir::Instruction cmp;
        cmp.opcode = tisc::ir::Opcode::CMP;
        cmp.operands = {is_neg.reg, val.reg, zero.reg};
        cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
        cmp.boolean_result = true;
        cmp.relation = tisc::ir::ComparisonRelation::Less;
        emit(cmp);

        auto skip_neg = new_label();
        copy_to_dest(val, dest);
        emit_jump_if_zero(skip_neg, is_neg);

        tisc::ir::Instruction neg_instr;
        neg_instr.opcode = tisc::ir::Opcode::FSUB;
        neg_instr.operands = {dest.reg, zero.reg, val.reg};
        neg_instr.primitive = tisc::ir::PrimitiveKind::Float;
        emit(neg_instr);

        emit_label(skip_neg);
      } else {
        // Integer case
        auto zero = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        tisc::ir::Instruction load_zero;
        load_zero.opcode = tisc::ir::Opcode::LOADI;
        load_zero.operands = {zero.reg, tisc::ir::Immediate{0}};
        emit(load_zero);

        auto is_neg = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
        tisc::ir::Instruction cmp;
        cmp.opcode = tisc::ir::Opcode::CMP;
        cmp.operands = {is_neg.reg, val.reg, zero.reg};
        cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
        cmp.boolean_result = true;
        cmp.relation = tisc::ir::ComparisonRelation::Less;
        emit(cmp);

        auto skip_neg = new_label();
        copy_to_dest(val, dest);
        emit_jump_if_zero(skip_neg, is_neg);

        tisc::ir::Instruction neg_instr;
        neg_instr.opcode = tisc::ir::Opcode::NEG;
        neg_instr.operands = {dest.reg, val.reg};
        emit(neg_instr);

        emit_label(skip_neg);
      }
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "bigint_from_int" || func_name == "bigint_to_int") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("BigInt conversion expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto val = ensure_expr_result(expr.arguments[0].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      if (func_name == "bigint_from_int") {
        tisc::ir::Instruction conv;
        conv.opcode = tisc::ir::Opcode::INT2BIGINT;
        conv.operands = {dest.reg, val.reg};
        conv.primitive = tisc::ir::PrimitiveKind::Integer;
        conv.is_conversion = true;
        emit(conv);
      } else {
        auto narrowed = emit_checked_i32_narrow(val);
        copy_to_dest(narrowed, dest);
      }
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "bigint_add") {
      if (expr.arguments.size() != 2) {
        throw std::runtime_error("BigInt add expects exactly two arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      auto lhs = ensure_expr_result(expr.arguments[0].get());
      auto rhs = ensure_expr_result(expr.arguments[1].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::ADD;
      instr.operands = {dest.reg, lhs.reg, rhs.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "bigint_mul") {
      if (expr.arguments.size() != 2) {
        throw std::runtime_error("BigInt mul expects exactly two arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      auto lhs = ensure_expr_result(expr.arguments[0].get());
      auto rhs = ensure_expr_result(expr.arguments[1].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::MUL;
      instr.operands = {dest.reg, lhs.reg, rhs.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "tensor_dot") {
      if (expr.arguments.size() != 2) {
        throw std::runtime_error("tensor_dot expects exactly two arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      auto lhs = ensure_expr_result(expr.arguments[0].get());
      auto rhs = ensure_expr_result(expr.arguments[1].get());

      auto temp_tensor = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction dot_instr;
      dot_instr.opcode = tisc::ir::Opcode::TTENDOT;
      dot_instr.operands = {temp_tensor.reg, lhs.reg, rhs.reg};
      emit(dot_instr);

      auto zero = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction load_zero;
      load_zero.opcode = tisc::ir::Opcode::LOADI;
      load_zero.operands = {zero.reg, tisc::ir::Immediate{0}};
      emit(load_zero);

      auto float_res = allocate_typed_register(tisc::ir::PrimitiveKind::Float);
      tisc::ir::Instruction get_instr;
      get_instr.opcode = tisc::ir::Opcode::TGET;
      get_instr.operands = {float_res.reg, temp_tensor.reg, zero.reg};
      emit(get_instr);

      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction conv;
      conv.opcode = tisc::ir::Opcode::F2I;
      conv.operands = {dest.reg, float_res.reg};
      emit(conv);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "frac_add" || func_name == "frac_sub" || func_name == "frac_mul" ||
        func_name == "frac_div") {
      if (expr.arguments.size() != 2) {
        throw std::runtime_error("Fraction arithmetic expects exactly two arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      auto lhs = ensure_expr_result(expr.arguments[0].get());
      auto rhs = ensure_expr_result(expr.arguments[1].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Fraction);
      tisc::ir::Instruction instr;
      if (func_name == "frac_add")
        instr.opcode = tisc::ir::Opcode::FRACADD;
      else if (func_name == "frac_sub")
        instr.opcode = tisc::ir::Opcode::FRACSUB;
      else if (func_name == "frac_mul")
        instr.opcode = tisc::ir::Opcode::FRACMUL;
      else
        instr.opcode = tisc::ir::Opcode::FRACDIV;
      instr.operands = {dest.reg, lhs.reg, rhs.reg};
      instr.primitive = tisc::ir::PrimitiveKind::Fraction;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "frac_from_int") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("frac_from_int expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto val = ensure_expr_result(expr.arguments[0].get());
      auto int_val = ensure_integer(val);
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Fraction);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::I2FRAC;
      instr.operands = {dest.reg, int_val.reg};
      instr.primitive = tisc::ir::PrimitiveKind::Fraction;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "frac_to_int") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("frac_to_int expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto val = ensure_expr_result(expr.arguments[0].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::FRAC2I;
      instr.operands = {dest.reg, val.reg};
      instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "frac_from_float") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("frac_from_float expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto val = ensure_expr_result(expr.arguments[0].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Fraction);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::F2FRAC;
      instr.operands = {dest.reg, val.reg};
      instr.primitive = tisc::ir::PrimitiveKind::Fraction;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "frac_to_float") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("frac_to_float expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto val = ensure_expr_result(expr.arguments[0].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Float);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::FRAC2F;
      instr.operands = {dest.reg, val.reg};
      instr.primitive = tisc::ir::PrimitiveKind::Float;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "sys_exit") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("sys_exit expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      (void)ensure_expr_result(expr.arguments[0].get());
      emit_simple(tisc::ir::Opcode::TRAP);
      return {};
    }
    if (func_name == "sys_time") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("sys_time expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Float);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::LOADI;
      instr.operands = {dest.reg};
      instr.literal_kind = tisc::LiteralKind::FloatHandle;
      instr.text_literal = "0";
      instr.primitive = tisc::ir::PrimitiveKind::Float;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "sys_entropy") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("sys_entropy expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      auto instr =
          tisc::ir::Instruction{tisc::ir::Opcode::LOADI, {dest.reg, tisc::ir::Immediate{0}}};
      instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "sys_proof") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("sys_proof expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::LOADI;
      instr.operands = {dest.reg};
      instr.literal_kind = tisc::LiteralKind::SymbolHandle;
      instr.text_literal = "std.sys.proof";
      instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "sys_reflect") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("sys_reflect expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::META_REFLECT;
      instr.operands = {dest.reg};
      emit(instr);
      return {};
    }
    if (func_name == "async_yield") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("async_yield expects no arguments.");
      }
      return {};
    }
    if (func_name == "async_sleep") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("async_sleep expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      (void)ensure_expr_result(expr.arguments[0].get());
      return {};
    }
    if (func_name == "async_thread") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("async_thread expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::LOADI;
      instr.operands = {dest.reg};
      instr.literal_kind = tisc::LiteralKind::SymbolHandle;
      instr.text_literal = "std.async.thread";
      instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "async_promise") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("async_promise expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::LOADI;
      instr.operands = {dest.reg};
      instr.literal_kind = tisc::LiteralKind::SymbolHandle;
      instr.text_literal = "std.async.promise";
      instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "agent_self_reflect") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("agent_self_reflect expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::META_REFLECT;
      instr.operands = {dest.reg};
      emit(instr);
      return {};
    }
    if (func_name == "symbolic_load" || func_name == "polynomial_load") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error(func_name + " expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto seed = ensure_expr_result(expr.arguments[0].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::SYMLOAD;
      instr.operands = {dest.reg, seed.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "symbolic_rewrite" || func_name == "polynomial_rewrite") {
      if (expr.arguments.size() != 3) {
        throw std::runtime_error(func_name + " expects exactly three arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      expr.arguments[2]->accept(*this);
      auto graph = ensure_expr_result(expr.arguments[0].get());
      auto match = ensure_expr_result(expr.arguments[1].get());
      auto repl = ensure_expr_result(expr.arguments[2].get());
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::SYMREWRITE;
      instr.operands = {graph.reg, match.reg, repl.reg};
      emit(instr);
      record_result(&expr, graph);
      return {};
    }
    if (func_name == "symbolic_canon" || func_name == "polynomial_canon") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error(func_name + " expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto graph = ensure_expr_result(expr.arguments[0].get());
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::SYMCANON;
      instr.operands = {graph.reg};
      emit(instr);
      record_result(&expr, graph);
      return {};
    }
    if (func_name == "symbolic_confluent" || func_name == "polynomial_confluent") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error(func_name + " expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto graph = ensure_expr_result(expr.arguments[0].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::SYMCONFLUENCE;
      instr.operands = {dest.reg, graph.reg};
      instr.primitive = tisc::ir::PrimitiveKind::Boolean;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }

    if (func_name == "dist_gossip") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("dist_gossip expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto value = ensure_expr_result(expr.arguments[0].get());
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::GOSSIP;
      instr.operands = {tisc::ir::Register{0}, value.reg};
      emit(instr);
      return {};
    }
    if (func_name == "dist_merge") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("dist_merge expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::MERGE;
      instr.operands = {dest.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "dist_sync") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("dist_sync expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto val = ensure_expr_result(expr.arguments[0].get());
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::TICKSYNC;
      instr.operands = {val.reg};
      emit(instr);
      return {};
    }
    if (func_name == "dist_coherence") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("dist_coherence expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::COHERENCE;
      instr.operands = {dest.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "dist_seal") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("dist_seal expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::DISTSEAL;
      instr.operands = {dest.reg};
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
    if (func_name == "io_stream" || func_name == "io_net") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error(func_name + " expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::LOADI;
      instr.operands = {dest.reg};
      instr.literal_kind = tisc::LiteralKind::SymbolHandle;
      instr.text_literal = (func_name == "io_stream") ? "std.io.stream" : "std.io.net";
      instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "core_assert") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("core_assert expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto cond = ensure_expr_result(expr.arguments[0].get());
      auto pass_label = new_label();
      emit_jump_if_not_zero(pass_label, cond);
      emit_simple(tisc::ir::Opcode::TRAP);
      emit_label(pass_label);
      return {};
    }
    if (func_name == "option_unwrap_or") {
      if (expr.arguments.size() != 2) {
        throw std::runtime_error("option_unwrap_or expects exactly two arguments.");
      }
      expr.arguments[0]->accept(*this);
      auto opt = ensure_expr_result(expr.arguments[0].get());
      expr.arguments[1]->accept(*this);
      auto fallback = ensure_expr_result(expr.arguments[1].get());

      auto cond = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      emit_option_is_some(cond, opt);

      tisc::ir::PrimitiveKind dest_kind = fallback.primitive;
      if (const auto* ty = typed_expr(&expr)) {
        auto inferred = categorize_primitive(ty);
        if (inferred != tisc::ir::PrimitiveKind::Unknown) {
          dest_kind = inferred;
        }
      }
      auto dest = allocate_typed_register(dest_kind);
      copy_to_dest(fallback, dest);

      auto use_fallback = new_label();
      emit_jump_if_zero(use_fallback, cond);
      auto payload = allocate_typed_register(dest_kind);
      emit_option_unwrap(payload, opt);
      copy_to_dest(payload, dest);
      emit_label(use_fallback);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "option_is_some") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("option_is_some expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto val = ensure_expr_result(expr.arguments[0].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      emit_option_is_some(dest, val);
      // VM's OptionIsSome returns 1 or 0 (ValueTag::Int).
      // Since we allocated a Boolean register, we accept the int result as bool representation.
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "option_is_none") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("option_is_none expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto val = ensure_expr_result(expr.arguments[0].get());
      auto is_some = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      emit_option_is_some(is_some, val);

      auto zero = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction load_zero;
      load_zero.opcode = tisc::ir::Opcode::LOADI;
      load_zero.operands = {zero.reg, tisc::ir::Immediate{0}};
      emit(load_zero);

      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction cmp;
      cmp.opcode = tisc::ir::Opcode::CMP;
      cmp.operands = {dest.reg, is_some.reg, zero.reg};
      cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
      cmp.boolean_result = true;
      cmp.relation = tisc::ir::ComparisonRelation::Equal;
      emit(cmp);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "option_unwrap") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("option_unwrap expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto val = ensure_expr_result(expr.arguments[0].get());

      tisc::ir::PrimitiveKind dest_kind = tisc::ir::PrimitiveKind::Unknown;
      if (const auto* ty = typed_expr(&expr)) {
        auto inferred = categorize_primitive(ty);
        if (inferred != tisc::ir::PrimitiveKind::Unknown) {
          dest_kind = inferred;
        }
      }
      auto dest = allocate_typed_register(dest_kind);
      emit_option_unwrap(dest, val);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "result_is_ok") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("result_is_ok expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto val = ensure_expr_result(expr.arguments[0].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      emit_result_is_ok(dest, val);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "result_is_err") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("result_is_err expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto val = ensure_expr_result(expr.arguments[0].get());
      auto is_ok = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      emit_result_is_ok(is_ok, val);

      auto zero = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction load_zero;
      load_zero.opcode = tisc::ir::Opcode::LOADI;
      load_zero.operands = {zero.reg, tisc::ir::Immediate{0}};
      emit(load_zero);

      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction cmp;
      cmp.opcode = tisc::ir::Opcode::CMP;
      cmp.operands = {dest.reg, is_ok.reg, zero.reg};
      cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
      cmp.boolean_result = true;
      cmp.relation = tisc::ir::ComparisonRelation::Equal;
      emit(cmp);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "result_unwrap") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("result_unwrap expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto val = ensure_expr_result(expr.arguments[0].get());

      tisc::ir::PrimitiveKind dest_kind = tisc::ir::PrimitiveKind::Unknown;
      if (const auto* ty = typed_expr(&expr)) {
        auto inferred = categorize_primitive(ty);
        if (inferred != tisc::ir::PrimitiveKind::Unknown) {
          dest_kind = inferred;
        }
      }
      auto dest = allocate_typed_register(dest_kind);
      emit_result_unwrap_ok(dest, val);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "result_unwrap_err") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("result_unwrap_err expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto val = ensure_expr_result(expr.arguments[0].get());

      tisc::ir::PrimitiveKind dest_kind = tisc::ir::PrimitiveKind::Unknown;
      if (const auto* ty = typed_expr(&expr)) {
        auto inferred = categorize_primitive(ty);
        if (inferred != tisc::ir::PrimitiveKind::Unknown) {
          dest_kind = inferred;
        }
      }
      auto dest = allocate_typed_register(dest_kind);
      emit_result_unwrap_err(dest, val);
      record_result(&expr, dest);
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
    if (func_name == "str_to_string") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("str_to_string expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto value = ensure_expr_result(expr.arguments[0].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      copy_to_dest(value, dest);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "str_split") {
      if (expr.arguments.size() != 2) {
        throw std::runtime_error("str_split expects exactly two arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      auto value = ensure_expr_result(expr.arguments[0].get());
      auto sep = ensure_expr_result(expr.arguments[1].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::STRSPLIT;
      instr.operands = {dest.reg, value.reg, sep.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "str_join") {
      if (expr.arguments.size() != 2) {
        throw std::runtime_error("str_join expects exactly two arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      auto parts = ensure_expr_result(expr.arguments[0].get());
      auto sep = ensure_expr_result(expr.arguments[1].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::STRJOIN;
      instr.operands = {dest.reg, parts.reg, sep.reg};
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
    if (func_name == "bytes_split") {
      if (expr.arguments.size() != 2) {
        throw std::runtime_error("bytes_split expects exactly two arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      auto value = ensure_expr_result(expr.arguments[0].get());
      auto sep = ensure_expr_result(expr.arguments[1].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::STRSPLIT;
      instr.operands = {dest.reg, value.reg, sep.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "bytes_join") {
      if (expr.arguments.size() != 2) {
        throw std::runtime_error("bytes_join expects exactly two arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      auto parts = ensure_expr_result(expr.arguments[0].get());
      auto sep = ensure_expr_result(expr.arguments[1].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::STRJOIN;
      instr.operands = {dest.reg, parts.reg, sep.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_len") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("collections_len expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto value = ensure_expr_result(expr.arguments[0].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::VECLEN;
      instr.operands = {dest.reg, value.reg};
      instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_is_empty") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("collections_is_empty expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto value = ensure_expr_result(expr.arguments[0].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::VECEMPTY;
      instr.operands = {dest.reg, value.reg};
      instr.primitive = tisc::ir::PrimitiveKind::Boolean;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_first") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("collections_first expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto value = ensure_expr_result(expr.arguments[0].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::VECFIRST;
      instr.operands = {dest.reg, value.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_last") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("collections_last expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto value = ensure_expr_result(expr.arguments[0].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::VECLAST;
      instr.operands = {dest.reg, value.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_push") {
      if (expr.arguments.size() != 2) {
        throw std::runtime_error("collections_push expects exactly two arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      auto value = ensure_expr_result(expr.arguments[0].get());
      auto item = ensure_expr_result(expr.arguments[1].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::VECPUSH;
      instr.operands = {dest.reg, value.reg, item.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_pop") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("collections_pop expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto value = ensure_expr_result(expr.arguments[0].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::VECPOP;
      instr.operands = {dest.reg, value.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_list") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("collections_list expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction vec_new;
      vec_new.opcode = tisc::ir::Opcode::STRVECNEW;
      vec_new.operands = {dest.reg};
      emit(vec_new);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_map") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("collections_map expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::MapNew;
      instr.operands = {dest.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_map_size") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("collections_map_size expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto map_vec = ensure_expr_result(expr.arguments[0].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::MapSize;
      instr.operands = {dest.reg, map_vec.reg};
      instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_map_has") {
      if (expr.arguments.size() != 2) {
        throw std::runtime_error("collections_map_has expects exactly two arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      auto map_vec = ensure_expr_result(expr.arguments[0].get());
      auto needle = ensure_expr_result(expr.arguments[1].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::MapHas;
      instr.operands = {dest.reg, map_vec.reg, needle.reg};
      instr.primitive = tisc::ir::PrimitiveKind::Boolean;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_map_get") {
      if (expr.arguments.size() != 2) {
        throw std::runtime_error("collections_map_get expects exactly two arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      auto map_vec = ensure_expr_result(expr.arguments[0].get());
      auto needle = ensure_expr_result(expr.arguments[1].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::MapGet;
      instr.operands = {dest.reg, map_vec.reg, needle.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_map_remove") {
      if (expr.arguments.size() != 2) {
        throw std::runtime_error("collections_map_remove expects exactly two arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      auto map_vec = ensure_expr_result(expr.arguments[0].get());
      auto needle = ensure_expr_result(expr.arguments[1].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::MapRemove;
      instr.operands = {dest.reg, map_vec.reg, needle.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_map_put") {
      if (expr.arguments.size() != 3) {
        throw std::runtime_error("collections_map_put expects exactly three arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      expr.arguments[2]->accept(*this);
      auto map_vec = ensure_expr_result(expr.arguments[0].get());
      auto key = ensure_expr_result(expr.arguments[1].get());
      auto value = ensure_expr_result(expr.arguments[2].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::MapPut;
      instr.operands = {dest.reg, map_vec.reg, key.reg,
                        value.reg};  // VM implementation assumes map is in A? No, wait.
      // My VM implementation used: A=Map, B=Key, C=Val. But wait, `MapPut` in my VM
      // implementation uses `ctx.registers[insn.a]` as map handle. TISC instructions generally
      // write to A. If MapPut modifies in place, it should return the map handle. My VM
      // implementation:
      //   auto* vec = string_vector_mut(ctx.registers[insn.a]);
      // So A is both input and output (in-place modification).
      // BUT `tisc::ir::Instruction` supports A, B, C.
      // If I emit operands {map_vec.reg, key.reg, value.reg}, then A=map, B=key, C=val.
      // And I allocate `dest` but don't use it in operands?
      // Ah, `copy_to_dest(map_vec, dest)` if I want a new register?
      // If I use map_vec directly as A, it modifies the register that holds the map handle.
      // That's fine since it's a handle. However, standard IR generation flow: allocate dest
      // register, emit instruction writing to dest. If I want `dest = MapPut(map, key, val)`
      // where `dest` is a new alias to the map? VM implementation: `ctx.registers[insn.a]` is
      // modified? No, `ctx.registers[insn.a]` holds the handle. `string_vector_mut` gets the
      // vector *pointed to* by the handle. So `ctx.registers[insn.a]` is NOT modified (the handle
      // is the same). The *vector* is modified. So: `MapPut` takes `map` in A. But if I want
      // `dest` to be the result of the expression (which is the map), I should probably copy
      // `map` to `dest` first, then operate on `dest`. Or make MapPut have 3 operands: Dest, Map,
      // Key, Value? No, TISC is limited to 3 operands (A, B, C). So: MapPut(Map, Key, Value) ->
      // modifies Map. Returns Map handle. If I want to support chaining or assignment, I need the
      // result in a register. Let's assume MapPut modifies `A` (the map handle reg) in place
      // (conceptually the object it points to). But what if I need `dest` to be a *different*
      // register from `map_vec`? `copy_to_dest(map_vec, dest);` then `MapPut(dest, key, value)`.

      copy_to_dest(map_vec, dest);
      instr.operands = {dest.reg, key.reg, value.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_map_keys") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("collections_map_keys expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto map_vec = ensure_expr_result(expr.arguments[0].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::MapKeys;
      instr.operands = {dest.reg, map_vec.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_set") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("collections_set expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::SetNew;
      instr.operands = {dest.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_set_size") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("collections_set_size expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto set_vec = ensure_expr_result(expr.arguments[0].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::SetSize;
      instr.operands = {dest.reg, set_vec.reg};
      instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_set_has") {
      if (expr.arguments.size() != 2) {
        throw std::runtime_error("collections_set_has expects exactly two arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      auto set_vec = ensure_expr_result(expr.arguments[0].get());
      auto needle = ensure_expr_result(expr.arguments[1].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::SetHas;
      instr.operands = {dest.reg, set_vec.reg, needle.reg};
      instr.primitive = tisc::ir::PrimitiveKind::Boolean;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_set_add") {
      if (expr.arguments.size() != 2) {
        throw std::runtime_error("collections_set_add expects exactly two arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      auto set_vec = ensure_expr_result(expr.arguments[0].get());
      auto needle = ensure_expr_result(expr.arguments[1].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      copy_to_dest(set_vec, dest);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::SetAdd;
      instr.operands = {dest.reg, needle.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_set_remove") {
      if (expr.arguments.size() != 2) {
        throw std::runtime_error("collections_set_remove expects exactly two arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      auto set_vec = ensure_expr_result(expr.arguments[0].get());
      auto needle = ensure_expr_result(expr.arguments[1].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      copy_to_dest(set_vec, dest);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::SetRemove;
      instr.operands = {dest.reg, needle.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_tree" || func_name == "collections_graph") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("collections container constructors expect no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction vec_new;
      vec_new.opcode = tisc::ir::Opcode::STRVECNEW;
      vec_new.operands = {dest.reg};
      emit(vec_new);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_graph_edge_count") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("collections_graph_edge_count expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto graph_vec = ensure_expr_result(expr.arguments[0].get());
      auto raw_len = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction len_instr;
      len_instr.opcode = tisc::ir::Opcode::VECLEN;
      len_instr.operands = {raw_len.reg, graph_vec.reg};
      len_instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(len_instr);

      auto two = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction load_two;
      load_two.opcode = tisc::ir::Opcode::LOADI;
      load_two.operands = {two.reg, tisc::ir::Immediate{2}};
      load_two.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(load_two);

      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction div_instr;
      div_instr.opcode = tisc::ir::Opcode::DIV;
      div_instr.operands = {dest.reg, raw_len.reg, two.reg};
      div_instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(div_instr);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "collections_graph_has_edge") {
      if (expr.arguments.size() != 3) {
        throw std::runtime_error("collections_graph_has_edge expects exactly three arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      expr.arguments[2]->accept(*this);
      auto graph_vec = ensure_expr_result(expr.arguments[0].get());
      auto from = ensure_expr_result(expr.arguments[1].get());
      auto to = ensure_expr_result(expr.arguments[2].get());

      auto two = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction load_two;
      load_two.opcode = tisc::ir::Opcode::LOADI;
      load_two.operands = {two.reg, tisc::ir::Immediate{2}};
      load_two.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(load_two);

      auto zero = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction load_zero;
      load_zero.opcode = tisc::ir::Opcode::LOADI;
      load_zero.operands = {zero.reg, tisc::ir::Immediate{0}};
      load_zero.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(load_zero);

      auto work = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      copy_to_dest(graph_vec, work);

      auto raw_len = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction len_instr;
      len_instr.opcode = tisc::ir::Opcode::VECLEN;
      len_instr.operands = {raw_len.reg, work.reg};
      len_instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(len_instr);

      auto rem = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction mod_instr;
      mod_instr.opcode = tisc::ir::Opcode::MOD;
      mod_instr.operands = {rem.reg, raw_len.reg, two.reg};
      mod_instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(mod_instr);

      auto has_odd_tail = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction odd_cmp;
      odd_cmp.opcode = tisc::ir::Opcode::CMP;
      odd_cmp.operands = {has_odd_tail.reg, rem.reg, zero.reg};
      odd_cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
      odd_cmp.boolean_result = true;
      odd_cmp.relation = tisc::ir::ComparisonRelation::NotEqual;
      emit(odd_cmp);

      auto trimmed_label = new_label();
      emit_jump_if_zero(trimmed_label, has_odd_tail);
      auto trimmed = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction trim_instr;
      trim_instr.opcode = tisc::ir::Opcode::VECPOP;
      trim_instr.operands = {trimmed.reg, work.reg};
      emit(trim_instr);
      copy_to_dest(trimmed, work);
      emit_label(trimmed_label);

      auto result = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction init_false;
      init_false.opcode = tisc::ir::Opcode::LOADI;
      init_false.operands = {result.reg, tisc::ir::Immediate{0}};
      init_false.primitive = tisc::ir::PrimitiveKind::Boolean;
      init_false.literal_kind = tisc::LiteralKind::Bool;
      emit(init_false);

      auto loop_label = new_label();
      auto found_label = new_label();
      auto end_label = new_label();

      emit_label(loop_label);
      auto cur_len = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction cur_len_instr;
      cur_len_instr.opcode = tisc::ir::Opcode::VECLEN;
      cur_len_instr.operands = {cur_len.reg, work.reg};
      cur_len_instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(cur_len_instr);

      auto has_pair = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction pair_cmp;
      pair_cmp.opcode = tisc::ir::Opcode::CMP;
      pair_cmp.operands = {has_pair.reg, cur_len.reg, two.reg};
      pair_cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
      pair_cmp.boolean_result = true;
      pair_cmp.relation = tisc::ir::ComparisonRelation::GreaterEqual;
      emit(pair_cmp);
      emit_jump_if_zero(end_label, has_pair);

      auto to_value = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction to_last;
      to_last.opcode = tisc::ir::Opcode::VECLAST;
      to_last.operands = {to_value.reg, work.reg};
      emit(to_last);

      auto no_to = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction pop_to;
      pop_to.opcode = tisc::ir::Opcode::VECPOP;
      pop_to.operands = {no_to.reg, work.reg};
      emit(pop_to);

      auto from_value = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction from_last;
      from_last.opcode = tisc::ir::Opcode::VECLAST;
      from_last.operands = {from_value.reg, no_to.reg};
      emit(from_last);

      auto no_pair = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction pop_from;
      pop_from.opcode = tisc::ir::Opcode::VECPOP;
      pop_from.operands = {no_pair.reg, no_to.reg};
      emit(pop_from);

      auto from_match = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction from_cmp;
      from_cmp.opcode = tisc::ir::Opcode::CMP;
      from_cmp.operands = {from_match.reg, from_value.reg, from.reg};
      from_cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
      from_cmp.boolean_result = true;
      from_cmp.relation = tisc::ir::ComparisonRelation::Equal;
      emit(from_cmp);

      auto check_to_label = new_label();
      emit_jump_if_not_zero(check_to_label, from_match);
      copy_to_dest(no_pair, work);
      emit_jump(loop_label);

      emit_label(check_to_label);
      auto to_match = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction to_cmp;
      to_cmp.opcode = tisc::ir::Opcode::CMP;
      to_cmp.operands = {to_match.reg, to_value.reg, to.reg};
      to_cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
      to_cmp.boolean_result = true;
      to_cmp.relation = tisc::ir::ComparisonRelation::Equal;
      emit(to_cmp);
      emit_jump_if_not_zero(found_label, to_match);

      copy_to_dest(no_pair, work);
      emit_jump(loop_label);

      emit_label(found_label);
      tisc::ir::Instruction set_true;
      set_true.opcode = tisc::ir::Opcode::LOADI;
      set_true.operands = {result.reg, tisc::ir::Immediate{1}};
      set_true.primitive = tisc::ir::PrimitiveKind::Boolean;
      set_true.literal_kind = tisc::LiteralKind::Bool;
      emit(set_true);

      emit_label(end_label);
      record_result(&expr, result);
      return {};
    }
    if (func_name == "collections_graph_add_edge") {
      if (expr.arguments.size() != 3) {
        throw std::runtime_error("collections_graph_add_edge expects exactly three arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      expr.arguments[2]->accept(*this);
      auto graph_vec = ensure_expr_result(expr.arguments[0].get());
      auto from = ensure_expr_result(expr.arguments[1].get());
      auto to = ensure_expr_result(expr.arguments[2].get());

      auto two = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction load_two;
      load_two.opcode = tisc::ir::Opcode::LOADI;
      load_two.operands = {two.reg, tisc::ir::Immediate{2}};
      load_two.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(load_two);

      auto zero = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction load_zero;
      load_zero.opcode = tisc::ir::Opcode::LOADI;
      load_zero.operands = {zero.reg, tisc::ir::Immediate{0}};
      load_zero.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(load_zero);

      auto work = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      copy_to_dest(graph_vec, work);
      auto result = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      copy_to_dest(graph_vec, result);

      auto raw_len = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction len_instr;
      len_instr.opcode = tisc::ir::Opcode::VECLEN;
      len_instr.operands = {raw_len.reg, work.reg};
      len_instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(len_instr);

      auto rem = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction mod_instr;
      mod_instr.opcode = tisc::ir::Opcode::MOD;
      mod_instr.operands = {rem.reg, raw_len.reg, two.reg};
      mod_instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(mod_instr);

      auto has_odd_tail = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction odd_cmp;
      odd_cmp.opcode = tisc::ir::Opcode::CMP;
      odd_cmp.operands = {has_odd_tail.reg, rem.reg, zero.reg};
      odd_cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
      odd_cmp.boolean_result = true;
      odd_cmp.relation = tisc::ir::ComparisonRelation::NotEqual;
      emit(odd_cmp);

      auto trimmed_label = new_label();
      emit_jump_if_zero(trimmed_label, has_odd_tail);
      auto trimmed = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction trim_instr;
      trim_instr.opcode = tisc::ir::Opcode::VECPOP;
      trim_instr.operands = {trimmed.reg, work.reg};
      emit(trim_instr);
      copy_to_dest(trimmed, work);
      emit_label(trimmed_label);

      auto found = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction init_false;
      init_false.opcode = tisc::ir::Opcode::LOADI;
      init_false.operands = {found.reg, tisc::ir::Immediate{0}};
      init_false.primitive = tisc::ir::PrimitiveKind::Boolean;
      init_false.literal_kind = tisc::LiteralKind::Bool;
      emit(init_false);

      auto loop_label = new_label();
      auto found_label = new_label();
      auto end_scan_label = new_label();
      emit_label(loop_label);

      auto cur_len = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction cur_len_instr;
      cur_len_instr.opcode = tisc::ir::Opcode::VECLEN;
      cur_len_instr.operands = {cur_len.reg, work.reg};
      cur_len_instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(cur_len_instr);

      auto has_pair = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction pair_cmp;
      pair_cmp.opcode = tisc::ir::Opcode::CMP;
      pair_cmp.operands = {has_pair.reg, cur_len.reg, two.reg};
      pair_cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
      pair_cmp.boolean_result = true;
      pair_cmp.relation = tisc::ir::ComparisonRelation::GreaterEqual;
      emit(pair_cmp);
      emit_jump_if_zero(end_scan_label, has_pair);

      auto to_value = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction to_last;
      to_last.opcode = tisc::ir::Opcode::VECLAST;
      to_last.operands = {to_value.reg, work.reg};
      emit(to_last);

      auto no_to = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction pop_to;
      pop_to.opcode = tisc::ir::Opcode::VECPOP;
      pop_to.operands = {no_to.reg, work.reg};
      emit(pop_to);

      auto from_value = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction from_last;
      from_last.opcode = tisc::ir::Opcode::VECLAST;
      from_last.operands = {from_value.reg, no_to.reg};
      emit(from_last);

      auto no_pair = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction pop_from;
      pop_from.opcode = tisc::ir::Opcode::VECPOP;
      pop_from.operands = {no_pair.reg, no_to.reg};
      emit(pop_from);

      auto from_match = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction from_cmp;
      from_cmp.opcode = tisc::ir::Opcode::CMP;
      from_cmp.operands = {from_match.reg, from_value.reg, from.reg};
      from_cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
      from_cmp.boolean_result = true;
      from_cmp.relation = tisc::ir::ComparisonRelation::Equal;
      emit(from_cmp);

      auto check_to_label = new_label();
      emit_jump_if_not_zero(check_to_label, from_match);
      copy_to_dest(no_pair, work);
      emit_jump(loop_label);

      emit_label(check_to_label);
      auto to_match = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction to_cmp;
      to_cmp.opcode = tisc::ir::Opcode::CMP;
      to_cmp.operands = {to_match.reg, to_value.reg, to.reg};
      to_cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
      to_cmp.boolean_result = true;
      to_cmp.relation = tisc::ir::ComparisonRelation::Equal;
      emit(to_cmp);
      emit_jump_if_not_zero(found_label, to_match);

      copy_to_dest(no_pair, work);
      emit_jump(loop_label);

      emit_label(found_label);
      tisc::ir::Instruction set_true;
      set_true.opcode = tisc::ir::Opcode::LOADI;
      set_true.operands = {found.reg, tisc::ir::Immediate{1}};
      set_true.primitive = tisc::ir::PrimitiveKind::Boolean;
      set_true.literal_kind = tisc::LiteralKind::Bool;
      emit(set_true);

      emit_label(end_scan_label);
      auto done_label = new_label();
      emit_jump_if_not_zero(done_label, found);

      auto with_from = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction push_from;
      push_from.opcode = tisc::ir::Opcode::VECPUSH;
      push_from.operands = {with_from.reg, result.reg, from.reg};
      emit(push_from);
      copy_to_dest(with_from, result);

      auto with_to = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction push_to;
      push_to.opcode = tisc::ir::Opcode::VECPUSH;
      push_to.operands = {with_to.reg, result.reg, to.reg};
      emit(push_to);
      copy_to_dest(with_to, result);

      emit_label(done_label);
      record_result(&expr, result);
      return {};
    }
    if (func_name == "collections_graph_remove_edge") {
      if (expr.arguments.size() != 3) {
        throw std::runtime_error(
            "collections_graph_remove_edge expects exactly three arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      expr.arguments[2]->accept(*this);
      auto graph_vec = ensure_expr_result(expr.arguments[0].get());
      auto from = ensure_expr_result(expr.arguments[1].get());
      auto to = ensure_expr_result(expr.arguments[2].get());

      auto two = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction load_two;
      load_two.opcode = tisc::ir::Opcode::LOADI;
      load_two.operands = {two.reg, tisc::ir::Immediate{2}};
      load_two.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(load_two);

      auto zero = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction load_zero;
      load_zero.opcode = tisc::ir::Opcode::LOADI;
      load_zero.operands = {zero.reg, tisc::ir::Immediate{0}};
      load_zero.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(load_zero);

      auto work = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      copy_to_dest(graph_vec, work);

      auto raw_len = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction len_instr;
      len_instr.opcode = tisc::ir::Opcode::VECLEN;
      len_instr.operands = {raw_len.reg, work.reg};
      len_instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(len_instr);

      auto rem = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction mod_instr;
      mod_instr.opcode = tisc::ir::Opcode::MOD;
      mod_instr.operands = {rem.reg, raw_len.reg, two.reg};
      mod_instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(mod_instr);

      auto has_odd_tail = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction odd_cmp;
      odd_cmp.opcode = tisc::ir::Opcode::CMP;
      odd_cmp.operands = {has_odd_tail.reg, rem.reg, zero.reg};
      odd_cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
      odd_cmp.boolean_result = true;
      odd_cmp.relation = tisc::ir::ComparisonRelation::NotEqual;
      emit(odd_cmp);

      auto trimmed_label = new_label();
      emit_jump_if_zero(trimmed_label, has_odd_tail);
      auto trimmed = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction trim_instr;
      trim_instr.opcode = tisc::ir::Opcode::VECPOP;
      trim_instr.operands = {trimmed.reg, work.reg};
      emit(trim_instr);
      copy_to_dest(trimmed, work);
      emit_label(trimmed_label);

      auto rev = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction rev_new;
      rev_new.opcode = tisc::ir::Opcode::STRVECNEW;
      rev_new.operands = {rev.reg};
      emit(rev_new);

      auto loop_label = new_label();
      auto done_label = new_label();
      auto check_to_label = new_label();
      auto keep_label = new_label();
      auto after_label = new_label();
      emit_label(loop_label);

      auto cur_len = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction cur_len_instr;
      cur_len_instr.opcode = tisc::ir::Opcode::VECLEN;
      cur_len_instr.operands = {cur_len.reg, work.reg};
      cur_len_instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(cur_len_instr);

      auto has_pair = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction pair_cmp;
      pair_cmp.opcode = tisc::ir::Opcode::CMP;
      pair_cmp.operands = {has_pair.reg, cur_len.reg, two.reg};
      pair_cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
      pair_cmp.boolean_result = true;
      pair_cmp.relation = tisc::ir::ComparisonRelation::GreaterEqual;
      emit(pair_cmp);
      emit_jump_if_zero(done_label, has_pair);

      auto to_value = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction to_last;
      to_last.opcode = tisc::ir::Opcode::VECLAST;
      to_last.operands = {to_value.reg, work.reg};
      emit(to_last);

      auto no_to = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction pop_to;
      pop_to.opcode = tisc::ir::Opcode::VECPOP;
      pop_to.operands = {no_to.reg, work.reg};
      emit(pop_to);

      auto from_value = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction from_last;
      from_last.opcode = tisc::ir::Opcode::VECLAST;
      from_last.operands = {from_value.reg, no_to.reg};
      emit(from_last);

      auto no_pair = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction pop_from;
      pop_from.opcode = tisc::ir::Opcode::VECPOP;
      pop_from.operands = {no_pair.reg, no_to.reg};
      emit(pop_from);

      auto from_match = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction from_cmp;
      from_cmp.opcode = tisc::ir::Opcode::CMP;
      from_cmp.operands = {from_match.reg, from_value.reg, from.reg};
      from_cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
      from_cmp.boolean_result = true;
      from_cmp.relation = tisc::ir::ComparisonRelation::Equal;
      emit(from_cmp);
      emit_jump_if_not_zero(check_to_label, from_match);
      emit_jump(keep_label);

      emit_label(check_to_label);
      auto to_match = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction to_cmp;
      to_cmp.opcode = tisc::ir::Opcode::CMP;
      to_cmp.operands = {to_match.reg, to_value.reg, to.reg};
      to_cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
      to_cmp.boolean_result = true;
      to_cmp.relation = tisc::ir::ComparisonRelation::Equal;
      emit(to_cmp);
      emit_jump_if_not_zero(after_label, to_match);
      emit_jump(keep_label);

      emit_label(keep_label);
      auto rev_with_to = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction push_to_instr;
      push_to_instr.opcode = tisc::ir::Opcode::VECPUSH;
      push_to_instr.operands = {rev_with_to.reg, rev.reg, to_value.reg};
      emit(push_to_instr);
      copy_to_dest(rev_with_to, rev);

      auto rev_with_from = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction push_from_instr;
      push_from_instr.opcode = tisc::ir::Opcode::VECPUSH;
      push_from_instr.operands = {rev_with_from.reg, rev.reg, from_value.reg};
      emit(push_from_instr);
      copy_to_dest(rev_with_from, rev);

      emit_label(after_label);
      copy_to_dest(no_pair, work);
      emit_jump(loop_label);

      emit_label(done_label);
      auto result = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction out_new;
      out_new.opcode = tisc::ir::Opcode::STRVECNEW;
      out_new.operands = {result.reg};
      emit(out_new);

      auto rev_work = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      copy_to_dest(rev, rev_work);
      auto rebuild_loop = new_label();
      auto rebuild_done = new_label();
      emit_label(rebuild_loop);

      auto rev_len = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction rev_len_instr;
      rev_len_instr.opcode = tisc::ir::Opcode::VECLEN;
      rev_len_instr.operands = {rev_len.reg, rev_work.reg};
      rev_len_instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(rev_len_instr);

      auto has_rev_item = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction rev_cmp;
      rev_cmp.opcode = tisc::ir::Opcode::CMP;
      rev_cmp.operands = {has_rev_item.reg, rev_len.reg, zero.reg};
      rev_cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
      rev_cmp.boolean_result = true;
      rev_cmp.relation = tisc::ir::ComparisonRelation::Greater;
      emit(rev_cmp);
      emit_jump_if_zero(rebuild_done, has_rev_item);

      auto rev_value = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction rev_last;
      rev_last.opcode = tisc::ir::Opcode::VECLAST;
      rev_last.operands = {rev_value.reg, rev_work.reg};
      emit(rev_last);

      auto rev_popped = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction rev_pop;
      rev_pop.opcode = tisc::ir::Opcode::VECPOP;
      rev_pop.operands = {rev_popped.reg, rev_work.reg};
      emit(rev_pop);

      auto out_pushed = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction out_push;
      out_push.opcode = tisc::ir::Opcode::VECPUSH;
      out_push.operands = {out_pushed.reg, result.reg, rev_value.reg};
      emit(out_push);
      copy_to_dest(out_pushed, result);
      copy_to_dest(rev_popped, rev_work);
      emit_jump(rebuild_loop);
      emit_label(rebuild_done);

      record_result(&expr, result);
      return {};
    }
    if (func_name == "collections_graph_neighbors") {
      if (expr.arguments.size() != 2) {
        throw std::runtime_error("collections_graph_neighbors expects exactly two arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      auto graph_vec = ensure_expr_result(expr.arguments[0].get());
      auto from = ensure_expr_result(expr.arguments[1].get());

      auto two = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction load_two;
      load_two.opcode = tisc::ir::Opcode::LOADI;
      load_two.operands = {two.reg, tisc::ir::Immediate{2}};
      load_two.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(load_two);

      auto zero = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction load_zero;
      load_zero.opcode = tisc::ir::Opcode::LOADI;
      load_zero.operands = {zero.reg, tisc::ir::Immediate{0}};
      load_zero.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(load_zero);

      auto work = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      copy_to_dest(graph_vec, work);

      auto raw_len = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction len_instr;
      len_instr.opcode = tisc::ir::Opcode::VECLEN;
      len_instr.operands = {raw_len.reg, work.reg};
      len_instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(len_instr);

      auto rem = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction mod_instr;
      mod_instr.opcode = tisc::ir::Opcode::MOD;
      mod_instr.operands = {rem.reg, raw_len.reg, two.reg};
      mod_instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(mod_instr);

      auto has_odd_tail = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction odd_cmp;
      odd_cmp.opcode = tisc::ir::Opcode::CMP;
      odd_cmp.operands = {has_odd_tail.reg, rem.reg, zero.reg};
      odd_cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
      odd_cmp.boolean_result = true;
      odd_cmp.relation = tisc::ir::ComparisonRelation::NotEqual;
      emit(odd_cmp);

      auto trimmed_label = new_label();
      emit_jump_if_zero(trimmed_label, has_odd_tail);
      auto trimmed = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction trim_instr;
      trim_instr.opcode = tisc::ir::Opcode::VECPOP;
      trim_instr.operands = {trimmed.reg, work.reg};
      emit(trim_instr);
      copy_to_dest(trimmed, work);
      emit_label(trimmed_label);

      auto rev = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction rev_new;
      rev_new.opcode = tisc::ir::Opcode::STRVECNEW;
      rev_new.operands = {rev.reg};
      emit(rev_new);

      auto loop_label = new_label();
      auto done_label = new_label();
      auto keep_label = new_label();
      auto after_label = new_label();
      emit_label(loop_label);

      auto cur_len = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction cur_len_instr;
      cur_len_instr.opcode = tisc::ir::Opcode::VECLEN;
      cur_len_instr.operands = {cur_len.reg, work.reg};
      cur_len_instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(cur_len_instr);

      auto has_pair = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction pair_cmp;
      pair_cmp.opcode = tisc::ir::Opcode::CMP;
      pair_cmp.operands = {has_pair.reg, cur_len.reg, two.reg};
      pair_cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
      pair_cmp.boolean_result = true;
      pair_cmp.relation = tisc::ir::ComparisonRelation::GreaterEqual;
      emit(pair_cmp);
      emit_jump_if_zero(done_label, has_pair);

      auto to_value = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction to_last;
      to_last.opcode = tisc::ir::Opcode::VECLAST;
      to_last.operands = {to_value.reg, work.reg};
      emit(to_last);

      auto no_to = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction pop_to;
      pop_to.opcode = tisc::ir::Opcode::VECPOP;
      pop_to.operands = {no_to.reg, work.reg};
      emit(pop_to);

      auto from_value = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction from_last;
      from_last.opcode = tisc::ir::Opcode::VECLAST;
      from_last.operands = {from_value.reg, no_to.reg};
      emit(from_last);

      auto no_pair = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction pop_from;
      pop_from.opcode = tisc::ir::Opcode::VECPOP;
      pop_from.operands = {no_pair.reg, no_to.reg};
      emit(pop_from);

      auto from_match = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction from_cmp;
      from_cmp.opcode = tisc::ir::Opcode::CMP;
      from_cmp.operands = {from_match.reg, from_value.reg, from.reg};
      from_cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
      from_cmp.boolean_result = true;
      from_cmp.relation = tisc::ir::ComparisonRelation::Equal;
      emit(from_cmp);
      emit_jump_if_zero(keep_label, from_match);

      auto rev_pushed = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction rev_push;
      rev_push.opcode = tisc::ir::Opcode::VECPUSH;
      rev_push.operands = {rev_pushed.reg, rev.reg, to_value.reg};
      emit(rev_push);
      copy_to_dest(rev_pushed, rev);
      emit_jump(after_label);

      emit_label(keep_label);

      emit_label(after_label);
      copy_to_dest(no_pair, work);
      emit_jump(loop_label);

      emit_label(done_label);
      auto result = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction out_new;
      out_new.opcode = tisc::ir::Opcode::STRVECNEW;
      out_new.operands = {result.reg};
      emit(out_new);

      auto rev_work = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      copy_to_dest(rev, rev_work);
      auto rebuild_loop = new_label();
      auto rebuild_done = new_label();
      emit_label(rebuild_loop);

      auto rev_len = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction rev_len_instr;
      rev_len_instr.opcode = tisc::ir::Opcode::VECLEN;
      rev_len_instr.operands = {rev_len.reg, rev_work.reg};
      rev_len_instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(rev_len_instr);

      auto has_rev_item = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction rev_cmp;
      rev_cmp.opcode = tisc::ir::Opcode::CMP;
      rev_cmp.operands = {has_rev_item.reg, rev_len.reg, zero.reg};
      rev_cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
      rev_cmp.boolean_result = true;
      rev_cmp.relation = tisc::ir::ComparisonRelation::Greater;
      emit(rev_cmp);
      emit_jump_if_zero(rebuild_done, has_rev_item);

      auto rev_value = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction rev_last;
      rev_last.opcode = tisc::ir::Opcode::VECLAST;
      rev_last.operands = {rev_value.reg, rev_work.reg};
      emit(rev_last);

      auto rev_popped = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction rev_pop;
      rev_pop.opcode = tisc::ir::Opcode::VECPOP;
      rev_pop.operands = {rev_popped.reg, rev_work.reg};
      emit(rev_pop);

      auto out_pushed = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction out_push;
      out_push.opcode = tisc::ir::Opcode::VECPUSH;
      out_push.operands = {out_pushed.reg, result.reg, rev_value.reg};
      emit(out_push);
      copy_to_dest(out_pushed, result);
      copy_to_dest(rev_popped, rev_work);
      emit_jump(rebuild_loop);
      emit_label(rebuild_done);

      record_result(&expr, result);
      return {};
    }
    if (func_name == "collections_graph_canonical") {
      // graph_canonical(g: Vector[T81String]) -> T81String
      // Drains the flat edge-pair vector [from0,to0,from1,to1,...] into a
      // vector of "from->to" strings, then joins them with "," to produce a
      // deterministic serialization of the graph's edge set.
      if (expr.arguments.size() != 1) {
        throw std::runtime_error("collections_graph_canonical expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto graph_vec = ensure_expr_result(expr.arguments[0].get());

      auto two = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      {
        tisc::ir::Instruction li;
        li.opcode = tisc::ir::Opcode::LOADI;
        li.operands = {two.reg, tisc::ir::Immediate{2}};
        li.primitive = tisc::ir::PrimitiveKind::Integer;
        emit(li);
      }
      auto zero = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      {
        tisc::ir::Instruction li;
        li.opcode = tisc::ir::Opcode::LOADI;
        li.operands = {zero.reg, tisc::ir::Immediate{0}};
        li.primitive = tisc::ir::PrimitiveKind::Integer;
        emit(li);
      }

      // Load constant "->"
      auto arrow = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      {
        tisc::ir::Instruction li;
        li.opcode = tisc::ir::Opcode::LOADI;
        li.operands = {arrow.reg};
        li.literal_kind = tisc::LiteralKind::SymbolHandle;
        li.text_literal = "->";
        li.primitive = tisc::ir::PrimitiveKind::Integer;
        emit(li);
      }

      // Load constant ","
      auto comma = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      {
        tisc::ir::Instruction li;
        li.opcode = tisc::ir::Opcode::LOADI;
        li.operands = {comma.reg};
        li.literal_kind = tisc::LiteralKind::SymbolHandle;
        li.text_literal = ",";
        li.primitive = tisc::ir::PrimitiveKind::Integer;
        emit(li);
      }

      // work = copy of graph_vec
      auto work = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      copy_to_dest(graph_vec, work);

      // Trim odd tail
      auto raw_len = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      {
        tisc::ir::Instruction li;
        li.opcode = tisc::ir::Opcode::VECLEN;
        li.operands = {raw_len.reg, work.reg};
        li.primitive = tisc::ir::PrimitiveKind::Integer;
        emit(li);
      }
      auto rem = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      {
        tisc::ir::Instruction mi;
        mi.opcode = tisc::ir::Opcode::MOD;
        mi.operands = {rem.reg, raw_len.reg, two.reg};
        mi.primitive = tisc::ir::PrimitiveKind::Integer;
        emit(mi);
      }
      auto has_odd = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      {
        tisc::ir::Instruction ci;
        ci.opcode = tisc::ir::Opcode::CMP;
        ci.operands = {has_odd.reg, rem.reg, zero.reg};
        ci.primitive = tisc::ir::PrimitiveKind::Boolean;
        ci.boolean_result = true;
        ci.relation = tisc::ir::ComparisonRelation::NotEqual;
        emit(ci);
      }
      auto trimmed_lbl = new_label();
      emit_jump_if_zero(trimmed_lbl, has_odd);
      {
        auto discard = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
        tisc::ir::Instruction pi;
        pi.opcode = tisc::ir::Opcode::VECPOP;
        pi.operands = {discard.reg, work.reg};
        emit(pi);
        copy_to_dest(discard, work);
      }
      emit_label(trimmed_lbl);

      // edge_strs = new vector to hold "from->to" strings
      auto edge_strs = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      {
        tisc::ir::Instruction ni;
        ni.opcode = tisc::ir::Opcode::STRVECNEW;
        ni.operands = {edge_strs.reg};
        emit(ni);
      }

      // Loop: drain work into edge_strs
      auto loop_lbl = new_label();
      auto done_lbl = new_label();
      emit_label(loop_lbl);

      auto cur_len = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      {
        tisc::ir::Instruction li;
        li.opcode = tisc::ir::Opcode::VECLEN;
        li.operands = {cur_len.reg, work.reg};
        li.primitive = tisc::ir::PrimitiveKind::Integer;
        emit(li);
      }
      auto has_pair = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      {
        tisc::ir::Instruction ci;
        ci.opcode = tisc::ir::Opcode::CMP;
        ci.operands = {has_pair.reg, cur_len.reg, two.reg};
        ci.primitive = tisc::ir::PrimitiveKind::Boolean;
        ci.boolean_result = true;
        ci.relation = tisc::ir::ComparisonRelation::GreaterEqual;
        emit(ci);
      }
      emit_jump_if_zero(done_lbl, has_pair);

      // Pop to_value
      auto to_val = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      {
        tisc::ir::Instruction li;
        li.opcode = tisc::ir::Opcode::VECLAST;
        li.operands = {to_val.reg, work.reg};
        emit(li);
      }
      auto after_to = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      {
        tisc::ir::Instruction pi;
        pi.opcode = tisc::ir::Opcode::VECPOP;
        pi.operands = {after_to.reg, work.reg};
        emit(pi);
      }
      // Pop from_value
      auto from_val = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      {
        tisc::ir::Instruction li;
        li.opcode = tisc::ir::Opcode::VECLAST;
        li.operands = {from_val.reg, after_to.reg};
        emit(li);
      }
      auto after_from = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      {
        tisc::ir::Instruction pi;
        pi.opcode = tisc::ir::Opcode::VECPOP;
        pi.operands = {after_from.reg, after_to.reg};
        emit(pi);
      }
      copy_to_dest(after_from, work);

      // edge_str = from_val + "->" + to_val
      auto half = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      {
        tisc::ir::Instruction ci;
        ci.opcode = tisc::ir::Opcode::STRCONCAT;
        ci.operands = {half.reg, from_val.reg, arrow.reg};
        emit(ci);
      }
      auto edge_str = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      {
        tisc::ir::Instruction ci;
        ci.opcode = tisc::ir::Opcode::STRCONCAT;
        ci.operands = {edge_str.reg, half.reg, to_val.reg};
        emit(ci);
      }
      // edge_strs = VECPUSH(edge_strs, edge_str)
      auto edge_strs_new = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      {
        tisc::ir::Instruction pi;
        pi.opcode = tisc::ir::Opcode::VECPUSH;
        pi.operands = {edge_strs_new.reg, edge_strs.reg, edge_str.reg};
        emit(pi);
      }
      copy_to_dest(edge_strs_new, edge_strs);
      emit_jump(loop_lbl);

      emit_label(done_lbl);

      // canonical = STRJOIN(edge_strs, ",")
      auto canonical = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      {
        tisc::ir::Instruction ji;
        ji.opcode = tisc::ir::Opcode::STRJOIN;
        ji.operands = {canonical.reg, edge_strs.reg, comma.reg};
        emit(ji);
      }
      record_result(&expr, canonical);
      return {};
    }
    if (func_name == "symbol_intern" || func_name == "symbol_to_string") {
      if (expr.arguments.size() != 1) {
        throw std::runtime_error(func_name + " expects exactly one argument.");
      }
      expr.arguments[0]->accept(*this);
      auto value = ensure_expr_result(expr.arguments[0].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      copy_to_dest(value, dest);
      record_result(&expr, dest);
      return {};
    }
    if (func_name == "symbol_eq" || func_name == "symbol_ne") {
      if (expr.arguments.size() != 2) {
        throw std::runtime_error(func_name + " expects exactly two arguments.");
      }
      expr.arguments[0]->accept(*this);
      expr.arguments[1]->accept(*this);
      auto lhs = ensure_expr_result(expr.arguments[0].get());
      auto rhs = ensure_expr_result(expr.arguments[1].get());
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::CMP;
      instr.operands = {dest.reg, lhs.reg, rhs.reg};
      instr.primitive = tisc::ir::PrimitiveKind::Boolean;
      instr.boolean_result = true;
      instr.relation = (func_name == "symbol_eq") ? tisc::ir::ComparisonRelation::Equal
                                                  : tisc::ir::ComparisonRelation::NotEqual;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }

    if (dynamic_cast<const VariableExpr*>(expr.callee.get()) ||
        dynamic_cast<const GenericTypeExpr*>(expr.callee.get())) {
      // RFC-0026 AI-M6: @attention / @qmatmul annotated function call sites are lowered
      // to ATTN / QMATMUL opcodes with packed operand encoding rather than CALL.
      auto ai_it = _ai_intrinsic_map.find(func_name);
      if (ai_it != _ai_intrinsic_map.end()) {
        const tisc::ir::Opcode ai_op = ai_it->second;
        if (ai_op == tisc::ir::Opcode::ATTN) {
          // ATTN RD, R_Q, PACK(R_K, R_V) — expects exactly 3 args: q, k, v
          if (expr.arguments.size() != 3) {
            throw std::runtime_error("@attention function '" + func_name +
                                     "' requires exactly 3 arguments (q, k, v).");
          }
          expr.arguments[0]->accept(*this);
          auto q_reg = ensure_expr_result(expr.arguments[0].get());
          expr.arguments[1]->accept(*this);
          auto k_reg = ensure_expr_result(expr.arguments[1].get());
          expr.arguments[2]->accept(*this);
          auto v_reg = ensure_expr_result(expr.arguments[2].get());
          auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
          const int32_t packed_kv =
              (k_reg.reg.index & 0xFF) | ((v_reg.reg.index & 0xFF) << 8);
          tisc::ir::Instruction attn;
          attn.opcode = tisc::ir::Opcode::ATTN;
          attn.operands = {dest.reg, q_reg.reg, tisc::ir::Immediate{packed_kv}};
          emit(attn);
          record_result(&expr, dest);
          return {};
        }
        if (ai_op == tisc::ir::Opcode::QMATMUL) {
          // QMATMUL RD, R_ACT, PACK(R_WT, R_SCALE) — expects exactly 3 args
          if (expr.arguments.size() != 3) {
            throw std::runtime_error("@qmatmul function '" + func_name +
                                     "' requires exactly 3 arguments (act, wt, scale).");
          }
          expr.arguments[0]->accept(*this);
          auto act_reg = ensure_expr_result(expr.arguments[0].get());
          expr.arguments[1]->accept(*this);
          auto wt_reg = ensure_expr_result(expr.arguments[1].get());
          expr.arguments[2]->accept(*this);
          auto scale_reg = ensure_expr_result(expr.arguments[2].get());
          auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
          const int32_t packed_ws =
              (wt_reg.reg.index & 0xFF) | ((scale_reg.reg.index & 0xFF) << 8);
          tisc::ir::Instruction qmm;
          qmm.opcode = tisc::ir::Opcode::QMATMUL;
          qmm.operands = {dest.reg, act_reg.reg, tisc::ir::Immediate{packed_ws}};
          emit(qmm);
          record_result(&expr, dest);
          return {};
        }
      }

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
          push.primitive = val.primitive;
          emit(push);
        }

        // Load function address
        auto addr = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        tisc::ir::Instruction load;
        load.opcode = tisc::ir::Opcode::LOADI;
        load.operands = {addr.reg, label_it->second};
        load.primitive = tisc::ir::PrimitiveKind::Integer;
        emit(load);

        // CALL
        tisc::ir::Instruction call;
        call.opcode = tisc::ir::Opcode::CALL;
        call.operands = {tisc::ir::Register{0}, addr.reg};
        call.primitive = tisc::ir::PrimitiveKind::Integer;
        emit(call);

        // Pop result if not void
        bool returns_void = false;
        tisc::ir::PrimitiveKind result_kind = tisc::ir::PrimitiveKind::Integer;
        if (_semantic) {
          const Type* type = _semantic->type_of(&expr);
          if (type) {
            if (type->kind == Type::Kind::Void) {
              returns_void = true;
            } else {
              result_kind = categorize_primitive(type);
            }
          }
        }
        if (!returns_void) {
          auto dest = allocate_typed_register(result_kind);
          tisc::ir::Instruction pop;
          pop.opcode = tisc::ir::Opcode::POP;
          pop.operands = {dest.reg};
          pop.primitive = result_kind;
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
    if (type_name == "T81Maybe") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("T81Maybe constructor expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      emit_make_option_none(dest);
      record_result(&expr, dest);
      return {};
    }
    if (type_name == "T81Promise") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("T81Promise constructor expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::LOADI;
      instr.operands = {dest.reg};
      instr.literal_kind = tisc::LiteralKind::SymbolHandle;
      instr.text_literal = "std.async.promise";
      instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (type_name == "T81Time") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("T81Time constructor expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Float);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::LOADI;
      instr.operands = {dest.reg};
      instr.literal_kind = tisc::LiteralKind::FloatHandle;
      instr.text_literal = "0";
      instr.primitive = tisc::ir::PrimitiveKind::Float;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (type_name == "T81Entropy") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("T81Entropy constructor expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      auto instr =
          tisc::ir::Instruction{tisc::ir::Opcode::LOADI, {dest.reg, tisc::ir::Immediate{0}}};
      instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (type_name == "T81Agent") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("T81Agent constructor expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::META_REFLECT;
      instr.operands = {dest.reg};
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
    if (type_name == "T81Polynomial" || type_name == "T81Symbolic") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error(type_name + " constructor expects no arguments.");
      }
      auto seed = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction seed_load;
      seed_load.opcode = tisc::ir::Opcode::LOADI;
      seed_load.operands = {seed.reg};
      seed_load.literal_kind = tisc::LiteralKind::SymbolHandle;
      seed_load.text_literal =
          (type_name == "T81Polynomial") ? "t81.poly.zero" : "t81.symbolic.root";
      seed_load.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(seed_load);

      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction sym_load;
      sym_load.opcode = tisc::ir::Opcode::SYMLOAD;
      sym_load.operands = {dest.reg, seed.reg};
      emit(sym_load);
      record_result(&expr, dest);
      return {};
    }

    if (type_name == "T81Quaternion" || type_name == "T81Prob" || type_name == "Cell") {
      auto dest =
          allocate_typed_register(type_name == "T81Prob" ? tisc::ir::PrimitiveKind::Float
                                                         : tisc::ir::PrimitiveKind::Integer);
      emit(tisc::ir::Instruction{tisc::ir::Opcode::NOP, {}});
      record_result(&expr, dest);
      return {};
    }
  }

  if (auto* generic_expr = dynamic_cast<const GenericTypeExpr*>(expr.callee.get())) {
    const Type* callee_type = typed_expr(generic_expr);
    // Fallback for cases where semantic analyzer might not have attached type info
    // or if it's simpler to check the name directly for known builtins
    std::string type_name = std::string(generic_expr->name.lexeme);

    bool handled = false;

    if (callee_type) {
      if (callee_type->kind == Type::Kind::List || callee_type->kind == Type::Kind::Map ||
          callee_type->kind == Type::Kind::Set || callee_type->kind == Type::Kind::Tree) {
        if (!expr.arguments.empty()) {
          throw std::runtime_error("Collection constructor expects no arguments.");
        }
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
        tisc::ir::Instruction vec_new;
        vec_new.opcode = tisc::ir::Opcode::STRVECNEW;
        vec_new.operands = {dest.reg};
        emit(vec_new);
        record_result(&expr, dest);
        handled = true;
        return {};
      }
    }

    // Fallback by name if semantic info missing or if Type::Kind logic failed
    if (!handled && (type_name == "List" || type_name == "Map" || type_name == "Set" ||
                     type_name == "Tree")) {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("Collection constructor expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction vec_new;
      vec_new.opcode = tisc::ir::Opcode::STRVECNEW;
      vec_new.operands = {dest.reg};
      emit(vec_new);
      record_result(&expr, dest);
      handled = true;  // Mark as handled
      return {};
    }

    if ((callee_type && callee_type->kind == Type::Kind::Option) ||
        std::string(generic_expr->name.lexeme) == "T81Maybe") {
      if (std::string(generic_expr->name.lexeme) == "T81Maybe") {
        if (!expr.arguments.empty()) {
          throw std::runtime_error("T81Maybe constructor expects no arguments.");
        }
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        emit_make_option_none(dest);
        record_result(&expr, dest);
        return {};
      }
    }
    if (callee_type && callee_type->kind == Type::Kind::Custom &&
        callee_type->custom_name == "T81Promise") {
      if (!expr.arguments.empty()) {
        throw std::runtime_error("T81Promise constructor expects no arguments.");
      }
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction instr;
      instr.opcode = tisc::ir::Opcode::LOADI;
      instr.operands = {dest.reg};
      instr.literal_kind = tisc::LiteralKind::SymbolHandle;
      instr.text_literal = "std.async.promise";
      instr.primitive = tisc::ir::PrimitiveKind::Integer;
      emit(instr);
      record_result(&expr, dest);
      return {};
    }
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

    // Fallback for user-defined generic function calls (e.g. MyFunc[T](args))
    {
      std::string func_name(generic_expr->name.lexeme);
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

        auto addr = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        tisc::ir::Instruction load;
        load.opcode = tisc::ir::Opcode::LOADI;
        load.operands = {addr.reg, label_it->second};
        emit(load);

        tisc::ir::Instruction call;
        call.opcode = tisc::ir::Opcode::CALL;
        call.operands = {tisc::ir::Register{0}, addr.reg};
        emit(call);

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

std::any IRGenerator::visit(const AssignExpr& expr) {
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

std::any IRGenerator::visit(const SimpleTypeExpr&) { return {}; }

std::any IRGenerator::visit(const GenericTypeExpr&) { return {}; }

std::any IRGenerator::visit(const SetLiteralExpr& expr) {
  // Use tensor-based approach like VectorLiteralExpr for better compatibility

  if (!_semantic) return {};

  // Try to get literal data from semantic analyzer
  const auto* data = _semantic ? _semantic->set_literal_data(&expr) : nullptr;
  if (data && !data->empty()) {
    // Static constant optimization path - use tensor
    t81::T729DynamicTensor tensor({static_cast<int>(data->size())}, *data);
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

  // Fallback to dynamic construction using STRVEC (like VectorLiteralExpr)
  auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
  tisc::ir::Instruction vec_new;
  vec_new.opcode = tisc::ir::Opcode::STRVECNEW;
  vec_new.operands = {dest.reg};
  emit(vec_new);

  for (const auto& element : expr.elements) {
    element->accept(*this);
    auto value = ensure_expr_result(element.get());
    tisc::ir::Instruction push;
    push.opcode = tisc::ir::Opcode::STRVECPUSH;
    push.operands = {dest.reg, value.reg};
    emit(push);
  }

  record_result(&expr, dest);
  return {};
}

std::any IRGenerator::visit(const MapLiteralExpr& expr) {
  // Use tensor-based approach like VectorLiteralExpr for better compatibility

  if (!_semantic) return {};

  // Try to get literal data from semantic analyzer
  const auto* data = _semantic ? _semantic->map_literal_data(&expr) : nullptr;
  if (data && !data->empty()) {
    // Static constant optimization path - use tensor
    // For maps, we store key-value pairs sequentially in the tensor
    t81::T729DynamicTensor tensor({static_cast<int>(data->size())}, *data);
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

  // Fallback to dynamic construction using STRVEC (like VectorLiteralExpr)
  auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
  tisc::ir::Instruction vec_new;
  vec_new.opcode = tisc::ir::Opcode::STRVECNEW;
  vec_new.operands = {dest.reg};
  emit(vec_new);

  for (const auto& [key, value] : expr.entries) {
    key->accept(*this);
    auto key_reg = ensure_expr_result(key.get());
    value->accept(*this);
    auto value_reg = ensure_expr_result(value.get());
    tisc::ir::Instruction push;
    push.opcode = tisc::ir::Opcode::STRVECPUSH;
    push.operands = {dest.reg, key_reg.reg};
    emit(push);
    push.operands = {dest.reg, value_reg.reg};
    emit(push);
  }

  record_result(&expr, dest);
  return {};
}

std::any IRGenerator::visit(const BlockExpr& expr) {
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

std::any IRGenerator::visit(const IfExpr& expr) {
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

std::any IRGenerator::visit(const MatchExpr& expr) {
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

std::any IRGenerator::visit(const FieldAccessExpr& expr) {
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

std::any IRGenerator::visit(const RecordLiteralExpr& expr) {
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

std::any IRGenerator::visit(const EnumLiteralExpr& expr) {
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

std::any IRGenerator::visit(const SymbolLiteralExpr& expr) {
  std::string contents = std::string(expr.value.lexeme);
  if (!contents.empty() && contents[0] == ':') {
    contents = contents.substr(1);
  }
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

std::any IRGenerator::visit(const InfiniteLiteralExpr& expr) {
  expr.seed->accept(*this);
  auto seed_val = ensure_expr_result(expr.seed.get());
  auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
  copy_to_dest(seed_val, dest);
  record_result(&expr, dest);
  return {};
}

std::any IRGenerator::visit(const InferExpr& expr) {
  expr.expression->accept(*this);
  auto val = ensure_expr_result(expr.expression.get());
  // infer <expr> -> TNeuralFwd(dest, val)
  // Assuming result is a Tensor (Integer handle)
  auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
  tisc::ir::Instruction instr;
  instr.opcode = tisc::ir::Opcode::TNEURAL_FWD;
  instr.operands = {dest.reg, val.reg};
  emit(instr);
  record_result(&expr, dest);
  return {};
}

std::any IRGenerator::visit(const VectorLiteralExpr& expr) {
  const Type* vector_type = typed_expr(&expr);

  // Matrix[T] literal: [[r0c0,r0c1,...],[r1c0,...],...] — build as 2D tensor
  if (vector_type && vector_type->kind == Type::Kind::Matrix && !expr.elements.empty()) {
    std::vector<float> flat_data;
    int ncols = 0;
    for (const auto& row_expr : expr.elements) {
      const auto* row_data = _semantic
                                 ? _semantic->vector_literal_data(
                                       dynamic_cast<const VectorLiteralExpr*>(row_expr.get()))
                                 : nullptr;
      if (row_data) {
        if (ncols == 0) ncols = static_cast<int>(row_data->size());
        flat_data.insert(flat_data.end(), row_data->begin(), row_data->end());
      } else {
        // Non-literal row — fall through to default handling
        flat_data.clear();
        ncols = 0;
        break;
      }
    }
    if (!flat_data.empty() && ncols > 0) {
      int nrows = static_cast<int>(expr.elements.size());
      t81::T729DynamicTensor tensor({nrows, ncols}, flat_data);
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
  }

  const bool is_string_vector = vector_type && vector_type->kind == Type::Kind::Vector &&
                                !vector_type->params.empty() &&
                                vector_type->params[0].kind == Type::Kind::String;
  if (is_string_vector) {
    auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
    tisc::ir::Instruction vec_new;
    vec_new.opcode = tisc::ir::Opcode::STRVECNEW;
    vec_new.operands = {dest.reg};
    emit(vec_new);
    for (const auto& element : expr.elements) {
      element->accept(*this);
      auto value = ensure_expr_result(element.get());
      tisc::ir::Instruction push;
      push.opcode = tisc::ir::Opcode::STRVECPUSH;
      push.operands = {dest.reg, value.reg};
      emit(push);
    }
    record_result(&expr, dest);
    return {};
  }

  if (!_semantic) {
    auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
    record_result(&expr, dest);
    return {};
  }
  const auto* data = _semantic->vector_literal_data(&expr);
  if (!data) {
    // Dynamic vector construction - FIXED: Check if this is T81Vector[T, N]
    // T81Vector[T, N] should always be treated as a tensor, not string vector
    // regardless of whether elements are constants or variables
    
    const Type* vector_type = typed_expr(&expr);
    const bool is_t81_vector = vector_type && 
                                 vector_type->kind == Type::Kind::Vector &&
                                 !vector_type->params.empty() &&
                                 vector_type->params[0].kind != Type::Kind::String;
    
    if (is_t81_vector) {
      auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction vec_new;
      vec_new.opcode = tisc::ir::Opcode::TNEW;
      vec_new.operands = {dest.reg};
      emit(vec_new);

      // Use TSET with index parameter for sequential element setting
      auto index_reg = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction zero_reg;
      zero_reg.opcode = tisc::ir::Opcode::LOADI;
      zero_reg.operands = {index_reg.reg, tisc::ir::Immediate{0}};
      emit(zero_reg);

      for (int i = 0; i < static_cast<int>(expr.elements.size()); ++i) {
        auto idx = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        tisc::ir::Instruction set_idx;
        set_idx.opcode = tisc::ir::Opcode::LOADI;
        set_idx.operands = {idx.reg, tisc::ir::Immediate{i}};
        emit(set_idx);

        const auto& element = expr.elements[i];
        element->accept(*this);
        auto value = ensure_expr_result(element.get());
        
        // Use tensor set operations with proper index
        tisc::ir::Instruction push;
        push.opcode = tisc::ir::Opcode::TSET;
        push.operands = {dest.reg, idx.reg, value.reg};
        emit(push);
      }

      record_result(&expr, dest);
      return {};
    }
    
    // Fall back to original string vector behavior for other vector types
    auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
    tisc::ir::Instruction vec_new;
    vec_new.opcode = tisc::ir::Opcode::STRVECNEW;
    vec_new.operands = {dest.reg};
    emit(vec_new);

    for (const auto& element : expr.elements) {
      element->accept(*this);
      auto value = ensure_expr_result(element.get());
      tisc::ir::Instruction push;
      push.opcode = tisc::ir::Opcode::STRVECPUSH;
      push.operands = {dest.reg, value.reg};
      emit(push);
    }

    record_result(&expr, dest);
    return {};
  }

  // Static constant optimization path
  t81::T729DynamicTensor tensor({static_cast<int>(data->size())}, *data);
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

std::any IRGenerator::visit(const IndexExpr& expr) {
  // ── Matrix 2D indexing: (matrix[row])[col] ─────────────────────────────
  // Detect (inner_IndexExpr)[col] where inner_IndexExpr.object : Matrix[T].
  // Emit: TSHAPE ncols, matrix, 1; MUL prod, row, ncols; ADD flat, prod, col; TGET result,
  // matrix, flat
  if (_semantic) {
    if (const auto* inner = dynamic_cast<const IndexExpr*>(expr.object.get())) {
      const Type* inner_obj_type = _semantic->type_of(inner->object.get());
      if (inner_obj_type && inner_obj_type->kind == Type::Kind::Matrix) {
        inner->object->accept(*this);
        auto matrix_reg = ensure_expr_result(inner->object.get());

        inner->index->accept(*this);
        auto row_reg = ensure_expr_result(inner->index.get());

        expr.index->accept(*this);
        auto col_reg = ensure_expr_result(expr.index.get());

        // ncols = tensor.shape[1]
        auto dim1_idx = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        tisc::ir::Instruction load_one;
        load_one.opcode = tisc::ir::Opcode::LOADI;
        load_one.operands = {dim1_idx.reg, tisc::ir::Immediate{1}};
        emit(load_one);

        auto ncols_reg = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        tisc::ir::Instruction tshape;
        tshape.opcode = tisc::ir::Opcode::TSHAPE;
        tshape.operands = {ncols_reg.reg, matrix_reg.reg, dim1_idx.reg};
        emit(tshape);

        auto prod_reg = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        tisc::ir::Instruction imul;
        imul.opcode = tisc::ir::Opcode::MUL;
        imul.operands = {prod_reg.reg, row_reg.reg, ncols_reg.reg};
        emit(imul);

        auto flat_reg = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        tisc::ir::Instruction iadd;
        iadd.opcode = tisc::ir::Opcode::ADD;
        iadd.operands = {flat_reg.reg, prod_reg.reg, col_reg.reg};
        emit(iadd);

        auto temp_dest = allocate_typed_register(tisc::ir::PrimitiveKind::Float);
        tisc::ir::Instruction tget;
        tget.opcode = tisc::ir::Opcode::TGET;
        tget.operands = {temp_dest.reg, matrix_reg.reg, flat_reg.reg};
        emit(tget);

        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
        tisc::ir::Instruction conv;
        conv.opcode = tisc::ir::Opcode::F2I;
        conv.operands = {dest.reg, temp_dest.reg};
        emit(conv);
        record_result(&expr, dest);
        return {};
      }
    }
  }

  // ── Map indexing: map[key] → OptionUnwrap(MapGet(map, key)) ────────────
  if (_semantic) {
    const Type* obj_sem_type = _semantic->type_of(expr.object.get());
    if (obj_sem_type && obj_sem_type->kind == Type::Kind::Map) {
      expr.object->accept(*this);
      auto map_reg = ensure_expr_result(expr.object.get());
      expr.index->accept(*this);
      auto key_reg = ensure_expr_result(expr.index.get());

      auto opt_dest = allocate_typed_register(tisc::ir::PrimitiveKind::Unknown);
      tisc::ir::Instruction map_get;
      map_get.opcode = tisc::ir::Opcode::MapGet;
      map_get.operands = {opt_dest.reg, map_reg.reg, key_reg.reg};
      emit(map_get);

      auto val_dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
      tisc::ir::Instruction unwrap;
      unwrap.opcode = tisc::ir::Opcode::OPTION_UNWRAP;
      unwrap.operands = {val_dest.reg, opt_dest.reg};
      emit(unwrap);
      record_result(&expr, val_dest);
      return {};
    }
  }

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

tisc::ir::ComparisonRelation IRGenerator::relation_from_token(TokenType type) {
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

IRGenerator::NumericCategory IRGenerator::categorize(const Type* type) const {
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

tisc::ir::PrimitiveKind IRGenerator::categorize_primitive(const Type* type) const {
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

// Map a TypeExpr annotation (from the AST) to a PrimitiveKind for coercion.
tisc::ir::PrimitiveKind IRGenerator::primitive_kind_from_type_expr(const TypeExpr* te) const {
  if (!te) return tisc::ir::PrimitiveKind::Unknown;
  if (auto* st = dynamic_cast<const SimpleTypeExpr*>(te)) {
    auto n = st->name.lexeme;
    if (n == "T81Float") return tisc::ir::PrimitiveKind::Float;
    if (n == "T81Fraction") return tisc::ir::PrimitiveKind::Fraction;
    if (n == "bool") return tisc::ir::PrimitiveKind::Boolean;
    if (n == "T81BigInt" || n == "T81Uint" || n == "T81Fixed" || n == "T81Complex" ||
        n == "T81Qutrit" || n == "i32" || n == "i16" || n == "i8" || n == "i2")
      return tisc::ir::PrimitiveKind::Integer;
  }
  return tisc::ir::PrimitiveKind::Unknown;
}

tisc::ir::Opcode IRGenerator::select_opcode(IRGenerator::NumericCategory kind, tisc::ir::Opcode integer_op,
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

const Type* IRGenerator::typed_expr(const Expr* expr) const {
  return _semantic ? _semantic->type_of(expr) : nullptr;
}

void IRGenerator::emit(tisc::ir::Instruction instr)
{ _program.add_instruction(std::move(instr)); }

void IRGenerator::emit_simple(tisc::ir::Opcode opcode) {
  tisc::ir::Instruction instr;
  instr.opcode = opcode;
  emit(instr);
}

void IRGenerator::emit_label(tisc::ir::Label label) {
  emit(tisc::ir::Instruction{tisc::ir::Opcode::LABEL, {label}});
}

void IRGenerator::emit_jump(tisc::ir::Label target) {
  emit(tisc::ir::Instruction{tisc::ir::Opcode::JMP, {target}});
}

void IRGenerator::emit_jump_if_zero(tisc::ir::Label target, const IRGenerator::TypedRegister& cond) {
  auto instr = tisc::ir::Instruction{tisc::ir::Opcode::JZ, {target, cond.reg}};
  instr.primitive = cond.primitive;
  emit(instr);
}

void IRGenerator::emit_jump_if_not_zero(tisc::ir::Label target, const IRGenerator::TypedRegister& cond) {
  auto instr = tisc::ir::Instruction{tisc::ir::Opcode::JNZ, {target, cond.reg}};
  instr.primitive = cond.primitive;
  emit(instr);
}

void IRGenerator::emit_option_is_some(const IRGenerator::TypedRegister& dest, const IRGenerator::TypedRegister& source) {
  emit(tisc::ir::Instruction{tisc::ir::Opcode::OPTION_IS_SOME, {dest.reg, source.reg}});
}

void IRGenerator::emit_option_unwrap(const IRGenerator::TypedRegister& dest, const IRGenerator::TypedRegister& source) {
  emit(tisc::ir::Instruction{tisc::ir::Opcode::OPTION_UNWRAP, {dest.reg, source.reg}});
}

void IRGenerator::emit_result_is_ok(const IRGenerator::TypedRegister& dest, const IRGenerator::TypedRegister& source) {
  emit(tisc::ir::Instruction{tisc::ir::Opcode::RESULT_IS_OK, {dest.reg, source.reg}});
}

void IRGenerator::emit_result_unwrap_ok(const IRGenerator::TypedRegister& dest, const IRGenerator::TypedRegister& source) {
  emit(tisc::ir::Instruction{tisc::ir::Opcode::RESULT_UNWRAP_OK, {dest.reg, source.reg}});
}

void IRGenerator::emit_result_unwrap_err(const IRGenerator::TypedRegister& dest, const IRGenerator::TypedRegister& source) {
  emit(tisc::ir::Instruction{tisc::ir::Opcode::RESULT_UNWRAP_ERR, {dest.reg, source.reg}});
}

void IRGenerator::emit_make_option_some(const IRGenerator::TypedRegister& dest, const IRGenerator::TypedRegister& payload) {
  emit(tisc::ir::Instruction{tisc::ir::Opcode::MAKE_OPTION_SOME, {dest.reg, payload.reg}});
}

void IRGenerator::emit_make_option_none(const IRGenerator::TypedRegister& dest) {
  emit(tisc::ir::Instruction{tisc::ir::Opcode::MAKE_OPTION_NONE, {dest.reg}});
}

void IRGenerator::emit_make_result_ok(const IRGenerator::TypedRegister& dest, const IRGenerator::TypedRegister& payload) {
  emit(tisc::ir::Instruction{tisc::ir::Opcode::MAKE_RESULT_OK, {dest.reg, payload.reg}});
}

void IRGenerator::emit_make_result_err(const IRGenerator::TypedRegister& dest, const IRGenerator::TypedRegister& payload) {
  emit(tisc::ir::Instruction{tisc::ir::Opcode::MAKE_RESULT_ERR, {dest.reg, payload.reg}});
}

void IRGenerator::emit_make_enum_variant(const IRGenerator::TypedRegister& dest, int global_variant_id) {
  tisc::ir::Instruction instr;
  instr.opcode = tisc::ir::Opcode::MAKE_ENUM_VARIANT;
  instr.operands = {dest.reg, tisc::ir::Immediate{global_variant_id}};
  emit(instr);
}

void IRGenerator::emit_make_enum_variant_payload(const IRGenerator::TypedRegister& dest, const IRGenerator::TypedRegister& payload,
                                    int global_variant_id) {
  tisc::ir::Instruction instr;
  instr.opcode = tisc::ir::Opcode::MAKE_ENUM_VARIANT_PAYLOAD;
  instr.operands = {dest.reg, payload.reg, tisc::ir::Immediate{global_variant_id}};
  emit(instr);
}

void IRGenerator::emit_enum_is_variant(const IRGenerator::TypedRegister& dest, const IRGenerator::TypedRegister& source,
                          int global_variant_id) {
  tisc::ir::Instruction instr;
  instr.opcode = tisc::ir::Opcode::ENUM_IS_VARIANT;
  instr.operands = {dest.reg, source.reg, tisc::ir::Immediate{global_variant_id}};
  emit(instr);
}

void IRGenerator::emit_enum_unwrap_payload(const IRGenerator::TypedRegister& dest, const IRGenerator::TypedRegister& source) {
  emit(tisc::ir::Instruction{tisc::ir::Opcode::ENUM_UNWRAP_PAYLOAD, {dest.reg, source.reg}});
}

tisc::ir::Register IRGenerator::new_register() {
  // Skip system registers R75-R80 reserved by VM
  if (_register_count >= 75 && _register_count <= 80) {
    _register_count = 81;
  }
  return tisc::ir::Register{_register_count++};
}

tisc::ir::Label IRGenerator::new_label()
{ return tisc::ir::Label{_label_count++}; }

IRGenerator::TypedRegister IRGenerator::evaluate_expr(const Expr* expr) {
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

void IRGenerator::record_result(const Expr* expr, IRGenerator::TypedRegister reg)
{ _expr_registers[expr] = reg; }

IRGenerator::TypedRegister IRGenerator::allocate_typed_register(tisc::ir::PrimitiveKind primitive) {
  return TypedRegister{new_register(), primitive};
}

IRGenerator::TypedRegister IRGenerator::ensure_kind(IRGenerator::TypedRegister source, tisc::ir::PrimitiveKind target) {
  if (target == tisc::ir::PrimitiveKind::Unknown || source.primitive == target) {
    return source;
  }
  if (source.primitive == tisc::ir::PrimitiveKind::Boolean &&
      target == tisc::ir::PrimitiveKind::Integer) {
    return source;
  }
  // Handle Float/Fraction → Integer (reverse coercions from `as` casts)
  if (target == tisc::ir::PrimitiveKind::Integer) {
    if (source.primitive == tisc::ir::PrimitiveKind::Float ||
        source.primitive == tisc::ir::PrimitiveKind::Fraction) {
      return ensure_integer(source);
    }
    return source;  // already integer-like
  }
  // Handle Float → Fraction
  if (source.primitive == tisc::ir::PrimitiveKind::Float &&
      target == tisc::ir::PrimitiveKind::Fraction) {
    auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Fraction);
    auto instr = tisc::ir::Instruction{tisc::ir::Opcode::F2FRAC, {dest.reg, source.reg}};
    instr.primitive = tisc::ir::PrimitiveKind::Fraction;
    instr.is_conversion = true;
    emit(instr);
    return dest;
  }
  // Handle Fraction → Float
  if (source.primitive == tisc::ir::PrimitiveKind::Fraction &&
      target == tisc::ir::PrimitiveKind::Float) {
    auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Float);
    auto instr = tisc::ir::Instruction{tisc::ir::Opcode::FRAC2F, {dest.reg, source.reg}};
    instr.primitive = tisc::ir::PrimitiveKind::Float;
    instr.is_conversion = true;
    emit(instr);
    return dest;
  }
  if (source.primitive != tisc::ir::PrimitiveKind::Integer) {
    // Unknown coercion — return as-is
    return source;
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
      return source;  // Unknown target — return as-is
  }
  auto dest = allocate_typed_register(target);
  auto instr = tisc::ir::Instruction{opcode, {dest.reg, source.reg}};
  instr.primitive = target;
  instr.is_conversion = true;
  emit(instr);
  return dest;
}

IRGenerator::TypedRegister IRGenerator::ensure_integer(IRGenerator::TypedRegister source) {
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

IRGenerator::TypedRegister IRGenerator::ensure_expr_result(const Expr* expr) const {
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

void IRGenerator::copy_to_dest(IRGenerator::TypedRegister source, IRGenerator::TypedRegister dest) {
  if (source.reg.index == dest.reg.index) {
    return;
  }
  tisc::ir::Instruction instr;
  instr.opcode = tisc::ir::Opcode::MOV;
  instr.operands = {dest.reg, source.reg};
  instr.primitive = dest.primitive;
  emit(instr);
}

IRGenerator::TypedRegister IRGenerator::emit_integer_literal(std::int64_t value) {
  auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
  auto instr = tisc::ir::Instruction{tisc::ir::Opcode::LOADI, {dest.reg, tisc::ir::Immediate{value}}};
  instr.primitive = tisc::ir::PrimitiveKind::Integer;
  emit(instr);
  return dest;
}

IRGenerator::TypedRegister IRGenerator::emit_checked_i32_narrow(IRGenerator::TypedRegister source) {
  if (source.primitive != tisc::ir::PrimitiveKind::Integer &&
      source.primitive != tisc::ir::PrimitiveKind::Unknown) {
    throw std::runtime_error("checked i32 narrowing requires integer-like input");
  }

  auto min_reg = emit_integer_literal(std::numeric_limits<std::int32_t>::min());
  auto max_reg = emit_integer_literal(std::numeric_limits<std::int32_t>::max());

  auto is_too_small = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
  tisc::ir::Instruction less_cmp;
  less_cmp.opcode = tisc::ir::Opcode::CMP;
  less_cmp.operands = {is_too_small.reg, source.reg, min_reg.reg};
  less_cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
  less_cmp.boolean_result = true;
  less_cmp.relation = tisc::ir::ComparisonRelation::Less;
  emit(less_cmp);

  auto lower_ok = new_label();
  emit_jump_if_zero(lower_ok, is_too_small);
  emit_simple(tisc::ir::Opcode::TRAP);
  emit_label(lower_ok);

  auto is_too_large = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
  tisc::ir::Instruction greater_cmp;
  greater_cmp.opcode = tisc::ir::Opcode::CMP;
  greater_cmp.operands = {is_too_large.reg, source.reg, max_reg.reg};
  greater_cmp.primitive = tisc::ir::PrimitiveKind::Boolean;
  greater_cmp.boolean_result = true;
  greater_cmp.relation = tisc::ir::ComparisonRelation::Greater;
  emit(greater_cmp);

  auto upper_ok = new_label();
  emit_jump_if_zero(upper_ok, is_too_large);
  emit_simple(tisc::ir::Opcode::TRAP);
  emit_label(upper_ok);

  auto frac_reg = allocate_typed_register(tisc::ir::PrimitiveKind::Fraction);
  auto i2frac = tisc::ir::Instruction{tisc::ir::Opcode::I2FRAC, {frac_reg.reg, source.reg}};
  i2frac.primitive = tisc::ir::PrimitiveKind::Fraction;
  i2frac.is_conversion = true;
  emit(i2frac);

  auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
  auto frac2i = tisc::ir::Instruction{tisc::ir::Opcode::FRAC2I, {dest.reg, frac_reg.reg}};
  frac2i.primitive = tisc::ir::PrimitiveKind::Integer;
  frac2i.is_conversion = true;
  emit(frac2i);
  return dest;
}

void IRGenerator::bind_variable(const std::string& name, IRGenerator::TypedRegister reg) {
  _variable_registers[name] = reg;
}

std::optional<IRGenerator::TypedRegister> IRGenerator::lookup_variable(std::string_view name) const {
  auto it = _variable_registers.find(std::string{name});
  if (it != _variable_registers.end()) {
    return it->second;
  }
  return std::nullopt;
}

void IRGenerator::bind_variable_from_initializer(const Token& name_token, const Expr* initializer) {
  TypedRegister reg{};
  if (initializer) {
    initializer->accept(*this);
    reg = ensure_expr_result(initializer);
  } else {
    reg = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
  }
  bind_variable(std::string(name_token.lexeme), reg);
}

void IRGenerator::enter_pattern_scope()
{ _pattern_scopes.emplace_back(); }

void IRGenerator::exit_pattern_scope() {
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

void IRGenerator::bind_pattern_variable(std::string name, const IRGenerator::TypedRegister& reg) {
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

void IRGenerator::bind_pattern_payload(const MatchPattern& pattern, const IRGenerator::TypedRegister& reg) {
  if (pattern.kind == MatchPattern::Kind::Identifier && !pattern.binding_is_wildcard) {
    bind_pattern_variable(std::string(pattern.identifier.lexeme), reg);
  }
}

void IRGenerator::bind_variant_payload(const MatchArm& arm, const IRGenerator::TypedRegister& reg) {
  if (arm.pattern.kind == MatchPattern::Kind::Variant && arm.pattern.variant_payload) {
    bind_pattern_payload(*arm.pattern.variant_payload, reg);
    return;
  }
  // For Option/Result arms the parsed pattern already represents the payload bindings.
  bind_pattern_payload(arm.pattern, reg);
}

std::string IRGenerator::guard_metadata_reason(const SemanticAnalyzer::MatchMetadata::ArmInfo& info,
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

void IRGenerator::emit_guard_metadata(const SemanticAnalyzer::MatchMetadata::ArmInfo* info,
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

const EnumInfo* IRGenerator::enum_info_for_name(std::string_view name) const {
  if (!_semantic) return nullptr;
  auto it = _semantic->enum_definitions().find(std::string(name));
  if (it == _semantic->enum_definitions().end()) {
    return nullptr;
  }
  return &it->second;
}

std::optional<int> IRGenerator::global_variant_id_for(std::string_view enum_name, int variant_id) const {
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

std::optional<int> IRGenerator::global_variant_id_for(
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

std::optional<int> IRGenerator::resolve_variant_index(std::string_view enum_name,
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

}  // namespace t81::frontend

