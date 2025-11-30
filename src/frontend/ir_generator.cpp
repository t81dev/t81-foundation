#include "t81/frontend/ir_generator.hpp"

namespace t81 {
namespace frontend {

tisc::ir::IntermediateProgram IRGenerator::generate(const std::vector<std::unique_ptr<Stmt>>& statements) {
    // This is a stub implementation.
    return _program;
}

std::any IRGenerator::visit(const ExpressionStmt& stmt) { return {}; }
std::any IRGenerator::visit(const VarStmt& stmt) { return {}; }
std::any IRGenerator::visit(const LetStmt& stmt) { return {}; }
std::any IRGenerator::visit(const BlockStmt& stmt) { return {}; }
std::any IRGenerator::visit(const IfStmt& stmt) { return {}; }
std::any IRGenerator::visit(const WhileStmt& stmt) { return {}; }
std::any IRGenerator::visit(const ReturnStmt& stmt) { return {}; }
std::any IRGenerator::visit(const FunctionStmt& stmt) { return {}; }
std::any IRGenerator::visit(const BinaryExpr& expr) { return {}; }
std::any IRGenerator::visit(const UnaryExpr& expr) { return {}; }
std::any IRGenerator::visit(const LiteralExpr& expr) { return {}; }
std::any IRGenerator::visit(const GroupingExpr& expr) { return {}; }
std::any IRGenerator::visit(const VariableExpr& expr) { return {}; }
std::any IRGenerator::visit(const CallExpr& expr) { return {}; }
std::any IRGenerator::visit(const AssignExpr& expr) { return {}; }
std::any IRGenerator::visit(const SimpleTypeExpr& expr) { return {}; }
std::any IRGenerator::visit(const GenericTypeExpr& expr) { return {}; }

void IRGenerator::emit(tisc::ir::Instruction instr) {
    _program.add_instruction(std::move(instr));
}

void IRGenerator::emit_label(tisc::ir::Label label) {
    emit(tisc::ir::Instruction{tisc::ir::Opcode::LABEL, {label}});
}

tisc::ir::Register IRGenerator::new_register() {
    return tisc::ir::Register{_register_count++};
}

tisc::ir::Label IRGenerator::new_label() {
    return tisc::ir::Label{_label_count++};
}

} // namespace frontend
} // namespace t81
