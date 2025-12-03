#ifndef T81_TEST_UTILS_HPP
#define T81_TEST_UTILS_HPP

#include "t81/frontend/ast.hpp"
#include "t81/frontend/parser.hpp"
#include "t81/frontend/semantic_analyzer.hpp"
#include <iostream>
#include <cassert>
#include <string>
#include <sstream>
#include <any>
#include <vector>

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
            name += ": " + print(*stmt.type);
        }
        if (stmt.initializer) {
            return parenthesize(name, {&stmt.initializer});
        }
        return std::string("(" + name + ")");
    }

    std::any visit(const LetStmt& stmt) override {
        std::string name = "let " + std::string(stmt.name.lexeme);
        if (stmt.type) {
            name += ": " + print(*stmt.type);
        }
        name += " =";
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

    std::any visit(const LoopStmt& stmt) override {
        std::stringstream ss;
        ss << "(loop";
        if (stmt.bound_kind == LoopStmt::BoundKind::Infinite) {
            ss << " @bounded(infinite)";
        } else if (stmt.bound_kind == LoopStmt::BoundKind::Static) {
            ss << " @bounded(" << stmt.bound_value.value_or(0) << ")";
        }
        ss << " (block";
        for (const auto& statement : stmt.body) {
            ss << " " << print(*statement);
        }
        ss << "))";
        return ss.str();
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
        for (size_t i = 0; i < stmt.params.size(); ++i) {
            ss << stmt.params[i].name.lexeme << ": " << print(*stmt.params[i].type);
            if (i < stmt.params.size() - 1) {
                ss << " ";
            }
        }
        ss << " )";
        if (stmt.return_type) {
            ss << " -> " << print(*stmt.return_type);
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

    std::any visit(const TypeDecl& stmt) override {
        std::stringstream ss;
        ss << "(type " << stmt.name.lexeme << " [";
        for (size_t i = 0; i < stmt.params.size(); ++i) {
            ss << stmt.params[i].lexeme;
            if (i + 1 < stmt.params.size()) {
                ss << ", ";
            }
        }
        ss << "] = " << print(*stmt.alias);
        ss << ")";
        return ss.str();
    }

    std::any visit(const RecordDecl& stmt) override {
        std::stringstream ss;
        ss << "(record " << stmt.name.lexeme;
        for (const auto& field : stmt.fields) {
            ss << " " << field.name.lexeme << ": ";
            if (field.type) {
                ss << print(*field.type);
            } else {
                ss << "<unknown>";
            }
        }
        ss << ")";
        return ss.str();
    }

    std::any visit(const EnumDecl& stmt) override {
        std::stringstream ss;
        ss << "(enum " << stmt.name.lexeme;
        for (const auto& variant : stmt.variants) {
            ss << " " << variant.name.lexeme;
            if (variant.payload) {
                ss << "(" << print(*variant.payload) << ")";
            }
        }
        ss << ")";
        return ss.str();
    }

    std::any visit(const BinaryExpr& expr) override {
        return parenthesize(expr.op.lexeme, {std::any(&expr.left), std::any(&expr.right)});
    }

    std::any visit(const UnaryExpr& expr) override {
        return parenthesize(expr.op.lexeme, {&expr.right});
    }

    std::any visit(const LiteralExpr& expr) override {
        return std::string(expr.value.lexeme);
    }

    std::any visit(const VectorLiteralExpr& expr) override {
        std::stringstream ss;
        ss << "[";
        for (size_t i = 0; i < expr.elements.size(); ++i) {
            ss << print(*expr.elements[i]);
            if (i + 1 < expr.elements.size()) ss << ", ";
        }
        ss << "]";
        return ss.str();
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

    std::any visit(const MatchExpr& expr) override {
        std::stringstream ss;
        ss << "(match " << print(*expr.scrutinee);
        for (const auto& arm : expr.arms) {
            ss << " (" << arm.keyword.lexeme;
            if (arm.has_binding) {
                ss << " " << arm.binding.lexeme;
            }
            ss << " => " << print(*arm.expression);
            ss << ")";
        }
        ss << ")";
        return ss.str();
    }

    std::any visit(const FieldAccessExpr& expr) override {
        return parenthesize("field " + std::string(expr.field.lexeme),
                            std::vector<const Expr*>{expr.object.get()});
    }

    std::any visit(const RecordLiteralExpr& expr) override {
        std::stringstream ss;
        ss << "(recordlit " << expr.type_name.lexeme;
        for (const auto& field : expr.fields) {
            ss << " " << field.first.lexeme;
            if (field.second) {
                ss << ": " << print(*field.second);
            }
        }
        ss << ")";
        return ss.str();
    }

    std::any visit(const EnumLiteralExpr& expr) override {
        std::stringstream ss;
        ss << "(enumlit " << expr.enum_name.lexeme << "." << expr.variant.lexeme;
        if (expr.payload) {
            ss << " " << print(*expr.payload);
        }
        ss << ")";
        return ss.str();
    }

    std::any visit(const SimpleTypeExpr& expr) override {
        return std::string(expr.name.lexeme);
    }

    std::any visit(const GenericTypeExpr& expr) override {
        std::vector<const Expr*> params;
        for (size_t i = 0; i < expr.param_count; ++i) {
            params.push_back(expr.params[i].get());
        }
        return parenthesize("generic " + std::string(expr.name.lexeme), params);
    }

private:
    std::string parenthesize(std::string_view name, const std::vector<const Expr*>& exprs) {
        std::stringstream ss;
        ss << "(" << name;
        for (const auto& expr : exprs) {
            ss << " " << print(*expr);
        }
        ss << ")";
        return ss.str();
    }

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

inline void expect_semantic_success(const std::string& source, const char* label) {
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();
    assert(!parser.had_error() && label);

    SemanticAnalyzer analyzer(stmts);
    analyzer.analyze();
    assert(!analyzer.had_error() && label);
}

inline void expect_semantic_failure(const std::string& source, const char* label, const std::string& expected_error = "") {
    Lexer lexer(source);
    Parser parser(lexer);
    auto stmts = parser.parse();
    if (parser.had_error()) return;

    SemanticAnalyzer analyzer(stmts);
    analyzer.analyze();
    assert(analyzer.had_error() && label);

    if (!expected_error.empty()) {
        for (const auto& diag : analyzer.diagnostics()) {
            if (diag.message.find(expected_error) != std::string::npos) {
                return;
            }
        }
        std::cerr << "Test '" << label << "' failed. Expected error containing: '" << expected_error << "', but no matching diagnostic was found." << std::endl;
        assert(false && "Expected diagnostic not found.");
    }
}

#endif // T81_TEST_UTILS_HPP
