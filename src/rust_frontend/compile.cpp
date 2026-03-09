#include "t81/rust_frontend/compile.hpp"

#include <cctype>
#include <fstream>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include "t81/frontend_adapter/c_bridge.hpp"

namespace t81::rust_frontend {

namespace {

enum class TokenKind {
  Eof,
  Identifier,
  Integer,
  KwFn,
  KwLet,
  KwMut,
  KwIf,
  KwElse,
  KwReturn,
  KwI32,
  KwTrue,
  KwFalse,
  KwWhile,
  KwFor,
  KwLoop,
  KwMatch,
  KwUnsafe,
  LParen,
  RParen,
  LBrace,
  RBrace,
  LBracket,
  RBracket,
  Comma,
  Semicolon,
  Colon,
  Arrow,
  Assign,
  Plus,
  Minus,
  Star,
  Slash,
  Percent,
  Bang,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
  EqualEqual,
  BangEqual,
  Amp,
  AmpAmp,
  Pipe,
  PipePipe,
  Caret,
  ShiftLeft,
  ShiftRight,
};

struct Token {
  TokenKind kind{TokenKind::Eof};
  std::string text;
  unsigned line{1};
  unsigned column{1};
};

bool fail(std::string* error_message, const std::string& message) {
  if (error_message) {
    *error_message = message;
  }
  return false;
}

std::string trim_copy(std::string_view text) {
  size_t start = 0;
  while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start]))) {
    ++start;
  }
  size_t end = text.size();
  while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1]))) {
    --end;
  }
  return std::string(text.substr(start, end - start));
}

std::string indent(int depth) {
  return std::string(static_cast<size_t>(depth) * 2, ' ');
}

class Lexer {
 public:
  explicit Lexer(std::string_view source) : source_(source) {}

  bool tokenize(std::vector<Token>& tokens, const std::string& diag_name, std::string* error) {
    while (true) {
      skip_whitespace_and_comments();
      if (pos_ >= source_.size()) {
        tokens.push_back({TokenKind::Eof, "", line_, column_});
        return true;
      }

      const unsigned token_line = line_;
      const unsigned token_column = column_;
      const char ch = source_[pos_];

      if (std::isalpha(static_cast<unsigned char>(ch)) || ch == '_') {
        std::string ident;
        while (pos_ < source_.size()) {
          const char c = source_[pos_];
          if (!std::isalnum(static_cast<unsigned char>(c)) && c != '_') {
            break;
          }
          ident.push_back(c);
          advance();
        }
        tokens.push_back({keyword_kind(ident), ident, token_line, token_column});
        continue;
      }

      if (std::isdigit(static_cast<unsigned char>(ch))) {
        std::string value;
        while (pos_ < source_.size() &&
               std::isdigit(static_cast<unsigned char>(source_[pos_]))) {
          value.push_back(source_[pos_]);
          advance();
        }
        tokens.push_back({TokenKind::Integer, value, token_line, token_column});
        continue;
      }

      auto push_single = [&](TokenKind kind) {
        tokens.push_back({kind, std::string(1, ch), token_line, token_column});
        advance();
      };

      switch (ch) {
        case '(':
          push_single(TokenKind::LParen);
          break;
        case ')':
          push_single(TokenKind::RParen);
          break;
        case '{':
          push_single(TokenKind::LBrace);
          break;
        case '}':
          push_single(TokenKind::RBrace);
          break;
        case '[':
          push_single(TokenKind::LBracket);
          break;
        case ']':
          push_single(TokenKind::RBracket);
          break;
        case ',':
          push_single(TokenKind::Comma);
          break;
        case ';':
          push_single(TokenKind::Semicolon);
          break;
        case ':':
          push_single(TokenKind::Colon);
          break;
        case '+':
          push_single(TokenKind::Plus);
          break;
        case '*':
          push_single(TokenKind::Star);
          break;
        case '%':
          push_single(TokenKind::Percent);
          break;
        case '^':
          push_single(TokenKind::Caret);
          break;
        case '-':
          if (match('>')) {
            tokens.push_back({TokenKind::Arrow, "->", token_line, token_column});
          } else {
            push_single(TokenKind::Minus);
          }
          break;
        case '!':
          if (match('=')) {
            tokens.push_back({TokenKind::BangEqual, "!=", token_line, token_column});
          } else {
            push_single(TokenKind::Bang);
          }
          break;
        case '=':
          if (match('=')) {
            tokens.push_back({TokenKind::EqualEqual, "==", token_line, token_column});
          } else {
            push_single(TokenKind::Assign);
          }
          break;
        case '<':
          if (match('=')) {
            tokens.push_back({TokenKind::LessEqual, "<=", token_line, token_column});
          } else if (match('<')) {
            tokens.push_back({TokenKind::ShiftLeft, "<<", token_line, token_column});
          } else {
            push_single(TokenKind::Less);
          }
          break;
        case '>':
          if (match('=')) {
            tokens.push_back({TokenKind::GreaterEqual, ">=", token_line, token_column});
          } else if (match('>')) {
            tokens.push_back({TokenKind::ShiftRight, ">>", token_line, token_column});
          } else {
            push_single(TokenKind::Greater);
          }
          break;
        case '&':
          if (match('&')) {
            tokens.push_back({TokenKind::AmpAmp, "&&", token_line, token_column});
          } else {
            push_single(TokenKind::Amp);
          }
          break;
        case '|':
          if (match('|')) {
            tokens.push_back({TokenKind::PipePipe, "||", token_line, token_column});
          } else {
            push_single(TokenKind::Pipe);
          }
          break;
        case '/':
          push_single(TokenKind::Slash);
          break;
        default:
          return fail(error, format_diag(diag_name, token_line, token_column,
                                         "unsupported Rust token in subset v0"));
      }
    }
  }

 private:
  void advance() {
    if (pos_ < source_.size() && source_[pos_] == '\n') {
      ++line_;
      column_ = 1;
    } else {
      ++column_;
    }
    ++pos_;
  }

  bool match(char expected) {
    if (pos_ + 1 >= source_.size() || source_[pos_ + 1] != expected) {
      return false;
    }
    advance();
    advance();
    return true;
  }

  void skip_whitespace_and_comments() {
    while (pos_ < source_.size()) {
      const char ch = source_[pos_];
      if (std::isspace(static_cast<unsigned char>(ch))) {
        advance();
        continue;
      }
      if (ch == '/' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '/') {
        while (pos_ < source_.size() && source_[pos_] != '\n') {
          advance();
        }
        continue;
      }
      if (ch == '/' && pos_ + 1 < source_.size() && source_[pos_ + 1] == '*') {
        advance();
        advance();
        while (pos_ + 1 < source_.size()) {
          if (source_[pos_] == '*' && source_[pos_ + 1] == '/') {
            advance();
            advance();
            break;
          }
          advance();
        }
        continue;
      }
      break;
    }
  }

  static TokenKind keyword_kind(const std::string& ident) {
    if (ident == "fn") return TokenKind::KwFn;
    if (ident == "let") return TokenKind::KwLet;
    if (ident == "mut") return TokenKind::KwMut;
    if (ident == "if") return TokenKind::KwIf;
    if (ident == "else") return TokenKind::KwElse;
    if (ident == "return") return TokenKind::KwReturn;
    if (ident == "i32") return TokenKind::KwI32;
    if (ident == "true") return TokenKind::KwTrue;
    if (ident == "false") return TokenKind::KwFalse;
    if (ident == "while") return TokenKind::KwWhile;
    if (ident == "for") return TokenKind::KwFor;
    if (ident == "loop") return TokenKind::KwLoop;
    if (ident == "match") return TokenKind::KwMatch;
    if (ident == "unsafe") return TokenKind::KwUnsafe;
    return TokenKind::Identifier;
  }

  static std::string format_diag(const std::string& diag_name,
                                 unsigned line,
                                 unsigned column,
                                 std::string_view message) {
    std::ostringstream oss;
    oss << diag_name << ':' << line << ':' << column << ": " << message;
    return oss.str();
  }

  std::string_view source_;
  size_t pos_{0};
  unsigned line_{1};
  unsigned column_{1};
};

class Parser {
 public:
  Parser(const std::vector<Token>& tokens, const std::string& diag_name)
      : tokens_(tokens), diag_name_(diag_name) {}

  bool parse_program(std::string& c_source, std::string* error) {
    std::vector<std::string> functions;
    while (!at(TokenKind::Eof)) {
      std::string function_text;
      if (!parse_function(function_text, error)) {
        return false;
      }
      functions.push_back(std::move(function_text));
    }
    std::ostringstream out;
    for (const auto& function : functions) {
      out << function << "\n";
    }
    c_source = out.str();
    return true;
  }

 private:
  struct Expr {
    std::string rendered;
    bool is_const_int{false};
  };

  bool parse_function(std::string& out, std::string* error) {
    if (!consume(TokenKind::KwFn)) {
      return fail_here(error, "expected 'fn' at top level");
    }
    Token name;
    if (!expect(TokenKind::Identifier, name, "expected function name", error)) {
      return false;
    }
    if (!seen_functions_.emplace(name.text).second) {
      return fail_at(name, "duplicate Rust function '" + name.text + "'", error);
    }
    if (!expect(TokenKind::LParen, "expected '(' after function name", error)) {
      return false;
    }
    std::vector<std::string> params;
    if (!at(TokenKind::RParen)) {
      while (true) {
        Token param_name;
        if (!expect(TokenKind::Identifier, param_name, "expected parameter name", error)) {
          return false;
        }
        if (!expect(TokenKind::Colon, "expected ':' after parameter name", error) ||
            !expect(TokenKind::KwI32, "only 'i32' parameters are supported in Rust subset v0",
                    error)) {
          return false;
        }
        params.push_back("int " + param_name.text);
        if (!consume(TokenKind::Comma)) {
          break;
        }
      }
    }
    if (!expect(TokenKind::RParen, "expected ')' after parameter list", error) ||
        !expect(TokenKind::Arrow, "expected '-> i32' return type", error) ||
        !expect(TokenKind::KwI32, "only 'i32' return types are supported in Rust subset v0",
                error)) {
      return false;
    }
    if (name.text == "main" && !params.empty()) {
      return fail_at(name, "'main' must have the exact signature 'fn main() -> i32' in Rust subset v0",
                     error);
    }

    std::string body;
    if (!parse_block(body, 1, error)) {
      return false;
    }

    std::ostringstream function;
    function << "int " << name.text << '(';
    for (size_t i = 0; i < params.size(); ++i) {
      if (i) {
        function << ", ";
      }
      function << params[i];
    }
    function << ") {\n" << body << "}";
    out = function.str();
    return true;
  }

  bool parse_block(std::string& out, int depth, std::string* error) {
    if (!expect(TokenKind::LBrace, "expected '{' to start block", error)) {
      return false;
    }
    std::ostringstream body;
    while (!at(TokenKind::RBrace)) {
      if (at(TokenKind::Eof)) {
        return fail_here(error, "unterminated block in Rust subset v0");
      }
      std::string stmt;
      if (!parse_statement(stmt, depth, error)) {
        return false;
      }
      body << stmt;
    }
    consume(TokenKind::RBrace);
    out = body.str();
    return true;
  }

  bool parse_statement(std::string& out, int depth, std::string* error) {
    if (at(TokenKind::KwLet)) {
      return parse_let_stmt(out, depth, error);
    }
    if (at(TokenKind::KwReturn)) {
      return parse_return_stmt(out, depth, error);
    }
    if (at(TokenKind::KwIf)) {
      return parse_if_stmt(out, depth, error);
    }
    if (at(TokenKind::LBrace)) {
      std::string nested;
      if (!parse_block(nested, depth + 1, error)) {
        return false;
      }
      out = indent(depth) + "{\n" + nested + indent(depth) + "}\n";
      return true;
    }
    if (at(TokenKind::KwWhile)) {
      return parse_while_stmt(out, depth, error);
    }
    if (at(TokenKind::KwFor)) {
      return fail_here(error, "'for' is not supported in Rust subset v0");
    }
    if (at(TokenKind::KwLoop)) {
      return fail_here(error, "'loop' is not supported in Rust subset v0");
    }
    if (at(TokenKind::KwMatch)) {
      return fail_here(error, "'match' is not supported in Rust subset v0");
    }
    if (at(TokenKind::KwUnsafe)) {
      return fail_here(error, "'unsafe' is not supported in Rust subset v0");
    }
    if (at(TokenKind::LBracket)) {
      return fail_here(error, "arrays are not supported in Rust subset v0");
    }
    if (at(TokenKind::Identifier) &&
        (peek().kind == TokenKind::Assign || peek().kind == TokenKind::LBracket)) {
      Expr target;
      if (!parse_assign_target(target, error)) {
        return false;
      }
      if (!expect(TokenKind::Assign, "expected '=' in assignment", error)) {
        return false;
      }
      Token name = current();
      Expr value;
      if (!parse_expr(value, 1, error)) {
        return false;
      }
      if (!expect(TokenKind::Semicolon, "expected ';' after assignment", error)) {
        return false;
      }
      out = indent(depth) + target.rendered + " = " + value.rendered + ";\n";
      return true;
    }

    Expr expr;
    if (!parse_expr(expr, 1, error)) {
      return false;
    }
    if (!expect(TokenKind::Semicolon, "expected ';' after expression statement", error)) {
      return false;
    }
    out = indent(depth) + expr.rendered + ";\n";
    return true;
  }

  bool parse_let_stmt(std::string& out, int depth, std::string* error) {
    consume(TokenKind::KwLet);
    consume(TokenKind::KwMut);
    Token name;
    if (!expect(TokenKind::Identifier, name, "expected local variable name after 'let'", error)) {
      return false;
    }
    bool is_array = false;
    std::string array_size;
    if (consume(TokenKind::Colon)) {
      if (consume(TokenKind::KwI32)) {
        is_array = false;
      } else if (consume(TokenKind::LBracket)) {
        is_array = true;
        if (!expect(TokenKind::KwI32,
                    "only '[i32; N]' fixed local arrays are supported in Rust subset v0", error) ||
            !expect(TokenKind::Semicolon,
                    "expected ';' inside fixed local array type '[i32; N]'", error)) {
          return false;
        }
        Token size_token;
        if (!expect(TokenKind::Integer, size_token,
                    "fixed local arrays must use an integer constant size in Rust subset v0",
                    error) ||
            !expect(TokenKind::RBracket, "expected ']' after fixed array type", error)) {
          return false;
        }
        array_size = size_token.text;
      } else {
        return fail_here(error, "only 'i32' and fixed local '[i32; N]' bindings are supported in Rust subset v0");
      }
    }
    if (!expect(TokenKind::Assign, "local bindings must have an initializer in Rust subset v0",
                error)) {
      return false;
    }
    if (is_array) {
      std::string initializer;
      if (!parse_array_initializer(initializer, error) ||
          !expect(TokenKind::Semicolon, "expected ';' after local binding", error)) {
        return false;
      }
      out = indent(depth) + "int " + name.text + "[" + array_size + "] = " + initializer + ";\n";
      return true;
    }
    Expr value;
    if (!parse_expr(value, 1, error) ||
        !expect(TokenKind::Semicolon, "expected ';' after local binding", error)) {
      return false;
    }
    out = indent(depth) + "int " + name.text + " = " + value.rendered + ";\n";
    return true;
  }

  bool parse_return_stmt(std::string& out, int depth, std::string* error) {
    consume(TokenKind::KwReturn);
    Expr value;
    if (!parse_expr(value, 1, error) ||
        !expect(TokenKind::Semicolon, "expected ';' after return value", error)) {
      return false;
    }
    out = indent(depth) + "return " + value.rendered + ";\n";
    return true;
  }

  bool parse_if_stmt(std::string& out, int depth, std::string* error) {
    consume(TokenKind::KwIf);
    Expr condition;
    if (!parse_expr(condition, 1, error)) {
      return false;
    }
    std::string then_block;
    if (!parse_block(then_block, depth + 1, error)) {
      return false;
    }
    std::ostringstream rendered;
    rendered << indent(depth) << "if (" << condition.rendered << ") {\n"
             << then_block << indent(depth) << "}";
    if (consume(TokenKind::KwElse)) {
      if (at(TokenKind::KwIf)) {
        std::string nested_if;
        if (!parse_if_stmt(nested_if, depth, error)) {
          return false;
        }
        rendered << " else " << trim_copy(nested_if);
      } else {
        std::string else_block;
        if (!parse_block(else_block, depth + 1, error)) {
          return false;
        }
        rendered << " else {\n" << else_block << indent(depth) << "}";
      }
    }
    rendered << '\n';
    out = rendered.str();
    return true;
  }

  bool parse_while_stmt(std::string& out, int depth, std::string* error) {
    consume(TokenKind::KwWhile);
    Expr condition;
    if (!parse_expr(condition, 1, error)) {
      return false;
    }
    std::string body;
    if (!parse_block(body, depth + 1, error)) {
      return false;
    }
    std::ostringstream rendered;
    rendered << indent(depth) << "while (" << condition.rendered << ") {\n"
             << body << indent(depth) << "}\n";
    out = rendered.str();
    return true;
  }

  bool parse_assign_target(Expr& out, std::string* error) {
    Token name;
    if (!expect(TokenKind::Identifier, name, "expected assignment target", error)) {
      return false;
    }
    out.rendered = name.text;
    out.is_const_int = false;
    if (consume(TokenKind::LBracket)) {
      Expr index;
      if (!parse_expr(index, 1, error)) {
        return false;
      }
      if (!index.is_const_int) {
        return fail_here(error, "only compile-time constant Rust array indices are supported in subset v0");
      }
      if (!expect(TokenKind::RBracket, "expected ']' after array index", error)) {
        return false;
      }
      out.rendered += "[" + index.rendered + "]";
    }
    return true;
  }

  bool parse_array_initializer(std::string& out, std::string* error) {
    if (!expect(TokenKind::LBracket, "expected '[' to start fixed array initializer", error)) {
      return false;
    }
    std::vector<std::string> values;
    if (!at(TokenKind::RBracket)) {
      while (true) {
        Expr value;
        if (!parse_expr(value, 1, error)) {
          return false;
        }
        if (!value.is_const_int) {
          return fail_here(error, "fixed Rust array initializers must use compile-time constant integer expressions");
        }
        values.push_back(value.rendered);
        if (!consume(TokenKind::Comma)) {
          break;
        }
      }
    }
    if (!expect(TokenKind::RBracket, "expected ']' after fixed array initializer", error)) {
      return false;
    }
    std::ostringstream rendered;
    rendered << "{";
    for (size_t i = 0; i < values.size(); ++i) {
      if (i) {
        rendered << ", ";
      }
      rendered << values[i];
    }
    rendered << "}";
    out = rendered.str();
    return true;
  }

  bool parse_expr(Expr& out, int min_prec, std::string* error) {
    Expr lhs;
    if (!parse_unary(lhs, error)) {
      return false;
    }
    while (true) {
      const TokenKind kind = current().kind;
      const int prec = precedence(kind);
      if (prec < min_prec) {
        break;
      }
      const std::string op = current().text;
      ++index_;
      Expr rhs;
      if (!parse_expr(rhs, prec + 1, error)) {
        return false;
      }
      lhs.rendered = "(" + lhs.rendered + " " + op + " " + rhs.rendered + ")";
      lhs.is_const_int = lhs.is_const_int && rhs.is_const_int;
    }
    out = std::move(lhs);
    return true;
  }

  bool parse_unary(Expr& out, std::string* error) {
    if (consume(TokenKind::Minus)) {
      Expr operand;
      if (!parse_unary(operand, error)) {
        return false;
      }
      out.rendered = "(-" + operand.rendered + ")";
      out.is_const_int = operand.is_const_int;
      return true;
    }
    if (consume(TokenKind::Bang)) {
      Expr operand;
      if (!parse_unary(operand, error)) {
        return false;
      }
      out.rendered = "(!" + operand.rendered + ")";
      out.is_const_int = operand.is_const_int;
      return true;
    }
    if (at(TokenKind::Amp)) {
      return fail_here(error, "references are not supported in Rust subset v0");
    }
    if (at(TokenKind::Star)) {
      return fail_here(error, "pointer dereference is not supported in Rust subset v0");
    }
    return parse_postfix(out, error);
  }

  bool parse_postfix(Expr& out, std::string* error) {
    Token name;
    if (expect(TokenKind::Identifier, name, "", nullptr)) {
      out.rendered = name.text;
      out.is_const_int = false;
      while (true) {
        if (consume(TokenKind::LParen)) {
          std::vector<std::string> args;
          if (!at(TokenKind::RParen)) {
            while (true) {
              Expr arg;
              if (!parse_expr(arg, 1, error)) {
                return false;
              }
              args.push_back(arg.rendered);
              if (!consume(TokenKind::Comma)) {
                break;
              }
            }
          }
          if (!expect(TokenKind::RParen, "expected ')' after function call arguments", error)) {
            return false;
          }
          std::ostringstream call;
          call << out.rendered << '(';
          for (size_t i = 0; i < args.size(); ++i) {
            if (i) {
              call << ", ";
            }
            call << args[i];
          }
          call << ')';
          out.rendered = call.str();
          out.is_const_int = false;
          continue;
        }
        if (consume(TokenKind::LBracket)) {
          Expr index;
          if (!parse_expr(index, 1, error)) {
            return false;
          }
          if (!index.is_const_int) {
            return fail_here(error, "only compile-time constant Rust array indices are supported in subset v0");
          }
          if (!expect(TokenKind::RBracket, "expected ']' after array index", error)) {
            return false;
          }
          out.rendered += "[" + index.rendered + "]";
          out.is_const_int = false;
          continue;
        }
        break;
      }
      return true;
    }
    return parse_primary(out, error);
  }

  bool parse_primary(Expr& out, std::string* error) {
    if (at(TokenKind::Integer)) {
      out.rendered = current().text;
      out.is_const_int = true;
      ++index_;
      return true;
    }
    if (consume(TokenKind::KwTrue)) {
      out.rendered = "1";
      out.is_const_int = true;
      return true;
    }
    if (consume(TokenKind::KwFalse)) {
      out.rendered = "0";
      out.is_const_int = true;
      return true;
    }
    if (consume(TokenKind::LParen)) {
      Expr nested;
      if (!parse_expr(nested, 1, error) ||
          !expect(TokenKind::RParen, "expected ')' after expression", error)) {
        return false;
      }
      out.rendered = "(" + nested.rendered + ")";
      out.is_const_int = nested.is_const_int;
      return true;
    }
    if (at(TokenKind::LBracket)) {
      return fail_here(error, "arrays are not supported in Rust subset v0");
    }
    if (at(TokenKind::Identifier)) {
      out.rendered = current().text;
      out.is_const_int = false;
      ++index_;
      return true;
    }
    return fail_here(error, "unsupported expression in Rust subset v0");
  }

  static int precedence(TokenKind kind) {
    switch (kind) {
      case TokenKind::PipePipe:
        return 1;
      case TokenKind::AmpAmp:
        return 2;
      case TokenKind::Pipe:
        return 3;
      case TokenKind::Caret:
        return 4;
      case TokenKind::Amp:
        return 5;
      case TokenKind::EqualEqual:
      case TokenKind::BangEqual:
        return 6;
      case TokenKind::Less:
      case TokenKind::LessEqual:
      case TokenKind::Greater:
      case TokenKind::GreaterEqual:
        return 7;
      case TokenKind::ShiftLeft:
      case TokenKind::ShiftRight:
        return 8;
      case TokenKind::Plus:
      case TokenKind::Minus:
        return 9;
      case TokenKind::Star:
      case TokenKind::Slash:
      case TokenKind::Percent:
        return 10;
      default:
        return -1;
    }
  }

  bool at(TokenKind kind) const {
    return current().kind == kind;
  }

  bool consume(TokenKind kind) {
    if (!at(kind)) {
      return false;
    }
    ++index_;
    return true;
  }

  bool expect(TokenKind kind, std::string_view message, std::string* error) {
    if (consume(kind)) {
      return true;
    }
    return fail_here(error, message);
  }

  bool expect(TokenKind kind, Token& out, std::string_view message, std::string* error) {
    if (!at(kind)) {
      return fail_here(error, message);
    }
    out = current();
    ++index_;
    return true;
  }

  bool fail_here(std::string* error, std::string_view message) const {
    return fail_at(current(), std::string(message), error);
  }

  bool fail_at(const Token& token, const std::string& message, std::string* error) const {
    std::ostringstream oss;
    oss << diag_name_ << ':' << token.line << ':' << token.column << ": " << message;
    return fail(error, oss.str());
  }

  const Token& current() const {
    if (index_ >= tokens_.size()) {
      return tokens_.back();
    }
    return tokens_[index_];
  }

  const Token& peek() const {
    if (index_ + 1 >= tokens_.size()) {
      return tokens_.back();
    }
    return tokens_[index_ + 1];
  }

  const std::vector<Token>& tokens_;
  std::string diag_name_;
  size_t index_{0};
  std::unordered_set<std::string> seen_functions_;
};

bool compile_via_c_adapter(const std::string& rust_source,
                           const std::string& diag_name,
                           std::string& output,
                           const CompileOptions& options,
                           std::string* error_message) {
  std::vector<Token> tokens;
  Lexer lexer(rust_source);
  if (!lexer.tokenize(tokens, diag_name, error_message)) {
    return false;
  }

  Parser parser(tokens, diag_name);
  std::string c_source;
  if (!parser.parse_program(c_source, error_message)) {
    return false;
  }

  t81::frontend_adapter::CompileOptions bridge_options;
  bridge_options.module_name = options.module_name;
  bridge_options.dcp_floats = options.dcp_floats;
  bridge_options.use_t81_dialect = options.use_t81_dialect;
  bridge_options.emit_comments = options.emit_comments;
  return t81::frontend_adapter::compile_normalized_c_to_mlir_text(
      c_source, diag_name, output, bridge_options, error_message);
}

}  // namespace

bool compile_source_to_mlir_text(const std::string& source,
                                 const std::string& diag_name,
                                 std::string& output,
                                 const CompileOptions& options,
                                 std::string* error_message) {
  return compile_via_c_adapter(source, diag_name, output, options, error_message);
}

bool compile_file_to_mlir(const std::filesystem::path& input,
                          const std::filesystem::path& output_path,
                          const CompileOptions& options,
                          std::string* error_message) {
  std::string source;
  if (!t81::frontend_adapter::read_text_file(input, source, error_message)) {
    return false;
  }
  std::string output;
  if (!compile_source_to_mlir_text(source, input.string(), output, options, error_message)) {
    return false;
  }
  return t81::frontend_adapter::write_text_file(output_path, output, error_message);
}

}  // namespace t81::rust_frontend
