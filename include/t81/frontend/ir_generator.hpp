#ifndef T81_FRONTEND_IR_GENERATOR_HPP
#define T81_FRONTEND_IR_GENERATOR_HPP

#include "t81/frontend/ast.hpp"
#include "t81/frontend/symbol_table.hpp"
#include "t81/tisc/ir.hpp"
#include <any>

namespace t81 {
namespace frontend {

/**
 * @brief Traverses the Abstract Syntax Tree (AST) and generates TISC IR.
 *
 * The IRGenerator implements the visitor pattern to walk the AST produced
 * by the parser. It manages a symbol table to track variables and functions,
 * and emits a linear sequence of TISC instructions and labels.
 */
class IRGenerator : public ExprVisitor, public StmtVisitor {
public:
    /**
     * @brief Generates a TISC program from a sequence of AST statements.
     * @param statements The top-level statements of the program from the parser.
     * @return A tisc::ir::IntermediateProgram containing the generated IR.
     */
    tisc::ir::IntermediateProgram generate(const std::vector<std::unique_ptr<Stmt>>& statements);

private:
    std::any visit(const ExpressionStmt& stmt) override;
    std::any visit(const VarStmt& stmt) override;
    std::any visit(const LetStmt& stmt) override;
    std::any visit(const BlockStmt& stmt) override;
    std::any visit(const IfStmt& stmt) override;
    std::any visit(const WhileStmt& stmt) override;
    std::any visit(const ReturnStmt& stmt) override;
    std::any visit(const FunctionStmt& stmt) override;

    std::any visit(const BinaryExpr& expr) override;
    std::any visit(const UnaryExpr& expr) override;
    std::any visit(const LiteralExpr& expr) override;
    std::any visit(const GroupingExpr& expr) override;
    std::any visit(const VariableExpr& expr) override;
    std::any visit(const CallExpr& expr) override;
    std::any visit(const AssignExpr& expr) override;
    std::any visit(const SimpleTypeExpr& expr) override;
    std::any visit(const GenericTypeExpr& expr) override;

    void emit(tisc::ir::Instruction instr);
    void emit_label(tisc::ir::Label label);
    tisc::ir::Register new_register();
    tisc::ir::Label new_label();

    tisc::ir::IntermediateProgram _program;
    SymbolTable _symbols;
    int _register_count = 0;
    int _label_count = 0;
};

} // namespace frontend
} // namespace t81

#endif // T81_FRONTEND_IR_GENERATOR_HPP
