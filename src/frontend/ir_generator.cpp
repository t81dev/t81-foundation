#include "t81/frontend/ir_generator.hpp"
#include <stdexcept>

namespace t81 {
namespace frontend {

using namespace t81::tisc::ir;

tisc::ir::IntermediateProgram IRGenerator::generate(const std::vector<std::unique_ptr<Stmt>>& statements) {
    for (const auto& stmt : statements) {
        if (stmt) { // Skip null statements from parse errors
            stmt->accept(*this);
        }
    }
    return std::move(_program);
}

// --- Statement Visitors ---

std::any IRGenerator::visit(const ExpressionStmt& stmt) {
    stmt.expression->accept(*this);
    return {};
}

std::any IRGenerator::visit(const VarStmt& stmt) {
    Register dest = new_register();
    if (stmt.initializer) {
        Register init = std::any_cast<Register>(stmt.initializer->accept(*this));
        emit({Opcode::MOV, {dest, init}});
    }
    _symbols.define(stmt.name.lexeme, Symbol{Symbol::Type::Variable, dest});
    return {};
}

std::any IRGenerator::visit(const LetStmt& stmt) {
    Register init = std::any_cast<Register>(stmt.initializer->accept(*this));
    _symbols.define(stmt.name.lexeme, Symbol{Symbol::Type::Variable, init});
    return {};
}

std::any IRGenerator::visit(const BlockStmt& stmt) {
    _symbols.enter_scope();
    for (const auto& statement : stmt.statements) {
        if (statement) statement->accept(*this);
    }
    _symbols.exit_scope();
    return {};
}

std::any IRGenerator::visit(const IfStmt& stmt) {
    if (auto* bin_expr = dynamic_cast<const BinaryExpr*>(stmt.condition.get())) {
        Register left = std::any_cast<Register>(bin_expr->left->accept(*this));
        Register right = std::any_cast<Register>(bin_expr->right->accept(*this));
        emit({Opcode::CMP, {left, right}});

        Label else_label = new_label();
        Label end_label = new_label();

        switch (bin_expr->op.type) {
            case TokenType::Less:
                emit({Opcode::JP, {else_label}});
                emit({Opcode::JZ, {else_label}});
                break;
            case TokenType::EqualEqual:
                emit({Opcode::JNZ, {else_label}});
                break;
            default:
                throw std::runtime_error("Unsupported comparison operator in if statement.");
        }

        stmt.then_branch->accept(*this);

        if (stmt.else_branch) {
            emit({Opcode::JMP, {end_label}});
            emit_label(else_label);
            stmt.else_branch->accept(*this);
            emit_label(end_label);
        } else {
            emit_label(else_label);
        }
    } else {
        Register condition = std::any_cast<Register>(stmt.condition->accept(*this));

        Register zero_reg = new_register();
        emit({Opcode::LOADI, {zero_reg, Immediate{0}}});
        emit({Opcode::CMP, {condition, zero_reg}});

        Label else_label = new_label();
        Label end_label = new_label();

        emit({Opcode::JZ, {else_label}});
        stmt.then_branch->accept(*this);

        if (stmt.else_branch) {
            emit({Opcode::JMP, {end_label}});
            emit_label(else_label);
            stmt.else_branch->accept(*this);
            emit_label(end_label);
        } else {
            emit_label(else_label);
        }
    }

    return {};
}

std::any IRGenerator::visit(const WhileStmt& stmt) {
    Label start_label = new_label();
    Label end_label = new_label();

    emit_label(start_label);

    if (auto* bin_expr = dynamic_cast<const BinaryExpr*>(stmt.condition.get())) {
        Register left = std::any_cast<Register>(bin_expr->left->accept(*this));
        Register right = std::any_cast<Register>(bin_expr->right->accept(*this));
        emit({Opcode::CMP, {left, right}});

        Opcode jump_op;
        switch (bin_expr->op.type) {
            case TokenType::Less:
                jump_op = Opcode::JP;
                break;
            case TokenType::EqualEqual:
                jump_op = Opcode::JNZ;
                break;
            default:
                throw std::runtime_error("Unsupported comparison operator in while statement.");
        }

        emit({jump_op, {end_label}});
        if (bin_expr->op.type == TokenType::Less) {
            emit({Opcode::JZ, {end_label}});
        }
    } else {
        Register condition = std::any_cast<Register>(stmt.condition->accept(*this));

        Register zero_reg = new_register();
        emit({Opcode::LOADI, {zero_reg, Immediate{0}}});
        emit({Opcode::CMP, {condition, zero_reg}});

        emit({Opcode::JZ, {end_label}});
    }

    stmt.body->accept(*this);
    emit({Opcode::JMP, {start_label}});

    emit_label(end_label);
    return {};
}

std::any IRGenerator::visit(const ReturnStmt& stmt) {
    if (stmt.value) {
        Register retval = std::any_cast<Register>(stmt.value->accept(*this));
        emit({Opcode::MOV, {Register{0}, retval}});
    }
    emit({Opcode::HALT, {}});
    return {};
}

std::any IRGenerator::visit(const FunctionStmt& stmt) {
    Label func_label = new_label();
    _symbols.define(stmt.name.lexeme, Symbol{Symbol::Type::Function, func_label});
    emit_label(func_label);

    // Prologue
    Register frame_pointer = new_register();
    emit({Opcode::PUSH, {frame_pointer}});

    _symbols.enter_scope();
    std::vector<Register> param_regs;
    for (const auto& param : stmt.params) {
        Register reg = new_register();
        _symbols.define(param.name.lexeme, Symbol{Symbol::Type::Variable, reg});
        param_regs.push_back(reg);
    }

    for (auto it = param_regs.rbegin(); it != param_regs.rend(); ++it) {
        emit({Opcode::POP, {*it}});
    }

    // The body of a function is a single block statement.
    if (!stmt.body.empty() && stmt.body[0]) {
        stmt.body[0]->accept(*this);
    }

    _symbols.exit_scope();

    // Epilogue
    emit({Opcode::POP, {frame_pointer}});
    emit({Opcode::RET, {}});

    return {};
}

// --- Expression Visitors ---

std::any IRGenerator::visit(const BinaryExpr& expr) {
    Register left = std::any_cast<Register>(expr.left->accept(*this));
    Register right = std::any_cast<Register>(expr.right->accept(*this));

    switch (expr.op.type) {
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::Percent: {
            Register dest = new_register();
            Opcode op;
            if (expr.op.type == TokenType::Plus) op = Opcode::ADD;
            else if (expr.op.type == TokenType::Minus) op = Opcode::SUB;
            else if (expr.op.type == TokenType::Star) op = Opcode::MUL;
            else if (expr.op.type == TokenType::Slash) op = Opcode::DIV;
            else op = Opcode::MOD;
            emit({op, {dest, left, right}});
            return dest;
        }

        default:
             throw std::runtime_error("Unsupported binary operator in IR generator.");
    }
}

std::any IRGenerator::visit(const UnaryExpr& expr) {
    Register right = std::any_cast<Register>(expr.right->accept(*this));
    Register dest = new_register();
    if (expr.op.type == TokenType::Minus) {
        emit({Opcode::NEG, {dest, right}});
    }
    return dest;
}

std::any IRGenerator::visit(const LiteralExpr& expr) {
    Register dest = new_register();
    switch (expr.value.type) {
        case TokenType::Integer: {
            long long value = std::stoll(std::string(expr.value.lexeme));
            emit({Opcode::LOADI, {dest, Immediate{value}}});
            break;
        }
        case TokenType::Float: {
            // TISC IR does not have a float type, so we will treat it as an integer for now.
            long long value = std::stoll(std::string(expr.value.lexeme));
            emit({Opcode::LOADI, {dest, Immediate{value}}});
            break;
        }
        default:
            throw std::runtime_error("Unsupported literal type in IR generator.");
    }
    return dest;
}

std::any IRGenerator::visit(const GroupingExpr& expr) {
    return expr.expression->accept(*this);
}

std::any IRGenerator::visit(const VariableExpr& expr) {
    if (auto symbol = _symbols.lookup(expr.name.lexeme)) {
        if (symbol->type == Symbol::Type::Variable) {
            return std::get<Register>(symbol.value().location);
        } else {
            return std::string(expr.name.lexeme);
        }
    }
    throw std::runtime_error("Undefined variable: " + std::string(expr.name.lexeme));
}

std::any IRGenerator::visit(const CallExpr& expr) {
    std::string func_name;
    if (auto* var_expr = dynamic_cast<const VariableExpr*>(expr.callee.get())) {
        func_name = std::string(var_expr->name.lexeme);
    } else {
        throw std::runtime_error("Unsupported callee type in IR generator.");
    }

    Label func_label;
    if (auto symbol = _symbols.lookup(func_name)) {
        if (symbol->type == Symbol::Type::Function) {
            func_label = std::get<Label>(symbol.value().location);
        } else {
            throw std::runtime_error("Cannot call a non-function.");
        }
    } else {
        throw std::runtime_error("Undefined function: " + func_name);
    }

    for (const auto& arg : expr.arguments) {
        Register arg_reg = std::any_cast<Register>(arg->accept(*this));
        emit({Opcode::PUSH, {arg_reg}});
    }

    Register dest = new_register();
    emit({Opcode::CALL, {func_label}});
    emit({Opcode::MOV, {dest, Register{0}}});

    return dest;
}

std::any IRGenerator::visit(const AssignExpr& expr) {
    Register value = std::any_cast<Register>(expr.value->accept(*this));
    if (auto symbol = _symbols.lookup(expr.name.lexeme)) {
        if (symbol->type == Symbol::Type::Variable) {
            auto reg = std::get<Register>(symbol.value().location);
            emit({Opcode::MOV, {reg, value}});
            return reg;
        }
    }
    throw std::runtime_error("Undefined variable: " + std::string(expr.name.lexeme));
}

std::any IRGenerator::visit(const SimpleTypeExpr& expr) {
    // Will be properly implemented in a future task.
    return {};
}

std::any IRGenerator::visit(const GenericTypeExpr& expr) {
    // Will be properly implemented in a future task.
    return {};
}

// --- Helper Methods ---

void IRGenerator::emit(Instruction instr) {
    _program.add_instruction(std::move(instr));
}

void IRGenerator::emit_label(Label label) {
    emit({Opcode::LABEL, {label}});
}

Register IRGenerator::new_register() {
    return Register{_register_count++};
}

Label IRGenerator::new_label() {
    return Label{_label_count++};
}

} // namespace frontend
} // namespace t81
