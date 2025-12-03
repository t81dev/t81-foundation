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
    std::any visit(const ReturnStmt&) override       { return {}; }
    std::any visit(const FunctionStmt&) override     { return {}; }
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

        bool has_some = false;
        bool has_none = false;
        bool has_ok = false;
        bool has_err = false;
        for (const auto& arm : expr.arms) {
            has_some |= (arm.variant == MatchArm::Variant::Some);
            has_none |= (arm.variant == MatchArm::Variant::None);
            has_ok |= (arm.variant == MatchArm::Variant::Ok);
            has_err |= (arm.variant == MatchArm::Variant::Err);
        }

        auto emit_arm = [&](MatchArm::Variant variant) {
            for (const auto& arm : expr.arms) {
                if (arm.variant == variant) {
                    if (variant == MatchArm::Variant::Some) {
                        emit_simple(tisc::ir::Opcode::OPTION_UNWRAP);
                    }
                    if (variant == MatchArm::Variant::Ok) {
                        emit_simple(tisc::ir::Opcode::RESULT_UNWRAP_OK);
                    }
                    if (variant == MatchArm::Variant::Err) {
                        emit_simple(tisc::ir::Opcode::RESULT_UNWRAP_ERR);
                    }
                    arm.expression->accept(*this);
                    return;
                }
            }
        };

        const Type* scrutinee_type = typed_expr(expr.scrutinee.get());
        bool scrutinee_is_option = scrutinee_type && scrutinee_type->kind == Type::Kind::Option;
        bool scrutinee_is_result = scrutinee_type && scrutinee_type->kind == Type::Kind::Result;

        if (scrutinee_is_option && has_some && has_none) {
            auto some_label = new_label();
            auto end_label = new_label();
            emit_simple(tisc::ir::Opcode::OPTION_IS_SOME);
            emit(tisc::ir::Instruction{tisc::ir::Opcode::JNZ, {some_label}});
            emit_arm(MatchArm::Variant::None);
            emit(tisc::ir::Instruction{tisc::ir::Opcode::JMP, {end_label}});
            emit_label(some_label);
            emit_arm(MatchArm::Variant::Some);
            emit_label(end_label);
            return {};
        }

        if (scrutinee_is_result && has_ok && has_err) {
            auto ok_label = new_label();
            auto end_label = new_label();
            emit_simple(tisc::ir::Opcode::RESULT_IS_OK);
            emit(tisc::ir::Instruction{tisc::ir::Opcode::JNZ, {ok_label}});
            emit_arm(MatchArm::Variant::Err);
            emit(tisc::ir::Instruction{tisc::ir::Opcode::JMP, {end_label}});
            emit_label(ok_label);
            emit_arm(MatchArm::Variant::Ok);
            emit_label(end_label);
            return {};
        }

        for (const auto& arm : expr.arms) {
            arm.expression->accept(*this);
        }
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
