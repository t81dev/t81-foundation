#include "t81/frontend/parser.hpp"
#include <iostream>

namespace t81 {
namespace frontend {

Parser::Parser(Lexer& lexer) : _lexer(lexer) {
    // Prime the pump
    _current = _lexer.next_token();
}

std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!is_at_end()) {
        statements.push_back(declaration());
    }
    return statements;
}

// --- Helper Methods ---

bool Parser::match(const std::vector<TokenType>& types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

bool Parser::check(TokenType type) {
    if (is_at_end()) return false;
    return peek().type == type;
}

Token Parser::advance() {
    if (!is_at_end()) {
        _previous = _current;
        _current = _lexer.next_token();
    }
    return previous();
}

bool Parser::is_at_end() {
    return peek().type == TokenType::Eof;
}

Token Parser::peek() {
    return _current;
}

Token Parser::previous() {
    return _previous;
}

Token Parser::consume(TokenType type, const char* message) {
    if (check(type)) return advance();
    _had_error = true;
    std::cerr << "Parse Error: " << message << " at line " << peek().line << std::endl;
    // For now, we don't have good error recovery.
    // In a real compiler, we would synchronize here.
    return peek();
}

// --- Grammar Rules ---

std::unique_ptr<Stmt> Parser::declaration() {
    try {
        if (match({TokenType::Fn})) return function("function");
        if (match({TokenType::Var})) return var_declaration();
        if (match({TokenType::Let})) return let_declaration();
        return statement();
    } catch (const std::runtime_error& error) {
        synchronize();
        return nullptr;
    }
}

std::unique_ptr<Stmt> Parser::function(const std::string& kind) {
    Token name = consume(TokenType::Identifier, ("Expect " + kind + " name.").c_str());
    consume(TokenType::LParen, ("Expect '(' after " + kind + " name.").c_str());
    std::vector<Parameter> parameters;
    if (!check(TokenType::RParen)) {
        do {
            if (parameters.size() >= 255) {
                _had_error = true;
                std::cerr << "Parse Error: Cannot have more than 255 parameters." << std::endl;
            }
            Token param_name = consume(TokenType::Identifier, "Expect parameter name.");
            consume(TokenType::Colon, "Expect ':' after parameter name.");
            parameters.push_back({param_name, type()});
        } while (match({TokenType::Comma}));
    }
    consume(TokenType::RParen, "Expect ')' after parameters.");

    std::unique_ptr<TypeExpr> return_type = nullptr;
    if (match({TokenType::Arrow})) {
        return_type = type();
    }

    consume(TokenType::LBrace, ("Expect '{' before " + kind + " body.").c_str());
    std::vector<std::unique_ptr<Stmt>> body = block();
    return std::make_unique<FunctionStmt>(name, std::move(parameters), std::move(return_type), std::move(body));
}

std::unique_ptr<Stmt> Parser::var_declaration() {
    Token name = consume(TokenType::Identifier, "Expect variable name.");
    std::unique_ptr<TypeExpr> type_expr = nullptr;
    if (match({TokenType::Colon})) {
        type_expr = type();
    }
    std::unique_ptr<Expr> initializer = nullptr;
    if (match({TokenType::Equal})) {
        initializer = expression();
    }
    consume(TokenType::Semicolon, "Expect ';' after variable declaration.");
    return std::make_unique<VarStmt>(name, std::move(type_expr), std::move(initializer));
}

std::unique_ptr<Stmt> Parser::let_declaration() {
    Token name = consume(TokenType::Identifier, "Expect constant name.");
    std::unique_ptr<TypeExpr> type_expr = nullptr;
    if (match({TokenType::Colon})) {
        type_expr = type();
    }
    consume(TokenType::Equal, "Expect '=' after constant name.");
    std::unique_ptr<Expr> initializer = expression();
    consume(TokenType::Semicolon, "Expect ';' after constant declaration.");
    return std::make_unique<LetStmt>(name, std::move(type_expr), std::move(initializer));
}

std::unique_ptr<Stmt> Parser::statement() {
    if (match({TokenType::If})) {
        consume(TokenType::LParen, "Expect '(' after 'if'.");
        auto condition = expression();
        consume(TokenType::RParen, "Expect ')' after if condition.");
        auto then_branch = statement();
        std::unique_ptr<Stmt> else_branch = nullptr;
        if (match({TokenType::Else})) {
            else_branch = statement();
        }
        return std::make_unique<IfStmt>(std::move(condition), std::move(then_branch), std::move(else_branch));
    }
    if (match({TokenType::While})) {
        consume(TokenType::LParen, "Expect '(' after 'while'.");
        auto condition = expression();
        consume(TokenType::RParen, "Expect ')' after while condition.");
        auto body = statement();
        return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
    }
    if (match({TokenType::Return})) {
        Token keyword = previous();
        std::unique_ptr<Expr> value = nullptr;
        if (!check(TokenType::Semicolon)) {
            value = expression();
        }
        consume(TokenType::Semicolon, "Expect ';' after return value.");
        return std::make_unique<ReturnStmt>(keyword, std::move(value));
    }
    if (match({TokenType::LBrace})) {
        return std::make_unique<BlockStmt>(block());
    }
    return expression_statement();
}

std::vector<std::unique_ptr<Stmt>> Parser::block() {
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!check(TokenType::RBrace) && !is_at_end()) {
        statements.push_back(declaration());
    }
    consume(TokenType::RBrace, "Expect '}' after block.");
    return statements;
}


std::unique_ptr<Stmt> Parser::expression_statement() {
    try {
        std::unique_ptr<Expr> expr = expression();
        consume(TokenType::Semicolon, "Expect ';' after expression.");
        return std::make_unique<ExpressionStmt>(std::move(expr));
    } catch (const std::runtime_error& error) {
        synchronize();
        return nullptr;
    }
}

std::unique_ptr<Expr> Parser::expression() {
    return assignment();
}

std::unique_ptr<Expr> Parser::assignment() {
    std::unique_ptr<Expr> expr = equality();

    if (match({TokenType::Equal})) {
        Token equals = previous();
        std::unique_ptr<Expr> value = assignment();

        if (auto* var_expr = dynamic_cast<VariableExpr*>(expr.get())) {
            Token name = var_expr->name;
            return std::make_unique<AssignExpr>(name, std::move(value));
        }

        _had_error = true;
        std::cerr << "Parse Error: Invalid assignment target at line " << equals.line << std::endl;
    }

    return expr;
}

std::unique_ptr<Expr> Parser::equality() {
    std::unique_ptr<Expr> expr = comparison();
    while (match({TokenType::BangEqual, TokenType::EqualEqual})) {
        Token op = previous();
        std::unique_ptr<Expr> right = comparison();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::comparison() {
    std::unique_ptr<Expr> expr = term();
    while (match({TokenType::Greater, TokenType::GreaterEqual, TokenType::Less, TokenType::LessEqual})) {
        Token op = previous();
        std::unique_ptr<Expr> right = term();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::term() {
    std::unique_ptr<Expr> expr = factor();
    while (match({TokenType::Minus, TokenType::Plus})) {
        Token op = previous();
        std::unique_ptr<Expr> right = factor();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::factor() {
    std::unique_ptr<Expr> expr = unary();
    while (match({TokenType::Slash, TokenType::Star, TokenType::Percent})) {
        Token op = previous();
        std::unique_ptr<Expr> right = unary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

std::unique_ptr<Expr> Parser::unary() {
    if (match({TokenType::Bang, TokenType::Minus})) {
        Token op = previous();
        std::unique_ptr<Expr> right = unary();
        return std::make_unique<UnaryExpr>(op, std::move(right));
    }
    return primary();
}

std::unique_ptr<Expr> Parser::primary() {
    if (match({TokenType::False, TokenType::True, TokenType::Integer, TokenType::Float, TokenType::String})) {
        return std::make_unique<LiteralExpr>(previous());
    }

    if (match({TokenType::LParen})) {
        std::unique_ptr<Expr> expr = expression();
        consume(TokenType::RParen, "Expect ')' after expression.");
        return std::make_unique<GroupingExpr>(std::move(expr));
    }

    if (match({TokenType::Identifier})) {
        Token name = previous();
        if (match({TokenType::LParen})) {
            std::vector<std::unique_ptr<Expr>> arguments;
            if (!check(TokenType::RParen)) {
                do {
                    arguments.push_back(expression());
                } while (match({TokenType::Comma}));
            }
            Token paren = consume(TokenType::RParen, "Expect ')' after arguments.");
            return std::make_unique<CallExpr>(std::make_unique<VariableExpr>(name), paren, std::move(arguments));
        }
        return std::make_unique<VariableExpr>(name);
    }

    _had_error = true;
    std::cerr << "Parse Error: Expect expression at line " << peek().line << std::endl;
    throw std::runtime_error("Expect expression.");
}

std::unique_ptr<TypeExpr> Parser::type() {
    if (match({TokenType::I32, TokenType::I16, TokenType::I8, TokenType::I2, TokenType::Bool, TokenType::Void, TokenType::T81BigInt, TokenType::T81Float, TokenType::T81Fraction, TokenType::Vector, TokenType::Matrix, TokenType::Tensor, TokenType::Graph, TokenType::Identifier})) {
        return std::make_unique<TypeExpr>(previous());
    }

    _had_error = true;
    std::cerr << "Parse Error: Expect type name at line " << peek().line << std::endl;
    throw std::runtime_error("Expect type name.");
}

void Parser::synchronize() {
    advance();

    while (!is_at_end()) {
        if (previous().type == TokenType::Semicolon) return;

        switch (peek().type) {
            case TokenType::Fn:
            case TokenType::Let:
            case TokenType::Var:
            case TokenType::For:
            case TokenType::If:
            case TokenType::While:
            case TokenType::Return:
                return;
            default:
                ; // Do nothing.
        }

        advance();
    }
}

} // namespace frontend
} // namespace t81
