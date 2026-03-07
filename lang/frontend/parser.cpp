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
#include <cctype>
#include <iostream>
#include <stdexcept>
#include "t81/frontend/semantic_analyzer.hpp"

namespace t81 {
namespace frontend {

namespace {

std::string token_type_name(TokenType type) {
  switch (type) {
    case TokenType::Module:
      return "'module'";
    case TokenType::Type:
      return "'type'";
    case TokenType::Const:
      return "'const'";
    case TokenType::Export:
      return "'export'";
    case TokenType::Fn:
      return "'fn'";
    case TokenType::Let:
      return "'let'";
    case TokenType::Var:
      return "'var'";
    case TokenType::Record:
      return "'record'";
    case TokenType::Enum:
      return "'enum'";
    case TokenType::If:
      return "'if'";
    case TokenType::Else:
      return "'else'";
    case TokenType::For:
      return "'for'";
    case TokenType::In:
      return "'in'";
    case TokenType::While:
      return "'while'";
    case TokenType::Loop:
      return "'loop'";
    case TokenType::Reflect:
      return "'reflect'";
    case TokenType::Recurse:
      return "'recurse'";
    case TokenType::Distributed:
      return "'distributed'";
    case TokenType::Infinite:
      return "'infinite'";
    case TokenType::Symbol:
      return "symbol literal";
    case TokenType::InfiniteLiteral:
      return "infinite literal";
    case TokenType::Infer:
      return "'infer'";
    case TokenType::Train:
      return "'train'";
    case TokenType::Break:
      return "'break'";
    case TokenType::Continue:
      return "'continue'";
    case TokenType::Return:
      return "'return'";
    case TokenType::Assert:
      return "'assert'";
    case TokenType::As:
      return "'as'";
    case TokenType::Mut:
      return "'mut'";
    case TokenType::Match:
      return "'match'";
    case TokenType::True:
      return "'true'";
    case TokenType::False:
      return "'false'";
    case TokenType::Void:
      return "'void'";
    case TokenType::Bool:
      return "'bool'";
    case TokenType::I32:
      return "'i32'";
    case TokenType::I16:
      return "'i16'";
    case TokenType::I8:
      return "'i8'";
    case TokenType::I2:
      return "'i2'";
    case TokenType::T81BigInt:
      return "'T81BigInt'";
    case TokenType::T81Float:
      return "'T81Float'";
    case TokenType::T81Prob:
      return "'T81Prob'";
    case TokenType::T81Fraction:
    case TokenType::Cell:
      return "'Cell'";
    case TokenType::T81Qutrit:
      return "'T81Qutrit'";
    case TokenType::T81Uint:
      return "'T81Uint'";
    case TokenType::T81String:
      return "'T81String'";
    case TokenType::T81Vector:
      return "'T81Vector'";
    case TokenType::Matrix:
      return "'matrix'";
    case TokenType::Tensor:
      return "'tensor'";
    case TokenType::Graph:
      return "'graph'";
    case TokenType::List:
      return "'list'";
    case TokenType::Map:
      return "'map'";
    case TokenType::Set:
      return "'set'";
    case TokenType::Tree:
      return "'tree'";
    case TokenType::Integer:
      return "integer literal";
    case TokenType::Float:
      return "float literal";
    case TokenType::String:
      return "string literal";
    case TokenType::ByteString:
      return "byte string literal";
    case TokenType::Ternary:
      return "ternary literal";
    case TokenType::Base81Integer:
      return "base81 integer literal";
    case TokenType::Base81Float:
      return "base81 float literal";
    case TokenType::T81Fixed:
      return "T81Fixed literal";
    case TokenType::T81Complex:
      return "T81Complex literal";
    case TokenType::T81Quaternion:
      return "T81Quaternion literal";
    case TokenType::Identifier:
      return "identifier";
    case TokenType::Plus:
      return "'+'";
    case TokenType::Minus:
      return "'-'";
    case TokenType::Star:
      return "'*'";
    case TokenType::StarStar:
      return "'**'";
    case TokenType::Slash:
      return "'/'";
    case TokenType::Percent:
      return "'%'";
    case TokenType::Equal:
      return "'='";
    case TokenType::EqualEqual:
      return "'=='";
    case TokenType::Bang:
      return "'!'";
    case TokenType::BangEqual:
      return "'!='";
    case TokenType::Less:
      return "'<'";
    case TokenType::LessEqual:
      return "'<='";
    case TokenType::Greater:
      return "'>'";
    case TokenType::GreaterEqual:
      return "'>='";
    case TokenType::Amp:
      return "'&'";
    case TokenType::AmpAmp:
      return "'&&'";
    case TokenType::Pipe:
      return "'|'";
    case TokenType::PipePipe:
      return "'||'";
    case TokenType::Caret:
      return "'^'";
    case TokenType::Tilde:
      return "'~'";
    case TokenType::LessLess:
      return "'<<'";
    case TokenType::GreaterGreater:
      return "'>>'";
    case TokenType::GreaterGreaterGreater:
      return "'>>>'";
    case TokenType::Question:
      return "'?'";
    case TokenType::LParen:
      return "'('";
    case TokenType::RParen:
      return "')'";
    case TokenType::LBrace:
      return "'{'";
    case TokenType::RBrace:
      return "'}'";
    case TokenType::LBracket:
      return "'['";
    case TokenType::RBracket:
      return "']'";
    case TokenType::Comma:
      return "','";
    case TokenType::Colon:
      return "':'";
    case TokenType::Semicolon:
      return "';'";
    case TokenType::Arrow:
      return "'->'";
    case TokenType::FatArrow:
      return "'=>'";
    case TokenType::DotDot:
      return "'..'";
    case TokenType::DotDotEq:
      return "'..='";
    case TokenType::Dot:
      return "'.'";
    case TokenType::At:
      return "'@'";
    case TokenType::Eof:
      return "end of input";
    case TokenType::Illegal:
      return "illegal token";
  }
  return "token";
}

std::string token_found_description(const Token& token) {
  if (token.type == TokenType::Eof) {
    return "end of input";
  }
  if (!token.lexeme.empty()) {
    return "'" + std::string(token.lexeme) + "'";
  }
  return token_type_name(token.type);
}

bool is_dot_field_segment_token(TokenType type) {
  switch (type) {
    case TokenType::Identifier:
    case TokenType::Module:
    case TokenType::Type:
    case TokenType::Const:
    case TokenType::Export:
    case TokenType::Fn:
    case TokenType::Let:
    case TokenType::Var:
    case TokenType::Record:
    case TokenType::Enum:
    case TokenType::If:
    case TokenType::Else:
    case TokenType::For:
    case TokenType::In:
    case TokenType::While:
    case TokenType::Loop:
    case TokenType::Reflect:
    case TokenType::Break:
    case TokenType::Continue:
    case TokenType::Return:
    case TokenType::Assert:
    case TokenType::As:
    case TokenType::Mut:
    case TokenType::Match:
    case TokenType::True:
    case TokenType::False:
    case TokenType::Void:
    case TokenType::Bool:
    case TokenType::I32:
    case TokenType::I16:
    case TokenType::I8:
    case TokenType::I2:
    case TokenType::T81BigInt:
    case TokenType::T81Float:
    case TokenType::T81Fraction:
    case TokenType::T81Fixed:
    case TokenType::T81Complex:
    case TokenType::T81Quaternion:
    case TokenType::T81Prob:
    case TokenType::Cell:
    case TokenType::T81Qutrit:
    case TokenType::T81Uint:
    case TokenType::T81String:
    case TokenType::T81Vector:
    case TokenType::Matrix:
    case TokenType::Tensor:
    case TokenType::Graph:
    case TokenType::List:
    case TokenType::Map:
    case TokenType::Set:
    case TokenType::Tree:
      return true;
    default:
      return false;
  }
}

}  // namespace

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
  std::cerr << file << ':' << token.line << ':' << token.column << ": error: " << message << '\n';
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
bool Parser::is_at_end() { return peek().type == TokenType::Eof; }

// Returns the current token without consuming it.
Token Parser::peek() { return _current; }

// Returns the most recently consumed token.
Token Parser::previous() { return _previous; }

// Consumes the current token if it matches the expected type. If not, it
// reports an error and returns a dummy token.
Token Parser::consume(TokenType type, const char* message) {
  if (check(type)) return advance();
  const Token found = peek();
  std::string detailed = std::string(message) + " (expected " + token_type_name(type) + ", found " +
                         token_found_description(found) + ")";
  report_error(found, detailed);
  throw std::runtime_error(detailed);
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
      default:;  // Do nothing.
    }
    advance();
  }
}

// --- Grammar Rules ---

// Parses a declaration.
// declaration -> fn_declaration | var_declaration | let_declaration | recurse_declaration |
// statement ;
std::unique_ptr<Stmt> Parser::declaration() {
  try {
    auto struct_attrs = parse_structural_attributes();
    auto function_attrs = parse_function_attributes();
    if (match({TokenType::Type})) return type_declaration();
    if (match({TokenType::Record})) return record_declaration(struct_attrs);
    if (match({TokenType::Enum})) return enum_declaration(struct_attrs);
    if (struct_attrs.has_value()) {
      const Token& anchor = struct_attrs->anchor.value_or(peek());
      report_error(anchor, "Structural attributes may only decorate records or enums.");
    }
    if (match({TokenType::Fn})) return function("function", std::move(function_attrs));
    if (match({TokenType::Recurse})) return recurse_declaration();
    if (function_attrs.has_value()) {
      const Token& anchor = function_attrs->anchor.value_or(peek());
      report_error(anchor, "Function attributes may only decorate functions.");
    }
    if (match({TokenType::Var})) return var_declaration();
    if (match({TokenType::Let})) return let_declaration();
    return statement();
  } catch (const std::runtime_error& error) {
    synchronize();
    return nullptr;
  }
}

std::unique_ptr<Stmt> Parser::recurse_declaration() {
  Token keyword = previous();
  Token name = consume(TokenType::Identifier, "Expect recursive function name.");
  consume(TokenType::LParen, "Expect '(' after name.");
  std::vector<Parameter> parameters;
  if (!check(TokenType::RParen)) {
    do {
      if (parameters.size() >= 255) {
        report_error(peek(), "Cannot have more than 255 parameters.");
      }
      Token param_name = consume(TokenType::Identifier, "Expect parameter name.");
      // Optional type annotation for recurse parameters? Spec example doesn't show types:
      // recurse factorial(n)
      // But T81 is strongly typed. Let's assume types are required or inferred.
      // For now, allow optional type.
      std::unique_ptr<TypeExpr> param_type = nullptr;
      if (match({TokenType::Colon})) {
        param_type = type();
      }
      parameters.push_back({param_name, std::move(param_type)});
    } while (match({TokenType::Comma}));
  }
  consume(TokenType::RParen, "Expect ')' after parameters.");
  consume(TokenType::LBrace, "Expect '{' before body.");
  std::vector<std::unique_ptr<Stmt>> body = block();
  return std::make_unique<RecurseStmt>(keyword, name, std::move(parameters), std::move(body));
}

// Parses a function declaration.
// function -> "fn" IDENTIFIER "(" parameters? ")" ( "->" type )? "{" block "}" ;
std::unique_ptr<Stmt> Parser::function(const std::string& kind,
                                       std::optional<FunctionAttributes> attributes) {
  Token name = consume(TokenType::Identifier, ("Expect " + kind + " name.").c_str());
  std::vector<Token> generic_params;
  if (match({TokenType::LBracket})) {
    do {
      if (generic_params.size() >= 8) {
        report_error(peek(), "Too many generic parameters (max 8)");
        break;
      }
      generic_params.push_back(consume(TokenType::Identifier, "Expect generic parameter name."));
    } while (match({TokenType::Comma}));
    consume(TokenType::RBracket, "Expect ']' after generic parameters.");
  }
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
  std::optional<std::int64_t> tier;
  bool is_pure = false;
  bool is_axion_verify = false;
  bool is_attention = false;
  bool is_qmatmul = false;
  if (attributes.has_value()) {
    if (attributes->tier.has_value()) {
      tier = attributes->tier;
    }
    is_pure = attributes->is_pure;
    is_axion_verify = attributes->is_axion_verify;
    is_attention = attributes->is_attention;
    is_qmatmul = attributes->is_qmatmul;
  }
  return std::make_unique<FunctionStmt>(name, std::move(generic_params), std::move(parameters),
                                        std::move(return_type), std::move(body), tier, is_pure,
                                        is_axion_verify, is_attention, is_qmatmul);
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
    if (!check(TokenType::Identifier)) {
      report_error(peek(), "Expect field name.");
      advance();  // Consume bad token
      // Sync to next semicolon or brace
      while (!check(TokenType::Semicolon) && !check(TokenType::RBrace) && !is_at_end()) {
        advance();
      }
      if (check(TokenType::Semicolon)) advance();
      continue;
    }

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
    if (!check(TokenType::Identifier)) {
      report_error(peek(), "Expect variant name.");
      advance();  // Consume bad token
      // Sync to next semicolon or brace
      while (!check(TokenType::Semicolon) && !check(TokenType::RBrace) && !is_at_end()) {
        advance();
      }
      if (check(TokenType::Semicolon)) advance();
      continue;
    }

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
// let_declaration -> "let" ( "mut" )? IDENTIFIER ( ":" type )? "=" expression ";" ;
std::unique_ptr<Stmt> Parser::let_declaration() {
  bool is_mutable = match({TokenType::Mut});
  Token name = consume(TokenType::Identifier, "Expect constant name.");
  std::unique_ptr<TypeExpr> type_expr = nullptr;
  if (match({TokenType::Colon})) {
    type_expr = type();
  }
  consume(TokenType::Equal, "Expect '=' after constant name.");
  std::unique_ptr<Expr> initializer = expression();
  consume(TokenType::Semicolon, "Expect ';' after constant declaration.");
  return std::make_unique<LetStmt>(name, std::move(type_expr), std::move(initializer), is_mutable);
}

// Parses a statement.
// statement -> if_stmt | while_stmt | return_stmt | block | expr_stmt ;
std::unique_ptr<Stmt> Parser::statement() {
  if (match({TokenType::If})) {
    auto condition = expression();
    auto then_branch = statement();
    std::unique_ptr<Stmt> else_branch = nullptr;
    if (match({TokenType::Else})) {
      else_branch = statement();
    }
    return std::make_unique<IfStmt>(std::move(condition), std::move(then_branch),
                                    std::move(else_branch));
  }
  if (match({TokenType::While})) {
    auto condition = expression();
    auto body = statement();
    return std::make_unique<WhileStmt>(std::move(condition), std::move(body));
  }
  if (match({TokenType::For})) {
    Token iterator = consume(TokenType::Identifier, "Expect iterator name.");
    consume(TokenType::In, "Expect 'in' after iterator name.");
    auto iterable = expression();
    auto body = statement();
    return std::make_unique<ForStmt>(iterator, std::move(iterable), std::move(body));
  }
  if (match({TokenType::Reflect})) {
    Token keyword = previous();
    consume(TokenType::LBrace, "Expect '{' after 'reflect'.");
    std::vector<std::unique_ptr<Stmt>> body = block();
    return std::make_unique<ReflectStmt>(keyword, std::move(body));
  }
  if (match({TokenType::Distributed})) {
    Token keyword = previous();
    consume(TokenType::LBrace, "Expect '{' after 'distributed'.");
    std::vector<std::unique_ptr<Stmt>> body = block();
    return std::make_unique<DistributedStmt>(keyword, std::move(body));
  }
  if (match({TokenType::Infinite})) {
    Token keyword = previous();
    consume(TokenType::LBrace, "Expect '{' after 'infinite'.");
    std::vector<std::unique_ptr<Stmt>> body = block();
    return std::make_unique<InfiniteStmt>(keyword, std::move(body));
  }
  if (match({TokenType::Train})) {
    Token keyword = previous();
    consume(TokenType::LParen, "Expect '(' after 'train'.");
    auto model = expression();
    consume(TokenType::RParen, "Expect ')' after train model expression.");
    consume(TokenType::LBrace, "Expect '{' before train body.");
    std::vector<std::unique_ptr<Stmt>> body = block();
    return std::make_unique<TrainStmt>(keyword, std::move(model), std::move(body));
  }
  if (check(TokenType::At) || check(TokenType::Loop)) {
    return loop_statement();
  }
  if (match({TokenType::Break})) {
    Token keyword = previous();
    consume(TokenType::Semicolon, "Expect ';' after 'break'.");
    return std::make_unique<BreakStmt>(keyword);
  }
  if (match({TokenType::Continue})) {
    Token keyword = previous();
    consume(TokenType::Semicolon, "Expect ';' after 'continue'.");
    return std::make_unique<ContinueStmt>(keyword);
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
  if (match({TokenType::Assert})) {
    Token keyword = previous();
    auto expr = expression();
    consume(TokenType::Semicolon, "Expect ';' after assert.");
    return std::make_unique<AssertStmt>(keyword, std::move(expr));
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
  std::unique_ptr<Expr> guard_expr;
  bool saw_annotation =
      parse_loop_annotation(loop_bound_kind, loop_bound_value, loop_attr, guard_expr);

  if (match({TokenType::For})) {
    Token iterator = consume(TokenType::Identifier, "Expect iterator name.");
    consume(TokenType::In, "Expect 'in' after iterator name.");
    auto iterable = expression();
    auto body = statement();
    return std::make_unique<ForStmt>(iterator, std::move(iterable), std::move(body),
                                     loop_bound_kind, loop_bound_value);
  }

  Token loop_token = consume(TokenType::Loop, "Expect 'loop' keyword.");

  if (saw_annotation && loop_token.type != TokenType::Loop) {
    report_error(loop_attr,
                 "'@bounded' annotation must be followed by a 'loop' or 'for' statement");
  }

  consume(TokenType::LBrace, "Expect '{' after 'loop'.");
  auto body = block();
  return std::make_unique<LoopStmt>(loop_token, loop_bound_kind, loop_bound_value,
                                    std::move(guard_expr), std::move(body));
}

std::pair<std::vector<std::unique_ptr<Stmt>>, std::unique_ptr<Expr>> Parser::parse_block_body() {
  std::vector<std::unique_ptr<Stmt>> statements;
  std::unique_ptr<Expr> final_expr = nullptr;

  while (!check(TokenType::RBrace) && !is_at_end()) {
    try {
      // Handle declarations that start with specific keywords
      auto struct_attrs = parse_structural_attributes();
      auto function_attrs = parse_function_attributes();

      if (match({TokenType::Type})) {
        statements.push_back(type_declaration());
        continue;
      }
      if (match({TokenType::Record})) {
        statements.push_back(record_declaration(struct_attrs));
        continue;
      }
      if (match({TokenType::Enum})) {
        statements.push_back(enum_declaration(struct_attrs));
        continue;
      }
      if (struct_attrs.has_value()) {
        const Token& anchor = struct_attrs->anchor.value_or(peek());
        report_error(anchor, "Structural attributes may only decorate records or enums.");
      }
      if (match({TokenType::Fn})) {
        statements.push_back(function("function"));
        continue;
      }
      if (function_attrs.has_value()) {
        const Token& anchor = function_attrs->anchor.value_or(peek());
        report_error(anchor, "Function attributes may only decorate functions.");
      }
      if (match({TokenType::Var})) {
        statements.push_back(var_declaration());
        continue;
      }
      if (match({TokenType::Let})) {
        statements.push_back(let_declaration());
        continue;
      }

      // Handle Statements vs Final Expression
      if (check(TokenType::If) || check(TokenType::While) || check(TokenType::For) ||
          check(TokenType::Reflect) || check(TokenType::Loop) || check(TokenType::At) ||
          check(TokenType::Break) || check(TokenType::Continue) || check(TokenType::Return) ||
          check(TokenType::Assert) || check(TokenType::LBrace)) {
        // Special handling for 'if' and '{' which can be expressions
        if (check(TokenType::If)) {
          consume(TokenType::If, "Expect 'if'.");
          auto condition = expression();

          if (check(TokenType::LBrace)) {
            // Braced -> Treat as IfExpr (which is more general than IfStmt with block)
            auto then_branch = block_expression();
            std::unique_ptr<Expr> else_branch = nullptr;
            if (match({TokenType::Else})) {
              if (check(TokenType::If)) {
                else_branch = if_expression();
              } else {
                else_branch = block_expression();
              }
            }
            auto if_expr = std::make_unique<IfExpr>(std::move(condition), std::move(then_branch),
                                                    std::move(else_branch));

            if (match({TokenType::Semicolon})) {
              statements.push_back(std::make_unique<ExpressionStmt>(std::move(if_expr)));
            } else if (check(TokenType::RBrace)) {
              final_expr = std::move(if_expr);
              break;
            } else {
              statements.push_back(std::make_unique<ExpressionStmt>(std::move(if_expr)));
            }
          } else {
            // Unbraced -> Must be IfStmt
            auto then_stmt = statement();
            std::unique_ptr<Stmt> else_stmt = nullptr;
            if (match({TokenType::Else})) {
              else_stmt = statement();
            }
            statements.push_back(std::make_unique<IfStmt>(
                std::move(condition), std::move(then_stmt), std::move(else_stmt)));
          }
          continue;
        }

        // Block `{ ... }`
        if (check(TokenType::LBrace)) {
          // This could be BlockStmt or BlockExpr.
          // If we parse as BlockExpr, we can convert to Stmt if not final.
          auto blk = block_expression();
          if (!check(TokenType::RBrace)) {
            // Not the end, must be statement
            // But wait, block_expression consumed the block.
            // We can just wrap it in ExpressionStmt (which is valid for BlockExpr).
            // Does BlockExpr return void if final_expr is null? Yes.
            // So ExpressionStmt(BlockExpr) is fine.

            // If it IS the end, we can keep it as final_expr?
            if (check(TokenType::RBrace)) {
              final_expr = std::move(blk);
              break;
            }
            statements.push_back(std::make_unique<ExpressionStmt>(std::move(blk)));
            continue;
          }
          // It was the end.
          final_expr = std::move(blk);
          break;
        }

        statements.push_back(statement());
        continue;
      }

      // Default: Expression
      auto expr = expression();
      if (match({TokenType::Semicolon})) {
        statements.push_back(std::make_unique<ExpressionStmt>(std::move(expr)));
      } else if (check(TokenType::RBrace)) {
        final_expr = std::move(expr);
        break;
      } else {
        report_error(peek(), "Expect ';' after expression.");
        break;
      }

    } catch (const std::runtime_error& error) {
      synchronize();
    }
  }
  return {std::move(statements), std::move(final_expr)};
}

// Parses a block of statements.
// block -> "{" declaration* "}" ;
std::vector<std::unique_ptr<Stmt>> Parser::block() {
  // Note: The opening brace '{' is usually consumed by the caller (e.g. function, if-stmt).
  // But wait, declaration -> statement -> block.
  // statement() matches LBrace and calls block(). So statement() consumed LBrace.
  // function() consumes LBrace and calls block().
  // loop() consumes LBrace and calls block().
  // So block() should NOT consume LBrace.

  auto [stmts, final_expr] = parse_block_body();
  consume(TokenType::RBrace, "Expect '}' after block.");

  if (final_expr) {
    stmts.push_back(std::make_unique<ExpressionStmt>(std::move(final_expr)));
  }
  return std::move(stmts);
}

std::unique_ptr<Expr> Parser::block_expression() {
  consume(TokenType::LBrace, "Expect '{' before block.");
  auto [stmts, final_expr] = parse_block_body();
  consume(TokenType::RBrace, "Expect '}' after block.");
  return std::make_unique<BlockExpr>(std::move(stmts), std::move(final_expr));
}

std::unique_ptr<Expr> Parser::if_expression() {
  consume(TokenType::If, "Expect 'if'.");
  auto condition = expression();

  auto then_branch = block_expression();
  std::unique_ptr<Expr> else_branch = nullptr;

  if (match({TokenType::Else})) {
    if (check(TokenType::If)) {
      else_branch = if_expression();
    } else {
      else_branch = block_expression();
    }
  }

  return std::make_unique<IfExpr>(std::move(condition), std::move(then_branch),
                                  std::move(else_branch));
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
std::unique_ptr<Expr> Parser::expression() { return assignment(); }

// Parses an assignment expression.
// assignment -> logical_or ( "=" assignment )? ;
// Changed to call logical_or instead of bitwise_or.
std::unique_ptr<Expr> Parser::assignment() {
  std::unique_ptr<Expr> expr = logical_or();
  if (match({TokenType::Equal})) {
    Token equals = previous();
    std::unique_ptr<Expr> value = assignment();

    if (dynamic_cast<VariableExpr*>(expr.get()) || dynamic_cast<IndexExpr*>(expr.get()) ||
        dynamic_cast<FieldAccessExpr*>(expr.get())) {
      return std::make_unique<AssignExpr>(std::move(expr), std::move(value));
    }

    report_error(equals, "Invalid assignment target");
  }
  return expr;
}

// Parses a logical OR expression.
// logical_or -> logical_and ( "||" logical_and )* ;
std::unique_ptr<Expr> Parser::logical_or() {
  std::unique_ptr<Expr> expr = logical_and();
  while (match({TokenType::PipePipe})) {
    Token op = previous();
    std::unique_ptr<Expr> right = logical_and();
    expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
  }
  return expr;
}

// Parses a logical AND expression.
// logical_and -> arrow ( "&&" arrow )* ;
// arrow/range are lower precedence than bitwise so they sit here.
std::unique_ptr<Expr> Parser::logical_and() {
  std::unique_ptr<Expr> expr = arrow();
  while (match({TokenType::AmpAmp})) {
    Token op = previous();
    std::unique_ptr<Expr> right = arrow();
    expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
  }
  return expr;
}

// Parses a bitwise OR expression.
// bitwise_or -> bitwise_xor ( "|" bitwise_xor )* ;
std::unique_ptr<Expr> Parser::bitwise_or() {
  std::unique_ptr<Expr> expr = bitwise_xor();
  while (match({TokenType::Pipe})) {
    Token op = previous();
    std::unique_ptr<Expr> right = bitwise_xor();
    expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
  }
  return expr;
}

// Parses a bitwise XOR expression.
// bitwise_xor -> bitwise_and ( "^" bitwise_and )* ;
std::unique_ptr<Expr> Parser::bitwise_xor() {
  std::unique_ptr<Expr> expr = bitwise_and();
  while (match({TokenType::Caret})) {
    Token op = previous();
    std::unique_ptr<Expr> right = bitwise_and();
    expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
  }
  return expr;
}

// Parses a bitwise AND expression.
// bitwise_and -> comparison ( "&" comparison )* ;
// Bitwise & has higher precedence than equality.
std::unique_ptr<Expr> Parser::bitwise_and() {
  std::unique_ptr<Expr> expr = comparison();
  while (match({TokenType::Amp})) {
    Token op = previous();
    std::unique_ptr<Expr> right = comparison();
    expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
  }
  return expr;
}

// Parses an arrow expression (used in recurse blocks).
// arrow -> range ( "->" range )* ;
std::unique_ptr<Expr> Parser::arrow() {
  std::unique_ptr<Expr> expr = range();
  while (match({TokenType::Arrow})) {
    Token op = previous();
    std::unique_ptr<Expr> right = range();
    expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
  }
  return expr;
}

// Parses a range expression.
// range -> equality ( ( ".." | "..=" ) equality )? ;
std::unique_ptr<Expr> Parser::range() {
  std::unique_ptr<Expr> expr = equality();
  if (match({TokenType::DotDot, TokenType::DotDotEq})) {
    Token op = previous();
    std::unique_ptr<Expr> right = equality();
    return std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
  }
  return expr;
}

// Parses an equality expression.
// equality -> bitwise_or ( ( "!=" | "==" ) bitwise_or )* ;
// Bitwise ops bind tighter than equality (Python/C-style).
std::unique_ptr<Expr> Parser::equality() {
  std::unique_ptr<Expr> expr = bitwise_or();
  while (match({TokenType::BangEqual, TokenType::EqualEqual})) {
    Token op = previous();
    std::unique_ptr<Expr> right = bitwise_or();
    expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
  }
  return expr;
}

// Parses a comparison expression.
// comparison -> shift ( ( ">" | ">=" | "<" | "<=" ) shift )* ;
// Changed to call shift() instead of term()
std::unique_ptr<Expr> Parser::comparison() {
  std::unique_ptr<Expr> expr = shift();
  while (
      match({TokenType::Greater, TokenType::GreaterEqual, TokenType::Less, TokenType::LessEqual})) {
    Token op = previous();
    std::unique_ptr<Expr> right = shift();
    expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
  }
  return expr;
}

// Parses a shift expression.
// shift -> term ( ( "<<" | ">>" | ">>>" ) term )* ;
std::unique_ptr<Expr> Parser::shift() {
  std::unique_ptr<Expr> expr = term();
  while (
      match({TokenType::LessLess, TokenType::GreaterGreater, TokenType::GreaterGreaterGreater})) {
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
// factor -> exponent ( ( "/" | "*" | "%" ) exponent )* ;
std::unique_ptr<Expr> Parser::factor() {
  std::unique_ptr<Expr> expr = exponent();
  while (match({TokenType::Slash, TokenType::Star, TokenType::Percent})) {
    Token op = previous();
    std::unique_ptr<Expr> right = exponent();
    expr = std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
  }
  return expr;
}

// Parses an exponentiation/matmul expression.
// exponent -> unary ( "**" exponent )? ; (Right-associative)
std::unique_ptr<Expr> Parser::exponent() {
  std::unique_ptr<Expr> expr = unary();
  if (match({TokenType::StarStar})) {
    Token op = previous();
    std::unique_ptr<Expr> right = exponent();
    return std::make_unique<BinaryExpr>(std::move(expr), op, std::move(right));
  }
  return expr;
}

// Parses a unary expression.
// unary -> ( "!" | "-" | "~" ) unary | primary ( "as" type )? ;
// Added Tilde, 'as' postfix cast
std::unique_ptr<Expr> Parser::unary() {
  if (match({TokenType::Bang, TokenType::Minus, TokenType::Tilde})) {
    Token op = previous();
    std::unique_ptr<Expr> right = unary();
    return std::make_unique<UnaryExpr>(op, std::move(right));
  }
  auto expr = primary();
  // Postfix 'as' cast: consume 'as' + type, return expression unchanged
  // (semantic coercion is handled by the let binding's declared type annotation)
  while (check(TokenType::As)) {
    match({TokenType::As});
    type();  // parse and discard the target type
  }
  return expr;
}

static bool is_type_start_token(const Token& token) {
  return token.type == TokenType::Identifier || token.type == TokenType::I32 ||
         token.type == TokenType::I16 || token.type == TokenType::I8 ||
         token.type == TokenType::I2 || token.type == TokenType::Bool ||
         token.type == TokenType::Void || token.type == TokenType::T81BigInt ||
         token.type == TokenType::T81Float || token.type == TokenType::T81Fraction ||
         token.type == TokenType::T81Fixed || token.type == TokenType::T81Complex ||
         token.type == TokenType::T81Quaternion || token.type == TokenType::T81Prob ||
         token.type == TokenType::Cell || token.type == TokenType::T81Qutrit ||
         token.type == TokenType::T81Uint || token.type == TokenType::T81Vector ||
         token.type == TokenType::Matrix || token.type == TokenType::Tensor ||
         token.type == TokenType::Graph || token.type == TokenType::List ||
         token.type == TokenType::Map || token.type == TokenType::Set ||
         token.type == TokenType::Tree || token.type == TokenType::String;
}

// Parses a primary expression, which is the highest-precedence expression.
// primary -> "false" | "true" | INTEGER | FLOAT | STRING | "(" expression ")" | IDENTIFIER ;
std::unique_ptr<Expr> Parser::primary() {
  std::unique_ptr<Expr> expr;

  if (match({TokenType::Match})) {
    return match_expression();
  } else if (match({TokenType::False, TokenType::True, TokenType::Integer, TokenType::Float,
                    TokenType::Base81Integer, TokenType::Base81Float, TokenType::String,
                    TokenType::ByteString, TokenType::Ternary})) {
    Token lit_tok = previous();
    expr = std::make_unique<LiteralExpr>(lit_tok);
    // Consume optional type suffixes on integer/float literals.
    // e.g. 127i8, -1000i16, 1i32 (keyword tokens), 81u (identifier 'u'),
    //      1.25fx (identifier 'x' after float 'f'), 0.5p (identifier 'p').
    if (lit_tok.type == TokenType::Integer || lit_tok.type == TokenType::Base81Integer) {
      if (check(TokenType::I8) || check(TokenType::I16) || check(TokenType::I32)) {
        advance();  // discard; type is determined by the let-binding annotation
      } else if (check(TokenType::Identifier) && peek().lexeme == "u") {
        advance();  // discard 'u' suffix for T81Uint literals
      }
    }
    if (lit_tok.type == TokenType::Float || lit_tok.type == TokenType::Base81Float) {
      // 1.25fx: lexer produces Float("1.25f") + Identifier("x")
      // 0.5p:   lexer produces Float("0.5") + Identifier("p")
      if (check(TokenType::Identifier) &&
          (peek().lexeme == "x" || peek().lexeme == "p")) {
        advance();  // discard fixed-point or probability suffix
        // Check if this was a probability literal (p suffix)
        if (previous().lexeme == "p") {
          // Create T81Prob literal from the float value
          Token prob_token = Token{TokenType::T81Prob, lit_tok.lexeme, lit_tok.line, lit_tok.column};
          expr = std::make_unique<LiteralExpr>(prob_token);
        } else {
          // Regular float with suffix (e.g., 1.25fx)
          expr = std::make_unique<LiteralExpr>(lit_tok);
        }
      } else {
        expr = std::make_unique<LiteralExpr>(lit_tok);
      }
    }
  } else if (match({TokenType::LBracket})) {
    Token bracket = previous();
    std::vector<std::unique_ptr<Expr>> elements;
    std::unique_ptr<Expr> repeat_count = nullptr;
    if (!check(TokenType::RBracket)) {
      elements.push_back(expression());
      if (match({TokenType::Semicolon})) {
        repeat_count = expression();
      } else {
        while (match({TokenType::Comma})) {
          if (check(TokenType::RBracket)) break;
          elements.push_back(expression());
        }
      }
    }
    consume(TokenType::RBracket, "Expect ']' after vector literal.");
    expr =
        std::make_unique<VectorLiteralExpr>(bracket, std::move(elements), std::move(repeat_count));
  } else if (match({TokenType::LBrace})) {
    Token brace = previous();
    std::vector<std::unique_ptr<Expr>> elements;
    std::vector<std::pair<std::unique_ptr<Expr>, std::unique_ptr<Expr>>> entries;
    
    // Check if this is a map literal by looking for => pattern
    bool is_map_literal = false;
    if (!check(TokenType::RBrace)) {
      // Look ahead to see if we have Symbol => pattern
      // If the first token is Symbol, assume it's a map literal
      if (check(TokenType::Symbol)) {
        is_map_literal = true;
      }
    }
    
    if (is_map_literal) {
      // Parse map literal: {:key => value, :key2 => value2}
      do {
        auto key = primary();
        consume(TokenType::FatArrow, "Expect '=>' after map key.");
        auto value = expression();
        entries.emplace_back(std::move(key), std::move(value));
      } while (match({TokenType::Comma}) && !check(TokenType::RBrace));
    } else {
      // Parse set literal: {expr1, expr2, expr3}
      if (!check(TokenType::RBrace)) {
        elements.push_back(expression());
        while (match({TokenType::Comma})) {
          if (check(TokenType::RBrace)) break;
          elements.push_back(expression());
        }
      }
    }
    
    consume(TokenType::RBrace, "Expect '}' after literal.");
    if (is_map_literal) {
      expr = std::make_unique<MapLiteralExpr>(brace, std::move(entries));
    } else {
      expr = std::make_unique<SetLiteralExpr>(brace, std::move(elements));
    }
  } else if (match({TokenType::LParen})) {
    std::unique_ptr<Expr> inner = expression();
    consume(TokenType::RParen, "Expect ')' after expression.");
    expr = std::make_unique<GroupingExpr>(std::move(inner));
  } else if (check(TokenType::If)) {
    return if_expression();
  } else if (check(TokenType::LBrace)) {
    return block_expression();
  } else if (match({TokenType::Identifier})) {
    Token name = previous();
    Token enum_name_token;
    Token variant_token;
    if (try_parse_enum_literal(name, enum_name_token, variant_token)) {
      std::unique_ptr<Expr> payload = nullptr;
      if (match({TokenType::LParen})) {
        payload = expression();
        consume(TokenType::RParen, "Expect ')' after enum variant payload.");
      }
      return std::make_unique<EnumLiteralExpr>(enum_name_token, variant_token, std::move(payload));
    }

    if (check(TokenType::LBracket)) {
      // Ambiguity check: Generic Type instantiation vs Indexing.
      // If the token after '[' looks like a type, we assume GenericType.
      // Otherwise (e.g. literal '0', expression start), we assume Indexing.
      Token next = _lexer.peek_next_token();
      bool looks_like_type = false;
      if (is_type_start_token(next)) {
        if (next.type == TokenType::Identifier) {
          // Heuristic: Types are usually Capitalized. Variables are lowercase.
          if (!next.lexeme.empty() && std::isupper(next.lexeme[0])) {
            looks_like_type = true;
          }
        } else {
          // Keywords like i32, bool are definitely types.
          looks_like_type = true;
        }
      }

      if (looks_like_type) {
        expr = parse_generic_type(name);
      } else {
        expr = std::make_unique<VariableExpr>(name);
      }
    } else if (check(TokenType::LBrace) && !name.lexeme.empty() &&
               std::isupper(static_cast<unsigned char>(name.lexeme[0]))) {
      // Only treat `Name { ... }` as a record literal when the identifier starts with
      // an uppercase letter. Lowercase identifiers (e.g. a variable `v`) followed by `{`
      // must NOT be greedily consumed — `{` may be the opening brace of a match/if body.
      advance();  // consume '{'
      return record_literal(std::move(name));
    } else {
      expr = std::make_unique<VariableExpr>(name);
    }
  } else if (match({TokenType::Symbol})) {
    expr = std::make_unique<SymbolLiteralExpr>(previous());
  } else if (match({TokenType::InfiniteLiteral})) {
    Token token = previous();
    consume(TokenType::LBrace, "Expect '{' after '∞'.");
    auto seed = expression();
    consume(TokenType::RBrace, "Expect '}' after infinite literal seed.");
    expr = std::make_unique<InfiniteLiteralExpr>(token, std::move(seed));
  } else if (match({TokenType::Infer})) {
    Token keyword = previous();
    auto inner = expression();
    expr = std::make_unique<InferExpr>(keyword, std::move(inner));
  } else if (is_type_start_token(peek())) {
    expr = type();
  } else {
    report_error(peek(), "Expect expression.");
    throw std::runtime_error("Expect expression.");
  }

  // Handle postfix operators: Call, FieldAccess, Indexing
  while (true) {
    if (match({TokenType::Dot})) {
      if (!is_dot_field_segment_token(peek().type)) {
        report_error(peek(), "Expect field name after '.'.");
        throw std::runtime_error("Expect field name after '.'.");
      }
      Token field = advance();
      expr = std::make_unique<FieldAccessExpr>(std::move(expr), field);
    } else if (match({TokenType::LParen})) {
      std::vector<std::unique_ptr<Expr>> arguments;
      if (!check(TokenType::RParen)) {
        do {
          arguments.push_back(expression());
        } while (match({TokenType::Comma}));
      }
      Token paren = consume(TokenType::RParen, "Expect ')' after arguments.");
      expr = std::make_unique<CallExpr>(std::move(expr), paren, std::move(arguments));
    } else if (check(TokenType::LBracket)) {
      // Verify if this is actually an index expression and not something else.
      // Note: parse_generic_type consumed '[' already if it was chosen above.
      // If we are here, we have an expression 'expr' and see '[', so it must be indexing.
      Token bracket = consume(TokenType::LBracket, "Expect '['.");
      std::unique_ptr<Expr> index = expression();
      consume(TokenType::RBracket, "Expect ']' after index.");
      expr = std::make_unique<IndexExpr>(std::move(expr), std::move(index), bracket);
    } else {
      break;
    }
  }

  return expr;
}

std::unique_ptr<Expr> Parser::match_expression() {
  std::unique_ptr<Expr> scrutinee = expression();
  consume(TokenType::LBrace, "Expect '{' before match arms.");

  std::vector<MatchArm> arms;
  while (!check(TokenType::RBrace) && !is_at_end()) {
    try {
      arms.push_back(match_arm());
    } catch (const std::runtime_error&) {
      // Recover to the next arm boundary inside match bodies.
      while (!check(TokenType::Semicolon) && !check(TokenType::Comma) &&
             !check(TokenType::RBrace) && !is_at_end()) {
        advance();
      }
      match({TokenType::Semicolon, TokenType::Comma});
      continue;
    }
    if (match({TokenType::Semicolon, TokenType::Comma})) {
      continue;
    }
    if (check(TokenType::RBrace)) {
      break;
    }

    report_error(peek(), "Expect ',' or ';' between match arms.");
    // If we're already at the next arm, continue parsing without discarding it.
    if (check(TokenType::Identifier)) {
      continue;
    }

    while (!check(TokenType::Semicolon) && !check(TokenType::Comma) && !check(TokenType::RBrace) &&
           !is_at_end()) {
      advance();
    }
    match({TokenType::Semicolon, TokenType::Comma});
  }

  consume(TokenType::RBrace, "Expect '}' after match arms.");
  return std::make_unique<MatchExpr>(std::move(scrutinee), std::move(arms));
}

std::unique_ptr<Expr> Parser::record_literal(Token type_name) {
  std::vector<std::pair<Token, std::unique_ptr<Expr>>> fields;
  if (!check(TokenType::RBrace)) {
    do {
      if (check(TokenType::RBrace)) break;
      Token field_name = consume(TokenType::Identifier, "Expect field name in record literal.");
      consume(TokenType::Colon, "Expect ':' after field name.");
      auto value = expression();
      fields.emplace_back(field_name, std::move(value));
    } while (match({TokenType::Comma, TokenType::Semicolon}));
  }
  consume(TokenType::RBrace, "Expect '}' after record literal.");
  return std::make_unique<RecordLiteralExpr>(type_name, std::move(fields));
}

MatchPattern Parser::parse_match_pattern() {
  MatchPattern pattern;
  if (match({TokenType::LBrace})) {
    pattern.kind = MatchPattern::Kind::Record;
    while (!check(TokenType::RBrace) && !is_at_end()) {
      if (!check(TokenType::Identifier)) {
        report_error(peek(), "Expect field name in record pattern.");
        while (!check(TokenType::Comma) && !check(TokenType::Semicolon) &&
               !check(TokenType::RBrace) && !is_at_end()) {
          advance();
        }
        match({TokenType::Comma, TokenType::Semicolon});
        continue;
      }

      Token field_name = advance();
      Token binding = field_name;
      if (match({TokenType::Colon})) {
        if (check(TokenType::Identifier)) {
          binding = advance();
        } else {
          report_error(peek(), "Expect binding name after ':' in record pattern.");
          while (!check(TokenType::Comma) && !check(TokenType::Semicolon) &&
                 !check(TokenType::RBrace) && !is_at_end()) {
            advance();
          }
        }
      }
      pattern.record_bindings.emplace_back(field_name, binding);

      if (match({TokenType::Comma, TokenType::Semicolon})) {
        continue;
      }
      if (check(TokenType::RBrace)) {
        break;
      }

      report_error(peek(), "Expect ',' or ';' between record pattern fields.");
      while (!check(TokenType::Comma) && !check(TokenType::Semicolon) &&
             !check(TokenType::RBrace) && !is_at_end()) {
        advance();
      }
      match({TokenType::Comma, TokenType::Semicolon});
    }
    consume(TokenType::RBrace, "Expect '}' after record pattern.");
    return pattern;
  }

  if (match({TokenType::Identifier})) {
    Token first = previous();
    if (match({TokenType::LParen})) {
      MatchPattern nested;
      if (!check(TokenType::RParen)) {
        nested = parse_match_pattern();
      }
      consume(TokenType::RParen, "Expect ')' after nested match binding.");
      pattern.kind = MatchPattern::Kind::Variant;
      pattern.variant_name = first;
      if (nested.kind != MatchPattern::Kind::None || !nested.tuple_bindings.empty() ||
          !nested.record_bindings.empty() || nested.binding_is_wildcard ||
          nested.variant_name.type != TokenType::Illegal) {
        pattern.variant_payload = std::make_unique<MatchPattern>(std::move(nested));
      }
      return pattern;
    }
    if (match({TokenType::Comma})) {
      pattern.kind = MatchPattern::Kind::Tuple;
      pattern.tuple_bindings.push_back(first);
      while (true) {
        if (!check(TokenType::Identifier)) {
          report_error(peek(), "Expect binding identifier in tuple pattern.");
          while (!check(TokenType::Comma) && !check(TokenType::RParen) && !is_at_end()) {
            advance();
          }
          if (!match({TokenType::Comma})) {
            break;
          }
          continue;
        }

        pattern.tuple_bindings.push_back(advance());
        if (!match({TokenType::Comma})) {
          break;
        }
      }
      return pattern;
    }
    pattern.kind = MatchPattern::Kind::Identifier;
    pattern.identifier = first;
    pattern.binding_is_wildcard = std::string_view(first.lexeme) == "_";
    return pattern;
  }

  report_error(peek(), "Expect pattern binding.");
  throw std::runtime_error("Expect pattern binding.");
  return pattern;
}

MatchArm Parser::match_arm() {
  Token keyword = consume(TokenType::Identifier, "Expect match arm variant.");
  MatchPattern pattern;

  if (match({TokenType::LParen})) {
    if (!check(TokenType::RParen)) {
      pattern = parse_match_pattern();
    }
    if (check(TokenType::RParen)) {
      advance();
    } else {
      report_error(peek(), "Expect ')' after match binding.");
      if (!check(TokenType::FatArrow) && !check(TokenType::If)) {
        throw std::runtime_error("Expect ')' after match binding.");
      }
    }
  }

  std::unique_ptr<Expr> guard = nullptr;
  if (match({TokenType::If})) {
    if (check(TokenType::FatArrow)) {
      report_error(peek(), "Expect guard expression after 'if' in match arm.");
    } else {
      guard = expression();
    }
  }

  consume(TokenType::FatArrow, "Expect '=>' after match arm pattern.");
  std::unique_ptr<Expr> body = expression();
  return MatchArm(keyword, std::move(pattern), std::move(guard), std::move(body));
}

bool Parser::parse_loop_annotation(LoopStmt::BoundKind& bound_kind,
                                   std::optional<std::int64_t>& bound_value, Token& attr_token,
                                   std::unique_ptr<Expr>& guard_expr) {
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
  guard_expr.reset();
  Token arg;
  if (match({TokenType::Identifier, TokenType::Loop, TokenType::Infinite})) {
    arg = previous();
    std::string_view lexeme{arg.lexeme};
    if (lexeme == "infinite") {
      bound_kind = LoopStmt::BoundKind::Infinite;
    } else if (lexeme == "loop") {
      bound_kind = LoopStmt::BoundKind::Guarded;
      consume(TokenType::LParen, "Expect '(' after 'loop'.");
      guard_expr = expression();
      consume(TokenType::RParen, "Expect ')' after guard expression.");
    } else {
      report_error(arg, "'@bounded' only accepts 'infinite', an integer, or 'loop(...)'");
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

std::optional<FunctionAttributes> Parser::parse_function_attributes() {
  FunctionAttributes attrs;
  bool seen = false;
  while (check(TokenType::At)) {
    Token lookahead = _lexer.peek_next_token();
    if (lookahead.type != TokenType::Identifier) {
      break;
    }
    std::string attr_candidate{lookahead.lexeme};
    if (attr_candidate != "tier" && attr_candidate != "pure" && attr_candidate != "axion_verify" &&
        attr_candidate != "attention" && attr_candidate != "qmatmul") {
      break;
    }
    match({TokenType::At});
    Token name = consume(TokenType::Identifier, "Expect attribute name after '@'.");
    if (!seen) {
      attrs.anchor = name;
    }
    seen = true;
    if (attr_candidate == "pure") {
      if (attrs.is_pure) {
        report_error(name, "Duplicate '@pure' attribute.");
      }
      attrs.is_pure = true;
    } else if (attr_candidate == "axion_verify") {
      if (attrs.is_axion_verify) {
        report_error(name, "Duplicate '@axion_verify' attribute.");
      }
      attrs.is_axion_verify = true;
    } else if (attr_candidate == "attention") {
      if (attrs.is_attention) {
        report_error(name, "Duplicate '@attention' attribute.");
      }
      attrs.is_attention = true;
    } else if (attr_candidate == "qmatmul") {
      if (attrs.is_qmatmul) {
        report_error(name, "Duplicate '@qmatmul' attribute.");
      }
      attrs.is_qmatmul = true;
    } else {
      // attr_candidate == "tier"
      consume(TokenType::LParen, "Expect '(' after attribute name.");
      if (attrs.tier.has_value()) {
        report_error(name, "Duplicate '@tier' attribute.");
      }
      Token value = consume(TokenType::Integer, "Expect integer tier value.");
      try {
        std::int64_t tier = std::stoll(std::string(value.lexeme));
        if (tier < 1 || tier > 5) {
          report_error(value, "Tier value must be in [1, 5].");
        } else {
          attrs.tier = tier;
        }
      } catch (const std::exception&) {
        report_error(value, "Invalid integer for tier value.");
      }
      consume(TokenType::RParen, "Expect ')' after attribute.");
    }
  }
  if (!seen) {
    return std::nullopt;
  }
  return attrs;
}

bool Parser::try_parse_enum_literal(const Token& token, Token& enum_name,
                                    Token& variant_name) const {
  std::string_view lexeme = token.lexeme;
  auto dot_pos = lexeme.find('.');
  if (dot_pos == std::string_view::npos || dot_pos == 0 || dot_pos + 1 >= lexeme.size()) {
    return false;
  }
  if (lexeme.find('.', dot_pos + 1) != std::string_view::npos) {
    return false;
  }
  std::string_view enum_part(lexeme.data(), dot_pos);
  std::string_view variant_part(lexeme.data() + dot_pos + 1, lexeme.size() - dot_pos - 1);
  if (enum_part.empty() || variant_part.empty()) {
    return false;
  }
  auto is_upper = [](std::string_view view) {
    unsigned char c = static_cast<unsigned char>(view.front());
    return std::isupper(c);
  };
  if (!is_upper(enum_part) || !is_upper(variant_part)) {
    return false;
  }
  enum_name = token;
  enum_name.lexeme = enum_part;
  variant_name = token;
  variant_name.lexeme = variant_part;
  variant_name.column = token.column + static_cast<int>(dot_pos + 1);
  return true;
}

std::unique_ptr<GenericTypeExpr> Parser::parse_generic_type(Token name) {
  consume(TokenType::LBracket, "Expect '[' after generic type name.");
  std::array<std::unique_ptr<Expr>, 8> parameters;
  size_t param_count = 0;
  std::string_view type_name{name.lexeme};

  // First parameter can be a type or an expression (for types like T81Fixed).
  if (is_type_start()) {
    parameters[param_count++] = type();
  } else {
    parameters[param_count++] = expression();
  }

  // Subsequent parameters are constant value expressions (structural-result types are treated
  // specially).
  while (match({TokenType::Comma})) {
    if (check(TokenType::RBracket)) {
      report_error(peek(), "Trailing comma in generic parameter list is not allowed.");
      break;
    }

    if (param_count >= 8) {
      report_error(peek(), "Too many generic parameters (max 8)");
      while (!check(TokenType::RBracket) && !is_at_end()) {
        advance();
      }
      break;
    }
    if (type_name == "Result" && param_count == 1) {
      parameters[param_count++] = type();
      continue;
    }
    if (is_type_start()) {
      parameters[param_count++] = type();
    } else {
      parameters[param_count++] = expression();
    }
  }

  consume(TokenType::RBracket, "Expect ']' after type parameters.");
  return std::make_unique<GenericTypeExpr>(name, std::move(parameters), param_count);
}

bool Parser::is_type_start() {
  return check(TokenType::Identifier) || check(TokenType::I32) || check(TokenType::I16) ||
         check(TokenType::I8) || check(TokenType::I2) || check(TokenType::Bool) ||
         check(TokenType::Void) || check(TokenType::T81BigInt) || check(TokenType::T81Float) ||
         check(TokenType::T81Fraction) || check(TokenType::T81Fixed) ||
         check(TokenType::T81Complex) || check(TokenType::T81Quaternion) ||
         check(TokenType::T81Prob) || check(TokenType::Cell) || check(TokenType::T81Qutrit) ||
         check(TokenType::T81Uint) || check(TokenType::T81Vector) || check(TokenType::Matrix) ||
         check(TokenType::Tensor) || check(TokenType::Graph) || check(TokenType::List) ||
         check(TokenType::Map) || check(TokenType::Set) || check(TokenType::Tree) ||
         check(TokenType::String);
}

// Parses a type expression.
// type -> (IDENTIFIER | primitive_type_keyword) ( "[" type ( "," expression )* "]" )? ;
std::unique_ptr<TypeExpr> Parser::type() {
  if (!is_type_start()) {
    report_error(peek(), "Expect type name");
    return nullptr;
  }
  Token name = advance();

  // Explicitly reject legacy angle bracket syntax
  if (peek().type == TokenType::Less) {
    report_error(peek(),
                 "Legacy '<...>' syntax for generics is not supported. Use '[...]' instead.");
    return nullptr;
  }

  if (check(TokenType::LBracket)) {
    return parse_generic_type(name);
  }

  return std::make_unique<SimpleTypeExpr>(name);
}

}  // namespace frontend
}  // namespace t81
