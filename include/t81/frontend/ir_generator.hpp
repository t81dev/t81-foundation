// include/t81/frontend/ir_generator.hpp
#ifndef T81_FRONTEND_IR_GENERATOR_HPP
#define T81_FRONTEND_IR_GENERATOR_HPP

#include "t81/frontend/ast.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include "t81/frontend/symbol_table.hpp"
#include "t81/tensor.hpp"
#include "t81/tisc/ir.hpp"
#include <any>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>

#include <unordered_map>

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
                case '\\': result.push_back('\\'); break;
                case '"': result.push_back('"'); break;
                case 'n': result.push_back('\n'); break;
                case 'r': result.push_back('\r'); break;
                case 't': result.push_back('\t'); break;
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
                default: result.push_back(esc); break;
            }
        } else {
            result.push_back(c);
        }
    }
    return result;
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
        for (const auto& stmt : statements) {
            stmt->accept(*this);
        }
        return std::move(_program);
    }

    const std::vector<LoopInfo>& loop_infos() const { return _loop_infos; }

    void attach_semantic_analyzer(const SemanticAnalyzer* analyzer) {
        _semantic = analyzer;
    }

    // Statements
    std::any visit(const ExpressionStmt& stmt) override {
        stmt.expression->accept(*this);
        return {};
    }

    std::any visit(const BlockStmt& stmt) override {
        for (const auto& s : stmt.statements) s->accept(*this);
        return {};
    }

    std::any visit(const VarStmt&) override          { return {}; }
    std::any visit(const LetStmt&) override          { return {}; }
    std::any visit(const IfStmt&) override           { return {}; }
    std::any visit(const WhileStmt&) override        { return {}; }
    std::any visit(const LoopStmt& stmt) override {
        auto entry_label = new_label();
        auto exit_label = new_label();
        emit_label(entry_label);
        for (const auto& statement : stmt.body) {
            statement->accept(*this);
        }
        emit(tisc::ir::Instruction{tisc::ir::Opcode::JMP, {entry_label}});
        emit_label(exit_label);

        LoopInfo info;
        info.entry_label = entry_label;
        info.exit_label = exit_label;
        if (_semantic) {
            if (const auto* meta = _semantic->loop_metadata_for(stmt)) {
                info.id = meta->id;
                info.depth = meta->depth;
                info.annotated = meta->annotated();
            }
        }
        _loop_infos.push_back(info);
        return {};
    }
    std::any visit(const ReturnStmt& stmt) override {
        if (stmt.value) {
            stmt.value->accept(*this);
        }
        return {};
    }
    std::any visit(const FunctionStmt& stmt) override {
        if (std::string_view(stmt.name.lexeme) != "main") {
            return {};
        }
        for (const auto& statement : stmt.body) {
            statement->accept(*this);
        }
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
            bool both_bool = left_type && right_type && left_type->kind == Type::Kind::Bool && right_type->kind == Type::Kind::Bool;

            NumericCategory left_cat = categorize(left_type);
            NumericCategory right_cat = categorize(right_type);
            NumericCategory target_category = left_cat;
            auto merge_category = [](NumericCategory a, NumericCategory b) {
                if (a == NumericCategory::Float || b == NumericCategory::Float) return NumericCategory::Float;
                if (a == NumericCategory::Fraction || b == NumericCategory::Fraction) return NumericCategory::Fraction;
                if (a == NumericCategory::Integer || b == NumericCategory::Integer) return NumericCategory::Integer;
                return NumericCategory::Unknown;
            };
            target_category = merge_category(left_cat, right_cat);
            if (target_category == NumericCategory::Unknown) target_category = merge_category(target_category, right_cat);

            tisc::ir::PrimitiveKind operand_primitive = primitive_kind;
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

            auto instr = tisc::ir::Instruction{tisc::ir::Opcode::CMP, {dest.reg, left_converted.reg, right_converted.reg}};
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
        std::string_view lexeme = expr.value.lexeme;
        int64_t value = std::stoll(std::string{lexeme});
        auto dest = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);

        auto instr = tisc::ir::Instruction{
            tisc::ir::Opcode::LOADI,
            {dest.reg, tisc::ir::Immediate{value}}
        };
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

    std::any visit(const UnaryExpr&) override        { return {}; }
    std::any visit(const VariableExpr&) override     { return {}; }
    std::any visit(const CallExpr& expr) override {
        if (auto var_expr = dynamic_cast<const VariableExpr*>(expr.callee.get())) {
            std::string func_name{var_expr->name.lexeme};
            if (func_name == "Some") {
                if (!expr.arguments.empty()) {
                    expr.arguments[0]->accept(*this);
                }
                emit_simple(tisc::ir::Opcode::MAKE_OPTION_SOME);
                return {};
            }
            if (func_name == "None") {
                emit_simple(tisc::ir::Opcode::MAKE_OPTION_NONE);
                return {};
            }
            if (func_name == "Ok") {
                if (!expr.arguments.empty()) {
                    expr.arguments[0]->accept(*this);
                }
                emit_simple(tisc::ir::Opcode::MAKE_RESULT_OK);
                return {};
            }
            if (func_name == "Err") {
                if (!expr.arguments.empty()) {
                    expr.arguments[0]->accept(*this);
                }
                emit_simple(tisc::ir::Opcode::MAKE_RESULT_ERR);
                return {};
            }
            if (func_name == "weights.load") {
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
        }
        for (const auto& arg : expr.arguments) {
            arg->accept(*this);
        }
        return {};
    }
    std::any visit(const AssignExpr&) override       { return {}; }
    std::any visit(const SimpleTypeExpr&) override   { return {}; }
    std::any visit(const GenericTypeExpr&) override  { return {}; }
    std::any visit(const MatchExpr& expr) override {
        expr.scrutinee->accept(*this);

        const SemanticAnalyzer::MatchMetadata* metadata = _semantic ? _semantic->match_metadata_for(expr) : nullptr;
        const Type* result_type = typed_expr(&expr);
        auto primitive = categorize_primitive(result_type);
        if (primitive == tisc::ir::PrimitiveKind::Unknown) {
            primitive = tisc::ir::PrimitiveKind::Integer;
        }
        auto dest = allocate_typed_register(primitive);
        record_result(&expr, dest);

        const Type* scrutinee_type = typed_expr(expr.scrutinee.get());
        bool scrutinee_is_option = scrutinee_type && scrutinee_type->kind == Type::Kind::Option;
        bool scrutinee_is_result = scrutinee_type && scrutinee_type->kind == Type::Kind::Result;
        auto scrutinee_reg = ensure_expr_result(expr.scrutinee.get());

        auto find_arm = [&](std::string_view name) -> const MatchArm* {
            for (const auto& arm : expr.arms) {
                if (std::string_view{arm.keyword.lexeme} == name) {
                    return &arm;
                }
            }
            return nullptr;
        };

        auto emit_match_arm = [&]<typename Prelude>(const MatchArm& arm,
                                                    tisc::ir::Label entry_label,
                                                    tisc::ir::Label guard_fail_target,
                                                    Prelude before_body,
                                                    tisc::ir::Label end_label,
                                                    const TypedRegister* variant_flag,
                                                    std::optional<int> variant_id) {
            emit_label(entry_label);
            std::optional<tisc::ir::Label> guard_fail_label;
            if (variant_flag && variant_id.has_value()) {
                emit_enum_is_variant(*variant_flag, scrutinee_reg, *variant_id);
                emit_jump_if_zero(guard_fail_target, *variant_flag);
            }
            if (arm.guard) {
                guard_fail_label = new_label();
                arm.guard->accept(*this);
                auto guard_value = ensure_expr_result(arm.guard.get());
                emit_jump_if_zero(*guard_fail_label, guard_value);
            }
            before_body();
            arm.expression->accept(*this);
            auto value = ensure_expr_result(arm.expression.get());
            copy_to_dest(value, dest);
            emit_jump(end_label);
            if (guard_fail_label) {
                emit_label(*guard_fail_label);
                emit_jump(guard_fail_target);
            }
        };

        auto finalize_branches = [&](tisc::ir::Label end_label, tisc::ir::Label unmatched_label) {
            emit_label(end_label);
            emit_label(unmatched_label);
            emit_simple(tisc::ir::Opcode::TRAP);
        };

        if (metadata && metadata->kind == SemanticAnalyzer::MatchMetadata::Kind::Option &&
            metadata->has_some && metadata->has_none && scrutinee_is_option) {
            const MatchArm* some_arm = find_arm("Some");
            const MatchArm* none_arm = find_arm("None");
            if (!some_arm || !none_arm) {
                for (const auto& arm : expr.arms) {
                    arm.expression->accept(*this);
                    auto value = ensure_expr_result(arm.expression.get());
                    copy_to_dest(value, dest);
                }
                return {};
            }

            auto end_label = new_label();
            auto unmatched_label = new_label();
            auto some_label = new_label();
            auto none_label = new_label();

            auto payload_reg = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
            auto flag_reg = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);

            emit_option_is_some(flag_reg, scrutinee_reg);
            emit_jump_if_not_zero(some_label, flag_reg);

            emit_match_arm(*none_arm, none_label, unmatched_label, []() {}, end_label, nullptr, std::nullopt);

            emit_match_arm(*some_arm, some_label, none_label,
                          [&]() { emit_option_unwrap(payload_reg, scrutinee_reg); },
                          end_label, nullptr, std::nullopt);

            finalize_branches(end_label, unmatched_label);
            return {};
        }

        if (metadata && metadata->kind == SemanticAnalyzer::MatchMetadata::Kind::Result &&
            metadata->has_ok && metadata->has_err && scrutinee_is_result) {
            const MatchArm* ok_arm = find_arm("Ok");
            const MatchArm* err_arm = find_arm("Err");
            if (!ok_arm || !err_arm) {
                for (const auto& arm : expr.arms) {
                    arm.expression->accept(*this);
                    auto value = ensure_expr_result(arm.expression.get());
                    copy_to_dest(value, dest);
                }
                return {};
            }

            auto end_label = new_label();
            auto unmatched_label = new_label();
            auto ok_label = new_label();
            auto err_label = new_label();

            auto payload_reg = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
            auto flag_reg = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);

            emit_result_is_ok(flag_reg, scrutinee_reg);
            emit_jump_if_not_zero(ok_label, flag_reg);

            emit_match_arm(*err_arm, err_label, unmatched_label, [&]() {
                emit_result_unwrap_err(payload_reg, scrutinee_reg);
            }, end_label, nullptr, std::nullopt);

            emit_match_arm(*ok_arm, ok_label, err_label, [&]() {
                emit_result_unwrap_ok(payload_reg, scrutinee_reg);
            }, end_label, nullptr, std::nullopt);

            finalize_branches(end_label, unmatched_label);
            return {};
        }

        if (metadata && metadata->kind == SemanticAnalyzer::MatchMetadata::Kind::Enum) {
            auto end_label = new_label();
            auto trap_label = new_label();
            std::vector<tisc::ir::Label> arm_labels(expr.arms.size());
            for (auto& label : arm_labels) {
                label = new_label();
            }
            auto variant_flag = allocate_typed_register(tisc::ir::PrimitiveKind::Boolean);
            auto payload_reg = allocate_typed_register(tisc::ir::PrimitiveKind::Integer);
            for (size_t i = 0; i < expr.arms.size(); ++i) {
                tisc::ir::Label guard_fail_target =
                    (i + 1 < expr.arms.size()) ? arm_labels[i + 1] : trap_label;
                const auto& arm_meta = metadata->arms[i];
                std::optional<int> variant_id;
                if (arm_meta.variant_id >= 0) {
                    variant_id = arm_meta.variant_id;
                }
                bool variant_has_payload =
                    arm_meta.payload_type.kind != Type::Kind::Unknown;

                emit_match_arm(expr.arms[i],
                               arm_labels[i],
                               guard_fail_target,
                               [&]() {
                                   if (variant_has_payload) {
                                       emit_enum_unwrap_payload(payload_reg, scrutinee_reg);
                                   }
                               },
                               end_label,
                               &variant_flag,
                               variant_id);
            }
            emit_label(trap_label);
            emit_simple(tisc::ir::Opcode::TRAP);
            emit_label(end_label);
            emit_simple(tisc::ir::Opcode::NOP);
            return {};
        }

        for (const auto& arm : expr.arms) {
            arm.expression->accept(*this);
            auto value = ensure_expr_result(arm.expression.get());
            copy_to_dest(value, dest);
        }
        return {};
    }

    std::any visit(const FieldAccessExpr& expr) override {
        auto value = evaluate_expr(expr.object.get());
        record_result(&expr, value);
        return {};
    }

    std::any visit(const RecordLiteralExpr& expr) override {
        for (const auto& field : expr.fields) {
            field.second->accept(*this);
        }
        tisc::ir::PrimitiveKind primitive = tisc::ir::PrimitiveKind::Integer;
        if (auto kind = categorize_primitive(typed_expr(&expr)); kind != tisc::ir::PrimitiveKind::Unknown) {
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
        if (auto kind = categorize_primitive(typed_expr(&expr)); kind != tisc::ir::PrimitiveKind::Unknown) {
            primitive = kind;
        }
        auto dest = allocate_typed_register(primitive);
        if (variant_id) {
            if (expr.payload) {
                auto payload_reg = ensure_expr_result(expr.payload.get());
                emit_make_enum_variant_payload(dest, payload_reg, *variant_id);
            } else {
                emit_make_enum_variant(dest, *variant_id);
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

private:
    struct TypedRegister {
        tisc::ir::Register reg;
        tisc::ir::PrimitiveKind primitive = tisc::ir::PrimitiveKind::Unknown;
    };

    enum class NumericCategory {
        Integer,
        Float,
        Fraction,
        Unknown
    };

    static tisc::ir::ComparisonRelation relation_from_token(TokenType type) {
        switch (type) {
            case TokenType::Less: return tisc::ir::ComparisonRelation::Less;
            case TokenType::LessEqual: return tisc::ir::ComparisonRelation::LessEqual;
            case TokenType::Greater: return tisc::ir::ComparisonRelation::Greater;
            case TokenType::GreaterEqual: return tisc::ir::ComparisonRelation::GreaterEqual;
            case TokenType::EqualEqual: return tisc::ir::ComparisonRelation::Equal;
            case TokenType::BangEqual: return tisc::ir::ComparisonRelation::NotEqual;
            default: return tisc::ir::ComparisonRelation::None;
        }
    }

    NumericCategory categorize(const Type* type) const {
        if (!type) return NumericCategory::Integer;
        switch (type->kind) {
            case Type::Kind::I2:
            case Type::Kind::I8:
            case Type::Kind::I16:
            case Type::Kind::I32:
            case Type::Kind::BigInt:
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
            case Type::Kind::I2:
            case Type::Kind::I8:
            case Type::Kind::I16:
            case Type::Kind::I32:
            case Type::Kind::BigInt:
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

    tisc::ir::Opcode select_opcode(NumericCategory kind,
                                   tisc::ir::Opcode integer_op,
                                   tisc::ir::Opcode float_op,
                                   tisc::ir::Opcode fraction_op) const {
        switch (kind) {
            case NumericCategory::Float: return float_op;
            case NumericCategory::Fraction: return fraction_op;
            default: return integer_op;
        }
    }

    const Type* typed_expr(const Expr* expr) const {
        return _semantic ? _semantic->type_of(expr) : nullptr;
    }

    void emit(tisc::ir::Instruction instr) {
        _program.add_instruction(std::move(instr));
    }

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

    void emit_make_enum_variant(const TypedRegister& dest, int variant_id) {
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::MAKE_ENUM_VARIANT;
        instr.operands = {dest.reg, tisc::ir::Immediate{variant_id}};
        emit(instr);
    }

    void emit_make_enum_variant_payload(const TypedRegister& dest,
                                       const TypedRegister& payload,
                                       int variant_id) {
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::MAKE_ENUM_VARIANT_PAYLOAD;
        instr.operands = {dest.reg, payload.reg, tisc::ir::Immediate{variant_id}};
        emit(instr);
    }

    void emit_enum_is_variant(const TypedRegister& dest,
                              const TypedRegister& source,
                              int variant_id) {
        tisc::ir::Instruction instr;
        instr.opcode = tisc::ir::Opcode::ENUM_IS_VARIANT;
        instr.operands = {dest.reg, source.reg, tisc::ir::Immediate{variant_id}};
        emit(instr);
    }

    void emit_enum_unwrap_payload(const TypedRegister& dest,
                                  const TypedRegister& source) {
        emit(tisc::ir::Instruction{tisc::ir::Opcode::ENUM_UNWRAP_PAYLOAD, {dest.reg, source.reg}});
    }

    tisc::ir::Register new_register() {
        return tisc::ir::Register{_register_count++};
    }

    tisc::ir::Label new_label() {
        return tisc::ir::Label{_label_count++};
    }

    TypedRegister evaluate_expr(const Expr* expr) {
        expr->accept(*this);
        auto it = _expr_registers.find(expr);
        if (it == _expr_registers.end()) {
            throw std::runtime_error("IRGenerator failed to record expression result");
        }
        return it->second;
    }

    void record_result(const Expr* expr, TypedRegister reg) {
        _expr_registers[expr] = reg;
    }

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

    TypedRegister ensure_expr_result(const Expr* expr) const {
        auto it = _expr_registers.find(expr);
        if (it == _expr_registers.end()) {
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

    std::optional<int> resolve_variant_index(std::string_view enum_name, std::string_view variant_name) const {
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
    int _register_count = 0;
    int _label_count = 0;
    std::unordered_map<const Expr*, TypedRegister> _expr_registers;
    std::vector<LoopInfo> _loop_infos;
};

} // namespace t81::frontend

#endif // T81_FRONTEND_IR_GENERATOR_HPP
