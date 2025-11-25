#include "t81/frontend/ir_generator.hpp"
#include <stdexcept>

namespace t81 {
namespace frontend {

tisc::Program IRGenerator::generate(const std::vector<std::unique_ptr<Stmt>>& statements) {
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
    tisc::Register dest = new_register();
    if (stmt.initializer) {
        tisc::Register init = std::any_cast<tisc::Register>(stmt.initializer->accept(*this));
        emit({tisc::Opcode::MOV, {dest, init}});
    }
    _symbols.define(stmt.name.lexeme, {Symbol::Type::Variable, dest});
    return {};
}

std::any IRGenerator::visit(const LetStmt& stmt) {
    tisc::Register init = std::any_cast<tisc::Register>(stmt.initializer->accept(*this));
    _symbols.define(stmt.name.lexeme, {Symbol::Type::Variable, init});
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
        tisc::Register left = std::any_cast<tisc::Register>(bin_expr->left->accept(*this));
        tisc::Register right = std::any_cast<tisc::Register>(bin_expr->right->accept(*this));
        emit({tisc::Opcode::CMP, {left, right}});

        tisc::Opcode jump_op;
        switch (bin_expr->op.type) {
            case TokenType::Less:
                jump_op = tisc::Opcode::JP;
                break;
            case TokenType::EqualEqual:
                jump_op = tisc::Opcode::JNZ;
                break;
            default:
                throw std::runtime_error("Unsupported comparison operator in if statement.");
        }

        tisc::Label else_label = new_label();
        tisc::Label end_label = new_label();

        emit({jump_op, {else_label}});
        if (bin_expr->op.type == TokenType::Less) {
            emit({tisc::Opcode::JZ, {else_label}});
        }
        stmt.then_branch->accept(*this);

        if (stmt.else_branch) {
            emit({tisc::Opcode::JMP, {end_label}});
            emit_label(else_label);
            stmt.else_branch->accept(*this);
            emit_label(end_label);
        } else {
            emit_label(else_label);
        }
    } else {
        tisc::Register condition = std::any_cast<tisc::Register>(stmt.condition->accept(*this));

        tisc::Register zero_reg = new_register();
        emit({tisc::Opcode::LOADI, {zero_reg, tisc::Immediate{0}}});
        emit({tisc::Opcode::CMP, {condition, zero_reg}});

        tisc::Label else_label = new_label();
        tisc::Label end_label = new_label();

        emit({tisc::Opcode::JZ, {else_label}});
        stmt.then_branch->accept(*this);

        if (stmt.else_branch) {
            emit({tisc::Opcode::JMP, {end_label}});
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
    tisc::Label start_label = new_label();
    tisc::Label end_label = new_label();

    emit_label(start_label);

    if (auto* bin_expr = dynamic_cast<const BinaryExpr*>(stmt.condition.get())) {
        tisc::Register left = std::any_cast<tisc::Register>(bin_expr->left->accept(*this));
        tisc::Register right = std::any_cast<tisc::Register>(bin_expr->right->accept(*this));
        emit({tisc::Opcode::CMP, {left, right}});

        tisc::Opcode jump_op;
        switch (bin_expr->op.type) {
            case TokenType::Less:
                jump_op = tisc::Opcode::JP;
                break;
            case TokenType::EqualEqual:
                jump_op = tisc::Opcode::JNZ;
                break;
            default:
                throw std::runtime_error("Unsupported comparison operator in while statement.");
        }

        emit({jump_op, {end_label}});
        if (bin_expr->op.type == TokenType::Less) {
            emit({tisc::Opcode::JZ, {end_label}});
        }
    } else {
        tisc::Register condition = std::any_cast<tisc::Register>(stmt.condition->accept(*this));

        tisc::Register zero_reg = new_register();
        emit({tisc::Opcode::LOADI, {zero_reg, tisc::Immediate{0}}});
        emit({tisc::Opcode::CMP, {condition, zero_reg}});

        emit({tisc::Opcode::JZ, {end_label}});
    }

    stmt.body->accept(*this);
    emit({tisc::Opcode::JMP, {start_label}});

    emit_label(end_label);
    return {};
}

std::any IRGenerator::visit(const ReturnStmt& stmt) {
    if (stmt.value) {
        tisc::Register retval = std::any_cast<tisc::Register>(stmt.value->accept(*this));
        emit({tisc::Opcode::MOV, {tisc::Register{0}, retval}});
    }
    return {};
}

std::any IRGenerator::visit(const FunctionStmt& stmt) {
    tisc::Label func_label = new_label();
    _symbols.define(stmt.name.lexeme, {Symbol::Type::Function, func_label});
    emit_label(func_label);

    // Prologue
    tisc::Register frame_pointer = new_register();
    emit({tisc::Opcode::PUSH, {frame_pointer}});

    _symbols.enter_scope();
    std::vector<tisc::Register> param_regs;
    for (const auto& param : stmt.params) {
        tisc::Register reg = new_register();
        _symbols.define(param.name.lexeme, {Symbol::Type::Variable, reg});
        param_regs.push_back(reg);
    }

    for (auto it = param_regs.rbegin(); it != param_regs.rend(); ++it) {
        emit({tisc::Opcode::POP, {*it}});
    }

    for (const auto& statement : stmt.body) {
        if (statement) statement->accept(*this);
    }
    _symbols.exit_scope();

    // Epilogue
    emit({tisc::Opcode::POP, {frame_pointer}});
    emit({tisc::Opcode::RET, {}});

    return {};
}

// --- Expression Visitors ---

std::any IRGenerator::visit(const BinaryExpr& expr) {
    tisc::Register left = std::any_cast<tisc::Register>(expr.left->accept(*this));
    tisc::Register right = std::any_cast<tisc::Register>(expr.right->accept(*this));

    switch (expr.op.type) {
        case TokenType::Plus:
        case TokenType::Minus:
        case TokenType::Star:
        case TokenType::Slash:
        case TokenType::Percent: {
            tisc::Register dest = new_register();
            tisc::Opcode op;
            if (expr.op.type == TokenType::Plus) op = tisc::Opcode::ADD;
            else if (expr.op.type == TokenType::Minus) op = tisc::Opcode::SUB;
            else if (expr.op.type == TokenType::Star) op = tisc::Opcode::MUL;
            else if (expr.op.type == TokenType::Slash) op = tisc::Opcode::DIV;
            else op = tisc::Opcode::MOD;
            emit({op, {dest, left, right}});
            return dest;
        }

        default:
             throw std::runtime_error("Unsupported binary operator in IR generator.");
    }
}

std::any IRGenerator::visit(const UnaryExpr& expr) {
    tisc::Register right = std::any_cast<tisc::Register>(expr.right->accept(*this));
    tisc::Register dest = new_register();
    if (expr.op.type == TokenType::Minus) {
        emit({tisc::Opcode::NEG, {dest, right}});
    }
    return dest;
}

std::any IRGenerator::visit(const LiteralExpr& expr) {
    tisc::Register dest = new_register();
    switch (expr.value.type) {
        case TokenType::Integer: {
            long long value = std::stoll(std::string(expr.value.lexeme));
            emit({tisc::Opcode::LOADI, {dest, tisc::Immediate{value}}});
            break;
        }
        case TokenType::Float: {
            // TISC IR does not have a float type, so we will treat it as an integer for now.
            long long value = std::stoll(std::string(expr.value.lexeme));
            emit({tisc::Opcode::LOADI, {dest, tisc::Immediate{value}}});
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
            return std::get<tisc::Register>(symbol->location);
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

    tisc::Label func_label;
    if (auto symbol = _symbols.lookup(func_name)) {
        if (symbol->type == Symbol::Type::Function) {
            func_label = std::get<tisc::Label>(symbol->location);
        } else {
            throw std::runtime_error("Cannot call a non-function.");
        }
    } else {
        throw std::runtime_error("Undefined function: " + func_name);
    }

    for (const auto& arg : expr.arguments) {
        tisc::Register arg_reg = std::any_cast<tisc::Register>(arg->accept(*this));
        emit({tisc::Opcode::PUSH, {arg_reg}});
    }

    tisc::Register dest = new_register();
    emit({tisc::Opcode::CALL, {func_label}});
    emit({tisc::Opcode::MOV, {dest, tisc::Register{0}}});

    return dest;
}

std::any IRGenerator::visit(const AssignExpr& expr) {
    tisc::Register value = std::any_cast<tisc::Register>(expr.value->accept(*this));
    if (auto symbol = _symbols.lookup(expr.name.lexeme)) {
        if (symbol->type == Symbol::Type::Variable) {
            auto reg = std::get<tisc::Register>(symbol->location);
            emit({tisc::Opcode::MOV, {reg, value}});
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

void IRGenerator::emit(tisc::Instruction instr) {
    _program.add_instruction(std::move(instr));
}

void IRGenerator::emit_label(tisc::Label label) {
    emit({tisc::Opcode::LABEL, {label}});
}

tisc::Register IRGenerator::new_register() {
    return tisc::Register{_register_count++};
}

tisc::Label IRGenerator::new_label() {
    return tisc::Label{_label_count++};
}

} // namespace frontend
} // namespace t81
