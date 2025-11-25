#include "t81/frontend/parser.hpp"
#include "t81/frontend/ast.hpp"
#include <iostream>
#include <cassert>
#include <string>
#include <sstream>
#include <any>

using namespace t81::frontend;

class AstPrinter : public ExprVisitor, public StmtVisitor {
public:
    std::string print(const Stmt& stmt) {
        return std::any_cast<std::string>(stmt.accept(*this));
    }

    std::string print(const Expr& expr) {
        return std::any_cast<std::string>(expr.accept(*this));
    }

    std::any visit(const ExpressionStmt& stmt) override {
        return parenthesize(";", {&stmt.expression});
    }

    std::any visit(const VarStmt& stmt) override {
        std::string name = "var " + std::string(stmt.name.lexeme);
        if (stmt.type) {
            name += ": " + std::string(stmt.type->name.lexeme);
        }
        if (stmt.initializer) {
            return parenthesize(name, {&stmt.initializer});
        }
        return std::string("(" + name + ")");
    }

    std::any visit(const LetStmt& stmt) override {
        std::string name = "let " + std::string(stmt.name.lexeme);
        if (stmt.type) {
            name += ": " + std::string(stmt.type->name.lexeme);
        }
        return parenthesize(name, {&stmt.initializer});
    }

    std::any visit(const BlockStmt& stmt) override {
        std::stringstream ss;
        ss << "(block";
        for (const auto& statement : stmt.statements) {
            ss << " " << print(*statement);
        }
        ss << ")";
        return ss.str();
    }

    std::any visit(const IfStmt& stmt) override {
        if (stmt.else_branch) {
            return parenthesize("if-else", {&stmt.condition, &stmt.then_branch, &stmt.else_branch});
        }
        return parenthesize("if", {&stmt.condition, &stmt.then_branch});
    }

    std::any visit(const WhileStmt& stmt) override {
        return parenthesize("while", {&stmt.condition, &stmt.body});
    }

    std::any visit(const ReturnStmt& stmt) override {
        if (stmt.value) {
            return parenthesize("return", {&stmt.value});
        }
        return std::string("(return)");
    }

    std::any visit(const FunctionStmt& stmt) override {
        std::stringstream ss;
        ss << "(fn " << stmt.name.lexeme;
        ss << " (";
        for (const auto& param : stmt.params) {
            ss << param.name.lexeme << ": " << param.type->name.lexeme << " ";
        }
        ss << ")";
        if (stmt.return_type) {
            ss << " -> " << stmt.return_type->name.lexeme;
        }
        ss << " ";
        ss << "(block";
        for (const auto& statement : stmt.body) {
            ss << " " << print(*statement);
        }
        ss << ")";
        ss << ")";
        return ss.str();
    }

    std::any visit(const BinaryExpr& expr) override {
        return parenthesize(expr.op.lexeme, {&expr.left, &expr.right});
    }

    std::any visit(const UnaryExpr& expr) override {
        return parenthesize(expr.op.lexeme, {&expr.right});
    }

    std::any visit(const LiteralExpr& expr) override {
        return std::string(expr.value.lexeme);
    }

    std::any visit(const GroupingExpr& expr) override {
        return parenthesize("group", {&expr.expression});
    }

    std::any visit(const VariableExpr& expr) override {
        return std::string(expr.name.lexeme);
    }

    std::any visit(const CallExpr& expr) override {
        std::vector<std::any> parts;
        parts.push_back(&expr.callee);
        for (const auto& arg : expr.arguments) {
            parts.push_back(&arg);
        }
        return parenthesize("call", parts);
    }

    std::any visit(const AssignExpr& expr) override {
        return parenthesize("= " + std::string(expr.name.lexeme), {&expr.value});
    }

    std::any visit(const TypeExpr& expr) override {
        return std::string(expr.name.lexeme);
    }

private:
    std::string parenthesize(std::string_view name, const std::vector<std::any>& parts) {
        std::stringstream ss;
        ss << "(" << name;
        for (const auto& part : parts) {
            ss << " ";
            if (part.type() == typeid(const std::unique_ptr<Expr>*)) {
                ss << print(**std::any_cast<const std::unique_ptr<Expr>*>(part));
            } else if (part.type() == typeid(const std::unique_ptr<Stmt>*)) {
                ss << print(**std::any_cast<const std::unique_ptr<Stmt>*>(part));
            }
        }
        ss << ")";
        return ss.str();
    }
};

int main() {
    std::string source = R"(
        fn fib(n: i32) -> i32 {
            if (n < 2) {
                return n;
            }
            return fib(n - 1) + fib(n - 2);
        }
    )";
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();

    assert(stmts.size() == 1);

    AstPrinter printer;
    std::string result = printer.print(*stmts[0]);
    std::string expected = "(fn fib (n: i32 ) -> i32 (block (if (< n 2) (block (return n))) (return (+ (call fib (- n 1)) (call fib (- n 2))))))";

    if (result != expected) {
        std::cerr << "Parser test failed!" << std::endl;
        std::cerr << "  Expected: " << expected << std::endl;
        std::cerr << "  Actual:   " << result << std::endl;
        return 1;
    }

    std::cout << "Parser test passed!" << std::endl;

    return 0;
}
