/**
 * @file parser.cpp
 * @brief Implements the Parser for the T81Lang frontend.
 *
 * This file contains the implementation of a recursive descent parser that
 * consumes a stream of tokens from the Lexer and produces an Abstract Syntax Tree (AST).
 * The parser is designed to be reasonably performant and to support basic
 * error recovery via synchronization.
 */

#include "t81/frontend/parser.hpp"
#include <iostream>

namespace t81 {
namespace frontend {

/**
 * @brief Constructs a new Parser.
 * @param lexer The Lexer instance providing the token stream.
 */
Parser::Parser(Lexer& lexer, std::string source_name)
    : _lexer(lexer), _source_name(std::move(source_name)) {
    // Prime the pump by fetching the first token. This ensures that `_current`
    // is valid before any parsing methods are called.
    _current = _lexer.next_token();
}

void Parser::report_error(const Token& token, const std::string& message) {
    const std::string file = _source_name.empty() ? "<source>" : _source_name;
    std::cerr << file << ':' << token.line << ':' << token.column
              << ": error: " << message << '\n';
    _had_error = true;
}

/**
 * @brief Parses the entire token stream and produces a list of statements.
 * @return A vector of unique_ptrs to the root statements of the AST.
 */
std::vector<std::unique_ptr<Stmt>> Parser::parse() {
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!is_at_end()) {
        statements.push_back(declaration());
    }
    return statements;
}

// --- Private Helper Methods ---

// Checks if the current token matches any of the given types. If so,
// it consumes the token and returns true.
bool Parser::match(const std::vector<TokenType>& types) {
    for (TokenType type : types) {
        if (check(type)) {
            advance();
            return true;
        }
    }
    return false;
}

// Returns true if the current token is of the given type, without consuming it.
bool Parser::check(TokenType type) {
    if (is_at_end()) return false;
    return peek().type == type;
}

// Consumes the current token and returns the previous token.
Token Parser::advance() {
    if (!is_at_end()) {
        _previous = _current;
        _current = _lexer.next_token();
    }
    return previous();
}

// Returns true if the parser has reached the end of the token stream.
bool Parser::is_at_end() {
    return peek().type == TokenType::Eof;
}

// Returns the current token without consuming it.
Token Parser::peek() {
    return _current;
}

// Returns the most recently consumed token.
Token Parser::previous() {
    return _previous;
}

// Consumes the current token if it matches the expected type. If not, it
// reports an error and returns a dummy token.
Token Parser::consume(TokenType type, const char* message) {
    if (check(type)) return advance();
    report_error(peek(), message);
    return {};
}

// Discards tokens until it finds a likely statement boundary. This is a
// simple panic-mode error recovery mechanism that helps report more than
// one error per file.
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

// --- Grammar Rules ---

// Parses a declaration.
// declaration -> fn_declaration | var_declaration | let_declaration | statement ;
std::unique_ptr<Stmt> Parser::declaration() {
    try {
        auto struct_attrs = parse_structural_attributes();
        if (match({TokenType::Type})) return type_declaration();
        if (match({TokenType::Record})) return record_declaration(struct_attrs);
        if (match({TokenType::Enum})) return enum_declaration(struct_attrs);
        if (struct_attrs.has_value()) {
            const Token& anchor = struct_attrs->anchor.value_or(peek());
            report_error(anchor, "Structural attributes may only decorate records or enums.");
        }
        if (match({TokenType::Fn})) return function("function");
        if (match({TokenType::Var})) return var_declaration();
        if (match({TokenType::Let})) return let_declaration();
        return statement();
    } catch (const std::runtime_error& error) {
        synchronize();
        return nullptr;
    }
}

// Parses a function declaration.
// function -> "fn" IDENTIFIER "(" parameters? ")" ( "->" type )? "{" block "}" ;
std::unique_ptr<Stmt> Parser::function(const std::string& kind) {
    Token name = consume(TokenType::Identifier, ("Expect " + kind + " name.").c_str());
    consume(TokenType::LParen, ("Expect '(' after " + kind + " name.").c_str());
    std::vector<Parameter> parameters;
    if (!check(TokenType::RParen)) {
        do {
        if (parameters.size() >= 255) {
            report_error(peek(), "Cannot have more than 255 parameters.");
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

std::unique_ptr<Stmt> Parser::type_declaration() {
    Token name = consume(TokenType::Identifier, "Expect type name.");
    std::vector<Token> parameters;
    if (match({TokenType::LBracket})) {
        do {
            if (parameters.size() >= 8) {
                report_error(peek(), "Too many generic parameters (max 8)");
                break;
            }
            parameters.push_back(consume(TokenType::Identifier, "Expect generic parameter name."));
        } while (match({TokenType::Comma}));
        consume(TokenType::RBracket, "Expect ']' after generic parameters.");
    }
    consume(TokenType::Equal, "Expect '=' after type declaration.");
    std::unique_ptr<TypeExpr> alias = type();
    consume(TokenType::Semicolon, "Expect ';' after type declaration.");
    return std::make_unique<TypeDecl>(name, std::move(parameters), std::move(alias));
}

std::unique_ptr<Stmt> Parser::record_declaration(std::optional<StructuralAttributes> attributes) {
    Token name = consume(TokenType::Identifier, "Expect record name.");
    consume(TokenType::LBrace, "Expect '{' after record name.");
    std::vector<RecordDecl::Field> fields;
    while (!check(TokenType::RBrace) && !is_at_end()) {
        Token field_name = consume(TokenType::Identifier, "Expect field name.");
        consume(TokenType::Colon, "Expect ':' after field name.");
        auto field_type = type();
        consume(TokenType::Semicolon, "Expect ';' after field declaration.");
        fields.push_back({field_name, std::move(field_type)});
    }
    consume(TokenType::RBrace, "Expect '}' after record declaration.");
    consume(TokenType::Semicolon, "Expect ';' after record declaration.");
    std::optional<std::int64_t> schema_version;
    std::optional<std::string> module_path;
    if (attributes) {
        schema_version = attributes->schema_version;
        module_path = attributes->module_path;
    }
    return std::make_unique<RecordDecl>(name, std::move(fields), schema_version, module_path);
}

std::unique_ptr<Stmt> Parser::enum_declaration(std::optional<StructuralAttributes> attributes) {
    Token name = consume(TokenType::Identifier, "Expect enum name.");
    consume(TokenType::LBrace, "Expect '{' after enum name.");
    std::vector<EnumDecl::Variant> variants;
    while (!check(TokenType::RBrace) && !is_at_end()) {
        Token variant = consume(TokenType::Identifier, "Expect variant name.");
        std::unique_ptr<TypeExpr> payload = nullptr;
        if (match({TokenType::LParen})) {
            payload = type();
            consume(TokenType::RParen, "Expect ')' after variant payload type.");
        }
        consume(TokenType::Semicolon, "Expect ';' after variant declaration.");
        variants.push_back({variant, std::move(payload)});
    }
    consume(TokenType::RBrace, "Expect '}' after enum declaration.");
    consume(TokenType::Semicolon, "Expect ';' after enum declaration.");
    std::optional<std::int64_t> schema_version;
    std::optional<std::string> module_path;
    if (attributes) {
        schema_version = attributes->schema_version;
        module_path = attributes->module_path;
    }
    return std::make_unique<EnumDecl>(name, std::move(variants), schema_version, module_path);
}

// Parses a variable declaration.
// var_declaration -> "var" IDENTIFIER ( ":" type )? ( "=" expression )? ";" ;
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

// Parses a constant declaration.
// let_declaration -> "let" IDENTIFIER ( ":" type )? "=" expression ";" ;
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

// Parses a statement.
// statement -> if_stmt | while_stmt | return_stmt | block | expr_stmt ;
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
    if (check(TokenType::At) || check(TokenType::Loop)) {
        return loop_statement();
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

std::unique_ptr<Stmt> Parser::loop_statement() {
    LoopStmt::BoundKind loop_bound_kind = LoopStmt::BoundKind::None;
    std::optional<std::int64_t> loop_bound_value;
    Token loop_attr{};
    bool saw_annotation = parse_loop_annotation(loop_bound_kind, loop_bound_value, loop_attr);

    Token loop_token = consume(TokenType::Loop, "Expect 'loop' keyword.");

    if (saw_annotation && loop_token.type != TokenType::Loop) {
        report_error(loop_attr, "'@bounded' annotation must be followed by a 'loop' statement");
    }

    consume(TokenType::LBrace, "Expect '{' after 'loop'.");
    auto body = block();
    return std::make_unique<LoopStmt>(loop_token, loop_bound_kind, loop_bound_value, std::move(body));
}

// Parses a block of statements.
// block -> "{" declaration* "}" ;
std::vector<std::unique_ptr<Stmt>> Parser::block() {
    std::vector<std::unique_ptr<Stmt>> statements;
    while (!check(TokenType::RBrace) && !is_at_end()) {
        statements.push_back(declaration());
    }
    consume(TokenType::RBrace, "Expect '}' after block.");
    return statements;
}

// Parses an expression statement.
// expr_stmt -> expression ";" ;
std::unique_ptr<Stmt> Parser::expression_statement() {
    std::unique_ptr<Expr> expr = expression();
    consume(TokenType::Semicolon, "Expect ';' after expression.");
    return std::make_unique<ExpressionStmt>(std::move(expr));
}

// Parses an expression.
// expression -> assignment ;
std::unique_ptr<Expr> Parser::expression() {
    return assignment();
}

// Parses an assignment expression.
// assignment -> IDENTIFIER "=" assignment | equality ;
std::unique_ptr<Expr> Parser::assignment() {
    std::unique_ptr<Expr> expr = equality();
    if (match({TokenType::Equal})) {
        Token equals = previous();
        std::unique_ptr<Expr> value = assignment();
        if (auto* var_expr = dynamic_cast<VariableExpr*>(expr.get())) {
            Token name = var_expr->name;
            return std::make_unique<AssignExpr>(name, std::move(value));
        }
        report_error(equals, "Invalid assignment target");
    }
    return expr;
}

// Parses an equality expression.
// equality -> comparison ( ( "!=" | "==" ) comparison )* ;
std::unique_ptr<Expr> Parser::equality() {
    std::unique_ptr<Expr> expr = comparison();
    while (match({TokenType::BangEqual, TokenType::EqualEqual})) {
        Token op = previous();
        std::unique_ptr<Expr> right = comparison();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

// Parses a comparison expression.
// comparison -> term ( ( ">" | ">=" | "<" | "<=" ) term )* ;
std::unique_ptr<Expr> Parser::comparison() {
    std::unique_ptr<Expr> expr = term();
    while (match({TokenType::Greater, TokenType::GreaterEqual, TokenType::Less, TokenType::LessEqual})) {
        Token op = previous();
        std::unique_ptr<Expr> right = term();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

// Parses an addition/subtraction expression.
// term -> factor ( ( "-" | "+" ) factor )* ;
std::unique_ptr<Expr> Parser::term() {
    std::unique_ptr<Expr> expr = factor();
    while (match({TokenType::Minus, TokenType::Plus})) {
        Token op = previous();
        std::unique_ptr<Expr> right = factor();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

// Parses a multiplication/division/modulo expression.
// factor -> unary ( ( "/" | "*" | "%" ) unary )* ;
std::unique_ptr<Expr> Parser::factor() {
    std::unique_ptr<Expr> expr = unary();
    while (match({TokenType::Slash, TokenType::Star, TokenType::Percent})) {
        Token op = previous();
        std::unique_ptr<Expr> right = unary();
        expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
    }
    return expr;
}

// Parses a unary expression.
// unary -> ( "!" | "-" ) unary | call ;
std::unique_ptr<Expr> Parser::unary() {
    if (match({TokenType::Bang, TokenType::Minus})) {
        Token op = previous();
        std::unique_ptr<Expr> right = unary();
        return std::make_unique<UnaryExpr>(op, std::move(right));
    }
    return primary();
}

// Parses a primary expression, which is the highest-precedence expression.
// primary -> "false" | "true" | INTEGER | FLOAT | STRING | "(" expression ")" | IDENTIFIER ;
std::unique_ptr<Expr> Parser::primary() {
    if (match({TokenType::Match})) {
        return match_expression();
    }

    if (match({TokenType::False, TokenType::True, TokenType::Integer, TokenType::Float, TokenType::String})) {
        return std::make_unique<LiteralExpr>(previous());
    }

    if (match({TokenType::LBracket})) {
        Token bracket = previous();
        std::vector<std::unique_ptr<Expr>> elements;
        if (!check(TokenType::RBracket)) {
            do {
                elements.push_back(expression());
            } while (match({TokenType::Comma}));
        }
        consume(TokenType::RBracket, "Expect ']' after vector literal.");
        return std::make_unique<VectorLiteralExpr>(bracket, std::move(elements));
    }

    if (match({TokenType::LParen})) {
        std::unique_ptr<Expr> expr = expression();
        consume(TokenType::RParen, "Expect ')' after expression.");
        return std::make_unique<GroupingExpr>(std::move(expr));
    }

    if (match({TokenType::Identifier})) {
        Token name = previous();
        if (check(TokenType::LBracket)) {
            return parse_generic_type(name);
        }
        if (match({TokenType::LBrace})) {
            return record_literal(std::move(name));
        }
        std::unique_ptr<Expr> expr;
        if (match({TokenType::LParen})) {
            std::vector<std::unique_ptr<Expr>> arguments;
            if (!check(TokenType::RParen)) {
                do {
                    arguments.push_back(expression());
                } while (match({TokenType::Comma}));
            }
            Token paren = consume(TokenType::RParen, "Expect ')' after arguments.");
            expr = std::make_unique<CallExpr>(std::make_unique<VariableExpr>(name), paren, std::move(arguments));
        } else {
            expr = std::make_unique<VariableExpr>(name);
        }
        while (match({TokenType::Dot})) {
            Token field = consume(TokenType::Identifier, "Expect field name after '.'.");
            expr = std::make_unique<FieldAccessExpr>(std::move(expr), field);
        }
        return expr;
    }

    report_error(peek(), "Expect expression.");
    throw std::runtime_error("Expect expression.");
}

std::unique_ptr<Expr> Parser::match_expression() {
    consume(TokenType::LParen, "Expect '(' after 'match'.");
    std::unique_ptr<Expr> scrutinee = expression();
    consume(TokenType::RParen, "Expect ')' after match scrutinee.");
    consume(TokenType::LBrace, "Expect '{' before match arms.");

    std::vector<MatchArm> arms;
    while (!check(TokenType::RBrace) && !is_at_end()) {
        arms.push_back(match_arm());
        if (match({TokenType::Semicolon, TokenType::Comma})) {
            continue;
        }
        break;
    }

    consume(TokenType::RBrace, "Expect '}' after match arms.");
    return std::make_unique<MatchExpr>(std::move(scrutinee), std::move(arms));
}

std::unique_ptr<Expr> Parser::record_literal(Token type_name) {
    std::vector<std::pair<Token, std::unique_ptr<Expr>>> fields;
    if (!check(TokenType::RBrace)) {
        do {
            Token field_name = consume(TokenType::Identifier, "Expect field name in record literal.");
            consume(TokenType::Colon, "Expect ':' after field name.");
            auto value = expression();
            fields.emplace_back(field_name, std::move(value));
        } while (match({TokenType::Comma, TokenType::Semicolon}));
    }
    consume(TokenType::RBrace, "Expect '}' after record literal.");
    return std::make_unique<RecordLiteralExpr>(type_name, std::move(fields));
}

MatchArm Parser::match_arm() {
    Token keyword = consume(TokenType::Identifier, "Expect match arm variant.");
    bool has_binding = false;
    Token binding{};
    bool binding_is_wildcard = false;

    if (match({TokenType::LParen})) {
        binding = consume(TokenType::Identifier, "Expect binding identifier.");
        has_binding = true;
        binding_is_wildcard = binding.lexeme == "_";
        consume(TokenType::RParen, "Expect ')' after match binding.");
    }

    consume(TokenType::FatArrow, "Expect '=>' after match arm pattern.");
    std::unique_ptr<Expr> body = expression();
    return MatchArm(keyword, has_binding, binding, binding_is_wildcard, std::move(body));
}

bool Parser::parse_loop_annotation(LoopStmt::BoundKind& bound_kind,
                                  std::optional<std::int64_t>& bound_value,
                                  Token& attr_token) {
    if (!match({TokenType::At})) {
        return false;
    }
    Token name = consume(TokenType::Identifier, "Expect attribute name after '@'.");
    attr_token = name;

    if (std::string_view{name.lexeme} != "bounded") {
        report_error(name, "Unsupported annotation '" + std::string(name.lexeme) + "'");
    }

    consume(TokenType::LParen, "Expect '(' after annotation name.");
    bound_kind = LoopStmt::BoundKind::None;
    bound_value.reset();
    Token arg;
    if (match({TokenType::Identifier})) {
        arg = previous();
        if (std::string_view{arg.lexeme} == "infinite") {
            bound_kind = LoopStmt::BoundKind::Infinite;
        } else {
            report_error(arg, "'@bounded' only accepts 'infinite' or an integer");
        }
    } else if (match({TokenType::Integer})) {
        arg = previous();
        try {
            bound_kind = LoopStmt::BoundKind::Static;
            bound_value = std::stoll(std::string(arg.lexeme));
        } catch (const std::exception&) {
            report_error(arg, std::string("Invalid loop bound '") + std::string(arg.lexeme) + "'");
        }
    } else {
        report_error(peek(), "'@bounded' requires an argument");
    }
    consume(TokenType::RParen, "Expect ')' after annotation argument.");

    return true;
}

std::optional<StructuralAttributes> Parser::parse_structural_attributes() {
    StructuralAttributes attrs;
    bool seen = false;
    while (check(TokenType::At)) {
        Token lookahead = _lexer.peek_next_token();
        if (lookahead.type != TokenType::Identifier) {
            break;
        }
        std::string attr_candidate{lookahead.lexeme};
        if (attr_candidate != "schema" && attr_candidate != "module") {
            break;
        }
        match({TokenType::At});
        Token name = consume(TokenType::Identifier, "Expect attribute name after '@'.");
        std::string attr_name{name.lexeme};
        if (!seen) {
            attrs.anchor = name;
        }
        seen = true;
        consume(TokenType::LParen, "Expect '(' after attribute name.");

        if (attr_name == "schema") {
            if (attrs.schema_version.has_value()) {
                report_error(name, "Duplicate '@schema' attribute.");
            }
            Token value = consume(TokenType::Integer, "Expect integer schema version.");
            try {
                std::int64_t version = std::stoll(std::string(value.lexeme));
                if (version <= 0) {
                    report_error(value, "Schema version must be positive.");
                } else {
                    attrs.schema_version = version;
                }
            } catch (const std::exception&) {
                report_error(value, "Invalid integer for schema version.");
            }
        } else if (attr_name == "module") {
            if (attrs.module_path.has_value()) {
                report_error(name, "Duplicate '@module' attribute.");
            }
            Token segment = consume(TokenType::Identifier, "Expect module name.");
            std::string path(segment.lexeme);
            while (match({TokenType::Dot})) {
                Token next = consume(TokenType::Identifier, "Expect module segment after '.'.");
                path.push_back('.');
                path.append(next.lexeme.data(), next.lexeme.size());
            }
            attrs.module_path = std::move(path);
        } else {
            report_error(name, "Unsupported attribute '" + attr_name + "'");
            while (!check(TokenType::RParen) && !is_at_end()) {
                advance();
            }
        }

        consume(TokenType::RParen, "Expect ')' after attribute.");
    }
    if (!seen) {
        return std::nullopt;
    }
    return attrs;
}

std::unique_ptr<GenericTypeExpr> Parser::parse_generic_type(Token name) {
    consume(TokenType::LBracket, "Expect '[' after generic type name.");
    std::array<std::unique_ptr<Expr>, 8> parameters;
    size_t param_count = 0;
    std::string_view type_name{name.lexeme};

    // First parameter must be a type.
    parameters[param_count++] = type();

    // Subsequent parameters are constant value expressions (structural-result types are treated specially).
    while (match({TokenType::Comma})) {
        if (param_count >= 8) {
            report_error(peek(), "Too many generic parameters (max 8)");
            return nullptr;
        }
        if (type_name == "Result" && param_count == 1) {
            parameters[param_count++] = type();
            continue;
        }
        parameters[param_count++] = primary();
    }

    consume(TokenType::RBracket, "Expect ']' after type parameters.");
    return std::make_unique<GenericTypeExpr>(name, std::move(parameters), param_count);
}

// Parses a type expression.
// type -> (IDENTIFIER | primitive_type_keyword) ( "[" type ( "," expression )* "]" )? ;
std::unique_ptr<TypeExpr> Parser::type() {
    if (!check(TokenType::Identifier) &&
        !check(TokenType::I32) && !check(TokenType::I16) && !check(TokenType::I8) && !check(TokenType::I2) &&
        !check(TokenType::Bool) && !check(TokenType::Void) &&
        !check(TokenType::T81BigInt) && !check(TokenType::T81Float) && !check(TokenType::T81Fraction) &&
        !check(TokenType::Vector) && !check(TokenType::Matrix) && !check(TokenType::Tensor) && !check(TokenType::Graph)) {
        report_error(peek(), "Expect type name");
        return nullptr;
    }
    Token name = advance();

    // Explicitly reject legacy angle bracket syntax
    if (peek().type == TokenType::Less) {
        report_error(peek(), "Legacy '<...>' syntax for generics is not supported. Use '[...]' instead.");
        return nullptr;
    }

    if (check(TokenType::LBracket)) {
        return parse_generic_type(name);
    }

    return std::make_unique<SimpleTypeExpr>(name);
}

} // namespace frontend
} // namespace t81
