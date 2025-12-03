#ifndef T81_FRONTEND_PARSER_HPP
#define T81_FRONTEND_PARSER_HPP

#include "t81/frontend/ast.hpp"
#include "t81/frontend/lexer.hpp"
#include <vector>
#include <memory>
#include <optional>
#include <cstdint>
#include <string>

namespace t81 {
namespace frontend {

class Parser {
public:
    Parser(Lexer& lexer, std::string source_name = {});

    std::vector<std::unique_ptr<Stmt>> parse();

    bool had_error() const { return _had_error; }

private:
    // Grammar rule methods
    std::unique_ptr<Stmt> declaration();
    std::unique_ptr<Stmt> loop_statement();
    std::unique_ptr<Stmt> function(const std::string& kind);
    std::unique_ptr<Stmt> type_declaration();
    std::unique_ptr<Stmt> record_declaration();
    std::unique_ptr<Stmt> enum_declaration();
    std::unique_ptr<Stmt> statement();
    std::unique_ptr<Stmt> var_declaration();
    std::unique_ptr<Stmt> let_declaration();
    std::unique_ptr<Stmt> expression_statement();
    std::vector<std::unique_ptr<Stmt>> block();

    std::unique_ptr<Expr> expression();
    std::unique_ptr<Expr> assignment();
    std::unique_ptr<Expr> equality();
    std::unique_ptr<Expr> comparison();
    std::unique_ptr<Expr> term();
    std::unique_ptr<Expr> factor();
    std::unique_ptr<Expr> unary();
    std::unique_ptr<Expr> primary();
    std::unique_ptr<Expr> match_expression();
    MatchArm match_arm();
    std::unique_ptr<Expr> record_literal(Token type_name);
    std::unique_ptr<TypeExpr> type();
    bool parse_loop_annotation(LoopStmt::BoundKind& bound_kind,
                               std::optional<std::int64_t>& bound_value,
                               Token& attr_token);
    std::unique_ptr<GenericTypeExpr> parse_generic_type(Token name);

    // Helper methods
    bool match(const std::vector<TokenType>& types);
    bool check(TokenType type);
    Token advance();
    bool is_at_end();
    Token peek();
    Token previous();
    Token consume(TokenType type, const char* message);
    void synchronize();

    Lexer& _lexer;
    Token _current;
    Token _previous;
    bool _had_error = false;
    std::string _source_name;
    void report_error(const Token& token, const std::string& message);
};

} // namespace frontend
} // namespace t81

#endif // T81_FRONTEND_PARSER_HPP
